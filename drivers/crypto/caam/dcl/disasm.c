/*
 * CAAM Descriptor Construction Library
 * Descriptor Disassembler
 *
 * This is EXPERIMENTAL and incomplete code. It assumes BE32 for the
 * moment, and much functionality remains to be filled in
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

#define NULL_LEADER (int8_t *)"            "

/*
 * Simple hexdumper for use by the disassembler. Displays 32-bit
 * words on a line-by-line bases with an offset shown,. and an
 * optional indentation/description string to prefix each line with.
 *
 * descdata     - data to dump
 * size         - size of buffer in words
 * wordsperline - number of words to display per line, minimum 1.
 *                4 is a practical maximum using an 80-character line
 * indentstr    - points to a string to ident or identify each line
 */
void desc_hexdump(u_int32_t *descdata,
		  u_int32_t  size,
		  u_int32_t  wordsperline,
		  int8_t    *indentstr)
{
	int i, idx, rem, line;

	idx = 0;
	rem = size;

	while (rem) {
		PRINT("%s[%02d] ", indentstr, idx);
		if (rem <= wordsperline)
			line = rem;
		else
			line = wordsperline;

		for (i = 0; i < line; i++) {
			PRINT("0x%08x ", descdata[idx]);
			rem--; idx++;
		}
		PRINT("\n");
	};

}
EXPORT_SYMBOL(desc_hexdump);


/*
 * FIXME: in general, this should be reworked to eschew cases
 */

static void show_shrhdr(u_int32_t *hdr)
{
	PRINT("   shrdesc: stidx=%d len=%d ",
	       (*hdr >> HDR_START_IDX_SHIFT) & HDR_START_IDX_MASK,
	       *hdr & HDR_DESCLEN_SHR_MASK);

	switch (*hdr & (HDR_SD_SHARE_MASK << HDR_SD_SHARE_SHIFT)) {
	case HDR_SHARE_NEVER:
		PRINT("share-never ");
		break;

	case HDR_SHARE_WAIT:
		PRINT("share-wait ");
		break;

	case HDR_SHARE_SERIAL:
		PRINT("share-serial ");
		break;

	case HDR_SHARE_ALWAYS:
		PRINT("share-always ");
		break;
	}

	if (*hdr & HDR_DNR)
		PRINT("noreplay ");

	if (*hdr & HDR_SAVECTX)
		PRINT("savectx ");

	if (*hdr & HDR_PROP_DNR)
		PRINT("propdnr ");

	PRINT("\n");
}

static void show_hdr(u_int32_t *hdr)
{
	if (*hdr & HDR_SHARED) {
		PRINT("   jobdesc: shrsz=%d len=%d ",
		      (*hdr >> HDR_START_IDX_SHIFT) & HDR_START_IDX_MASK,
		      *hdr & HDR_DESCLEN_MASK);
	} else {
		PRINT("   jobdesc: stidx=%d len=%d ",
		      (*hdr >> HDR_START_IDX_SHIFT) & HDR_START_IDX_MASK,
		      *hdr & HDR_DESCLEN_MASK);
	}
	switch (*hdr & (HDR_JD_SHARE_MASK << HDR_JD_SHARE_SHIFT)) {
	case HDR_SHARE_NEVER:
		PRINT("share-never ");
		break;

	case HDR_SHARE_WAIT:
		PRINT("share-wait ");
		break;

	case HDR_SHARE_SERIAL:
		PRINT("share-serial ");
		break;

	case HDR_SHARE_ALWAYS:
		PRINT("share-always ");
		break;

	case HDR_SHARE_DEFER:
		PRINT("share-defer ");
		break;
	}

	if (*hdr & HDR_DNR)
		PRINT("noreplay ");

	if (*hdr & HDR_TRUSTED)
		PRINT("trusted ");

	if (*hdr & HDR_MAKE_TRUSTED)
		PRINT("mktrusted ");

	if (*hdr & HDR_SHARED)
		PRINT("getshared ");

	if (*hdr & HDR_REVERSE)
		PRINT("reversed ");

	PRINT("\n");
}

