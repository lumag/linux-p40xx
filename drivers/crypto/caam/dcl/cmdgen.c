/*
 * CAAM Descriptor Construction Library
 * Descriptor Command Generator
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

/*
 * NOTE: CAAM can be configured for either 32-bit mode or 36-bit mode
 * at core initialization time. At this time, cmdgen assumes 32-bit
 * mode, but an option to handle either case needs to be added to it,
 * either as a build-time or run-time option.
 */

/**
 * cmd_insert_shared_hdr()
 * Insert a shared descriptor header into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the header
 * just constructed. If an error occurred, returns 0.
 *
 * @descwd   = pointer to target descriptor word to hold this command.
 *             Note that this should always be the first word of a
 *             descriptor.
 * @startidx = index to continuation of descriptor data, normally the
 *             first descriptor word past a PDB. This tells DECO what
 *             to skip over.
 * @desclen  = length of descriptor in words, including header.
 * @ctxsave  = Saved or erases context when a descriptor is self-shared
 *             - CTX_SAVE  = context saved between iterations
 *             - CTX_ERASE = context is erased
 * @share    = Share state of this descriptor:
 *             - SHR_NEVER  = Never share. Fetching is repeated for each
 *                            processing pass.
 *             - SHR_WAIT   = Share once processing starts.
 *             - SHR_SERIAL = Share once completed.
 *             - SHR_ALWAYS = Always share (except keys)
 *
 * Note: Headers should normally be constructed as the final operation
 *       in the descriptor construction, because the start index and
 *       overall descriptor length will likely not be known until
 *       construction is complete. For this reason, there is little use
 *       to the "incremental pointer" convention. The exception is probably
 *       in the construction of simple descriptors where the size is easily
 *       known early in the construction process.
 **/
u_int32_t *cmd_insert_shared_hdr(u_int32_t *descwd, u_int8_t startidx,
				 u_int8_t desclen, enum ctxsave ctxsave,
				 enum shrst share)
{
	*descwd = CMD_SHARED_DESC_HDR | HDR_ONE |
		  ((startidx & HDR_START_IDX_MASK) << HDR_START_IDX_SHIFT) |
		  (desclen & HDR_DESCLEN_SHR_MASK) |
		  (share << HDR_SD_SHARE_SHIFT) |
		  ((ctxsave == CTX_SAVE) ? HDR_SAVECTX : 0);

	return descwd + 1;
}

/**
 * cmd_insert_hdr()
 * Insert a standard descriptor header into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the header
 * just constructed. If an error occurred, returns 0.
 *
 * @descwd   = pointer to target descriptor word to hold this command.
 *             Note that this should always be the first word of a
 *             descriptor.
 *
 * @startidx = index to continuation of descriptor data, or if
 *             sharenext = SHRNXT_SHARED, then specifies the size
 *             of the associated shared descriptor referenced in
 *             the following instruction.
 *
 * @desclen  = length of descriptor in words, including header
 *
 * @share    = Share state for this descriptor:
 *             - SHR_NEVER  = Never share. Fetching is repeated for each
 *                            processing pass.
 *             - SHR_WAIT   = Share once processing starts.
 *             - SHR_SERIAL = Share once completed.
 *             - SHR_ALWAYS = Always share (except keys)
 *             - SHR_DEFER  = Use the referenced sharedesc to determine
 *                             sharing intent
 *
 * @sharenext = Control state of shared descriptor processing
 *              - SHRNXT_SHARED = This is a job descriptor consisting
 *                                of a header and a pointer to a shared
 *                                descriptor only.
 *              - SHRNXT_LENGTH = This is a detailed job descriptor, thus
 *                                desclen refers to the full length of this
 *                                descriptor.
 *
 * @reverse   = Reverse execution order between this job descriptor, and
 *              an associated shared descriptor:
 *              - ORDER_REVERSE - execute this descriptor before the shared
 *                                descriptor referenced.
 *              - ORDER_FORWARD - execute the shared descriptor, then this
 *                                descriptor.
 *
 * @mktrusted = DESC_SIGN - sign this descriptor prior to execuition
 *              DESC_STD  - leave descriptor non-trusted
 *
 * Note: Headers should normally be constructed as the final operation
 *       of descriptor construction, because the start index and
 *       overall descriptor length will likely not be known until
 *       construction is complete. For this reason, there is little use
 *       to the "incremental pointer" convention. The exception is probably
 *       in the construction of simple descriptors where the size is easily
 *       known early in the construction process.
 **/
u_int32_t *cmd_insert_hdr(u_int32_t *descwd, u_int8_t startidx,
			  u_int8_t desclen, enum shrst share,
			  enum shrnext sharenext, enum execorder reverse,
			  enum mktrust mktrusted)
{
	*descwd = CMD_DESC_HDR | HDR_ONE |
		  ((startidx & HDR_START_IDX_MASK) << HDR_START_IDX_SHIFT) |
		  (desclen & HDR_DESCLEN_MASK) |
		  (share << HDR_SD_SHARE_SHIFT) |
		  ((sharenext == SHRNXT_SHARED) ? HDR_SHARED : 0) |
		  ((reverse == ORDER_REVERSE) ? HDR_REVERSE : 0) |
		  ((mktrusted = DESC_SIGN) ? HDR_MAKE_TRUSTED : 0);

