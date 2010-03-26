/*
 * caam - Freescale Integrated Security Engine (SEC) device driver
 *
 * Copyright (c) 2008,2009 Freescale Semiconductor, Inc.
 *
 * Based on talitos Scatterlist Crypto API driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * relationship of job descriptors to shared descriptors (SteveC Dec 10 2008):
 *
 * ---------------                     ---------------
 * | JobDesc #1  |-------------------->|  ShareDesc  |
 * | *(packet 1) |                     |   (PDB)     |
 * ---------------      |------------->|  (hashKey)  |
 *       .              |              | (cipherKey) |
 *       .              |    |-------->| (operation) |
 * ---------------      |    |         ---------------
 * | JobDesc #2  |------|    |
 * | *(packet 2) |           |
 * ---------------           |
 *       .                   |
 *       .                   |
 * ---------------           |
 * | JobDesc #3  |------------
 * | *(packet 3) |
 * ---------------
 *
 * The SharedDesc never changes for a connection unless rekeyed, but
 * each packet will likely be in a different place. So all we need
 * to know to process the packet is where the input is, where the
 * output goes, and what context we want to process with. Context is
 * in the SharedDesc, packet references in the JobDesc.
 *
 * So, a job desc looks like:
 *
 * ---------------------
 * | Header            |
 * | ShareDesc Pointer |
 * | SEQ_OUT_PTR       |
 * | (output buffer)   |
 * | SEQ_IN_PTR        |
 * | (input buffer)    |
 * | LOAD (to DECO)    |
 * ---------------------
 */

#include "compat.h"
#include "regs.h"
#include "intern.h"
#include "desc.h"
#include "jq.h"
#include "dcl/dcl.h"

/*
 * crypto alg
 */
#define CAAM_CRA_PRIORITY		3000
#define CAAM_MAX_KEY_SIZE		64
/* max IV is max of AES_BLOCK_SIZE, DES3_EDE_BLOCK_SIZE */
#define CAAM_MAX_IV_LENGTH		16

/* hardcoded for now, should probably get from packet data IHL field */
#define ALGAPI_IP_HDR_LEN 20

#ifdef DEBUG
/* for print_hex_dumps with line references */
#define xstr(s) str(s)
#define str(s) #s
#define debug(format, arg...) printk(format, arg)
#else
#define debug(format, arg...)
#endif

/*
 * per-session context
 */
struct caam_ctx {
	struct device *dev;
	int class1_alg_type;
	int class2_alg_type;
	u8 key[CAAM_MAX_KEY_SIZE];
	unsigned int keylen;
	unsigned int enckeylen;
	unsigned int authkeylen;
	unsigned int authsize;
	u32 *shared_desc_encap;
	u32 *shared_desc_decap;
	u32 shared_desc_encap_phys;
	u32 shared_desc_decap_phys;
	int shared_desc_encap_len;
	int shared_desc_decap_len;
	spinlock_t first_lock;
};

static int aead_authenc_setauthsize(struct crypto_aead *authenc,
				    unsigned int authsize)
{
	struct caam_ctx *ctx = crypto_aead_ctx(authenc);

	debug("setauthsize: authsize %d\n", authsize);

	switch (authsize) {
	case 12: ctx->class2_alg_type = AUTH_TYPE_IPSEC_SHA1HMAC_96;
		 break;
	case 20: ctx->class2_alg_type = AUTH_TYPE_IPSEC_SHA1HMAC_160;
		 break;
	}

	ctx->authsize = authsize;

	return 0;
}

static int build_protocol_desc_ipsec_decap(struct caam_ctx *ctx,
					   struct aead_request *req)
{
	gfp_t flags = req->base.flags & CRYPTO_TFM_REQ_MAY_SLEEP ? GFP_KERNEL :
		      GFP_ATOMIC;
	u32 *shdesc, *shdescptr;
	int startidx, endidx;

	/* build shared descriptor for this session */
	shdesc = kzalloc(36 /* minimum for this protocol */ +
			 (sizeof(u32) + CAAM_MAX_KEY_SIZE) * 2 +
			 ALGAPI_IP_HDR_LEN, GFP_DMA | flags);
	if (!shdesc) {
		dev_err(ctx->dev, "could not allocate shared descriptor\n");
		return -ENOMEM;
	}

	shdescptr = shdesc;

	/* skip shared header (filled in last) */
	shdescptr++;

	/*
	 * options byte:
	 * ipv4
	 * ip hdr len: fixed 20.
	 * next hdr offset: fixed 9.
	 * linux doesn't support Extended Sequence Numbers
	 * as of time of writing: PDBOPTS_ESPCBC_ESN not set.
	 */
	*shdescptr++ = ALGAPI_IP_HDR_LEN << 16 |
		       9 << 8 | /* next hdr offset (9 bytes in) */
		       PDBOPTS_ESPCBC_OUTFMT | /* decapsulated output only */
		       PDBOPTS_ESPCBC_TUNNEL; /* transport not supported yet */
	/*
	 * our choice of protocol operation descriptor command
	 * requires we pretend we have a full-fledged, 36-byte pdb
	 */

	/* Skip reserveds */
	shdescptr += 2;

