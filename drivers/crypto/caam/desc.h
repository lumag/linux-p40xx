/*
 * CAAM descriptor composition header
 * Definitions to support CAAM descriptor instruction generation
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

#ifndef DESC_H
#define DESC_H

/* Max size of any CAAM descriptor in 32-bit words */
#define MAX_CAAM_DESCSIZE       63

/*
 * Supported descriptor command types as they show up
 * inside a descriptor command word.
 */
#define CMD_SHIFT               27
#define CMD_MASK                0xf8000000

#define CMD_KEY                 (0x00 << CMD_SHIFT)
#define CMD_SEQ_KEY             (0x01 << CMD_SHIFT)
#define CMD_LOAD                (0x02 << CMD_SHIFT)
#define CMD_SEQ_LOAD            (0x03 << CMD_SHIFT)
#define CMD_FIFO_LOAD           (0x04 << CMD_SHIFT)
#define CMD_SEQ_FIFO_LOAD       (0x05 << CMD_SHIFT)
#define CMD_STORE               (0x0a << CMD_SHIFT)
#define CMD_SEQ_STORE           (0x0b << CMD_SHIFT)
#define CMD_FIFO_STORE          (0x0c << CMD_SHIFT)
#define CMD_SEQ_FIFO_STORE      (0x0d << CMD_SHIFT)
#define CMD_MOVE                (0x0f << CMD_SHIFT)
#define CMD_OPERATION           (0x10 << CMD_SHIFT)
#define CMD_SIGNATURE           (0x12 << CMD_SHIFT)
#define CMD_JUMP                (0x14 << CMD_SHIFT)
#define CMD_MATH                (0x15 << CMD_SHIFT)
#define CMD_DESC_HDR            (0x16 << CMD_SHIFT)
#define CMD_SHARED_DESC_HDR     (0x17 << CMD_SHIFT)
#define CMD_SEQ_IN_PTR          (0x1e << CMD_SHIFT)
#define CMD_SEQ_OUT_PTR         (0x1f << CMD_SHIFT)

/* General-purpose class selector for all commands */
#define CLASS_SHIFT             25
#define CLASS_MASK              (0x03 << CLASS_SHIFT)

#define CLASS_NONE              (0x00 << CLASS_SHIFT)
#define CLASS_1                 (0x01 << CLASS_SHIFT)
#define CLASS_2                 (0x02 << CLASS_SHIFT)
#define CLASS_BOTH              (0x03 << CLASS_SHIFT)

/*
 * Descriptor header command constructs
 * Covers shared, job, and trusted descriptor headers
 */

/*
 * Do Not Run - marks a descriptor inexecutable if there was
 * a preceding error somewhere
 */
#define HDR_DNR                 0x01000000

/*
 * One - should always be set. Combination of ONE (always
 * set) and ZRO (always clear) forms an endianness sanity check
 */
#define HDR_ONE                 0x00800000

/* Start Index or SharedDesc Length */
#define HDR_START_IDX_MASK      0x3f
#define HDR_START_IDX_SHIFT     16

/* If shared descriptor header, 6-bit length */
#define HDR_DESCLEN_SHR_MASK  0x3f

/* If non-shared header, 7-bit length */
#define HDR_DESCLEN_MASK      0x7f

/* This is a TrustedDesc (if not SharedDesc) */
#define HDR_TRUSTED             0x00004000

/* Make into TrustedDesc (if not SharedDesc) */
#define HDR_MAKE_TRUSTED        0x00002000

/* Save context if self-shared (if SharedDesc) */
#define HDR_SAVECTX             0x00001000

/* Next item points to SharedDesc */
#define HDR_SHARED              0x00001000

/*
 * Reverse Execution Order - execute JobDesc first, then
 * execute SharedDesc (normally SharedDesc goes first).
 */
#define HDR_REVERSE             0x00000800

/* Propogate DNR property to SharedDesc */
#define HDR_PROP_DNR            0x00000800

/* JobDesc/SharedDesc share property */
#define HDR_SD_SHARE_MASK       0x03
#define HDR_SD_SHARE_SHIFT      8
#define HDR_JD_SHARE_MASK       0x07
#define HDR_JD_SHARE_SHIFT      8

#define HDR_SHARE_NEVER         (0x00 << HDR_SD_SHARE_SHIFT)
#define HDR_SHARE_WAIT          (0x01 << HDR_SD_SHARE_SHIFT)
#define HDR_SHARE_SERIAL        (0x02 << HDR_SD_SHARE_SHIFT)
#define HDR_SHARE_ALWAYS        (0x03 << HDR_SD_SHARE_SHIFT)
#define HDR_SHARE_DEFER         (0x04 << HDR_SD_SHARE_SHIFT)

/* JobDesc/SharedDesc descriptor length */
#define HDR_JD_LENGTH_MASK      0x7f
#define HDR_SD_LENGTH_MASK      0x3f

/*
 * KEY/SEQ_KEY Command Constructs
 */

/* Key Destination Class: 01 = Class 1, 02 - Class 2  */
#define KEY_DEST_CLASS_SHIFT    25  /* use CLASS_1 or CLASS_2 */
#define KEY_DEST_CLASS_MASK     (0x03 << KEY_DEST_CLASS_SHIFT)

/* Scatter-Gather Table/Variable Length Field */
#define KEY_SGF                 0x01000000

/* Immediate - Key follows command in the descriptor */
#define KEY_IMM                 0x00800000

/*
 * Encrypted - Key is encrypted either with the KEK, or
 * with the TDKEK if this descriptor is trusted
 */
#define KEY_ENC                 0x00400000

/*
 * KDEST - Key Destination: 0 - class key register,
 * 1 - PKHA 'e', 2 - AFHA Sbox, 3 - MDHA split-key
 */
#define KEY_DEST_SHIFT          16
#define KEY_DEST_MASK           (0x03 << KEY_DEST_SHIFT)

#define KEY_DEST_CLASS_REG      (0x00 << KEY_DEST_SHIFT)
#define KEY_DEST_PKHA_E         (0x01 << KEY_DEST_SHIFT)
#define KEY_DEST_AFHA_SBOX      (0x02 << KEY_DEST_SHIFT)
#define KEY_DEST_MDHA_SPLIT     (0x03 << KEY_DEST_SHIFT)

/* Length in bytes */
#define KEY_LENGTH_MASK         0x000003ff

/*
 * LOAD/SEQ_LOAD/STORE/SEQ_STORE Command Constructs
 */

/*
 * Load/Store Destination: 0 = class independent CCB,
 * 1 = class 1 CCB, 2 = class 2 CCB, 3 = DECO
 */
#define LDST_CLASS_SHIFT        25
#define LDST_CLASS_IND_CCB      (0x00 << LDST_CLASS_SHIFT)
#define LDST_CLASS_1_CCB        (0x01 << LDST_CLASS_SHIFT)
#define LDST_CLASS_2_CCB        (0x02 << LDST_CLASS_SHIFT)
#define LDST_CLASS_DECO         (0x03 << LDST_CLASS_SHIFT)

/* Scatter-Gather Table/Variable Length Field */
#define LDST_SGF                0x01000000

/* Immediate - Key follows this command in descriptor    */
#define LDST_IMM                0x00800000

/* SRC/DST - Destination for LOAD, Source for STORE   */
#define LDST_SRCDST_MASK        0x7f
#define LDST_SRCDST_SHIFT       16

/* Offset in source/destination                        */
#define LDST_OFFSET_MASK        0xff
#define LDST_OFFSET_SHIFT       8

/* Data length in bytes                                 */
#define LDST_LEN_MASK           0xff
#define LDST_LEN_SHIFT          0

/*
 * FIFO_LOAD/FIFO_STORE/SEQ_FIFO_LOAD/SEQ_FIFO_STORE
 * Command Constructs
 */

/*
 * Load Destination: 0 = skip (SEQ_FIFO_LOAD only),
 * 1 = Load for Class1, 2 = Load for Class2, 3 = Load both
 * Store Source: 0 = normal, 1 = Class1key, 2 = Class2key
 */
