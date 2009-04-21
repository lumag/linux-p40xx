/*
 * CAAM Descriptor Construction Library
 * Basic job descriptor construction
 *
 * Copyright (c) 2009, Freescale Semiconductor, Inc.
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
 * cnstr_seq_jobdesc() - Construct simple sequence job descriptor
 * Returns: 0 (for now)
 *
 * @jobdesc - pointer to a buffer to build the target job descriptor
 *            within
 * @jobdescsz - size of target job descriptor buffer
 * @shrdesc - pointer to pre-existing shared descriptor to use with
 *            this job
 * @shrdescsz - size of pre-existing shared descriptor
 * @inbuf - pointer to input frame
 * @insize - size of input frame
 * @outbuf - pointer to output frame
 * @outsize - size of output frame
 *
 * Constructs a simple job descriptor that contains 3 references:
 *   (1) A shared descriptor to do the work. This is normally assumed
 *       to be some sort of a protocol sharedesc, but can be any sharedesc.
 *   (2) A packet/frame for input data
 *   (3) A packet/frame for output data
 *
 * The created descriptor is always a simple reverse-order descriptor,
 * and has no provisions for other content specifications.
 **/
int cnstr_seq_jobdesc(u_int32_t *jobdesc, u_int16_t *jobdescsz,
		      u_int32_t *shrdesc, u_int16_t shrdescsz,
		      void *inbuf, u_int32_t insize,
		      void *outbuf, u_int32_t outsize)
{
	u_int32_t *next;

	/*
	 * Basic structure is
	 * - header (assume sharing, reverse order)
	 * - sharedesc physical address
	 * - SEQ_OUT_PTR
	 * - SEQ_IN_PTR
	 */

	/* Make running pointer past where header will go */
	next = jobdesc;
	next++;

	/* Insert sharedesc */
	*next++ = (u_int32_t)shrdesc;

	/* Sequence pointers */
	next = cmd_insert_seq_out_ptr(next, outbuf, outsize, PTR_DIRECT);
	next = cmd_insert_seq_in_ptr(next, inbuf, insize, PTR_DIRECT);

	/* Now update header */
	*jobdescsz = next - jobdesc; /* add 1 to include header */
	cmd_insert_hdr(jobdesc, shrdescsz, *jobdescsz, SHR_SERIAL,
		       SHRNXT_SHARED, ORDER_REVERSE, DESC_STD);

	return 0;
}





/**
 * Construct a blockcipher request as a single job
 *
 * @descbuf - pointer to buffer for descriptor construction
 * @bufsz - size of constructed descriptor (as output)
 * @data_in - input message
 * @data_out - output message
 * @datasz - size of message
 * @key - cipher key
 * @keylen - size of cipher key
 * @iv - cipher IV
 * @ivlen - size of cipher IV
 * @dir - DIR_ENCRYPT or DIR_DECRYPT
 * @cipher - algorithm from OP_ALG_ALGSEL_
 * @clear - clear descriptor buffer before construction
 **/

int cnstr_jobdesc_blkcipher_cbc(u_int32_t *descbuf, u_int16_t *bufsz,
				u_int8_t *data_in, u_int8_t *data_out,
				u_int32_t datasz,
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
		memset(start, 0, (*bufsz * sizeof(u_int32_t)));

	startidx = descbuf - start;
	descbuf = cmd_insert_seq_in_ptr(descbuf, data_in, datasz,
					PTR_DIRECT);

	descbuf = cmd_insert_seq_out_ptr(descbuf, data_out, datasz,
					 PTR_DIRECT);

	descbuf = cmd_insert_load(descbuf, iv, LDST_CLASS_1_CCB,
				  0, LDST_SRCDST_BYTE_CONTEXT, 0, (ivlen >> 3),
				  ITEM_REFERENCE);

	descbuf = cmd_insert_key(descbuf, key, keylen, PTR_DIRECT,
				 KEYDST_KEYREG, KEY_CLEAR, ITEM_REFERENCE,
				 ITEM_CLASS1);

	mval = 0;
	descbuf = cmd_insert_math(descbuf, MATH_FUN_SUB, MATH_SRC0_SEQINLEN,
				  MATH_SRC1_IMM, MATH_DEST_VARSEQINLEN,
				  4, 0, 0, 0, &mval);

	descbuf = cmd_insert_math(descbuf, MATH_FUN_ADD, MATH_SRC0_SEQINLEN,
				  MATH_SRC1_IMM, MATH_DEST_VARSEQOUTLEN,
				  4, 0, 0, 0, &mval);

	descbuf = cmd_insert_alg_op(descbuf, OP_TYPE_CLASS1_ALG, cipher,
				    OP_ALG_AAI_CBC, MDSTATE_COMPLETE,
				    ICV_CHECK_OFF, dir);

	descbuf = cmd_insert_seq_fifo_load(descbuf, LDST_CLASS_1_CCB,
					   FIFOLDST_VLF,
					   (FIFOLD_TYPE_MSG |
					   FIFOLD_TYPE_LAST1), 0);

	descbuf = cmd_insert_seq_fifo_store(descbuf, LDST_CLASS_1_CCB,
					    FIFOLDST_VLF,
					    FIFOST_TYPE_MESSAGE_DATA, 0);


	/* Now update the header with size/offsets */
	endidx = descbuf - start;
	cmd_insert_hdr(start, 0, endidx, SHR_NEVER, SHRNXT_LENGTH,
		       ORDER_FORWARD, DESC_STD);

	*bufsz = endidx;

	return 0;
}



