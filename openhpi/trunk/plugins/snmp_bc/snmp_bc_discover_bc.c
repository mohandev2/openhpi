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
 *      Sean Dague <sdague@users.sf.net>
 *      Steve Sherman <stevees@us.ibm.com>
 *      Chris Chia <cchia@users.sf.net>
 */

#include <glib.h>
#include <snmp_bc_plugin.h>
#include <snmp_bc_utils.h>
#define GET_GUID()                                                             \
        oid_ptr = oh_derive_string(&(e->u.res_event.entry.ResourceEntity),     \
                                   res_info_ptr->mib.OidUuid);                 \
        if (oid_ptr == NULL) {                                                 \
                dbg("Cannot derive oid %s, set GUID to zero", oid_ptr);        \
                memset(&guid, 0, sizeof(SaHpiGuidT));  /*default to zero*/     \
        }                                                                      \
        else {                                                                 \
                err = snmp_bc_get_guid(custom_handle, oid_ptr, &guid);         \
                if ( err == SA_ERR_HPI_BUSY ) {                                \
                        dbg("%d snmp busy, retry get_guid\n", err);            \
                        err = snmp_bc_get_guid(custom_handle, oid_ptr, &guid); \
                }                                                              \
                if (err) {                                                     \
                        dbg("Failed to get GUID, Error=%d\n", err);            \
                        memset(&guid,0,sizeof(SaHpiGuidT)); /*default to zero*/\
                }                                                              \
        }                                                                      \
        memmove(e->u.res_event.entry.ResourceInfo.Guid, guid, sizeof(SaHpiGuidT));\
        g_free(oid_ptr);


