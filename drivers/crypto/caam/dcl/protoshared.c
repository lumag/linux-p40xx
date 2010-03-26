/*
 * CAAM Descriptor Construction Library
 * Protocol-level Shared Descriptor Constructors
 *
 * Copyright (c) 2008, 2009, Freescale Semiconductor, Inc.
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../compat.h"
#include "dcl.h"

/**
 * Protocol-level shared descriptor constructors
 *
 * These build a full protocol-level shared descriptor for semi-autonomous
 * processing of secured traffic through CAAM. Such descriptors function
 * as single-pass processors (integrating cipher and authentication
 * functions into a single logical step) with the added factor of
 * performing protocol-level packet manipulation in the same step
 * in the packet-handling process, by maintaining protocol-level
 * connection state information within the descriptor itself.
 *
 * For each function, the arguments are uniform:
 *
 * Inputs:
 *
 *    * descbuf    = Points to a buffer to construct the descriptor in.
 *                   All CAAM descriptors are built of an array of up to
 *                   63 32-bit words. If the caller wishes to construct
 *                   a descriptor directly in the executable buffer, then
 *                   that buffer must be hardware DMA-able, and physically
 *                   contiguous.
 *
 *    * bufsize    = Points to an unsigned 16-bit word with the max length
 *                   of the buffer to hold the descriptor. This will be
 *                   written back to with the actual size of the descriptor
 *                   once constructed. (Note: bounds checking not yet
 *                   implemented).
 *
 *    * pdb        = Points to a block of data (struct pdbcont) used to
 *                   describe the content if the Protocol Data Block to be
 *                   maintained inside the descriptor. PDB content is
 *                   protocol and mode specific.
 *
 *    * cipherdata = Points to a block of data used to describe the cipher
 *                   information for encryption/decryption of packet
 *                   content:
 *                   - algtype = one of CIPHER_TYPE_IPSEC_xxx
 *                   - key     = pointer to the cipher key data
 *                   - keydata = size of the key data in bits
 *
 *    * authdata   = Points to a block of data used to describe the
 *                   authentication information for validating the
 *                   authenticity of the packet source.
 *                   - algtype = one of AUTH_TYPE_IPSEC_xxx
 *                   - key     = pointer to the HMAC key data
 *                   - keydata = size of the key data in bits
 *
 *    * clear      = If nonzero, buffer is cleared before writing
 *
 * Returns:
 *
 *    * -1 if the descriptor creation failed for any reason, zero
 *      if creation succeeded.
 *
 */

/*
 * IPSec ESP CBC decapsulation case:
 *
 * pdb.opthdrlen      = Size of inbound header to skip over.
 * pdb.transmode      = PDB_TUNNEL/PDB_TRANSPORT for tunnel or transport
 *                      handling for the next header.
 * pdb.pclvers        = PDB_IPV4/PDB_IPV6 as appropriate for this connection.
 * pdb.seq.esn        = PDB_NO_ESN unless extended sequence numbers are to
 *                      be supported, then PDB_INCLUDE_ESN.
 * pdb.seq/antirplysz = PDB_ANTIRPLY_NONE if no antireplay window is to be
 *                      maintained in the PDB. Otherwise may be
 *                      PDB_ANTIRPLY_32 for a 32-entry window, or
 *                      PDB_ANTIRPLY_64 for a 64-entry window.
 *
 **/
int32_t cnstr_pcl_shdsc_ipsec_cbc_decap(u_int32_t           *descbuf,
					u_int16_t           *bufsize,
					struct pdbcont      *pdb,
					struct cipherparams *cipherdata,
					struct authparams   *authdata,
					u_int8_t             clear)
{
	u_int32_t *start;
	u_int8_t   pdbopts;
	u_int16_t  startidx, endidx;

	start = descbuf++; /* save start for eventual header write */
			   /* bump to first word of PDB */

	/* first, got to have clean buffer */
	if (!descbuf)
		return -1;

	/* Prove other arguments */
	if (!authdata->key)
		return -3;

	if (!cipherdata->key)
		return -4;

	if (!authdata->keylen)
		return -5;

	if (!cipherdata->keylen)
		return -6;

	/* If user requested a buffer clear, do it from the start */
	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

	/* Build PDB, beginning with length and options */
	pdbopts = 0;

	if (pdb->transmode == PDB_TUNNEL)
		pdbopts |= PDBOPTS_ESPCBC_TUNNEL;

	if (pdb->pclvers == PDB_IPV6)
		pdbopts |= PDBOPTS_ESPCBC_IPVSN;

	if (pdb->outfmt == PDB_OUTPUT_DECAP_PDU)
		pdbopts |= PDBOPTS_ESPCBC_OUTFMT;

	if (pdb->seq.esn == PDB_INCLUDE_ESN)
		pdbopts |= PDBOPTS_ESPCBC_ESN;

	switch (pdb->seq.antirplysz) {
	case PDB_ANTIRPLY_32:
		pdbopts |= PDBOPTS_ESPCBC_ARS32;
		break;

	case PDB_ANTIRPLY_64:
		pdbopts |= PDBOPTS_ESPCBC_ARS64;
		break;

	case PDB_ANTIRPLY_NONE:
	default:
		pdbopts |= PDBOPTS_ESPCBC_ARSNONE;
		break;
	}

	*descbuf++ = (pdb->opthdrlen << 16) | pdbopts;

	/* Skip reserved section */
	descbuf += 2;

	/* Skip DECO writeback part */
	descbuf += 4;

	/* Save current location for computing start index */
	startidx = descbuf - start;

	/*
	 * PDB now complete
	 * Now process keys, starting with class 2/authentication
	 * This is assuming keys are immediate for sharedesc
	 */
	descbuf = cmd_insert_key(descbuf, authdata->key, authdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS2);

	/* Now the class 1/cipher key */
	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);


	/* Insert the operation command */
	descbuf = cmd_insert_proto_op_ipsec(descbuf,
					    cipherdata->algtype,
					    authdata->algtype,
					    DIR_DECAP);

	/* Now update the header with size/offsets */
	endidx = descbuf - start + 1; /* add 1 to include header */

	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_ALWAYS);

	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_pcl_shdsc_ipsec_cbc_decap);

