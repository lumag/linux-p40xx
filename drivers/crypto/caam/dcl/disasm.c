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
	u_int32_t keylen, *keydata;

	keylen  = *cmd & KEY_LENGTH_MASK;
	keydata = cmd + 1;

	PRINT("    seqkey: len=%d ", keylen);

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

	if (*cmd & KEY_VLF)
		PRINT("variable ");

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

static void show_load(u_int32_t *cmd, u_int8_t *idx)
{
	u_int32_t ldlen, *lddata;

	ldlen  = *cmd & LDST_LEN_MASK;
	lddata = cmd + 1; /* point to key or pointer */

	PRINT("        ld: ");

	switch (*cmd & CLASS_MASK) {
	case LDST_CLASS_IND_CCB:
		PRINT("CCB-class-ind");
		break;

	case LDST_CLASS_1_CCB:
		PRINT("CCB-class1");
		break;

	case LDST_CLASS_2_CCB:
		PRINT("CCB-class2");
		break;

	case LDST_CLASS_DECO:
		PRINT("DECO");
		break;
	}

	if (*cmd & LDST_SGF)
		PRINT(" scatter-gather");

	if (*cmd & LDST_IMM)
		PRINT(" inline");

	switch (*cmd & LDST_SRCDST_MASK) {
	case LDST_SRCDST_BYTE_CONTEXT:
		PRINT(" byte-ctx");
		break;

	case LDST_SRCDST_BYTE_KEY:
		PRINT(" byte-key");
		break;

	case LDST_SRCDST_BYTE_INFIFO:
		PRINT(" byte-infifo");
		break;

	case LDST_SRCDST_BYTE_OUTFIFO:
		PRINT(" byte-outfifo");
		break;

	case LDST_SRCDST_WORD_MODE_REG:
		PRINT(" word-mode");
		break;

	case LDST_SRCDST_WORD_KEYSZ_REG:
		PRINT(" word-keysz");
		break;

	case LDST_SRCDST_WORD_DATASZ_REG:
		PRINT(" word-datasz");
		break;

	case LDST_SRCDST_WORD_ICVSZ_REG:
		PRINT(" word-icvsz");
		break;

	case LDST_SRCDST_WORD_CHACTRL:
		PRINT(" word-cha-ctrl");
		break;

	case LDST_SRCDST_WORD_IRQCTRL:
		PRINT(" word-irq-ctrl");
		break;

	case LDST_SRCDST_WORD_CLRW:
		PRINT(" word-clear");
		break;

	case LDST_SRCDST_WORD_STAT:
		PRINT(" word-status");
		break;

	default:
		PRINT(" <unk-dest>");
		break;
	}

	PRINT(" offset=%d len=%d",
	      (*cmd & LDST_OFFSET_MASK) >> LDST_OFFSET_SHIFT,
	      (*cmd & LDST_LEN_MASK));

	PRINT("\n");

	if (*cmd & LDST_IMM) {
		desc_hexdump(lddata, ldlen >> 2, 4, NULL_LEADER);
		(*idx) += ldlen >> 2;
	} else {
		PRINT("          : @0x%08x\n", *lddata);
		(*idx)++;
	}

	(*idx)++;
}

static void show_seq_load(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("     seqld: ");

	switch (*cmd & CLASS_MASK) {
	case LDST_CLASS_IND_CCB:
		PRINT("CCB-class-ind");
		break;

	case LDST_CLASS_1_CCB:
		PRINT("CCB-class1");
		break;

	case LDST_CLASS_2_CCB:
		PRINT("CCB-class2");
		break;

	case LDST_CLASS_DECO:
		PRINT("DECO");
		break;
	}

	if (*cmd & LDST_VLF)
		PRINT(" variable");

	switch (*cmd & LDST_SRCDST_MASK) {
	case LDST_SRCDST_BYTE_CONTEXT:
		PRINT(" byte-ctx");
		break;

	case LDST_SRCDST_BYTE_KEY:
		PRINT(" byte-key");
		break;

	case LDST_SRCDST_BYTE_INFIFO:
		PRINT(" byte-infifo");
		break;

	case LDST_SRCDST_BYTE_OUTFIFO:
		PRINT(" byte-outfifo");
		break;

	case LDST_SRCDST_WORD_MODE_REG:
		PRINT(" word-mode");
		break;

	case LDST_SRCDST_WORD_KEYSZ_REG:
		PRINT(" word-keysz");
		break;

	case LDST_SRCDST_WORD_DATASZ_REG:
		PRINT(" word-datasz");
		break;

	case LDST_SRCDST_WORD_ICVSZ_REG:
		PRINT(" word-icvsz");
		break;

	case LDST_SRCDST_WORD_CHACTRL:
		PRINT(" word-cha-ctrl");
		break;

	case LDST_SRCDST_WORD_IRQCTRL:
		PRINT(" word-irq-ctrl");
		break;

	case LDST_SRCDST_WORD_CLRW:
		PRINT(" word-clear");
		break;

	case LDST_SRCDST_WORD_STAT:
		PRINT(" word-status");
		break;

	default:
		PRINT(" <unk-dest>");
		break;
	}

	PRINT(" offset=%d len=%d",
	      (*cmd & LDST_OFFSET_MASK) >> LDST_OFFSET_SHIFT,
	      (*cmd & LDST_LEN_MASK));

	PRINT("\n");
	(*idx)++;
}