	/* Skip optional ESN */
	shdescptr++;

	/* copy Seq. Num */
	*shdescptr++ = (u32 *)((u32 *)sg_virt(req->assoc) + 1);

	/* Skip ARS */
	shdescptr += 2;

	/* Save current location for computing start index */
	startidx = shdescptr - shdesc;

	/*
	 * process keys, starting with class 2/authentication
	 * This is assuming keys are immediate for sharedesc
	 */
	shdescptr = cmd_insert_key(shdescptr, ctx->key,
				   ctx->authkeylen * 8, PTR_DIRECT,
				   KEYDST_KEYREG, KEY_CLEAR, ITEM_INLINE,
				   ITEM_CLASS2);

	/* class 1/cipher key */
	shdescptr = cmd_insert_key(shdescptr, ctx->key + ctx->authkeylen,
				   ctx->enckeylen * 8, PTR_DIRECT,
				   KEYDST_KEYREG, KEY_CLEAR, ITEM_INLINE,
				   ITEM_CLASS1);

	/* insert the operation command */
	shdescptr = cmd_insert_proto_op_ipsec(shdescptr, ctx->class1_alg_type,
					      ctx->class2_alg_type, DIR_DECAP);

	/* update the header with size/offsets */
	endidx = shdescptr - shdesc + 1; /* add 1 to include header */

	cmd_insert_shared_hdr(shdesc, startidx, endidx, CTX_ERASE, SHR_SERIAL);

#ifdef DEBUG
	print_hex_dump(KERN_ERR, "shrdesc@"xstr(__LINE__)": ",
		       DUMP_PREFIX_ADDRESS, 16, 4, shdesc,
		       (shdescptr - shdesc + 1) * 4, 1);
	caam_desc_disasm(shdesc);
#endif

	ctx->shared_desc_decap_len = endidx * sizeof(u32);

	/* now we know the length, stop wasting preallocated shdesc space */
	ctx->shared_desc_decap = krealloc(shdesc, ctx->shared_desc_decap_len,
					  GFP_DMA | flags);

	ctx->shared_desc_decap_phys = dma_map_single(ctx->dev, shdesc,
						     endidx * sizeof(u32),
						     DMA_BIDIRECTIONAL);
	if (dma_mapping_error(ctx->dev, ctx->shared_desc_decap_phys)) {
		dev_err(ctx->dev, "unable to map shared descriptor\n");
		kfree(ctx->shared_desc_decap);
		return -ENOMEM;
	}

	return 0;
}

static int build_protocol_desc_ipsec_encap(struct caam_ctx *ctx,
					   struct aead_request *areq)
{
	gfp_t flags = areq->base.flags & CRYPTO_TFM_REQ_MAY_SLEEP ? GFP_KERNEL :
		      GFP_ATOMIC;
	int  startidx, endidx;
	u32 *shdesc, *shdescptr;

	/* build shared descriptor for this session */
	shdesc = kzalloc(36 /* minimum for this protocol */ +
			 (sizeof(u32) + CAAM_MAX_KEY_SIZE) * 2 +
			 ALGAPI_IP_HDR_LEN, GFP_DMA | flags);
	if (!shdesc) {
		dev_err(ctx->dev, "could not allocate shared descriptor\n");
		return -ENOMEM;
	}

	shdescptr = shdesc;

	/* skip shared header (filled in last) */
	shdescptr++;

	/*
	 * next header is IPv4 (fixed for tunnel mode)
	 * options byte: IVsrc is RNG
         * we do not Prepend IP header to output frame
	 */
	*shdescptr++ = 4 << 16 | /* next hdr = IPv4 */
		       9 << 8 | /* next hdr offset (9 bytes in) */
		       PDBOPTS_ESPCBC_IPHDRSRC | /* IP header comes from PDB */
#if !defined(DEBUG)
		       PDBOPTS_ESPCBC_IVSRC  | /* IV src is RNG */
#endif
		       PDBOPTS_ESPCBC_TUNNEL; /* transport not supported yet */
	/*
	 * need to pretend we have a full fledged pdb, otherwise get:
	 * [caam error] IPsec encapsulation: PDB is only 4 bytes, \
	 * expected at least 36 bytes
	 */

	/* Skip sequence numbers */
	shdescptr += 2;

	/* Skip IV */
#ifdef DEBUG
	memcpy(shdesc + 4, "myivmyivmyivmyiv", 16);
#endif
	shdescptr += 4;

	/* Skip SPI */
	shdescptr++;

	/* fixed IP header length */
	*shdescptr++ = ALGAPI_IP_HDR_LEN;

	shdescptr += ALGAPI_IP_HDR_LEN / sizeof(u32);

	/* </pretention> */

	/* Save current location for computing start index */
	startidx = shdescptr - shdesc;

	/* process keys, starting with class 2/authentication */
	/* This is assuming keys are immediate for sharedesc */
	shdescptr = cmd_insert_key(shdescptr, ctx->key, ctx->authkeylen * 8,
				   PTR_DIRECT, KEYDST_KEYREG, KEY_CLEAR,
				   ITEM_INLINE, ITEM_CLASS2);