/**
 * IPSec ESP CBC encapsulation case:
 *
 * pdbinfo.opthdrlen = Size of outbound IP header to be prepended to
 *                     output.
 * pdbinfo.opthdr    = Pointer to the IP header to be prepended to the
 *                     output, of size opthdrlen.
 * pdbinfo.transmode = PDB_TUNNEL/PDB_TRANSPORT for tunnel/transport
 *                     handling for the next header.
 * pdbinfo.pclvers   = PDB_IPV4/PDB_IPV6 as appropriate for this connection.
 * pdbinfo.seq.esn   = PDB_NO_ESN unless extended sequence numbers are to
 *                     be supported, then PDB_INCLUDE_ESN.
 * pdbinfo.ivsrc     = PDB_IV_FROM_PDB if the IV is to be maintained in
 *                     the PDB, else PDB_IV_FROM_RNG if the IV is to
 *                     be generated internally by CAAM's random number
 *                     generator.
 *
 **/
int32_t cnstr_pcl_shdsc_ipsec_cbc_encap(u_int32_t           *descbuf,
					u_int16_t           *bufsize,
					struct pdbcont      *pdb,
					struct cipherparams *cipherdata,
					struct authparams   *authdata,
					u_int8_t             clear)
{
	u_int32_t *start;
	u_int8_t   pdbopts;
	u_int16_t  hdrwds, startidx, endidx;
	u_int32_t *hdrbuf;

	start = descbuf++; /* save start for eventual header write */
			   /* bump to first word of PDB */

	/* Verify a clean buffer */
	if (!descbuf)
		return -1;

	/* Verify other pertinent arguments */
	if (!authdata->key)
		return -3;

	if (!cipherdata->key)
		return -4;

	if (!authdata->keylen)
		return -5;

	if (!cipherdata->keylen)
		return -6;

	/* If user requested a buffer clear, do it from the start */
	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

	/* Construct PDB, starting with options word */
	pdbopts = 0;

	if (pdb->transmode == PDB_TUNNEL)
		pdbopts |= PDBOPTS_ESPCBC_TUNNEL;

	if (pdb->opthdrlen)
		pdbopts |= PDBOPTS_ESPCBC_INCIPHDR | PDBOPTS_ESPCBC_IPHDRSRC;

	if (pdb->pclvers == PDB_IPV6)
		pdbopts |= PDBOPTS_ESPCBC_IPVSN;

	if (pdb->seq.esn == PDB_INCLUDE_ESN)
		pdbopts |= PDBOPTS_ESPCBC_ESN;

	if (pdb->ivsrc == PDB_IV_FROM_RNG)
		pdbopts |= PDBOPTS_ESPCBC_IVSRC;

	*descbuf++ = pdbopts;

	/* Skip sequence numbers */
	descbuf += 2;

	/* Skip IV */
	descbuf += 4;

	/* Skip SPI */
	descbuf++;

		*descbuf++ = pdb->opthdrlen;
	/* If PDB-resident IPHeader, set size and append it */
	if (pdb->opthdr && pdb->opthdrlen) {
		hdrwds = pdb->opthdrlen >> 2;
		hdrbuf = (u_int32_t *)pdb->opthdr;
		while (hdrwds) {
			*descbuf++ = *hdrbuf++;
			hdrwds--;
		}
	}

	/* Save current location for computing start index */
	startidx = descbuf - start;

	/*
	 * Now process keys, starting with class 2/authentication
	 * This is assuming keys are immediate for sharedesc
	 */
	descbuf = cmd_insert_key(descbuf, authdata->key, authdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS2);

	/* Now the class 1/cipher key */
	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);

	/* Now insert the operation command */
	descbuf = cmd_insert_proto_op_ipsec(descbuf,
					    cipherdata->algtype,
					    authdata->algtype,
					    DIR_ENCAP);

	/*
	 * Now update the header with size/offsets
	 * add 1 to include header
	 */
	endidx = descbuf - start + 1;

	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_ALWAYS);

	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_pcl_shdsc_ipsec_cbc_encap);

/**
 * Construct IPSec ESP encapsulation protocol-level sharedesc
 * Requires a covered MDHA splitkey.
 *
 * Inputs:
 * @descbuf - pointer to buffer used for descriptor construction
 *
 * @bufsize - pointer to size to be written back upon completion
 *
 * @pdb - pointer to the PDB to be used with this descriptor. This
 *       structure will be copied inline to the descriptor under
 *       construction. No error checking will be made. Refer to the
 *       block guide for a detailed discussion of the decapsulation
 *       PDB, and it's unioned sub structure that is cipher-dependent
 *
 * @opthdr - Optional header to be prepended to an encapsulated frame.
 *           Size of the optional header is defined in pdb.opt_hdr_len
 *
 * @cipherdata - Pointer to blockcipher transform definitions.
 *
 * @authdata - Pointer to authentication transform definitions. Note
 *             that since a split key is to be used, the size of the
 *             split key itself is specified (even though the buffer
 *             containing the key may have additional padding depending
 *             on the size).
 **/
int32_t cnstr_shdsc_ipsec_encap(u_int32_t *descbuf, u_int16_t *bufsize,
				    struct ipsec_encap_pdb *pdb,
				    u_int8_t *opthdr,
				    struct cipherparams *cipherdata,
				    struct authparams *authdata)
{
	u_int32_t *start, *keyjump, opthdrsz;
	u_int16_t startidx, endidx;

	start = descbuf;

	/* Compute optional header size, rounded up to descriptor word size */
	opthdrsz = (pdb->ip_hdr_len + 3) & ~3;

	/* copy in core of PDB */
	memcpy(descbuf, pdb, sizeof(struct ipsec_encap_pdb));
	descbuf += sizeof(struct ipsec_encap_pdb) >> 2;

	/*
	 * If optional header, compute optional header size
	 * rounded up to descriptor word size, and copy in
	 */
	if (pdb->ip_hdr_len) {
		opthdrsz = (pdb->ip_hdr_len + 3) & ~3;
		memcpy(descbuf, opthdr, opthdrsz);
		descbuf += opthdrsz >> 2;
	}

	startidx = descbuf - start;

	/*
	 * Insert an empty instruction for a shared-JUMP past the keys
	 * Update later, once the size of the key block is known
	 */
	keyjump = descbuf++;

	/* Insert keys */
	descbuf = cmd_insert_key(descbuf, authdata->key, authdata->keylen,
				 PTR_DIRECT, KEYDST_MD_SPLIT, KEY_COVERED,
				 ITEM_REFERENCE, ITEM_CLASS2);

	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_REFERENCE, ITEM_CLASS1);

	/*
	 * Key jump can now be written (now that we know the size of the
	 * key block). This can now happen anytime before the final
	 * sizes are computed.
	 */
	cmd_insert_jump(keyjump, JUMP_TYPE_LOCAL, CLASS_BOTH, JUMP_TEST_ALL,
			JUMP_COND_SHRD | JUMP_COND_SELF, descbuf - keyjump,
			NULL);

	descbuf = cmd_insert_proto_op_ipsec(descbuf,
					    cipherdata->algtype,
					    authdata->algtype,
					    DIR_ENCAP);

	endidx = descbuf - start + 1;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_SAVE, SHR_SERIAL);
	*bufsize = endidx;
	return 0;
};
EXPORT_SYMBOL(cnstr_shdsc_ipsec_encap);

