/*      -*- linux-c -*-
 *
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
 *	David Judkovics <djudkovi@us.ibm.com>
 */

#include <stdio.h>
#include <time.h>

#include <SaHpi.h>
#include <openhpi.h>
#include <oh_utils.h>
#include <snmp_utils.h>
#include <snmp_client.h>
#include <snmp_client_res.h>
#include <snmp_client_utils.h>

#include <netinet/in.h>

/* prototypes of static functions */
static int rpt_cache_resources_add(RPTable *temp_rptable, 
				   SaHpiRptEntryT *rpt_cache_buff,
				   struct resource_data *remote_res_data,
				   int num_entries);

#if 0
int snmp_get_bulk( void *sessp, 
		   const char *bulk_objid, 
		   struct snmp_pdu *bulk_pdu, 
		   struct snmp_pdu **bulk_response )
{
	size_t anOID_len = MAX_OID_LEN;
	oid anOID[MAX_OID_LEN];
	int status;


	/* Create the PDU for theenrty_count data for our request. */
	read_objid(bulk_objid, anOID, &anOID_len);

	bulk_pdu = snmp_pdu_create(SNMP_MSG_GETBULK);
 	
	bulk_pdu->non_repeaters = 0; 
	
	bulk_pdu->max_repetitions = NUM_REPITIONS;
	
	snmp_add_null_var(bulk_pdu, anOID, anOID_len);
	
	/* Send the Request out.*/
	status = snmp_sess_synch_response(sessp, bulk_pdu, bulk_response);

	return(status);

}
#endif
static int rpt_cache_resources_add( RPTable *temp_rptable,
				    SaHpiRptEntryT *rpt_cache_buff,
				    struct resource_data *remote_res_data,
				    int num_entries)
{	int status = SA_OK;
	int i;

	/* append entity root to resource entity paths */
	for (i=0; i < num_entries; i++) {

		if (!(&remote_res_data[i])) {
			dbg("remote_resdata pointer NULL");
			status = SA_ERR_HPI_ERROR;
			break;
		}
 
		rpt_cache_buff[i].ResourceId = oh_uid_from_entity_path(&rpt_cache_buff[i].ResourceEntity);

		/* add the resource */
		if ( oh_add_resource( temp_rptable, &rpt_cache_buff[i], &remote_res_data[i], 0 ) ) {
			status = SA_ERR_HPI_ERROR;
			dbg("oh_add_resource failed for resource %d", i);
		}
	}

	return(status);
}

