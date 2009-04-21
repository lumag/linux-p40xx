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
 * 802.16 WiMAX encapsulation
 *
 * @descbuf - Pointer to descriptor build buffer
 *
 * @bufsize - Pointer to value to write size of built descriptor to
 *
 * @pdb.framecheck - nonzero if frame check word to be included
 * @pdb.nonce - Nonce to use
 * @pdb.b0_flags - B0 flags
 * @pdb.ctr_flags - CTR flags
 * @pdb.PN - PN value
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
				struct wimax_pdb *pdb,
				struct cipherparams *cipherdata,
				u_int8_t mode, u_int8_t clear)
{
	u_int32_t pdbopts;
	u_int32_t *start;
	u_int16_t startidx, endidx;

	start = descbuf++;

	if (!descbuf)
		return -1;

	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

	/*
	 * Construct encap PDB
	 * u24 - (reserved)
	 * u8  - options
	 * u32 - nonce
	 * u8  - b0 flags
	 * u8  - counter flags
	 * u16 - counter init
	 * u32 - PN
	 */

	/* options word */
	if (pdb->framecheck)
		pdbopts = 1;
	*descbuf++ = pdbopts;

	/* Nonce */
	pdbopts = pdb->nonce;
	*descbuf++ = pdbopts;

	/* B0 flags / CTR flags / initial counter */
	pdbopts = (pdb->b0_flags << 24) | (pdb->ctr_flags << 16) |
		  pdb->ctr_initial_count;
	*descbuf++ = pdbopts;

	/* PN word */
	pdbopts = pdb->PN;
	*descbuf++ = pdbopts;

	/* Done with PDB, build AES-CTR key block */
	startidx = descbuf - start;

	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);

	/* end with OPERATION */
	descbuf = cmd_insert_proto_op_wimax(descbuf, mode, DIR_ENCAP);

	/* Now compute shared header */
	endidx = descbuf - start + 1;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_SERIAL);
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
 * @pdb.framecheck - nonzero if frame check word to be included
 * @pdb.nonce - Nonce to use
 * @pdb.iv_flags - IV flags
 * @pdb.ctr_flags - counter flags
 * @pdb.ctr_initial_count - counter value
 * @pdb.PN - PN value
 * @pdb.antireplay_len - length of antireplay window
 *
 * @cipherdata.keylen - key size in bytes
 * @cipherdata.key - key data to inline
 *
 * @mode - nonzero for OFDMa, else OFDM
 *
 * @clear - nonzero clears descriptor buffer
 **/

int32_t cnstr_shdsc_wimax_decap(u_int32_t *descbuf, u_int16_t *bufsize,
				struct wimax_pdb *pdb,
				struct cipherparams *cipherdata,
				u_int8_t mode, u_int8_t clear)
{
	u_int32_t pdbopts;
	u_int32_t *start;
	u_int16_t startidx, endidx;

	start = descbuf++;

	if (!descbuf)
		return -1;

	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

	/*
	 * Construct decap PDB
	 * u24 - (reserved)
	 * u8  - options
	 * u32 - nonce
	 * u8  - IV flags
	 * u8  - counter flags
	 * u16 - counter init
	 * u32 - PN
	 * u16 - (reserved)
	 * u16 - antireplay length
	 * u64 - antireplay scorecard
	 */

	/* options word */
	if (pdb->framecheck)
		pdbopts = 1;
	if (pdb->antireplay_len)
		pdbopts |= 0x40;
	*descbuf++ = pdbopts;

	/* Nonce */
	pdbopts = pdb->nonce;
	*descbuf++ = pdbopts;

	/* IV flags / CTR flags / initial counter */
	pdbopts = (pdb->iv_flags << 24) | (pdb->ctr_flags << 16) |
		  pdb->ctr_initial_count;
	*descbuf++ = pdbopts;

	/* PN word */
	pdbopts = pdb->PN;
	*descbuf++ = pdbopts;

	/* Antireplay */
	pdbopts = pdb->antireplay_len;
	*descbuf++ = pdbopts; /* write length */
	memset(descbuf, 0, 8);
	descbuf += 2;

	/* Done with PDB, build key block */
	startidx = descbuf - start;

	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);

	/* end with OPERATION */
	descbuf = cmd_insert_proto_op_wimax(descbuf, mode, DIR_DECAP);

	/* Now compute shared header */
	endidx = descbuf - start + 1;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_SERIAL);
	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_shdsc_wimax_decap);