	return descwd + 1;
}

/**
 * cmd_insert_key()
 * Insert a key command into a descriptor
 *
 * Returns: If successful, returns a pointer to the target word
 * incremented past the newly-inserted command (including item pointer
 * or inlined data). Effectively, this becomes a pointer to the next
 * word to receive a new command in this descriptor. If error, returns 0
 *
 * @descwd  = pointer to target descriptor word to hold this command
 *
 * @key     = pointer to key data as an array of bytes.
 *
 * @keylen  = pointer to key size, expressed in bits.
 *
 * @sgref   = pointer is actual data, or a scatter-gather list
 *            representing the key:
 *            - PTR_DIRECT = points to data
 *            - PTR_SGLIST = points to CAAM-specific scatter gather
 *              table. Cannot use if imm = ITEM_INLINE.
 *
 * @dest    = target destination in CAAM to receive the key. This may be:
 *            - KEYDST_KEYREG   = Key register in the CHA selected by an
 *                                OPERATION command.
 *            - KEYDST_PK_E     = The 'e' register in the public key block
 *            - KEYDST_AF_SBOX  = Direct SBOX load if ARC4 is selected
 *            - KEYDST_MD_SPLIT = Message digest IPAD/OPAD direct load.
 *
 * @cover   = Key was encrypted, and must be decrypted during the load.
 *            If trusted descriptor, use TDEK, else use JDEK to decrypt.
 *            - KEY_CLEAR   = key is cleartext, no decryption needed
 *            - KEY_COVERED = key is ciphertext, decrypt.
 *
 * @imm     = Key can either be referenced, or loaded into the descriptor
 *            immediately following the command for improved performance.
 *            - ITEM_REFERENCE = a pointer follows the command.
 *            - ITEM_INLINE    = key data follows the command, padded out
 *                                to a descriptor word boundary.
 *
 * @purpose = Sends the key to the class 1 or 2 CHA as selected by an
 *            OPERATION command. If dest is KEYDST_PK_E or KEYDST_AF_SBOX,
 *            then this must be ITEM_CLASS1.
 **/
u_int32_t *cmd_insert_key(u_int32_t *descwd, u_int8_t *key, u_int32_t keylen,
			  enum ref_type sgref, enum key_dest dest,
			  enum key_cover cover, enum item_inline imm,
			  enum item_purpose purpose)
{
	u_int32_t *nextwd, *keybuf;
	u_int32_t  keysz, keywds;

	if ((!descwd) || (!key))
		return 0;

	/* If PK 'e' or AF SBOX load, can't be class 2 key */
	if (((dest == KEYDST_PK_E) || (dest == KEYDST_AF_SBOX)) &&
	    (purpose == ITEM_CLASS2))
		return 0;

	/* sg table can't be inlined */
	if ((sgref == PTR_SGLIST) && (imm == ITEM_INLINE))
		return 0;

	nextwd = descwd;

	/* Convert size (in bits) to adequate byte length */
	keysz = ((keylen & KEY_LENGTH_MASK) >> 3);
	if (keylen & 0x00000007)
		keysz++;

	/* Build command word */
	*nextwd = CMD_KEY;
	switch (dest) {
	case KEYDST_KEYREG:
		*nextwd |= KEY_DEST_CLASS_REG;
		break;

	case KEYDST_PK_E:
		*nextwd |= KEY_DEST_PKHA_E;
		break;

	case KEYDST_AF_SBOX:
		*nextwd |= KEY_DEST_AFHA_SBOX;
		break;

	case KEYDST_MD_SPLIT:
		*nextwd |= KEY_DEST_MDHA_SPLIT;
		break;
	}

	if (cover == KEY_COVERED)
		*nextwd |= KEY_ENC;

	if (imm == ITEM_INLINE)
		*nextwd |= KEY_IMM;

	switch (purpose) {
	case ITEM_CLASS1:
		*nextwd |= CLASS_1;
		break;

	case ITEM_CLASS2:
		*nextwd |= CLASS_2;
		break;

	default:
		return 0;
	};
	if (sgref == PTR_SGLIST)
		*nextwd |= KEY_SGF;

	*nextwd++ |= keysz;

	if (imm == ITEM_INLINE) {
		keywds = keysz >> 2;
		keybuf = (u_int32_t *)key;
		while (keywds) {
			*nextwd++ = *keybuf++;
			keywds--;
		}
	} else
		*nextwd++ = (u_int32_t)key;
	return nextwd;
}