/**
 * HMAC as a single job
 * @descbuf - descriptor buffer
 * @bufsize - limit/returned descriptor buffer size
 * @msg     - pointer to message being processed
 * @msgsz   - size of message in bytes
 * @digest  - output buffer for digest (size derived from cipher)
 * @key     - key data (size derived from cipher)
 * @cipher  - OP_ALG_ALGSEL_MD5/SHA1-512
 * @icv     - HMAC comparison for ICV, NULL if no check desired
 * @clear   - clear buffer before writing
 **/

int32_t cnstr_jobdesc_hmac(u_int32_t *descbuf, u_int16_t *bufsize,
			   u_int8_t *msg, u_int32_t msgsz, u_int8_t *digest,
			   u_int8_t *key, u_int32_t cipher, u_int8_t *icv,
			   u_int8_t clear)
{
	u_int32_t *start;
	u_int16_t startidx, endidx;
	u_int8_t storelen;
	enum icvsel opicv;

	start = descbuf++;

	if (clear)
		memset(start, 0, (*bufsize * sizeof(u_int32_t)));

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
				 KEYDST_KEYREG, KEY_CLEAR, ITEM_REFERENCE,
				 ITEM_CLASS2);

	descbuf = cmd_insert_fifo_load(descbuf, msg, msgsz, LDST_CLASS_2_CCB,
				       0, 0, 0,
				       (FIFOLD_TYPE_MSG |
					FIFOLD_TYPE_LASTBOTH));

	descbuf = cmd_insert_alg_op(descbuf, OP_TYPE_CLASS2_ALG, cipher,
				    OP_ALG_AAI_HMAC, MDSTATE_COMPLETE,
				    opicv, DIR_ENCRYPT);

	descbuf = cmd_insert_store(descbuf, digest, LDST_CLASS_2_CCB, 0,
				   LDST_SRCDST_BYTE_CONTEXT, 0, storelen,
				   ITEM_REFERENCE);

	endidx = descbuf - start;
	cmd_insert_hdr(start, 0, endidx, SHR_NEVER, SHRNXT_LENGTH,
		       ORDER_FORWARD, DESC_STD);

	*bufsize = endidx;

	return 0;
}
EXPORT_SYMBOL(cnstr_jobdesc_hmac);

/**
 * RSA exponentiation as a single job - cnstr_jobdesc_pkha_rsaexp()
 *
 * @descbuf - pointer to buffer to hold descriptor
 * @bufsiz - pointer to size of written descriptor
 * @pkin - Values of A, B, E, and N
 * @out - Encrypted output
 * @out_siz - size of buffer for encrypted output
 * @clear - nonzero if descriptor buffer space is to be cleared
 *          before construction
 **/

