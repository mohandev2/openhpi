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
#include<oh_domain.h>
#include <oh_utils.h>
#include <string.h>

/* need to update some information here */
void entity_update_rpt(RPTable *table, SaHpiResourceIdT rid, int present)
{
  	struct ohoi_resource_info	*res_info;
       
	res_info = oh_get_resource_data(table, rid);

	if (present) {
		res_info->presence = 1;
		dbg("Resource %d present", rid);
	}else {
	  	dbg("Resource %d not present", rid);
		res_info->presence = 0;
	}
}

static void entity_presence(ipmi_entity_t	*entity,
			    int			present,
			    void		*cb_data,
			    ipmi_event_t	*event)
{
	struct oh_handler_state *handler = (struct oh_handler_state *)cb_data;

	SaHpiRptEntryT  *rpt;
	SaHpiResourceIdT rid;
	ipmi_entity_id_t ent_id;

	ent_id = ipmi_entity_convert_to_id(entity);

	rpt = ohoi_get_resource_by_entityid(handler->rptcache, &ent_id);
	if (!rpt) {
		dbg("No rpt");
		return;
	}
	rid = rpt->ResourceId;
	dbg("%s %s",ipmi_entity_get_entity_id_string(entity), present?"present":"not present");

	entity_update_rpt(handler->rptcache, rid, present);
}

static void get_entity_event(ipmi_entity_t	*entity,
			     SaHpiRptEntryT	*entry, void *cb_data)
{
	SaHpiEntityPathT entity_ep;
	int er;

	struct ohoi_handler *ipmi_handler = cb_data;

	entry->ResourceInfo.ResourceRev = 0;
	entry->ResourceInfo.SpecificVer = 0;
	entry->ResourceInfo.DeviceSupport = 0;
	entry->ResourceInfo.ManufacturerId = 0;
	entry->ResourceInfo.ProductId = 0;
	entry->ResourceInfo.FirmwareMajorRev = 0;
	entry->ResourceInfo.FirmwareMinorRev = 0;
	
	entry->ResourceInfo.AuxFirmwareRev = 0;
	entry->ResourceEntity.Entry[0].EntityType 
                = ipmi_entity_get_entity_id(entity);
	entry->ResourceEntity.Entry[0].EntityLocation 
                = ipmi_entity_get_entity_instance(entity);
	entry->ResourceEntity.Entry[1].EntityType = SAHPI_ENT_ROOT;
	entry->ResourceEntity.Entry[1].EntityLocation = 0;
	    /* AdvancedTCA fixe-up */

        if (ipmi_entity_get_entity_instance(entity) >= 96) {
                        entry->ResourceEntity.Entry[0].EntityLocation -= 96;
                                //|ipmi_entity_get_device_channel(entity)
                                //|ipmi_entity_get_device_address(entity);
        }

        if ((ipmi_entity_get_entity_id(entity) == 160) &&
                        (ipmi_entity_get_entity_instance(entity) == 96)) {
                dbg("SBC_Blade");
               entry->ResourceEntity.Entry[0].EntityType = SAHPI_ENT_SBC_BLADE;
        }

        if ((ipmi_entity_get_entity_id(entity) == 160) &&
                        (ipmi_entity_get_entity_instance(entity) == 102)) {
                dbg("DISK Blade");
               entry->ResourceEntity.Entry[0].EntityType = SAHPI_ENT_DISK_BLADE;
        }

        /* End AdvancedTCA Fix-ups */

	
	/* let's append entity_root from config */

	oh_encode_entitypath(ipmi_handler->entity_root, &entity_ep);

	oh_concat_ep(&entry->ResourceEntity, &entity_ep);

	entry->EntryId = 0;
	entry->ResourceId = oh_uid_from_entity_path(&entry->ResourceEntity);

	entry->ResourceCapabilities = SAHPI_CAPABILITY_RESOURCE;
	
	entry->HotSwapCapabilities = 0;
	if (ipmi_entity_supports_managed_hot_swap(entity)) {
			entry->ResourceCapabilities |= SAHPI_CAPABILITY_MANAGED_HOTSWAP;
			/* if entity supports managed hot swap
			 * check if it has indicator */
			dbg("Entity %d supports managed hotswap\n", entry->ResourceId);

			/* we need only return value from function */
			er = ipmi_entity_get_hot_swap_indicator(entity, NULL, NULL);
			if (!er) {
				dbg("resource %d has HS Indicator\n", entry->ResourceId);
				entry->HotSwapCapabilities |= SAHPI_HS_CAPABILITY_INDICATOR_SUPPORTED;
			}
	}

	/* OpenIPMI used ipmi_entity_hot_swappable to indicate it's FRU
	 * do not use ipmi_entity_get_is_fru()
	 * it's used only for checking if entity has FRU data
	 */
	if(ipmi_entity_hot_swappable(entity)) {
		dbg("Entity supports simplified hotswap");
		entry->ResourceCapabilities |= SAHPI_CAPABILITY_FRU;
	}

	if (ipmi_entity_get_is_fru(entity)) {
		dbg("Entity supports FRU Inventory Data");
		entry->ResourceCapabilities |= SAHPI_CAPABILITY_INVENTORY_DATA;
	}
			
	if (entry->ResourceEntity.Entry[0].EntityType == SAHPI_ENT_SYSTEM_BOARD)
	{	/* This is the BMC entry, so we need to add watchdog. */
		entry->ResourceCapabilities |= SAHPI_CAPABILITY_WATCHDOG;
	}
	entry->ResourceSeverity = SAHPI_OK;
	entry->ResourceTag.DataType = SAHPI_TL_TYPE_TEXT;
	
	entry->ResourceTag.Language = SAHPI_LANG_ENGLISH;

	ipmi_entity_get_id(entity, entry->ResourceTag.Data, SAHPI_MAX_TEXT_BUFFER_LENGTH);
	if ((strlen(entry->ResourceTag.Data) == 0)
	    || (!strcmp(entry->ResourceTag.Data, "invalid"))
	    || ((ipmi_entity_get_entity_id(entity) == 7 || 15))) {

			char *str;
			char *str2;

			str = ipmi_entity_get_entity_id_string(entity);

			if (!(strcmp(str, "invalid"))) {
				dbg("entity_id_string is invlaid, bad SDR..stick to entity_id");
			} else {
			  	/* Let's identify the entity-instance in the ResourceTag */
				  str2 = (char *)calloc(strlen(str) + 1, sizeof(char));
				  snprintf(str2, strlen(str) + 3, "%s-%d",str,
					   	ipmi_entity_get_entity_instance(entity));
				  memcpy(entry->ResourceTag.Data, str2, strlen(str2) + 1);
			}
	}
	entry->ResourceTag.DataLength = (SaHpiUint32T)strlen(entry->ResourceTag.Data);
}