#define FIFOLD_CLASS_SHIFT      25
#define FIFOLD_CLASS_SKIP       (0x00 << FIFOLD_CLASS_SHIFT)
#define FIFOLD_CLASS_CLASS1     (0x01 << FIFOLD_CLASS_SHIFT)
#define FIFOLD_CLASS_CLASS2     (0x02 << FIFOLD_CLASS_SHIFT)
#define FIFOLD_CLASS_BOTH       (0x03 << FIFOLD_CLASS_SHIFT)

#define FIFOST_CLASS_NORMAL     (0x00 << FIFOLD_CLASS_SHIFT)
#define FIFOST_CLASS_CLASS1KEY  (0x01 << FIFOLD_CLASS_SHIFT)
#define FIFOST_CLASS_CLASS2KEY  (0x02 << FIFOLD_CLASS_SHIFT)

/*
 * Scatter-Gather Table/Variable Length Field
 * If set for FIFO_LOAD, refers to a SG table. Within
 * SEQ_FIFO_LOAD, is variable input sequence
 */
#define FIFOLDST_SGF            0x01000000

/* Immediate - Key follows command in descriptor */
#define FIFOLDST_IMM            0x00800000

/*
 * Extended Length - use 32-bit extended length that
 * follows the pointer field. Illegal with IMM set
 */
#define FIFOLDST_EXT            0x00400000

/* Input data type.*/
#define FIFOLD_TYPE_SHIFT       16

/* PK types */
#define FIFOLD_TYPE_PK_A0       (0x00 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_PK_A1       (0x01 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_PK_A2       (0x02 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_PK_A3       (0x03 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_PK_B0       (0x04 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_PK_B1       (0x05 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_PK_B2       (0x06 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_PK_B3       (0x07 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_PK_N        (0x08 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_PK_A        (0x0c << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_PK_B        (0x0d << FIFOLD_TYPE_SHIFT)

/* Other types. Need to OR in last/flush bits as desired */
#define FIFOLD_TYPE_MSG         (0x10 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_MSG1OUT2    (0x18 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_IV          (0x20 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_BITDATA     (0x28 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_AAD         (0x30 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_ICV         (0x38 << FIFOLD_TYPE_SHIFT)

/* Last/Flush bits for use with "other" types above */
#define FIFOLD_TYPE_NOACTION    (0x00 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_FLUSH1      (0x01 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_LAST1       (0x02 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_LAST2FLUSH  (0x03 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_LAST2       (0x04 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_LAST2FLUSH1 (0x05 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_LASTBOTH    (0x06 << FIFOLD_TYPE_SHIFT)
#define FIFOLD_TYPE_LASTBOTHFL  (0x07 << FIFOLD_TYPE_SHIFT)

/*
 * OPERATION Command Constructs
 */

/* Operation type selectors - OP TYPE */
#define OP_TYPE_SHIFT           24
#define OP_TYPE_MASK            (0x07 << OP_TYPE_SHIFT)

#define OP_TYPE_UNI_PROTOCOL    (0x00 << OP_TYPE_SHIFT)
#define OP_TYPE_PK              (0x01 << OP_TYPE_SHIFT)
#define OP_TYPE_CLASS1_ALG      (0x02 << OP_TYPE_SHIFT)
#define OP_TYPE_CLASS2_ALG      (0x04 << OP_TYPE_SHIFT)
#define OP_TYPE_DECAP_PROTOCOL  (0x06 << OP_TYPE_SHIFT)
#define OP_TYPE_ENCAP_PROTOCOL  (0x07 << OP_TYPE_SHIFT)

/* ProtocolID selectors - PROTID */
#define OP_PCLID_SHIFT          16
#define OP_PCLID_MASK           (0xff << 16)

/* Assuming OP_TYPE = OP_TYPE_UNI_PROTOCOL */
#define OP_PCLID_IKEV1_PRF      (0x01 << OP_PCLID_SHIFT)
#define OP_PCLID_IKEV2_PRF      (0x02 << OP_PCLID_SHIFT)
#define OP_PCLID_SSL30_PRF      (0x08 << OP_PCLID_SHIFT)
#define OP_PCLID_TLS10_PRF      (0x09 << OP_PCLID_SHIFT)
#define OP_PCLID_TLS11_PRF      (0x0a << OP_PCLID_SHIFT)
#define OP_PCLID_DTLS10_PRF     (0x0c << OP_PCLID_SHIFT)

/* Assuming OP_TYPE = OP_TYPE_DECAP_PROTOCOL/ENCAP_PROTOCOL */
#define OP_PCLID_IPSEC          (0x01 << OP_PCLID_SHIFT)
#define OP_PCLID_SRTP           (0x02 << OP_PCLID_SHIFT)
#define OP_PCLID_MACSEC         (0x03 << OP_PCLID_SHIFT)
#define OP_PCLID_WIFI           (0x04 << OP_PCLID_SHIFT)
#define OP_PCLID_WIMAX          (0x05 << OP_PCLID_SHIFT)
#define OP_PCLID_SSL30          (0x08 << OP_PCLID_SHIFT)
#define OP_PCLID_TLS10          (0x09 << OP_PCLID_SHIFT)
#define OP_PCLID_TLS11          (0x0a << OP_PCLID_SHIFT)
#define OP_PCLID_TLS12          (0x0b << OP_PCLID_SHIFT)
#define OP_PCLID_DTLS           (0x0c << OP_PCLID_SHIFT)

/* Assuming OP_TYPE = other... */
#define OP_PCLID_PRF            (0x06 << OP_PCLID_SHIFT)
#define OP_PCLID_BLOB           (0x0d << OP_PCLID_SHIFT)
#define OP_PCLID_SECRETKEY      (0x11 << OP_PCLID_SHIFT)
#define OP_PCLID_PUBLICKEYPAIR  (0x14 << OP_PCLID_SHIFT)
#define OP_PCLID_DSASIGN        (0x15 << OP_PCLID_SHIFT)
#define OP_PCLID_DSAVERIFY      (0x16 << OP_PCLID_SHIFT)

/*
 * ProtocolInfo selectors
 */
#define OP_PCLINFO_MASK                          0xffff

/* for OP_PCLID_IPSEC */
#define OP_PCL_IPSEC_CIPHER_MASK                 0xff00
#define OP_PCL_IPSEC_AUTH_MASK                   0x00ff

#define OP_PCL_IPSEC_DES_IV64                    0x0100
#define OP_PCL_IPSEC_DES                         0x0200
#define OP_PCL_IPSEC_3DES                        0x0300
#define OP_PCL_IPSEC_AES_CBC                     0x0c00
#define OP_PCL_IPSEC_AES_CTR                     0x0d00
#define OP_PCL_IPSEC_AES_XTS                     0x1600
#define OP_PCL_IPSEC_AES_CCM8                    0x0e00
#define OP_PCL_IPSEC_AES_CCM12                   0x0f00
#define OP_PCL_IPSEC_AES_CCM16                   0x1000
#define OP_PCL_IPSEC_AES_GCM8                    0x1200
#define OP_PCL_IPSEC_AES_GCM12                   0x1300
#define OP_PCL_IPSEC_AES_GCM16                   0x1400

#define OP_PCL_IPSEC_HMAC_NULL                   0x0000
#define OP_PCL_IPSEC_HMAC_MD5_96                 0x0001
#define OP_PCL_IPSEC_HMAC_SHA1_96                0x0002
#define OP_PCL_IPSEC_AES_XCBC_MAC_96             0x0005
#define OP_PCL_IPSEC_HMAC_MD5_128                0x0006
#define OP_PCL_IPSEC_HMAC_SHA1_160               0x0007
#define OP_PCL_IPSEC_HMAC_SHA2_256_128           0x000c
#define OP_PCL_IPSEC_HMAC_SHA2_384_192           0x000d
#define OP_PCL_IPSEC_HMAC_SHA2_512_256           0x000e

