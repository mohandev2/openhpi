/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2004
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *      Steve Sherman <stevees@us.ibm.com>
 */

#ifndef __SNMP_BC_EVENT_H
#define __SNMP_BC_EVENT_H

SaErrorT event2hpi_hash_init(struct oh_handler_state *handle);

SaErrorT event2hpi_hash_free(struct oh_handler_state *handle);

SaErrorT snmp_bc_discover_res_events(struct oh_handler_state *handle,
				     SaHpiEntityPathT *ep,
				     const struct ResourceInfo *res_info_ptr);

SaErrorT snmp_bc_discover_sensor_events(struct oh_handler_state *handle,
					SaHpiEntityPathT *ep,
					SaHpiSensorNumT sid,
					const struct snmp_bc_sensor *rpt_sensor);

SaErrorT snmp_bc_log2event(struct oh_handler_state *handle,
			   gchar *logstr,
			   SaHpiEventT *event,
			   int isdst);

SaErrorT snmp_bc_add_to_eventq(struct oh_handler_state *handle,
			       SaHpiEventT *thisEvent);
#endif