static void show_key(u_int32_t *cmd, u_int8_t *idx)
{
	u_int32_t keylen, *keydata;

	keylen  = *cmd & KEY_LENGTH_MASK;
	keydata = cmd + 1; /* point to key or pointer */

	PRINT("       key: len=%d ", keylen);

	switch (*cmd & CLASS_MASK) {
	case CLASS_1:
		PRINT("class1");
		break;

	case CLASS_2:
		PRINT("class2");
		break;

	}

	switch (*cmd & KEY_DEST_MASK) {
	case KEY_DEST_CLASS_REG:
		PRINT("->keyreg ");
		break;

	case KEY_DEST_PKHA_E:
		PRINT("->pk-e ");
		break;

	case KEY_DEST_AFHA_SBOX:
		PRINT("->af-sbox ");
		break;

	case KEY_DEST_MDHA_SPLIT:
		PRINT("->md-split ");
		break;
	}

	if (*cmd & KEY_SGF)
		PRINT("scattered ");

	if (*cmd & KEY_ENC)
		PRINT("encrypted ");

	if (*cmd & KEY_IMM)
		PRINT("inline ");

	PRINT("\n");

	(*idx)++;

	if (*cmd & KEY_IMM) {
		desc_hexdump(keydata, keylen >> 2, 4, NULL_LEADER);
		(*idx) += keylen >> 2;
	} else {
		PRINT("          : @0x%08x\n", *keydata);
		(*idx)++;
	}
}

static void show_seq_key(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("    seqkey: ");
	PRINT("\n");
	(*idx)++;
}

static void show_load(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("        ld: ");
	PRINT("\n");
	(*idx)++;
}

static void show_seq_load(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("     seqld: ");

	switch (*cmd & CLASS_MASK) {
	case LDST_CLASS_IND_CCB:
		PRINT("CCB class-indep. access");
		break;

	case LDST_CLASS_1_CCB:
		PRINT("CCB class 1 access");
		break;

	case LDST_CLASS_2_CCB:
		PRINT("CCB class 2 access");
		break;

	case LDST_CLASS_DECO:
		PRINT("DECO access");
		break;
	}

	if (*cmd & LDST_SGF)
		PRINT(" scatter-gather");

	PRINT(" dest=%d offset=%d len=%d",
	      (*cmd >> LDST_SRCDST_SHIFT) & LDST_SRCDST_MASK,
	      (*cmd >> LDST_OFFSET_SHIFT) & LDST_OFFSET_MASK,
	      *cmd & LDST_LEN_MASK);

	PRINT("\n");
	(*idx)++;
}

static void show_fifo_load(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("    fifold: ");
	PRINT("\n");
	(*idx)++;
}

static void show_seq_fifo_load(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT(" seqfifold: ");
	PRINT("\n");
	(*idx)++;
}

static void show_store(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("       str: ");
	PRINT("\n");
	(*idx)++;
}

static void show_seq_store(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("    seqstr: ");
	PRINT("\n");
	(*idx)++;
}

static void show_fifo_store(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("   fifostr: ");
	PRINT("\n");
	(*idx)++;
}

static void show_seq_fifo_store(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("seqfifostr: ");
	PRINT("\n");
	(*idx)++;
}

static void show_move(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("      move: ");
	PRINT("\n");
	(*idx)++;
}

static void decode_unidir_pcl_op(u_int32_t *cmd)
{
	switch (*cmd & OP_PCLID_MASK) {
	case OP_PCLID_IKEV1_PRF:
		PRINT("ike_v1_prf ");
		break;
	case OP_PCLID_IKEV2_PRF:
		PRINT("ike_v2_prf ");
		break;
	case OP_PCLID_SSL30_PRF:
		PRINT("ssl3.0_prf ");
		break;
	case OP_PCLID_TLS10_PRF:
		PRINT("tls1.0_prf ");
		break;
	case OP_PCLID_TLS11_PRF:
		PRINT("tls1.1_prf ");
		break;
	case OP_PCLID_DTLS10_PRF:
		PRINT("dtls1.0_prf ");
		break;
	}
}