/* For SRTP - OP_PCLID_SRTP */
#define OP_PCL_SRTP_CIPHER_MASK                  0xff00
#define OP_PCL_SRTP_AUTH_MASK                    0x00ff

#define OP_PCL_SRTP_AES_CTR                      0x0d00

#define OP_PCL_SRTP_HMAC_SHA1_160                0x0007

/* For SSL 3.0 - OP_PCLID_SSL30 */
#define OP_PCL_SSL30_AES_128_CBC_SHA             0x002f
#define OP_PCL_SSL30_AES_128_CBC_SHA_2           0x0030
#define OP_PCL_SSL30_AES_128_CBC_SHA_3           0x0031
#define OP_PCL_SSL30_AES_128_CBC_SHA_4           0x0032
#define OP_PCL_SSL30_AES_128_CBC_SHA_5           0x0033
#define OP_PCL_SSL30_AES_128_CBC_SHA_6           0x0034
#define OP_PCL_SSL30_AES_128_CBC_SHA_7           0x008c
#define OP_PCL_SSL30_AES_128_CBC_SHA_8           0x0090
#define OP_PCL_SSL30_AES_128_CBC_SHA_9           0x0094
#define OP_PCL_SSL30_AES_128_CBC_SHA_10          0xc004
#define OP_PCL_SSL30_AES_128_CBC_SHA_11          0xc009
#define OP_PCL_SSL30_AES_128_CBC_SHA_12          0xc00e
#define OP_PCL_SSL30_AES_128_CBC_SHA_13          0xc013
#define OP_PCL_SSL30_AES_128_CBC_SHA_14          0xc018
#define OP_PCL_SSL30_AES_128_CBC_SHA_15          0xc01d
#define OP_PCL_SSL30_AES_128_CBC_SHA_16          0xc01e
#define OP_PCL_SSL30_AES_128_CBC_SHA_17          0xc01f

#define OP_PCL_SSL30_AES_256_CBC_SHA             0x0035
#define OP_PCL_SSL30_AES_256_CBC_SHA_2           0x0036
#define OP_PCL_SSL30_AES_256_CBC_SHA_3           0x0037
#define OP_PCL_SSL30_AES_256_CBC_SHA_4           0x0038
#define OP_PCL_SSL30_AES_256_CBC_SHA_5           0x0039
#define OP_PCL_SSL30_AES_256_CBC_SHA_6           0x003a
#define OP_PCL_SSL30_AES_256_CBC_SHA_7           0x008d
#define OP_PCL_SSL30_AES_256_CBC_SHA_8           0x0091
#define OP_PCL_SSL30_AES_256_CBC_SHA_9           0x0095
#define OP_PCL_SSL30_AES_256_CBC_SHA_10          0xc005
#define OP_PCL_SSL30_AES_256_CBC_SHA_11          0xc00a
#define OP_PCL_SSL30_AES_256_CBC_SHA_12          0xc00f
#define OP_PCL_SSL30_AES_256_CBC_SHA_13          0xc014
#define OP_PCL_SSL30_AES_256_CBC_SHA_14          0xc019
#define OP_PCL_SSL30_AES_256_CBC_SHA_15          0xc020
#define OP_PCL_SSL30_AES_256_CBC_SHA_16          0xc021
#define OP_PCL_SSL30_AES_256_CBC_SHA_17          0xc022

#define OP_PCL_SSL30_3DES_EDE_CBC_MD5            0x0023

#define OP_PCL_SSL30_3DES_EDE_CBC_SHA            0x001f
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_2          0x008b
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_3          0x008f
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_4          0x0093
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_5          0x000a
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_6          0x000d
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_7          0x0010
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_8          0x0013
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_9          0x0016
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_10         0x001b
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_11         0xc003
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_12         0xc008
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_13         0xc00d
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_14         0xc012
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_15         0xc017
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_16         0xc01a
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_17         0xc01b
#define OP_PCL_SSL30_3DES_EDE_CBC_SHA_18         0xc01c

#define OP_PCL_SSL30_DES40_CBC_MD5               0x0029

#define OP_PCL_SSL30_DES_CBC_MD5                 0x0022

#define OP_PCL_SSL30_DES40_CBC_SHA               0x0008
#define OP_PCL_SSL30_DES40_CBC_SHA_2             0x000b
#define OP_PCL_SSL30_DES40_CBC_SHA_3             0x000e
#define OP_PCL_SSL30_DES40_CBC_SHA_4             0x0011
#define OP_PCL_SSL30_DES40_CBC_SHA_5             0x0014
#define OP_PCL_SSL30_DES40_CBC_SHA_6             0x0019
#define OP_PCL_SSL30_DES40_CBC_SHA_7             0x0026

#define OP_PCL_SSL30_DES_CBC_SHA                 0x001e
#define OP_PCL_SSL30_DES_CBC_SHA_2               0x0009
#define OP_PCL_SSL30_DES_CBC_SHA_3               0x000c
#define OP_PCL_SSL30_DES_CBC_SHA_4               0x000f
#define OP_PCL_SSL30_DES_CBC_SHA_5               0x0012
#define OP_PCL_SSL30_DES_CBC_SHA_6               0x0015
#define OP_PCL_SSL30_DES_CBC_SHA_7               0x001a

#define OP_PCL_SSL30_RC4_128_MD5                 0x0024
#define OP_PCL_SSL30_RC4_128_MD5_2               0x0004
#define OP_PCL_SSL30_RC4_128_MD5_3               0x0018

#define OP_PCL_SSL30_RC4_40_MD5                  0x002b
#define OP_PCL_SSL30_RC4_40_MD5_2                0x0003
#define OP_PCL_SSL30_RC4_40_MD5_3                0x0017

#define OP_PCL_SSL30_RC4_128_SHA                 0x0020
#define OP_PCL_SSL30_RC4_128_SHA_2               0x008a
#define OP_PCL_SSL30_RC4_128_SHA_3               0x008e
#define OP_PCL_SSL30_RC4_128_SHA_4               0x0092
#define OP_PCL_SSL30_RC4_128_SHA_5               0x0005
#define OP_PCL_SSL30_RC4_128_SHA_6               0xc002
#define OP_PCL_SSL30_RC4_128_SHA_7               0xc007
#define OP_PCL_SSL30_RC4_128_SHA_8               0xc00c
#define OP_PCL_SSL30_RC4_128_SHA_9               0xc011
#define OP_PCL_SSL30_RC4_128_SHA_10              0xc016

#define OP_PCL_SSL30_RC4_40_SHA                  0x0028


/* For TLS 1.0 - OP_PCLID_TLS10 */
#define OP_PCL_TLS10_AES_128_CBC_SHA             0x002f
#define OP_PCL_TLS10_AES_128_CBC_SHA_2           0x0030
#define OP_PCL_TLS10_AES_128_CBC_SHA_3           0x0031
#define OP_PCL_TLS10_AES_128_CBC_SHA_4           0x0032
#define OP_PCL_TLS10_AES_128_CBC_SHA_5           0x0033
#define OP_PCL_TLS10_AES_128_CBC_SHA_6           0x0034
#define OP_PCL_TLS10_AES_128_CBC_SHA_7           0x008c
#define OP_PCL_TLS10_AES_128_CBC_SHA_8           0x0090
#define OP_PCL_TLS10_AES_128_CBC_SHA_9           0x0094
#define OP_PCL_TLS10_AES_128_CBC_SHA_10          0xc004
#define OP_PCL_TLS10_AES_128_CBC_SHA_11          0xc009
#define OP_PCL_TLS10_AES_128_CBC_SHA_12          0xc00e
#define OP_PCL_TLS10_AES_128_CBC_SHA_13          0xc013
#define OP_PCL_TLS10_AES_128_CBC_SHA_14          0xc018
#define OP_PCL_TLS10_AES_128_CBC_SHA_15          0xc01d
#define OP_PCL_TLS10_AES_128_CBC_SHA_16          0xc01e
#define OP_PCL_TLS10_AES_128_CBC_SHA_17          0xc01f