/**
 * Construct IPSec ESP decapsulation protocol-level sharedesc
 * Requires a covered MDHA splitkey.
 *
 * Inputs:
 * @descbuf - pointer to buffer used for descriptor construction
 *
 * @bufsize - pointer to size to be written back upon completion
 *
 * @pdb - pointer to the PDB to be used with this descriptor. This
 *       structure will be copied inline to the descriptor under
 *       construction. No error checking will be made. Refer to the
 *       block guide for a detailed discussion of the decapsulation
 *       PDB, and it's unioned sub structure that is cipher-dependent
 *
 * @cipherdata - Pointer to blockcipher transform definitions.
 *
 * @authdata - Pointer to authentication transform definitions. Note
 *             that since a split key is to be used, the size of the
 *             split key itself is specified (even though the buffer
 *             containing the key may have additional padding depending
 *             on the size).
 **/
int32_t cnstr_shdsc_ipsec_decap(u_int32_t *descbuf, u_int16_t *bufsize,
				struct ipsec_decap_pdb *pdb,
				struct cipherparams *cipherdata,
				struct authparams *authdata)
{
	u_int32_t *start, *keyjump;
	u_int16_t startidx, endidx;

	start = descbuf;

	/* copy in PDB */
	memcpy(descbuf, pdb, sizeof(struct ipsec_decap_pdb));
	descbuf += sizeof(struct ipsec_decap_pdb) >> 2;

	startidx = descbuf - start;

	/*
	 * Insert an empty instruction for a shared-JUMP past the keys
	 * Update later, once the size of the key block is known
	 */
	keyjump = descbuf++;

	/* Insert keys */
	descbuf = cmd_insert_key(descbuf, authdata->key, authdata->keylen,
				 PTR_DIRECT, KEYDST_MD_SPLIT, KEY_COVERED,
				 ITEM_REFERENCE, ITEM_CLASS2);

	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_REFERENCE, ITEM_CLASS1);

	/*
	 * Key jump comes here (now that we know the size of the key block.
	 * Update before we insert the OP
	 */
	cmd_insert_jump(keyjump, JUMP_TYPE_LOCAL, CLASS_BOTH, JUMP_TEST_ALL,
			JUMP_COND_SHRD | JUMP_COND_SELF, descbuf - keyjump,
			NULL);

	descbuf = cmd_insert_proto_op_ipsec(descbuf,
					    cipherdata->algtype,
					    authdata->algtype,
					    DIR_DECAP);

	endidx = descbuf - start + 1;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_SAVE, SHR_SERIAL);
	*bufsize = endidx;
	return 0;
};
EXPORT_SYMBOL(cnstr_shdsc_ipsec_decap);

/**
 * IEEE 802.11i WiFi encapsulation
 **/
int32_t cnstr_shdsc_wifi_encap(u_int32_t *descbuf, u_int16_t *bufsize,
				struct wifi_encap_pdb *pdb,
				struct cipherparams *cipherdata)
{
	u_int32_t *start, *keyjump;
	u_int16_t startidx, endidx;

	start = descbuf;

	memcpy(descbuf, pdb, sizeof(struct wifi_encap_pdb));
	descbuf += sizeof(struct wifi_encap_pdb) >> 2;

	startidx = descbuf - start;
	keyjump = descbuf++;
	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);

	cmd_insert_jump(keyjump, JUMP_TYPE_LOCAL, CLASS_BOTH, JUMP_TEST_ALL,
			JUMP_COND_SHRD | JUMP_COND_SELF, descbuf - keyjump,
			NULL);

	descbuf = cmd_insert_proto_op_wifi(descbuf, DIR_ENCAP);

	endidx = descbuf - start + 1;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_SAVE, SHR_SERIAL);
	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_shdsc_wifi_encap);

/**
 * IEEE 802.11 WiFi decapsulation
 **/
int32_t cnstr_shdsc_wifi_decap(u_int32_t *descbuf, u_int16_t *bufsize,
				struct wifi_decap_pdb *pdb,
				struct cipherparams *cipherdata)
{
	u_int32_t *start, *keyjump;
	u_int16_t startidx, endidx;

	start = descbuf;

	memcpy(descbuf, pdb, sizeof(struct wifi_decap_pdb));
	descbuf += sizeof(struct wifi_decap_pdb) >> 2;

	startidx = descbuf - start;
	keyjump = descbuf++;
	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);

	cmd_insert_jump(keyjump, JUMP_TYPE_LOCAL, CLASS_BOTH, JUMP_TEST_ALL,
			JUMP_COND_SHRD | JUMP_COND_SELF, descbuf - keyjump,
			NULL);

	descbuf = cmd_insert_proto_op_wifi(descbuf, DIR_DECAP);

	endidx = descbuf - start + 1;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_SAVE, SHR_SERIAL);
	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_shdsc_wifi_decap);

/**
 * 802.16 WiMAX encapsulation
 *
 * @descbuf - Pointer to descriptor build buffer
 *
 * @bufsize - Pointer to value to write size of built descriptor to
 *
 * @pdb - pointer to pre-initialized PDB
 *
 * @cipherdata.keylen - key size in bytes
 * @cipherdata.key - key data to inline
 *
 * @mode - nonzero for OFDMa, else OFDM
 *
 * @clear - nonzero clears descriptor buffer
 *
 **/
int32_t cnstr_shdsc_wimax_encap(u_int32_t *descbuf, u_int16_t *bufsize,
				struct wimax_encap_pdb *pdb,
				struct cipherparams *cipherdata,
				u_int8_t mode)
{
	u_int32_t *start, *keyjump;
	u_int16_t startidx, endidx;

	start = descbuf;

	memcpy(descbuf, pdb, sizeof(struct wimax_encap_pdb));
	descbuf += sizeof(struct wimax_encap_pdb) >> 2;

	startidx = descbuf - start;
	keyjump = descbuf++;
	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);

	cmd_insert_jump(keyjump, JUMP_TYPE_LOCAL, CLASS_BOTH, JUMP_TEST_ALL,
			JUMP_COND_SHRD | JUMP_COND_SELF, descbuf - keyjump,
			NULL);

	descbuf = cmd_insert_proto_op_wimax(descbuf, mode, DIR_ENCAP);

	/* Now compute shared header */
	endidx = descbuf - start + 1;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_SAVE, SHR_SERIAL);
	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_shdsc_wimax_encap);