	/* class 1/cipher key */
	shdescptr = cmd_insert_key(shdescptr, ctx->key + ctx->authkeylen,
				   ctx->enckeylen * 8, PTR_DIRECT,
				   KEYDST_KEYREG, KEY_CLEAR, ITEM_INLINE,
				   ITEM_CLASS1);

	/* insert the operation command */
	shdescptr = cmd_insert_proto_op_ipsec(shdescptr, ctx->class1_alg_type,
					      ctx->class2_alg_type, DIR_ENCAP);

	/* update the header with size/offsets */
	endidx = shdescptr - shdesc + 1; /* add 1 to include header */

	/* get spec-viol with CTX_SAVE here:
	 * [caam spec-viol] shared descriptor with INIT=1
	 */
	cmd_insert_shared_hdr(shdesc, startidx, endidx, CTX_ERASE,
			      SHR_SERIAL);

#ifdef DEBUG
	print_hex_dump(KERN_ERR, "shrdesc@"xstr(__LINE__)": ",
		       DUMP_PREFIX_ADDRESS, 16, 4, shdesc,
		       (shdescptr - shdesc + 1) * 4, 1);
	caam_desc_disasm(shdesc);
#endif

	ctx->shared_desc_encap_len = endidx * sizeof(u32);

	/* now we know the length, stop wasting preallocated shdesc space */
	ctx->shared_desc_encap = krealloc(shdesc, ctx->shared_desc_encap_len,
					  GFP_DMA | flags);

	ctx->shared_desc_encap_phys = dma_map_single(ctx->dev, shdesc,
						     endidx * sizeof(u32),
						     DMA_BIDIRECTIONAL);
	if (dma_mapping_error(ctx->dev, ctx->shared_desc_encap_phys)) {
		dev_err(ctx->dev, "unable to map shared descriptor\n");
		kfree(ctx->shared_desc_encap);
		return -ENOMEM;
	}

	return 0;
}

static int aead_authenc_setkey(struct crypto_aead *aead,
			       const u8 *key, unsigned int keylen)
{
	struct caam_ctx *ctx = crypto_aead_ctx(aead);
	struct rtattr *rta = (void *)key;
	struct crypto_authenc_key_param *param;
	unsigned int authkeylen;
	unsigned int enckeylen;

	if (!RTA_OK(rta, keylen))
		goto badkey;

	if (rta->rta_type != CRYPTO_AUTHENC_KEYA_PARAM)
		goto badkey;

	if (RTA_PAYLOAD(rta) < sizeof(*param))
		goto badkey;

	param = RTA_DATA(rta);
	enckeylen = be32_to_cpu(param->enckeylen);

	key += RTA_ALIGN(rta->rta_len);
	keylen -= RTA_ALIGN(rta->rta_len);

	if (keylen < enckeylen)
		goto badkey;

	authkeylen = keylen - enckeylen;

	if (keylen > CAAM_MAX_KEY_SIZE)
		goto badkey;

	memcpy(&ctx->key, key, keylen);

	ctx->keylen = keylen;
	ctx->enckeylen = enckeylen;
	ctx->authkeylen = authkeylen;

	return 0;
badkey:
	crypto_aead_set_flags(aead, CRYPTO_TFM_RES_BAD_KEY_LEN);
	return -EINVAL;
}

/*
 * ipsec_esp_edesc - s/w-extended ipsec_esp descriptor
 * @src_nents: number of segments in input scatterlist
 * @dst_nents: number of segments in output scatterlist
 * @assoc_nents: number of segments in associated data (SPI+Seq) scatterlist
 * @desc: h/w descriptor (variable length; must not exceed MAX_CAAM_DESCSIZE)
 * @flatbed_phys: bus physical mapped address of flatbed
 * @flatbed: space for flattened i/o data (if {src,dst}_nents > 1)
 *           (until s-g support added)
 * @dma_len: length of dma mapped flatbed space
 */
struct ipsec_esp_edesc {
	int src_nents;
	int dst_nents;
	int assoc_nents;
	u32 desc[MAX_CAAM_DESCSIZE];
	int dma_len;
	u32 *flatbed_phys;
	u8 flatbed[0];
};

static void ipsec_esp_unmap(struct device *dev,
			    struct ipsec_esp_edesc *edesc,
			    struct aead_request *areq)
{
	dma_unmap_sg(dev, areq->assoc, edesc->assoc_nents, DMA_TO_DEVICE);

	dma_unmap_sg(dev, areq->src, edesc->src_nents ? : 1,
		     DMA_BIDIRECTIONAL);

	if (edesc->dma_len)
		dma_unmap_single(dev, edesc->flatbed_phys, edesc->dma_len,
				 DMA_BIDIRECTIONAL);
}

/*
 * ipsec_esp descriptor callbacks
 */
