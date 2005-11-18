/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2005
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *        Renier Morales <renierm@users.sf.net>
 *        Daniel de Araujo <ddearauj@us.ibm.com>
 */
 
#ifndef RTAS_DISCOVER_H
#define RTAS_DISCOVER_H

#include <glib.h>
#include <SaHpi.h>
#include <oh_handler.h>
#include <oh_utils.h>
#include <oh_error.h>
#include <oh_domain.h>
#include <librtas.h>

#define RTAS_SENSORS_PATH	"/proc/device-tree/rtas/rtas-sensors"


SaErrorT rtas_discover_sensors(struct oh_handler_state *handle,
			       struct oh_event *res_oh_event);
			       
#endif /* RTAS_DISCOVER_H */