static void decode_ipsec_pclinfo(u_int32_t *cmd)
{
	switch (*cmd & OP_PCL_IPSEC_CIPHER_MASK) {
	case OP_PCL_IPSEC_DES_IV64:
		PRINT("des-iv64 ");
		break;

	case OP_PCL_IPSEC_DES:
		PRINT("des ");
		break;

	case OP_PCL_IPSEC_3DES:
		PRINT("3des ");
		break;

	case OP_PCL_IPSEC_AES_CBC:
		PRINT("aes-cbc ");
		break;

	case OP_PCL_IPSEC_AES_CTR:
		PRINT("aes-ctr ");
		break;

	case OP_PCL_IPSEC_AES_XTS:
		PRINT("aes-xts ");
		break;

	case OP_PCL_IPSEC_AES_CCM8:
		PRINT("aes-ccm8 ");
		break;

	case OP_PCL_IPSEC_AES_CCM12:
		PRINT("aes-ccm12 ");
		break;

	case OP_PCL_IPSEC_AES_CCM16:
		PRINT("aes-ccm16 ");
		break;

	case OP_PCL_IPSEC_AES_GCM8:
		PRINT("aes-ccm8 ");
		break;

	case OP_PCL_IPSEC_AES_GCM12:
		PRINT("aes-ccm12 ");
		break;

	case OP_PCL_IPSEC_AES_GCM16:
		PRINT("aes-ccm16 ");
		break;
	}

	switch (*cmd & OP_PCL_IPSEC_AUTH_MASK) {
	case OP_PCL_IPSEC_HMAC_NULL:
		PRINT("hmac-null ");
		break;

	case OP_PCL_IPSEC_HMAC_MD5_96:
		PRINT("hmac-md5-96 ");
		break;

	case OP_PCL_IPSEC_HMAC_SHA1_96:
		PRINT("hmac-sha1-96 ");
		break;

	case OP_PCL_IPSEC_AES_XCBC_MAC_96:
		PRINT("aes-xcbcmac-96 ");
		break;

	case OP_PCL_IPSEC_HMAC_MD5_128:
		PRINT("hmac-md5-128 ");
		break;

	case OP_PCL_IPSEC_HMAC_SHA1_160:
		PRINT("hmac-sha1-160 ");
		break;

	case OP_PCL_IPSEC_HMAC_SHA2_256_128:
		PRINT("hmac-sha2-256-128 ");
		break;

	case OP_PCL_IPSEC_HMAC_SHA2_384_192:
		PRINT("hmac-sha2-384-192 ");
		break;

	case OP_PCL_IPSEC_HMAC_SHA2_512_256:
		PRINT("hmac-sha2-512-256 ");
		break;
	}
}

/* need a BUNCH of these decoded... */
static void decode_bidir_pcl_op(u_int32_t *cmd)
{
	switch (*cmd & OP_PCLID_MASK) {
	case OP_PCLID_IPSEC:
		PRINT("ipsec ");
		decode_ipsec_pclinfo(cmd);
		break;

	case OP_PCLID_SRTP:
		PRINT("srtp ");
		PRINT("pclinfo=0x%04x ", *cmd & OP_PCLINFO_MASK);
		break;

	case OP_PCLID_MACSEC:
		PRINT("macsec ");
		PRINT("pclinfo=0x%04x ", *cmd & OP_PCLINFO_MASK);
		break;

	case OP_PCLID_WIFI:
		PRINT("wifi ");
		PRINT("pclinfo=0x%04x ", *cmd & OP_PCLINFO_MASK);
		break;

	case OP_PCLID_WIMAX:
		PRINT("wimax ");
		PRINT("pclinfo=0x%04x ", *cmd & OP_PCLINFO_MASK);
		break;

	case OP_PCLID_SSL30:
		PRINT("ssl3.0 ");
		PRINT("pclinfo=0x%04x ", *cmd & OP_PCLINFO_MASK);
		break;

	case OP_PCLID_TLS10:
		PRINT("tls1.0 ");
		PRINT("pclinfo=0x%04x ", *cmd & OP_PCLINFO_MASK);
		break;

	case OP_PCLID_TLS11:
		PRINT("tls1.1 ");
		PRINT("pclinfo=0x%04x ", *cmd & OP_PCLINFO_MASK);
		break;

	case OP_PCLID_TLS12:
		PRINT("tls1.2 ");
		PRINT("pclinfo=0x%04x ", *cmd & OP_PCLINFO_MASK);
		break;

	case OP_PCLID_DTLS:
		PRINT("dtls ");
		PRINT("pclinfo=0x%04x ", *cmd & OP_PCLINFO_MASK);
		break;
	}
}