/**
 * 802.16 WiMAX decapsulation
 *
 * @descbuf - Pointer to descriptor build buffer
 *
 * @bufsize - Pointer to value to write size of built descriptor to
 *
 * @pdb - pointer to pre-initialized PDB
 *
 * @cipherdata.keylen - key size in bytes
 * @cipherdata.key - key data to inline
 *
 * @mode - nonzero for OFDMa, else OFDM
 *
 * @clear - nonzero clears descriptor buffer
 **/
int32_t cnstr_shdsc_wimax_decap(u_int32_t *descbuf, u_int16_t *bufsize,
				struct wimax_decap_pdb *pdb,
				struct cipherparams *cipherdata,
				u_int8_t mode)
{
	u_int32_t *start, *keyjump;
	u_int16_t startidx, endidx;

	start = descbuf;

	memcpy(descbuf, pdb, sizeof(struct wimax_decap_pdb));
	descbuf += sizeof(struct wimax_decap_pdb) >> 2;

	startidx = descbuf - start;
	keyjump = descbuf++;
	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);

	cmd_insert_jump(keyjump, JUMP_TYPE_LOCAL, CLASS_BOTH, JUMP_TEST_ALL,
			JUMP_COND_SHRD | JUMP_COND_SELF, descbuf - keyjump,
			NULL);

	descbuf = cmd_insert_proto_op_wimax(descbuf, mode, DIR_DECAP);

	/* Now compute shared header */
	endidx = descbuf - start + 1;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_SAVE, SHR_SERIAL);
	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_shdsc_wimax_decap);

/**
 * 802.1AE MacSec encapsulation
 *
 * @descbuf - Pointer to descriptor build buffer
 *
 * @bufsize - Pointer to value to write size of built descriptor to
 *
 * @pdb - pointer to pre-initialized PDB
 *
 * @cipherdata.keylen - key size in bytes
 * @cipherdata.key - key data to inline
 *
 * @clear - nonzero clears descriptor buffer
 **/

int32_t cnstr_shdsc_macsec_encap(u_int32_t *descbuf, u_int16_t *bufsize,
				 struct macsec_encap_pdb *pdb,
				 struct cipherparams *cipherdata)
{
	u_int32_t *start, *keyjump;
	u_int16_t startidx, endidx;

	start = descbuf;

	memcpy(descbuf, pdb, sizeof(struct macsec_encap_pdb));
	descbuf += sizeof(struct macsec_encap_pdb) >> 2;

	startidx = descbuf - start;
	keyjump = descbuf++;
	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);

	cmd_insert_jump(keyjump, JUMP_TYPE_LOCAL, CLASS_BOTH, JUMP_TEST_ALL,
			JUMP_COND_SHRD | JUMP_COND_SELF, descbuf - keyjump,
			NULL);

	descbuf = cmd_insert_proto_op_macsec(descbuf, DIR_ENCAP);

	endidx = descbuf - start + 1;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_SAVE, SHR_SERIAL);
	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_shdsc_macsec_encap);

/**
 * 802.1AE MacSec decapsulation
 *
 * @descbuf - Pointer to descriptor build buffer
 *
 * @bufsize - Pointer to value to write size of built descriptor to
 *
 * @pdb - pointer to pre-initialized PDB
 *
 * @cipherdata.keylen - key size in bytes
 * @cipherdata.key - key data to inline
 *
 * @clear - nonzero clears descriptor buffer
 **/
int32_t cnstr_shdsc_macsec_decap(u_int32_t *descbuf, u_int16_t *bufsize,
				 struct macsec_decap_pdb *pdb,
				 struct cipherparams *cipherdata)
{
	u_int32_t *start, *keyjump;
	u_int16_t startidx, endidx;

	start = descbuf;

	memcpy(descbuf, pdb, sizeof(struct macsec_decap_pdb));
	descbuf += sizeof(struct macsec_decap_pdb) >> 2;

	startidx = descbuf - start;
	keyjump = descbuf++;
	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);

	cmd_insert_jump(keyjump, JUMP_TYPE_LOCAL, CLASS_BOTH, JUMP_TEST_ALL,
			JUMP_COND_SHRD | JUMP_COND_SELF, descbuf - keyjump,
			NULL);

	descbuf = cmd_insert_proto_op_macsec(descbuf, DIR_DECAP);

	endidx = descbuf - start + 1;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_SAVE, SHR_SERIAL);
	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_shdsc_macsec_decap);

/**
 * SNOW/f8 (UEA2) as a shared descriptor
 *
 * @descbuf - pointer to descriptor-under-construction buffer
 * @bufsize - points to size to be updated at completion
 * @key - cipher key
 * @keylen - size of key in bits
 * @dir - cipher direction (DIR_ENCRYPT/DIR_DECRYPT)
 * @count - UEA2 count value (32 bits)
 * @bearer - UEA2 bearer ID (5 bits)
 * @direction - UEA2 direction (1 bit)
 * @clear - nonzero if descriptor buffer clear requested
 **/
int32_t cnstr_shdsc_snow_f8(u_int32_t *descbuf, u_int16_t *bufsize,
			    u_int8_t *key, u_int32_t keylen,
			    enum algdir dir, u_int32_t count,
			    u_int8_t bearer, u_int8_t direction,
			    u_int8_t clear)
{
	u_int32_t *start;
	u_int16_t startidx, endidx;
	u_int32_t mval;

	u_int64_t COUNT       = count;
	u_int64_t BEARER      = bearer;
	u_int64_t DIRECTION   = direction;

	u_int64_t context = (COUNT << 32) | (BEARER << 27) | (DIRECTION << 26);

	start = descbuf++; /* save start for eventual header write */

	/* Verify a clean buffer */
	if (!descbuf)
		return -1;

	/* If user requested a buffer clear, do it from the start */
	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

	/* Save current location for computing start index */
	startidx = descbuf - start;

	descbuf = cmd_insert_key(descbuf, key, keylen, PTR_DIRECT,
				 KEYDST_KEYREG, KEY_CLEAR, ITEM_INLINE,
				 ITEM_CLASS1);

	mval = 0;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_SUB, MATH_SRC0_SEQINLEN,
				  MATH_SRC1_REG2, MATH_DEST_VARSEQINLEN,
				  4, 0, 0, 0, &mval);

	descbuf = cmd_insert_math(descbuf, MATH_FUN_SUB, MATH_SRC0_SEQINLEN,
				  MATH_SRC1_REG2, MATH_DEST_VARSEQOUTLEN,
				  4, 0, 0, 0, &mval);

	descbuf = cmd_insert_alg_op(descbuf, OP_TYPE_CLASS1_ALG,
				    OP_ALG_ALGSEL_SNOW, OP_ALG_AAI_F8,
				    MDSTATE_COMPLETE, ICV_CHECK_OFF, dir);

	descbuf = cmd_insert_load(descbuf, &context, LDST_CLASS_1_CCB,
				  0, LDST_SRCDST_BYTE_CONTEXT, 0, 8,
				  ITEM_INLINE);

	descbuf = cmd_insert_seq_fifo_load(descbuf, LDST_CLASS_1_CCB,
					   FIFOLDST_VLF,
					   (FIFOLD_TYPE_MSG |
					   FIFOLD_TYPE_LASTBOTH), 32);

	descbuf = cmd_insert_seq_fifo_store(descbuf, LDST_CLASS_1_CCB,
					    FIFOLDST_VLF,
					    FIFOST_TYPE_MESSAGE_DATA, 32);

	/* Now update the header with size/offsets */
	endidx = descbuf - start;

	cmd_insert_shared_hdr(start, startidx, endidx, CTX_SAVE, SHR_ALWAYS);

	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_shdsc_snow_f8);