/**
 * cmd_insert_seq_key()
 * Insert a sequence key command into a descriptor
 *
 * Returns: If successful, returns a pointer to the target word
 * incremented past the newly-inserted command (including item pointer
 * or inlined data). Effectively, this becomes a pointer to the next
 * word to receive a new command in this descriptor. If error, returns 0
 *
 * @descwd  = pointer to target descriptor word to hold this command
 *
 * @keylen  = pointer to key size, expressed in bits.
 *
 * @sgref   = pointer is actual data, or a scatter-gather list
 *            representing the key:
 *            - PTR_DIRECT = points to data
 *            - PTR_SGLIST = points to CAAM-specific scatter gather
 *              table. Cannot use if imm = ITEM_INLINE.
 *
 * @dest    = target destination in CAAM to receive the key. This may be:
 *            - KEYDST_KEYREG   = Key register in the CHA selected by an
 *                                OPERATION command.
 *            - KEYDST_PK_E     = The 'e' register in the public key block
 *            - KEYDST_AF_SBOX  = Direct SBOX load if ARC4 is selected
 *            - KEYDST_MD_SPLIT = Message digest IPAD/OPAD direct load.
 *
 * @cover   = Key was encrypted, and must be decrypted during the load.
 *            If trusted descriptor, use TDEK, else use JDEK to decrypt.
 *            - KEY_CLEAR   = key is cleartext, no decryption needed
 *            - KEY_COVERED = key is ciphertext, decrypt.
 *
 * @purpose = Sends the key to the class 1 or 2 CHA as selected by an
 *            OPERATION command. If dest is KEYDST_PK_E or KEYDST_AF_SBOX,
 *            then this must be ITEM_CLASS1.
 **/
u_int32_t *cmd_insert_seq_key(u_int32_t *descwd, u_int32_t keylen,
			      enum ref_type sgref, enum key_dest dest,
			      enum key_cover cover, enum item_purpose purpose)
{
	u_int32_t *nextwd, keysz;

	if (!descwd)
		return 0;

	/* If PK 'e' or AF SBOX load, can't be class 2 key */
	if (((dest == KEYDST_PK_E) || (dest == KEYDST_AF_SBOX)) &&
	    (purpose == ITEM_CLASS2))
		return 0;

	nextwd = descwd;

	/* Convert size (in bits) to adequate byte length */
	keysz = ((keylen & KEY_LENGTH_MASK) >> 3);
	if (keylen & 0x00000007)
		keysz++;

	/* Build command word */
	*nextwd = CMD_SEQ_KEY;
	switch (dest) {
	case KEYDST_KEYREG:
		*nextwd |= KEY_DEST_CLASS_REG;
		break;

	case KEYDST_PK_E:
		*nextwd |= KEY_DEST_PKHA_E;
		break;

	case KEYDST_AF_SBOX:
		*nextwd |= KEY_DEST_AFHA_SBOX;
		break;

	case KEYDST_MD_SPLIT:
		*nextwd |= KEY_DEST_MDHA_SPLIT;
		break;
	}

	if (cover == KEY_COVERED)
		*nextwd |= KEY_ENC;

	switch (purpose) {
	case ITEM_CLASS1:
		*nextwd |= CLASS_1;
		break;

	case ITEM_CLASS2:
		*nextwd |= CLASS_2;
		break;

	default:
		return 0;
	};
	if (sgref == PTR_SGLIST)
		*nextwd |= KEY_SGF;

	*nextwd++ |= keysz;

	return nextwd;
}

/**
 * cmd_insert_proto_op_ipsec()
 * Insert an IPSec protocol operation command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd    = pointer to target descriptor word intended to hold
 *              this command. For an OPERATION command, this is normally
 *              the final word of a single descriptor.
 *
 * @cipheralg = blockcipher selection for this protocol descriptor.
 *              This should be one of CIPHER_TYPE_IPSEC_.
 *
 * @authalg   = authentication selection for this protocol descriptor.
 *              This should be one of AUTH_TYPE_IPSEC_.
 *
 * @dir       = Select DIR_ENCAP for encapsulation, or DIR_DECAP for
 *              decapsulation operations.
 **/
u_int32_t *cmd_insert_proto_op_ipsec(u_int32_t *descwd, u_int8_t cipheralg,
				     u_int8_t authalg, enum protdir dir)
{
	*descwd = CMD_OPERATION | OP_PCLID_IPSEC;

	switch (dir) {
	case DIR_ENCAP:
		*descwd |= OP_TYPE_ENCAP_PROTOCOL;
		break;

	case DIR_DECAP:
		*descwd |= OP_TYPE_DECAP_PROTOCOL;
		break;

	default:
		return 0;
	}

	/*
	 * Note that these cipher selectors match the PFKEY selectors
	 * almost 1 for 1, so this is really little more than an error
	 * check
	 */
	switch (cipheralg) {
	case CIPHER_TYPE_IPSEC_DESCBC:
		*descwd |= OP_PCL_IPSEC_DES;
		break;

	case CIPHER_TYPE_IPSEC_3DESCBC:
		*descwd |= OP_PCL_IPSEC_3DES;
		break;

	case CIPHER_TYPE_IPSEC_AESCBC:
		*descwd |= OP_PCL_IPSEC_AES_CBC;
		break;

	case CIPHER_TYPE_IPSEC_AESCTR:
		*descwd |= OP_PCL_IPSEC_AES_CTR;
		break;

	case CIPHER_TYPE_IPSEC_AES_CCM_ICV8:
		*descwd |= OP_PCL_IPSEC_AES_CCM8;
		break;

	case CIPHER_TYPE_IPSEC_AES_CCM_ICV12:
		*descwd |= OP_PCL_IPSEC_AES_CCM12;
		break;

	case CIPHER_TYPE_IPSEC_AES_CCM_ICV16:
		*descwd |= OP_PCL_IPSEC_AES_CCM16;
		break;

	case CIPHER_TYPE_IPSEC_AES_GCM_ICV8:
		*descwd |= OP_PCL_IPSEC_AES_GCM8;
		break;

	case CIPHER_TYPE_IPSEC_AES_GCM_ICV12:
		*descwd |= OP_PCL_IPSEC_AES_GCM12;
		break;

	case CIPHER_TYPE_IPSEC_AES_GCM_ICV16:
		*descwd |= OP_PCL_IPSEC_AES_GCM16;
		break;

	default:
		return 0;
	}

	/*
	 * Authentication selectors. These do not match the PFKEY
	 * selectors
	 */

	switch (authalg) {
	case AUTH_TYPE_IPSEC_MD5HMAC_96:
		*descwd |= OP_PCL_IPSEC_HMAC_MD5_96;
		break;

	case AUTH_TYPE_IPSEC_SHA1HMAC_96:
		*descwd |= OP_PCL_IPSEC_HMAC_SHA1_96;
		break;

	case AUTH_TYPE_IPSEC_AESXCBCMAC_96:
		*descwd |= OP_PCL_IPSEC_AES_XCBC_MAC_96;
		break;

	case AUTH_TYPE_IPSEC_SHA1HMAC_160:
		*descwd |= OP_PCL_IPSEC_HMAC_SHA1_160;
		break;

	case AUTH_TYPE_IPSEC_SHA2HMAC_256:
		*descwd |= OP_PCL_IPSEC_HMAC_SHA2_256_128;
		break;

	case AUTH_TYPE_IPSEC_SHA2HMAC_384:
		*descwd |= OP_PCL_IPSEC_HMAC_SHA2_384_192;
		break;

	case AUTH_TYPE_IPSEC_SHA2HMAC_512:
		*descwd |= OP_PCL_IPSEC_HMAC_SHA2_512_256;
		break;

	default:
		return 0;
	}

	return descwd++;
}