SaErrorT snmp_bc_discover(struct oh_handler_state *handle,
			  SaHpiEntityPathT *ep_root)
{

	if (!handle || !ep_root)
		return SA_ERR_HPI_INVALID_PARAMS;
	int i;
	SaErrorT err;
        struct oh_event *e;
	struct snmp_value get_value, get_active;
	struct ResourceInfo *res_info_ptr;
	struct snmp_bc_hnd *custom_handle = (struct snmp_bc_hnd *)handle->data;
        SaHpiGuidT guid;
        char    *oid_ptr;

        /* Discover Chassis, Blades, Expansion Cards */
	err = snmp_bc_snmp_get(custom_handle, SNMP_BC_BLADE_VECTOR, &get_value);
        if (err || get_value.type != ASN_OCTET_STR) {
		dbg("Cannot get OID=%s; Received Type=%d; Error=%s.",
		      SNMP_BC_BLADE_VECTOR, get_value.type, oh_lookup_error(err));
		if (err) { return(err); }
		else { return(SA_ERR_HPI_INTERNAL_ERROR); }
        }
	
	/****************** 
	 * Discover Chassis
	 ******************/
	e = (struct oh_event *)g_malloc0(sizeof(struct oh_event));
	if (e == NULL) {
		dbg("Out of memory.");
		return(SA_ERR_HPI_OUT_OF_SPACE);
	}

	e->type = OH_ET_RESOURCE;
	e->did = oh_get_default_domain_id();

	if (custom_handle->platform == SNMP_BC_PLATFORM_BCT) {
		e->u.res_event.entry = snmp_rpt_array_bct[BCT_RPT_ENTRY_CHASSIS].rpt;
	}
	else {
		e->u.res_event.entry = snmp_rpt_array[BC_RPT_ENTRY_CHASSIS].rpt;	
	}

	e->u.res_event.entry.ResourceEntity = *ep_root;
	e->u.res_event.entry.ResourceId = 
		oh_uid_from_entity_path(&(e->u.res_event.entry.ResourceEntity));
	{ /* Generate Chassis Resource Tag */
		SaHpiTextBufferT build_name;

		oh_init_textbuffer(&build_name);
		oh_append_textbuffer(&build_name, snmp_rpt_array[BC_RPT_ENTRY_CHASSIS].comment);
		if (custom_handle->platform == SNMP_BC_PLATFORM_BC) {
			oh_append_textbuffer(&build_name, " Integrated Chassis");
		}
		else {
			if (custom_handle->platform == SNMP_BC_PLATFORM_BCT) {
				oh_append_textbuffer(&build_name, " Telco Chassis");
			}
			else {
				oh_append_textbuffer(&build_name, " Chassis");
			}
		}
		snmp_bc_create_resourcetag(&(e->u.res_event.entry.ResourceTag),
					   build_name.Data,
					   ep_root->Entry[0].EntityLocation);
	}

	trace("Discovered resource=%s.", e->u.res_event.entry.ResourceTag.Data);

	/* Create platform-specific info space to add to infra-structure */
	res_info_ptr = g_memdup(&(snmp_rpt_array[BC_RPT_ENTRY_CHASSIS].res_info),
				sizeof(struct ResourceInfo));
	res_info_ptr->cur_state = SAHPI_HS_STATE_ACTIVE;

        /* Get BC UUID and convert to GUID */
        GET_GUID();

	/* Add resource to temporary event cache/queue */
	err = oh_add_resource(custom_handle->tmpcache, 
			      &(e->u.res_event.entry), 
			      res_info_ptr, 0);
	if (err) {
		dbg("Failed to add resource. Error=%s.", oh_lookup_error(err));
		g_free(e);
		return(err);
	}
	custom_handle->tmpqueue = g_slist_append(custom_handle->tmpqueue, e);

	/* Find resource's events, sensors, controls, etc. */
	snmp_bc_discover_res_events(handle, &(e->u.res_event.entry.ResourceEntity), res_info_ptr);
	snmp_bc_discover_sensors(handle, snmp_bc_chassis_sensors, e);
 	snmp_bc_discover_controls(handle, snmp_bc_chassis_controls, e);
	snmp_bc_discover_inventories(handle, snmp_bc_chassis_inventories, e);

	/***************** 
	 * Discover Blades
	 *****************/
	for (i=0; i < strlen(get_value.string); i++) {
		if (get_value.string[i] == '1') {

			e = (struct oh_event *)g_malloc0(sizeof(struct oh_event));
			if (e == NULL) {
				dbg("Out of memory.");
				return(SA_ERR_HPI_OUT_OF_SPACE);
			}

			e->type = OH_ET_RESOURCE;
			e->did = oh_get_default_domain_id();
			e->u.res_event.entry = snmp_rpt_array[BC_RPT_ENTRY_BLADE].rpt;
			ep_concat(&(e->u.res_event.entry.ResourceEntity), ep_root);
			set_ep_instance(&(e->u.res_event.entry.ResourceEntity),
					SAHPI_ENT_SBC_BLADE, i + SNMP_BC_HPI_LOCATION_BASE);
			e->u.res_event.entry.ResourceId = 
				oh_uid_from_entity_path(&(e->u.res_event.entry.ResourceEntity));
			snmp_bc_create_resourcetag(&(e->u.res_event.entry.ResourceTag),
						   snmp_rpt_array[BC_RPT_ENTRY_BLADE].comment,
						   i + SNMP_BC_HPI_LOCATION_BASE);

			trace("Discovered resource=%s.", e->u.res_event.entry.ResourceTag.Data);

			/* Create platform-specific info space to add to infra-structure */
			res_info_ptr = g_memdup(&(snmp_rpt_array[BC_RPT_ENTRY_BLADE].res_info),
						sizeof(struct ResourceInfo));
			res_info_ptr->cur_state = SAHPI_HS_STATE_ACTIVE;

                        /* Get BC UUID and convert to GUID */
                        GET_GUID();

			/* Add resource to temporary event cache/queue */
			err = oh_add_resource(custom_handle->tmpcache, 
					      &(e->u.res_event.entry),
					      res_info_ptr, 0);
			if (err) {
				dbg("Failed to add resource. Error=%s.", oh_lookup_error(err));
				g_free(e);
				return(err);
			}
			custom_handle->tmpqueue = g_slist_append(custom_handle->tmpqueue, e);
			
			/* Find resource's events, sensors, controls, etc. */
			snmp_bc_discover_res_events(handle, &(e->u.res_event.entry.ResourceEntity), res_info_ptr);
			snmp_bc_discover_sensors(handle, snmp_bc_blade_sensors, e);
			snmp_bc_discover_controls(handle, snmp_bc_blade_controls, e);
			if (custom_handle->platform == SNMP_BC_PLATFORM_BCT) {
				snmp_bc_discover_controls(handle, snmp_bct_blade_controls, e);
			}
			snmp_bc_discover_inventories(handle, snmp_bc_blade_inventories, e);
			
			/******************************** 
			 * Discover Blade Expansion Cards
			 ********************************/
			{
				SaHpiEntityPathT ep;
                                gchar *oid;

				ep = snmp_rpt_array[BC_RPT_ENTRY_BLADE_ADDIN_CARD].rpt.ResourceEntity;
				ep_concat(&ep, ep_root);
				set_ep_instance(&ep, SAHPI_ENT_ADD_IN_CARD, i + SNMP_BC_HPI_LOCATION_BASE);
				set_ep_instance(&ep, SAHPI_ENT_SBC_BLADE, i + SNMP_BC_HPI_LOCATION_BASE);

				oid = oh_derive_string(&ep, SNMP_BC_BLADE_ADDIN_VECTOR);
				if (oid == NULL) {
					dbg("Cannot derive %s.", SNMP_BC_BLADE_ADDIN_VECTOR);
					return(SA_ERR_HPI_INTERNAL_ERROR);
				}

				err = snmp_bc_snmp_get(custom_handle, oid, &get_value);
				g_free(oid);

				if (!err && get_value.integer != 0) {

					/* Found an expansion card */
					e = (struct oh_event *)g_malloc0(sizeof(struct oh_event));
					if (e == NULL) {
						dbg("Out of memory.");
						return(SA_ERR_HPI_OUT_OF_SPACE);
					}
	
					e->type = OH_ET_RESOURCE;
					e->did = oh_get_default_domain_id();
					e->u.res_event.entry = snmp_rpt_array[BC_RPT_ENTRY_BLADE_ADDIN_CARD].rpt;
					e->u.res_event.entry.ResourceEntity = ep;
					e->u.res_event.entry.ResourceId = oh_uid_from_entity_path(&ep);
					snmp_bc_create_resourcetag(&(e->u.res_event.entry.ResourceTag),
								   snmp_rpt_array[BC_RPT_ENTRY_BLADE_ADDIN_CARD].comment,
								   i + SNMP_BC_HPI_LOCATION_BASE);

					trace("Discovered resource=%s.", e->u.res_event.entry.ResourceTag.Data);

					/* Create platform-specific info space to add to infra-structure */
					res_info_ptr = g_memdup(&(snmp_rpt_array[BC_RPT_ENTRY_BLADE_ADDIN_CARD].res_info),
								sizeof(struct ResourceInfo));
					res_info_ptr->cur_state = SAHPI_HS_STATE_ACTIVE;

                                        /* Get BC UUID and convert to GUID */
                                        GET_GUID();

					/* Add resource to temporary event cache/queue */
					err = oh_add_resource(custom_handle->tmpcache, 
							      &(e->u.res_event.entry),
							      res_info_ptr, 0);
					if (err) {
						dbg("Failed to add resource. Error=%s.", oh_lookup_error(err));
						g_free(e);
						return(err);
					}
					custom_handle->tmpqueue = g_slist_append(custom_handle->tmpqueue, e);
			
					/* Find resource's events, sensors, controls, etc. */
					snmp_bc_discover_res_events(handle, &(e->u.res_event.entry.ResourceEntity), res_info_ptr);
					snmp_bc_discover_sensors(handle, snmp_bc_blade_addin_sensors, e);
					snmp_bc_discover_controls(handle, snmp_bc_blade_addin_controls, e);
					snmp_bc_discover_inventories(handle, snmp_bc_blade_addin_inventories, e);
				}
			}
		}
	}

        /*************** 
	 * Discover Fans
	 ***************/
	err = snmp_bc_snmp_get(custom_handle, SNMP_BC_FAN_VECTOR, &get_value);
        if (err || get_value.type != ASN_OCTET_STR) {
		dbg("Cannot get OID=%s; Received Type=%d; Error=%s.",
		      SNMP_BC_FAN_VECTOR, get_value.type, oh_lookup_error(err));
		if (err) { return(err); }
		else { return(SA_ERR_HPI_INTERNAL_ERROR); }
        }

	for (i=0; i < strlen(get_value.string); i++) {
		if (get_value.string[i] == '1') {
			e = (struct oh_event *)g_malloc0(sizeof(struct oh_event));
			if (e == NULL) {
				dbg("Out of memory.");
				return(SA_ERR_HPI_OUT_OF_SPACE);
			}

			e->type = OH_ET_RESOURCE;
			e->did = oh_get_default_domain_id();
			e->u.res_event.entry = snmp_rpt_array[BC_RPT_ENTRY_BLOWER_MODULE].rpt;
			ep_concat(&(e->u.res_event.entry.ResourceEntity), ep_root);
			set_ep_instance(&(e->u.res_event.entry.ResourceEntity),
					SAHPI_ENT_FAN, i + SNMP_BC_HPI_LOCATION_BASE);
			e->u.res_event.entry.ResourceId = 
				oh_uid_from_entity_path(&(e->u.res_event.entry.ResourceEntity));
			snmp_bc_create_resourcetag(&(e->u.res_event.entry.ResourceTag),
						   snmp_rpt_array[BC_RPT_ENTRY_BLOWER_MODULE].comment,
						   i + SNMP_BC_HPI_LOCATION_BASE);

			trace("Discovered resource=%s.", e->u.res_event.entry.ResourceTag.Data);

			/* Create platform-specific info space to add to infra-structure */
			res_info_ptr = g_memdup(&(snmp_rpt_array[BC_RPT_ENTRY_BLOWER_MODULE].res_info),
						sizeof(struct ResourceInfo));
			res_info_ptr->cur_state = SAHPI_HS_STATE_ACTIVE;

                        /* Get BC UUID and convert to GUID */
                        GET_GUID();

			/* Add resource to temporary event cache/queue */
			err = oh_add_resource(custom_handle->tmpcache, 
					      &(e->u.res_event.entry),
					      res_info_ptr, 0);
			if (err) {
				dbg("Failed to add resource. Error=%s.", oh_lookup_error(err));
				g_free(e);
				return(err);
			}
			custom_handle->tmpqueue = g_slist_append(custom_handle->tmpqueue, e);
			
			/* Find resource's events, sensors, controls, etc. */
			snmp_bc_discover_res_events(handle, &(e->u.res_event.entry.ResourceEntity), res_info_ptr);
			snmp_bc_discover_sensors(handle, snmp_bc_fan_sensors, e);
			snmp_bc_discover_controls(handle, snmp_bc_fan_controls, e);
			snmp_bc_discover_inventories(handle, snmp_bc_fan_inventories, e);
		}
	}

        /************************
	 * Discover Power Modules
	 ************************/
	err = snmp_bc_snmp_get(custom_handle, SNMP_BC_POWER_VECTOR, &get_value);
        if (err || get_value.type != ASN_OCTET_STR) {
		dbg("Cannot get OID=%s; Received Type=%d; Error=%s.",
		      SNMP_BC_POWER_VECTOR, get_value.type, oh_lookup_error(err));
		if (err) { return(err); }
		else { return(SA_ERR_HPI_INTERNAL_ERROR); }
        }

	for (i=0; i < strlen(get_value.string); i++) {
		if (get_value.string[i] == '1') {
			e = (struct oh_event *)g_malloc0(sizeof(struct oh_event));
			if (e == NULL) {
				dbg("Out of memory.");
				return(SA_ERR_HPI_OUT_OF_SPACE);
			}

			e->type = OH_ET_RESOURCE;
			e->did = oh_get_default_domain_id();
			e->u.res_event.entry = snmp_rpt_array[BC_RPT_ENTRY_POWER_MODULE].rpt;
			ep_concat(&(e->u.res_event.entry.ResourceEntity), ep_root);
			set_ep_instance(&(e->u.res_event.entry.ResourceEntity),
					SAHPI_ENT_POWER_SUPPLY, i + SNMP_BC_HPI_LOCATION_BASE);
			e->u.res_event.entry.ResourceId = 
				oh_uid_from_entity_path(&(e->u.res_event.entry.ResourceEntity));
			snmp_bc_create_resourcetag(&(e->u.res_event.entry.ResourceTag),
						   snmp_rpt_array[BC_RPT_ENTRY_POWER_MODULE].comment,
						   i + SNMP_BC_HPI_LOCATION_BASE);

			trace("Discovered resource=%s.", e->u.res_event.entry.ResourceTag.Data);

			/* Create platform-specific info space to add to infra-structure */
			res_info_ptr = g_memdup(&(snmp_rpt_array[BC_RPT_ENTRY_POWER_MODULE].res_info),
						sizeof(struct ResourceInfo));
			res_info_ptr->cur_state = SAHPI_HS_STATE_ACTIVE;

                        /* Get BC UUID and convert to GUID */
                        GET_GUID();

			/* Add resource to temporary event cache/queue */
			err = oh_add_resource(custom_handle->tmpcache, 
					      &(e->u.res_event.entry),
					      res_info_ptr, 0);
			if (err) {
				dbg("Failed to add resource. Error=%s.", oh_lookup_error(err));
				g_free(e);
				return(err);
			}
			custom_handle->tmpqueue = g_slist_append(custom_handle->tmpqueue, e);
			
			/* Find resource's events, sensors, controls, etc. */
			snmp_bc_discover_res_events(handle, &(e->u.res_event.entry.ResourceEntity), res_info_ptr);
			snmp_bc_discover_sensors(handle, snmp_bc_power_sensors, e);
			snmp_bc_discover_controls(handle, snmp_bc_power_controls, e);
			snmp_bc_discover_inventories(handle, snmp_bc_power_inventories, e);
		}
	}

	/******************* 
	 * Discover Switches
	 *******************/
	err = snmp_bc_snmp_get(custom_handle, SNMP_BC_SWITCH_VECTOR, &get_value);
        if (err || get_value.type != ASN_OCTET_STR) {
		dbg("Cannot get OID=%s; Received Type=%d; Error=%s.",
		      SNMP_BC_SWITCH_VECTOR, get_value.type, oh_lookup_error(err));
		if (err) { return(err); }
		else { return(SA_ERR_HPI_INTERNAL_ERROR); }
        }

	for (i=0; i < strlen(get_value.string); i++) {
		if (get_value.string[i] == '1') {
			e = (struct oh_event *)g_malloc0(sizeof(struct oh_event));
			if (e == NULL) {
				dbg("Out of memory.");
				return(SA_ERR_HPI_OUT_OF_SPACE);
			}

			e->type = OH_ET_RESOURCE;
			e->did = oh_get_default_domain_id();
			e->u.res_event.entry = snmp_rpt_array[BC_RPT_ENTRY_SWITCH_MODULE].rpt;
			ep_concat(&(e->u.res_event.entry.ResourceEntity), ep_root);
			set_ep_instance(&(e->u.res_event.entry.ResourceEntity),
					SAHPI_ENT_INTERCONNECT, i + SNMP_BC_HPI_LOCATION_BASE);
			e->u.res_event.entry.ResourceId = 
				oh_uid_from_entity_path(&(e->u.res_event.entry.ResourceEntity));
			snmp_bc_create_resourcetag(&(e->u.res_event.entry.ResourceTag),
						   snmp_rpt_array[BC_RPT_ENTRY_SWITCH_MODULE].comment,
						   i + SNMP_BC_HPI_LOCATION_BASE);

			trace("Discovered resource=%s.", e->u.res_event.entry.ResourceTag.Data);

			/* Create platform-specific info space to add to infra-structure */
			res_info_ptr = g_memdup(&(snmp_rpt_array[BC_RPT_ENTRY_SWITCH_MODULE].res_info),
						sizeof(struct ResourceInfo));
			res_info_ptr->cur_state = SAHPI_HS_STATE_ACTIVE;

                        /* Get BC UUID and convert to GUID */
                        GET_GUID();

			/* Add resource to temporary event cache/queue */
			err = oh_add_resource(custom_handle->tmpcache, 
					      &(e->u.res_event.entry),
					      res_info_ptr, 0);
			if (err) {
				dbg("Failed to add resource. Error=%s.", oh_lookup_error(err));
				g_free(e);
				return(err);
			}
			custom_handle->tmpqueue = g_slist_append(custom_handle->tmpqueue, e);
			
			/* Find resource's events, sensors, controls, etc. */
			snmp_bc_discover_res_events(handle, &(e->u.res_event.entry.ResourceEntity), res_info_ptr);
			snmp_bc_discover_sensors(handle, snmp_bc_switch_sensors, e);
			snmp_bc_discover_controls(handle, snmp_bc_switch_controls, e);
			snmp_bc_discover_inventories(handle, snmp_bc_switch_inventories, e);
		}
	}

	/********************** 
	 * Discover Media Tray
	 *********************/
	err = snmp_bc_snmp_get(custom_handle, SNMP_BC_MEDIATRAY_EXISTS, &get_value);
        if (err || get_value.type != ASN_INTEGER) {
		dbg("Cannot get OID=%s; Received Type=%d; Error=%s.",
		      SNMP_BC_MEDIATRAY_EXISTS, get_value.type, oh_lookup_error(err));
		if (err) { return(err); }
		else { return(SA_ERR_HPI_INTERNAL_ERROR); }
        }

	if (get_value.integer) {
		e = (struct oh_event *)g_malloc0(sizeof(struct oh_event));
		if (e == NULL) {
			dbg("Out of memory.");
			return(SA_ERR_HPI_OUT_OF_SPACE);
		}

		e->type = OH_ET_RESOURCE;
		e->did = oh_get_default_domain_id();
		e->u.res_event.entry = snmp_rpt_array[BC_RPT_ENTRY_MEDIA_TRAY].rpt;
		ep_concat(&(e->u.res_event.entry.ResourceEntity), ep_root);
		set_ep_instance(&(e->u.res_event.entry.ResourceEntity),
				SAHPI_ENT_PERIPHERAL_BAY, i + SNMP_BC_HPI_LOCATION_BASE);
		e->u.res_event.entry.ResourceId = 
			oh_uid_from_entity_path(&(e->u.res_event.entry.ResourceEntity));
		snmp_bc_create_resourcetag(&(e->u.res_event.entry.ResourceTag),
					   snmp_rpt_array[BC_RPT_ENTRY_MEDIA_TRAY].comment,
					   SNMP_BC_HPI_LOCATION_BASE);

		trace("Discovered resource=%s.", e->u.res_event.entry.ResourceTag.Data);

		/* Create platform-specific info space to add to infra-structure */
		res_info_ptr = g_memdup(&(snmp_rpt_array[BC_RPT_ENTRY_MEDIA_TRAY].res_info),
					sizeof(struct ResourceInfo));
		res_info_ptr->cur_state = SAHPI_HS_STATE_ACTIVE;

                /* Get BC UUID and convert to GUID */
                GET_GUID();

		/* Add resource to temporary event cache/queue */
		err = oh_add_resource(custom_handle->tmpcache, 
				      &(e->u.res_event.entry),
				      res_info_ptr, 0);
		if (err) {
			dbg("Failed to add resource. Error=%s.", oh_lookup_error(err));
			g_free(e);
			return(err);
		}
		custom_handle->tmpqueue = g_slist_append(custom_handle->tmpqueue, e);
		
		/* Find resource's events, sensors, controls, etc. */
		snmp_bc_discover_res_events(handle, &(e->u.res_event.entry.ResourceEntity), res_info_ptr);
		snmp_bc_discover_sensors(handle, snmp_bc_mediatray_sensors, e);
		snmp_bc_discover_controls(handle, snmp_bc_mediatray_controls, e);
		snmp_bc_discover_inventories(handle, snmp_bc_mediatray_inventories, e);
	}
	

	/***************************** 
	 * Discover Management Modules
	 *****************************/
	err = snmp_bc_snmp_get(custom_handle, SNMP_BC_MGMNT_VECTOR, &get_value);
        if (err || get_value.type != ASN_OCTET_STR) {
		dbg("Cannot get OID=%s; Received Type=%d; Error=%s.",
		      SNMP_BC_MGMNT_VECTOR, get_value.type, oh_lookup_error(err));
		if (err) { return(err); }
		else { return(SA_ERR_HPI_INTERNAL_ERROR); }
        }

	for (i=0; i < strlen(get_value.string); i++) {
		err = snmp_bc_snmp_get(custom_handle, SNMP_BC_MGMNT_ACTIVE, &get_active);
		if (err || get_active.type != ASN_INTEGER) {
			dbg("Cannot get OID=%s; Received Type=%d; Error=%s.",
			      SNMP_BC_MGMNT_ACTIVE, get_active.type, oh_lookup_error(err));
			if (err) { return(err); }
			else { return(SA_ERR_HPI_INTERNAL_ERROR); }
		}

		/* Find active Management module */
		if (get_active.integer == i+1) {
			e = (struct oh_event *)g_malloc0(sizeof(struct oh_event));
			if (e == NULL) {
				dbg("Out of memory.");
				return(SA_ERR_HPI_OUT_OF_SPACE);
			}

			e->type = OH_ET_RESOURCE;
			e->did = oh_get_default_domain_id();
			e->u.res_event.entry = snmp_rpt_array[BC_RPT_ENTRY_MGMNT_MODULE].rpt;
			ep_concat(&(e->u.res_event.entry.ResourceEntity), ep_root);
			set_ep_instance(&(e->u.res_event.entry.ResourceEntity),
					SAHPI_ENT_SYS_MGMNT_MODULE, i + SNMP_BC_HPI_LOCATION_BASE);
			e->u.res_event.entry.ResourceId = 
				oh_uid_from_entity_path(&(e->u.res_event.entry.ResourceEntity));
			snmp_bc_create_resourcetag(&(e->u.res_event.entry.ResourceTag),
						   snmp_rpt_array[BC_RPT_ENTRY_MGMNT_MODULE].comment,
						   i + SNMP_BC_HPI_LOCATION_BASE);

			trace("Discovered resource=%s.", e->u.res_event.entry.ResourceTag.Data);

			/* Create platform-specific info space to add to infra-structure */
			res_info_ptr = g_memdup(&(snmp_rpt_array[BC_RPT_ENTRY_MGMNT_MODULE].res_info),
						sizeof(struct ResourceInfo));
			res_info_ptr->cur_state = SAHPI_HS_STATE_ACTIVE;

                        /* Get BC UUID and convert to GUID */
                        GET_GUID();

			/* Add resource to temporary event cache/queue */
			err = oh_add_resource(custom_handle->tmpcache, 
					      &(e->u.res_event.entry),
					      res_info_ptr, 0);
			if (err) {
				dbg("Failed to add resource. Error=%s.", oh_lookup_error(err));
				g_free(e);
				return(err);
			}
			custom_handle->tmpqueue = g_slist_append(custom_handle->tmpqueue, e);
			
			/* Find resource's events, sensors, controls, etc. */
			snmp_bc_discover_res_events(handle, &(e->u.res_event.entry.ResourceEntity), res_info_ptr);
			snmp_bc_discover_sensors(handle, snmp_bc_mgmnt_sensors, e);
			snmp_bc_discover_controls(handle, snmp_bc_mgmnt_controls, e);
			snmp_bc_discover_inventories(handle, snmp_bc_mgmnt_inventories, e);
		}
	}

	return(SA_OK);
}
