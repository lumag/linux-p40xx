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



static u8 class_2_key[] = {
	0x00, 0xe0, 0xf0, 0xa0,
	0x00, 0xd0, 0xf0, 0xa0,
	0x0a, 0xd0, 0xf0, 0xa0,
	0x0b, 0xd0, 0xf0, 0xa0,
	0x0f, 0xd0, 0xf0, 0xa0 };

static u8 class_1_key[] = {
	0x00, 0x0e, 0x0f, 0x00,
	0x0c, 0x0f, 0x0a, 0x00,
	0x0a, 0x0f, 0x0a, 0x00,
	0x0b, 0x0f, 0x0a, 0x00 };


extern wait_queue_head_t jqtest_wq;

void jq_ipsec_done(struct device *dev, u32 *head, u32 status, void *auxarg)
{
	/* Bump volatile completion test value and wake calling thread */
	(*(int *)auxarg)++;
	wake_up_interruptible(&jqtest_wq);
}


int jq_ipsec_esp_no_term(struct device *dev, int show)
{
	int stat, exit, rtnval = 0;
	u32 *sdesc, *jdesc, *sdmap, *jdmap;
	u8 *inbuf, *outbuf, *inmap, *outmap;
	u16 sdsz, jdsz, inbufsz, outbufsz;
	int jqarg;
	struct pdbcont pdb;
	struct cipherparams cipher;
	struct authparams auth;

	jqarg = 0;

	/* Allocate more than necessary for both descs */
	sdsz = 64 * sizeof(u32);
	jdsz = 16 * sizeof(u32);
	sdesc = kzalloc(sdsz, GFP_KERNEL | GFP_DMA);
	jdesc = kzalloc(jdsz, GFP_KERNEL | GFP_DMA);

	/* Allocate buffers */
	inbufsz = 64;
	outbufsz = 116;
	inbuf = kmalloc(inbufsz, GFP_KERNEL | GFP_DMA);
	outbuf = kmalloc(outbufsz, GFP_KERNEL | GFP_DMA);

	if ((sdesc == NULL) || (jdesc == NULL) ||
	    (inbuf == NULL) || (outbuf == NULL)) {
		printk(KERN_INFO "jq_ipsec_esp_no_term: can't get buffers\n");
		kfree(sdesc);
		kfree(jdesc);
		kfree(inbuf);
		kfree(outbuf);
		return -1;
	};

	/* Fill out PDB options, no optional header */
	pdb.opthdrlen = 0;
	pdb.opthdr = NULL;
	pdb.transmode = PDB_TUNNEL;
	pdb.pclvers = PDB_IPV4;
	pdb.outfmt = PDB_OUTPUT_COPYALL;
	pdb.ivsrc = PDB_IV_FROM_RNG;
	pdb.seq.esn = PDB_NO_ESN;
	pdb.seq.antirplysz = PDB_ANTIRPLY_NONE;

	/* Do transforms */
	cipher.algtype = CIPHER_TYPE_IPSEC_AESCBC;
	cipher.key = class_1_key;
	cipher.keylen = 16 * 8; /* AES keysize in bits */

	auth.algtype = AUTH_TYPE_IPSEC_SHA1HMAC_96;
	auth.key = class_2_key;
	auth.keylen = 20 * 8; /* SHA1 keysize in bits */

	/* Now construct */
	stat = cnstr_pcl_shdsc_ipsec_cbc_encap(sdesc, &sdsz, &pdb, &cipher,
					       &auth, 0);
	if (stat) {
		printk(KERN_INFO
		       "jq_ipsec_esp_no_term: sharedesc construct failed\n");
		kfree(sdesc);
		kfree(jdesc);
		return -1;
	};

	/* Map data prior to jobdesc build */
	sdmap = (u32 *)dma_map_single(dev, sdesc, sdsz, DMA_BIDIRECTIONAL);
	inmap = (u8 *)dma_map_single(dev, inbuf, inbufsz, DMA_TO_DEVICE);
	outmap = (u8 *)dma_map_single(dev, outbuf, outbufsz, DMA_FROM_DEVICE);

	/* build jobdesc */
	cnstr_seq_jobdesc(jdesc, &jdsz, sdmap, sdsz, inmap, inbufsz,
			  outmap, outbufsz);

	/* map jobdesc */
	jdmap = (u32 *)dma_map_single(dev, jdesc, jdsz, DMA_TO_DEVICE);

	/* Show it before we run it */
	if (show == SHOW_DESC) {
		caam_desc_disasm(jdesc);
		caam_desc_disasm(sdesc);
	}

	/* Enqueue and block*/
	stat = caam_jq_enqueue(dev, jdesc, jq_ipsec_done, (void *)&jqarg);
	if (stat) {
		printk(KERN_INFO "jq_ipsec_esp_no_term: can't enqueue\n");
		rtnval = -1;
	}
	exit = wait_event_interruptible(jqtest_wq, (jqarg));
	if (exit)
		printk(KERN_INFO "jq_ipsec_esp_no_term: interrupted\n");


	dma_unmap_single(dev, (u32)sdmap, sdsz, DMA_BIDIRECTIONAL);
	dma_unmap_single(dev, (u32)jdmap, jdsz, DMA_TO_DEVICE);
	dma_unmap_single(dev, (u32)inmap, inbufsz, DMA_TO_DEVICE);
	dma_unmap_single(dev, (u32)outmap, outbufsz, DMA_FROM_DEVICE);
	kfree(sdesc);
	kfree(jdesc);
	kfree(inbuf);
	kfree(outbuf);

	return rtnval;
}