/**
 * MacSec encapsulation
 *
 * @descbuf - Pointer to descriptor build buffer
 *
 * @bufsize - Pointer to value to write size of built descriptor to
 *
 * pdb.framecheck - nonzero if frame-check-sequence is to be
 *                  included in the output
 * pdb.aad_len - length of AAD to include
 * pdb.sci - SCI, if to be included
 * pdb.PN - packet number
 * pdb.ethertype - 16 bit ethertype to include
 * pdb.tci_an - TCI/AN byte
 *
 * @cipherdata.keylen - key size in bytes
 * @cipherdata.key - key data to inline
 *
 * @clear - nonzero clears descriptor buffer
 **/

int32_t cnstr_shdsc_macsec_encap(u_int32_t *descbuf, u_int16_t *bufsize,
				 struct macsec_pdb *pdb,
				 struct cipherparams *cipherdata,
				 u_int8_t clear)
{
	u_int32_t pdbopts;
	u_int32_t *start;
	u_int16_t startidx, endidx;

	start = descbuf++;

	if (!descbuf)
		return -1;

	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

	/* options word */
	pdbopts = pdb->aad_len >> 16;
	if (pdb->framecheck)
		pdbopts |= 1;
	*descbuf++ = pdbopts;

	/* SCI */
	memcpy(descbuf, &pdb->sci, 8);
	descbuf += 2;

	pdbopts = (pdb->ethertype << 16) | (pdb->tci_an << 8);
	*descbuf++ = pdbopts;

	/* PN word */
	pdbopts = pdb->PN;
	*descbuf++ = pdbopts;

	/* Done with PDB, build key block */
	startidx = descbuf - start;

	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);

	/* end with OPERATION */
	descbuf = cmd_insert_proto_op_macsec(descbuf, DIR_ENCAP);

	/* Now compute shared header */
	endidx = descbuf - start + 1;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_SERIAL);
	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_shdsc_macsec_encap);

/**
 * MacSec decapsulation
 *
 * @descbuf - Pointer to descriptor build buffer
 *
 * @bufsize - Pointer to value to write size of built descriptor to
 *
 * pdb.framecheck - nonzero if frame-check-sequence is on
 * pdb.aad_len - length of AAD used
 * pdb.sci - SCI, if to be used
 * pdb.PN - packet number
 * pdb.ethertype - 16 bit ethertype
 * pdb.tci_an - TCI/AN byte
 * pdb.antireplay_len - antireplay length
 *
 * @cipherdata.keylen - key size in bytes
 * @cipherdata.key - key data to inline
 *
 * @clear - nonzero clears descriptor buffer
 **/
int32_t cnstr_shdsc_macsec_decap(u_int32_t *descbuf, u_int16_t *bufsize,
				 struct macsec_pdb *pdb,
				 struct cipherparams *cipherdata,
				 u_int8_t clear)
{
	u_int32_t pdbopts;
	u_int32_t *start;
	u_int16_t startidx, endidx;

	start = descbuf++;

	if (!descbuf)
		return -1;

	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

	/* options word */
	pdbopts = pdb->aad_len >> 16;
	if (pdb->framecheck)
		pdbopts |= 1;
	if (pdb->antireplay_len)
		pdbopts |= 0x40;
	*descbuf++ = pdbopts;

	/* SCI */
	memcpy(descbuf, &pdb->sci, 8);
	descbuf += 2;

	/* antireplay length */
	pdbopts = pdb->antireplay_len;
	*descbuf++ = pdbopts;

	/* PN word */
	pdbopts = pdb->PN;
	*descbuf++ = pdbopts;

	/* antireplay scorecard */
	memset(descbuf, 0, 8);
	descbuf += 2;

	/* Done with PDB, build key block */
	startidx = descbuf - start;

	descbuf = cmd_insert_key(descbuf, cipherdata->key, cipherdata->keylen,
				 PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				 ITEM_INLINE, ITEM_CLASS1);

	/* end with OPERATION */
	descbuf = cmd_insert_proto_op_macsec(descbuf, DIR_DECAP);

	/* Now compute shared header */
	endidx = descbuf - start + 1;
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_SERIAL);
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

	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_ALWAYS);

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

	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_ALWAYS);

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
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_ALWAYS);

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

	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_ALWAYS);

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