static void show_fifo_load(u_int32_t *cmd, u_int8_t *idx)
{
	u_int16_t datalen;
	u_int32_t *data;

	data = cmd + 1;

	PRINT("    fifold: class=");
	switch (*cmd & CLASS_MASK) {
	case FIFOLD_CLASS_SKIP:
		PRINT("skip");
		break;

	case FIFOLD_CLASS_CLASS1:
		PRINT("class1");
		break;

	case FIFOLD_CLASS_CLASS2:
		PRINT("class2");
		break;

	case FIFOLD_CLASS_BOTH:
		PRINT("both");
		break;
	}

	if (*cmd & FIFOLDST_SGF_MASK)
		PRINT(" sgf");

	if (*cmd & FIFOLD_IMM_MASK)
		PRINT(" imm");

	if (*cmd & FIFOLDST_EXT_MASK)
		PRINT(" ext");

	PRINT(" type=");
	if ((*cmd & FIFOLD_TYPE_PK_MASK) == FIFOLD_TYPE_PK) {
		PRINT("pk-");
		switch (*cmd * FIFOLD_TYPE_PK_TYPEMASK) {
		case FIFOLD_TYPE_PK_A0:
			PRINT("a0");
			break;

		case FIFOLD_TYPE_PK_A1:
			PRINT("a1");
			break;

		case FIFOLD_TYPE_PK_A2:
			PRINT("a2");
			break;

		case FIFOLD_TYPE_PK_A3:
			PRINT("a3");
			break;

		case FIFOLD_TYPE_PK_B0:
			PRINT("b0");
			break;

		case FIFOLD_TYPE_PK_B1:
			PRINT("b1");
			break;

		case FIFOLD_TYPE_PK_B2:
			PRINT("b2");
			break;

		case FIFOLD_TYPE_PK_B3:
			PRINT("b3");
			break;

		case FIFOLD_TYPE_PK_N:
			PRINT("n");
			break;

		case FIFOLD_TYPE_PK_A:
			PRINT("a");
			break;

		case FIFOLD_TYPE_PK_B:
			PRINT("b");
			break;
		}
	} else {
		switch (*cmd & FIFOLD_TYPE_MSG_MASK)  {
		case FIFOLD_TYPE_MSG:
			PRINT("msg");
			break;

		case FIFOLD_TYPE_MSG1OUT2:
			PRINT("msg1->2");
			break;

		case FIFOLD_TYPE_IV:
			PRINT("IV");
			break;

		case FIFOLD_TYPE_BITDATA:
			PRINT("bit");
			break;

		case FIFOLD_TYPE_AAD:
			PRINT("AAD");
			break;

		case FIFOLD_TYPE_ICV:
			PRINT("ICV");
			break;
		}

		if (*cmd & FIFOLD_TYPE_LAST2)
			PRINT("-l2");

		if (*cmd & FIFOLD_TYPE_LAST1)
			PRINT("-l1");

		if (*cmd & FIFOLD_TYPE_FLUSH1)
			PRINT("-f1");
	}

	datalen = (*cmd & FIFOLDST_LEN_MASK);
	PRINT(" len = %d\n", datalen);
	(*idx)++;

	if (*cmd & FIFOLDST_EXT)
		PRINT("          : extlen=0x%08x\n", (*idx)++);

}

static void show_seq_fifo_load(u_int32_t *cmd, u_int8_t *idx)
{
	u_int16_t datalen;
	u_int32_t *data;

	data = cmd + 1;

	PRINT(" seqfifold: class=");
	switch (*cmd & CLASS_MASK) {
	case FIFOLD_CLASS_SKIP:
		PRINT("skip");
		break;

	case FIFOLD_CLASS_CLASS1:
		PRINT("class1");
		break;

	case FIFOLD_CLASS_CLASS2:
		PRINT("class2");
		break;

	case FIFOLD_CLASS_BOTH:
		PRINT("both");
		break;
	}

	if (*cmd & FIFOLDST_VLF_MASK)
		PRINT(" vlf");

	if (*cmd & FIFOLD_IMM_MASK)
		PRINT(" imm");

	if (*cmd & FIFOLDST_EXT_MASK)
		PRINT(" ext");

	PRINT(" type=");
	if ((*cmd & FIFOLD_TYPE_PK_MASK) == FIFOLD_TYPE_PK) {
		PRINT("pk-");
		switch (*cmd * FIFOLD_TYPE_PK_TYPEMASK) {
		case FIFOLD_TYPE_PK_A0:
			PRINT("a0");
			break;

		case FIFOLD_TYPE_PK_A1:
			PRINT("a1");
			break;

		case FIFOLD_TYPE_PK_A2:
			PRINT("a2");
			break;

		case FIFOLD_TYPE_PK_A3:
			PRINT("a3");
			break;

		case FIFOLD_TYPE_PK_B0:
			PRINT("b0");
			break;

		case FIFOLD_TYPE_PK_B1:
			PRINT("b1");
			break;

		case FIFOLD_TYPE_PK_B2:
			PRINT("b2");
			break;

		case FIFOLD_TYPE_PK_B3:
			PRINT("b3");
			break;

		case FIFOLD_TYPE_PK_N:
			PRINT("n");
			break;

		case FIFOLD_TYPE_PK_A:
			PRINT("a");
			break;

		case FIFOLD_TYPE_PK_B:
			PRINT("b");
			break;
		}
	} else {
		switch (*cmd & FIFOLD_TYPE_MSG_MASK)  {
		case FIFOLD_TYPE_MSG:
			PRINT("msg");
			break;

		case FIFOLD_TYPE_MSG1OUT2:
			PRINT("msg1->2");
			break;

		case FIFOLD_TYPE_IV:
			PRINT("IV");
			break;

		case FIFOLD_TYPE_BITDATA:
			PRINT("bit");
			break;

		case FIFOLD_TYPE_AAD:
			PRINT("AAD");
			break;

		case FIFOLD_TYPE_ICV:
			PRINT("ICV");
			break;
		}

		if (*cmd & FIFOLD_TYPE_LAST2)
			PRINT("-l2");

		if (*cmd & FIFOLD_TYPE_LAST1)
			PRINT("-l1");

		if (*cmd & FIFOLD_TYPE_FLUSH1)
			PRINT("-f1");
	}

	datalen = (*cmd & FIFOLDST_LEN_MASK);
	PRINT(" len = %d\n", datalen);
	(*idx)++;

	if (*cmd & FIFOLDST_EXT)
		PRINT("          : extlen=0x%08x\n", (*idx)++);

}