static void decode_pk_op(u_int32_t *cmd)
{
	switch (*cmd & OP_PCLID_MASK) {
	case OP_PCLID_PRF:
		PRINT("prf ");
		break;
	case OP_PCLID_BLOB:
		PRINT("sm-blob ");
		break;
	case OP_PCLID_SECRETKEY:
		PRINT("secret-key ");
		break;
	case OP_PCLID_PUBLICKEYPAIR:
		PRINT("pk-pair ");
		break;
	case OP_PCLID_DSASIGN:
		PRINT("dsa-sign ");
		break;
	case OP_PCLID_DSAVERIFY:
		PRINT("dsa-verify ");
		break;
	}
}

static void decode_class1_op(u_int32_t *cmd)
{
}

static void decode_class2_op(u_int32_t *cmd)
{
}

static void show_op(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT(" operation: type=");

	switch (*cmd & OP_TYPE_MASK) {
	case OP_TYPE_UNI_PROTOCOL:
		PRINT("unidir-pcl ");
		decode_unidir_pcl_op(cmd);
		break;

	case OP_TYPE_PK:
		PRINT("public-key ");
		decode_pk_op(cmd);
		break;

	case OP_TYPE_CLASS1_ALG:
		PRINT("class1-op ");
		decode_class1_op(cmd);
		break;

	case OP_TYPE_CLASS2_ALG:
		PRINT("class2-op ");
		decode_class2_op(cmd);
		break;

	case OP_TYPE_DECAP_PROTOCOL:
		PRINT("decap-pcl ");
		decode_bidir_pcl_op(cmd);
		break;

	case OP_TYPE_ENCAP_PROTOCOL:
		PRINT("encap-pcl ");
		decode_bidir_pcl_op(cmd);
		break;
	}
	PRINT("\n");
	(*idx)++;
}

static void show_signature(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT(" signature: ");
	PRINT("\n");
	(*idx)++;
}

static void show_jump(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("      jump: ");
	PRINT("\n");
	(*idx)++;
}

static void show_math(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("      math: ");
	PRINT("\n");
	(*idx)++;
}

static void show_seq_in_ptr(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("  seqinptr:");
	if (*cmd & SQIN_RBS)
		PRINT(" release buf");
	if (*cmd & SQIN_INL)
		PRINT(" inline");
	if (*cmd & SQIN_SGF)
		PRINT(" scatter-gather");
	if (*cmd & SQIN_PRE) {
		PRINT(" PRE");
	} else {
		PRINT(" ptr=0x%08x", *(cmd + 1));
		(*idx)++;
	}
	if (*cmd & SQIN_EXT)
		PRINT(" EXT");
	else
		PRINT(" len=%d", *cmd & 0xffff);
	if (*cmd & SQIN_RTO)
		PRINT(" RTO");
	PRINT("\n");
	(*idx)++;
}

static void show_seq_out_ptr(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT(" seqoutptr:");
	if (*cmd & SQOUT_SGF)
		PRINT(" scatter-gather");
	if (*cmd & SQOUT_PRE) {
		PRINT(" PRE");
	} else {
		PRINT(" ptr=0x%08x", *(cmd + 1));
		(*idx)++;
	}
	if (*cmd & SQOUT_EXT)
		PRINT(" EXT");
	else
		PRINT(" len=%d", *cmd & 0xffff);
	PRINT("\n");
	(*idx)++;
}