/**
 * SNOW/f9 (UIA2) as a shared descriptor
 *
 * @descbuf - pointer to descriptor-under-construction buffer
 * @bufsize - points to size to be updated at completion
 * @key - cipher key
 * @keylen - size of key in bits
 * @dir - cipher direction (DIR_ENCRYPT/DIR_DECRYPT)
 * @count - UEA2 count value (32 bits)
 * @fresh - UEA2 fresh value ID (32 bits)
 * @direction - UEA2 direction (1 bit)
 * @clear - nonzero if descriptor buffer clear requested
 **/
int32_t cnstr_shdsc_snow_f9(u_int32_t *descbuf, u_int16_t *bufsize,
			    u_int8_t *key, u_int32_t keylen,
			    enum algdir dir, u_int32_t count,
			    u_int32_t fresh, u_int8_t direction,
			    u_int8_t clear)
{
	u_int32_t *start;
	u_int16_t startidx, endidx;
	u_int32_t mval;

	u_int64_t ct = count;
	u_int64_t fr = fresh;
	u_int64_t dr = direction;

	u_int64_t context[2];

	context[0] = (ct << 32) | (dr << 26);
	context[1] = fr;

	start = descbuf++; /* header skip */

	if (!descbuf)
		return -1;

	/* buffer clear */
	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

	startidx = descbuf - start;

	descbuf = cmd_insert_key(descbuf, key, keylen, PTR_DIRECT,
				 KEYDST_KEYREG, KEY_CLEAR, ITEM_INLINE,
				 ITEM_CLASS1);

	/* compute sequences */
	mval = 0;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_SUB, MATH_SRC0_SEQINLEN,
				  MATH_SRC1_REG2, MATH_DEST_VARSEQINLEN,
				  4, 0, 0, 0, &mval);

	descbuf = cmd_insert_alg_op(descbuf, OP_TYPE_CLASS1_ALG,
				    OP_ALG_ALGSEL_SNOW, OP_ALG_AAI_F9,
				    MDSTATE_COMPLETE, ICV_CHECK_OFF, dir);

	descbuf = cmd_insert_load(descbuf, &context, LDST_CLASS_1_CCB,
				  0, LDST_SRCDST_BYTE_CONTEXT, 0, 16,
				  ITEM_INLINE);

	descbuf = cmd_insert_seq_fifo_load(descbuf, LDST_CLASS_1_CCB,
					   FIFOLDST_VLF,
					   (FIFOLD_TYPE_MSG |
					   FIFOLD_TYPE_LASTBOTH), 0);

	/* Save lower half of MAC out into a 32-bit sequence */
	descbuf = cmd_insert_seq_store(descbuf, LDST_CLASS_1_CCB, 0,
				       LDST_SRCDST_BYTE_CONTEXT, 4, 4);

	endidx = descbuf - start;

	cmd_insert_shared_hdr(start, startidx, endidx, CTX_SAVE, SHR_ALWAYS);

	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_shdsc_snow_f9);

/**
 * CBC blockcipher
 * @descbuf - descriptor buffer
 * @bufsize - limit/returned descriptor buffer size
 * @key     - key data to inline
 * @keylen  - key length
 * @iv      - iv data
 * @ivsize  - iv length
 * @dir     - DIR_ENCRYPT/DIR_DECRYPT
 * @cipher  - OP_ALG_ALGSEL_AES/DES/3DES
 * @clear   - clear buffer before writing
 **/
int32_t cnstr_shdsc_cbc_blkcipher(u_int32_t *descbuf, u_int16_t *bufsize,
				  u_int8_t *key, u_int32_t keylen,
				  u_int8_t *iv, u_int32_t ivlen,
				  enum algdir dir, u_int32_t cipher,
				  u_int8_t clear)
{
	u_int32_t *start;
	u_int16_t startidx, endidx;
	u_int32_t mval;

	start = descbuf++; /* save start for eventual header write */

	if (!descbuf)
		return -1;

	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

	startidx = descbuf - start;

	/* Insert Key */
	descbuf = cmd_insert_key(descbuf, key, keylen, PTR_DIRECT,
				 KEYDST_KEYREG, KEY_CLEAR, ITEM_INLINE,
				 ITEM_CLASS1);

	/* Compute variable sequence */
	mval = 0;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_SUB, MATH_SRC0_SEQINLEN,
				  MATH_SRC1_REG2, MATH_DEST_VARSEQINLEN,
				  4, 0, 0, 0, &mval);

	descbuf = cmd_insert_math(descbuf, MATH_FUN_SUB, MATH_SRC0_SEQINLEN,
				  MATH_SRC1_REG2, MATH_DEST_VARSEQOUTLEN,
				  4, 0, 0, 0, &mval);

	/* Set cipher, AES/DES/3DES */
	descbuf = cmd_insert_alg_op(descbuf, OP_TYPE_CLASS1_ALG, cipher,
				    OP_ALG_AAI_CBC, MDSTATE_INIT,
				    ICV_CHECK_OFF, dir);

	/* IV load, convert size */
	descbuf = cmd_insert_load(descbuf, iv, LDST_CLASS_1_CCB,
				  0, LDST_SRCDST_BYTE_CONTEXT, 0, (ivlen >> 3),
				  ITEM_INLINE);

	/* Insert sequence load/store with VLF */
	descbuf = cmd_insert_seq_fifo_load(descbuf, LDST_CLASS_1_CCB,
					   FIFOLDST_VLF,
					   (FIFOLD_TYPE_MSG |
					   FIFOLD_TYPE_LASTBOTH), 32);

	descbuf = cmd_insert_seq_fifo_store(descbuf, LDST_CLASS_1_CCB,
					    FIFOLDST_VLF,
					    FIFOST_TYPE_MESSAGE_DATA, 32);

	/* Now update the header with size/offsets */
	endidx = descbuf - start;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_SAVE, SHR_ALWAYS);

	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_shdsc_cbc_blkcipher);