/* FIXME: these should be built from bit defs not yet present in desc.h */
static const u_int32_t rlc_fifo_cmd[] = {
	/* IFIFO dest=skip,flush,stype=DFIFO,dtype=msg,len=3 */
	0x04F00003u,
	 /* IFIFO dest=skip,flush,stype=DFIFO,dtype=msg,len=80 */
	0x04F00050u,
	/* IFIFO dest=class1,last1,flush1,stype=DFIFO,dtype=msg,len=80 */
	0x54F00050u
};

static const u_int32_t rlc_segnum_mask[] = {
	0x00000fff,
	0x00000000
};

/* misc values for adding immediates to math/move/load/etc. */
static const u_int32_t cmd_imm_src[] = {
	0, 1, 19, 16, 4, 60, 8
};

static const u_int8_t jump_imm_src[] = {
	-10
};

int32_t cnstr_pcl_shdsc_3gpp_rlc_decap(u_int32_t *descbuf, u_int16_t *bufsize,
				       u_int8_t *key, u_int32_t keysz,
				       u_int32_t count, u_int32_t bearer,
				       u_int32_t direction,
				       u_int32_t payload_sz, u_int8_t clear)
{
	u_int32_t *start;
	u_int16_t startidx, endidx;
	u_int32_t kctx[2];

	start = descbuf++;

	if (!descbuf)
		return -1;


	/* build count/bearer/direction as register-loadable words */
	kctx[0] = count;
	kctx[1] = ((bearer & 0x1f) << 27) | ((direction & 0x01) << 26);

	startidx = descbuf - start;

	/* Key */
	descbuf = cmd_insert_key(descbuf, key, keysz, PTR_DIRECT, KEYDST_KEYREG,
				 KEY_CLEAR, ITEM_REFERENCE, ITEM_CLASS1);

	/* load DCTRL - disable auto Info FIFO entries */
	descbuf = cmd_insert_load(descbuf, NULL, LDST_CLASS_DECO, 0,
				  LDST_SRCDST_WORD_DECOCTRL, 0x08, 0x00,
				  ITEM_INLINE);

	/* get encrypted RLC PDU w/header */
	descbuf = cmd_insert_seq_fifo_load(descbuf, LDST_CLASS_1_CCB, 0,
					   FIFOLD_TYPE_MSG | FIFOLD_TYPE_LAST1,
					   83);

	/* load imm Info FIFO */
	descbuf = cmd_insert_load(descbuf, (u_int32_t *)&rlc_fifo_cmd[1],
				  LDST_CLASS_DECO, 0,
				  LDST_SRCDST_WORD_INFO_FIFO, 0, 8,
				  ITEM_INLINE);

	/* load imm Info FIFO */
	descbuf = cmd_insert_load(descbuf, (u_int32_t *)&rlc_fifo_cmd[2],
				  LDST_CLASS_DECO, 0,
				  LDST_SRCDST_WORD_INFO_FIFO, 0, 8,
				  ITEM_INLINE);

	/* load imm MATH0 */
	descbuf = cmd_insert_load(descbuf, kctx, LDST_CLASS_DECO, 0,
				  LDST_SRCDST_WORD_DECO_MATH0, 0, 8,
				  ITEM_INLINE);

	/* add imm: IFIFO + 0 -> MATH1 (3B from InfoFIFO) */
	descbuf = cmd_insert_math(descbuf, MATH_FUN_ADD, MATH_SRC0_IMM,
				  MATH_SRC1_INFIFO, MATH_DEST_REG0, 8, 0, 0,
				  MATH_IFB, (u_int32_t *)&cmd_imm_src[0]);

	/* shl imm: MATH1 << 4 -> MATH1 (left-align RLC PDU header) */
	descbuf = cmd_insert_math(descbuf, MATH_FUN_ADD, MATH_SRC0_REG0,
				  MATH_SRC1_IMM, MATH_DEST_REG1, 8, 0, 0,
				  MATH_IFB, (u_int32_t *)&cmd_imm_src[1]);

	/* MATH1 (2B PDU) -> OFIFO (8 bytes get moved) */
	descbuf = cmd_insert_move(descbuf, 0, MOVE_SRC_MATH1,
				  MOVE_DEST_OUTFIFO, 0, 2);

	/* Store 2-byte PDU header */
	descbuf = cmd_insert_seq_fifo_store(descbuf, LDST_CLASS_IND_CCB, 0,
					    FIFOST_TYPE_MESSAGE_DATA, 2);

	/* wait for DMA CALM */
	descbuf = cmd_insert_jump(descbuf, JUMP_TYPE_LOCAL, JUMP_TEST_ALL,
				  JUMP_COND_CALM, 1, NULL);

	/* DCTRL - reset OFIFO to flush extra 6 bytes */
	descbuf = cmd_insert_load(descbuf, NULL, LDST_CLASS_DECO, 0,
				  LDST_SRCDST_WORD_DECOCTRL, 0, 32,
				  ITEM_INLINE);

	/* shr imm MATH1 -> MATH2 (align seqnum with PPDB data) */
	descbuf = cmd_insert_math(descbuf, MATH_FUN_RSHIFT, MATH_SRC0_REG1,
				  MATH_SRC1_IMM, MATH_DEST_REG2, 8, 0, 0,
				  MATH_IFB, (u_int32_t *)&cmd_imm_src[2]);

	/* mask Math2 to isolate seqnum */
	descbuf = cmd_insert_math(descbuf, MATH_FUN_AND, MATH_SRC0_REG2,
				  MATH_SRC1_IMM, MATH_DEST_REG2, 8, 0, 0, 0,
				  (u_int32_t *)rlc_segnum_mask);

	/* add seqnum to PPDB data */
	descbuf = cmd_insert_math(descbuf, MATH_FUN_ADD, MATH_SRC0_REG0,
				  MATH_SRC1_REG2, MATH_DEST_REG1, 8, 0, 0,
				  0, NULL);

	/* MATH0 (PPDB data) -> CTX1 */
	descbuf = cmd_insert_move(descbuf, 0, MOVE_SRC_MATH0,
				  MOVE_DEST_CLASS1CTX, 0, 8);

	/* shl imm: MATH1 << 16 -> MATH1 (4 bits left) */
	descbuf = cmd_insert_math(descbuf, MATH_FUN_LSHIFT, MATH_SRC0_REG1,
				  MATH_SRC1_IMM, MATH_DEST_REG1, 8, 0, 0,
				  0, (u_int32_t *)&cmd_imm_src[3]);

	/* load imm MATH3 */
	descbuf = cmd_insert_load(descbuf, &payload_sz, LDST_CLASS_DECO,
				  0, LDST_SRCDST_WORD_DECO_MATH3, 0, 4,
				  ITEM_INLINE);

	/* add imm: IFIFO (next 8B of payload) + 0 -> MATH2 */
	descbuf = cmd_insert_math(descbuf, MATH_FUN_ADD, MATH_SRC0_IMM,
				  MATH_SRC1_INFIFO, MATH_DEST_REG2,
				  8, 0, 0, 0, (u_int32_t *)&cmd_imm_src[0]);

	/* shr imm: Math2 >> 4 -> MATH0 */
	descbuf = cmd_insert_math(descbuf, MATH_FUN_RSHIFT, MATH_SRC0_REG2,
				  MATH_SRC1_IMM, MATH_DEST_REG0,
				  8, 0, 0, 0, (u_int32_t *)&cmd_imm_src[4]);

	/* or: MATH0 (60 lsbs) | MATH1 (4 msbs) -> MATH1 */
	descbuf = cmd_insert_math(descbuf, MATH_FUN_OR, MATH_SRC0_REG0,
				  MATH_SRC1_REG1, MATH_DEST_REG1, 8, 0, 0, 0,
				  NULL);

	/* MATH1 -> IFIFO */
	descbuf = cmd_insert_move(descbuf, 0, MOVE_SRC_MATH1,
				  MOVE_DEST_CLASS1INFIFO, 0, 8);

	/* shl imm: MATH2 << 60 -> MATH1 */
	descbuf = cmd_insert_math(descbuf, MATH_FUN_LSHIFT, MATH_SRC0_REG2,
				  MATH_SRC1_IMM, MATH_DEST_REG1, 8, 0, 0, 0,
				  (u_int32_t *)&cmd_imm_src[5]);

	/* dec MATH3 by 8 (number of bytes loaded) */
	descbuf = cmd_insert_math(descbuf, MATH_FUN_SUB, MATH_SRC0_REG3,
				  MATH_SRC1_IMM, MATH_DEST_REG3, 8, 0, 0,
				  MATH_IFB, (u_int32_t *)&cmd_imm_src[6]);

	/* jump back to L1 if MATH3 counter indicates more data */
	descbuf = cmd_insert_jump(descbuf, JUMP_TYPE_LOCAL, JUMP_TEST_ALL,
				  JUMP_COND_MATH_NV | JUMP_COND_MATH_Z,
				  jump_imm_src[0], NULL);

	/* load Class1 Data Size (exec before InfoFIFO entry made w/last1) */
	descbuf = cmd_insert_load(descbuf, &payload_sz, LDST_CLASS_1_CCB,
				  0, LDST_SRCDST_WORD_DATASZ_REG, 0, 4,
				  ITEM_INLINE);

	/* load Info FIFO */
	descbuf = cmd_insert_load(descbuf, (u_int32_t *)&rlc_fifo_cmd[3],
				  LDST_CLASS_DECO, 0,
				  LDST_SRCDST_WORD_INFO_FIFO, 0, 4,
				  ITEM_INLINE);

	/* operation KFHA f8 initfinal */
	descbuf = cmd_insert_alg_op(descbuf, OP_TYPE_CLASS1_ALG,
				    OP_ALG_ALGSEL_KASUMI, OP_ALG_AAI_F8,
				    OP_ALG_AS_INITFINAL, ICV_CHECK_OFF,
				    DIR_ENCRYPT);

	/* store msg */
	descbuf = cmd_insert_seq_fifo_store(descbuf, LDST_CLASS_1_CCB, 0,
					    FIFOST_TYPE_MESSAGE_DATA,
					    payload_sz);

	/* wait for CLASS1 CHA done */
	descbuf = cmd_insert_jump(descbuf, JUMP_TYPE_LOCAL,
				  JUMP_TEST_ALL, 0, 1, NULL);

	/* load imm to cha ClrWrittenReg to clear mode */
	descbuf = cmd_insert_load(descbuf, (u_int32_t *)&cmd_imm_src[1],
				  LDST_CLASS_IND_CCB, 0,
				  LDST_SRCDST_WORD_CLRW, 0, 4, ITEM_INLINE);

	/* load imm to cha CTL to release KFHA */
	descbuf = cmd_insert_load(descbuf, (u_int32_t *)&cmd_imm_src[3],
				  LDST_CLASS_IND_CCB, 0,
				  LDST_SRCDST_WORD_CHACTRL, 0, 4, ITEM_INLINE);

	endidx = descbuf - start + 1; /* add 1 to include header */
	cmd_insert_shared_hdr(start, startidx, endidx, CTX_ERASE, SHR_ALWAYS);

	*bufsize = endidx;

	return 0;
}