static void ipsec_esp_encrypt_done(struct device *dev, void *desc,
				   int err, void *context)
{
	struct ipsec_esp_edesc *edesc =
		 container_of(desc, struct ipsec_esp_edesc, desc);
	struct aead_request *areq = context;
	struct crypto_aead *aead = crypto_aead_reqtfm(areq);
	struct caam_ctx *ctx = crypto_aead_ctx(aead);
	int ivsize = crypto_aead_ivsize(aead);

	ipsec_esp_unmap(dev, edesc, areq);

	if (!err && edesc->dma_len) {
#ifdef DEBUG
		print_hex_dump(KERN_ERR, "flatbed@"xstr(__LINE__)": ",
			       DUMP_PREFIX_ADDRESS, 16, 4, &edesc->flatbed[0],
			       areq->assoclen + 36 + areq->cryptlen +
			       ctx->authsize, 1);
#endif
		/* copy IV to giv */
		memcpy(sg_virt(areq->assoc) + areq->assoclen,
		       &edesc->flatbed[areq->assoclen], ivsize);

		/* copy ciphertext and generated ICV to dst */
		sg_copy_from_buffer(areq->dst, edesc->dst_nents ? : 1,
				    &edesc->flatbed[areq->assoclen + ivsize],
				    areq->cryptlen + ctx->authsize);
#ifdef DEBUG
		print_hex_dump(KERN_ERR, "assocout@"xstr(__LINE__)": ",
			       DUMP_PREFIX_ADDRESS, 16, 4, sg_virt(areq->assoc),
			       areq->assoclen + areq->cryptlen +
			       ctx->authsize + 36, 1);
#endif
	}

	kfree(edesc);

	aead_request_complete(areq, err);
}

static void ipsec_esp_decrypt_done(struct device *dev, void *desc, int err,
				   void *context)
{
	struct ipsec_esp_edesc *edesc =
		 container_of(desc, struct ipsec_esp_edesc, desc);
	struct aead_request *areq = context;
	struct crypto_aead *aead = crypto_aead_reqtfm(areq);
	struct caam_ctx *ctx = crypto_aead_ctx(aead);
	int ivsize = crypto_aead_ivsize(aead);

	ipsec_esp_unmap(dev, edesc, areq);

	if (!err && edesc->dma_len) {
#ifdef DEBUG
		print_hex_dump(KERN_ERR, "flatbed@"xstr(__LINE__)": ",
			       DUMP_PREFIX_ADDRESS, 16, 4, &edesc->flatbed[0],
			       areq->assoclen + 36 + areq->cryptlen +
			       ctx->authsize, 1);
#endif
		/* copy ciphertext and generated ICV to dst */
		sg_copy_from_buffer(areq->dst, edesc->dst_nents ? : 1,
				    &edesc->flatbed[ALGAPI_IP_HDR_LEN +
				                    areq->assoclen + ivsize],
				    areq->cryptlen + ctx->authsize);
#ifdef DEBUG
		print_hex_dump(KERN_ERR, "assocout@"xstr(__LINE__)": ",
			       DUMP_PREFIX_ADDRESS, 16, 4, sg_virt(areq->assoc),
			       areq->assoclen + areq->cryptlen +
			       ctx->authsize + 36, 1);
#endif
	}

	/*
	 * verify hw auth check passed else return -EBADMSG
	 */
	debug("err 0x%08x\n", err);
	if ((err & JQSTA_CCBERR_ERRID_MASK) == JQSTA_CCBERR_ERRID_ICVCHK)
		err = -EBADMSG;


	kfree(edesc);

	aead_request_complete(areq, err);
}

/*
 * fill in and submit ipsec_esp job descriptor
 */