/**
 * HMAC shared
 * @descbuf - descriptor buffer
 * @bufsize - limit/returned descriptor buffer size
 * @key     - key data to inline (length based on cipher)
 * @cipher  - OP_ALG_ALGSEL_MD5/SHA1-512
 * @icv     - HMAC comparison for ICV, NULL if no check desired
 * @clear   - clear buffer before writing
 **/
int32_t cnstr_shdsc_hmac(u_int32_t *descbuf, u_int16_t *bufsize,
			 u_int8_t *key, u_int32_t cipher, u_int8_t *icv,
			 u_int8_t clear)
{
	u_int32_t *start;
	u_int16_t startidx, endidx;
	u_int32_t mval;
	u_int8_t storelen;
	enum icvsel opicv;

	start = descbuf++;

	if (!descbuf)
		return -1;

	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

	/* Compute fixed-size store based on alg selection */
	switch (cipher) {
	case OP_ALG_ALGSEL_MD5:
		storelen = 16;
		break;

	case OP_ALG_ALGSEL_SHA1:
		storelen = 20;
		break;

	case OP_ALG_ALGSEL_SHA224:
		storelen = 28;
		break;

	case OP_ALG_ALGSEL_SHA256:
		storelen = 32;
		break;

	case OP_ALG_ALGSEL_SHA384:
		storelen = 48;

	case OP_ALG_ALGSEL_SHA512:
		storelen = 64;

	default:
		return -1;
	}

	if (icv != NULL)
		opicv = ICV_CHECK_ON;
	else
		opicv = ICV_CHECK_OFF;

	startidx = descbuf - start;

	descbuf = cmd_insert_key(descbuf, key, storelen * 8, PTR_DIRECT,
				 KEYDST_KEYREG, KEY_CLEAR, ITEM_INLINE,
				 ITEM_CLASS2);

	/* compute sequences */
	mval = 0;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_SUB, MATH_SRC0_SEQINLEN,
				  MATH_SRC1_REG2, MATH_DEST_VARSEQINLEN,
				  4, 0, 0, 0, &mval);

	descbuf = cmd_insert_math(descbuf, MATH_FUN_SUB, MATH_SRC0_SEQINLEN,
				  MATH_SRC1_REG2, MATH_DEST_VARSEQOUTLEN,
				  4, 0, 0, 0, &mval);

	/* Do operation */
	descbuf = cmd_insert_alg_op(descbuf, OP_TYPE_CLASS2_ALG, cipher,
				    OP_ALG_AAI_HMAC, MDSTATE_COMPLETE,
				    opicv, DIR_ENCRYPT);

	/* Do load (variable length) */
	descbuf = cmd_insert_seq_fifo_load(descbuf, LDST_CLASS_2_CCB,
					   FIFOLDST_VLF,
					   (FIFOLD_TYPE_MSG |
					   FIFOLD_TYPE_LASTBOTH), 32);

	descbuf = cmd_insert_seq_store(descbuf, LDST_CLASS_2_CCB, 0,
				       LDST_SRCDST_BYTE_CONTEXT, 0, storelen);

	endidx = descbuf - start;

	cmd_insert_shared_hdr(start, startidx, endidx, CTX_SAVE, SHR_ALWAYS);

	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_shdsc_hmac);

/**
 * 3GPP RLC decapsulation - cnstr_pcl_shdsc_3gpp_rlc_decap()
 *
 * @descbuf - pointer to buffer for descriptor construction
 * @bufsize - size of descriptor written
 * @key - f8 cipher key
 * @keysz - size of cipher key
 * @count - f8 count value
 * @bearer - f8 bearer value
 * @direction - f8 direction value
 * @payload_sz - size of payload (this is not using VLF)
 * @clear - clear descriptor buffer before construction
 **/