static void add_entity_event(ipmi_entity_t	        *entity,
			     struct oh_handler_state    *handler)
{
		struct ohoi_resource_info *ohoi_res_info;
		SaHpiRptEntryT	entry;

		dbg("adding ipmi entity: %s", ipmi_entity_get_entity_id_string(entity));

		struct ohoi_handler *ipmi_handler = handler->data;

		ohoi_res_info = g_malloc0(sizeof(*ohoi_res_info));

		if (!ohoi_res_info) {
				dbg("Out of memory");
				return;
		}

		ohoi_res_info->type       = OHOI_RESOURCE_ENTITY; 
		ohoi_res_info->u.entity_id= ipmi_entity_convert_to_id(entity);
		ohoi_res_info->presence = 0;

		get_entity_event(entity, &entry, ipmi_handler);	

		oh_add_resource(handler->rptcache, &entry, ohoi_res_info, 1);

		

		
		

		
		
		
		

		
		
		
		

		
		
		
		

		

		

		
                
                
        	

		

		
		
		
		
		

}

void ohoi_remove_entity(struct oh_handler_state *handler,
			SaHpiResourceIdT res_id)
{
  	struct oh_event *e;

  	dbg("Entity (%d) removed", res_id);

	/* Now put an event for the resource to DEL */
	e = g_malloc0(sizeof(*e));
	memset(e, 0, sizeof(*e));

	e->did = oh_get_default_domain_id();
	e->type = OH_ET_RESOURCE_DEL;
	e->u.res_event.entry.ResourceId = res_id;

	handler->eventq = g_slist_append(handler->eventq, e);
}

void ohoi_entity_event(enum ipmi_update_e       op,
                       ipmi_domain_t            *domain,
                       ipmi_entity_t            *entity,
                       void                     *cb_data)
{
  	struct oh_handler_state *handler = cb_data;
	int rv;
		
	switch (op) {
	  	case IPMI_ADDED:
		  	add_entity_event(entity, handler);

			dbg("Entity added: %d.%d", 
					ipmi_entity_get_entity_id(entity), 
					ipmi_entity_get_entity_instance(entity));

			/* entity presence overall */
			rv = ipmi_entity_set_presence_handler(entity,
							      entity_presence,
							      handler);       		
			if (rv) 
				dbg("ipmi_entity_set_presence_handler: %#x", rv);

			/* hotswap handler */
			rv = ipmi_entity_add_hot_swap_handler(entity, ohoi_hot_swap_cb, cb_data);
			if(rv)
			  	dbg("Failed to set entity hot swap handler");

			/* sensors */
			rv= ipmi_entity_set_sensor_update_handler(entity,
								  ohoi_sensor_event,
								  handler);
			if (rv) {
				dbg("ipmi_entity_set_sensor_update_handler: %#x", rv);
				return;
			}
                                                                                
			/* controls */
			rv = ipmi_entity_set_control_update_handler(entity,
								    ohoi_control_event,
								    handler);
                                                                                
			if (rv) {
				dbg("ipmi_entity_set_control_update_handler: %#x", rv);
				return;
			}
                                                                                
			/* inventory (a.k.a FRU) */
			rv = ipmi_entity_set_fru_update_handler(entity,
								ohoi_inventory_event,
								handler);
			if (rv) {
			  	dbg("ipmi_entity_set_fru_update_handler: %#x", rv);
				return;
			}
				                                                                              
			break;
			
		case IPMI_DELETED:
			return;              /* we need to remove_entity */
			dbg("Entity deleted: %d.%d", 
						ipmi_entity_get_entity_id(entity), 
						ipmi_entity_get_entity_instance(entity));
			break;
			
		case IPMI_CHANGED:
			add_entity_event(entity, handler);

			dbg("Entity changed: %d.%d",
						ipmi_entity_get_entity_id(entity), 
						ipmi_entity_get_entity_instance(entity));
			break;
		default:
			dbg("Entity: Unknow change?!");
	}

}
