/* BSD License
 * Copyright (C) by Intel Crop.
 * Author: Louis Zhuang <louis.zhuang@linux.intel.com>
 */

#include <string.h>

#include <openhpi.h>
#include <oh_plugin.h>
#include <SaHpi.h>

static enum {
	OH_STAT_UNINIT,
	OH_STAT_READY,
	OH_STAT_FINAL
} oh_hpi_state = OH_STAT_UNINIT;
static const int entry_id_offset = 1000;

#define OH_STATE_READY_CHECK 					\
	do {							\
		if (OH_STAT_READY!=oh_hpi_state) {		\
			dbg("Uninitialized HPI");		\
			return SA_ERR_HPI_UNINITIALIZED;	\
		}						\
	}while(0)						

SaErrorT SAHPI_API saHpiInitialize(SAHPI_OUT SaHpiVersionT *HpiImplVersion)
{
	if (OH_STAT_UNINIT!=oh_hpi_state) {
		dbg("Cannot initialize twice");
		return SA_ERR_HPI_DUPLICATE;
	}
	
	init_session();
	init_domain();
	if (init_plugin()<0) {
		dbg("Can not load/init plugin");
		return SA_ERR_HPI_NOT_PRESENT;
	}
	
	oh_hpi_state= OH_STAT_READY;
	return SA_OK;
}

SaErrorT SAHPI_API saHpiFinalize(void)
{
	struct oh_domain *d;
	
	OH_STATE_READY_CHECK;	

	d = domain_get(SAHPI_DEFAULT_DOMAIN_ID);
	if (!d) {
		dbg("No DEFAULT DOMAIN");
		return SA_ERR_HPI_UNKNOWN;
	}

	d->abi->close(d->hnd);
	
	uninit_domain();
	uninit_session();

	oh_hpi_state = OH_STAT_FINAL;
	return SA_OK;
}

SaErrorT SAHPI_API saHpiSessionOpen(
		SAHPI_IN SaHpiDomainIdT DomainId,
		SAHPI_OUT SaHpiSessionIdT *SessionId,
		SAHPI_IN void *SecurityParams)
{
	struct oh_domain  *d;
	struct oh_session *s;
	int rv;
	
	OH_STATE_READY_CHECK;
	
	d = domain_get(DomainId);
	if (!d) {
		dbg("Invalid domain");
		return SA_ERR_HPI_INVALID_DOMAIN;
	}

	rv = session_add(d, &s);
	if (rv<0) {
		dbg("Out of space");
		return SA_ERR_HPI_OUT_OF_SPACE;
	}	
	*SessionId = s->sid;
	
	return SA_OK;
}


SaErrorT SAHPI_API saHpiSessionClose(SAHPI_IN SaHpiSessionIdT SessionId)
{
	struct oh_session *s;
	
	OH_STATE_READY_CHECK;
	
	s = session_get(SessionId);
	if (!s) {
		dbg("Invalid session");
		return SA_ERR_HPI_INVALID_SESSION;
	}

	session_del(s);	
	return SA_OK;
}


SaErrorT SAHPI_API saHpiResourcesDiscover(SAHPI_IN SaHpiSessionIdT SessionId)
{
	struct oh_session *s;
	int rv =0;
	
	OH_STATE_READY_CHECK;

	s = session_get(SessionId);
	if (!s) {
		dbg("Invalid session");
		return SA_ERR_HPI_INVALID_SESSION;
	}
	
	rv = session_get_events(s);
	if (rv<0) {
		dbg("Error when polling");
		return SA_ERR_HPI_UNKNOWN;
	}

	return SA_OK;
}

SaErrorT SAHPI_API saHpiRptInfoGet(
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_OUT SaHpiRptInfoT *RptInfo)
{
	struct oh_session *s;
	struct oh_domain  *d;
	struct timeval 	*upt;
	
	OH_STATE_READY_CHECK;

	s = session_get(SessionId);
	if (!s) {
		dbg("Invalid session");
		return SA_ERR_HPI_INVALID_SESSION;
	}
	
	d   = s->domain;
	upt = &d->update_time; 
	RptInfo->UpdateCount 	= d->update_counter;
	RptInfo->UpdateTimestamp= (SaHpiTimeT)upt->tv_sec*1000000000 + upt->tv_usec*1000;
	return SA_OK;
}

