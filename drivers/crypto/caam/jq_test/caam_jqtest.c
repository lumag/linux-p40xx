/*
 * caam_jqtest.c - Top-level code for a JobQ unit test
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

#define JQTEST_CYCLES 3

static int __init caam_jqtest(void)
{
	int stat, i, q, owned_queues, qid[4];
	struct device *qdev[4], *ctrldev;
	struct device_node *ctrlnode;
	struct of_device *ofdev;

	/* Find a CAAM instance via device tree */
	ctrlnode = of_find_compatible_node(NULL, NULL, "fsl,sec4.0");
	if (ctrlnode == NULL) {
		printk(KERN_INFO "caam_jqtest: no compatible node found\n");
		return -1;
	}
	ofdev = of_find_device_by_node(ctrlnode);
	if (ofdev == NULL) {
		printk(KERN_INFO "caam_jqtest: no device found\n");
		return -1;
	}
	ctrldev = &ofdev->dev;

	/* Get all the queues available */
	owned_queues = 0;
	for (q = 0; q < 4; q++) {
		qid[q] = caam_jq_register(ctrldev, &qdev[q]);
		if (qid[q] >= 0)
			owned_queues++;
	}

	if (!owned_queues) {
		printk(KERN_INFO "caam_jqtest: no queues available\n");
		return -1;
	}

	/* Now run cases */
	printk(KERN_INFO "caam_jqtest: running cases on %d available queues\n",
	       owned_queues);
	for (q = 0; q < owned_queues; q++) {
		for (i = 0; i < JQTEST_CYCLES; i++) {

			stat = jq_ipsec_esp_split(qdev[q], NO_SHOW_DESC);
			if (stat)
				printk(KERN_INFO
				"jq_ipsec_esp_noterm: fail on queue %d\n",
				qid[q]);

			stat = jq_aes_cbc_shared(qdev[q], NO_SHOW_DESC);
			if (stat)
				printk(KERN_INFO
				       "jq_aes_cbc_shared: fail on queue %d\n",
				       qid[q]);

			stat = jq_aes_cbc_job(qdev[q], NO_SHOW_DESC);
			if (stat)
				printk(KERN_INFO
				       "jq_aes_cbc_job: fail on queue %d\n",
				       qid[q]);

			stat = jq_snow_f8(qdev[q], NO_SHOW_DESC);
			if (stat)
				printk(KERN_INFO
				       "jq_snow_f8: fail on queue %d\n",
				       qid[q]);

			stat = jq_snow_f9(qdev[q], NO_SHOW_DESC);
			if (stat)
				printk(KERN_INFO
				       "jq_snow_f9: fail on queue %d\n",
				       qid[q]);

		}
		printk(KERN_INFO "caam_jqtest: %d cycles on queue %d\n", i, q);
	}

	/* Deregister, release queues */
	for (q = 0; q < owned_queues; q++)
		caam_jq_deregister(qdev[q]);

	return 0;
}

static void __exit caam_jqtest_remove(void)
{
	return;
}

module_init(caam_jqtest);
module_exit(caam_jqtest_remove);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("FSL CAAM JobQ Test Module");
MODULE_AUTHOR("Freescale Semiconductor - NMG/STC");