/**
 * Insert a 802.16 WiMAX protocol OP instruction. These can only be
 * AES-CCM
 *
 * @descwd    = pointer to target descriptor word intended to hold
 *              this command. For an OPERATION command, this is normally
 *              the final word of a single descriptor.
 *
 * @mode      = nonzero is OFDMa, else assume OFDM
 *
 * @dir       = Select DIR_ENCAP for encapsulation, or DIR_DECAP for
 *              decapsulation operations.
 **/
u_int32_t *cmd_insert_proto_op_wimax(u_int32_t *descwd, u_int8_t mode,
				     enum protdir dir)
{
	*descwd = CMD_OPERATION | OP_PCLID_WIMAX |
		  (mode ? OP_PCL_WIMAX_OFDMA : OP_PCL_WIMAX_OFDM);

	return descwd++;
}

/**
 * Insert a 802.11 WiFi protocol OP instruction
 *
 * @descwd    = pointer to target descriptor word intended to hold
 *              this command. For an OPERATION command, this is normally
 *              the final word of a single descriptor.
 *
 * @dir       = Select DIR_ENCAP for encapsulation, or DIR_DECAP for
 *              decapsulation operations.
 **/
u_int32_t *cmd_insert_proto_op_wifi(u_int32_t *descwd, enum protdir dir)
{
	*descwd = CMD_OPERATION | OP_PCLID_WIFI | OP_PCL_WIFI;

	return descwd++;
}

/**
 * Insert a MacSec protocol OP instruction
 *
 * @descwd    = pointer to target descriptor word intended to hold
 *              this command. For an OPERATION command, this is normally
 *              the final word of a single descriptor.
 *
 * @dir       = Select DIR_ENCAP for encapsulation, or DIR_DECAP for
 *              decapsulation operations.
 **/
u_int32_t *cmd_insert_proto_op_macsec(u_int32_t *descwd, enum protdir dir)
{
	*descwd = CMD_OPERATION | OP_PCLID_MACSEC | OP_PCL_MACSEC;

	return descwd++;
}

/**
 * Insert a unidirectional protocol OP instruction
 *
 * cmd_insert_proto_op_unidir()
 *
 * @descwd    = pointer to target descriptor word intended to hold
 *              this command. For an OPERATION command, this is normally
 *              the final word of a single descriptor.
 *
 * @protid    = Select any PROTID field for a unidirectional protocol
 *              from OP_PCLID_
 *
 * #protinfo  = Select constant or bits to accompany protid
 **/
u_int32_t *cmd_insert_proto_op_unidir(u_int32_t *descwd, u_int32_t protid,
				      u_int32_t protinfo)
{
	*descwd = CMD_OPERATION | OP_TYPE_UNI_PROTOCOL | protid | protinfo;

	return descwd++;
}

/**
 * cmd_insert_alg_op()
 * Insert a simple algorithm operation command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd    = pointer to target descriptor word intended to hold
 *              this command.
 *
 * @optype = use as class 1 or 2 with OP_TYPE_CLASSx_ALG
 *
 * @algtype = cipher selection, should be one of ALG_TYPE_
 *
 * @algmode = mode selection, should be one of ALG_MODE_. Some
 *            combinations are ORable depending on application.
 *
 * @mdstate = if a message digest is being processed, determines
 *            the processing state. Can be MDSTATE_UPDATE, MDSTATE_INIT,
 *            MDSTATE_FINAL, or MDSTATE_COMPLETE.
 *
 * @icv = if a message digest, or a cipher with an inclusive authentication
 *        function, then ICV_CHECK_ON selects an inline signature
 *        comparison on the computed result.
 *
 * @dir       = Select DIR_ENCRYPT or DIR_DECRYPT
 **/
