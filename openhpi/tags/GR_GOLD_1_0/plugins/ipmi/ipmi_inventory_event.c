/*      -*- linux-c -*-
 *
 * Copyright (c) 2003 by Intel Corp.
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
 */

#include "ipmi.h"
#include <epath_utils.h>
#include <uid_utils.h>
#include <string.h>


static void add_inventory_event_rdr(
                SaHpiRdrT		*rdr,
                SaHpiEntityPathT	parent_ep,
                SaHpiResourceIdT	res_id)
{
		dbg("Adding FRU RDR\n");
        static int inv_count = 0;
        char name[] = "FRU Inventory data";
        
        rdr->RecordId = 0;
        rdr->RdrType = SAHPI_INVENTORY_RDR;
        rdr->Entity = parent_ep;

        rdr->RdrTypeUnion.InventoryRec.EirId = inv_count++;
        rdr->RdrTypeUnion.InventoryRec.Oem   = 0;

        rdr->IdString.DataType = SAHPI_TL_TYPE_ASCII6;
        rdr->IdString.Language = SAHPI_LANG_ENGLISH;
        rdr->IdString.DataLength = sizeof(name);

        memcpy(rdr->IdString.Data, name, sizeof(name));
}

static void add_inventory_event(ipmi_entity_t     *ent,
			      struct oh_handler_state *handler,
			      SaHpiEntityPathT  parent_ep,
			      SaHpiResourceIdT  rid)
{
        struct oh_event *e;

        e = malloc(sizeof(*e));
        if (!e) {
                dbg("Out of memory");
                return;
        }
        memset(e, 0, sizeof(*e));

        e->type = OH_ET_RDR;

        add_inventory_event_rdr(&e->u.rdr_event.rdr, parent_ep, rid);

        rid = oh_uid_lookup(&e->u.rdr_event.rdr.Entity);
        
        oh_add_rdr(handler->rptcache, rid, &e->u.rdr_event.rdr, NULL, 0);
}

/* Per IPMI spec., one FRU per entity */
void ohoi_inventory_event(enum ipmi_update_e    op,
                          ipmi_entity_t         *entity,
                          void                  *cb_data)
{
       struct oh_handler_state  *handler = cb_data;
	   struct ohoi_handler *ipmi_handler = handler->data;

       ipmi_entity_id_t         entity_id;
       SaHpiRptEntryT           *rpt_entry;

       entity_id = ipmi_entity_convert_to_id(entity);
       
       rpt_entry = ohoi_get_resource_by_entityid(
                       handler->rptcache,
                       &entity_id);
       if (!rpt_entry) {
               dump_entity_id("FRU without RPT entry?!", entity_id);
               return;
       }

       if (op == IPMI_ADDED) {
			   dbg("FRU added\n");
               rpt_entry->ResourceCapabilities |= SAHPI_CAPABILITY_INVENTORY_DATA;

               add_inventory_event(entity, handler, 
                               rpt_entry->ResourceEntity,
                               rpt_entry->ResourceId);

			   ipmi_handler->FRU_done = 1;
       }
			   
}