static void show_store(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("       str: ");
	switch (*cmd & LDST_CLASS_MASK) {
	case LDST_CLASS_IND_CCB:
		PRINT("ccb-indep ");
		break;

	case LDST_CLASS_1_CCB:
		PRINT("ccb-class1 ");
		break;

	case LDST_CLASS_2_CCB:
		PRINT("ccb-class2 ");
		break;

	case LDST_CLASS_DECO:
		PRINT("deco ");
		break;
	}

	if (*cmd & LDST_SGF)
		PRINT("sgf ");

	if (*cmd & LDST_IMM)
		PRINT("imm ");

	PRINT("src=");
	switch (*cmd & LDST_SRCDST_MASK) {
	case LDST_SRCDST_BYTE_CONTEXT:
		PRINT("byte-ctx ");
		break;

	case LDST_SRCDST_BYTE_KEY:
		PRINT("byte-key ");
		break;

	case LDST_SRCDST_WORD_MODE_REG:
		PRINT("word-mode ");
		break;

	case LDST_SRCDST_WORD_KEYSZ_REG:
		PRINT("word-keysz ");
		break;

	case LDST_SRCDST_WORD_DATASZ_REG:
		PRINT("word-datasz ");
		break;

	case LDST_SRCDST_WORD_ICVSZ_REG:
		PRINT("word-icvsz ");
		break;

	case LDST_SRCDST_WORD_CHACTRL:
		PRINT("cha-ctrl ");
		break;

	case LDST_SRCDST_WORD_IRQCTRL:
		PRINT("irq-ctrl ");
		break;

	case LDST_SRCDST_WORD_CLRW:
		PRINT("clr-written ");
		break;

	case LDST_SRCDST_WORD_STAT:
		PRINT("status ");
		break;

	default:
		PRINT("(unk) ");
		break;
	}

	PRINT("offset=%d ", (*cmd & LDST_OFFSET_MASK) >> LDST_OFFSET_SHIFT);
	PRINT("len=%d ", (*cmd & LDST_LEN_MASK) >> LDST_LEN_SHIFT);
	PRINT("\n");
	(*idx)++;
}

static void show_seq_store(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("    seqstr: ");
	switch (*cmd & LDST_CLASS_MASK) {
	case LDST_CLASS_IND_CCB:
		PRINT("ccb-indep ");
		break;

	case LDST_CLASS_1_CCB:
		PRINT("ccb-class1 ");
		break;

	case LDST_CLASS_2_CCB:
		PRINT("ccb-class2 ");
		break;

	case LDST_CLASS_DECO:
		PRINT("deco ");
		break;
	}

	if (*cmd & LDST_VLF)
		PRINT("vlf ");

	if (*cmd & LDST_IMM)
		PRINT("imm ");

	PRINT("src=");
	switch (*cmd & LDST_SRCDST_MASK) {
	case LDST_SRCDST_BYTE_CONTEXT:
		PRINT("byte-ctx ");
		break;

	case LDST_SRCDST_BYTE_KEY:
		PRINT("byte-key ");
		break;

	case LDST_SRCDST_WORD_MODE_REG:
		PRINT("word-mode ");
		break;

	case LDST_SRCDST_WORD_KEYSZ_REG:
		PRINT("word-keysz ");
		break;

	case LDST_SRCDST_WORD_DATASZ_REG:
		PRINT("word-datasz ");
		break;

	case LDST_SRCDST_WORD_ICVSZ_REG:
		PRINT("word-icvsz ");
		break;

	case LDST_SRCDST_WORD_CHACTRL:
		PRINT("cha-ctrl ");
		break;

	case LDST_SRCDST_WORD_IRQCTRL:
		PRINT("irq-ctrl ");
		break;

	case LDST_SRCDST_WORD_CLRW:
		PRINT("clr-written ");
		break;

	case LDST_SRCDST_WORD_STAT:
		PRINT("status ");
		break;

	default:
		PRINT("<unk> ");
		break;
	}

	PRINT("offset=%d ", (*cmd & LDST_OFFSET_MASK) >> LDST_OFFSET_SHIFT);
	PRINT("len=%d ", (*cmd & LDST_LEN_MASK) >> LDST_LEN_SHIFT);
	PRINT("\n");
	(*idx)++;
}

