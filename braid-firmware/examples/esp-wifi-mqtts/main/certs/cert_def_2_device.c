
#include "atcacert/atcacert_def.h"

const uint8_t g_cert_template_2_device[] = {
    0x30, 0x82, 0x01, 0x98, 0x30, 0x82, 0x01, 0x3d,  0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0x66,
    0x40, 0x38, 0xf5, 0x8d, 0x6d, 0x77, 0x88, 0xf3,  0xcc, 0xde, 0xe1, 0xda, 0x76, 0x55, 0x2a, 0x30,
    0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d,  0x04, 0x03, 0x02, 0x30, 0x2b, 0x31, 0x0c, 0x30,
    0x0a, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x03,  0x45, 0x53, 0x50, 0x31, 0x1b, 0x30, 0x19, 0x06,
    0x03, 0x55, 0x04, 0x03, 0x0c, 0x12, 0x53, 0x61,  0x6d, 0x70, 0x6c, 0x65, 0x20, 0x53, 0x69, 0x67,
    0x6e, 0x65, 0x72, 0x20, 0x46, 0x46, 0x46, 0x46,  0x30, 0x20, 0x17, 0x0d, 0x32, 0x30, 0x30, 0x34,
    0x32, 0x39, 0x30, 0x35, 0x30, 0x30, 0x30, 0x30,  0x5a, 0x18, 0x0f, 0x33, 0x30, 0x30, 0x30, 0x31,
    0x32, 0x33, 0x31, 0x32, 0x33, 0x35, 0x39, 0x35,  0x39, 0x5a, 0x30, 0x2b, 0x31, 0x0c, 0x30, 0x0a,
    0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x03, 0x45,  0x53, 0x50, 0x31, 0x1b, 0x30, 0x19, 0x06, 0x03,
    0x55, 0x04, 0x03, 0x0c, 0x12, 0x30, 0x31, 0x32,  0x33, 0x32, 0x41, 0x44, 0x39, 0x33, 0x33, 0x37,
    0x35, 0x37, 0x30, 0x30, 0x36, 0x45, 0x45, 0x30,  0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48,
    0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86,  0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42,
    0x00, 0x04, 0xfe, 0x08, 0x1b, 0xa3, 0xf1, 0xce,  0xf2, 0x7a, 0xfe, 0x97, 0xf3, 0xfe, 0x43, 0x31,
    0x8d, 0x3d, 0x94, 0x4d, 0x6c, 0x06, 0xc3, 0x90,  0x90, 0x53, 0xe6, 0x5d, 0x42, 0xdf, 0x5a, 0xdc,
    0x5a, 0x3a, 0xd7, 0x1f, 0x38, 0xee, 0x82, 0x5e,  0x9b, 0xe1, 0x45, 0x77, 0xe4, 0xc9, 0x82, 0x15,
    0x1b, 0xac, 0x32, 0x03, 0x15, 0x85, 0xfd, 0x0a,  0x8b, 0x4e, 0xc7, 0x1f, 0x22, 0xd3, 0xa1, 0x18,
    0x8e, 0x88, 0xa3, 0x41, 0x30, 0x3f, 0x30, 0x0e,  0x06, 0x03, 0x55, 0x1d, 0x0f, 0x01, 0x01, 0xff,
    0x04, 0x04, 0x03, 0x02, 0x05, 0xe0, 0x30, 0x0c,  0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff,
    0x04, 0x02, 0x30, 0x00, 0x30, 0x1f, 0x06, 0x03,  0x55, 0x1d, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80,
    0x14, 0xd0, 0x9f, 0x1c, 0x0e, 0x11, 0xed, 0x7a,  0x5b, 0x0c, 0x7e, 0x98, 0xd2, 0x5f, 0x25, 0xf1,
    0xd2, 0x7a, 0xcb, 0x34, 0x62, 0x30, 0x0a, 0x06,  0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03,
    0x02, 0x03, 0x49, 0x00, 0x30, 0x46, 0x02, 0x21,  0x00, 0xb4, 0x1d, 0x28, 0x52, 0x51, 0x2f, 0xfb,
    0x6d, 0x7e, 0x0e, 0xb5, 0xd8, 0xe7, 0x85, 0x90,  0xc9, 0x48, 0xd6, 0xec, 0x86, 0xdc, 0x7a, 0x01,
    0xff, 0x2b, 0xe4, 0xb4, 0x1f, 0x67, 0x3c, 0x7c,  0x79, 0x02, 0x21, 0x00, 0xce, 0x74, 0x4d, 0x3f,
    0x8c, 0x04, 0x42, 0xea, 0x34, 0x91, 0x8f, 0xa5,  0xf4, 0xeb, 0x54, 0xe8, 0x57, 0x4c, 0x0b, 0x6b,
    0xeb, 0x65, 0x8e, 0x18, 0x61, 0x9b, 0xd2, 0xf5,  0x74, 0xda, 0xef, 0x41
};

const atcacert_def_t g_cert_def_2_device = {
    .type                   = CERTTYPE_X509,
    .template_id            = 2,
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
    .expire_date_format     = DATEFMT_RFC5280_GEN,
    .tbs_cert_loc           = {
        .offset = 4,
        .count  = 321
    },
    .expire_years           = 0,
    .public_key_dev_loc     = {
        .zone      = DEVZONE_DATA,
        .slot      = 0,
        .is_genkey = 1,
        .offset    = 0,
        .count     = 64
    },
    .comp_cert_dev_loc      = {
        .zone      = DEVZONE_DATA,
        .slot      = 10,
        .is_genkey = 0,
        .offset    = 0,
        .count     = 72
    },
    .std_cert_elements      = {
        { // STDCERT_PUBLIC_KEY
            .offset = 194,
            .count  = 64
        },
        { // STDCERT_SIGNATURE
            .offset = 337,
            .count  = 75
        },
        { // STDCERT_ISSUE_DATE
            .offset = 92,
            .count  = 13
        },
        { // STDCERT_EXPIRE_DATE
            .offset = 0,
            .count  = 0
        },
        { // STDCERT_SIGNER_ID
            .offset = 84,
            .count  = 4
        },
        { // STDCERT_CERT_SN
            .offset = 15,
            .count  = 16
        },
        { // STDCERT_AUTH_KEY_ID
            .offset = 305,
            .count  = 20
        },
        { // STDCERT_SUBJ_KEY_ID
            .offset = 0,
            .count  = 0
        }
    },
    .cert_elements          = NULL,
    .cert_elements_count    = 0,
    .cert_template          = g_cert_template_2_device,
    .cert_template_size     = sizeof(g_cert_template_2_device)
};