u_int32_t *cmd_insert_alg_op(u_int32_t *descwd, u_int32_t optype,
			     u_int32_t algtype, u_int32_t algmode,
			     enum mdstatesel mdstate, enum icvsel icv,
			     enum algdir dir)
{
	*descwd = CMD_OPERATION | optype | algtype | algmode |
		mdstate << OP_ALG_AS_SHIFT |
		icv << OP_ALG_ICV_SHIFT |
		(dir ? OP_ALG_DECRYPT : OP_ALG_ENCRYPT);

	return ++descwd;
}

/**
 * cmd_insert_pkha_op()
 * Insert a PKHA-algorithm operation command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd    = pointer to target descriptor word intended to hold
 *              this command.
 *
 * @pkmode = mode selection, OR of OP_ALG_PKMODE_ from one of 3 sets
 * (clear mem, mod arithmetic, copy mem)
 **/
u_int32_t *cmd_insert_pkha_op(u_int32_t *descwd, u_int32_t pkmode)
{
	*descwd = CMD_OPERATION | OP_TYPE_CLASS1_ALG | OP_ALG_PK | pkmode;

	return ++descwd;
}

/*
 * FIXME: the following two functions are functionally identical
 * 	  and need refactoring, including macro definitions - e.g,
 * 	  the SGF bit doesn't change among commands.
 */
/**
 * cmd_insert_seq_in_ptr()
 * Insert an SEQ IN PTR command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd    = pointer to target descriptor word intended to hold
 *              this command. For an OPERATION command, this is normally
 *              the final word of a single descriptor.
 * @ptr       = bus address pointing to the input data buffer
 * @len       = input length
 * @sgref     = pointer is actual data, or a scatter-gather list
 *              representing the key:
 *              - PTR_DIRECT = points to data
 *              - PTR_SGLIST = points to CAAM-specific scatter gather
 *                table.
 **/
u_int32_t *cmd_insert_seq_in_ptr(u_int32_t *descwd, void *ptr, u_int32_t len,
				 enum ref_type sgref)
{
	*descwd = CMD_SEQ_IN_PTR | ((sgref == PTR_SGLIST) ? SQIN_SGF : 0) | len;

	*(descwd + 1) = (u_int32_t)ptr;

	if (len > 0xffff) {
		*descwd |= SQIN_EXT;
		*(descwd + 2) = len;
		return descwd + 3;
	}

	return descwd + 2;
}

/**
 * cmd_insert_seq_out_ptr()
 * Insert an SEQ OUT PTR command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd    = pointer to target descriptor word intended to hold
 *              this command. For an OPERATION command, this is normally
 *              the final word of a single descriptor.
 * @ptr       = bus address pointing to the output data buffer
 * @len       = output length
 * @sgref     = pointer is actual data, or a scatter-gather list
 *              representing the key:
 *              - PTR_DIRECT = points to data
 *              - PTR_SGLIST = points to CAAM-specific scatter gather
 *                table.
 **/
u_int32_t *cmd_insert_seq_out_ptr(u_int32_t *descwd, void *ptr, u_int32_t len,
				  enum ref_type sgref)
{
	*descwd = CMD_SEQ_OUT_PTR | ((sgref == PTR_SGLIST) ? SQOUT_SGF : 0) |
		  len;

	*(descwd + 1) = (u_int32_t)ptr;

	if (len > 0xffff) {
		*descwd |= SQOUT_EXT;
		*(descwd + 2) = len;
		return descwd + 3;
	}

	return descwd + 2;
}

/**
 * cmd_insert_load()
 * Insert an LOAD command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd       = pointer to target descriptor word intended to hold
 *                 this command. For an OPERATION command, this is normally
 *                 the final word of a single descriptor.
 *
 * @data         = pointer to data to be loaded
 *
 * @class_access = LDST_CLASS_IND_CCB = access class-independent objects in CCB
 *               = LDST_CLASS_1_CCB   = access class 1 objects in CCB
 *               = LDST_CLASS_2_CCB   = access class 2 objects in CCB
 *               = LDST_CLASS_DECO    = access DECO objects
 *
 * @sgflag	 = reference is a scatter/gather list if LDST_SGF
 *
 * @dest         = destination
 *
 * @offset       = the start point for writing in the destination
 *
 * @len          = length of data in bytes
 *
 * @imm          = destination data is to be inlined into descriptor itself
 **/
u_int32_t *cmd_insert_load(u_int32_t *descwd, void *data,
			   u_int32_t class_access, u_int32_t sgflag,
			   u_int32_t dest, u_int8_t offset,
			   u_int8_t len, enum item_inline imm)
{
	int words;
	u_int32_t *nextin;

	*descwd = CMD_LOAD | (class_access & CLASS_MASK) | sgflag | dest |
		  (offset << LDST_OFFSET_SHIFT) | len |
		  ((imm & LDST_IMM_MASK) << LDST_IMM_SHIFT);

	descwd++;

	if (imm == ITEM_INLINE) {
		words = len >> 2;
		nextin = (u_int32_t *)data;
		while (words) {
			*descwd++ = *nextin++;
			words--;
		}
	} else
		*descwd++ = (u_int32_t)data;

	return descwd;
}

