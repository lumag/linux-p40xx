/*
 * jq_ipsec.c - JobQ unit test for termination-less IPSec examples
 *
 * Copyright (c) 2008, 2009, Freescale Semiconductor, Inc.
 * All rights reserved.
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

#include "caam_jqtest.h"


/* Class 2 padded all the way out to 64 bytes */
static u8 class_2_key[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f };

static u8 class_1_key[] = {
	0x00, 0x0e, 0x0f, 0x00, 0x0c, 0x0f, 0x0a, 0x00,
	0x0a, 0x0f, 0x0a, 0x00, 0x0b, 0x0f, 0x0a, 0x00 };

#define OPTHDRSZ 52
static const u_int8_t opthdr[] = {
	0x34, 0x00, 0x10, 0x45, 0x00, 0x40, 0x25, 0x12,
	0xd2, 0x86, 0x06, 0x40, 0x77, 0x46, 0x43, 0x0a,
	0xc0, 0x46, 0x43, 0x0a, 0x16, 0x00, 0x22, 0xd0,
	0x5d, 0x89, 0x18, 0x88, 0x9c, 0xee, 0x19, 0x12,
	0xbc, 0x21, 0x10, 0x80, 0x00, 0x00, 0x08, 0x98,
	0x0a, 0x08, 0x01, 0x01, 0x22, 0x75, 0x9a, 0xa6,
	0xdb, 0x14, 0x3f, 0x08
};

static const u_int8_t incmp[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x02, 0x03, 0x14, 0x15, 0x06, 0x07,
	0x18, 0x19, 0x0a, 0x0b, 0x1c, 0x1d, 0x0e, 0x0f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
};

static struct completion completion;

void jq_ipsec_done(struct device *dev, u32 *head, u32 status, void *auxarg)
{
	/* Write back returned status, and continue */
	*(u32 *)auxarg = status;
	complete(&completion);
}

#define SHA1_HMAC_KEYSZ 20
#define SHA1_HMAC_PADSZ 20
#define SHA224_HMAC_KEYSZ 28
#define SHA224_HMAC_PADSZ 32
#define SHA256_HMAC_KEYSZ 32
#define SHA256_HMAC_PADSZ 32
#define SHA384_HMAC_KEYSZ 48
#define SHA384_HMAC_PADSZ 64
#define SHA512_HMAC_KEYSZ 64
#define SHA512_HMAC_PADSZ 64

#define AES_KEYSZ 16