int get_sahpi_table_entries( RPTable *temp_rptable, 
			     struct oh_handler_state *handle, 
			     int num_entries )
{
	struct snmp_pdu *pdu = NULL;
	struct snmp_pdu *response = NULL;
	int status = SA_OK;
	int snmp_status;
	struct variable_list *vars;		 

	oid anOID[MAX_OID_LEN];
	oid *indices;

	int i;
	char *ep = NULL;

	SaHpiRptEntryT *rpt_cache = NULL;

	struct resource_data *remote_res_data = NULL;

	struct snmp_client_hnd 
		*custom_handle = (struct snmp_client_hnd *)handle->data;
		    
	/* allocate memory for temp rpt_cache */
	rpt_cache = g_malloc0(num_entries * sizeof(*rpt_cache));
	if (!rpt_cache){
		dbg("ERROR: while allocating memory for rpt_cache");
		return(SA_ERR_HPI_ERROR);
	}

	/* allocate memory for temp remote_res_data */
	remote_res_data = g_malloc0(num_entries * sizeof(*remote_res_data));
	if (!remote_res_data){
		dbg("ERROR: while allocating memory for remote_res_data");
		return(SA_ERR_HPI_ERROR);
	}

	/* allocate memory for temp resources indices storage */
	indices = g_malloc0(NUM_RES_INDICES * sizeof(*indices));
	if (!indices){
		dbg("ERROR: while allocating memory for indices");
		return(SA_ERR_HPI_ERROR);
	}

	/* SA_HPI_DOMAIN_ID */
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     sa_hpi_entry, 
				     SA_HPI_ENTRY_OID_LEN, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;
		
		/* SA_HPI_DOMAIN_ID */
		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_UNSIGNED)	{

				/* save the domain, this may not be the domain for the local machine */
				rpt_cache[i].DomainId = *vars->val.integer;

				/* save the remote resource data for furhter remote queeries */
				remote_res_data[i].remote_domain = vars->name[vars->name_length - 3];
  				remote_res_data[i].remote_resource_id = vars->name[vars->name_length - 2];
				remote_res_data[i].remote_entry_id = vars->name[vars->name_length - 1];

				/* save the index values for this resource */
				/* really just want the last index of the last resource */
				/* INDEX { saHpiDomainID, saHpiResourceID ,saHpiEntryID } */
				indices[0] = vars->name[vars->name_length - 3];
				indices[1] = vars->name[vars->name_length - 2];
				indices[2] = vars->name[vars->name_length - 1];


			} else
				dbg("SA_HPI_DOMAIN_ID:something terrible has happened");

			vars = vars->next_variable;
		} 
		display_vars(response);
 	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);

	sc_free_pdu(&response); 
	
	/* SA_HPI_ENTRY_ID */
	build_res_oid(anOID, sa_hpi_domain_id, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;
		
		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_UNSIGNED)
				rpt_cache[i].EntryId = *vars->val.integer; 
			else
				dbg("SA_HPI_ENTRY_ID:something terrible has happened");				
			vars = vars->next_variable;
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_RESOURCE_ID */
	build_res_oid(anOID, sa_hpi_entry_id, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;
		
		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_UNSIGNED)
				rpt_cache[i].ResourceId = *vars->val.integer; 
			else
				dbg("SA_HPI_RESOURCE_ID:something terrible has happened");
			vars = vars->next_variable;
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response);

	/* SA_HPI_RESOURCE_ENTITY_PATH */
	build_res_oid(anOID, sa_hpi_resource_id, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_OCTET_STR)  {
				ep = g_malloc0(vars->val_len + 1);
				memcpy(ep, vars->val.string, vars->val_len);
				if(oh_encode_entitypath(ep, &rpt_cache[i].ResourceEntity))
					dbg("something terrible happened with SA_HPI_RESOURCE_ENTITY_PATH");
				g_free(ep);				
			}
			else
				dbg("SA_HPI_RESOURCE_ENTITY_PATH:something terrible has happened");			
			vars = vars->next_variable;
		} 
		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_RESOURCE_CAPABILITIES */
	build_res_oid(anOID, sa_hpi_resource_entity_path, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_UNSIGNED)
				rpt_cache[i].ResourceCapabilities = *vars->val.integer; 
			else
				dbg("SA_HPI_RESOURCE_CAPABILITIES:something terrible has happened");
			vars = vars->next_variable;
		
		}
		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_RESOURCE_SEVERITY */
	build_res_oid(anOID, sa_hpi_resource_capabilities, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_INTEGER)
				rpt_cache[i].ResourceSeverity = *vars->val.integer; 
			else
				dbg("SA_HPI_RESOURCE_SEVERITY:something terrible has happened");
			vars = vars->next_variable;
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_RESOURCE_INFO_RESOURCE_REV */
	build_res_oid(anOID, sa_hpi_resource_severity, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_UNSIGNED)
				rpt_cache[i].ResourceInfo.ResourceRev = *vars->val.integer; 
			else
				dbg("SA_HPI_RESOURCE_INFO_RESOURCE_REV:something terrible has happened");
			vars = vars->next_variable;
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_DOMAIN_RESOURCE_INFO_SPECIFIC_VER */
	build_res_oid(anOID, sa_hpi_resource_info_resource_rev, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_UNSIGNED)
				rpt_cache[i].ResourceInfo.SpecificVer = *vars->val.integer; 
			else
				dbg("SA_HPI_DOMAIN_RESOURCE_INFO_SPECIFIC_VER:something terrible has happened");
			vars = vars->next_variable;
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_DOMAIN_RESOURCE_INFO_DEVICE_SUPPORT */
	build_res_oid(anOID, sa_hpi_domain_resource_info_specific_ver, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_UNSIGNED)
				rpt_cache[i].ResourceInfo.DeviceSupport = *vars->val.integer; 
			else
				dbg("SA_HPI_DOMAIN_RESOURCE_INFO_DEVICE_SUPPORT:something terrible has happened");
			vars = vars->next_variable;	
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_DOMAIN_RESOURCE_INFO_MANUFACTUER_ID */
	build_res_oid(anOID, sa_hpi_domain_resource_info_device_support, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_UNSIGNED)
				rpt_cache[i].ResourceInfo.ManufacturerId = *vars->val.integer; 
			else
				dbg("SA_HPI_DOMAIN_RESOURCE_INFO_MANUFACTUER_ID:something terrible has happened");
			vars = vars->next_variable;
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_DOMAIN_RESOURCE_INFO_PRODUCT_ID */
	build_res_oid(anOID, sa_hpi_domain_resource_info_manufactuer_id, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_UNSIGNED)
				rpt_cache[i].ResourceInfo.ProductId = *vars->val.integer; 
			else
				dbg("SA_HPI_DOMAIN_RESOURCE_INFO_PRODUCT_ID:something terrible has happened");
			vars = vars->next_variable;
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_DOMAIN_RESOURCE_INFO_FIRMWARE_MAJOR_REV */
	build_res_oid(anOID, sa_hpi_domain_resource_info_product_id, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_UNSIGNED)
				rpt_cache[i].ResourceInfo.FirmwareMajorRev = *vars->val.integer; 
			else
				dbg("SA_HPI_DOMAIN_RESOURCE_INFO_FIRMWARE_MAJOR_REV:something terrible has happened");
			vars = vars->next_variable;
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_DOMAIN_RESOURCE_INFO_FIRMWARE_MINOR_REV */
	build_res_oid(anOID, sa_hpi_domain_resource_info_firmware_major_rev, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_UNSIGNED)
				rpt_cache[i].ResourceInfo.FirmwareMinorRev = *vars->val.integer; 
			else
				dbg("SA_HPI_DOMAIN_RESOURCE_INFO_FIRMWARE_MINOR_REV:something terrible has happened");
			vars = vars->next_variable;
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_DOMAIN_RESOURCE_INFO_AUX_FIRMWARE_REV */
	build_res_oid(anOID, sa_hpi_domain_resource_info_firmware_minor_rev, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_UNSIGNED)
				rpt_cache[i].ResourceInfo.AuxFirmwareRev = *vars->val.integer; 
			else
				dbg("SA_HPI_DOMAIN_RESOURCE_INFO_AUX_FIRMWARE_REV:something terrible has happened");
			vars = vars->next_variable;
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_DOMAIN_RESOURCE_TAG_TEXT_TYPE */
	build_res_oid(anOID, sa_hpi_domain_resource_info_aux_firmware_rev, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_INTEGER)
				rpt_cache[i].ResourceTag.DataType = *vars->val.integer; 
			else
				dbg("SA_HPI_DOMAIN_RESOURCE_TAG_TEXT_TYPE:something terrible has happened");
			vars = vars->next_variable;  
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_DOMAIN_RESOURCE_TAG_TEXT_LANGUAGE */
	build_res_oid(anOID, sa_hpi_domain_resource_tag_text_type, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_INTEGER)
				rpt_cache[i].ResourceTag.Language = *vars->val.integer; 
			else
				dbg("SA_HPI_DOMAIN_RESOURCE_TAG_TEXT_LANGUAGE:something terrible has happened");
			vars = vars->next_variable;
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 

	/* SA_HPI_DOMAIN_RESOURCE_TAG */
	build_res_oid(anOID, sa_hpi_domain_resource_tag_text_language, SA_HPI_ENTRY_TABLE_VARIABLE_OID_LENGTH, indices, NUM_RES_INDICES); 
	snmp_status = snmp_getn_bulk(custom_handle->sessp, 
				     anOID, 
				     SA_HPI_ENTRY_TABLE_VARIABLE_FULL_OID_LENGTH, 
				     pdu, 
				     &response, 
				     num_entries);
	if (status == SA_OK && snmp_status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		/* Get the data from the response */
		vars = response->variables;

		for (i = 0; i < num_entries; i++) {
			if (vars->type == ASN_OCTET_STR) {
				memcpy(&rpt_cache[i].ResourceTag.Data, vars->val.string, vars->val_len);
				rpt_cache[i].ResourceTag.DataLength = vars->val_len;
			}
			else
				dbg("SA_HPI_DOMAIN_RESOURCE_TAG:something terrible has happened");
			vars = vars->next_variable;
		}

		display_vars(response);
	} else
		status = net_snmp_failure(custom_handle, snmp_status, response);
	sc_free_pdu(&response); 
       
	if(status == SA_OK) {
		/* add the resources to the plugin's primary rptcache, */
		/* the one maintained in oh_handler_state pointer */
		dbg("********************************************************");
		dbg("** Adding rpt_cache and remote_res_data to temp_table **");
		dbg("********************************************************");
		status = rpt_cache_resources_add(temp_rptable, rpt_cache, remote_res_data, num_entries);
	} else {
		/*free the temporary rptcahce buffer used to capture the */
		/* snmp remote resource data */
		if( remote_res_data )
			g_free( remote_res_data );
	}

	/*free the temporary rptcahce buffer used to capture the */
	/* snmp resource data */
	if( rpt_cache )
		 g_free( rpt_cache );

	return(status);
}