#define OP_PCL_TLS10_AES_256_CBC_SHA             0x0035
#define OP_PCL_TLS10_AES_256_CBC_SHA_2           0x0036
#define OP_PCL_TLS10_AES_256_CBC_SHA_3           0x0037
#define OP_PCL_TLS10_AES_256_CBC_SHA_4           0x0038
#define OP_PCL_TLS10_AES_256_CBC_SHA_5           0x0039
#define OP_PCL_TLS10_AES_256_CBC_SHA_6           0x003a
#define OP_PCL_TLS10_AES_256_CBC_SHA_7           0x008d
#define OP_PCL_TLS10_AES_256_CBC_SHA_8           0x0091
#define OP_PCL_TLS10_AES_256_CBC_SHA_9           0x0095
#define OP_PCL_TLS10_AES_256_CBC_SHA_10          0xc005
#define OP_PCL_TLS10_AES_256_CBC_SHA_11          0xc00a
#define OP_PCL_TLS10_AES_256_CBC_SHA_12          0xc00f
#define OP_PCL_TLS10_AES_256_CBC_SHA_13          0xc014
#define OP_PCL_TLS10_AES_256_CBC_SHA_14          0xc019
#define OP_PCL_TLS10_AES_256_CBC_SHA_15          0xc020
#define OP_PCL_TLS10_AES_256_CBC_SHA_16          0xc021
#define OP_PCL_TLS10_AES_256_CBC_SHA_17          0xc022

/* #define OP_PCL_TLS10_3DES_EDE_CBC_MD5            0x0023 */

#define OP_PCL_TLS10_3DES_EDE_CBC_SHA            0x001f
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_2          0x008b
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_3          0x008f
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_4          0x0093
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_5          0x000a
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_6          0x000d
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_7          0x0010
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_8          0x0013
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_9          0x0016
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_10         0x001b
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_11         0xc003
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_12         0xc008
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_13         0xc00d
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_14         0xc012
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_15         0xc017
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_16         0xc01a
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_17         0xc01b
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA_18         0xc01c

#define OP_PCL_TLS10_DES40_CBC_MD5               0x0029

#define OP_PCL_TLS10_DES_CBC_MD5                 0x0022

#define OP_PCL_TLS10_DES40_CBC_SHA               0x0008
#define OP_PCL_TLS10_DES40_CBC_SHA_2             0x000b
#define OP_PCL_TLS10_DES40_CBC_SHA_3             0x000e
#define OP_PCL_TLS10_DES40_CBC_SHA_4             0x0011
#define OP_PCL_TLS10_DES40_CBC_SHA_5             0x0014
#define OP_PCL_TLS10_DES40_CBC_SHA_6             0x0019
#define OP_PCL_TLS10_DES40_CBC_SHA_7             0x0026


#define OP_PCL_TLS10_DES_CBC_SHA                 0x001e
#define OP_PCL_TLS10_DES_CBC_SHA_2               0x0009
#define OP_PCL_TLS10_DES_CBC_SHA_3               0x000c
#define OP_PCL_TLS10_DES_CBC_SHA_4               0x000f
#define OP_PCL_TLS10_DES_CBC_SHA_5               0x0012
#define OP_PCL_TLS10_DES_CBC_SHA_6               0x0015
#define OP_PCL_TLS10_DES_CBC_SHA_7               0x001a

#define OP_PCL_TLS10_RC4_128_MD5                 0x0024
#define OP_PCL_TLS10_RC4_128_MD5_2               0x0004
#define OP_PCL_TLS10_RC4_128_MD5_3               0x0018

#define OP_PCL_TLS10_RC4_40_MD5                  0x002b
#define OP_PCL_TLS10_RC4_40_MD5_2                0x0003
#define OP_PCL_TLS10_RC4_40_MD5_3                0x0017

#define OP_PCL_TLS10_RC4_128_SHA                 0x0020
#define OP_PCL_TLS10_RC4_128_SHA_2               0x008a
#define OP_PCL_TLS10_RC4_128_SHA_3               0x008e
#define OP_PCL_TLS10_RC4_128_SHA_4               0x0092
#define OP_PCL_TLS10_RC4_128_SHA_5               0x0005
#define OP_PCL_TLS10_RC4_128_SHA_6               0xc002
#define OP_PCL_TLS10_RC4_128_SHA_7               0xc007
#define OP_PCL_TLS10_RC4_128_SHA_8               0xc00c
#define OP_PCL_TLS10_RC4_128_SHA_9               0xc011
#define OP_PCL_TLS10_RC4_128_SHA_10              0xc016

#define OP_PCL_TLS10_RC4_40_SHA                  0x0028

#define OP_PCL_TLS10_3DES_EDE_CBC_MD5            0xff23
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA160         0xff30
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA224         0xff34
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA256         0xff36
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA384         0xff33
#define OP_PCL_TLS10_3DES_EDE_CBC_SHA512         0xff35
#define OP_PCL_TLS10_AES_128_CBC_SHA160          0xff80
#define OP_PCL_TLS10_AES_128_CBC_SHA224          0xff84
#define OP_PCL_TLS10_AES_128_CBC_SHA256          0xff86
#define OP_PCL_TLS10_AES_128_CBC_SHA384          0xff83
#define OP_PCL_TLS10_AES_128_CBC_SHA512          0xff85
#define OP_PCL_TLS10_AES_192_CBC_SHA160          0xff20
#define OP_PCL_TLS10_AES_192_CBC_SHA224          0xff24
#define OP_PCL_TLS10_AES_192_CBC_SHA256          0xff26
#define OP_PCL_TLS10_AES_192_CBC_SHA384          0xff23
#define OP_PCL_TLS10_AES_192_CBC_SHA512          0xff25
#define OP_PCL_TLS10_AES_256_CBC_SHA160          0xff60
#define OP_PCL_TLS10_AES_256_CBC_SHA224          0xff64
#define OP_PCL_TLS10_AES_256_CBC_SHA256          0xff66
#define OP_PCL_TLS10_AES_256_CBC_SHA384          0xff63
#define OP_PCL_TLS10_AES_256_CBC_SHA512          0xff65



/* For TLS 1.1 - OP_PCLID_TLS11 */
#define OP_PCL_TLS11_AES_128_CBC_SHA             0x002f
#define OP_PCL_TLS11_AES_128_CBC_SHA_2           0x0030
#define OP_PCL_TLS11_AES_128_CBC_SHA_3           0x0031
#define OP_PCL_TLS11_AES_128_CBC_SHA_4           0x0032
#define OP_PCL_TLS11_AES_128_CBC_SHA_5           0x0033
#define OP_PCL_TLS11_AES_128_CBC_SHA_6           0x0034
#define OP_PCL_TLS11_AES_128_CBC_SHA_7           0x008c
#define OP_PCL_TLS11_AES_128_CBC_SHA_8           0x0090
#define OP_PCL_TLS11_AES_128_CBC_SHA_9           0x0094
#define OP_PCL_TLS11_AES_128_CBC_SHA_10          0xc004
#define OP_PCL_TLS11_AES_128_CBC_SHA_11          0xc009
#define OP_PCL_TLS11_AES_128_CBC_SHA_12          0xc00e
#define OP_PCL_TLS11_AES_128_CBC_SHA_13          0xc013
#define OP_PCL_TLS11_AES_128_CBC_SHA_14          0xc018
#define OP_PCL_TLS11_AES_128_CBC_SHA_15          0xc01d
#define OP_PCL_TLS11_AES_128_CBC_SHA_16          0xc01e
#define OP_PCL_TLS11_AES_128_CBC_SHA_17          0xc01f

