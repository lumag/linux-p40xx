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

/*
 * FIXME: the following two functions are functionally identical
 * 	  and need refactoring, including macro definitions - e.g,
 * 	  the SGF bit doesn't change among commands.
 */
int *cmd_insert_seq_in_ptr(u_int32_t *descwd, u_int32_t *ptr, u_int32_t len,
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

int *cmd_insert_seq_out_ptr(u_int32_t *descwd, u_int32_t *ptr, u_int32_t len,
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

int *cmd_insert_seq_load(u_int32_t *descwd, unsigned int class_access,
			 int variable_len_flag, unsigned char dest,
			 unsigned char offset, unsigned char len)
{
	*descwd = CMD_SEQ_LOAD | (class_access & CLASS_MASK) |
		  (variable_len_flag ? LDST_SGF : 0) |
		  ((dest & LDST_SRCDST_MASK) << LDST_SRCDST_SHIFT) |
		  ((offset & LDST_OFFSET_MASK) << LDST_OFFSET_SHIFT) |
		  ((len & LDST_LEN_MASK) << LDST_LEN_SHIFT);

	return descwd + 1;
}

int *cmd_insert_seq_fifo_load(u_int32_t *descwd, unsigned int class_access,
			      int variable_len_flag, unsigned char data_type,
			      u_int32_t len, u_int32_t *ptr)
{
	*descwd = CMD_SEQ_FIFO_LOAD | (class_access & CLASS_MASK) |
		  (variable_len_flag ? FIFOLDST_SGF : 0) |
		  data_type | ((len & LDST_LEN_MASK) << LDST_LEN_SHIFT);

	*(descwd + 1) = (u_int32_t)ptr;

	if (len > 0xffff) {
		*descwd |= FIFOLDST_EXT;
		*(descwd + 2) = len;
		return descwd + 3;
	}

	return descwd + 2;
}
