/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2004-2006
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *      Renier Morales <renierm@users.sf.net>
 *
 */

#ifndef __OH_ERROR_H
#define __OH_ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <config.h>

/* this is put here intentionally as there are too many instances
 * of unqualified sprintf calls in plugin code. Use snprintf instead
 * to ensure there are no buffer overruns 
 */
#undef sprintf
#pragma GCC poison sprintf

#ifdef __cplusplus
extern "C" {
#endif

#ifdef OH_DBG_MSGS
#ifndef OH_DAEMON_ENABLED
#define dbg(format, ...) \
        do { \
                if (getenv("OPENHPI_DEBUG") && !strcmp("YES",getenv("OPENHPI_DEBUG"))) { \
                        fprintf(stderr, " %s:%d:%s: ", __FILE__, __LINE__, __func__); \
                        fprintf(stderr, format "\n", ## __VA_ARGS__); \
                } \
        } while(0)
#else
#define dbg(format, ...) \
	do { \
		syslog(3, "DEBUG (%s:%d:%s) > "format, __FILE__, __LINE__, __func__,## __VA_ARGS__); \
		fprintf(stderr, format"\n",## __VA_ARGS__); \
	} while(0)
#endif
#else
#define dbg(format, ...)
#endif

#ifdef OH_DBG_MSGS
#ifndef OH_DAEMON_ENABLED
#define trace(format, ...) \
        do { \
                if (getenv("OPENHPI_DEBUG_TRACE") && !strcmp("YES",getenv("OPENHPI_DEBUG_TRACE"))) { \
                        fprintf(stderr, " %s:%d:%s: ", __FILE__, __LINE__, __func__); \
                        fprintf(stderr, format "\n", ## __VA_ARGS__); \
                } \
        } while(0)
#else
#define trace(format, ...) \
	do { \
		syslog(5, "INFO (%s:%d:%s) > "format, __FILE__, __LINE__, __func__,## __VA_ARGS__); \
		fprintf(stderr, format"\n",## __VA_ARGS__); \
	} while(0)
#endif
#else
#define trace(format, ...)
#endif

#ifdef __cplusplus
}
#endif
        
#endif /* __OH_ERROR_H */