static void show_fifo_store(u_int32_t *cmd, u_int8_t *idx)
{
	u_int16_t datalen;
	u_int32_t *data;

	data = cmd + 1;

	PRINT("   fifostr: class=");
	switch (*cmd & CLASS_MASK) {
	case FIFOST_CLASS_NORMAL:
		PRINT("norm");
		break;

	case FIFOST_CLASS_CLASS1KEY:
		PRINT("class1");
		break;

	case FIFOST_CLASS_CLASS2KEY:
		PRINT("class2");
		break;

	default:
		PRINT("<unk>");
		break;
	}

	if (*cmd & FIFOLDST_SGF_MASK)
		PRINT(" sgf");

	if (*cmd & FIFOST_CONT_MASK)
		PRINT(" cont");

	if (*cmd & FIFOLDST_EXT_MASK)
		PRINT(" ext");

	PRINT(" type=");
	switch (*cmd & FIFOLD_TYPE_MSG_MASK)  {
	case FIFOST_TYPE_PKHA_A0:
		PRINT("pk-a0");
		break;

	case FIFOST_TYPE_PKHA_A1:
		PRINT("pk-a1");
		break;

	case FIFOST_TYPE_PKHA_A2:
		PRINT("pk-a2");
		break;

	case FIFOST_TYPE_PKHA_A3:
		PRINT("pk-a3");
		break;

	case FIFOST_TYPE_PKHA_B0:
		PRINT("pk-b0");
		break;

	case FIFOST_TYPE_PKHA_B1:
		PRINT("pk-b1");
		break;

	case FIFOST_TYPE_PKHA_B2:
		PRINT("pk-b2");
		break;

	case FIFOST_TYPE_PKHA_B3:
		PRINT("pk-b3");
		break;

	case FIFOST_TYPE_PKHA_N:
		PRINT("pk-n");
		break;

	case FIFOST_TYPE_PKHA_A:
		PRINT("pk-a");
		break;

	case FIFOST_TYPE_PKHA_B:
		PRINT("pk-b");
		break;

	case FIFOST_TYPE_AF_SBOX_JKEK:
		PRINT("af-sbox-jkek");
		break;

	case FIFOST_TYPE_AF_SBOX_TKEK:
		PRINT("af-sbox-tkek");
		break;

	case FIFOST_TYPE_PKHA_E_JKEK:
		PRINT("pk-e-jkek");
		break;

	case FIFOST_TYPE_PKHA_E_TKEK:
		PRINT("pk-e-tkek");
		break;

	case FIFOST_TYPE_KEY_KEK:
		PRINT("key-kek");
		break;

	case FIFOST_TYPE_KEY_TKEK:
		PRINT("key-tkek");
		break;

	case FIFOST_TYPE_SPLIT_KEK:
		PRINT("split-kek");
		break;

	case FIFOST_TYPE_SPLIT_TKEK:
		PRINT("split-tkek");
		break;

	case FIFOST_TYPE_OUTFIFO_KEK:
		PRINT("outf-kek");
		break;

	case FIFOST_TYPE_OUTFIFO_TKEK:
		PRINT("outf-tkek");
		break;

	case FIFOST_TYPE_MESSAGE_DATA:
		PRINT("msg");
		break;

	case FIFOST_TYPE_RNGSTORE:
		PRINT("rng");
		break;

	case FIFOST_TYPE_RNGFIFO:
		PRINT("rngf");
		break;

	case FIFOST_TYPE_SKIP:
		PRINT("skip");
		break;

	default:
		PRINT("<unk>");
		break;
	};

	datalen = (*cmd & FIFOLDST_LEN_MASK);
	PRINT(" len = %d\n", datalen);
	(*idx)++;

	if (*cmd & FIFOLDST_EXT)
		PRINT("          : extlen=0x%08x\n", (*idx)++);

	(*idx)++;
}

static void show_seq_fifo_store(u_int32_t *cmd, u_int8_t *idx)
{
	u_int16_t datalen;
	u_int32_t *data;

	data = cmd + 1;

	PRINT("seqfifostr: class=");
	switch (*cmd & CLASS_MASK) {
	case FIFOST_CLASS_NORMAL:
		PRINT("norm");
		break;

	case FIFOST_CLASS_CLASS1KEY:
		PRINT("class1");
		break;

	case FIFOST_CLASS_CLASS2KEY:
		PRINT("class2");
		break;

	default:
		PRINT("<unk>");
		break;
	}

	if (*cmd & FIFOLDST_VLF_MASK)
		PRINT(" vlf");

	if (*cmd & FIFOST_CONT_MASK)
		PRINT(" cont");

	if (*cmd & FIFOLDST_EXT_MASK)
		PRINT(" ext");

	PRINT(" type=");
	switch (*cmd & FIFOLD_TYPE_MSG_MASK)  {
	case FIFOST_TYPE_PKHA_A0:
		PRINT("pk-a0");
		break;

	case FIFOST_TYPE_PKHA_A1:
		PRINT("pk-a1");
		break;

	case FIFOST_TYPE_PKHA_A2:
		PRINT("pk-a2");
		break;

	case FIFOST_TYPE_PKHA_A3:
		PRINT("pk-a3");
		break;

	case FIFOST_TYPE_PKHA_B0:
		PRINT("pk-b0");
		break;

	case FIFOST_TYPE_PKHA_B1:
		PRINT("pk-b1");
		break;

	case FIFOST_TYPE_PKHA_B2:
		PRINT("pk-b2");
		break;

	case FIFOST_TYPE_PKHA_B3:
		PRINT("pk-b3");
		break;

	case FIFOST_TYPE_PKHA_N:
		PRINT("pk-n");
		break;

	case FIFOST_TYPE_PKHA_A:
		PRINT("pk-a");
		break;

	case FIFOST_TYPE_PKHA_B:
		PRINT("pk-b");
		break;

	case FIFOST_TYPE_AF_SBOX_JKEK:
		PRINT("af-sbox-jkek");
		break;

	case FIFOST_TYPE_AF_SBOX_TKEK:
		PRINT("af-sbox-tkek");
		break;

	case FIFOST_TYPE_PKHA_E_JKEK:
		PRINT("pk-e-jkek");
		break;

	case FIFOST_TYPE_PKHA_E_TKEK:
		PRINT("pk-e-tkek");
		break;

	case FIFOST_TYPE_KEY_KEK:
		PRINT("key-kek");
		break;

	case FIFOST_TYPE_KEY_TKEK:
		PRINT("key-tkek");
		break;

	case FIFOST_TYPE_SPLIT_KEK:
		PRINT("split-kek");
		break;

	case FIFOST_TYPE_SPLIT_TKEK:
		PRINT("split-tkek");
		break;

	case FIFOST_TYPE_OUTFIFO_KEK:
		PRINT("outf-kek");
		break;

	case FIFOST_TYPE_OUTFIFO_TKEK:
		PRINT("outf-tkek");
		break;

	case FIFOST_TYPE_MESSAGE_DATA:
		PRINT("msg");
		break;

	case FIFOST_TYPE_RNGSTORE:
		PRINT("rng");
		break;

	case FIFOST_TYPE_RNGFIFO:
		PRINT("rngf");
		break;

	case FIFOST_TYPE_SKIP:
		PRINT("skip");
		break;

	default:
		PRINT("<unk>");
		break;
	};

	datalen = (*cmd & FIFOLDST_LEN_MASK);
	PRINT(" len = %d\n", datalen);
	(*idx)++;

	if (*cmd & FIFOLDST_EXT)
		PRINT("          : extlen=0x%08x\n", (*idx)++);

	(*idx)++;
}