int jq_ipsec_esp_split(struct device *dev, int show)
{
	int stat, exit, rtnval = 0;
	u32 *jdesc, *jdmap, *encapdesc, *encapdmap, *decapdesc, *decapdmap;
	u32 rqstatus;
	u8 *hmackeybuf, *spkeybuf, *hmackeymap, *spkeymap;
	u8 *inbuf, *encapbuf, *decapbuf, *inmap, *encapmap, *decapmap;
	u8 *cipherkeybuf, *cipherkeymap;
	u16 jdsz, encapdsz, decapdsz, cipherkeysz, hmackeysz;
	u16 padsz, inbufsz, encapbufsz, decapbufsz;
	struct ipsec_encap_pdb encappdb;
	struct ipsec_decap_pdb decappdb;
	struct authparams auth;
	struct cipherparams cipher;
	u8 err[256];
	u8 testname[] = "jq_ipsec_esp_split";

	init_completion(&completion);

	/*
	 * Compute buffer/descriptor sizes
	 */
	cipherkeysz = AES_KEYSZ;
	hmackeysz = SHA1_HMAC_KEYSZ;
	padsz = SHA1_HMAC_PADSZ;
	inbufsz = 64;
	encapbufsz = 8+16+inbufsz+28; /* payload+SPI+IV+pad+padlen+ICV */
	decapbufsz = inbufsz;

	jdsz = MAX_CAAM_DESCSIZE * sizeof(u32);
	encapdsz = MAX_CAAM_DESCSIZE * sizeof(u32);
	decapdsz = MAX_CAAM_DESCSIZE * sizeof(u32);

	/*
	 * Alloc buffer/descriptor space
	 */
	hmackeybuf = kmalloc(hmackeysz, GFP_KERNEL | GFP_DMA);
	cipherkeybuf = kmalloc(cipherkeysz, GFP_KERNEL | GFP_DMA);
	spkeybuf = kmalloc(padsz, GFP_KERNEL | GFP_DMA);
	inbuf = kmalloc(inbufsz, GFP_KERNEL | GFP_DMA);
	encapbuf = kmalloc(encapbufsz, GFP_KERNEL | GFP_DMA);
	decapbuf = kmalloc(decapbufsz, GFP_KERNEL | GFP_DMA);

	jdesc = kzalloc(jdsz, GFP_KERNEL | GFP_DMA);
	encapdesc = kzalloc(encapdsz, GFP_KERNEL | GFP_DMA);
	decapdesc = kzalloc(decapdsz, GFP_KERNEL | GFP_DMA);

	if ((hmackeybuf == NULL) || (cipherkeybuf == NULL) ||
	    (spkeybuf == NULL) || (jdesc == NULL) || (encapdesc == NULL) ||
	    (decapdesc == NULL) || (inbuf == NULL) || (encapbuf == NULL) ||
	    (decapbuf == NULL)) {
		printk(KERN_INFO "%s: can't get buffers\n", testname);
		rtnval = -1;
		goto freeup;
	};

	/*
	 * First step, generate MD split-key for shared descriptors to use
	 * Precoat key buffer
	 */
	memcpy(hmackeybuf, class_2_key, hmackeysz);

	/* Map job, input key, output splitkey */
	jdmap = (u32 *)dma_map_single(dev, jdesc, jdsz, DMA_TO_DEVICE);
	hmackeymap = (u8 *)dma_map_single(dev, hmackeybuf, hmackeysz,
					  DMA_TO_DEVICE);
	spkeymap = (u8 *)dma_map_single(dev, spkeybuf, padsz,
					DMA_FROM_DEVICE);

	/*
	 * Construct keysplitter using mapped HMAC input buffer
	 * and mapped split key output buffer
	 */
	stat = cnstr_jobdesc_mdsplitkey(jdesc, &jdsz, hmackeymap,
					OP_ALG_ALGSEL_SHA1, spkeymap);

	if (show == SHOW_DESC)
		caam_desc_disasm(jdesc, DISASM_SHOW_RAW | DISASM_SHOW_OFFSETS);

	/* Execute keysplitter */
	stat = caam_jq_enqueue(dev, jdesc, jq_ipsec_done, (void *)&rqstatus);
	if (stat) {
		printk(KERN_INFO "%s can't enqueue\n", testname);
		rtnval = -1;
	}
	exit = wait_for_completion_interruptible(&completion);
	if (exit) {
		printk(KERN_INFO "%s interrupted\n", testname);
		rtnval = -1;
	}

	/* Unmap splitter desc, input key, output splitkey */
	dma_unmap_single(dev, (u32)jdmap, jdsz, DMA_TO_DEVICE);
	dma_unmap_single(dev, (u32)hmackeymap, hmackeysz, DMA_TO_DEVICE);
	dma_unmap_single(dev, (u32)spkeymap, padsz, DMA_FROM_DEVICE);

	/* If problem, report, free jobdesc and splitkey, return */
	if ((rtnval) || (rqstatus)) {
		printk(KERN_INFO "%s: request status = 0x%08x\n", testname,
		       rqstatus);
		printk(KERN_INFO "%s\n", caam_jq_strstatus(err, rqstatus));
		goto freeup;
	}

	/*
	 * Now we have a covered HMAC split key. Build our encapsulation
	 * and decapsulation descriptors
	 */

	/* Precoat blockcipher key */
	memcpy(cipherkeybuf, class_1_key, cipherkeysz);

	/*
	 * Set up a PDB for the encapsulation side. In this case, am not
	 * doing header prepend
	 */
	memset(&encappdb, 0, sizeof(encappdb));
	/* encappdb.options = PDBOPTS_ESPCBC_TUNNEL; */
	/* encappdb.opt_hdr_len = OPTHDRSZ; */
	encappdb.cbc.iv[0] = 0x01234569;
	encappdb.cbc.iv[1] = 0x89abcdef;
	encappdb.cbc.iv[2] = 0xfedcba98;
	encappdb.cbc.iv[3] = 0x76543210;

	/* Set up a PDB for the decapsulation side */
	/* decappdb.ip_hdr_len = OPTHDRSZ; */
	decappdb.options = PDBOPTS_ESPCBC_OUTFMT;

	/* Map key buffers before the descbuild */
	spkeymap = (u8 *)dma_map_single(dev, spkeybuf, padsz, DMA_TO_DEVICE);
	cipherkeymap = (u8 *)dma_map_single(dev, cipherkeybuf, 16,
					    DMA_TO_DEVICE);

	/* Now set up transforms with mapped cipher and auth keys */
	cipher.algtype = CIPHER_TYPE_IPSEC_AESCBC;
	cipher.key = cipherkeymap;
	cipher.keylen = cipherkeysz * 8; /* AES keysize in bits */

	auth.algtype = AUTH_TYPE_IPSEC_SHA1HMAC_96;
	auth.key = spkeymap;
	auth.keylen = SHA1_HMAC_KEYSZ * 2 * 8;

	/*
	 * Have our keys, cipher selections, and PDB set up for the
	 * sharedesc constructor. Now construct it
	 */
	stat = cnstr_shdsc_ipsec_encap(encapdesc, &encapdsz, &encappdb,
				       (u8 *)opthdr, &cipher, &auth);
	if (stat) {
		printk(KERN_INFO
		       "%s: encap sharedesc construct failed\n", testname);
		goto freeup;
		return -1;
	};

	stat = cnstr_shdsc_ipsec_decap(decapdesc, &decapdsz, &decappdb,
				       &cipher, &auth);
	if (stat) {
		printk(KERN_INFO
		       "%s: decap sharedesc construct failed\n", testname);
		goto freeup;
		return -1;
	};

	/*
	 * Starting encapsulation phase. Precoat the input frame
	 * with a known pattern
	 */
	memcpy(inbuf, incmp, inbufsz);

	/*
	 * Encap shared descriptor is ready to go. Now, map it.
	 * Also, map the input frame and output frame before building
	 * a sequence job descriptor to run the frame
	 */
	encapdmap = (u32 *)dma_map_single(dev, encapdesc, encapdsz,
					  DMA_BIDIRECTIONAL);
	inmap = (u8 *)dma_map_single(dev, inbuf, inbufsz, DMA_TO_DEVICE);
	encapmap = (u8 *)dma_map_single(dev, encapbuf, encapbufsz,
					DMA_FROM_DEVICE);

	/* With mapped buffers, build/map jobdesc for this frame */
	stat = cnstr_seq_jobdesc(jdesc, &jdsz, encapdmap, encapdsz,
				 inmap, inbufsz, encapmap, encapbufsz);

	jdmap = (u32 *)dma_map_single(dev, jdesc, jdsz, DMA_TO_DEVICE);
	if (stat) {
		printk(KERN_INFO
		       "%s: encap jobdesc construct failed\n", testname);
		goto freeup;
		return -1;
	};

	/* Show it before we run it */
	if (show == SHOW_DESC) {
		caam_desc_disasm(jdesc, DISASM_SHOW_OFFSETS);
		caam_desc_disasm(encapdesc, DISASM_SHOW_OFFSETS);
	}

	/* Enqueue the encapsulation job and block */
	stat = caam_jq_enqueue(dev, jdesc, jq_ipsec_done, (void *)&rqstatus);
	if (stat) {
		printk(KERN_INFO "%s: can't enqueue\n", testname);
		rtnval = -1;
	}
	exit = wait_for_completion_interruptible(&completion);
	if (exit) {
		printk(KERN_INFO "%s interrupted\n", testname);
		rtnval = -1;
	}

	/* Upmap shared desc, job desc, and inputs */
	dma_unmap_single(dev, (u32)encapdmap, encapdsz, DMA_BIDIRECTIONAL);
	dma_unmap_single(dev, (u32)jdmap, jdsz, DMA_TO_DEVICE);
	dma_unmap_single(dev, (u32)inmap, inbufsz, DMA_TO_DEVICE);
	dma_unmap_single(dev, (u32)encapmap, encapbufsz, DMA_FROM_DEVICE);

	if ((rtnval) || (rqstatus)) {
		printk(KERN_INFO "%s: request status = 0x%08x\n",
		       testname, rqstatus);
		printk(KERN_INFO "%s\n", caam_jq_strstatus(err, rqstatus));
		goto freeup;
	}

	/*
	 * Now do decapsulation phase. Map the decapsulation descriptor
	 * and encapsulation-input/decapsulation-output frames in their
	 * correct directions
	 */
	decapdmap = (u32 *)dma_map_single(dev, decapdesc, decapdsz,
					  DMA_BIDIRECTIONAL);
	encapmap = (u8 *)dma_map_single(dev, encapbuf, encapbufsz,
					DMA_TO_DEVICE);
	decapmap = (u8 *)dma_map_single(dev, decapbuf, decapbufsz,
					DMA_FROM_DEVICE);

	/* With mapped buffers, build/map jobdesc for this frame */
	stat = cnstr_seq_jobdesc(jdesc, &jdsz, decapdmap, decapdsz,
				 encapmap, encapbufsz, decapmap, decapbufsz);

	jdmap = (u32 *)dma_map_single(dev, jdesc, jdsz, DMA_TO_DEVICE);
	if (stat) {
		printk(KERN_INFO
		       "%s: encap jobdesc construct failed\n", testname);
		goto freeup;
		return -1;
	};

	if (show == SHOW_DESC) {
		caam_desc_disasm(jdesc, DISASM_SHOW_OFFSETS);
		caam_desc_disasm(decapdesc, DISASM_SHOW_OFFSETS);
	}

	/* Enqueue the decapsulation job and block */
	stat = caam_jq_enqueue(dev, jdesc, jq_ipsec_done, (void *)&rqstatus);
	if (stat) {
		printk(KERN_INFO "%s: can't enqueue\n", testname);
		rtnval = -1;
	}
	exit = wait_for_completion_interruptible(&completion);
	if (exit) {
		printk(KERN_INFO "%s interrupted\n", testname);
		rtnval = -1;
	}

	/* Upmap shared desc, job desc, inputs and outputs */
	dma_unmap_single(dev, (u32)decapdmap, decapdsz, DMA_BIDIRECTIONAL);
	dma_unmap_single(dev, (u32)jdmap, jdsz, DMA_TO_DEVICE);
	dma_unmap_single(dev, (u32)encapmap, encapbufsz, DMA_FROM_DEVICE);
	dma_unmap_single(dev, (u32)decapmap, decapbufsz, DMA_FROM_DEVICE);


freeup:
	kfree(hmackeybuf);
	kfree(cipherkeybuf);
	kfree(spkeybuf);
	kfree(jdesc);
	kfree(encapdesc);
	kfree(decapdesc);
	kfree(inbuf);
	kfree(encapbuf);
	kfree(decapbuf);
	return rtnval;
}