static int ipsec_esp(struct ipsec_esp_edesc *edesc, struct aead_request *areq,
		     u8 *giv, enum protdir direction,
		     void (*callback) (struct device *dev, void *desc, int err,
				       void *context))
{
	struct crypto_aead *aead = crypto_aead_reqtfm(areq);
	struct caam_ctx *ctx = crypto_aead_ctx(aead);
	struct device *dev = ctx->dev;
	u32 *desc = &edesc->desc[0];
	u32 *descptr = desc;
	int startidx, endidx, ret, sg_count, assoc_sg_count, len, padlen;
	int nbytes, pos, ivsize = crypto_aead_ivsize(aead);
	u8 *ptr;

#ifdef DEBUG
	debug("assoclen %d cryptlen %d authsize %d\n",
	      areq->assoclen, areq->cryptlen,ctx->authsize);
	print_hex_dump(KERN_ERR, "ipv4hdr@"xstr(__LINE__)": ",
		       DUMP_PREFIX_ADDRESS, 16, 4,
		       ((char *)sg_virt(areq->assoc) - ALGAPI_IP_HDR_LEN),
		       ALGAPI_IP_HDR_LEN, 1);
	print_hex_dump(KERN_ERR, "inassoc@"xstr(__LINE__)": ",
		       DUMP_PREFIX_ADDRESS, 16, 4, sg_virt(areq->assoc),
		       areq->assoclen + ivsize + areq->cryptlen + ctx->authsize, 1);
#endif

	/* skip job header (filled in last) */
	descptr++;

	/* skip shared descriptor pointer (filled in later) */
	descptr++;

	/* Save current location for computing start index later */
	startidx = descptr - desc;

	/*
	 * insert the SEQ IN (data in) command
	 * assoc is bidirectional because we're using the protocol descriptor
	 * and encap takes SPI + seq from PDB.
	 */
	assoc_sg_count = dma_map_sg(dev, areq->assoc, edesc->assoc_nents ? : 1,
				    DMA_BIDIRECTIONAL);
	if (areq->src == areq->dst)
		sg_count = dma_map_sg(dev, areq->src, edesc->src_nents ? : 1,
				      DMA_BIDIRECTIONAL);
	else
		sg_count = dma_map_sg(dev, areq->src, edesc->src_nents ? : 1,
				      DMA_TO_DEVICE);
	if (direction == DIR_ENCAP) {
		if (!edesc->dma_len) {
			ptr = (u8 *)sg_dma_address(areq->src);
			padlen = *(u8 *)((u8 *)sg_virt(areq->src)
					 + areq->cryptlen - 2);
		} else {
			/* we might want to cp this to the PDB instead */
			nbytes = sg_copy_to_buffer(areq->assoc,
						   assoc_sg_count,
						   &edesc->flatbed[0 /*pos*/],
						   areq->assoclen);
			BUG_ON(nbytes != areq->assoclen);
			pos = nbytes;
			pos += ivsize; /* leave in-place space for output IV */
			nbytes = sg_copy_to_buffer(areq->src,
						   sg_count,
						   &edesc->flatbed[pos],
						   areq->cryptlen +
						   ctx->authsize);
			BUG_ON(nbytes != areq->cryptlen + ctx->authsize);
#ifdef DEBUG
			print_hex_dump(KERN_ERR, "flatbed@"xstr(__LINE__)": ",
				       DUMP_PREFIX_ADDRESS, 16, 4,
				       &edesc->flatbed[0], areq->assoclen +
				       ivsize + areq->cryptlen + ctx->authsize
				       + 16, 1);
#endif
			ptr = (u8 *)dma_map_single(dev,
						   &edesc->flatbed[areq->
							assoclen + ivsize],
						   edesc->dma_len,
						   DMA_BIDIRECTIONAL);
			edesc->flatbed_phys = ptr;
			padlen = edesc->flatbed[pos + areq->cryptlen - 2];
		}
		debug("padlen is %d\n",padlen);
		/* cryptlen includes padlen / is blocksize aligned */
		len = areq->cryptlen - padlen - 2;
	} else {
		debug("seq.num %d\n",
		      *(u32 *)((u32 *)sg_virt(areq->assoc) + 1));
#undef INJECT_ICV_CHECK_FAILURE
#ifdef INJECT_ICV_CHECK_FAILURE
		/*
		 * intentionally tamper with every 13th packet's data
		 * to verify proper ICV check result propagation
		 */
		if((*(u32 *)((u32 *)sg_virt(areq->assoc) + 1)) &&
		   (((*(u32 *)((u32 *)sg_virt(areq->assoc) + 1) % 13) == 0))) {
			dev_warn(dev, "foiling packet data\n");
			(*(u32 *)((u32 *)sg_virt(areq->assoc) + 13))++;
		}
#endif
		if (!edesc->dma_len) {
			ptr = (u8 *)sg_dma_address(areq->src) - ivsize -
			      areq->assoclen - ALGAPI_IP_HDR_LEN;
		} else {
			/* manually copy input skb to flatbed */
			memcpy(&edesc->flatbed[0], sg_virt(areq->assoc)
			       - ALGAPI_IP_HDR_LEN, ALGAPI_IP_HDR_LEN);
			pos = ALGAPI_IP_HDR_LEN;
			/* SPI and sequence number */
			nbytes = sg_copy_to_buffer(areq->assoc,
						   assoc_sg_count,
						   &edesc->flatbed[pos],
						   areq->assoclen);
			BUG_ON(nbytes != areq->assoclen);
			pos += nbytes;
			/* the IV */
			memcpy(&edesc->flatbed[pos], (u8 *)sg_virt(areq->assoc)
			       + areq->assoclen, ivsize);
			pos += ivsize;
			/* and the payload */
			nbytes = sg_copy_to_buffer(areq->src,
						   sg_count,
						   &edesc->flatbed[pos],
						   areq->cryptlen +
						   ctx->authsize);
			BUG_ON(nbytes != areq->cryptlen + ctx->authsize);
#ifdef DEBUG
			print_hex_dump(KERN_ERR, "flatbed@"xstr(__LINE__)": ",
				       DUMP_PREFIX_ADDRESS, 16, 4,
				       &edesc->flatbed[0], areq->assoclen + ivsize +
				       areq->cryptlen + ctx->authsize + 16, 1);
#endif
			ptr = (u8 *)dma_map_single(dev, &edesc->flatbed[0],
						   edesc->dma_len,
						   DMA_BIDIRECTIONAL);
			edesc->flatbed_phys = ptr;
		}
		len =  ALGAPI_IP_HDR_LEN + areq->assoclen + ivsize +
		       areq->cryptlen + ctx->authsize;
	}
	descptr = cmd_insert_seq_in_ptr(descptr, ptr, len, PTR_DIRECT);

	/* insert the SEQ OUT (data out) command */
	sg_count = dma_map_sg(dev, areq->dst, edesc->dst_nents ? : 1,
			      DMA_BIDIRECTIONAL);

	if (direction == DIR_ENCAP) {
		len = areq->assoclen + ivsize + areq->cryptlen + ctx->authsize;
		/*
		 * if flattening, adjust phys addr to offset of iv data
		 * within same flatbed
		 */
		if (!edesc->dma_len)
			ptr = (u8 *) sg_dma_address(areq->assoc);
		else
			ptr -= areq->assoclen + ivsize;
	} else {
		len = areq->cryptlen + ctx->authsize;
		if (!edesc->dma_len)
			ptr = (u8 *) sg_dma_address(areq->dst);
		else
			ptr += ALGAPI_IP_HDR_LEN + areq->assoclen + ivsize;
	}

	descptr = cmd_insert_seq_out_ptr(descptr, ptr, len ,PTR_DIRECT);

	/*
	 * write the job descriptor header with shared descriptor length,
	 * reverse order execution, and size/offsets.
	 */
	endidx = descptr - desc;

	len = (direction == DIR_ENCAP) ? ctx->shared_desc_encap_len
				       : ctx->shared_desc_decap_len;
	cmd_insert_hdr(desc, len / sizeof(u32),
		       endidx, SHR_SERIAL, SHRNXT_SHARED /* has_shared */,
		       ORDER_REVERSE, DESC_STD /*don't make trusted*/);
#ifdef DEBUG
	print_hex_dump(KERN_ERR, "jobdesc@"xstr(__LINE__)": ",
		       DUMP_PREFIX_ADDRESS, 16, 4, desc,
		       (descptr - desc + 1) * 4, 1);
	caam_desc_disasm(desc);
#endif

	ret = caam_jq_enqueue(ctx->dev, desc, callback, areq);
	if (!ret)
		ret = -EINPROGRESS;
	else {
		ipsec_esp_unmap(dev, edesc, areq);
		kfree(edesc);
	}

	return ret;
}