SaErrorT SAHPI_API saHpiRptEntryGet(
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiEntryIdT EntryId,
		SAHPI_OUT SaHpiEntryIdT *NextEntryId,
		SAHPI_OUT SaHpiRptEntryT *RptEntry)
{
	int n;
	int i;
	struct list_head  *tmp;
	struct oh_resource *r;
	struct oh_session *s;
	struct oh_domain  *d;
	
	OH_STATE_READY_CHECK;

	s = session_get(SessionId);
	if (!s) {
		dbg("Invalid session");
		return SA_ERR_HPI_INVALID_SESSION;
	}
	
	d   = s->domain;
	
	switch (EntryId) {
	case SAHPI_FIRST_ENTRY:
		n = 0;
		break;
	default:
		n = EntryId - entry_id_offset;
		break;
	}
	
	i = 0;
	r = NULL;
	list_for_each(tmp, &d->res_list) {
		if (i==n) {
			r = list_container(tmp, struct oh_resource, node);
			break;
		}
		i++;
	}
	
	if (!r) {
		dbg("Invalid EntryId");
		return SA_ERR_HPI_INVALID;
	}
	
	memcpy(RptEntry, &r->entry, sizeof(*RptEntry));

	if (r->node.next == &d->res_list) { //last entry
		*NextEntryId = SAHPI_LAST_ENTRY;
	} else {
		*NextEntryId = n+1+entry_id_offset;
	}
	return SA_OK;
}

SaErrorT SAHPI_API saHpiRptEntryGetByResourceId(
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_OUT SaHpiRptEntryT *RptEntry)
{
	struct oh_session *s;
	struct oh_domain  *d;
	struct oh_resource *res;
	
	OH_STATE_READY_CHECK;

	s = session_get(SessionId);
	if (!s) {
		dbg("Invalid session");
		return SA_ERR_HPI_INVALID_SESSION;
	}
	
	d   = s->domain;
	
	res = get_resource(d, ResourceId);
	if (!res) {
		dbg("Invalid resource");
		return SA_ERR_HPI_INVALID_RESOURCE;
	}

	memcpy(RptEntry, &res->entry, sizeof(*RptEntry));

	return SA_OK;
}

SaErrorT SAHPI_API saHpiResourceSeveritySet(
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiSeverityT Severity)
{
	struct oh_session *s;
	struct oh_domain  *d;
	struct oh_resource *res;
	
	OH_STATE_READY_CHECK;

	s = session_get(SessionId);
	if (!s) {
		dbg("Invalid session");
		return SA_ERR_HPI_INVALID_SESSION;
	}
	
	d   = s->domain;
	
	res = get_resource(d, ResourceId);
	if (!res) {
		dbg("Invalid resource");
		return SA_ERR_HPI_INVALID_RESOURCE;
	}
	
	res->entry.ResourceSeverity = Severity;	
	return SA_OK;
}

SaErrorT SAHPI_API saHpiResourceTagSet(
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiTextBufferT *ResourceTag)
{
	struct oh_session *s;
	struct oh_domain  *d;
	struct oh_resource *res;
	
	OH_STATE_READY_CHECK;

	s = session_get(SessionId);
	if (!s) {
		dbg("Invalid session");
		return SA_ERR_HPI_INVALID_SESSION;
	}
	
	d   = s->domain;
	
	res = get_resource(d, ResourceId);
	if (!res) {
		dbg("Invalid resource");
		return SA_ERR_HPI_INVALID_RESOURCE;
	}
	
	memcpy(&res->entry.ResourceTag, ResourceTag, sizeof(res->entry.ResourceTag));	
	return SA_OK;
}

SaErrorT SAHPI_API saHpiResourceIdGet(
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_OUT SaHpiResourceIdT *ResourceId)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiEntitySchemaGet(
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_OUT SaHpiUint32T *SchemaId)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}


SaErrorT SAHPI_API saHpiEventLogInfoGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_OUT SaHpiSelInfoT *Info)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiEventLogEntryGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiSelEntryIdT EntryId,
		SAHPI_OUT SaHpiSelEntryIdT *PrevEntryId,
		SAHPI_OUT SaHpiSelEntryIdT *NextEntryId,
		SAHPI_OUT SaHpiSelEntryT *EventLogEntry,
		SAHPI_INOUT SaHpiRdrT *Rdr,
		SAHPI_INOUT SaHpiRptEntryT *RptEntry)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiEventLogEntryAdd (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiSelEntryT *EvtEntry)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiEventLogEntryDelete (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiSelEntryIdT EntryId)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiEventLogClear (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiEventLogTimeGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_OUT SaHpiTimeT *Time)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiEventLogTimeSet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiTimeT Time)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiEventLogStateGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_OUT SaHpiBoolT *Enable)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiEventLogStateSet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiBoolT Enable)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiSubscribe (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiBoolT ProvideActiveAlarms)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}