/**
 * cmd_insert_fifo_load()
 * Insert a FIFO_LOAD command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd = pointer to target descriptor word intended to hold
 *           this command.
 *
 * @data   = pointer to data to be loaded
 *
 * @len    = length of data in bits (NOT bytes)
 *
 * @class  = LDST_CLASS_IND_CCB = access class-independent objects in CCB
 *         = LDST_CLASS_1_CCB   = access class 1 objects in CCB
 *         = LDST_CLASS_2_CCB   = access class 2 objects in CCB
 *         = LDST_CLASS_DECO    = access DECO objects
 *
 * @sgflag = reference is a scatter/gather list if FIFOLDST_SGF
 *
 * @imm    = destination data is to be inlined into descriptor itself
 *           if FIFOLDST_IMM
 *
 * @ext    = use extended length field following the pointer if
 *           FIFOLDST_EXT
 *
 * @type   = FIFO input type, an OR combination of FIFOLD_TYPE_
 *           type and last/flush bits
 **/
u_int32_t *cmd_insert_fifo_load(u_int32_t *descwd, void *data, u_int32_t len,
				u_int32_t class_access, u_int32_t sgflag,
				u_int32_t imm, u_int32_t ext, u_int32_t type)
{
	int words;
	u_int32_t *nextin;

	*descwd = CMD_FIFO_LOAD | (class_access & CLASS_MASK) | sgflag |
		  imm | ext | type;

	if (!ext)
		*descwd |= (len & FIFOLDST_LEN_MASK);

	descwd++;

	if (imm == FIFOLD_IMM) {
		words = len >> 2;
		nextin = (u_int32_t *)data;
		while (words) {
			*descwd++ = *nextin++;
			words--;
		}
	} else
		*descwd++ = (u_int32_t)data;

	if (ext)
		*descwd++ = len;

	return descwd;
}

/**
 * cmd_insert_seq_load()
 * Insert an SEQ LOAD command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd       = pointer to target descriptor word intended to hold
 *                 this command. For an OPERATION command, this is normally
 *                 the final word of a single descriptor.
 * @class_access = LDST_CLASS_IND_CCB = access class-independent objects in CCB
 *               = LDST_CLASS_1_CCB   = access class 1 objects in CCB
 *               = LDST_CLASS_2_CCB   = access class 2 objects in CCB
 *               = LDST_CLASS_DECO    = access DECO objects
 * @variable_len_flag = use the variable input sequence length
 * @dest         = destination
 * @offset       = the start point for writing in the destination
 * @len          = length of data in bytes
 *
 **/
u_int32_t *cmd_insert_seq_load(u_int32_t *descwd, u_int32_t class_access,
			       u_int32_t variable_len_flag, u_int32_t dest,
			       u_int8_t offset, u_int8_t len)
{
	*descwd = CMD_SEQ_LOAD | (class_access & CLASS_MASK) |
		  (variable_len_flag ? LDST_SGF : 0) |
		  ((dest & LDST_SRCDST_MASK) << LDST_SRCDST_SHIFT) |
		  ((offset & LDST_OFFSET_MASK) << LDST_OFFSET_SHIFT) |
		  ((len & LDST_LEN_MASK) << LDST_LEN_SHIFT);

	return descwd + 1;
}

/**
 * cmd_insert_seq_fifo_load()
 * Insert an SEQ FIFO LOAD command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd       = pointer to target descriptor word intended to hold
 *                 this command. For an OPERATION command, this is normally
 *                 the final word of a single descriptor.
 * @class_access = LDST_CLASS_IND_CCB = access class-independent objects in CCB
 *               = LDST_CLASS_1_CCB   = access class 1 objects in CCB
 *               = LDST_CLASS_2_CCB   = access class 2 objects in CCB
 *               = LDST_CLASS_DECO    = access DECO objects
 * @variable_len_flag = use the variable input sequence length
 * @data_type    = FIFO input data type (FIFOLD_TYPE_* in caam_desc.h)
 * @len          = input length
 **/
u_int32_t *cmd_insert_seq_fifo_load(u_int32_t *descwd, u_int32_t class_access,
				    u_int32_t variable_len_flag,
				    u_int32_t data_type, u_int32_t len)
{
	*descwd = CMD_SEQ_FIFO_LOAD | (class_access & CLASS_MASK) |
		  (variable_len_flag ? FIFOLDST_SGF : 0) |
		  data_type | ((len & LDST_LEN_MASK) << LDST_LEN_SHIFT);

	if (len > 0xffff) {
		*descwd |= FIFOLDST_EXT;
		*(descwd + 1) = len;
		return descwd + 2;
	}

	return descwd + 1;
}