int32_t cnstr_pcl_shdsc_3gpp_rlc_decap(u_int32_t *descbuf, u_int16_t *bufsize,
				       u_int8_t *key, u_int32_t keysz,
				       u_int32_t count, u_int32_t bearer,
				       u_int32_t direction,
				       u_int16_t payload_sz, u_int8_t clear)
{
	u_int32_t *start;
	u_int16_t startidx, endidx;
	u_int32_t kctx[2];
	u_int32_t ififo_cmd;
	u_int32_t tmp_imm;
	u_int64_t rlc_segnum_mask = 0x00000fff00000000ull;

	start = descbuf++;

	if (!descbuf)
		return -1;

	/* build count/bearer/direction as register-loadable words */
	kctx[0] = count;
	kctx[1] = ((bearer & 0x1f) << 27) | ((direction & 0x01) << 26);

	startidx = descbuf - start;

	/* Load the cipher key */
	descbuf = cmd_insert_key(descbuf, key, keysz, PTR_DIRECT,
				 KEYDST_KEYREG, KEY_CLEAR, ITEM_REFERENCE,
				 ITEM_CLASS1);

	/* Shut off automatic Info FIFO entries */
	descbuf = cmd_insert_load(descbuf, NULL, LDST_CLASS_DECO, 0,
				  LDST_SRCDST_WORD_DECOCTRL, 0x08, 0x00,
				  ITEM_INLINE);

	/* Read encrypted 83-byte RLC PDU w/header */
	descbuf = cmd_insert_seq_fifo_load(descbuf, LDST_CLASS_1_CCB, 0,
					   FIFOLD_TYPE_MSG | FIFOLD_TYPE_LAST1,
					   83);

	/* Stuff info-fifo with 3-byte then payload-size flush */
	ififo_cmd = 0x04F00003; /* FIXME: bitdefs in desc.h would be better */
	descbuf = cmd_insert_load(descbuf, &ififo_cmd,
				  LDST_CLASS_IND_CCB, 0,
				  LDST_SRCDST_WORD_INFO_FIFO, 0, 4,
				  ITEM_INLINE);

	ififo_cmd = 0x04f00000 | payload_sz;
	descbuf = cmd_insert_load(descbuf, &ififo_cmd,
				  LDST_CLASS_IND_CCB, 0,
				  LDST_SRCDST_WORD_INFO_FIFO, 0, 4,
				  ITEM_INLINE);

	/* Now load prebuilt cipher context for KFHA */
	descbuf = cmd_insert_load(descbuf, kctx, LDST_CLASS_DECO, 0,
				  LDST_SRCDST_WORD_DECO_MATH0, 0, 8,
				  ITEM_INLINE);

	/* Get PDU header, left align, and store */
	tmp_imm = 0;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_ADD, MATH_SRC0_IMM,
				  MATH_SRC1_INFIFO, MATH_DEST_REG1, 8, 0, 0,
				  MATH_IFB, &tmp_imm);

	tmp_imm = 4;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_LSHIFT, MATH_SRC0_REG1,
				  MATH_SRC1_IMM, MATH_DEST_REG1, 8, 0, 0,
				  MATH_IFB, &tmp_imm);

	descbuf = cmd_insert_move(descbuf, 0, MOVE_SRC_MATH1,
				  MOVE_DEST_OUTFIFO, 0, 2);

	descbuf = cmd_insert_seq_fifo_store(descbuf, LDST_CLASS_IND_CCB, 0,
					    FIFOST_TYPE_MESSAGE_DATA, 2);

	/* wait for DMA CALM before proceeding */
	descbuf = cmd_insert_jump(descbuf, CLASS_NONE, JUMP_TYPE_LOCAL,
				  JUMP_TEST_ALL, JUMP_COND_CALM, 1, NULL);

	/* reset output FIFO */
	descbuf = cmd_insert_load(descbuf, NULL, LDST_CLASS_DECO, 0,
				  LDST_SRCDST_WORD_DECOCTRL, 0, 32,
				  ITEM_INLINE);

	/* Align sequence number with PPDB, add it, then use as c1 context */
	tmp_imm = 19;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_RSHIFT, MATH_SRC0_REG1,
				  MATH_SRC1_IMM, MATH_DEST_REG2, 8, 0, 0,
				  MATH_IFB, &tmp_imm);

	descbuf = cmd_insert_math(descbuf, MATH_FUN_AND, MATH_SRC0_REG2,
				  MATH_SRC1_IMM, MATH_DEST_REG2, 8, 0, 0, 0,
				  (u_int32_t *)&rlc_segnum_mask);

	descbuf = cmd_insert_math(descbuf, MATH_FUN_ADD, MATH_SRC0_REG0,
				  MATH_SRC1_REG2, MATH_DEST_REG0, 8, 0, 0,
				  0, NULL);

	descbuf = cmd_insert_move(descbuf, 0, MOVE_SRC_MATH0,
				  MOVE_DEST_CLASS1CTX, 0, 8);

	/* shift sequence number left 4 */
	tmp_imm = 16;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_LSHIFT, MATH_SRC0_REG1,
				  MATH_SRC1_IMM, MATH_DEST_REG1, 8, 0, 0,
				  MATH_IFB, &tmp_imm);

	tmp_imm = payload_sz; /* convert to 32-bit immediate */
	descbuf = cmd_insert_load(descbuf, &tmp_imm, LDST_CLASS_DECO,
				  0, LDST_SRCDST_WORD_DECO_MATH3, 0, 4,
				  ITEM_INLINE);

	/*
	 * Top of payload loop. Process 8 bytes of payload,
	 * shift right to M0
	 */
	tmp_imm = 0;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_ADD, MATH_SRC0_IMM,
				  MATH_SRC1_INFIFO, MATH_DEST_REG2,
				  8, 0, 0, MATH_IFB, &tmp_imm);

	tmp_imm = 4;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_RSHIFT, MATH_SRC0_REG2,
				  MATH_SRC1_IMM, MATH_DEST_REG0,
				  8, 0, 0, MATH_IFB, &tmp_imm);

	/* 64 bits to in fifo */
	descbuf = cmd_insert_math(descbuf, MATH_FUN_OR, MATH_SRC0_REG0,
				  MATH_SRC1_REG1, MATH_DEST_REG1, 8, 0, 0, 0,
				  NULL);

	descbuf = cmd_insert_move(descbuf, 0, MOVE_SRC_MATH1,
				  MOVE_DEST_CLASS1INFIFO, 0, 8);

	tmp_imm = 60;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_LSHIFT, MATH_SRC0_REG2,
				  MATH_SRC1_IMM, MATH_DEST_REG1, 8, 0, 0,
				  MATH_IFB, &tmp_imm);

	/* M3 = data processed */
	tmp_imm = 8;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_SUB, MATH_SRC0_REG3,
				  MATH_SRC1_IMM, MATH_DEST_REG3, 8, 0, 0,
				  MATH_IFB, &tmp_imm);

	/* end of 8 byte processing loop, go back to top if more  */
	descbuf = cmd_insert_jump(descbuf, CLASS_NONE, JUMP_TYPE_LOCAL,
				  JUMP_TEST_INVALL,
				  JUMP_COND_MATH_NV | JUMP_COND_MATH_Z,
				  -11, NULL);

	/* Get c1 data size */
	tmp_imm = payload_sz; /* convert to 32-bit imm value */
	descbuf = cmd_insert_load(descbuf, &tmp_imm, LDST_CLASS_1_CCB,
				  0, LDST_SRCDST_WORD_DATASZ_REG, 0, 4,
				  ITEM_INLINE);

	/* Set up info-fifo for OP, store, wait for done */
	ififo_cmd = 0x54f00000 | payload_sz;
	descbuf = cmd_insert_load(descbuf, &ififo_cmd, LDST_CLASS_IND_CCB, 0,
				  LDST_SRCDST_WORD_INFO_FIFO, 0, 4,
				  ITEM_INLINE);

	descbuf = cmd_insert_alg_op(descbuf, OP_TYPE_CLASS1_ALG,
				    OP_ALG_ALGSEL_KASUMI, OP_ALG_AAI_F8,
				    OP_ALG_AS_INITFINAL, ICV_CHECK_OFF,
				    DIR_DECRYPT);

	descbuf = cmd_insert_seq_fifo_store(descbuf, LDST_CLASS_IND_CCB, 0,
					    FIFOST_TYPE_MESSAGE_DATA,
					    payload_sz);

	descbuf = cmd_insert_jump(descbuf, CLASS_1, JUMP_TYPE_LOCAL,
				  JUMP_TEST_ALL, 0, 1, NULL);

	/*
	 * Cleanup. Clear mode, and release CHA
	 */
	tmp_imm = 1;
	descbuf = cmd_insert_load(descbuf, &tmp_imm, LDST_CLASS_IND_CCB, 0,
				  LDST_SRCDST_WORD_CLRW, 0, 4, ITEM_INLINE);

	tmp_imm = 0x10;
	descbuf = cmd_insert_load(descbuf, &tmp_imm, LDST_CLASS_IND_CCB, 0,
				  LDST_SRCDST_WORD_CHACTRL, 0, 4, ITEM_INLINE);

	/* Finally, header update once length known */
	endidx = descbuf - start;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_ALWAYS);

	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_pcl_shdsc_3gpp_rlc_decap);