#define OP_PCL_TLS11_AES_256_CBC_SHA             0x0035
#define OP_PCL_TLS11_AES_256_CBC_SHA_2           0x0036
#define OP_PCL_TLS11_AES_256_CBC_SHA_3           0x0037
#define OP_PCL_TLS11_AES_256_CBC_SHA_4           0x0038
#define OP_PCL_TLS11_AES_256_CBC_SHA_5           0x0039
#define OP_PCL_TLS11_AES_256_CBC_SHA_6           0x003a
#define OP_PCL_TLS11_AES_256_CBC_SHA_7           0x008d
#define OP_PCL_TLS11_AES_256_CBC_SHA_8           0x0091
#define OP_PCL_TLS11_AES_256_CBC_SHA_9           0x0095
#define OP_PCL_TLS11_AES_256_CBC_SHA_10          0xc005
#define OP_PCL_TLS11_AES_256_CBC_SHA_11          0xc00a
#define OP_PCL_TLS11_AES_256_CBC_SHA_12          0xc00f
#define OP_PCL_TLS11_AES_256_CBC_SHA_13          0xc014
#define OP_PCL_TLS11_AES_256_CBC_SHA_14          0xc019
#define OP_PCL_TLS11_AES_256_CBC_SHA_15          0xc020
#define OP_PCL_TLS11_AES_256_CBC_SHA_16          0xc021
#define OP_PCL_TLS11_AES_256_CBC_SHA_17          0xc022

/* #define OP_PCL_TLS11_3DES_EDE_CBC_MD5            0x0023 */

#define OP_PCL_TLS11_3DES_EDE_CBC_SHA            0x001f
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_2          0x008b
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_3          0x008f
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_4          0x0093
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_5          0x000a
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_6          0x000d
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_7          0x0010
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_8          0x0013
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_9          0x0016
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_10         0x001b
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_11         0xc003
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_12         0xc008
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_13         0xc00d
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_14         0xc012
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_15         0xc017
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_16         0xc01a
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_17         0xc01b
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA_18         0xc01c

#define OP_PCL_TLS11_DES40_CBC_MD5               0x0029

#define OP_PCL_TLS11_DES_CBC_MD5                 0x0022

#define OP_PCL_TLS11_DES40_CBC_SHA               0x0008
#define OP_PCL_TLS11_DES40_CBC_SHA_2             0x000b
#define OP_PCL_TLS11_DES40_CBC_SHA_3             0x000e
#define OP_PCL_TLS11_DES40_CBC_SHA_4             0x0011
#define OP_PCL_TLS11_DES40_CBC_SHA_5             0x0014
#define OP_PCL_TLS11_DES40_CBC_SHA_6             0x0019
#define OP_PCL_TLS11_DES40_CBC_SHA_7             0x0026

#define OP_PCL_TLS11_DES_CBC_SHA                 0x001e
#define OP_PCL_TLS11_DES_CBC_SHA_2               0x0009
#define OP_PCL_TLS11_DES_CBC_SHA_3               0x000c
#define OP_PCL_TLS11_DES_CBC_SHA_4               0x000f
#define OP_PCL_TLS11_DES_CBC_SHA_5               0x0012
#define OP_PCL_TLS11_DES_CBC_SHA_6               0x0015
#define OP_PCL_TLS11_DES_CBC_SHA_7               0x001a

#define OP_PCL_TLS11_RC4_128_MD5                 0x0024
#define OP_PCL_TLS11_RC4_128_MD5_2               0x0004
#define OP_PCL_TLS11_RC4_128_MD5_3               0x0018

#define OP_PCL_TLS11_RC4_40_MD5                  0x002b
#define OP_PCL_TLS11_RC4_40_MD5_2                0x0003
#define OP_PCL_TLS11_RC4_40_MD5_3                0x0017

#define OP_PCL_TLS11_RC4_128_SHA                 0x0020
#define OP_PCL_TLS11_RC4_128_SHA_2               0x008a
#define OP_PCL_TLS11_RC4_128_SHA_3               0x008e
#define OP_PCL_TLS11_RC4_128_SHA_4               0x0092
#define OP_PCL_TLS11_RC4_128_SHA_5               0x0005
#define OP_PCL_TLS11_RC4_128_SHA_6               0xc002
#define OP_PCL_TLS11_RC4_128_SHA_7               0xc007
#define OP_PCL_TLS11_RC4_128_SHA_8               0xc00c
#define OP_PCL_TLS11_RC4_128_SHA_9               0xc011
#define OP_PCL_TLS11_RC4_128_SHA_10              0xc016

#define OP_PCL_TLS11_RC4_40_SHA                  0x0028

#define OP_PCL_TLS11_3DES_EDE_CBC_MD5            0xff23
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA160         0xff30
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA224         0xff34
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA256         0xff36
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA384         0xff33
#define OP_PCL_TLS11_3DES_EDE_CBC_SHA512         0xff35
#define OP_PCL_TLS11_AES_128_CBC_SHA160          0xff80
#define OP_PCL_TLS11_AES_128_CBC_SHA224          0xff84
#define OP_PCL_TLS11_AES_128_CBC_SHA256          0xff86
#define OP_PCL_TLS11_AES_128_CBC_SHA384          0xff83
#define OP_PCL_TLS11_AES_128_CBC_SHA512          0xff85
#define OP_PCL_TLS11_AES_192_CBC_SHA160          0xff20
#define OP_PCL_TLS11_AES_192_CBC_SHA224          0xff24
#define OP_PCL_TLS11_AES_192_CBC_SHA256          0xff26
#define OP_PCL_TLS11_AES_192_CBC_SHA384          0xff23
#define OP_PCL_TLS11_AES_192_CBC_SHA512          0xff25
#define OP_PCL_TLS11_AES_256_CBC_SHA160          0xff60
#define OP_PCL_TLS11_AES_256_CBC_SHA224          0xff64
#define OP_PCL_TLS11_AES_256_CBC_SHA256          0xff66
#define OP_PCL_TLS11_AES_256_CBC_SHA384          0xff63
#define OP_PCL_TLS11_AES_256_CBC_SHA512          0xff65


/* For TLS 1.2 - OP_PCLID_TLS12 */
#define OP_PCL_TLS12_AES_128_CBC_SHA             0x002f
#define OP_PCL_TLS12_AES_128_CBC_SHA_2           0x0030
#define OP_PCL_TLS12_AES_128_CBC_SHA_3           0x0031
#define OP_PCL_TLS12_AES_128_CBC_SHA_4           0x0032
#define OP_PCL_TLS12_AES_128_CBC_SHA_5           0x0033
#define OP_PCL_TLS12_AES_128_CBC_SHA_6           0x0034
#define OP_PCL_TLS12_AES_128_CBC_SHA_7           0x008c
#define OP_PCL_TLS12_AES_128_CBC_SHA_8           0x0090
#define OP_PCL_TLS12_AES_128_CBC_SHA_9           0x0094
#define OP_PCL_TLS12_AES_128_CBC_SHA_10          0xc004
#define OP_PCL_TLS12_AES_128_CBC_SHA_11          0xc009
#define OP_PCL_TLS12_AES_128_CBC_SHA_12          0xc00e
#define OP_PCL_TLS12_AES_128_CBC_SHA_13          0xc013
#define OP_PCL_TLS12_AES_128_CBC_SHA_14          0xc018
#define OP_PCL_TLS12_AES_128_CBC_SHA_15          0xc01d
#define OP_PCL_TLS12_AES_128_CBC_SHA_16          0xc01e
#define OP_PCL_TLS12_AES_128_CBC_SHA_17          0xc01f

#define OP_PCL_TLS12_AES_256_CBC_SHA             0x0035
#define OP_PCL_TLS12_AES_256_CBC_SHA_2           0x0036
#define OP_PCL_TLS12_AES_256_CBC_SHA_3           0x0037
#define OP_PCL_TLS12_AES_256_CBC_SHA_4           0x0038
#define OP_PCL_TLS12_AES_256_CBC_SHA_5           0x0039
#define OP_PCL_TLS12_AES_256_CBC_SHA_6           0x003a
#define OP_PCL_TLS12_AES_256_CBC_SHA_7           0x008d
#define OP_PCL_TLS12_AES_256_CBC_SHA_8           0x0091
#define OP_PCL_TLS12_AES_256_CBC_SHA_9           0x0095
#define OP_PCL_TLS12_AES_256_CBC_SHA_10          0xc005
#define OP_PCL_TLS12_AES_256_CBC_SHA_11          0xc00a
#define OP_PCL_TLS12_AES_256_CBC_SHA_12          0xc00f
#define OP_PCL_TLS12_AES_256_CBC_SHA_13          0xc014
#define OP_PCL_TLS12_AES_256_CBC_SHA_14          0xc019
#define OP_PCL_TLS12_AES_256_CBC_SHA_15          0xc020
#define OP_PCL_TLS12_AES_256_CBC_SHA_16          0xc021
#define OP_PCL_TLS12_AES_256_CBC_SHA_17          0xc022