/*
 * derive number of elements in scatterlist
 */
static int sg_count(struct scatterlist *sg_list, int nbytes)
{
	struct scatterlist *sg = sg_list;
	int sg_nents = 0;

	while (nbytes) {
		sg_nents++;
		nbytes -= sg->length;
		sg = sg_next(sg);
	}

	return sg_nents;
}

/*
 * allocate and map the ipsec_esp extended descriptor
 */
static struct ipsec_esp_edesc *ipsec_esp_edesc_alloc(struct aead_request *areq,
						     enum protdir direction)
{
	struct crypto_aead *aead = crypto_aead_reqtfm(areq);
	struct caam_ctx *ctx = crypto_aead_ctx(aead);
	int ivsize = crypto_aead_ivsize(aead);
	gfp_t flags = areq->base.flags & CRYPTO_TFM_REQ_MAY_SLEEP ? GFP_KERNEL :
		      GFP_ATOMIC;
	int assoc_nents, src_nents, dst_nents, alloc_len, dma_len = 0;
	struct ipsec_esp_edesc *edesc;

	assoc_nents = sg_count(areq->assoc, areq->assoclen);
	assoc_nents = (assoc_nents == 1) ? 0 : assoc_nents;

	src_nents = sg_count(areq->src, areq->cryptlen + ctx->authsize);
	src_nents = (src_nents == 1) ? 0 : src_nents;

	if (areq->dst == areq->src) {
		dst_nents = src_nents;
	} else {
		dev_err(ctx->dev, "src!=dst case not handled\n");
		BUG();
		dst_nents = sg_count(areq->dst, areq->cryptlen + ctx->authsize);
		dst_nents = (dst_nents == 1) ? 0 : dst_nents;
	}

	/*
	 * allocate space for base edesc plus the link tables,
	 * allowing for two separate entries for ICV and generated ICV (+ 2),
	 * and the ICV data itself
	 */
	alloc_len = sizeof(struct ipsec_esp_edesc);
	if (assoc_nents || src_nents || dst_nents) {
		/* size for worst case (encap/decap) */
		dma_len = ALGAPI_IP_HDR_LEN + areq->assoclen + ivsize +
			  areq->cryptlen + ivsize /* no max - we know the pad */
			  + ctx->authsize;
	}

	edesc = kmalloc(sizeof(struct ipsec_esp_edesc) + dma_len,
		        GFP_DMA | flags);
	if (!edesc) {
		dev_err(ctx->dev, "could not allocate extended descriptor\n");
		return ERR_PTR(-ENOMEM);
	}

	edesc->assoc_nents = assoc_nents;
	edesc->src_nents = src_nents;
	edesc->dst_nents = dst_nents;
	edesc->dma_len = dma_len;

	return edesc;
}

static int aead_authenc_encrypt(struct aead_request *req)
{
	printk("%s unimplemented\n", __FUNCTION__);

	return -EINVAL;
}

static int aead_authenc_encrypt_first(struct aead_request *req)
{
	printk("%s unimplemented\n", __FUNCTION__);

	return -EINVAL;
}