/**
 * cmd_insert_store()
 * Insert a STORE command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd       = pointer to target descriptor word intended to hold
 *                 this command. For an OPERATION command, this is normally
 *                 the final word of a single descriptor.
 *
 * @data         = pointer to data to be stored
 *
 * @class_access = LDST_CLASS_IND_CCB = access class-independent objects in CCB
 *               = LDST_CLASS_1_CCB   = access class 1 objects in CCB
 *               = LDST_CLASS_2_CCB   = access class 2 objects in CCB
 *               = LDST_CLASS_DECO    = access DECO objects
 *
 * @sgflag       = reference is a scatter-gather list if LDST_SGF
 *
 * @src          = source
 *
 * @offset       = the start point for writing in the destination
 *
 * @len          = length of data in bytes
 *
 * @imm          = data is to be inlined into descriptor itself
 **/
u_int32_t *cmd_insert_store(u_int32_t *descwd, void *data,
			    u_int32_t class_access, u_int32_t sg_flag,
			    u_int32_t src, u_int8_t offset,
			    u_int8_t len, enum item_inline imm)
{
	int words;
	u_int32_t *nextin;

	*descwd = CMD_STORE | class_access | sg_flag | src |
		  ((offset & LDST_OFFSET_MASK) << LDST_OFFSET_SHIFT) |
		  ((len & LDST_LEN_MASK) << LDST_LEN_SHIFT) |
		  ((imm & LDST_IMM_MASK) << LDST_IMM_SHIFT);

	descwd++;

	if (imm == ITEM_INLINE) {
		words = len >> 2;
		nextin = (u_int32_t *)data;
		while (words) {
			*descwd++ = *nextin++;
			words--;
		}
	} else
		*descwd++ = (u_int32_t)data;

	return descwd;
}

/**
 * cmd_insert_seq_store()
 * Insert a SEQ STORE command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd       = pointer to target descriptor word intended to hold
 *                 this command. For an OPERATION command, this is normally
 *                 the final word of a single descriptor.
 *
 * @class_access = LDST_CLASS_IND_CCB = access class-independent objects in CCB
 *               = LDST_CLASS_1_CCB   = access class 1 objects in CCB
 *               = LDST_CLASS_2_CCB   = access class 2 objects in CCB
 *               = LDST_CLASS_DECO    = access DECO objects
 *
 * @variable_len_flag = use the variable input sequence length
 *
 * @src          = source
 *
 * @offset       = the start point for writing in the destination
 *
 * @len          = length of data in bytes
 **/
u_int32_t *cmd_insert_seq_store(u_int32_t *descwd, u_int32_t class_access,
				u_int32_t variable_len_flag, u_int32_t src,
				u_int8_t offset, u_int8_t len)
{
	*descwd = CMD_SEQ_STORE | (class_access & CLASS_MASK) |
		  (variable_len_flag ? LDST_SGF : 0) |
		  src | ((offset << LDST_OFFSET_SHIFT) & LDST_OFFSET_MASK) |
		  ((len << LDST_LEN_SHIFT) & LDST_LEN_MASK);

	return descwd + 1;
}

/**
 * cmd_insert_fifo_store()
 * Insert a FIFO_STORE command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd = pointer to target descriptor word intended to hold
 *           this command.
 *
 * @data   = pointer to data to be loaded
 *
 * @len    = length of data in bits (NOT bytes)
 *
 * @class  = LDST_CLASS_IND_CCB = access class-independent objects in CCB
 *         = LDST_CLASS_1_CCB   = access class 1 objects in CCB
 *         = LDST_CLASS_2_CCB   = access class 2 objects in CCB
 *         = LDST_CLASS_DECO    = access DECO objects
 *
 * @sgflag = reference is a scatter/gather list if FIFOLDST_SGF
 *
 * @imm    = destination data is to be inlined into descriptor itself
 *           if FIFOLDST_IMM
 *
 * @ext    = use extended length field following the pointer if
 *           FIFOLDST_EXT
 *
 * @type   = FIFO input type, an OR combination of FIFOLD_TYPE_
 *           type and last/flush bits
 **/
u_int32_t *cmd_insert_fifo_store(u_int32_t *descwd, void *data, u_int32_t len,
				 u_int32_t class_access, u_int32_t sgflag,
				 u_int32_t imm, u_int32_t ext, u_int32_t type)
{
	int words;
	u_int32_t *nextin;

	*descwd = CMD_FIFO_LOAD | (class_access & CLASS_MASK) | sgflag |
		  imm | ext | type;

	if (!ext)
		*descwd |= (len & FIFOLDST_LEN_MASK);

	descwd++;

	if (imm == FIFOLD_IMM) {
		words = len >> 2;
		nextin = (u_int32_t *)data;
		while (words) {
			*descwd++ = *nextin++;
			words--;
		}
	} else
		*descwd++ = (u_int32_t)data;

	if (ext)
		*descwd++ = len;

	return descwd;
}

/**
 * cmd_insert_seq_fifo_store()
 * Insert a SEQ FIFO STORE command into a descriptor
 *
 * Returns: Pointer to next incremental descriptor word past the command
 * just constructed. If an error occurred, returns 0;
 *
 * @descwd       = pointer to target descriptor word intended to hold
 *                 this command. For an OPERATION command, this is normally
 *                 the final word of a single descriptor.
 *
 * @class_access = LDST_CLASS_IND_CCB = access class-independent objects in CCB
 *               = LDST_CLASS_1_CCB   = access class 1 objects in CCB
 *               = LDST_CLASS_2_CCB   = access class 2 objects in CCB
 *               = LDST_CLASS_DECO    = access DECO objects
 *
 * @variable_len_flag = use the variable input sequence length
 *
 * @out_type     = FIFO output data type (FIFOST_TYPE_* in caam_desc.h)
 *
 * @len          = output length
 **/