/* #define OP_PCL_TLS12_3DES_EDE_CBC_MD5            0x0023 */

#define OP_PCL_TLS12_3DES_EDE_CBC_SHA            0x001f
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_2          0x008b
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_3          0x008f
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_4          0x0093
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_5          0x000a
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_6          0x000d
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_7          0x0010
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_8          0x0013
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_9          0x0016
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_10         0x001b
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_11         0xc003
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_12         0xc008
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_13         0xc00d
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_14         0xc012
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_15         0xc017
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_16         0xc01a
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_17         0xc01b
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA_18         0xc01c

#define OP_PCL_TLS12_DES40_CBC_MD5               0x0029

#define OP_PCL_TLS12_DES_CBC_MD5                 0x0022

#define OP_PCL_TLS12_DES40_CBC_SHA               0x0008
#define OP_PCL_TLS12_DES40_CBC_SHA_2             0x000b
#define OP_PCL_TLS12_DES40_CBC_SHA_3             0x000e
#define OP_PCL_TLS12_DES40_CBC_SHA_4             0x0011
#define OP_PCL_TLS12_DES40_CBC_SHA_5             0x0014
#define OP_PCL_TLS12_DES40_CBC_SHA_6             0x0019
#define OP_PCL_TLS12_DES40_CBC_SHA_7             0x0026

#define OP_PCL_TLS12_DES_CBC_SHA                 0x001e
#define OP_PCL_TLS12_DES_CBC_SHA_2               0x0009
#define OP_PCL_TLS12_DES_CBC_SHA_3               0x000c
#define OP_PCL_TLS12_DES_CBC_SHA_4               0x000f
#define OP_PCL_TLS12_DES_CBC_SHA_5               0x0012
#define OP_PCL_TLS12_DES_CBC_SHA_6               0x0015
#define OP_PCL_TLS12_DES_CBC_SHA_7               0x001a

#define OP_PCL_TLS12_RC4_128_MD5                 0x0024
#define OP_PCL_TLS12_RC4_128_MD5_2               0x0004
#define OP_PCL_TLS12_RC4_128_MD5_3               0x0018

#define OP_PCL_TLS12_RC4_40_MD5                  0x002b
#define OP_PCL_TLS12_RC4_40_MD5_2                0x0003
#define OP_PCL_TLS12_RC4_40_MD5_3                0x0017

#define OP_PCL_TLS12_RC4_128_SHA                 0x0020
#define OP_PCL_TLS12_RC4_128_SHA_2               0x008a
#define OP_PCL_TLS12_RC4_128_SHA_3               0x008e
#define OP_PCL_TLS12_RC4_128_SHA_4               0x0092
#define OP_PCL_TLS12_RC4_128_SHA_5               0x0005
#define OP_PCL_TLS12_RC4_128_SHA_6               0xc002
#define OP_PCL_TLS12_RC4_128_SHA_7               0xc007
#define OP_PCL_TLS12_RC4_128_SHA_8               0xc00c
#define OP_PCL_TLS12_RC4_128_SHA_9               0xc011
#define OP_PCL_TLS12_RC4_128_SHA_10              0xc016

#define OP_PCL_TLS12_RC4_40_SHA                  0x0028

/* #define OP_PCL_TLS12_AES_128_CBC_SHA256          0x003c */
#define OP_PCL_TLS12_AES_128_CBC_SHA256_2        0x003e
#define OP_PCL_TLS12_AES_128_CBC_SHA256_3        0x003f
#define OP_PCL_TLS12_AES_128_CBC_SHA256_4        0x0040
#define OP_PCL_TLS12_AES_128_CBC_SHA256_5        0x0067
#define OP_PCL_TLS12_AES_128_CBC_SHA256_6        0x006c

/* #define OP_PCL_TLS12_AES_256_CBC_SHA256          0x003d */
#define OP_PCL_TLS12_AES_256_CBC_SHA256_2        0x0068
#define OP_PCL_TLS12_AES_256_CBC_SHA256_3        0x0069
#define OP_PCL_TLS12_AES_256_CBC_SHA256_4        0x006a
#define OP_PCL_TLS12_AES_256_CBC_SHA256_5        0x006b
#define OP_PCL_TLS12_AES_256_CBC_SHA256_6        0x006d

/* AEAD_AES_xxx_CCM/GCM remain to be defined... */

#define OP_PCL_TLS12_3DES_EDE_CBC_MD5            0xff23
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA160         0xff30
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA224         0xff34
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA256         0xff36
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA384         0xff33
#define OP_PCL_TLS12_3DES_EDE_CBC_SHA512         0xff35
#define OP_PCL_TLS12_AES_128_CBC_SHA160          0xff80
#define OP_PCL_TLS12_AES_128_CBC_SHA224          0xff84
#define OP_PCL_TLS12_AES_128_CBC_SHA256          0xff86
#define OP_PCL_TLS12_AES_128_CBC_SHA384          0xff83
#define OP_PCL_TLS12_AES_128_CBC_SHA512          0xff85
#define OP_PCL_TLS12_AES_192_CBC_SHA160          0xff20
#define OP_PCL_TLS12_AES_192_CBC_SHA224          0xff24
#define OP_PCL_TLS12_AES_192_CBC_SHA256          0xff26
#define OP_PCL_TLS12_AES_192_CBC_SHA384          0xff23
#define OP_PCL_TLS12_AES_192_CBC_SHA512          0xff25
#define OP_PCL_TLS12_AES_256_CBC_SHA160          0xff60
#define OP_PCL_TLS12_AES_256_CBC_SHA224          0xff64
#define OP_PCL_TLS12_AES_256_CBC_SHA256          0xff66
#define OP_PCL_TLS12_AES_256_CBC_SHA384          0xff63
#define OP_PCL_TLS12_AES_256_CBC_SHA512          0xff65

/* For DTLS - OP_PCLID_DTLS */

#define OP_PCL_DTLS_AES_128_CBC_SHA              0x002f
#define OP_PCL_DTLS_AES_128_CBC_SHA_2            0x0030
#define OP_PCL_DTLS_AES_128_CBC_SHA_3            0x0031
#define OP_PCL_DTLS_AES_128_CBC_SHA_4            0x0032
#define OP_PCL_DTLS_AES_128_CBC_SHA_5            0x0033
#define OP_PCL_DTLS_AES_128_CBC_SHA_6            0x0034
#define OP_PCL_DTLS_AES_128_CBC_SHA_7            0x008c
#define OP_PCL_DTLS_AES_128_CBC_SHA_8            0x0090
#define OP_PCL_DTLS_AES_128_CBC_SHA_9            0x0094
#define OP_PCL_DTLS_AES_128_CBC_SHA_10           0xc004
#define OP_PCL_DTLS_AES_128_CBC_SHA_11           0xc009
#define OP_PCL_DTLS_AES_128_CBC_SHA_12           0xc00e
#define OP_PCL_DTLS_AES_128_CBC_SHA_13           0xc013
#define OP_PCL_DTLS_AES_128_CBC_SHA_14           0xc018
#define OP_PCL_DTLS_AES_128_CBC_SHA_15           0xc01d
#define OP_PCL_DTLS_AES_128_CBC_SHA_16           0xc01e
#define OP_PCL_DTLS_AES_128_CBC_SHA_17           0xc01f