/*
 * Top-level descriptor disassembler
 *
 * desc - points to the descriptor to disassemble. First command
 *        must be a header, or shared header, and the overall size
 *        is determined by this. Does not handle a QI preheader as
 *        it's first command, and cannot yet follow links in a list
 *        of descriptors
 */
void caam_desc_disasm(u_int32_t *desc)
{
	u_int8_t   len, idx, stidx;

	stidx  = 0;

	/*
	 * First word must be header or shared header, or we're done
	 * If we have a valid header, save off indices and size for
	 * determining descriptor area boundaries
	 */
	switch (*desc & CMD_MASK) {
	case CMD_SHARED_DESC_HDR:
		len   = *desc & HDR_DESCLEN_SHR_MASK;
		/* if shared, stidx becomes size of PDB space */
		stidx = (*desc >> HDR_START_IDX_SHIFT) &
			HDR_START_IDX_MASK;
		show_shrhdr(desc);
		break;

	case CMD_DESC_HDR:
		len   = *desc & HDR_DESCLEN_MASK;
		/* if std header, stidx is size of sharedesc */
		stidx = (*desc >> HDR_START_IDX_SHIFT) &
			HDR_START_IDX_MASK;

		show_hdr(desc);

		if (*desc & HDR_SHARED) {
			stidx = 2; /* just skip past sharedesc ptr */
			PRINT("            sharedesc->0x%08x\n", desc[1]);
		}
		break;

	default:
		PRINT("caam_desc_disasm() no initial header: 0x%08x\n",
		      *desc);
		return;
	}

	/*
	 * Show PDB area (that between header and startindex)
	 * Improve PDB content dumps later...
	 */
	desc_hexdump(&desc[1], stidx - 1, 4, (int8_t *)"     (pdb): ");

	/*
	 * Now go process remaining commands in sequence
	 */

	idx = stidx;

	while (idx < len) {
		switch (desc[idx] & CMD_MASK) {
		case CMD_KEY:
			show_key(&desc[idx], &idx);
			break;

		case CMD_SEQ_KEY:
			show_seq_key(&desc[idx], &idx);
			break;

		case CMD_LOAD:
			show_load(&desc[idx], &idx);
			break;

		case CMD_SEQ_LOAD:
			show_seq_load(&desc[idx], &idx);
			break;

		case CMD_FIFO_LOAD:
			show_fifo_load(&desc[idx], &idx);
			break;

		case CMD_SEQ_FIFO_LOAD:
			show_seq_fifo_load(&desc[idx], &idx);
			break;

		case CMD_STORE:
			show_store(&desc[idx], &idx);
			break;

		case CMD_SEQ_STORE:
			show_seq_store(&desc[idx], &idx);
			break;

		case CMD_FIFO_STORE:
			show_fifo_store(&desc[idx], &idx);
			break;

		case CMD_SEQ_FIFO_STORE:
			show_seq_fifo_store(&desc[idx], &idx);
			break;

		case CMD_MOVE:
			show_move(&desc[idx], &idx);
			break;

		case CMD_OPERATION:
			show_op(&desc[idx], &idx);
			break;

		case CMD_SIGNATURE:
			show_signature(&desc[idx], &idx);
			break;

		case CMD_JUMP:
			show_jump(&desc[idx], &idx);
			break;

		case CMD_MATH:
			show_math(&desc[idx], &idx);
			break;

		case CMD_SEQ_IN_PTR:
			show_seq_in_ptr(&desc[idx], &idx);
			break;

		case CMD_SEQ_OUT_PTR:
			show_seq_out_ptr(&desc[idx], &idx);
			break;

		default:
			PRINT("<unrecognized command>: 0x%08x\n",
			      desc[idx++]);
			break;
		}
	}
}
EXPORT_SYMBOL(caam_desc_disasm);