u_int32_t *cmd_insert_seq_fifo_store(u_int32_t *descwd, u_int32_t class_access,
				     u_int32_t variable_len_flag,
				     u_int32_t out_type, u_int32_t len)
{
	*descwd = CMD_SEQ_FIFO_STORE | (class_access & CLASS_MASK) |
		  (variable_len_flag ? FIFOLDST_SGF : 0) |
		  out_type | ((len & LDST_LEN_MASK) << LDST_LEN_SHIFT);

	if (len > 0xffff) {
		*descwd |= FIFOLDST_EXT;
		*(descwd + 1) = len;
		return descwd + 2;
	}

	return descwd + 1;
}

/**
 * cmd_insert_jump()
 * Insert a JUMP command into a descriptor
 *
 * Returns: pointer to next incremental descriptor word past the command
 * just constructed. No error is returned.
 *
 * @descwd = pointer to target descriptor word intended to hold this
 *           command.
 *
 * @jtype = type of jump operation to perform, of JUMP_TYPE_
 *
 * @test = type of test to perform, one of JUMP_TEST_
 *
 * @cond = condition codes to test agaist. OR combination of any of
 *         JUMP_CC_MATH_ or any of JUMP_CC_MATH_, but not both
 *
 * @offset = Relative offset for jump within the descriptor
 *
 * @jmpdesc = pointer of descriptor to pass control to, only valid
 *            if jtype = JUMP_NONLOCAL
 **/
u_int32_t *cmd_insert_jump(u_int32_t *descwd, u_int32_t jtype,
			   u_int32_t test, u_int32_t cond,
			   u_int8_t offset, u_int32_t *jmpdesc)
{
	*descwd++ = CMD_JUMP | jtype | test | cond | offset;

	if (jtype == JUMP_TYPE_NONLOCAL)
		*descwd++ = (u_int32_t)jmpdesc;

	return descwd;
}

/**
 * cmd_insert_math()
 * Insert a MATH command into a descriptor
 *
 * Returns: pointer to next incremental descriptor word past the command
 * just constructed. No error is returned.
 *
 * @descwd = pointer to target descriptor word intended to hold this
 *           command.
 *
 * @func = Function to perform. One of MATH_FUN_
 *
 * @src0 = First of two value sources for comparison. One of MATH_SRC0_
 *
 * @src1 = Second of two value sources for comparison. One of MATH_SRC1_
 *
 * @dest = Destination for the result. One of MATH_DEST_
 *
 * @len = Length of the ALU (or immediate value) in bytes.
 *
 * @flagupd = if MATH_FLAG_NO_UPDATE, prevents the result from updating
 *            flags, else use MATH_FLAG_UPDATE
 *
 * @stall = if MATH_STALL, cause the instruction to require one extra
 *          clock cycle, else use MATH_NO_STALL.
 *
 * @immediate = if MATH_IMMEDIATE, will insert a 4 byte immediate
 *              value into the descriptor to use as 1 of the two
 *              sources, else if not needed, use MATH_NO_IMMEDIATE
 *
 * @data = inline data sized per len. If MATH_IMMEDIATE is used,
 *           must only be a 4-byte value to inline into the descriptor
 **/
u_int32_t *cmd_insert_math(u_int32_t *descwd, u_int32_t func,
			    u_int32_t src0, u_int32_t src1,
			    u_int32_t dest, u_int32_t len,
			    u_int32_t flagupd, u_int32_t stall,
			    u_int32_t immediate, u_int32_t *data)
 {

	*descwd++ = CMD_MATH | func | src0 | src1 | dest |
		    (len & MATH_LEN_MASK) | flagupd | stall | immediate;

	if ((immediate) ||
	   ((src0 & MATH_SRC0_MASK) == MATH_SRC0_IMM) ||
	   ((src1 & MATH_SRC1_MASK) == MATH_SRC1_IMM))
		*descwd++ = *data;


	return descwd;
 }

/**
 * cmd_insert_move()
 * Insert a MOVE command into a descriptor
 *
 * Returns: pointer to next incremental descriptor word past the
 * command just constructed. No error is returned.
 *
 * @descwd = pointer to target descriptor word intended to hold this
 *           command.
 *
 * @waitcomp = if MOVE_WAITCOMPLETE specified, stall execution until
 *             the MOVE completes. This is only valid if it is using
 *             the DMA CCB, else use MOVE_NOWAIT.
 *
 * @src = defines the source for the move. Must be one of MOVE_SRC_
 *
 * @dst = defined the destination for the move. Must be one of
 *        MOVE_INTSRC_DEST_
 *
 * @offset = specifies the offset into the source or destination
 *           to use.
 *
 * @length = specifies the length of the data to move
 **/
u_int32_t *cmd_insert_move(u_int32_t *descwd, u_int32_t waitcomp,
			   u_int32_t src, u_int32_t dst, u_int8_t offset,
			   u_int8_t length)
{
	*descwd++ = CMD_MOVE | waitcomp | src | dst |
	(offset << MOVE_OFFSET_SHIFT) | length;

	return descwd;
}
