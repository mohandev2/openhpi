/*      -*- linux-c -*-
 *
 * Copyright (c) 2003 by Intel Corp.
 * (C) Copyright IBM Corp. 2003
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Authors:
 *     Louis Zhuang <louis.zhuang@linux.intel.com>
 *     Sean Dague <http://dague.net/sean>
 *     Rusty Lynch
 *     Renier Morales <renierm@users.sf.net>
 */

#ifndef __OPENHPI_H
#define __OPENHPI_H

#include <SaHpi.h>
#include <glib.h>
#include <oh_config.h>
#include <oh_plugin.h>
#include <oh_init.h>
#include <oh_lock.h>
#include <oh_error.h>
#include <oh_domain.h>
#include <oh_session.h>
#include <oh_alarm.h>
#include <oh_hotswap.h>
#include <oh_utils.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Representation of additional resource data
 * stored in the data field rpt_utils for each
 * resource
 */

struct oh_resource_data
{
        /*
           The handler id of the resource
        */
        unsigned int hid;

        /*
         * The two fields are valid when resource is
         * CAPABILITY_HOTSWAP
         */

        int                controlled;
        SaHpiTimeoutT      auto_extract_timeout;
};

#define oh_session_event oh_hpi_event

#ifdef __cplusplus
}
#endif

#endif /* __OPENHPI_H */