#define OP_PCL_DTLS_AES_256_CBC_SHA              0x0035
#define OP_PCL_DTLS_AES_256_CBC_SHA_2            0x0036
#define OP_PCL_DTLS_AES_256_CBC_SHA_3            0x0037
#define OP_PCL_DTLS_AES_256_CBC_SHA_4            0x0038
#define OP_PCL_DTLS_AES_256_CBC_SHA_5            0x0039
#define OP_PCL_DTLS_AES_256_CBC_SHA_6            0x003a
#define OP_PCL_DTLS_AES_256_CBC_SHA_7            0x008d
#define OP_PCL_DTLS_AES_256_CBC_SHA_8            0x0091
#define OP_PCL_DTLS_AES_256_CBC_SHA_9            0x0095
#define OP_PCL_DTLS_AES_256_CBC_SHA_10           0xc005
#define OP_PCL_DTLS_AES_256_CBC_SHA_11           0xc00a
#define OP_PCL_DTLS_AES_256_CBC_SHA_12           0xc00f
#define OP_PCL_DTLS_AES_256_CBC_SHA_13           0xc014
#define OP_PCL_DTLS_AES_256_CBC_SHA_14           0xc019
#define OP_PCL_DTLS_AES_256_CBC_SHA_15           0xc020
#define OP_PCL_DTLS_AES_256_CBC_SHA_16           0xc021
#define OP_PCL_DTLS_AES_256_CBC_SHA_17           0xc022

/* #define OP_PCL_DTLS_3DES_EDE_CBC_MD5             0x0023 */

#define OP_PCL_DTLS_3DES_EDE_CBC_SHA             0x001f
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_2           0x008b
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_3           0x008f
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_4           0x0093
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_5           0x000a
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_6           0x000d
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_7           0x0010
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_8           0x0013
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_9           0x0016
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_10          0x001b
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_11          0xc003
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_12          0xc008
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_13          0xc00d
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_14          0xc012
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_15          0xc017
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_16          0xc01a
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_17          0xc01b
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA_18          0xc01c

#define OP_PCL_DTLS_DES40_CBC_MD5                0x0029

#define OP_PCL_DTLS_DES_CBC_MD5                  0x0022

#define OP_PCL_DTLS_DES40_CBC_SHA                0x0008
#define OP_PCL_DTLS_DES40_CBC_SHA_2              0x000b
#define OP_PCL_DTLS_DES40_CBC_SHA_3              0x000e
#define OP_PCL_DTLS_DES40_CBC_SHA_4              0x0011
#define OP_PCL_DTLS_DES40_CBC_SHA_5              0x0014
#define OP_PCL_DTLS_DES40_CBC_SHA_6              0x0019
#define OP_PCL_DTLS_DES40_CBC_SHA_7              0x0026


#define OP_PCL_DTLS_DES_CBC_SHA                  0x001e
#define OP_PCL_DTLS_DES_CBC_SHA_2                0x0009
#define OP_PCL_DTLS_DES_CBC_SHA_3                0x000c
#define OP_PCL_DTLS_DES_CBC_SHA_4                0x000f
#define OP_PCL_DTLS_DES_CBC_SHA_5                0x0012
#define OP_PCL_DTLS_DES_CBC_SHA_6                0x0015
#define OP_PCL_DTLS_DES_CBC_SHA_7                0x001a


#define OP_PCL_DTLS_3DES_EDE_CBC_MD5             0xff23
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA160          0xff30
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA224          0xff34
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA256          0xff36
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA384          0xff33
#define OP_PCL_DTLS_3DES_EDE_CBC_SHA512          0xff35
#define OP_PCL_DTLS_AES_128_CBC_SHA160           0xff80
#define OP_PCL_DTLS_AES_128_CBC_SHA224           0xff84
#define OP_PCL_DTLS_AES_128_CBC_SHA256           0xff86
#define OP_PCL_DTLS_AES_128_CBC_SHA384           0xff83
#define OP_PCL_DTLS_AES_128_CBC_SHA512           0xff85
#define OP_PCL_DTLS_AES_192_CBC_SHA160           0xff20
#define OP_PCL_DTLS_AES_192_CBC_SHA224           0xff24
#define OP_PCL_DTLS_AES_192_CBC_SHA256           0xff26
#define OP_PCL_DTLS_AES_192_CBC_SHA384           0xff23
#define OP_PCL_DTLS_AES_192_CBC_SHA512           0xff25
#define OP_PCL_DTLS_AES_256_CBC_SHA160           0xff60
#define OP_PCL_DTLS_AES_256_CBC_SHA224           0xff64
#define OP_PCL_DTLS_AES_256_CBC_SHA256           0xff66
#define OP_PCL_DTLS_AES_256_CBC_SHA384           0xff63
#define OP_PCL_DTLS_AES_256_CBC_SHA512           0xff65

/*
 * SEQ_IN_PTR Command Constructs
 */

/* Release Buffers */
#define SQIN_RBS               0x04000000

/* Sequence pointer is really a descriptor */
#define SQIN_INL               0x02000000

/* Sequence pointer is a scatter-gather table */
#define SQIN_SGF               0x01000000

/* Appends to a previous pointer */
#define SQIN_PRE               0x00800000

/* Use extended length following pointer */
#define SQIN_EXT               0x00400000

/* Restore sequence with pointer/length */
#define SQIN_RTO               0x00200000

/*
 * SEQ_OUT_PTR Command Constructs
 */

/* Sequence pointer is a scatter-gather table */
#define SQOUT_SGF              0x01000000

/* Appends to a previous pointer */
#define SQOUT_PRE              0x00800000

/* Use extended length following pointer */
#define SQOUT_EXT              0x00400000

/*
 * SIGNATURE Command Constructs
 */

/* TYPE field is all that's relevant */
#define SIGN_TYPE_SHIFT         16

#define SIGN_TYPE_FINAL         (0x00 << SIGN_TYPE_SHIFT)
#define SIGN_TYPE_FINAL_RESTORE (0x01 << SIGN_TYPE_SHIFT)
#define SIGN_TYPE_FINAL_NONZERO (0x02 << SIGN_TYPE_SHIFT)
#define SIGN_TYPE_IMM_2         (0x0a << SIGN_TYPE_SHIFT)
#define SIGN_TYPE_IMM_3         (0x0b << SIGN_TYPE_SHIFT)
#define SIGN_TYPE_IMM_4         (0x0c << SIGN_TYPE_SHIFT)

/*
 * MOVE Command Constructs
 */

#define MOVE_AUX_SHIFT          25

#define MOVE_WAITCOMP           0x01000000

#define MOVE_SRC_SHIFT          20
#define MOVE_SRC_CLASS1CTX      (0x00 << MOVE_SRC_SHIFT)
#define MOVE_SRC_CLASS2CTX      (0x01 << MOVE_SRC_SHIFT)
#define MOVE_SRC_OUTFIFO        (0x02 << MOVE_SRC_SHIFT)
#define MOVE_SRC_DESCBUF        (0x03 << MOVE_SRC_SHIFT)
#define MOVE_SRC_MATH0          (0x04 << MOVE_SRC_SHIFT)
#define MOVE_SRC_MATH1          (0x05 << MOVE_SRC_SHIFT)
#define MOVE_SRC_MATH2          (0x06 << MOVE_SRC_SHIFT)
#define MOVE_SRC_MATH3          (0x07 << MOVE_SRC_SHIFT)
#define MOVE_SRC_INFIFO         (0x08 << MOVE_SRC_SHIFT)

#define MOVE_DEST_SHIFT         16
#define MOVE_DEST_CLASS1CTX     (0x00 << MOVE_DEST_SHIFT)
#define MOVE_DEST_CLASS2CTX     (0x01 << MOVE_DEST_SHIFT)
#define MOVE_DEST_OUTFIFO       (0x02 << MOVE_DEST_SHIFT)
#define MOVE_DEST_DESCBUF       (0x03 << MOVE_DEST_SHIFT)
#define MOVE_DEST_MATH0         (0x04 << MOVE_DEST_SHIFT)
#define MOVE_DEST_MATH1         (0x05 << MOVE_DEST_SHIFT)
#define MOVE_DEST_MATH2         (0x06 << MOVE_DEST_SHIFT)
#define MOVE_DEST_MATH3         (0x07 << MOVE_DEST_SHIFT)
#define MOVE_DEST_CLASS1INFIFO  (0x08 << MOVE_DEST_SHIFT)
#define MOVE_DEST_CLASS2INFIFO  (0x09 << MOVE_DEST_SHIFT)
#define MOVE_DEST_PK_A          (0x0c << MOVE_DEST_SHIFT)
#define MOVE_DEST_CLASS1KEY     (0x0d << MOVE_DEST_SHIFT)
#define MOVE_DEST_CLASS2KEY     (0x0e << MOVE_DEST_SHIFT)