static int aead_authenc_decrypt(struct aead_request *req)
{
	struct crypto_aead *aead = crypto_aead_reqtfm(req);
	struct caam_ctx *ctx = crypto_aead_ctx(aead);
	struct ipsec_esp_edesc *edesc;

	req->cryptlen -= ctx->authsize;

	/* allocate extended descriptor */
	edesc = ipsec_esp_edesc_alloc(req, DIR_DECAP);
	if (IS_ERR(edesc))
		return PTR_ERR(edesc);

	/* insert shared descriptor pointer */
	edesc->desc[1] = ctx->shared_desc_decap_phys;

	return ipsec_esp(edesc, req, NULL, DIR_DECAP, ipsec_esp_decrypt_done);
}

static int aead_authenc_decrypt_first(struct aead_request *req)
{
	struct crypto_aead *aead = crypto_aead_reqtfm(req);
	struct caam_ctx *ctx = crypto_aead_ctx(aead);
	int err;

        spin_lock_bh(&ctx->first_lock);
        if (crypto_aead_crt(aead)->decrypt != aead_authenc_decrypt_first)
                goto unlock;

	err = build_protocol_desc_ipsec_decap(ctx, req);
	if (err) {
		spin_unlock_bh(&ctx->first_lock);
		return err;
	}

	/* copy sequence number to PDB */
	*(u32 *)(ctx->shared_desc_decap + 5) =
		*(u32 *)((u32 *)sg_virt(req->assoc) + 1);

        crypto_aead_crt(aead)->decrypt = aead_authenc_decrypt;
unlock:
        spin_unlock_bh(&ctx->first_lock);

	return aead_authenc_decrypt(req);
}

static int aead_authenc_givencrypt(struct aead_givcrypt_request *req)
{
	struct aead_request *areq = &req->areq;
	struct crypto_aead *aead = crypto_aead_reqtfm(areq);
	struct caam_ctx *ctx = crypto_aead_ctx(aead);
	struct ipsec_esp_edesc *edesc;

	/* allocate extended descriptor */
	edesc = ipsec_esp_edesc_alloc(areq, DIR_ENCAP);
	if (IS_ERR(edesc))
		return PTR_ERR(edesc);

	/* insert shared descriptor pointer */
	edesc->desc[1] = ctx->shared_desc_encap_phys;

	return ipsec_esp(edesc, areq, req->giv, DIR_ENCAP,
			 ipsec_esp_encrypt_done);
}

static int aead_authenc_givencrypt_first(struct aead_givcrypt_request *req)
{
	struct aead_request *areq = &req->areq;
	struct crypto_aead *aead = crypto_aead_reqtfm(areq);
	struct caam_ctx *ctx = crypto_aead_ctx(aead);
	int err;

        spin_lock_bh(&ctx->first_lock);
        if (crypto_aead_crt(aead)->givencrypt != aead_authenc_givencrypt_first)
                goto unlock;

	err = build_protocol_desc_ipsec_encap(ctx, areq);
	if (err) {
		spin_unlock_bh(&ctx->first_lock);
		return err;
	}

	/* copy sequence number to PDB */
	*(u64 *)(ctx->shared_desc_encap + 2) = req->seq;

	/* and the SPI */
	*(ctx->shared_desc_encap + 8) = *((u32 *)sg_virt(areq->assoc));

        crypto_aead_crt(aead)->givencrypt = aead_authenc_givencrypt;
unlock:
        spin_unlock_bh(&ctx->first_lock);

	return aead_authenc_givencrypt(req);
}

struct caam_alg_template {
	char name[CRYPTO_MAX_ALG_NAME];
	char driver_name[CRYPTO_MAX_ALG_NAME];
	unsigned int blocksize;
	struct aead_alg aead;
	struct device *dev;
	int class1_alg_type;
	int class2_alg_type;
};

static struct caam_alg_template driver_algs[] = {
	/* single-pass ipsec_esp descriptor */
	{
		.name = "authenc(hmac(sha1),cbc(aes))",
		.driver_name = "authenc-hmac-sha1-cbc-aes-caam",
		.blocksize = AES_BLOCK_SIZE,
		.aead = {
			.setkey = aead_authenc_setkey,
			.setauthsize = aead_authenc_setauthsize,
			.encrypt = aead_authenc_encrypt_first,
			.decrypt = aead_authenc_decrypt_first,
			.givencrypt = aead_authenc_givencrypt_first,
			.geniv = "<built-in>",
			.ivsize = AES_BLOCK_SIZE,
			.maxauthsize = SHA1_DIGEST_SIZE,
			},
		.class1_alg_type = CIPHER_TYPE_IPSEC_AESCBC,
		.class2_alg_type = AUTH_TYPE_IPSEC_SHA1HMAC_96,
	},
	{
		.name = "authenc(hmac(sha1),cbc(des))",
		.driver_name = "authenc-hmac-sha1-cbc-des-caam",
		.blocksize = DES_BLOCK_SIZE,
		.aead = {
			.setkey = aead_authenc_setkey,
			.setauthsize = aead_authenc_setauthsize,
			.encrypt = aead_authenc_encrypt_first,
			.decrypt = aead_authenc_decrypt_first,
			.givencrypt = aead_authenc_givencrypt_first,
			.geniv = "<built-in>",
			.ivsize = DES_BLOCK_SIZE,
			.maxauthsize = SHA1_DIGEST_SIZE,
			},
		.class1_alg_type = CIPHER_TYPE_IPSEC_DESCBC,
		.class2_alg_type = AUTH_TYPE_IPSEC_SHA1HMAC_96,
	},
};