int32_t cnstr_pcl_shdsc_3gpp_rlc_encap(u_int32_t *descbuf, u_int16_t *bufsize,
				       u_int8_t *key, u_int32_t keysz,
				       u_int32_t count, u_int32_t bearer,
				       u_int32_t direction,
				       u_int16_t payload_sz)
{
	u_int32_t *start;
	u_int16_t startidx, endidx;
	u_int32_t kctx[2];
	u_int32_t tmp_imm;

	start = descbuf++;

	if (!descbuf)
		return -1;

	/* build count/bearer/direction as register-loadable words */
	kctx[0] = count;
	kctx[1] = ((bearer & 0x1f) << 27) | ((direction & 0x01) << 26);

	startidx = descbuf - start;

	descbuf = cmd_insert_seq_load(descbuf, LDST_CLASS_DECO, 0,
				      LDST_SRCDST_WORD_DECO_MATH1, 0, 2);

	descbuf = cmd_insert_load(descbuf, kctx, LDST_CLASS_DECO, 0,
				  LDST_SRCDST_WORD_DECO_MATH0, 0, 8,
				  ITEM_INLINE);

	descbuf = cmd_insert_key(descbuf, key, keysz, PTR_DIRECT,
				 KEYDST_KEYREG, KEY_CLEAR, ITEM_REFERENCE,
				 ITEM_CLASS1);

	descbuf = cmd_insert_seq_fifo_load(descbuf, CLASS_1, 0,
					   FIFOLD_TYPE_LAST1, payload_sz);

	descbuf = cmd_insert_jump(descbuf, JUMP_TYPE_LOCAL, CLASS_NONE,
				  JUMP_TEST_ALL, JUMP_COND_CALM, 1, NULL);

	tmp_imm = 4;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_RSHIFT, MATH_SRC0_REG1,
				  MATH_SRC1_IMM, MATH_DEST_REG1, 8, 0, 0,
				  MATH_IFB, &tmp_imm);

	tmp_imm = 15;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_RSHIFT, MATH_SRC0_REG1,
				  MATH_SRC1_IMM, MATH_DEST_REG2, 8, 0, 0,
				  MATH_IFB, &tmp_imm);

	tmp_imm = 0x0fff0000;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_AND, MATH_SRC0_REG2,
				  MATH_SRC1_IMM, MATH_DEST_REG2, 8, 0, 0,
				  0, &tmp_imm);

	descbuf = cmd_insert_math(descbuf, MATH_FUN_ADD, MATH_SRC0_REG0,
				  MATH_SRC1_REG2, MATH_DEST_REG0, 8, 0, 0, 0,
				  NULL);

	descbuf = cmd_insert_move(descbuf, 0, MOVE_SRC_MATH0,
				  MOVE_DEST_CLASS1CTX, 0, 8);

	descbuf = cmd_insert_alg_op(descbuf, OP_TYPE_CLASS1_ALG,
				    OP_ALG_ALGSEL_KASUMI, OP_ALG_AAI_F8,
				    OP_ALG_AS_INITFINAL, ICV_CHECK_OFF,
				    DIR_ENCRYPT);

	tmp_imm = payload_sz + 3;
	descbuf = cmd_insert_load(descbuf, &tmp_imm, LDST_CLASS_DECO, 0,
				  LDST_SRCDST_WORD_DECO_MATH3, 0, 4,
				  ITEM_INLINE);

	descbuf = cmd_insert_jump(descbuf, JUMP_TYPE_LOCAL, CLASS_1,
				  JUMP_TEST_ALL, 0, 1, NULL);

	tmp_imm = 1;
	descbuf = cmd_insert_load(descbuf, &tmp_imm, LDST_CLASS_IND_CCB,
				  0, LDST_SRCDST_WORD_CLRW, 0, 4,
				  ITEM_INLINE);

	tmp_imm = 0x10;
	descbuf = cmd_insert_load(descbuf, &tmp_imm, LDST_CLASS_IND_CCB,
				  0, LDST_SRCDST_WORD_CHACTRL, 0, 4,
				  ITEM_INLINE);

	tmp_imm = 0;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_AND, MATH_SRC0_ZERO,
				  MATH_SRC1_REG2, MATH_DEST_REG2, 8, MATH_NFU,
				  0, MATH_IFB, &tmp_imm);

	descbuf = cmd_insert_jump(descbuf, JUMP_TYPE_LOCAL, CLASS_NONE,
				  JUMP_TEST_ANY, JUMP_COND_MATH_Z, 2, NULL);

	descbuf = cmd_insert_move(descbuf, MOVE_WAITCOMP, MOVE_SRC_OUTFIFO,
				  MOVE_DEST_MATH2, 0, 8);

	tmp_imm = 20;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_RSHIFT, MATH_SRC0_REG2,
				  MATH_SRC1_IMM, MATH_DEST_REG0, 8, 0, 0,
				  MATH_IFB, &tmp_imm);

	descbuf = cmd_insert_math(descbuf, MATH_FUN_OR, MATH_SRC0_REG0,
				  MATH_SRC1_REG1, MATH_DEST_REG1, 8, 0, 0, 0,
				  NULL);

	descbuf = cmd_insert_move(descbuf, 0, MOVE_SRC_MATH1, MOVE_DEST_OUTFIFO,
				  0, 8);

	tmp_imm = 8;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_SUB, MATH_SRC0_REG3,
				  MATH_SRC1_IMM, MATH_DEST_REG3, 8, 0, 0,
				  MATH_IFB, &tmp_imm);

	tmp_imm = 44;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_LSHIFT, MATH_SRC0_REG2,
				  MATH_SRC1_IMM, MATH_DEST_REG1, 8, 0, 0,
				  MATH_IFB, &tmp_imm);

	descbuf = cmd_insert_jump(descbuf, JUMP_TYPE_LOCAL, CLASS_NONE,
				  JUMP_TEST_ANY, JUMP_COND_MATH_NV, 4, NULL);

	tmp_imm = 0xfffc;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_AND, MATH_SRC0_REG3,
				  MATH_SRC1_IMM, MATH_DEST_NONE, 4, 0, 0,
				  MATH_IFB, &tmp_imm);

	descbuf = cmd_insert_jump(descbuf, JUMP_TYPE_LOCAL, CLASS_NONE,
				  JUMP_TEST_ALL, 0, -14, NULL);

	descbuf = cmd_insert_seq_store(descbuf, FIFOLD_CLASS_SKIP, 0,
				       FIFOST_TYPE_MESSAGE_DATA, 0,
				       payload_sz + 3);

	endidx = descbuf - start;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_ALWAYS);

	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_pcl_shdsc_3gpp_rlc_encap);