#define MOVE_OFFSET_SHIFT       8
#define MOVE_OFFSET_MASK        0xff

#define MOVE_LEN_MASK           0xff

/*
 * MATH Command Constructs
 */

#define MATH_IFB                0x04000000
#define MATH_NFU                0x02000000
#define MATH_STL                0x01000000

/* Function selectors */
#define MATH_FUN_SHIFT          20
#define MATH_FUN_ADD            (0x00 << MATH_FUN_SHIFT)
#define MATH_FUN_ADDC           (0x01 << MATH_FUN_SHIFT)
#define MATH_FUN_SUB            (0x02 << MATH_FUN_SHIFT)
#define MATH_FUN_SUBB           (0x03 << MATH_FUN_SHIFT)
#define MATH_FUN_OR             (0x04 << MATH_FUN_SHIFT)
#define MATH_FUN_AND            (0x05 << MATH_FUN_SHIFT)
#define MATH_FUN_XOR            (0x06 << MATH_FUN_SHIFT)
#define MATH_FUN_LSHIFT         (0x07 << MATH_FUN_SHIFT)
#define MATH_FUN_RSHIFT         (0x08 << MATH_FUN_SHIFT)
#define MATH_FUN_SHLD           (0x09 << MATH_FUN_SHIFT)

/* Source 0 selectors */
#define MATH_SRC0_SHIFT         16
#define MATH_SRC0_REG0          (0x00 << MATH_SRC0_SHIFT)
#define MATH_SRC0_REG1          (0x01 << MATH_SRC0_SHIFT)
#define MATH_SRC0_REG2          (0x02 << MATH_SRC0_SHIFT)
#define MATH_SRC0_REG3          (0x03 << MATH_SRC0_SHIFT)
#define MATH_SRC0_IMM           (0x04 << MATH_SRC0_SHIFT)
#define MATH_SRC0_SEQINLEN      (0x08 << MATH_SRC0_SHIFT)
#define MATH_SRC0_SEQOUTLEN     (0x09 << MATH_SRC0_SHIFT)
#define MATH_SRC0_VARSEQINLEN   (0x0a << MATH_SRC0_SHIFT)
#define MATH_SRC0_VARSEQOUTLEN  (0x0b << MATH_SRC0_SHIFT)
#define MATH_SRC0_ZERO          (0x0c << MATH_SRC0_SHIFT)

/* Source 1 selectors */
#define MATH_SRC1_SHIFT         12
#define MATH_SRC1_REG0          (0x00 << MATH_SRC1_SHIFT)
#define MATH_SRC1_REG1          (0x01 << MATH_SRC1_SHIFT)
#define MATH_SRC1_REG2          (0x02 << MATH_SRC1_SHIFT)
#define MATH_SRC1_REG3          (0x03 << MATH_SRC1_SHIFT)
#define MATH_SRC1_IMM           (0x04 << MATH_SRC1_SHIFT)
#define MATH_SRC1_INFIFO        (0x0a << MATH_SRC1_SHIFT)
#define MATH_SRC1_OUTFIFO       (0x0b << MATH_SRC1_SHIFT)
#define MATH_SRC1_ONE           (0x0c << MATH_SRC1_SHIFT)

/* Destination selectors */
#define MATH_DEST_SHIFT         8
#define MATH_DEST_REG0          (0x00 << MATH_DEST_SHIFT)
#define MATH_DEST_REG1          (0x01 << MATH_DEST_SHIFT)
#define MATH_DEST_REG2          (0x02 << MATH_DEST_SHIFT)
#define MATH_DEST_REG3          (0x03 << MATH_DEST_SHIFT)
#define MATH_DEST_SEQINLEN      (0x08 << MATH_DEST_SHIFT)
#define MATH_DEST_SEQOUTLEN     (0x09 << MATH_DEST_SHIFT)
#define MATH_DEST_VARSEQINLEN   (0x0a << MATH_DEST_SHIFT)
#define MATH_DEST_VARSEQOUTLEN  (0x0b << MATH_DEST_SHIFT)
#define MATH_DEST_NONE          (0x0f << MATH_DEST_SHIFT)

/* Length selectors */
#define MATH_LEN_1BYTE          0x01
#define MATH_LEN_2BYTE          0x02
#define MATH_LEN_4BYTE          0x04
#define MATH_LEN_8BYTE          0x08

/*
 * JUMP Command Constructs
 */

#define JUMP_CLASS_SHIFT        25 /* General class selectors OK */

#define JUMP_JSL                0x01000000

#define JUMP_TYPE_SHIFT         22
#define JUMP_TYPE_LOCAL         (0x00 << JUMP_TYPE_SHIFT)
#define JUMP_TYPE_NONLOCAL      (0x01 << JUMP_TYPE_SHIFT)
#define JUMP_TYPE_HALT          (0x02 << JUMP_TYPE_SHIFT)
#define JUMP_TYPE_HALT_USER     (0x03 << JUMP_TYPE_SHIFT)

#define JUMP_TEST_SHIFT         16
#define JUMP_TEST_ALL           (0x00 << JUMP_TEST_SHIFT)
#define JUMP_TEST_INVALL        (0x01 << JUMP_TEST_SHIFT)
#define JUMP_TEST_ANY           (0x02 << JUMP_TEST_SHIFT)
#define JUMP_TEST_INVANY        (0x03 << JUMP_TEST_SHIFT)

/* If JUMP_JSL clear, these condition codes apply */
#define JUMP_COND_PK_0          0x00008000
#define JUMP_COND_PK_GCD_1      0x00004000
#define JUMP_COND_PK_PRIME      0x00002000
#define JUMP_COND_MATH_N        0x00000800
#define JUMP_COND_MATH_Z        0x00000400
#define JUMP_COND_MATH_C        0x00000200
#define JUMP_COND_MATH_NV       0x00000100

/* If JUMP_JSL set, these condition codes apply */
#define JUMP_COND_JQP           0x00008000
#define JUMP_COND_SHRD          0x00004000
#define JUMP_COND_SELF          0x00002000
#define JUMP_COND_CALM          0x00001000
#define JUMP_COND_NIP           0x00000800
#define JUMP_COND_NIFP          0x00000400
#define JUMP_COND_NOP           0x00000200
#define JUMP_COND_NCP           0x00000100

/*
 * PDB internal definitions
 */

/* IPSec ESP CBC Encap/Decap Options */
#define PDBOPTS_ESPCBC_ARSNONE  0x00   /* no antireplay window              */
#define PDBOPTS_ESPCBC_ARS32    0x40   /* 32-entry antireplay window        */
#define PDBOPTS_ESPCBC_ARS64    0xc0   /* 64-entry antireplay window        */
#define PDBOPTS_ESPCBC_IVSRC    0x20   /* IV comes from internal random gen */
#define PDBOPTS_ESPCBC_ESN      0x10   /* extended sequence included        */
#define PDBOPTS_ESPCBC_OUTFMT   0x08   /* output only decapsulation (decap) */
#define PDBOPTS_ESPCBC_IPHDRSRC 0x08   /* IP header comes from PDB (encap)  */
#define PDBOPTS_ESPCBC_INCIPHDR 0x04   /* Prepend IP header to output frame */
#define PDBOPTS_ESPCBC_IPVSN    0x02   /* process IPv6 header               */
#define PDBOPTS_ESPCBC_TUNNEL   0x01   /* tunnel mode next-header byte      */

#endif /* DESC_H */
