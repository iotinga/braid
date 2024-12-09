
#include "atcacert/atcacert_def.h"

const uint8_t g_csr_template_3_device[] = {
    0x30, 0x81, 0xe6, 0x30, 0x81, 0x8d, 0x02, 0x01,  0x00, 0x30, 0x2b, 0x31, 0x0c, 0x30, 0x0a, 0x06,
    0x03, 0x55, 0x04, 0x0a, 0x0c, 0x03, 0x45, 0x53,  0x50, 0x31, 0x1b, 0x30, 0x19, 0x06, 0x03, 0x55,
    0x04, 0x03, 0x0c, 0x12, 0x53, 0x61, 0x6d, 0x70,  0x6c, 0x65, 0x20, 0x44, 0x65, 0x76, 0x69, 0x63,
    0x65, 0x20, 0x46, 0x46, 0x46, 0x46, 0x30, 0x59,  0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce,
    0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48,  0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00,
    0x04, 0xef, 0x4f, 0xb2, 0x94, 0xbe, 0xa6, 0xda,  0x16, 0x22, 0x3e, 0xfc, 0xcd, 0x4a, 0x2c, 0x2a,
    0xe8, 0xf8, 0xaa, 0x7b, 0x58, 0xe3, 0x6d, 0x7e,  0xd4, 0xfc, 0x9d, 0x8b, 0x4e, 0xc1, 0x57, 0xd3,
    0x3b, 0x74, 0xa6, 0x03, 0xac, 0x60, 0xc0, 0xe6,  0xaa, 0x08, 0x2b, 0x93, 0x91, 0xe4, 0x8a, 0xa1,
    0xdb, 0x84, 0x15, 0xee, 0x25, 0x41, 0xf7, 0x6d,  0xaa, 0xa5, 0x79, 0x7f, 0x94, 0x85, 0x03, 0x6c,
    0xba, 0xa0, 0x00, 0x30, 0x0a, 0x06, 0x08, 0x2a,  0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x03,
    0x48, 0x00, 0x30, 0x45, 0x02, 0x20, 0x31, 0x36,  0x24, 0x67, 0x37, 0xf4, 0xf7, 0x4e, 0xa4, 0x3f,
    0x4b, 0x4e, 0x1d, 0xc7, 0xe9, 0xc7, 0x34, 0x71,  0x8a, 0x31, 0xf9, 0xc6, 0xcf, 0xb2, 0x5d, 0x11,
    0x08, 0xf7, 0x01, 0x5a, 0x5d, 0xc6, 0x02, 0x21,  0x00, 0x99, 0xa4, 0xf3, 0x71, 0x7f, 0x90, 0xd6,
    0x30, 0xa1, 0xb6, 0xdd, 0xcc, 0x2d, 0xc0, 0xe6,  0x13, 0x27, 0xde, 0xca, 0x17, 0xc0, 0x2e, 0x4c,
    0x71, 0x8c, 0x6f, 0xdf, 0x67, 0x02, 0x2f, 0x8b,  0x4a
};

const atcacert_def_t g_csr_def_3_device = {
    .type                   = CERTTYPE_X509,
    .template_id            = 3,
    .chain_id               = 0,
    .private_key_slot       = 0,
    .sn_source              = SNSRC_PUB_KEY_HASH,
    .cert_sn_dev_loc        = { 
        .zone      = DEVZONE_NONE,
        .slot      = 0,
        .is_genkey = 0,
        .offset    = 0,
        .count     = 0
    },
    .issue_date_format      = DATEFMT_RFC5280_UTC,
    .expire_date_format     = DATEFMT_RFC5280_UTC,
    .tbs_cert_loc           = {
        .offset = 3,
        .count  = 144
    },
    .expire_years           = 0,
    .public_key_dev_loc     = {
        .zone      = DEVZONE_NONE,
        .slot      = 0,
        .is_genkey = 1,
        .offset    = 0,
        .count     = 64
    },
    .comp_cert_dev_loc      = {
        .zone      = DEVZONE_NONE,
        .slot      = 0,
        .is_genkey = 0,
        .offset    = 0,
        .count     = 0
    },
    .std_cert_elements      = {
        { // STDCERT_PUBLIC_KEY
            .offset = 81,
            .count  = 64
        },
        { // STDCERT_SIGNATURE
            .offset = 159,
            .count  = 74
        },
        { // STDCERT_ISSUE_DATE
            .offset = 0,
            .count  = 0
        },
        { // STDCERT_EXPIRE_DATE
            .offset = 0,
            .count  = 0
        },
        { // STDCERT_SIGNER_ID
            .offset = 0,
            .count  = 0
        },
        { // STDCERT_CERT_SN
            .offset = 0,
            .count  = 0
        },
        { // STDCERT_AUTH_KEY_ID
            .offset = 0,
            .count  = 0
        },
        { // STDCERT_SUBJ_KEY_ID
            .offset = 0,
            .count  = 0
        }
    },
    .cert_elements          = NULL,
    .cert_elements_count    = 0,
    .cert_template          = g_csr_template_3_device,
    .cert_template_size     = sizeof(g_csr_template_3_device)
};