SaErrorT SAHPI_API saHpiUnsubscribe (
		SAHPI_IN SaHpiSessionIdT SessionId)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiEventGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiTimeoutT Timeout,
		SAHPI_OUT SaHpiEventT *Event,
		SAHPI_INOUT SaHpiRdrT *Rdr,
		SAHPI_INOUT SaHpiRptEntryT *RptEntry)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiRdrGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiEntryIdT EntryId,
		SAHPI_OUT SaHpiEntryIdT *NextEntryId,
		SAHPI_OUT SaHpiRdrT *Rdr)
{
	struct oh_session *s;
	struct oh_domain  *d;
	struct oh_resource *res;
	struct oh_rdr	  *rdr;
	
	int n;
	int i;
	struct list_head  *tmp;
	
	OH_STATE_READY_CHECK;

	s = session_get(SessionId);
	if (!s) {
		dbg("Invalid session");
		return SA_ERR_HPI_INVALID_SESSION;
	}
	
	d = s->domain;
	
	res = get_resource(d, ResourceId);
	if (!res) {
		dbg("Invalid resource");
		return SA_ERR_HPI_INVALID_RESOURCE;
	}
	
	switch (EntryId) {
	case SAHPI_FIRST_ENTRY:
		n = 0;
		break;
	default:
		n = EntryId - entry_id_offset;
		break;
	}
	
	i = 0;
	rdr = NULL;
	list_for_each(tmp, &res->rdr_list) {
		if (i==n) {
			rdr = list_container(tmp, struct oh_rdr, node);
			break;
		}
		i++;
	}
	
	if (!rdr) {
		dbg("Invalid EntryId");
		return SA_ERR_HPI_INVALID;
	}
	
	memcpy(Rdr, &rdr->rdr, sizeof(*Rdr));

	if (rdr->node.next == &res->rdr_list) { //last entry
		*NextEntryId = SAHPI_LAST_ENTRY;
	} else {
		*NextEntryId = n+1+entry_id_offset;
	}
	return SA_OK;
}

SaErrorT SAHPI_API saHpiSensorReadingGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiSensorNumT SensorNum,
		SAHPI_OUT SaHpiSensorReadingT *Reading)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiSensorReadingConvert (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiSensorNumT SensorNum,
		SAHPI_IN SaHpiSensorReadingT *ReadingInput,
		SAHPI_OUT SaHpiSensorReadingT *ConvertedReading)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiSensorThresholdsGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiSensorNumT SensorNum,
		SAHPI_OUT SaHpiSensorThresholdsT *SensorThresholds)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiSensorThresholdsSet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiSensorNumT SensorNum,
		SAHPI_IN SaHpiSensorThresholdsT *SensorThresholds)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiSensorTypeGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiSensorNumT SensorNum,
		SAHPI_OUT SaHpiSensorTypeT *Type,
		SAHPI_OUT SaHpiEventCategoryT *Category)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiSensorEventEnablesGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiSensorNumT SensorNum,
		SAHPI_OUT SaHpiSensorEvtEnablesT *Enables)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiSensorEventEnablesSet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiSensorNumT SensorNum,
		SAHPI_IN SaHpiSensorEvtEnablesT *Enables)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiControlTypeGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiCtrlNumT CtrlNum,
		SAHPI_OUT SaHpiCtrlTypeT *Type)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiControlStateGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiCtrlNumT CtrlNum,
		SAHPI_INOUT SaHpiCtrlStateT *CtrlState)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiControlStateSet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiCtrlNumT CtrlNum,
		SAHPI_IN SaHpiCtrlStateT *CtrlState)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiEntityInventoryDataRead (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiEirIdT EirId,
		SAHPI_IN SaHpiUint32T BufferSize,
		SAHPI_OUT SaHpiInventoryDataT *InventData,
		SAHPI_OUT SaHpiUint32T *ActualSize)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiEntityInventoryDataWrite (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiEirIdT EirId,
		SAHPI_IN SaHpiInventoryDataT *InventData)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiWatchdogTimerGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiWatchdogNumT WatchdogNum,
		SAHPI_OUT SaHpiWatchdogT *Watchdog)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiWatchdogTimerSet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiWatchdogNumT WatchdogNum,
		SAHPI_IN SaHpiWatchdogT *Watchdog)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiWatchdogTimerReset (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiWatchdogNumT WatchdogNum)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiHotSwapControlRequest (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiResourceActiveSet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiResourceInactiveSet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiAutoInsertTimeoutGet(
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_OUT SaHpiTimeoutT *Timeout)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiAutoInsertTimeoutSet(
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiTimeoutT Timeout)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiAutoExtractTimeoutGet(
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_OUT SaHpiTimeoutT *Timeout)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiAutoExtractTimeoutSet(
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiTimeoutT Timeout)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiHotSwapStateGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_OUT SaHpiHsStateT *State)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiHotSwapActionRequest (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiHsActionT Action)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiResourcePowerStateGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_OUT SaHpiHsPowerStateT *State)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiResourcePowerStateSet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiHsPowerStateT State)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiHotSwapIndicatorStateGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_OUT SaHpiHsIndicatorStateT *State)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiHotSwapIndicatorStateSet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiHsIndicatorStateT State)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiParmControl (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiParmActionT Action)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiResourceResetStateGet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_OUT SaHpiResetActionT *ResetAction)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}

SaErrorT SAHPI_API saHpiResourceResetStateSet (
		SAHPI_IN SaHpiSessionIdT SessionId,
		SAHPI_IN SaHpiResourceIdT ResourceId,
		SAHPI_IN SaHpiResetActionT ResetAction)
{
	return SA_ERR_HPI_UNSUPPORTED_API;
}