struct caam_crypto_alg {
	struct list_head entry;
	struct device *dev;
	int class1_alg_type;
	int class2_alg_type;
	struct crypto_alg crypto_alg;
};

static int caam_cra_init(struct crypto_tfm *tfm)
{
	struct crypto_alg *alg = tfm->__crt_alg;
	struct caam_crypto_alg *caam_alg =
		 container_of(alg, struct caam_crypto_alg, crypto_alg);
	struct caam_ctx *ctx = crypto_tfm_ctx(tfm);

	/* update context with ptr to dev */
	ctx->dev = caam_alg->dev;

	/* copy descriptor header template value */
	ctx->class1_alg_type = caam_alg->class1_alg_type;
	ctx->class2_alg_type = caam_alg->class2_alg_type;

	spin_lock_init(&ctx->first_lock);

	return 0;
}

static void caam_cra_exit(struct crypto_tfm *tfm)
{
	struct caam_ctx *ctx = crypto_tfm_ctx(tfm);

	if (!dma_mapping_error(ctx->dev, ctx->shared_desc_encap_phys))
		dma_unmap_single(ctx->dev, ctx->shared_desc_encap_phys,
				 ctx->shared_desc_encap_len, DMA_BIDIRECTIONAL);
	kfree(ctx->shared_desc_encap);

	if (!dma_mapping_error(ctx->dev, ctx->shared_desc_decap_phys))
		dma_unmap_single(ctx->dev, ctx->shared_desc_decap_phys,
				 ctx->shared_desc_decap_len, DMA_BIDIRECTIONAL);
	kfree(ctx->shared_desc_decap);
}

void caam_algapi_remove(struct device *dev)
{
	struct caam_drv_private *priv = dev_get_drvdata(dev);
	struct caam_crypto_alg *t_alg, *n;

	if (!priv->alg_list.next)
		return;

	list_for_each_entry_safe(t_alg, n, &priv->alg_list, entry) {
		crypto_unregister_alg(&t_alg->crypto_alg);
		list_del(&t_alg->entry);
		kfree(t_alg);
	}
}

static struct caam_crypto_alg *caam_alg_alloc(struct device *dev,
					      struct caam_alg_template
					      *template)
{
	struct caam_crypto_alg *t_alg;
	struct crypto_alg *alg;

	t_alg = kzalloc(sizeof(struct caam_crypto_alg), GFP_KERNEL);
	if (!t_alg) {
		dev_err(dev, "failed to allocate t_alg\n");
		return ERR_PTR(-ENOMEM);
	}

	alg = &t_alg->crypto_alg;

	snprintf(alg->cra_name, CRYPTO_MAX_ALG_NAME, "%s", template->name);
	snprintf(alg->cra_driver_name, CRYPTO_MAX_ALG_NAME, "%s",
		 template->driver_name);
	alg->cra_module = THIS_MODULE;
	alg->cra_init = caam_cra_init;
	alg->cra_exit = caam_cra_exit;
	alg->cra_priority = CAAM_CRA_PRIORITY;
	alg->cra_flags = CRYPTO_ALG_TYPE_AEAD | CRYPTO_ALG_ASYNC;
	alg->cra_blocksize = template->blocksize;
	alg->cra_alignmask = 0;
	alg->cra_type = &crypto_aead_type;
	alg->cra_ctxsize = sizeof(struct caam_ctx);
	alg->cra_u.aead = template->aead;

	t_alg->class1_alg_type = template->class1_alg_type;
	t_alg->class2_alg_type = template->class2_alg_type;
	t_alg->dev = dev;

	return t_alg;
}

void caam_jq_algapi_init(struct device *ctrldev)
{
	struct caam_drv_private *priv = dev_get_drvdata(ctrldev);
	struct device *dev;
	int i, err;

	INIT_LIST_HEAD(&priv->alg_list);

	err = caam_jq_register(ctrldev, &dev);
	if (err) {
		dev_err(ctrldev, "algapi error in job queue registration: %d\n",
			err);
		return;
	}

	/* register crypto algorithms the device supports */
	for (i = 0; i < ARRAY_SIZE(driver_algs); i++) {
		/* TODO: check if h/w supports alg */
		struct caam_crypto_alg *t_alg;

		t_alg = caam_alg_alloc(dev, &driver_algs[i]);
		if (IS_ERR(t_alg)) {
			err = PTR_ERR(t_alg);
			dev_warn(dev, "%s alg registration failed\n",
				t_alg->crypto_alg.cra_driver_name);
			continue;
		}

		err = crypto_register_alg(&t_alg->crypto_alg);
		if (err) {
			dev_warn(dev, "%s alg registration failed\n",
				t_alg->crypto_alg.cra_driver_name);
			kfree(t_alg);
		} else {
			list_add_tail(&t_alg->entry, &priv->alg_list);
			dev_info(dev, "%s\n",
				 t_alg->crypto_alg.cra_driver_name);
		}
	}

	priv->algapi_jq = dev;
}