int cnstr_jobdesc_pkha_rsaexp(u_int32_t *descbuf, u_int16_t *bufsz,
			      struct pk_in_params *pkin,
			      u_int8_t *out, u_int32_t out_siz,
			      u_int8_t clear)
{
	u_int32_t *start;
	u_int16_t startidx, endidx;

	start = descbuf++;
	startidx = descbuf - start;

	/* class=1 dest=e len=8 s=#ram_e_data */
	descbuf = cmd_insert_key(descbuf, pkin->e, pkin->e_siz, PTR_DIRECT,
				 KEYDST_PK_E, KEY_CLEAR, ITEM_REFERENCE,
				 ITEM_CLASS1);

	/* class=1 type=n len=8 s=#ram_n_data */
	descbuf = cmd_insert_fifo_load(descbuf, pkin->n, pkin->n_siz,
				       LDST_CLASS_1_CCB, 0, 0, 0,
				       FIFOLD_TYPE_PK_N);

	/* class=1 type=a len=8 s=#ram_a_data */
	descbuf = cmd_insert_fifo_load(descbuf, pkin->a, pkin->a_siz,
				       LDST_CLASS_1_CCB, 0, 0, 0,
				       FIFOLD_TYPE_PK_A);

	/* class=1 type=b len=9 s=#ram_b_data */
	descbuf = cmd_insert_fifo_load(descbuf, pkin->b, pkin->b_siz,
				       LDST_CLASS_1_CCB, 0, 0, 0,
				       FIFOLD_TYPE_PK_B);

	/* alg=8 type=pkha func=f2m_exp dest=b */
	descbuf = cmd_insert_pkha_op(descbuf, OP_ALG_PKMODE_MOD_EXPO);

	/* type=b len=8 d=#ram_exp_data */
	descbuf = cmd_insert_fifo_store(descbuf, out, out_siz,
					LDST_CLASS_1_CCB, 0, 0, 0,
					FIFOST_TYPE_PKHA_B);

	endidx = descbuf - start;
	cmd_insert_hdr(start, 0, endidx, SHR_NEVER, SHRNXT_LENGTH,
		       ORDER_FORWARD, DESC_STD);

	*bufsz = endidx;
	return 0;
}

/* FIXME: clear-written reg content should perhaps be defined in desc.h */
static const u_int32_t clrw_imm = 0x00210000;


/**
 * Binary DSA-verify as a single job - cnstr_jobdesc_dsaverify()
 *
 * @descbuf - pointer to descriptor buffer for construction
 * @bufsz - pointer to size of descriptor constructed (output)
 * @dsadata - pointer to DSA parameters
 * @msg - pointer to input message for verfication
 * @msg_sz - size of message to verify
 * @clear - clear buffer before writing descriptor
 **/

int cnstr_jobdesc_dsaverify(u_int32_t *descbuf, u_int16_t *bufsz,
			    struct dsa_pdb *dsadata, u_int8_t *msg,
			    u_int32_t msg_sz, u_int8_t clear)
{
	u_int32_t *start;
	u_int16_t startidx, endidx;

	start = descbuf++;
	startidx = descbuf - start;

	/* Build 9-word PDB with pointers to params */
	memcpy(descbuf, dsadata, 9 * sizeof(u_int32_t));
	descbuf += 9;

	/* SHA1 hash command */
	descbuf = cmd_insert_alg_op(descbuf, OP_TYPE_CLASS2_ALG,
				    OP_ALG_ALGSEL_SHA1, OP_ALG_AAI_HASH,
				    MDSTATE_COMPLETE, ICV_CHECK_OFF,
				    DIR_DECRYPT);

	/* FIFO load of message */
	descbuf = cmd_insert_fifo_load(descbuf, msg, msg_sz,
				LDST_CLASS_2_CCB, 0, 0, 0,
				FIFOLD_TYPE_MSG | FIFOLD_TYPE_LAST2);

	/* Store SHA-1 hash from context to f */
	descbuf = cmd_insert_store(descbuf, dsadata->f, LDST_CLASS_2_CCB, 0,
			    LDST_SRCDST_BYTE_CONTEXT, 0, 20, ITEM_REFERENCE);

	/* Wait for CALM */
	descbuf = cmd_insert_jump(descbuf, JUMP_TYPE_LOCAL, JUMP_TEST_ALL,
				  JUMP_COND_CALM, 1, NULL);

	/* LOAD immediate to clear-written register */
	descbuf = cmd_insert_load(descbuf, (u_int32_t *)&clrw_imm, CLASS_NONE,
				  0, LDST_SRCDST_WORD_CLRW, 0,
				  sizeof(clrw_imm), ITEM_INLINE);

	/* DSAVERIFY OP | decrypt private | ECC | Binary field */
	descbuf = cmd_insert_proto_op_unidir(descbuf, OP_PCLID_DSAVERIFY,
					     OP_PCL_PKPROT_DECRYPT |
					     OP_PCL_PKPROT_ECC |
					     OP_PCL_PKPROT_F2M);

	/* Header, factor in PDB offset */
	endidx = descbuf - start;
	cmd_insert_hdr(start, startidx, endidx, SHR_NEVER, SHRNXT_LENGTH,
		       ORDER_FORWARD, DESC_STD);

	*bufsz = endidx;
	return 0;
}
