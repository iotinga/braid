#pragma once

/*
 * This file contains configurations defaults for set in the build phase.
 */

#ifndef CFG_FW_VERSION_MAJOR
#warning "fw version major not defined"
#define CFG_FW_VERSION_MAJOR 0
#endif

#ifndef CFG_FW_VERSION_MINOR
#warning "fw version minor not defined"
#define CFG_FW_VERSION_MINOR 0
#endif

#ifndef CFG_FW_VERSION_PATCH
#warning "fw version patch not defined"
#define CFG_FW_VERSION_PATCH 0
#endif

#ifndef CFG_HARDWARE_VERSION
#warning "hardware version not defined"
#define CFG_HARDWARE_VERSION 1
#endif

#ifndef CFG_HARDWARE_MODEL
#warning "hardware model not defined"
#define CFG_HARDWARE_MODEL 1
#endif

#ifndef CFG_LOG_LEVEL
#warning "log level not defined"
#define CFG_LOG_LEVEL 7
#endif

#ifndef CFG_LOG_USE_COLORS
#warning "cli use color not defined"
#define CFG_LOG_USE_COLORS 1
#endif

#define STR(x)  #x
#define XSTR(x) STR(x)
#define VERSION_STR                                                                                                    \
    ("$$FW_VERSION=v" XSTR(CFG_FW_VERSION_MAJOR) "." XSTR(CFG_FW_VERSION_MINOR) "." XSTR(                              \
        CFG_FW_VERSION_PATCH) "-" XSTR(CFG_FW_VERSION_COMMIT) "$$")