/*      -*- linux-c -*-
 *
 * Copyright (c) 2003 by Intel Corp.
 * (C) Copyright IBM Corp. 2003-2004
 * Copyright (c) 2004 by FORCE Computers.
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
 *     Sean Dague <sdague@users.sf.net>
 *     Rusty Lynch
 *     David Judkovics <djudkovi@us.ibm.com>
 *     Thomas Kanngieser <thomas.kanngieser@fci.com>
 *     Renier Morales <renierm@users.sf.net>
 *     Racing Guo <racing.guo@intel.com>
 */

#include <string.h>
#include <SaHpi.h>
#include <openhpi.h>
#include <sahpimacros.h>

/*********************************************************************
 *
 * Begin SAHPI B.1.1 Functions. For full documentation please see
 * the specification
 *
 ********************************************************************/

SaHpiVersionT SAHPI_API saHpiVersionGet ()
{
        return SAHPI_INTERFACE_VERSION;
}

SaErrorT SAHPI_API saHpiSessionOpen(
        SAHPI_IN  SaHpiDomainIdT  DomainId,
        SAHPI_OUT SaHpiSessionIdT *SessionId,
        SAHPI_IN  void            *SecurityParams)
{
        SaHpiSessionIdT sid;
        SaHpiDomainIdT did;
        
        if (SessionId == NULL) {
                dbg("Invalid Session Id pointer");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        /* Security Params required to be NULL by the spec at this point */
        if (SecurityParams != NULL) {
                dbg("SecurityParams must be NULL");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        if (oh_initialized() != SA_OK && oh_initialize() != SA_OK) {
                dbg("ERROR. Could not initialize the library");
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        if (DomainId == SAHPI_UNSPECIFIED_DOMAIN_ID)
                did = oh_get_default_domain_id();
        else
                did = DomainId;

        sid = oh_create_session(did);
        if(!sid) {
                dbg("Domain %d does not exist or unable to create session!", did);
                return SA_ERR_HPI_INVALID_DOMAIN;
        }

        *SessionId = sid;

        return SA_OK;
}

SaErrorT SAHPI_API saHpiSessionClose(
        SAHPI_IN SaHpiSessionIdT SessionId)
{
        OH_CHECK_INIT_STATE(SessionId);

        return oh_destroy_session(SessionId);
}

SaErrorT SAHPI_API saHpiDiscover(
        SAHPI_IN SaHpiSessionIdT SessionId)
{
        SaHpiDomainIdT did;
        int rv = SA_ERR_HPI_ERROR;
        unsigned int hid = 0, next_hid;
        struct oh_handler *h = NULL;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        data_access_lock();
        oh_lookup_next_handler(hid, &next_hid);
        while (next_hid) {
                hid = next_hid;
                h = oh_lookup_handler(hid);
                if (h->abi->discover_resources(h->hnd) == SA_OK && rv)
                        rv = SA_OK;

                oh_lookup_next_handler(hid, &next_hid);
        }
        data_access_unlock();

        if (rv) {
                dbg("Error attempting to discover resources in Domain %d",did);
                return SA_ERR_HPI_UNKNOWN;
        }

        rv = oh_get_events();

        if (rv < 0) {
                dbg("Error attempting to process resources in Domain %d",did);
                return SA_ERR_HPI_UNKNOWN;
        }

        return SA_OK;
}


/*********************************************************************
 *
 *  Domain Functions
 *
 ********************************************************************/

SaErrorT SAHPI_API saHpiDomainInfoGet (
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_OUT SaHpiDomainInfoT *DomainInfo)
{
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!DomainInfo) return SA_ERR_HPI_INVALID_PARAMS;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        OH_GET_DOMAIN(did, d); /* Lock domain */
        DomainInfo->DomainId = d->id;
        DomainInfo->DomainCapabilities = d->capabilities;
        DomainInfo->IsPeer = d->is_peer;
        DomainInfo->RptUpdateCount = d->rpt.update_count;
        DomainInfo->RptUpdateTimestamp = d->rpt.update_timestamp;
        memcpy(DomainInfo->Guid, d->guid, sizeof(SaHpiGuidT));
        DomainInfo->DomainTag = d->tag;
        oh_release_domain(d); /* Unlock domain */

        return SA_OK;
}

/***********************************************************
 *
 *  For OpenHPI 2.0 this is always going to return
 *  NOT_PRESENT unless it is called invalidly.  True
 *  DRT support won't be there until 2.2.
 *
 **********************************************************/

SaErrorT SAHPI_API saHpiDrtEntryGet (
        SAHPI_IN  SaHpiSessionIdT     SessionId,
        SAHPI_IN  SaHpiEntryIdT       EntryId,
        SAHPI_OUT SaHpiEntryIdT       *NextEntryId,
        SAHPI_OUT SaHpiDrtEntryT      *DrtEntry)
{
        SaHpiDomainIdT did;
        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        
        if((DrtEntry == NULL) || 
           (NextEntryId == NULL) || 
           (EntryId == SAHPI_LAST_ENTRY)) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        return SA_ERR_HPI_NOT_PRESENT;
}

SaErrorT SAHPI_API saHpiDomainTagSet (
        SAHPI_IN  SaHpiSessionIdT      SessionId,
        SAHPI_IN  SaHpiTextBufferT     *DomainTag)
{
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!DomainTag || !oh_valid_textbuffer(DomainTag))
                return SA_ERR_HPI_INVALID_PARAMS;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        OH_GET_DOMAIN(did, d); /* Lock domain */
        d->tag = *DomainTag;
        oh_release_domain(d); /* Unlock domain */

        return SA_OK;
}

/*********************************************************************
 *
 *  Resource Functions
 *
 ********************************************************************/

SaErrorT SAHPI_API saHpiRptEntryGet(
        SAHPI_IN  SaHpiSessionIdT SessionId,
        SAHPI_IN  SaHpiEntryIdT   EntryId,
        SAHPI_OUT SaHpiEntryIdT   *NextEntryId,
        SAHPI_OUT SaHpiRptEntryT  *RptEntry)
{
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        SaHpiRptEntryT *req_entry;
        SaHpiRptEntryT *next_entry;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        /* Test pointer parameters for invalid pointers */
        if ((NextEntryId == NULL) || (RptEntry == NULL)) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        /* I believe this is the only current reserved value
           here, though others may come in the future. */
        if (EntryId == SAHPI_LAST_ENTRY) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_GET_DOMAIN(did, d); /* Lock domain */

        if (EntryId == SAHPI_FIRST_ENTRY) {
                req_entry = oh_get_resource_next(&(d->rpt), SAHPI_FIRST_ENTRY);
        } else {
                req_entry = oh_get_resource_by_id(&(d->rpt), EntryId);
        }

        /* if the entry was NULL, clearly have an issue */
        if (req_entry == NULL) {
                dbg("Invalid EntryId %d in Domain %d", EntryId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_NOT_PRESENT;
        }

        memcpy(RptEntry, req_entry, sizeof(*RptEntry));

        next_entry = oh_get_resource_next(&(d->rpt), req_entry->EntryId);

        if(next_entry != NULL) {
                *NextEntryId = next_entry->EntryId;
        } else {
                *NextEntryId = SAHPI_LAST_ENTRY;
        }

        oh_release_domain(d); /* Unlock domain */

        return SA_OK;
}

SaErrorT SAHPI_API saHpiRptEntryGetByResourceId(
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_IN  SaHpiResourceIdT ResourceId,
        SAHPI_OUT SaHpiRptEntryT   *RptEntry)
{
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        SaHpiRptEntryT *req_entry;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        /* Test pointer parameters for invalid pointers */
        if (ResourceId == SAHPI_UNSPECIFIED_RESOURCE_ID ||
            RptEntry == NULL)
        {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_GET_DOMAIN(did, d); /* Lock domain */

        req_entry = oh_get_resource_by_id(&(d->rpt), ResourceId);

        /*
         * is this case really supposed to be an error?  I thought
         * there was a valid return for "not found in domain"
         */

        if (req_entry == NULL) {
                dbg("No such Resource Id %d in Domain %d", ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_INVALID_CMD;
        }

        memcpy(RptEntry, req_entry, sizeof(*RptEntry));

        oh_release_domain(d); /* Unlock domain */

        return SA_OK;
}

SaErrorT SAHPI_API saHpiResourceSeveritySet(
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiSeverityT   Severity)
{
        /* this requires a new abi call to push down to the plugin */
        int (*set_res_sev)(void *hnd, SaHpiResourceIdT id,
                             SaHpiSeverityT sev);

        SaHpiDomainIdT did;
        struct oh_handler *h = NULL;
        struct oh_domain *d = NULL;
        SaErrorT error = SA_OK;

        OH_CHECK_INIT_STATE(SessionId);

        if (ResourceId == SAHPI_UNSPECIFIED_RESOURCE_ID) {
                dbg("Invalid resource id, SAHPI_UNSPECIFIED_RESOURCE_ID passed.");
                return SA_ERR_HPI_INVALID_PARAMS;
        } else if (!oh_lookup_severity(Severity)) {
                dbg("Invalid severity %d passed.", Severity);
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_res_sev = h->abi->set_resource_severity;

        if (!set_res_sev) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        if ((error = set_res_sev(h->hnd, ResourceId, Severity)) != SA_OK) {
                dbg("Setting severity failed for ResourceId %d in Domain %d",
                    ResourceId, did);
                return error;
        }
        
        /* Alarm Handling */
        if (error == SA_OK) {                
                oh_detect_res_sev_alarm(did, ResourceId, Severity);                
        }

        /* to get rpt entry into infrastructure */
        oh_get_events();        

        return error;
}

SaErrorT SAHPI_API saHpiResourceTagSet(
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiTextBufferT *ResourceTag)
{
        SaErrorT rv;
        SaErrorT (*set_res_tag)(void *hnd, SaHpiResourceIdT id,
                                SaHpiTextBufferT *ResourceTag);

        SaHpiDomainIdT did;
        struct oh_handler *h = NULL;
        struct oh_domain *d = NULL;
        SaHpiRptEntryT *rptentry;
        
        if (ResourceTag == NULL)
                return SA_ERR_HPI_INVALID_PARAMS;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        
        if (!ResourceTag || !oh_valid_textbuffer(ResourceTag))
                return SA_ERR_HPI_INVALID_PARAMS;

        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_res_tag = h->abi->set_resource_tag;

        if (!set_res_tag) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_res_tag(h->hnd, ResourceId, ResourceTag);
        if (rv != SA_OK) {
                dbg("Tag set failed for Resource %d in Domain %d",
                    ResourceId, did);
                return rv;
        }

        OH_GET_DOMAIN(did, d); /* Lock domain */
        rptentry = oh_get_resource_by_id(&(d->rpt), ResourceId);
        if (!rptentry) {
                dbg("Tag set failed: No Resource %d in Domain %d",
                    ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_NOT_PRESENT;
        }
        rptentry->ResourceTag = *ResourceTag;
        oh_release_domain(d); /* Unlock domain */

        return SA_OK;
}

SaErrorT SAHPI_API saHpiResourceIdGet(
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_OUT SaHpiResourceIdT *ResourceId)
{
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        SaHpiRptEntryT *rptentry;
        struct oh_global_param ep_param = { .type = OPENHPI_ON_EP };

        if (ResourceId == NULL)
                return SA_ERR_HPI_INVALID_PARAMS;
        
        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        oh_get_global_param(&ep_param);
                
        OH_GET_DOMAIN(did, d); /* Lock domain */
        rptentry = oh_get_resource_by_ep(&(d->rpt), &ep_param.u.on_ep);
        if (!rptentry) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_UNKNOWN;
        }

        *ResourceId = rptentry->ResourceId;
        oh_release_domain(d); /* Unlock domain */

        return SA_OK;
}

/*********************************************************************
 *
 *  Event Log Functions
 *
 ********************************************************************/

SaErrorT SAHPI_API saHpiEventLogInfoGet (
        SAHPI_IN  SaHpiSessionIdT    SessionId,
        SAHPI_IN  SaHpiResourceIdT   ResourceId,
        SAHPI_OUT SaHpiEventLogInfoT *Info)
{
        SaErrorT rv;
        SaErrorT (*get_func) (void *, SaHpiResourceIdT, SaHpiEventLogInfoT *);
        SaHpiRptEntryT *res;
        struct oh_handler *h = NULL;
        struct oh_domain *d = NULL;
        SaHpiDomainIdT did;

        /* Test pointer parameters for invalid pointers */
        if (Info == NULL)
        {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */

        /* test for special domain case */
        if (ResourceId == SAHPI_UNSPECIFIED_RESOURCE_ID) {
                rv = oh_el_info(d->del, Info);
                oh_release_domain(d); /* Unlock domain */
                return rv;
        }

        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_EVENT_LOG)) {
                dbg("Resource %d in Domain %d does not have EL",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        get_func = h->abi->get_el_info;

        if (!get_func) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_func(h->hnd, ResourceId, Info);

        if (rv != SA_OK) {
                dbg("EL info get failed");
        }

        return rv;
}

SaErrorT SAHPI_API saHpiEventLogEntryGet (
        SAHPI_IN    SaHpiSessionIdT       SessionId,
        SAHPI_IN    SaHpiResourceIdT      ResourceId,
        SAHPI_IN    SaHpiEventLogEntryIdT EntryId,
        SAHPI_OUT   SaHpiEventLogEntryIdT *PrevEntryId,
        SAHPI_OUT   SaHpiEventLogEntryIdT *NextEntryId,
        SAHPI_OUT   SaHpiEventLogEntryT   *EventLogEntry,
        SAHPI_INOUT SaHpiRdrT             *Rdr,
        SAHPI_INOUT SaHpiRptEntryT        *RptEntry)
{
        SaErrorT rv;
        SaErrorT (*get_el_entry)(void *hnd, SaHpiResourceIdT id,
				SaHpiEventLogEntryIdT current,
				SaHpiEventLogEntryIdT *prev,
				SaHpiEventLogEntryIdT *next,
				SaHpiEventLogEntryT *entry,
				SaHpiRdrT  *rdr,
				SaHpiRptEntryT  *rptentry);
				
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        struct oh_domain *d;
        oh_el_entry *elentry;
        SaErrorT retc;
        SaHpiDomainIdT did;

        /* Test pointer parameters for invalid pointers */
        if (!PrevEntryId || !EventLogEntry || !NextEntryId ||
            EntryId == SAHPI_NO_MORE_ENTRIES)
        {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */

        /* test for special domain case */
        if (ResourceId == SAHPI_UNSPECIFIED_RESOURCE_ID) {
                retc = oh_el_get(d->del, EntryId, PrevEntryId, NextEntryId,
                                 &elentry);
                if (retc == SA_OK) {
                        memcpy(EventLogEntry, &elentry->event, sizeof(SaHpiEventLogEntryT));
			if (Rdr)
				memcpy(Rdr, &elentry->rdr, sizeof(SaHpiRdrT)); 	
			if (RptEntry)
				memcpy(RptEntry, &elentry->res, sizeof(SaHpiRptEntryT));
		}
                oh_release_domain(d); /* Unlock domain */
                return retc;
        }

        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_EVENT_LOG)) {
                dbg("Resource %d in Domain %d does not have EL",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
	oh_release_domain(d); /* Unlock domain */
	
        get_el_entry = h->abi->get_el_entry;

        if (!get_el_entry) {
                dbg("This api is not supported");
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_el_entry(h->hnd, ResourceId,
				EntryId, PrevEntryId,
				NextEntryId, EventLogEntry,
				Rdr, RptEntry);

        if(rv != SA_OK)
                dbg("EL entry get failed\n");

        return rv;
}

SaErrorT SAHPI_API saHpiEventLogEntryAdd (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiEventT      *EvtEntry)
{
        SaErrorT rv;
        SaErrorT (*add_el_entry)(void *hnd, SaHpiResourceIdT id,
                                  const SaHpiEventT *Event);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        struct oh_domain *d;
        SaHpiDomainIdT did;

        OH_CHECK_INIT_STATE(SessionId);
        
        if (EvtEntry == NULL) {
                dbg("Error: Event Log Entry is NULL");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        if (EvtEntry->EventType != SAHPI_ET_USER ||
            EvtEntry->Source != SAHPI_UNSPECIFIED_RESOURCE_ID) {
                dbg("Error: Event Log Entry is not USER or has invalid Source");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        if (!oh_lookup_severity(EvtEntry->Severity)) {
                dbg("Error: Event Log Entry Severity %s is invalid",
                    oh_lookup_severity(EvtEntry->Severity));
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        if(!oh_valid_textbuffer(&(EvtEntry->EventDataUnion.UserEvent.UserEventData))) {
                dbg("Error: Event Log UserData is invalid");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */

        /* test for special domain case */
        if (ResourceId == SAHPI_UNSPECIFIED_RESOURCE_ID) {
                rv = oh_el_append(d->del, EvtEntry, NULL, NULL);
                oh_release_domain(d); /* Unlock domain */
                return rv;
        }

        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_EVENT_LOG)) {
                dbg("Resource %d in Domain %d does not have EL.",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        add_el_entry = h->abi->add_el_entry;

        if (!add_el_entry) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = add_el_entry(h->hnd, ResourceId, EvtEntry);
        if(rv != SA_OK) {
                dbg("EL add entry failed");
        }


        /* to get REL entry into infrastructure */
        rv = oh_get_events();
        if(rv != SA_OK) {
                dbg("Event loop failed");
        }

        return rv;
}

SaErrorT SAHPI_API saHpiEventLogClear (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId)
{
        SaErrorT rv;
        SaErrorT (*clear_el)(void *hnd, SaHpiResourceIdT id);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        struct oh_domain *d;
        SaHpiDomainIdT did;


        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */

        /* test for special domain case */
        if (ResourceId == SAHPI_UNSPECIFIED_RESOURCE_ID) {
                rv = oh_el_clear(d->del);
                oh_release_domain(d); /* Unlock domain */
                return rv;
        }

        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_EVENT_LOG)) {
                dbg("Resource %d in Domain %d does not have EL",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        clear_el = h->abi->clear_el;
        if (!clear_el) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = clear_el(h->hnd, ResourceId);
        if(rv != SA_OK) {
                dbg("EL delete entry failed");
        }

        return rv;
}

SaErrorT SAHPI_API saHpiEventLogTimeGet (
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_IN  SaHpiResourceIdT ResourceId,
        SAHPI_OUT SaHpiTimeT       *Time)
{
        SaHpiEventLogInfoT info;
        SaErrorT rv;

        /* Test pointer parameters for invalid pointers */
        if (Time == NULL)
        {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        rv = saHpiEventLogInfoGet(SessionId, ResourceId, &info);

        if(rv != SA_OK) {
                return rv;
        }

        *Time = info.CurrentTime;

        return SA_OK;
}

SaErrorT SAHPI_API saHpiEventLogTimeSet (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiTimeT       Time)
{
        SaErrorT rv;
        SaErrorT (*set_el_time)(void *hnd, SaHpiResourceIdT id, SaHpiTimeT time);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        struct oh_domain *d;
        SaHpiDomainIdT did;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */

        /* test for special domain case */
        if (ResourceId == SAHPI_UNSPECIFIED_RESOURCE_ID) {
                rv = oh_el_timeset(d->del, Time);
                oh_release_domain(d); /* Unlock domain */
                return rv;
        }

        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_EVENT_LOG)) {
                dbg("Resource %d in Domain %d does not have EL",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_el_time = h->abi->set_el_time;

        if (!set_el_time) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_el_time(h->hnd, ResourceId, Time);
        if(rv != SA_OK) {
                dbg("Set EL time failed");
        }

        return rv;
}

SaErrorT SAHPI_API saHpiEventLogStateGet (
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_IN  SaHpiResourceIdT ResourceId,
        SAHPI_OUT SaHpiBoolT       *Enable)
{
        SaHpiEventLogInfoT info;
        SaErrorT rv;

        /* Test pointer parameters for invalid pointers */
        if (Enable == NULL)
        {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        rv = saHpiEventLogInfoGet(SessionId, ResourceId, &info);

        if(rv != SA_OK) {
                return rv;
        }

        *Enable = info.Enabled;

        return SA_OK;
}

SaErrorT SAHPI_API saHpiEventLogStateSet (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiBoolT       Enable)
{
        struct oh_domain *d;
        SaHpiDomainIdT did;
        SaHpiRptEntryT *res;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */

        /* test for special domain case */
        if (ResourceId == SAHPI_UNSPECIFIED_RESOURCE_ID) {                
                d->del->enabled = Enable;
                oh_release_domain(d); /* Unlock domain */
                return SA_OK;
        }
        
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_EVENT_LOG)) {
                dbg("Resource %d in Domain %d does not have EL",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        } else {
                oh_release_domain(d); /* Unlock domain */
                /* FIXME: Need to add set_el_state abi call */
                return SA_ERR_HPI_UNSUPPORTED_API;
        }
}

SaErrorT SAHPI_API saHpiEventLogOverflowReset (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId)
{
        struct oh_handler *h;
        struct oh_domain *d;
        SaHpiDomainIdT did;
	SaHpiRptEntryT *res;
	SaErrorT rv = SA_OK;
	SaErrorT (*reset_el_overflow)(void *hnd, SaHpiResourceIdT id);

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */

        /* test for special domain case */
        if (ResourceId == SAHPI_UNSPECIFIED_RESOURCE_ID) {
                rv = oh_el_overflowreset(d->del);
                oh_release_domain(d); /* Unlock domain */
                return rv;
        }

        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_EVENT_LOG)) {
                dbg("Resource %d in Domain %d does not have EL",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        reset_el_overflow = h->abi->reset_el_overflow;

        if (!reset_el_overflow) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = reset_el_overflow(h->hnd, ResourceId);
        if(rv != SA_OK) {
                trace("Reset EL Oveerflow not SA_OK");
        }

        return rv;

}

/*********************************************************************
 *
 *  Event Functions
 *
 ********************************************************************/

SaErrorT SAHPI_API saHpiSubscribe (
        SAHPI_IN SaHpiSessionIdT SessionId)
{
        SaHpiDomainIdT did;
        SaHpiBoolT session_state;
        SaErrorT error;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        error = oh_get_session_subscription(SessionId, &session_state);
        if (error) {
                dbg("Error subscribing to SessionId: %d", SessionId);
                return error;
        }

        if (session_state != OH_UNSUBSCRIBED) {
                dbg("Cannot subscribe if session is not unsubscribed.");
                return SA_ERR_HPI_DUPLICATE;
        }

        error = oh_set_session_subscription(SessionId, SAHPI_TRUE);

        return error;
}

SaErrorT SAHPI_API saHpiUnsubscribe (
        SAHPI_IN SaHpiSessionIdT SessionId)
{
        SaHpiDomainIdT did;
        SaHpiBoolT session_state;
        SaErrorT error;
        SaErrorT terror = SA_OK;
        struct oh_event event;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        error = oh_get_session_subscription(SessionId, &session_state);
        if (error) {
                dbg("Error reading session subscription from SessionId: %d", SessionId);
                return error;
        }

        if (session_state == OH_UNSUBSCRIBED) {
                dbg("Cannot unsubscribe if session is not subscribed.");
                return SA_ERR_HPI_INVALID_REQUEST;
        }

        error = oh_set_session_subscription(SessionId, SAHPI_FALSE);
        if (error) {
                dbg("Error unsubscribing to SessionId: %d", SessionId);
                return error;
        }

        /* Flush session's event queue 
         * we use a temp error variable as the dequeue loop signals an
         * end via a TIMEOUT error.  If it is anything else, we assign it
         * to the return value.
         */
        
        error = SA_OK;

        while (terror == SA_OK) {
                terror = oh_dequeue_session_event(SessionId,
                                                  SAHPI_TIMEOUT_IMMEDIATE,
                                                  &event);
        }
        
        if(terror != SA_ERR_HPI_TIMEOUT) {
                dbg("Unexpected error occured in clearing the event queues");
                error = terror;
        }

        return error;
}

SaErrorT SAHPI_API saHpiEventGet (
        SAHPI_IN    SaHpiSessionIdT      SessionId,
        SAHPI_IN    SaHpiTimeoutT        Timeout,
        SAHPI_OUT   SaHpiEventT          *Event,
        SAHPI_INOUT SaHpiRdrT            *Rdr,
        SAHPI_INOUT SaHpiRptEntryT       *RptEntry,
        SAHPI_INOUT SaHpiEvtQueueStatusT *EventQueueStatus)
{

        SaHpiDomainIdT did;
        SaHpiBoolT session_state;
        struct oh_event e;
        SaErrorT error;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        if (Timeout < SAHPI_TIMEOUT_BLOCK || !Event) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        if( !oh_run_threaded() && Timeout != SAHPI_TIMEOUT_IMMEDIATE) {
                dbg("Can not support timeouts in non threaded mode");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        error = oh_get_session_subscription(SessionId, &session_state);
        if (error) return error;

        if (session_state != OH_SUBSCRIBED) {
                return SA_ERR_HPI_INVALID_REQUEST;
        }

        if ( !oh_run_threaded() ) {
                error = oh_get_events();
                if (error < 0) return SA_ERR_HPI_UNKNOWN;
        }

        error = oh_dequeue_session_event(SessionId, Timeout, &e);
        if (error) return error;

        /* Return event, resource and rdr */
        *Event = e.u.hpi_event.event;

        /* EventQueueStatus may be NULL if you don't care */
        if(EventQueueStatus) {
                *EventQueueStatus = 0x0000;
        }

        if (RptEntry && e.u.hpi_event.res.ResourceId != 0) {
                *RptEntry = e.u.hpi_event.res;
        }

        if (Rdr && e.u.hpi_event.rdr.RecordId != 0) {
                *Rdr = e.u.hpi_event.rdr;
        }

        return SA_OK;
}

SaErrorT SAHPI_API saHpiEventAdd (
        SAHPI_IN SaHpiSessionIdT SessionId,
        SAHPI_IN SaHpiEventT     *EvtEntry)
{
        SaHpiDomainIdT did;
        struct oh_event e;
        GArray *session_list = NULL;
        SaErrorT error;
        unsigned int i;

	error = oh_valid_addevent(EvtEntry);
	if (error) return(error);

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        e.did = did;
        e.hid = 0;
        e.type = OH_ET_HPI;
        e.u.hpi_event.event = *EvtEntry;

        session_list = oh_list_sessions(did);
        for (i = 0; i < session_list->len; i++) {
                error = oh_queue_session_event(SessionId, &e);
                if (error) break;
        }
        g_array_free(session_list, TRUE);

        return error;
}

/*********************************************************************
 *
 *  DAT Functions
 *
 ********************************************************************/

SaErrorT SAHPI_API saHpiAlarmGetNext (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiSeverityT  Severity,
                SAHPI_IN SaHpiBoolT      UnacknowledgedOnly,
                SAHPI_INOUT SaHpiAlarmT  *Alarm)
{
        SaHpiDomainIdT did = 0;
        SaHpiAlarmT *a = NULL;
        SaHpiStatusCondTypeT type = SAHPI_STATUS_COND_TYPE_USER;
        struct oh_domain *d = NULL;
        SaErrorT error = SA_ERR_HPI_NOT_PRESENT;
        
        OH_CHECK_INIT_STATE(SessionId);
        
        if (!oh_lookup_severity(Severity) || !Alarm) {
                return SA_ERR_HPI_INVALID_PARAMS;
        } else if (Alarm->AlarmId == SAHPI_LAST_ENTRY) {
                return error;
        }
        
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        
        a = oh_get_alarm(d, &Alarm->AlarmId, &Severity, &type,
                         NULL, NULL, NULL, NULL,
                         UnacknowledgedOnly, 1); /* get next alarm */
        if (a) {
                if (Alarm->AlarmId != SAHPI_FIRST_ENTRY &&
                    Alarm->Timestamp != a->Timestamp) {
                        error = SA_ERR_HPI_INVALID_DATA;
                } else {
                        error = SA_OK;
                }
        }
                
        oh_release_domain(d);
        return error;
}

SaErrorT SAHPI_API saHpiAlarmGet(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiAlarmIdT   AlarmId,
                SAHPI_OUT SaHpiAlarmT    *Alarm)
{
        SaHpiDomainIdT did = 0;
        struct oh_domain *d = NULL;
        SaHpiAlarmT *a = NULL;
        SaHpiStatusCondTypeT type = SAHPI_STATUS_COND_TYPE_USER;
        SaErrorT error = SA_ERR_HPI_NOT_PRESENT;
        
        OH_CHECK_INIT_STATE(SessionId);
        
        if (!Alarm) return SA_ERR_HPI_INVALID_PARAMS;
        
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        
        a = oh_get_alarm(d, &Alarm->AlarmId, NULL, &type,
                         NULL, NULL, NULL, NULL,
                         0, 0);
        if (a) {
                memcpy(Alarm, a, sizeof(SaHpiAlarmT));
                error = SA_OK;
        }        
                
        oh_release_domain(d);
        return error;
}

SaErrorT SAHPI_API saHpiAlarmAcknowledge(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiAlarmIdT   AlarmId,
                SAHPI_IN SaHpiSeverityT  Severity)
{
        SaHpiDomainIdT did = 0;
        struct oh_domain *d = NULL;
        SaHpiAlarmT *a = NULL;
        SaHpiStatusCondTypeT type = SAHPI_STATUS_COND_TYPE_USER;
        SaErrorT error = SA_ERR_HPI_NOT_PRESENT;
        
        OH_CHECK_INIT_STATE(SessionId);
        
        if (AlarmId == SAHPI_ENTRY_UNSPECIFIED &&
            !oh_lookup_severity(Severity))
                return SA_ERR_HPI_INVALID_PARAMS;
        
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        
        if (AlarmId != SAHPI_ENTRY_UNSPECIFIED) { /* Acknowledge specific alarm */
                a = oh_get_alarm(d, &AlarmId, NULL, &type,
                                 NULL, NULL, NULL, NULL,
                                 0, 0);
                if (a) {
                        a->Acknowledged = SAHPI_TRUE;
                        error = SA_OK;
                }
        } else { /* Acknowledge group of alarms, by severity */
                SaHpiAlarmIdT aid = SAHPI_FIRST_ENTRY;
                a = oh_get_alarm(d, &aid, &Severity, &type,
                                 NULL, NULL, NULL, NULL,
                                 0, 1);                
                while (a) {
                        a->Acknowledged = SAHPI_TRUE;
                        a = oh_get_alarm(d, &a->AlarmId, &Severity, &type,
                                         NULL, NULL, NULL, NULL,
                                         0, 1);
                }
                error = SA_OK;
        }
        
        oh_release_domain(d);
        return error;
}

SaErrorT SAHPI_API saHpiAlarmAdd(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_INOUT SaHpiAlarmT  *Alarm)
{
        SaHpiDomainIdT did = 0;
        struct oh_domain *d = NULL;
        SaHpiAlarmT *a = NULL;
                        
        OH_CHECK_INIT_STATE(SessionId);
        
        if (!Alarm &&
            !oh_lookup_severity(Alarm->Severity) &&
            Alarm->AlarmCond.Type != SAHPI_STATUS_COND_TYPE_USER)
                return SA_ERR_HPI_INVALID_PARAMS;
        
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        
        /* Add new alarm */
        a = oh_add_alarm(d, Alarm);
                        
        oh_release_domain(d);
        return SA_OK;
}

SaErrorT SAHPI_API saHpiAlarmDelete(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiAlarmIdT   AlarmId,
                SAHPI_IN SaHpiSeverityT  Severity)
{
        SaHpiDomainIdT did = 0;
        struct oh_domain *d = NULL;
        SaHpiAlarmT *a = NULL;
        SaHpiStatusCondTypeT type = SAHPI_STATUS_COND_TYPE_USER;
        SaErrorT error = SA_ERR_HPI_NOT_PRESENT;
        
        OH_CHECK_INIT_STATE(SessionId);
        
        if (AlarmId == SAHPI_ENTRY_UNSPECIFIED &&
            !oh_lookup_severity(Severity))
                return SA_ERR_HPI_INVALID_PARAMS;
        
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        
        if (AlarmId != SAHPI_ENTRY_UNSPECIFIED) { /* Look for specific alarm */
                a = oh_get_alarm(d, &AlarmId, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 0, 0);
                if (a) {
                        if (a->AlarmCond.Type != SAHPI_STATUS_COND_TYPE_USER) {
                                error = SA_ERR_HPI_READ_ONLY;
                        } else {
                                d->dat.list = g_slist_remove(d->dat.list, a);
                                g_free(a);
                                error = SA_OK;
                        }
                }
        } else { /* Delete group of alarms by severity */
                oh_remove_alarm(d, &Severity, &type, NULL, NULL,
                                NULL, NULL, NULL, 1);
                error = SA_OK;
        }
        
        oh_release_domain(d);
        return error;
}

/*********************************************************************
 *
 *  RDR Functions
 *
 ********************************************************************/

SaErrorT SAHPI_API saHpiRdrGet (
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_IN  SaHpiResourceIdT ResourceId,
        SAHPI_IN  SaHpiEntryIdT    EntryId,
        SAHPI_OUT SaHpiEntryIdT    *NextEntryId,
        SAHPI_OUT SaHpiRdrT        *Rdr)
{
        struct oh_domain *d;
        SaHpiDomainIdT did;
        SaHpiRptEntryT *res = NULL;
        SaHpiRdrT *rdr_cur;
        SaHpiRdrT *rdr_next;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        /* Test pointer parameters for invalid pointers */
        if (EntryId == SAHPI_LAST_ENTRY || !Rdr || !NextEntryId)
        {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_GET_DOMAIN(did, d); /* Lock domain */

        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_RDR)) {
                dbg("No RDRs for Resource %d in Domain %d",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        if(EntryId == SAHPI_FIRST_ENTRY) {
                rdr_cur = oh_get_rdr_next(&(d->rpt), ResourceId, SAHPI_FIRST_ENTRY);
        } else {
                rdr_cur = oh_get_rdr_by_id(&(d->rpt), ResourceId, EntryId);
        }

        if (rdr_cur == NULL) {
                dbg("Requested RDR, Domain[%d]->Resource[%d]->RDR[%d], is not present",
                    did, ResourceId, EntryId);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_NOT_PRESENT;
        }

        memcpy(Rdr, rdr_cur, sizeof(*Rdr));

        rdr_next = oh_get_rdr_next(&(d->rpt), ResourceId, rdr_cur->RecordId);
        if(rdr_next == NULL) {
                *NextEntryId = SAHPI_LAST_ENTRY;
        } else {
                *NextEntryId = rdr_next->RecordId;
        }

        oh_release_domain(d); /* Unlock domain */

        return SA_OK;
}

SaErrorT SAHPI_API saHpiRdrGetByInstrumentId (
        SAHPI_IN  SaHpiSessionIdT    SessionId,
        SAHPI_IN  SaHpiResourceIdT   ResourceId,
        SAHPI_IN  SaHpiRdrTypeT      RdrType,
        SAHPI_IN  SaHpiInstrumentIdT InstrumentId,
        SAHPI_OUT SaHpiRdrT          *Rdr)
{
        SaHpiRptEntryT *res = NULL;
        SaHpiRdrT *rdr_cur;
        SaHpiDomainIdT did;
        SaHpiCapabilitiesT cap;
        struct oh_domain *d = NULL;

        /* Test pointer parameters for invalid pointers */
        if (!oh_lookup_rdrtype(RdrType) ||
            RdrType == SAHPI_NO_RECORD || !Rdr)
        {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */

        OH_RESOURCE_GET(d, ResourceId, res);
        cap = res->ResourceCapabilities;

        if(!(cap & SAHPI_CAPABILITY_RDR)) {
                dbg("No RDRs for Resource %d in Domain %d",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        /* ensure that the resource has something of that type */
        switch(RdrType) {
        case SAHPI_CTRL_RDR:
                if(!(cap & SAHPI_CAPABILITY_CONTROL)) {
                        dbg("No Controls for Resource %d in Domain %d",ResourceId,did);
                        oh_release_domain(d); /* Unlock domain */
                        return SA_ERR_HPI_CAPABILITY;
                }
                break;
        case SAHPI_SENSOR_RDR:
                if(!(cap & SAHPI_CAPABILITY_SENSOR)) {
                        dbg("No Sensors for Resource %d in Domain %d",ResourceId,did);
                        oh_release_domain(d); /* Unlock domain */
                        return SA_ERR_HPI_CAPABILITY;
                }
                break;
        case SAHPI_INVENTORY_RDR:
                if(!(cap & SAHPI_CAPABILITY_INVENTORY_DATA)) {
                        dbg("No IDRs for Resource %d in Domain %d",ResourceId,did);
                        oh_release_domain(d); /* Unlock domain */
                        return SA_ERR_HPI_CAPABILITY;
                }
                break;
        case SAHPI_WATCHDOG_RDR:
                if(!(cap & SAHPI_CAPABILITY_WATCHDOG)) {
                        dbg("No Watchdogs for Resource %d in Domain %d",ResourceId,did);
                        oh_release_domain(d); /* Unlock domain */
                        return SA_ERR_HPI_CAPABILITY;
                }
                break;
        case SAHPI_ANNUNCIATOR_RDR:
                if(!(cap & SAHPI_CAPABILITY_ANNUNCIATOR)) {
                        dbg("No Annunciators for Resource %d in Domain %d",ResourceId,did);
                        oh_release_domain(d); /* Unlock domain */
                        return SA_ERR_HPI_CAPABILITY;
                }
                break;
        default:
                dbg("Not a valid Rdr Type %d", RdrType);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        /* now that we have a pretty good notion that all is well, try the lookup */

        rdr_cur = oh_get_rdr_by_type(&(d->rpt), ResourceId, RdrType, InstrumentId);

        if (rdr_cur == NULL) {
                dbg("Requested RDR, Domain[%d]->Resource[%d]->RDR[%d,%d], is not present",
                    did, ResourceId, RdrType, InstrumentId);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_NOT_PRESENT;
        }
        memcpy(Rdr, rdr_cur, sizeof(*Rdr));
        oh_release_domain(d); /* Unlock domain */


        return SA_OK;
}

/*********************************************************************
 *
 *  Sensor Functions
 *
 ********************************************************************/

SaErrorT SAHPI_API saHpiSensorReadingGet (
        SAHPI_IN    SaHpiSessionIdT     SessionId,
        SAHPI_IN    SaHpiResourceIdT    ResourceId,
        SAHPI_IN    SaHpiSensorNumT     SensorNum,
        SAHPI_INOUT SaHpiSensorReadingT *Reading,
        SAHPI_INOUT SaHpiEventStateT    *EventState)
{
        SaErrorT rv;
        SaErrorT (*get_func) (void *, SaHpiResourceIdT, SaHpiSensorNumT,
                              SaHpiSensorReadingT *, SaHpiEventStateT *);

        struct oh_handler *h;
        SaHpiRptEntryT *res;
        SaHpiRdrT *rdr;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_SENSOR)) {
                dbg("Resource %d in Domain %d doesn't have sensors",
                    ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        rdr = oh_get_rdr_by_type(&(d->rpt), ResourceId, SAHPI_SENSOR_RDR, SensorNum);

        if (!rdr) {
                dbg("No Sensor %d found for ResourceId %d in Domain %d",
                    SensorNum, ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_NOT_PRESENT;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        get_func = h->abi->get_sensor_reading;

        if (!get_func) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_func(h->hnd, ResourceId, SensorNum, Reading, EventState);

        return rv;
}

SaErrorT SAHPI_API saHpiSensorThresholdsGet (
        SAHPI_IN SaHpiSessionIdT        SessionId,
        SAHPI_IN SaHpiResourceIdT       ResourceId,
        SAHPI_IN SaHpiSensorNumT        SensorNum,
        SAHPI_IN SaHpiSensorThresholdsT *SensorThresholds)
{
        SaErrorT rv;
        SaErrorT (*get_func) (void *, SaHpiResourceIdT, SaHpiSensorNumT,
                              SaHpiSensorThresholdsT *);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!SensorThresholds) return SA_ERR_HPI_INVALID_PARAMS;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_SENSOR)) {
                dbg("Resource %d in Domain %d doesn't have sensors",
                    ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        get_func = h->abi->get_sensor_thresholds;

        if (!get_func) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_func(h->hnd, ResourceId, SensorNum, SensorThresholds);

        return rv;
}

SaErrorT SAHPI_API saHpiSensorThresholdsSet (
        SAHPI_IN  SaHpiSessionIdT        SessionId,
        SAHPI_IN  SaHpiResourceIdT       ResourceId,
        SAHPI_IN  SaHpiSensorNumT        SensorNum,
        SAHPI_OUT SaHpiSensorThresholdsT *SensorThresholds)
{
        SaErrorT rv;
        SaErrorT (*set_func) (void *, SaHpiResourceIdT, SaHpiSensorNumT,
                              const SaHpiSensorThresholdsT *);
        SaHpiRptEntryT *res;
        SaHpiRdrT *rdr;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!SensorThresholds) return SA_ERR_HPI_INVALID_DATA;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_SENSOR)) {
                dbg("Resource %d in Domain %d doesn't have sensors",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        rdr = oh_get_rdr_by_type(&(d->rpt), ResourceId, SAHPI_SENSOR_RDR, SensorNum);

        if (!rdr) {
                dbg("No Sensor %d found for ResourceId %d in Domain %d",
                    SensorNum, ResourceId, did);
                oh_release_domain(d);
                return SA_ERR_HPI_NOT_PRESENT;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_func = h->abi->set_sensor_thresholds;
        if (!set_func) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_func(h->hnd, ResourceId, SensorNum, SensorThresholds);

        return rv;
}

SaErrorT SAHPI_API saHpiSensorTypeGet (
        SAHPI_IN  SaHpiSessionIdT     SessionId,
        SAHPI_IN  SaHpiResourceIdT    ResourceId,
        SAHPI_IN  SaHpiSensorNumT     SensorNum,
        SAHPI_OUT SaHpiSensorTypeT    *Type,
        SAHPI_OUT SaHpiEventCategoryT *Category)
{
        SaHpiRptEntryT *res;
        SaHpiRdrT *rdr;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!Type || !Category) return SA_ERR_HPI_INVALID_PARAMS;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_SENSOR)) {
                dbg("Resource %d in Domain %d doesn't have sensors",
                    ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        rdr = oh_get_rdr_by_type(&(d->rpt), ResourceId, SAHPI_SENSOR_RDR, SensorNum);

        if (!rdr) {
                dbg("No Sensor num %d found for Resource %d in Domain %d",
                    SensorNum, ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_NOT_PRESENT;
        }

        memcpy(Type,
               &(rdr->RdrTypeUnion.SensorRec.Type),
               sizeof(SaHpiSensorTypeT));

        memcpy(Category,
               &(rdr->RdrTypeUnion.SensorRec.Category),
               sizeof(SaHpiEventCategoryT));

        oh_release_domain(d); /* Unlock domain */

        return SA_OK;
}

SaErrorT SAHPI_API saHpiSensorEnableGet (
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_IN  SaHpiResourceIdT ResourceId,
        SAHPI_IN  SaHpiSensorNumT  SensorNum,
        SAHPI_OUT SaHpiBoolT       *SensorEnabled)
{
        SaErrorT rv;
        SaErrorT (*get_sensor_enable)(void *hnd, SaHpiResourceIdT,
                                      SaHpiSensorNumT,
                                      SaHpiBoolT *enable);

        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!SensorEnabled) return SA_ERR_HPI_INVALID_PARAMS;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_SENSOR)) {
                dbg("Resource %d doesn't have sensors in Domain %d",
                    ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        get_sensor_enable = h->abi->get_sensor_enable;
        if (!get_sensor_enable) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_sensor_enable(h->hnd, ResourceId, SensorNum, SensorEnabled);
	return rv;
}

SaErrorT SAHPI_API saHpiSensorEnableSet (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiSensorNumT  SensorNum,
        SAHPI_IN SaHpiBoolT       SensorEnabled)
{
        SaErrorT rv;
        SaErrorT (*set_sensor_enable)(void *hnd, SaHpiResourceIdT,
                                      SaHpiSensorNumT,
                                      SaHpiBoolT enable);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_SENSOR)) {
                dbg("Resource %d doesn't have sensors in Domain %d",
                    ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_sensor_enable = h->abi->set_sensor_enable;
        if (!set_sensor_enable) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_sensor_enable(h->hnd, ResourceId, SensorNum, SensorEnabled);
        if (rv == SA_OK) {                
                oh_detect_sensor_enable_alarm(did, ResourceId,
                                              SensorNum, SensorEnabled);                
        }

        return rv;
}

SaErrorT SAHPI_API saHpiSensorEventEnableGet (
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_IN  SaHpiResourceIdT ResourceId,
        SAHPI_IN  SaHpiSensorNumT  SensorNum,
        SAHPI_OUT SaHpiBoolT       *SensorEventsEnabled)
{
        SaErrorT rv;
        SaErrorT (*get_sensor_event_enables)(void *hnd, SaHpiResourceIdT,
                                             SaHpiSensorNumT,
                                             SaHpiBoolT *enables);

        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!SensorEventsEnabled) return SA_ERR_HPI_INVALID_PARAMS;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_SENSOR)) {
                dbg("Resource %d doesn't have sensors in Domain %d",
                    ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        get_sensor_event_enables = h->abi->get_sensor_event_enables;
        if (!get_sensor_event_enables) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_sensor_event_enables(h->hnd, ResourceId, SensorNum, SensorEventsEnabled);

        return rv;
}

SaErrorT SAHPI_API saHpiSensorEventEnableSet (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiSensorNumT  SensorNum,
        SAHPI_IN SaHpiBoolT       SensorEventsEnabled)
{
        SaErrorT rv;
        SaErrorT (*set_sensor_event_enables)(void *hnd, SaHpiResourceIdT,
                                             SaHpiSensorNumT,
                                             const SaHpiBoolT enables);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_SENSOR)) {
                dbg("Resource %d doesn't have sensors in Domain %d",
                    ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_sensor_event_enables = h->abi->set_sensor_event_enables;
        if (!set_sensor_event_enables) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_sensor_event_enables(h->hnd, ResourceId,
                                      SensorNum, SensorEventsEnabled);
        if (rv == SA_OK) {                
                oh_detect_sensor_enable_alarm(did, ResourceId,
                                              SensorNum, SensorEventsEnabled);
        }

        return rv;
}

SaErrorT SAHPI_API saHpiSensorEventMasksGet (
        SAHPI_IN  SaHpiSessionIdT         SessionId,
        SAHPI_IN  SaHpiResourceIdT        ResourceId,
        SAHPI_IN  SaHpiSensorNumT         SensorNum,
        SAHPI_INOUT SaHpiEventStateT      *AssertEventMask,
        SAHPI_INOUT SaHpiEventStateT      *DeassertEventMask)
{
        SaErrorT rv;
        SaErrorT (*get_sensor_event_masks)(void *hnd, SaHpiResourceIdT,
                                           SaHpiSensorNumT,
                                           SaHpiEventStateT   *AssertEventMask,
                                           SaHpiEventStateT   *DeassertEventMask);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!AssertEventMask) return SA_ERR_HPI_INVALID_PARAMS;
        if (!DeassertEventMask) return SA_ERR_HPI_INVALID_PARAMS;


        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_SENSOR)) {
                dbg("Resource %d doesn't have sensors in Domain %d",
                    ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        get_sensor_event_masks = h->abi->get_sensor_event_masks;
        if (!get_sensor_event_masks) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_sensor_event_masks(h->hnd, ResourceId, SensorNum,
                                    AssertEventMask, DeassertEventMask);
        return rv;
}

SaErrorT SAHPI_API saHpiSensorEventMasksSet (
        SAHPI_IN  SaHpiSessionIdT                 SessionId,
        SAHPI_IN  SaHpiResourceIdT                ResourceId,
        SAHPI_IN  SaHpiSensorNumT                 SensorNum,
        SAHPI_IN  SaHpiSensorEventMaskActionT     Action,
        SAHPI_IN  SaHpiEventStateT                AssertEventMask,
        SAHPI_IN  SaHpiEventStateT                DeassertEventMask)
{
        SaErrorT rv;
        SaErrorT (*set_sensor_event_masks)(void *hnd, SaHpiResourceIdT,
                                           SaHpiSensorNumT,
					   SaHpiSensorEventMaskActionT   Action,
                                           SaHpiEventStateT   AssertEventMask,
                                           SaHpiEventStateT   DeassertEventMask);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_SENSOR)) {
                dbg("Resource %d doesn't have sensors in Domain %d",
                    ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_sensor_event_masks = h->abi->set_sensor_event_masks;
        if (!set_sensor_event_masks) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_sensor_event_masks(h->hnd, ResourceId, SensorNum,
				    Action,
                                    AssertEventMask,
                                    DeassertEventMask);
        if (rv == SA_OK) {                
                oh_detect_sensor_mask_alarm(did, ResourceId,
                                            SensorNum,
                                            Action, DeassertEventMask);
        }
        
        return rv;
}

/* End Sensor functions */

/*********************************************************************
 *
 *  Control Functions
 *
 ********************************************************************/

SaErrorT SAHPI_API saHpiControlTypeGet (
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_IN  SaHpiResourceIdT ResourceId,
        SAHPI_IN  SaHpiCtrlNumT    CtrlNum,
        SAHPI_OUT SaHpiCtrlTypeT   *Type)
{
        SaHpiRptEntryT *res;
        SaHpiRdrT *rdr;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!Type) return SA_ERR_HPI_INVALID_PARAMS;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_CONTROL)) {
                dbg("Resource %d in Domain %d doesn't have controls",
                    ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        rdr = oh_get_rdr_by_type(&(d->rpt), ResourceId, SAHPI_CTRL_RDR, CtrlNum);
        if (!rdr) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_NOT_PRESENT;
        }

        memcpy(Type,
               &(rdr->RdrTypeUnion.CtrlRec.Type),
               sizeof(SaHpiCtrlTypeT));

        oh_release_domain(d); /* Unlock domain */

        return SA_OK;
}

SaErrorT SAHPI_API saHpiControlGet (
        SAHPI_IN    SaHpiSessionIdT  SessionId,
        SAHPI_IN    SaHpiResourceIdT ResourceId,
        SAHPI_IN    SaHpiCtrlNumT    CtrlNum,
        SAHPI_OUT   SaHpiCtrlModeT   *CtrlMode,
        SAHPI_INOUT SaHpiCtrlStateT  *CtrlState)
{
        SaErrorT rv;
        SaErrorT (*get_func)(void *, SaHpiResourceIdT, SaHpiCtrlNumT,
                             SaHpiCtrlModeT *, SaHpiCtrlStateT *);
        SaHpiRptEntryT *res;
        SaHpiRdrT *rdr;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_CONTROL)) {
                dbg("Resource %d in Domain %d doesn't have controls",
                    ResourceId, d->id);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);

        rdr = oh_get_rdr_by_type(&(d->rpt), ResourceId, SAHPI_CTRL_RDR, CtrlNum);
        if (!rdr) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_NOT_PRESENT;
        }
        if(rdr->RdrTypeUnion.CtrlRec.WriteOnly) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_INVALID_CMD;
        }

        oh_release_domain(d); /* Unlock domain */
        
        get_func = h->abi->get_control_state;
        if (!get_func) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_func(h->hnd, ResourceId, CtrlNum, CtrlMode, CtrlState);

        return rv;
}

SaErrorT SAHPI_API saHpiControlSet (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiCtrlNumT    CtrlNum,
        SAHPI_IN SaHpiCtrlModeT   CtrlMode,
        SAHPI_IN SaHpiCtrlStateT  *CtrlState)
{
        SaErrorT rv;
        SaErrorT (*set_func)(void *, SaHpiResourceIdT, SaHpiCtrlNumT, SaHpiCtrlModeT, SaHpiCtrlStateT *);

        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!oh_lookup_ctrlmode(CtrlMode) ||
            (CtrlMode != SAHPI_CTRL_MODE_AUTO && !CtrlState) ||
            (CtrlState && CtrlState->Type == SAHPI_CTRL_TYPE_DIGITAL &&
             !oh_lookup_ctrlstatedigital(CtrlState->StateUnion.Digital)) ||
            (CtrlState && CtrlState->Type == SAHPI_CTRL_TYPE_STREAM &&
             CtrlState->StateUnion.Stream.StreamLength > SAHPI_CTRL_MAX_STREAM_LENGTH))
        {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_CONTROL)) {
                dbg("Resource %d in Domain %d doesn't have controls",
                    ResourceId, did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_func = h->abi->set_control_state;
        if (!set_func) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_func(h->hnd, ResourceId, CtrlNum, CtrlMode, CtrlState);

        return rv;
}


/*********************************************************************
 *
 *  Inventory Functions
 *
 ********************************************************************/

SaErrorT SAHPI_API saHpiIdrInfoGet(
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_IN  SaHpiResourceIdT ResourceId,
        SAHPI_IN  SaHpiIdrIdT      IdrId,
        SAHPI_OUT SaHpiIdrInfoT    *IdrInfo)
{

        SaHpiRptEntryT *res;
        SaErrorT rv = SA_OK;    /* Default to SA_OK */
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        struct oh_handler *h;
        SaErrorT (*set_func)(void *, SaHpiResourceIdT, SaHpiIdrIdT, SaHpiIdrInfoT *);

        if (IdrInfo == NULL) {
                dbg("NULL IdrInfo");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        /* Interface and conformance checking */
        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_INVENTORY_DATA)) {
                dbg("Resource %d in Domain %d doesn't have inventory data",
                    ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_func = h->abi->get_idr_info;
        if (!set_func) {
                dbg("Plugin does not have this function in jump table.");
                return SA_ERR_HPI_INVALID_CMD;
        }

        /* Access Inventory Info from plugin */
        trace("Access IdrInfo from plugin.");
        rv = set_func(h->hnd, ResourceId, IdrId, IdrInfo);

        return rv;
}

SaErrorT SAHPI_API saHpiIdrAreaHeaderGet(
        SAHPI_IN  SaHpiSessionIdT     SessionId,
        SAHPI_IN  SaHpiResourceIdT    ResourceId,
        SAHPI_IN  SaHpiIdrIdT         IdrId,
        SAHPI_IN  SaHpiIdrAreaTypeT   AreaType,
        SAHPI_IN  SaHpiEntryIdT       AreaId,
        SAHPI_OUT SaHpiEntryIdT       *NextAreaId,
        SAHPI_OUT SaHpiIdrAreaHeaderT *Header)
{

        SaHpiRptEntryT *res;
        SaErrorT rv = SA_OK;    /* Default to SA_OK */
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        struct oh_handler *h;
        SaErrorT (*set_func)(void *, SaHpiResourceIdT, SaHpiIdrIdT, SaHpiIdrAreaTypeT,
                              SaHpiEntryIdT, SaHpiEntryIdT *,  SaHpiIdrAreaHeaderT *);

        if ( ((AreaType < SAHPI_IDR_AREATYPE_INTERNAL_USE) ||
             ((AreaType > SAHPI_IDR_AREATYPE_PRODUCT_INFO) &&
             (AreaType != SAHPI_IDR_AREATYPE_UNSPECIFIED)  &&
             (AreaType != SAHPI_IDR_AREATYPE_OEM)) ||
             (AreaId == SAHPI_LAST_ENTRY)||
             (NextAreaId == NULL) ||
             (Header == NULL)))   {
                dbg("Invalid Parameters");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        /* Interface and conformance checking */
        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_INVENTORY_DATA)) {
                dbg("Resource %d in Domain %d doesn't have inventory data",
                    ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_func = h->abi->get_idr_area_header;
        if (!set_func) {
                dbg("Plugin does not have this function in jump table.");
                return SA_ERR_HPI_INVALID_CMD;
        }


        /* Access Inventory Info from plugin */
        trace("Access IdrAreaHeader from plugin.");
        rv = set_func(h->hnd, ResourceId, IdrId, AreaType, AreaId, NextAreaId, Header);

        return rv;
}

SaErrorT SAHPI_API saHpiIdrAreaAdd(
        SAHPI_IN  SaHpiSessionIdT   SessionId,
        SAHPI_IN  SaHpiResourceIdT  ResourceId,
        SAHPI_IN  SaHpiIdrIdT       IdrId,
        SAHPI_IN  SaHpiIdrAreaTypeT AreaType,
        SAHPI_OUT SaHpiEntryIdT     *AreaId)
{
        SaHpiRptEntryT *res;
        SaErrorT rv = SA_OK;    /* Default to SA_OK */
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        struct oh_handler *h;
        SaErrorT (*set_func)(void *, SaHpiResourceIdT, SaHpiIdrIdT, SaHpiIdrAreaTypeT,
                                SaHpiEntryIdT *);

        if ( ((AreaType < SAHPI_IDR_AREATYPE_INTERNAL_USE) ||
             ((AreaType > SAHPI_IDR_AREATYPE_PRODUCT_INFO) &&
             (AreaType != SAHPI_IDR_AREATYPE_UNSPECIFIED)  &&
             (AreaType != SAHPI_IDR_AREATYPE_OEM)) ||
             (AreaId == NULL)))   {
                dbg("Invalid Parameters");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        /* Interface and conformance checking */
        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_INVENTORY_DATA)) {
                dbg("Resource %d in Domain %d doesn't have inventory data",
                    ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_func = h->abi->add_idr_area;
        if (!set_func) {
                dbg("Plugin does not have this function in jump table.");
                return SA_ERR_HPI_INVALID_CMD;
        }

        /* Access Inventory Info from plugin */
        trace("Access IdrAreaAdd from plugin.");
        rv = set_func(h->hnd, ResourceId, IdrId, AreaType, AreaId);

        return rv;
}

SaErrorT SAHPI_API saHpiIdrAreaDelete(
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiIdrIdT      IdrId,
        SAHPI_IN SaHpiEntryIdT    AreaId)
{
        SaHpiRptEntryT *res;
        SaErrorT rv = SA_OK;    /* Default to SA_OK */
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        struct oh_handler *h;
        SaErrorT (*set_func)(void *, SaHpiResourceIdT, SaHpiIdrIdT, SaHpiEntryIdT );

        if (AreaId == SAHPI_LAST_ENTRY)   {
                dbg("Invalid Parameters");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        /* Interface and conformance checking */
        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_INVENTORY_DATA)) {
                dbg("Resource %d in Domain %d doesn't have inventory data",
                    ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_func = h->abi->del_idr_area;
        if (!set_func) {
                dbg("Plugin does not have this function in jump table.");
                return SA_ERR_HPI_INVALID_CMD;
        }

        /* Access Inventory Info from plugin */
        trace("Access IdrAreaDelete from plugin.");
        rv = set_func(h->hnd, ResourceId, IdrId, AreaId);

        return rv;
}

SaErrorT SAHPI_API saHpiIdrFieldGet(
        SAHPI_IN  SaHpiSessionIdT    SessionId,
        SAHPI_IN  SaHpiResourceIdT   ResourceId,
        SAHPI_IN  SaHpiIdrIdT        IdrId,
        SAHPI_IN  SaHpiEntryIdT      AreaId,
        SAHPI_IN  SaHpiIdrFieldTypeT FieldType,
        SAHPI_IN  SaHpiEntryIdT      FieldId,
        SAHPI_OUT SaHpiEntryIdT      *NextFieldId,
        SAHPI_OUT SaHpiIdrFieldT     *Field)
{
        SaHpiRptEntryT *res;
        SaErrorT rv = SA_OK;    /* Default to SA_OK */
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        struct oh_handler *h;
        SaErrorT (*set_func)(void *, SaHpiResourceIdT, SaHpiIdrIdT,
                             SaHpiEntryIdT, SaHpiIdrFieldTypeT, SaHpiEntryIdT,
                             SaHpiEntryIdT *, SaHpiIdrFieldT * );

        if ((((FieldType > SAHPI_IDR_FIELDTYPE_CUSTOM) &&
             (FieldType != SAHPI_IDR_FIELDTYPE_UNSPECIFIED)) ||
             (AreaId == SAHPI_LAST_ENTRY) ||
             (FieldId == SAHPI_LAST_ENTRY) ||
             (NextFieldId == NULL) ||
             (Field == NULL)))    {
                dbg("Invalid Parameters");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        /* Interface and conformance checking */
        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_INVENTORY_DATA)) {
                dbg("Resource %d in Domain %d doesn't have inventory data",
                    ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_func = h->abi->get_idr_field;
        if (!set_func) {
                dbg("Plugin does not have this function in jump table.");
                return SA_ERR_HPI_INVALID_CMD;
        }


        /* Access Inventory Info from plugin */
        trace("Access saHpiIdrFieldGet from plugin.");
        rv = set_func(h->hnd, ResourceId, IdrId, AreaId,
                      FieldType, FieldId, NextFieldId, Field);

        return rv;
}

SaErrorT SAHPI_API saHpiIdrFieldAdd(
        SAHPI_IN    SaHpiSessionIdT  SessionId,
        SAHPI_IN    SaHpiResourceIdT ResourceId,
        SAHPI_IN    SaHpiIdrIdT      IdrId,
        SAHPI_INOUT SaHpiIdrFieldT   *Field)
{
        SaHpiRptEntryT *res;
        SaErrorT rv = SA_OK;    /* Default to SA_OK */
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        struct oh_handler *h;
        SaErrorT (*set_func)(void *, SaHpiResourceIdT, SaHpiIdrIdT,  SaHpiIdrFieldT * );

        if (!Field)   {
                dbg("Invalid Parameter: Field is NULL ");
                return SA_ERR_HPI_INVALID_PARAMS;
        } else if (Field->Type > SAHPI_IDR_FIELDTYPE_CUSTOM) {
                dbg("Invalid Parameters in Field->Type");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        /* Interface and conformance checking */
        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_INVENTORY_DATA)) {
                dbg("Resource %d in Domain %d doesn't have inventory data",
                    ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_func = h->abi->add_idr_field;
        if (!set_func) {
                dbg("Plugin does not have this function in jump table.");
                return SA_ERR_HPI_INVALID_CMD;
        }

        /* Access Inventory Info from plugin */
        trace("Access saHpiIdrFieldAdd from plugin.");
        rv = set_func(h->hnd, ResourceId, IdrId, Field);

        return rv;
}

SaErrorT SAHPI_API saHpiIdrFieldSet(
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiIdrIdT      IdrId,
        SAHPI_IN SaHpiIdrFieldT   *Field)
{
        SaHpiRptEntryT *res;
        SaErrorT rv = SA_OK;    /* Default to SA_OK */
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        struct oh_handler *h;
        SaErrorT (*set_func)(void *, SaHpiResourceIdT, SaHpiIdrIdT, SaHpiIdrFieldT * );

        if (!Field)   {
                dbg("Invalid Parameter: Field is NULL ");
                return SA_ERR_HPI_INVALID_PARAMS;
        } else if (Field->Type > SAHPI_IDR_FIELDTYPE_CUSTOM) {
                dbg("Invalid Parameters in Field->Type");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        /* Interface and conformance checking */
        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_INVENTORY_DATA)) {
                dbg("Resource %d in Domain %d doesn't have inventory data",
                    ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_func = h->abi->set_idr_field;
        if (!set_func) {
                dbg("Plugin does not have this function in jump table.");
                return SA_ERR_HPI_INVALID_CMD;
        }

        /* Access Inventory Info from plugin */
        trace("Access saHpiIdrFieldSet from plugin.");
        rv = set_func(h->hnd, ResourceId, IdrId, Field);

        return rv;
}

SaErrorT SAHPI_API saHpiIdrFieldDelete(
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiIdrIdT      IdrId,
        SAHPI_IN SaHpiEntryIdT    AreaId,
        SAHPI_IN SaHpiEntryIdT    FieldId)
{
        SaHpiRptEntryT *res;
        SaErrorT rv = SA_OK;    /* Default to SA_OK */
        SaHpiDomainIdT did;
        struct oh_handler *h;
        struct oh_domain *d = NULL;
        SaErrorT (*set_func)(void *, SaHpiResourceIdT, SaHpiIdrIdT, SaHpiEntryIdT, SaHpiEntryIdT );

        if (FieldId == SAHPI_LAST_ENTRY || AreaId == SAHPI_LAST_ENTRY)   {
                dbg("Invalid Parameters");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        /* Interface and conformance checking */
        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_INVENTORY_DATA)) {
                dbg("Resource %d in Domain %d doesn't have inventory data",
                    ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_func = h->abi->del_idr_field;
        if (!set_func) {
                dbg("Plugin does not have this function in jump table.");
                return SA_ERR_HPI_INVALID_CMD;
        }

        /* Access Inventory Info from plugin */
        trace("Access saHpiIdrFieldDelete from plugin.");
        rv = set_func(h->hnd, ResourceId, IdrId, AreaId, FieldId);

        return rv;
}

/* End of Inventory Functions  */

/*********************************************************************
 *
 *  Watchdog Functions
 *
 ********************************************************************/

SaErrorT SAHPI_API saHpiWatchdogTimerGet (
        SAHPI_IN  SaHpiSessionIdT   SessionId,
        SAHPI_IN  SaHpiResourceIdT  ResourceId,
        SAHPI_IN  SaHpiWatchdogNumT WatchdogNum,
        SAHPI_OUT SaHpiWatchdogT    *Watchdog)
{
        SaErrorT rv;
        SaErrorT (*get_func)(void *, SaHpiResourceIdT, SaHpiWatchdogNumT, SaHpiWatchdogT *);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        struct oh_domain *d = NULL;
        SaHpiDomainIdT did;

        if (!Watchdog) return SA_ERR_HPI_INVALID_PARAMS;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_WATCHDOG)) {
                dbg("Resource %d in Domain %d doesn't have watchdog",
                    ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        get_func = h->abi->get_watchdog_info;
        if (!get_func) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_func(h->hnd, ResourceId, WatchdogNum, Watchdog);

        return rv;
}

SaErrorT SAHPI_API saHpiWatchdogTimerSet (
        SAHPI_IN SaHpiSessionIdT   SessionId,
        SAHPI_IN SaHpiResourceIdT  ResourceId,
        SAHPI_IN SaHpiWatchdogNumT WatchdogNum,
        SAHPI_IN SaHpiWatchdogT    *Watchdog)
{
        SaErrorT rv;
        SaErrorT (*set_func)(void *, SaHpiResourceIdT, SaHpiWatchdogNumT, SaHpiWatchdogT *);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        struct oh_domain *d = NULL;
        SaHpiDomainIdT did;

        if (!Watchdog ||
            (Watchdog && (!oh_lookup_watchdogtimeruse(Watchdog->TimerUse) ||
                          !oh_lookup_watchdogaction(Watchdog->TimerAction) ||
                          !oh_lookup_watchdogpretimerinterrupt(Watchdog->PretimerInterrupt)))) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_WATCHDOG)) {
                dbg("Resource %d in Domain %d doesn't have watchdog",
                    ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_func = h->abi->set_watchdog_info;
        if (!set_func) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_func(h->hnd, ResourceId, WatchdogNum, Watchdog);

        return rv;
}

SaErrorT SAHPI_API saHpiWatchdogTimerReset (
        SAHPI_IN SaHpiSessionIdT   SessionId,
        SAHPI_IN SaHpiResourceIdT  ResourceId,
        SAHPI_IN SaHpiWatchdogNumT WatchdogNum)
{
        SaErrorT rv;
        SaErrorT (*reset_func)(void *, SaHpiResourceIdT, SaHpiWatchdogNumT);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        struct oh_domain *d = NULL;
        SaHpiDomainIdT did;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_WATCHDOG)) {
                dbg("Resource %d in Domain %d doesn't have watchdog",
                    ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        reset_func = h->abi->reset_watchdog;
        if (!reset_func) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = reset_func(h->hnd, ResourceId, WatchdogNum);

        return rv;
}

/*******************************************************************************
 *
 *  Annunciator Functions
 *
 ******************************************************************************/

SaErrorT SAHPI_API saHpiAnnunciatorGetNext(
        SAHPI_IN SaHpiSessionIdT            SessionId,
        SAHPI_IN SaHpiResourceIdT           ResourceId,
        SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
        SAHPI_IN SaHpiSeverityT             Severity,
        SAHPI_IN SaHpiBoolT                 UnacknowledgedOnly,
        SAHPI_INOUT SaHpiAnnouncementT      *Announcement)
{
        return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiAnnunciatorGet(
        SAHPI_IN SaHpiSessionIdT            SessionId,
        SAHPI_IN SaHpiResourceIdT           ResourceId,
        SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
        SAHPI_IN SaHpiEntryIdT              EntryId,
        SAHPI_OUT SaHpiAnnouncementT        *Announcement)
{
        return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiAnnunciatorAcknowledge(
        SAHPI_IN SaHpiSessionIdT            SessionId,
        SAHPI_IN SaHpiResourceIdT           ResourceId,
        SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
        SAHPI_IN SaHpiEntryIdT              EntryId,
        SAHPI_IN SaHpiSeverityT             Severity)
{
        return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiAnnunciatorAdd(
        SAHPI_IN SaHpiSessionIdT            SessionId,
        SAHPI_IN SaHpiResourceIdT           ResourceId,
        SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
        SAHPI_INOUT SaHpiAnnouncementT      *Announcement)
{
        return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiAnnunciatorDelete(
        SAHPI_IN SaHpiSessionIdT            SessionId,
        SAHPI_IN SaHpiResourceIdT           ResourceId,
        SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
        SAHPI_IN SaHpiEntryIdT              EntryId,
        SAHPI_IN SaHpiSeverityT             Severity)
{
        return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiAnnunciatorModeGet(
        SAHPI_IN SaHpiSessionIdT            SessionId,
        SAHPI_IN SaHpiResourceIdT           ResourceId,
        SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
        SAHPI_OUT SaHpiAnnunciatorModeT     *Mode)
{
        return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiAnnunciatorModeSet(
        SAHPI_IN SaHpiSessionIdT            SessionId,
        SAHPI_IN SaHpiResourceIdT           ResourceId,
        SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
        SAHPI_IN SaHpiAnnunciatorModeT      Mode)
{
        return SA_ERR_HPI_UNSUPPORTED_API;
}

/*******************************************************************************
 *
 *  Hotswap Functions
 *
 ******************************************************************************/

SaErrorT SAHPI_API saHpiHotSwapPolicyCancel (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId)
{
        SaHpiRptEntryT *res;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        struct oh_resource_data *rd;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        rd = oh_get_resource_data(&(d->rpt), ResourceId);
        if (!rd) {
                dbg( "Can't find resource data for Resource %d in Domain %d",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        rd->controlled = 1;
        oh_release_domain(d); /* Unlock domain */

        return SA_OK;
}

SaErrorT SAHPI_API saHpiResourceActiveSet (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId)
{
        SaErrorT rv;
        SaErrorT (*set_hotswap_state)(void *hnd, SaHpiResourceIdT,
                                      SaHpiHsStateT state);

        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        struct oh_resource_data *rd;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        rd = oh_get_resource_data(&(d->rpt), ResourceId);
        if (!rd) {
                dbg( "Can't find resource data for Resource %d in Domain %d",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        if (!rd->controlled) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_INVALID_CMD;
        }

        /* this was done in the old code, so we do it here */
        rd->controlled = 0;

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_hotswap_state = h->abi->set_hotswap_state;
        if (!set_hotswap_state) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_hotswap_state(h->hnd, ResourceId, SAHPI_HS_STATE_ACTIVE);


        return rv;
}

SaErrorT SAHPI_API saHpiResourceInactiveSet (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId)
{
        SaErrorT rv;
        SaErrorT (*set_hotswap_state)(void *hnd, SaHpiResourceIdT rid,
                                      SaHpiHsStateT state);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;
        struct oh_resource_data *rd;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        rd = oh_get_resource_data(&(d->rpt), ResourceId);
        if (!rd) {
                dbg("Can't find resource data for Resource %d in Domain %d",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        if (!rd->controlled) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_INVALID_CMD;
        }

        rd->controlled = 0;

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_hotswap_state = h->abi->set_hotswap_state;
        if (!set_hotswap_state) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_hotswap_state(h->hnd, ResourceId, SAHPI_HS_STATE_INACTIVE);

        return rv;
}

SaErrorT SAHPI_API saHpiAutoInsertTimeoutGet(
        SAHPI_IN  SaHpiSessionIdT SessionId,
        SAHPI_OUT SaHpiTimeoutT   *Timeout)
{

        SaHpiDomainIdT did;

        if (!Timeout) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        *Timeout = get_hotswap_auto_insert_timeout();

        return SA_OK;
}

SaErrorT SAHPI_API saHpiAutoInsertTimeoutSet(
        SAHPI_IN SaHpiSessionIdT SessionId,
        SAHPI_IN SaHpiTimeoutT   Timeout)
{
        SaHpiDomainIdT did;

        if (Timeout != SAHPI_TIMEOUT_IMMEDIATE &&
            Timeout != SAHPI_TIMEOUT_BLOCK &&
            Timeout < 0)
                return SA_ERR_HPI_INVALID_PARAMS;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);

        set_hotswap_auto_insert_timeout(Timeout);

        return SA_OK;
}

SaErrorT SAHPI_API saHpiAutoExtractTimeoutGet(
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_IN  SaHpiResourceIdT ResourceId,
        SAHPI_OUT SaHpiTimeoutT    *Timeout)
{
        SaHpiRptEntryT *res;
        struct oh_resource_data *rd;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!Timeout) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        rd = oh_get_resource_data(&(d->rpt), ResourceId);
        if (!rd) {
                dbg("Cannot find resource data for Resource %d in Domain %d",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        *Timeout = rd->auto_extract_timeout;
        oh_release_domain(d); /* Unlock domain */

        return SA_OK;
}

SaErrorT SAHPI_API saHpiAutoExtractTimeoutSet(
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiTimeoutT    Timeout)
{
        SaHpiRptEntryT *res;
        struct oh_resource_data *rd;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (Timeout != SAHPI_TIMEOUT_IMMEDIATE &&
            Timeout != SAHPI_TIMEOUT_BLOCK &&
            Timeout < 0)
                return SA_ERR_HPI_INVALID_PARAMS;

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        rd = oh_get_resource_data(&(d->rpt), ResourceId);
        if (!rd) {
                dbg("Cannot find resource data for Resource %d in Domain %d",ResourceId,did);
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        rd->auto_extract_timeout = Timeout;
        oh_release_domain(d); /* Unlock domain */

        return SA_OK;
}

SaErrorT SAHPI_API saHpiHotSwapStateGet (
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_IN  SaHpiResourceIdT ResourceId,
        SAHPI_OUT SaHpiHsStateT    *State)
{
        SaErrorT rv;
        SaErrorT (*get_hotswap_state)(void *hnd, SaHpiResourceIdT rid,
                                      SaHpiHsStateT *state);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!State) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_FRU)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        get_hotswap_state = h->abi->get_hotswap_state;
        if (!get_hotswap_state) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_hotswap_state(h->hnd, ResourceId, State);

        return rv;
}

SaErrorT SAHPI_API saHpiHotSwapActionRequest (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiHsActionT   Action)
{
        SaErrorT rv;
        SaErrorT (*request_hotswap_action)(void *hnd, SaHpiResourceIdT rid,
                                           SaHpiHsActionT act);

        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!oh_lookup_hsaction(Action)) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        request_hotswap_action = h->abi->request_hotswap_action;
        if (!request_hotswap_action) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = request_hotswap_action(h->hnd, ResourceId, Action);

        oh_get_events();

        return rv;
}

SaErrorT SAHPI_API saHpiHotSwapIndicatorStateGet (
        SAHPI_IN  SaHpiSessionIdT        SessionId,
        SAHPI_IN  SaHpiResourceIdT       ResourceId,
        SAHPI_OUT SaHpiHsIndicatorStateT *State)
{
        SaErrorT rv;
        SaErrorT (*get_indicator_state)(void *hnd, SaHpiResourceIdT id,
                                        SaHpiHsIndicatorStateT *state);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!State) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        if (!(res->HotSwapCapabilities & SAHPI_HS_CAPABILITY_INDICATOR_SUPPORTED)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        get_indicator_state = h->abi->get_indicator_state;
        if (!get_indicator_state) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_indicator_state(h->hnd, ResourceId, State);

        return rv;
}

SaErrorT SAHPI_API saHpiHotSwapIndicatorStateSet (
        SAHPI_IN SaHpiSessionIdT        SessionId,
        SAHPI_IN SaHpiResourceIdT       ResourceId,
        SAHPI_IN SaHpiHsIndicatorStateT State)
{
        SaErrorT rv;
        SaErrorT (*set_indicator_state)(void *hnd, SaHpiResourceIdT id,
                                        SaHpiHsIndicatorStateT state);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!oh_lookup_hsindicatorstate(State)) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        if (!(res->HotSwapCapabilities & SAHPI_HS_CAPABILITY_INDICATOR_SUPPORTED)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_indicator_state = h->abi->set_indicator_state;
        if (!set_indicator_state) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_indicator_state(h->hnd, ResourceId, State);

        return rv;
}

/*******************************************************************************
 *
 *  Configuration Function(s)
 *
 ******************************************************************************/

SaErrorT SAHPI_API saHpiParmControl (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiParmActionT Action)
{
        SaErrorT rv;
        SaErrorT (*control_parm)(void *, SaHpiResourceIdT, SaHpiParmActionT);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!oh_lookup_parmaction(Action)) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_CONFIGURATION)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        control_parm = h->abi->control_parm;
        if (!control_parm) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = control_parm(h->hnd, ResourceId, Action);

        return rv;
}

/*******************************************************************************
 *
 *  Reset Functions
 *
 ******************************************************************************/


SaErrorT SAHPI_API saHpiResourceResetStateGet (
        SAHPI_IN  SaHpiSessionIdT   SessionId,
        SAHPI_IN  SaHpiResourceIdT  ResourceId,
        SAHPI_OUT SaHpiResetActionT *ResetAction)
{
        SaErrorT rv;
        SaErrorT (*get_func)(void *, SaHpiResourceIdT, SaHpiResetActionT *);

        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!ResetAction) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_RESET)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        get_func = h->abi->get_reset_state;
        if (!get_func) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_func(h->hnd, ResourceId, ResetAction);

        return rv;
}

SaErrorT SAHPI_API saHpiResourceResetStateSet (
        SAHPI_IN SaHpiSessionIdT   SessionId,
        SAHPI_IN SaHpiResourceIdT  ResourceId,
        SAHPI_IN SaHpiResetActionT ResetAction)
{
        SaErrorT rv;
        SaErrorT (*set_func)(void *, SaHpiResourceIdT, SaHpiResetActionT);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!oh_lookup_resetaction(ResetAction)) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_RESET)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_func = h->abi->set_reset_state;
        if (!set_func) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_func(h->hnd, ResourceId, ResetAction);

        return rv;
}

/*******************************************************************************
 *
 *  Power Functions
 *
 ******************************************************************************/

SaErrorT SAHPI_API saHpiResourcePowerStateGet (
        SAHPI_IN  SaHpiSessionIdT  SessionId,
        SAHPI_IN  SaHpiResourceIdT ResourceId,
        SAHPI_OUT SaHpiPowerStateT *State)
{
        SaErrorT rv;
        SaErrorT (*get_power_state)(void *hnd, SaHpiResourceIdT id,
                                    SaHpiPowerStateT *state);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!State) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_POWER)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        get_power_state = h->abi->get_power_state;
        if (!get_power_state) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = get_power_state(h->hnd, ResourceId, State);

        return rv;
}

SaErrorT SAHPI_API saHpiResourcePowerStateSet (
        SAHPI_IN SaHpiSessionIdT  SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiPowerStateT State)
{
        SaErrorT rv;
        SaErrorT (*set_power_state)(void *hnd, SaHpiResourceIdT id,
                                    SaHpiPowerStateT state);
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        SaHpiDomainIdT did;
        struct oh_domain *d = NULL;

        if (!oh_lookup_powerstate(State)) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        OH_CHECK_INIT_STATE(SessionId);
        OH_GET_DID(SessionId, did);
        OH_GET_DOMAIN(did, d); /* Lock domain */
        OH_RESOURCE_GET(d, ResourceId, res);

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_POWER)) {
                oh_release_domain(d); /* Unlock domain */
                return SA_ERR_HPI_CAPABILITY;
        }

        OH_HANDLER_GET(d, ResourceId, h);
        oh_release_domain(d); /* Unlock domain */

        set_power_state = h->abi->set_power_state;
        if (!set_power_state) {
                return SA_ERR_HPI_INVALID_CMD;
        }

        rv = set_power_state(h->hnd, ResourceId, State);

        return rv;
}