static void show_move(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("      move: ");

	switch (*cmd & MOVE_SRC_MASK) {
	case MOVE_SRC_CLASS1CTX:
		PRINT("class1-ctx");
		break;

	case MOVE_SRC_CLASS2CTX:
		PRINT("class2-ctx");
		break;

	case MOVE_SRC_OUTFIFO:
		PRINT("outfifo");
		break;

	case MOVE_SRC_DESCBUF:
		PRINT("descbuf");
		break;

	case MOVE_SRC_MATH0:
		PRINT("math0");
		break;

	case MOVE_SRC_MATH1:
		PRINT("math1");
		break;

	case MOVE_SRC_MATH2:
		PRINT("math2");
		break;

	case MOVE_SRC_MATH3:
		PRINT("math3");
		break;

	case MOVE_SRC_INFIFO:
		PRINT("infifo");
		break;

	default:
		PRINT("<unk>");
		break;
	}

	PRINT("->");

	switch (*cmd & MOVE_DEST_MASK) {
	case MOVE_DEST_CLASS1CTX:
		PRINT("class1-ctx ");
		break;

	case MOVE_DEST_CLASS2CTX:
		PRINT("class2-ctx ");
		break;

	case MOVE_DEST_OUTFIFO:
		PRINT("outfifo ");
		break;

	case MOVE_DEST_DESCBUF:
		PRINT("descbuf ");
		break;

	case MOVE_DEST_MATH0:
		PRINT("math0 ");
		break;

	case MOVE_DEST_MATH1:
		PRINT("math1 ");
		break;

	case MOVE_DEST_MATH2:
		PRINT("math2 ");
		break;

	case MOVE_DEST_MATH3:
		PRINT("math3 ");
		break;

	case MOVE_DEST_CLASS1INFIFO:
		PRINT("class1-infifo ");
		break;

	case MOVE_DEST_CLASS2INFIFO:
		PRINT("class2-infifo ");
		break;

	case MOVE_DEST_PK_A:
		PRINT("pk-a ");
		break;

	case MOVE_DEST_CLASS1KEY:
		PRINT("class1-key ");
		break;

	case MOVE_DEST_CLASS2KEY:
		PRINT("class2-key ");
		break;

	default:
		PRINT("<unk> ");
		break;
	}

	PRINT("offset=%d ", (*cmd & MOVE_OFFSET_MASK) >> MOVE_OFFSET_SHIFT);

	PRINT("length=%d ", (*cmd & MOVE_LEN_MASK) >> MOVE_LEN_SHIFT);

	if (*cmd & MOVE_WAITCOMP)
		PRINT("wait ");

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

static void decode_class12_op(u_int32_t *cmd)
{
	/* Algorithm type */
	switch (*cmd & OP_ALG_ALGSEL_MASK) {
	case OP_ALG_ALGSEL_AES:
		PRINT("aes ");
		break;

	case OP_ALG_ALGSEL_DES:
		PRINT("des ");
		break;

	case OP_ALG_ALGSEL_3DES:
		PRINT("3des ");
		break;

	case OP_ALG_ALGSEL_ARC4:
		PRINT("arc4 ");
		break;

	case OP_ALG_ALGSEL_MD5:
		PRINT("md5 ");
		break;

	case OP_ALG_ALGSEL_SHA1:
		PRINT("sha1 ");
		break;

	case OP_ALG_ALGSEL_SHA224:
		PRINT("sha224 ");
		break;

	case OP_ALG_ALGSEL_SHA256:
		PRINT("sha256 ");
		break;

	case OP_ALG_ALGSEL_SHA384:
		PRINT("sha384 ");
		break;

	case OP_ALG_ALGSEL_SHA512:
		PRINT("sha512 ");
		break;

	case OP_ALG_ALGSEL_RNG:
		PRINT("rng ");
		break;

	case OP_ALG_ALGSEL_SNOW:
		PRINT("snow ");
		break;

	case OP_ALG_ALGSEL_KASUMI:
		PRINT("kasumi ");
		break;

	case OP_ALG_ALGSEL_CRC:
		PRINT("crc ");
		break;

	default:
		PRINT("unknown-alg ");
	}

	/* Additional info */
	switch (*cmd & OP_ALG_ALGSEL_MASK) {
	case OP_ALG_ALGSEL_AES:
		switch (*cmd & OP_ALG_AAI_MASK) {
		case OP_ALG_AAI_CTR_MOD128:
			PRINT("ctr128 ");
			break;

		case OP_ALG_AAI_CTR_MOD8:
			PRINT("ctr8 ");
			break;

		case OP_ALG_AAI_CTR_MOD16:
			PRINT("ctr16 ");
			break;

		case OP_ALG_AAI_CTR_MOD24:
			PRINT("ctr24 ");
			break;

		case OP_ALG_AAI_CTR_MOD32:
			PRINT("ctr32 ");
			break;

		case OP_ALG_AAI_CTR_MOD40:
			PRINT("ctr40 ");
			break;

		case OP_ALG_AAI_CTR_MOD48:
			PRINT("ctr48 ");
			break;

		case OP_ALG_AAI_CTR_MOD56:
			PRINT("ctr56 ");
			break;

		case OP_ALG_AAI_CTR_MOD64:
			PRINT("ctr64 ");
			break;

		case OP_ALG_AAI_CTR_MOD72:
			PRINT("ctr72 ");
			break;

		case OP_ALG_AAI_CTR_MOD80:
			PRINT("ctr80 ");
			break;

		case OP_ALG_AAI_CTR_MOD88:
			PRINT("ctr88 ");
			break;

		case OP_ALG_AAI_CTR_MOD96:
			PRINT("ctr96 ");
			break;

		case OP_ALG_AAI_CTR_MOD104:
			PRINT("ctr104 ");
			break;

		case OP_ALG_AAI_CTR_MOD112:
			PRINT("ctr112 ");
			break;

		case OP_ALG_AAI_CTR_MOD120:
			PRINT("ctr120 ");
			break;

		case OP_ALG_AAI_CBC:
			PRINT("cbc ");
			break;

		case OP_ALG_AAI_ECB:
			PRINT("ecb ");
			break;

		case OP_ALG_AAI_CFB:
			PRINT("cfb ");
			break;

		case OP_ALG_AAI_OFB:
			PRINT("ofb ");
			break;

		case OP_ALG_AAI_XTS:
			PRINT("xts ");
			break;

		case OP_ALG_AAI_CMAC:
			PRINT("cmac ");
			break;

		case OP_ALG_AAI_XCBC_MAC:
			PRINT("xcbc-mac ");
			break;

		case OP_ALG_AAI_CCM:
			PRINT("ccm ");
			break;

		case OP_ALG_AAI_GCM:
			PRINT("gcm ");
			break;

		case OP_ALG_AAI_CBC_XCBCMAC:
			PRINT("cbc-xcbc-mac ");
			break;

		case OP_ALG_AAI_CTR_XCBCMAC:
			PRINT("ctr-xcbc-mac ");
			break;

		case OP_ALG_AAI_DK:
			PRINT("dk ");
			break;

		}
		break;

	case OP_ALG_ALGSEL_DES:
	case OP_ALG_ALGSEL_3DES:
	switch (*cmd & OP_ALG_AAI_MASK) {
	case OP_ALG_AAI_CBC:
		PRINT("cbc ");
		break;

	case OP_ALG_AAI_ECB:
		PRINT("ecb ");
		break;

	case OP_ALG_AAI_CFB:
		PRINT("cfb ");
		break;

	case OP_ALG_AAI_OFB:
		PRINT("ofb ");
		break;

	case OP_ALG_AAI_CHECKODD:
		PRINT("chkodd ");
		break;
	}
	break;

	case OP_ALG_ALGSEL_RNG:
	switch (*cmd & OP_ALG_AAI_MASK) {
	case OP_ALG_AAI_RNG:
		PRINT("rng ");
		break;

	case OP_ALG_AAI_RNG_NOZERO:
		PRINT("rng-no0 ");
		break;

	case OP_ALG_AAI_RNG_ODD:
		PRINT("rngodd ");
		break;
	}
	break;


	case OP_ALG_ALGSEL_SNOW:
	case OP_ALG_ALGSEL_KASUMI:
	switch (*cmd & OP_ALG_AAI_MASK) {
	case OP_ALG_AAI_F8:
		PRINT("f8 ");
		break;

	case OP_ALG_AAI_F9:
		PRINT("f9 ");
		break;

	case OP_ALG_AAI_GSM:
		PRINT("gsm ");
		break;

	case OP_ALG_AAI_EDGE:
		PRINT("edge ");
		break;
	}
	break;

	case OP_ALG_ALGSEL_CRC:
	switch (*cmd & OP_ALG_AAI_MASK) {
	case OP_ALG_AAI_802:
		PRINT("802 ");
		break;

	case OP_ALG_AAI_3385:
		PRINT("3385 ");
		break;

	case OP_ALG_AAI_CUST_POLY:
		PRINT("custom-poly ");
		break;

	case OP_ALG_AAI_DIS:
		PRINT("dis ");
		break;

	case OP_ALG_AAI_DOS:
		PRINT("dos ");
		break;

	case OP_ALG_AAI_DOC:
		PRINT("doc ");
		break;
	}
	break;

	case OP_ALG_ALGSEL_MD5:
	case OP_ALG_ALGSEL_SHA1:
	case OP_ALG_ALGSEL_SHA224:
	case OP_ALG_ALGSEL_SHA256:
	case OP_ALG_ALGSEL_SHA384:
	case OP_ALG_ALGSEL_SHA512:
	switch (*cmd & OP_ALG_AAI_MASK) {
	case OP_ALG_AAI_HMAC:
		PRINT("hmac ");
		break;

	case OP_ALG_AAI_SMAC:
		PRINT("smac ");
		break;

	case OP_ALG_AAI_HMAC_PRECOMP:
		PRINT("hmac-pre ");
		break;
	}
	break;

	default:
		PRINT("unknown-aai ");
	}

	if (*cmd & OP_ALG_TYPE_MASK) {
		switch (*cmd & OP_ALG_AS_MASK) {
		case OP_ALG_AS_UPDATE:
			PRINT("update ");
			break;

		case OP_ALG_AS_INIT:
			PRINT("init ");
			break;

		case OP_ALG_AS_FINALIZE:
			PRINT("final ");
			break;

		case OP_ALG_AS_INITFINAL:
			PRINT("init-final ");
			break;
		}
	}

	if (*cmd & OP_ALG_ICV_MASK)
		PRINT("icv ");

	if (*cmd & OP_ALG_DIR_MASK)
		PRINT("encrypt ");
	else
		PRINT("decrypt ");

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
		decode_class12_op(cmd);
		break;

	case OP_TYPE_CLASS2_ALG:
		PRINT("class2-op ");
		decode_class12_op(cmd);
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
	switch (*cmd & CLASS_MASK) {
	case CLASS_NONE:
		break;

	case CLASS_1:
		PRINT("class-1 ");
		break;

	case CLASS_2:
		PRINT("class-2 ");
		break;

	case CLASS_BOTH:
		PRINT("class-both ");
		break;
	}


	switch (*cmd & JUMP_TYPE_MASK) {
	case JUMP_TYPE_LOCAL:
		PRINT("local");
		break;

	case JUMP_TYPE_NONLOCAL:
		PRINT("nonlocal");
		break;

	case JUMP_TYPE_HALT:
		PRINT("halt");
		break;

	case JUMP_TYPE_HALT_USER:
		PRINT("halt-user");
		break;
	}

	switch (*cmd & JUMP_TEST_MASK) {
	case JUMP_TEST_ALL:
		PRINT(" all");
		break;

	case JUMP_TEST_INVALL:
		PRINT(" !all");
		break;

	case JUMP_TEST_ANY:
		PRINT(" any");
		break;

	case JUMP_TEST_INVANY:
		PRINT(" !any");
		break;
	}

	if (!(*cmd & JUMP_JSL)) {
		if (*cmd & JUMP_COND_PK_0)
			PRINT(" pk-0");

		if (*cmd & JUMP_COND_PK_GCD_1)
			PRINT(" pk-gcd=1");

		if (*cmd & JUMP_COND_PK_PRIME)
			PRINT(" pk-prime");

		if (*cmd & JUMP_COND_MATH_N)
			PRINT(" math-n");

		if (*cmd & JUMP_COND_MATH_Z)
			PRINT(" math-z");

		if (*cmd & JUMP_COND_MATH_C)
			PRINT(" math-c");

		if (*cmd & JUMP_COND_MATH_NV)
			PRINT(" math-nv");
	} else {
		if (*cmd & JUMP_COND_JQP)
			PRINT(" jq-pend");

		if (*cmd & JUMP_COND_SHRD)
			PRINT(" share-skip");

		if (*cmd & JUMP_COND_SELF)
			PRINT(" share-ctx");

		if (*cmd & JUMP_COND_CALM)
			PRINT(" complete");

		if (*cmd & JUMP_COND_NIP)
			PRINT(" no-input");

		if (*cmd & JUMP_COND_NIFP)
			PRINT(" no-infifo");

		if (*cmd & JUMP_COND_NOP)
			PRINT(" no-output");

		if (*cmd & JUMP_COND_NCP)
			PRINT(" no-ctxld");
	}

	if ((*cmd & JUMP_TYPE_MASK) == JUMP_TYPE_LOCAL) {
		PRINT(" ->offset=%d\n", (*cmd & JUMP_OFFSET_MASK));
		(*idx)++;
	} else {
		PRINT(" ->@0x%08x\n", (*idx + 1));
		*idx += 2;
	}
}

static void show_math(u_int32_t *cmd, u_int8_t *idx)
{
	u_int32_t mathlen, *mathdata;

	mathlen  = *cmd & MATH_LEN_MASK;
	mathdata = cmd + 1;

	PRINT("      math: ");
	if (*cmd & MATH_IFB)
		PRINT("imm4 ");
	if (*cmd & MATH_NFU)
		PRINT("noflag ");
	if (*cmd & MATH_STL)
		PRINT("stall ");

	PRINT("fun=");
	switch (*cmd & MATH_FUN_MASK) {
	case MATH_FUN_ADD:
		PRINT("add");
		break;

	case MATH_FUN_ADDC:
		PRINT("addc");
		break;

	case MATH_FUN_SUB:
		PRINT("sub");
		break;

	case MATH_FUN_SUBB:
		PRINT("subc");
		break;

	case MATH_FUN_OR:
		PRINT("or");
		break;

	case MATH_FUN_AND:
		PRINT("and");
		break;

	case MATH_FUN_XOR:
		PRINT("xor");
		break;

	case MATH_FUN_LSHIFT:
		PRINT("lsh");
		break;

	case MATH_FUN_RSHIFT:
		PRINT("rsh");
		break;

	case MATH_FUN_SHLD:
		PRINT("shld");
		break;
	}


	PRINT(" src0=");
	switch (*cmd & MATH_SRC0_MASK) {
	case MATH_SRC0_REG0:
		PRINT("r0");
		break;

	case MATH_SRC0_REG1:
		PRINT("r1");
		break;

	case MATH_SRC0_REG2:
		PRINT("r2");
		break;

	case MATH_SRC0_REG3:
		PRINT("r3");
		break;

	case MATH_SRC0_IMM:
		PRINT("imm");
		break;

	case MATH_SRC0_SEQINLEN:
		PRINT("seqinlen");
		break;

	case MATH_SRC0_SEQOUTLEN:
		PRINT("seqoutlen");
		break;

	case MATH_SRC0_VARSEQINLEN:
		PRINT("vseqinlen");
		break;

	case MATH_SRC0_VARSEQOUTLEN:
		PRINT("vseqoutlen");
		break;

	case MATH_SRC0_ZERO:
		PRINT("0");
		break;
	};

	PRINT(" src1=");
	switch (*cmd & MATH_SRC1_MASK) {
	case MATH_SRC1_REG0:
		PRINT("r0");
		break;

	case MATH_SRC1_REG1:
		PRINT("r1");
		break;

	case MATH_SRC1_REG2:
		PRINT("r2");
		break;

	case MATH_SRC1_REG3:
		PRINT("r3");
		break;

	case MATH_SRC1_IMM:
		PRINT("imm");
		break;

	case MATH_SRC1_INFIFO:
		PRINT("infifo");
		break;

	case MATH_SRC1_OUTFIFO:
		PRINT("outfifo");
		break;

	case MATH_SRC1_ONE:
		PRINT("1");
		break;

	};

	PRINT(" dest=");
	switch (*cmd & MATH_DEST_MASK) {
	case MATH_DEST_REG0:
		PRINT("r0");
		break;

	case MATH_DEST_REG1:
		PRINT("r1");
		break;

	case MATH_DEST_REG2:
		PRINT("r2");
		break;

	case MATH_DEST_REG3:
		PRINT("r3");
		break;

	case MATH_DEST_SEQINLEN:
		PRINT("seqinlen");
		break;

	case MATH_DEST_SEQOUTLEN:
		PRINT("seqoutlen");
		break;

	case MATH_DEST_VARSEQINLEN:
		PRINT("vseqinlen");
		break;

	case MATH_DEST_VARSEQOUTLEN:
		PRINT("vseqoutlen");
		break;

	case MATH_DEST_NONE:
		PRINT("none");
		break;
	};

	PRINT(" len=%d\n", mathlen);

	(*idx)++;

	if  (((*cmd & MATH_SRC0_MASK) == MATH_SRC0_IMM) ||
	     ((*cmd & MATH_SRC1_MASK) == MATH_SRC1_IMM)) {
		desc_hexdump(cmd + 1, 1, 4, (int8_t *)"            ");
		(*idx)++;
	};
};

static void show_seq_in_ptr(u_int32_t *cmd, u_int8_t *idx)
{
	PRINT("  seqinptr:");
	if (*cmd & SQIN_RBS)
		PRINT(" release buf");
	if (*cmd & SQIN_INL)
		PRINT(" inline");
	if (*cmd & SQIN_SGF)
		PRINT(" s/g");
	if (*cmd & SQIN_PRE) {
		PRINT(" PRE");
	} else {
		PRINT(" @0x%08x", *(cmd + 1));
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
		PRINT(" @0x%08x", *(cmd + 1));
		(*idx)++;
	}
	if (*cmd & SQOUT_EXT)
		PRINT(" EXT");
	else
		PRINT(" len=%d", *cmd & 0xffff);
	PRINT("\n");
	(*idx)++;
}

/**
 * caam_desc_disasm() - Top-level descriptor disassembler
 * @desc - points to the descriptor to disassemble. First command
 *         must be a header, or shared header, and the overall size
 *         is determined by this. Does not handle a QI preheader as
 *         it's first command, and cannot yet follow links in a list
 *         of descriptors
 **/
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
		/*
		 * Show PDB area (that between header and startindex)
		 * Improve PDB content dumps later...
		 */
		desc_hexdump(&desc[1], stidx - 1, 4, (int8_t *)"     (pdb): ");
		idx = stidx;
		break;

	case CMD_DESC_HDR:
		len   = *desc & HDR_DESCLEN_MASK;
		/* if std header, stidx is size of sharedesc */
		stidx = (*desc >> HDR_START_IDX_SHIFT) &
			HDR_START_IDX_MASK;

		show_hdr(desc);
		idx = 1; /* next instruction past header */

		if (*desc & HDR_SHARED) {
			stidx = 2; /* just skip past sharedesc ptr */
			PRINT("            sharedesc->0x%08x\n", desc[1]);
		}
		break;

	default:
		PRINT("caam_desc_disasm(): no header: 0x%08x\n",
		      *desc);
		return;
	}


	/*
	 * Now go process remaining commands in sequence
	 */


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
