/*
 * marshaling/demarshaling of hpi functions
 *
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
 *     Thomas Kanngieser <thomas.kanngieser@fci.com>
 *     W. David Ashley <dashley@us.ibm.com.com>
 */

#include "marshal_hpi.h"
#include <stdio.h>


static const cMarshalType *saHpiVersionGetIn[] =
{
  0
};


static const cMarshalType *saHpiVersionGetOut[] =
{
  &SaHpiVersionType, // result (SaHpiVersionT)
  0
};


static const cMarshalType *saHpiSessionOpenIn[] =
{
  &SaHpiDomainIdType, // domain id (SaHpiDomainIdT)
  0
};


static const cMarshalType *saHpiSessionOpenOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  0
};


static const cMarshalType *saHpiSessionCloseIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  0
};


static const cMarshalType *saHpiSessionCloseOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiDiscoverIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  0
};


static const cMarshalType *saHpiDiscoverOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiDomainInfoGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  0
};


static const cMarshalType *saHpiDomainInfoGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiDomainInfoType,
  0
};


static const cMarshalType *saHpiDrtEntryGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiEntryIdType, // entry id (SaHpiEntryIdT)
  0
};


static const cMarshalType *saHpiDrtEntryGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiEntryIdType,
  &SaHpiDrtEntryType,
  0
};


static const cMarshalType *saHpiDomainTagSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiTextBufferType, // domain tag (SaHpiTextBufferT)
  0
};


static const cMarshalType *saHpiDomainTagSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiRptEntryGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiEntryIdType,
  0
};

static const cMarshalType *saHpiRptEntryGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiEntryIdType,
  &SaHpiRptEntryType,
  0
};


static const cMarshalType *saHpiRptEntryGetByResourceIdIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiRptEntryGetByResourceIdOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiRptEntryType,
  0
};


static const cMarshalType *saHpiResourceSeveritySetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiSeverityType,
  0
};

static const cMarshalType *saHpiResourceSeveritySetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiResourceTagSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiTextBufferType,
  0
};

static const cMarshalType *saHpiResourceTagSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiResourceIdGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  0
};

static const cMarshalType *saHpiResourceIdGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiResourceIdType,
  0
};


static const cMarshalType *saHpiEventLogInfoGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiEventLogInfoGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiEventLogInfoType,
  0
};


static const cMarshalType *saHpiEventLogEntryGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiEventLogEntryIdType,
  &SaHpiRdrType,
  &SaHpiRptEntryType,
  0
};

static const cMarshalType *saHpiEventLogEntryGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiEventLogEntryIdType,
  &SaHpiEventLogEntryIdType,
  &SaHpiEventLogEntryType,
  &SaHpiRdrType,
  &SaHpiRptEntryType,
  0
};


static const cMarshalType *saHpiEventLogEntryAddIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiEventType,
  0
};

static const cMarshalType *saHpiEventLogEntryAddOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiEventLogClearIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiEventLogClearOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiEventLogTimeGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiEventLogTimeGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiTimeType,
  0
};


static const cMarshalType *saHpiEventLogTimeSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiTimeType,
  0
};

static const cMarshalType *saHpiEventLogTimeSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiEventLogStateGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiEventLogStateGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiBoolType,
  0
};


static const cMarshalType *saHpiEventLogStateSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiBoolType,
  0
};

static const cMarshalType *saHpiEventLogStateSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiEventLogOverflowResetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiEventLogOverflowResetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiSubscribeIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  0
};

static const cMarshalType *saHpiSubscribeOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiUnsubscribeIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  0
};

static const cMarshalType *saHpiUnsubscribeOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiEventGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiTimeoutType,
  &SaHpiRdrType,
  &SaHpiRptEntryType,
  &SaHpiEvtQueueStatusType,
  0
};

static const cMarshalType *saHpiEventGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiEventType,
  &SaHpiRdrType,
  &SaHpiRptEntryType,
  &SaHpiEvtQueueStatusType,
  0
};


static const cMarshalType *saHpiEventAddIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiEventType,
  0
};

static const cMarshalType *saHpiEventAddOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiAlarmGetNextIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiSeverityType,
  &SaHpiBoolType,
  &SaHpiAlarmType,
  0
};

static const cMarshalType *saHpiAlarmGetNextOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiAlarmType,
  0
};


static const cMarshalType *saHpiAlarmGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiAlarmIdType,
  0
};

static const cMarshalType *saHpiAlarmGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiAlarmType,
  0
};


static const cMarshalType *saHpiAlarmAcknowledgeIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiAlarmIdType,
  &SaHpiSeverityType,
  0
};

static const cMarshalType *saHpiAlarmAcknowledgeOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiAlarmAddIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiAlarmType,
  0
};

static const cMarshalType *saHpiAlarmAddOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiAlarmType,
  0
};


static const cMarshalType *saHpiAlarmDeleteIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiAlarmIdType,
  &SaHpiSeverityType,
  0
};

static const cMarshalType *saHpiAlarmDeleteOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiRdrGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiEntryIdType,
  0
};

static const cMarshalType *saHpiRdrGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiEntryIdType,
  &SaHpiRdrType,
  0
};


static const cMarshalType *saHpiRdrGetByInstrumentIdIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiRdrTypeType,
  &SaHpiInstrumentIdType,
  0
};

static const cMarshalType *saHpiRdrGetByInstrumentIdOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiRdrType,
  0
};


static const cMarshalType *saHpiSensorReadingGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiSensorNumType,
  &SaHpiSensorReadingType,
  &SaHpiEventStateType,
  0
};

static const cMarshalType *saHpiSensorReadingGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiSensorReadingType,
  &SaHpiEventStateType,
  0
};


static const cMarshalType *saHpiSensorThresholdsGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiSensorNumType,
  0
};

static const cMarshalType *saHpiSensorThresholdsGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiSensorThresholdsType,
  0
};


static const cMarshalType *saHpiSensorThresholdsSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiSensorNumType,
  &SaHpiSensorThresholdsType,
  0
};

static const cMarshalType *saHpiSensorThresholdsSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiSensorTypeGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiSensorNumType,
  0
};

static const cMarshalType *saHpiSensorTypeGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiSensorTypeType,
  &SaHpiEventCategoryType,
  0
};


static const cMarshalType *saHpiSensorEnableGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiSensorNumType,
  0
};

static const cMarshalType *saHpiSensorEnableGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiBoolType,
  0
};


static const cMarshalType *saHpiSensorEnableSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiSensorNumType,
  &SaHpiBoolType,
  0
};

static const cMarshalType *saHpiSensorEnableSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiSensorEventEnableGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiSensorNumType,
  0
};

static const cMarshalType *saHpiSensorEventEnableGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiBoolType,
  0
};


static const cMarshalType *saHpiSensorEventEnableSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiSensorNumType,
  &SaHpiBoolType,
  0
};

static const cMarshalType *saHpiSensorEventEnableSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiSensorEventMasksGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiSensorNumType,
  &SaHpiEventStateType,
  &SaHpiEventStateType,
  0
};

static const cMarshalType *saHpiSensorEventMasksGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiEventStateType,
  &SaHpiEventStateType,
  0
};


static const cMarshalType *saHpiSensorEventMasksSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiSensorNumType,
  &SaHpiUint32Type,
  &SaHpiEventStateType,
  &SaHpiEventStateType,
  0
};

static const cMarshalType *saHpiSensorEventMasksSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiControlTypeGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiCtrlNumType,
  0
};

static const cMarshalType *saHpiControlTypeGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiCtrlTypeType,
  0
};


static const cMarshalType *saHpiControlGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiCtrlNumType,
  &SaHpiCtrlStateType,
  0
};

static const cMarshalType *saHpiControlGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiCtrlModeType,
  &SaHpiCtrlStateType,
  0
};


static const cMarshalType *saHpiControlSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiCtrlNumType,
  &SaHpiCtrlModeType,
  &SaHpiCtrlStateType,
  0
};

static const cMarshalType *saHpiControlSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiIdrInfoGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiIdrIdType,
  0
};

static const cMarshalType *saHpiIdrInfoGetOut[] =
{
  &SaErrorType,            // result (SaErrorT)
  &SaHpiIdrInfoType,
  0
};


static const cMarshalType *saHpiIdrAreaHeaderGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiIdrIdType,
  &SaHpiIdrAreaTypeType,
  &SaHpiEntryIdType,
  0
};

static const cMarshalType *saHpiIdrAreaHeaderGetOut[] =
{
  &SaErrorType,            // result (SaErrorT)
  &SaHpiEntryIdType,
  &SaHpiIdrAreaHeaderType,
  0
};


static const cMarshalType *saHpiIdrAreaAddIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiIdrIdType,
  &SaHpiIdrAreaTypeType,
  0
};

static const cMarshalType *saHpiIdrAreaAddOut[] =
{
  &SaErrorType,            // result (SaErrorT)
  &SaHpiEntryIdType,
  0
};


static const cMarshalType *saHpiIdrAreaDeleteIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiIdrIdType,
  &SaHpiEntryIdType,
  0
};

static const cMarshalType *saHpiIdrAreaDeleteOut[] =
{
  &SaErrorType,            // result (SaErrorT)
  0
};


static const cMarshalType *saHpiIdrFieldGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiIdrIdType,
  &SaHpiEntryIdType,
  &SaHpiIdrFieldTypeType,
  &SaHpiEntryIdType,
  0
};

static const cMarshalType *saHpiIdrFieldGetOut[] =
{
  &SaErrorType,            // result (SaErrorT)
  &SaHpiEntryIdType,
  &SaHpiIdrFieldTypeType,
  0
};


static const cMarshalType *saHpiIdrFieldAddIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiIdrIdType,
  &SaHpiIdrFieldTypeType,
  0
};

static const cMarshalType *saHpiIdrFieldAddOut[] =
{
  &SaErrorType,            // result (SaErrorT)
  &SaHpiIdrFieldTypeType,
  0
};


static const cMarshalType *saHpiIdrFieldSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiIdrIdType,
  &SaHpiIdrFieldTypeType,
  0
};

static const cMarshalType *saHpiIdrFieldSetOut[] =
{
  &SaErrorType,            // result (SaErrorT)
  0
};


static const cMarshalType *saHpiIdrFieldDeleteIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiIdrIdType,
  &SaHpiEntryIdType,
  &SaHpiEntryIdType,
  0
};

static const cMarshalType *saHpiIdrFieldDeleteOut[] =
{
  &SaErrorType,            // result (SaErrorT)
  0
};


static const cMarshalType *saHpiWatchdogTimerGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiWatchdogNumType,
  0
};

static const cMarshalType *saHpiWatchdogTimerGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiWatchdogType,
  0
};


static const cMarshalType *saHpiWatchdogTimerSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiWatchdogNumType,
  &SaHpiWatchdogType,
  0
};

static const cMarshalType *saHpiWatchdogTimerSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiWatchdogTimerResetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiWatchdogNumType,
  0
};

static const cMarshalType *saHpiWatchdogTimerResetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiAnnunciatorGetNextIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiAnnunciatorNumType,
  &SaHpiSeverityType,
  &SaHpiBoolType,
  &SaHpiAnnouncementType,
  0
};

static const cMarshalType *saHpiAnnunciatorGetNextOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiAnnouncementType,
  0
};


static const cMarshalType *saHpiAnnunciatorGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiAnnunciatorNumType,
  &SaHpiEntryIdType,
  0
};

static const cMarshalType *saHpiAnnunciatorGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiAnnouncementType,
  0
};


static const cMarshalType *saHpiAnnunciatorAcknowledgeIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiAnnunciatorNumType,
  &SaHpiEntryIdType,
  &SaHpiSeverityType,
  0
};

static const cMarshalType *saHpiAnnunciatorAcknowledgeOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiAnnunciatorAddIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiAnnunciatorNumType,
  &SaHpiAnnouncementType,
  0
};

static const cMarshalType *saHpiAnnunciatorAddOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiAnnouncementType,
  0
};


static const cMarshalType *saHpiAnnunciatorDeleteIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiAnnunciatorNumType,
  &SaHpiEntryIdType,
  &SaHpiSeverityType,
  0
};

static const cMarshalType *saHpiAnnunciatorDeleteOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiAnnunciatorModeGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiAnnunciatorNumType,
  0
};

static const cMarshalType *saHpiAnnunciatorModeGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiAnnunciatorModeType,
  0
};


static const cMarshalType *saHpiAnnunciatorModeSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiAnnunciatorNumType,
  &SaHpiAnnunciatorModeType,
  0
};

static const cMarshalType *saHpiAnnunciatorModeSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiHotSwapPolicyCancelIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiHotSwapPolicyCancelOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiResourceActiveSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiResourceActiveSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiResourceInactiveSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiResourceInactiveSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiAutoInsertTimeoutGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  0
};

static const cMarshalType *saHpiAutoInsertTimeoutGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiTimeoutType,
  0
};


static const cMarshalType *saHpiAutoInsertTimeoutSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiTimeoutType,
  0
};

static const cMarshalType *saHpiAutoInsertTimeoutSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiAutoExtractTimeoutGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiAutoExtractTimeoutGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiTimeoutType,
  0
};


static const cMarshalType *saHpiAutoExtractTimeoutSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiTimeoutType,
  0
};

static const cMarshalType *saHpiAutoExtractTimeoutSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiHotSwapStateGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiHotSwapStateGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiHsStateType,
  0
};


static const cMarshalType *saHpiHotSwapActionRequestIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiHsActionType,
  0
};

static const cMarshalType *saHpiHotSwapActionRequestOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiHotSwapIndicatorStateGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiHotSwapIndicatorStateGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiHsIndicatorStateType,
  0
};


static const cMarshalType *saHpiHotSwapIndicatorStateSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiHsIndicatorStateType,
  0
};

static const cMarshalType *saHpiHotSwapIndicatorStateSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiParmControlIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiParmActionType,
  0
};

static const cMarshalType *saHpiParmControlOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiResourceResetStateGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiResourceResetStateGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiResetActionType,
  0
};


static const cMarshalType *saHpiResourceResetStateSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiResetActionType,
  0
};

static const cMarshalType *saHpiResourceResetStateSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};


static const cMarshalType *saHpiResourcePowerStateGetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  0
};

static const cMarshalType *saHpiResourcePowerStateGetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiPowerStateType,
  0
};


static const cMarshalType *saHpiResourcePowerStateSetIn[] =
{
  &SaHpiSessionIdType, // session id (SaHpiSessionIdT)
  &SaHpiResourceIdType,
  &SaHpiPowerStateType,
  0
};

static const cMarshalType *saHpiResourcePowerStateSetOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};

static const cMarshalType *oHpiPluginLoadIn[] =
{
  &SaHpiTextBufferType, // plugin name (SaHpiTextBufferT)
  0
};


static const cMarshalType *oHpiPluginLoadOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};

static const cMarshalType *oHpiPluginUnloadIn[] =
{
  &SaHpiTextBufferType, // plugin name (SaHpiTextBufferT)
  0
};


static const cMarshalType *oHpiPluginUnloadOut[] =
{
  &SaErrorType, // result (SaErrorT)
  0
};

static const cMarshalType *oHpiPluginInfoIn[] =
{
  &SaHpiTextBufferType, // plugin name (SaHpiTextBufferT)
  0
};


static const cMarshalType *oHpiPluginInfoOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &oHpiPluginInfoType,
  0
};

static const cMarshalType *oHpiPluginGetNextIn[] =
{
  &SaHpiTextBufferType, // plugin name (SaHpiTextBufferT)
  0
};


static const cMarshalType *oHpiPluginGetNextOut[] =
{
  &SaErrorType, // result (SaErrorT)
  &SaHpiTextBufferType, // next plugin name (SaHpiTextBufferT)
  0
};


static cHpiMarshal hpi_marshal[] =
{
  dHpiMarshalEntry( saHpiVersionGet ),
  dHpiMarshalEntry( saHpiSessionOpen ),
  dHpiMarshalEntry( saHpiSessionClose ),
  dHpiMarshalEntry( saHpiDiscover ),
  dHpiMarshalEntry( saHpiDomainInfoGet ),
  dHpiMarshalEntry( saHpiDrtEntryGet ),
  dHpiMarshalEntry( saHpiDomainTagSet ),
  dHpiMarshalEntry( saHpiRptEntryGet ),
  dHpiMarshalEntry( saHpiRptEntryGetByResourceId ),
  dHpiMarshalEntry( saHpiResourceSeveritySet ),
  dHpiMarshalEntry( saHpiResourceTagSet ),
  dHpiMarshalEntry( saHpiResourceIdGet ),
  dHpiMarshalEntry( saHpiEventLogInfoGet ),
  dHpiMarshalEntry( saHpiEventLogEntryGet ),
  dHpiMarshalEntry( saHpiEventLogEntryAdd ),
  dHpiMarshalEntry( saHpiEventLogClear ),
  dHpiMarshalEntry( saHpiEventLogTimeGet ),
  dHpiMarshalEntry( saHpiEventLogTimeSet ),
  dHpiMarshalEntry( saHpiEventLogStateGet ),
  dHpiMarshalEntry( saHpiEventLogStateSet ),
  dHpiMarshalEntry( saHpiEventLogOverflowReset ),
  dHpiMarshalEntry( saHpiSubscribe ),
  dHpiMarshalEntry( saHpiUnsubscribe ),
  dHpiMarshalEntry( saHpiEventGet ),
  dHpiMarshalEntry( saHpiEventAdd ),
  dHpiMarshalEntry( saHpiAlarmGetNext ),
  dHpiMarshalEntry( saHpiAlarmGet ),
  dHpiMarshalEntry( saHpiAlarmAcknowledge ),
  dHpiMarshalEntry( saHpiAlarmAdd ),
  dHpiMarshalEntry( saHpiAlarmDelete ),
  dHpiMarshalEntry( saHpiRdrGet ),
  dHpiMarshalEntry( saHpiRdrGetByInstrumentId ),
  dHpiMarshalEntry( saHpiSensorReadingGet ),
  dHpiMarshalEntry( saHpiSensorThresholdsGet ),
  dHpiMarshalEntry( saHpiSensorThresholdsSet ),
  dHpiMarshalEntry( saHpiSensorTypeGet ),
  dHpiMarshalEntry( saHpiSensorEnableGet ),
  dHpiMarshalEntry( saHpiSensorEnableSet ),
  dHpiMarshalEntry( saHpiSensorEventEnableGet ),
  dHpiMarshalEntry( saHpiSensorEventEnableSet ),
  dHpiMarshalEntry( saHpiSensorEventMasksGet ),
  dHpiMarshalEntry( saHpiSensorEventMasksSet ),
  dHpiMarshalEntry( saHpiControlTypeGet ),
  dHpiMarshalEntry( saHpiControlGet ),
  dHpiMarshalEntry( saHpiControlSet ),
  dHpiMarshalEntry( saHpiIdrInfoGet ),
  dHpiMarshalEntry( saHpiIdrAreaHeaderGet ),
  dHpiMarshalEntry( saHpiIdrAreaAdd ),
  dHpiMarshalEntry( saHpiIdrAreaDelete ),
  dHpiMarshalEntry( saHpiIdrFieldGet ),
  dHpiMarshalEntry( saHpiIdrFieldAdd ),
  dHpiMarshalEntry( saHpiIdrFieldSet ),
  dHpiMarshalEntry( saHpiIdrFieldDelete ),
  dHpiMarshalEntry( saHpiWatchdogTimerGet ),
  dHpiMarshalEntry( saHpiWatchdogTimerSet ),
  dHpiMarshalEntry( saHpiWatchdogTimerReset ),
  dHpiMarshalEntry( saHpiAnnunciatorGetNext ),
  dHpiMarshalEntry( saHpiAnnunciatorGet ),
  dHpiMarshalEntry( saHpiAnnunciatorAcknowledge ),
  dHpiMarshalEntry( saHpiAnnunciatorAdd ),
  dHpiMarshalEntry( saHpiAnnunciatorDelete ),
  dHpiMarshalEntry( saHpiAnnunciatorModeGet ),
  dHpiMarshalEntry( saHpiAnnunciatorModeSet ),
  dHpiMarshalEntry( saHpiHotSwapPolicyCancel ),
  dHpiMarshalEntry( saHpiResourceActiveSet ),
  dHpiMarshalEntry( saHpiResourceInactiveSet ),
  dHpiMarshalEntry( saHpiAutoInsertTimeoutGet ),
  dHpiMarshalEntry( saHpiAutoInsertTimeoutSet ),
  dHpiMarshalEntry( saHpiAutoExtractTimeoutGet ),
  dHpiMarshalEntry( saHpiAutoExtractTimeoutSet ),
  dHpiMarshalEntry( saHpiHotSwapStateGet ),
  dHpiMarshalEntry( saHpiHotSwapActionRequest ),
  dHpiMarshalEntry( saHpiHotSwapIndicatorStateGet ),
  dHpiMarshalEntry( saHpiHotSwapIndicatorStateSet ),
  dHpiMarshalEntry( saHpiParmControl ),
  dHpiMarshalEntry( saHpiResourceResetStateGet ),
  dHpiMarshalEntry( saHpiResourceResetStateSet ),
  dHpiMarshalEntry( saHpiResourcePowerStateGet ),
  dHpiMarshalEntry( saHpiResourcePowerStateSet ),
  dHpiMarshalEntry( oHpiPluginLoad ),
  dHpiMarshalEntry( oHpiPluginUnload ),
  dHpiMarshalEntry( oHpiPluginInfo ),
  dHpiMarshalEntry( oHpiPluginGetNext ),
};


static int hpi_marshal_num = sizeof( hpi_marshal ) / sizeof( cHpiMarshal );

static int hpi_marshal_init = 0;


cHpiMarshal *
HpiMarshalFind( int id )
{
  if ( !hpi_marshal_init )
     {
       int i;

       for( i = 0; i < hpi_marshal_num; i++ )
	  {
//          printf("Entry %d\n", i);
	    hpi_marshal[i].m_request_len = MarshalSizeArray( hpi_marshal[i].m_request );
	    hpi_marshal[i].m_reply_len   = MarshalSizeArray( hpi_marshal[i].m_reply );
	  }

       hpi_marshal_init = 1;
     }

  id--;

  if ( id < 0 || id >= hpi_marshal_num )
       return 0;

  return &hpi_marshal[id];
}


int
HpiMarshalRequest( cHpiMarshal *m, void *buffer, const void **param )
{
  return MarshalArray( m->m_request, param, buffer );
}


int
HpiMarshalRequest1( cHpiMarshal *m, void *buffer, const void *p1 )
{
  const void *param[1];
  param[0] = p1;

  return HpiMarshalRequest( m, buffer, param );
}


int
HpiMarshalRequest2( cHpiMarshal *m, void *buffer, const void *p1, const void *p2  )
{
  const void *param[2];
  param[0] = p1;
  param[1] = p2;

  return HpiMarshalRequest( m, buffer, param );
}


int
HpiMarshalRequest3( cHpiMarshal *m, void *buffer, const void *p1, const void *p2, const void *p3 )
{
  const void *param[3];
  param[0] = p1;
  param[1] = p2;
  param[2] = p3;

  return HpiMarshalRequest( m, buffer, param );
}


int
HpiMarshalRequest4( cHpiMarshal *m, void *buffer, const void *p1, const void *p2,
		    const void *p3, const void *p4 )
{
  const void *param[4];
  param[0] = p1;
  param[1] = p2;
  param[2] = p3;
  param[3] = p4;

  return HpiMarshalRequest( m, buffer, param );
}


int
HpiMarshalRequest5( cHpiMarshal *m, void *buffer, const void *p1, const void *p2,
		    const void *p3, const void *p4, const void *p5 )
{
  const void *param[5];
  param[0] = p1;
  param[1] = p2;
  param[2] = p3;
  param[3] = p4;
  param[4] = p5;

  return HpiMarshalRequest( m, buffer, param );
}


int
HpiMarshalRequest6( cHpiMarshal *m, void *buffer, const void *p1, const void *p2,
		    const void *p3, const void *p4, const void *p5, const void *p6 )
{
  const void *param[6];
  param[0] = p1;
  param[1] = p2;
  param[2] = p3;
  param[3] = p4;
  param[4] = p5;
  param[5] = p6;

  return HpiMarshalRequest( m, buffer, param );
}


int
HpiDemarshalRequest( int byte_order, cHpiMarshal *m, const void *buffer, void **params )
{
  return DemarshalArray( byte_order, m->m_request, params, buffer );
}


int
HpiDemarshalRequest1( int byte_order, cHpiMarshal *m, const void *buffer, void *p1 )
{
  void *param[1];
  param[0] = p1;

  return HpiDemarshalRequest( byte_order, m, buffer, param );
}


int
HpiDemarshalRequest2( int byte_order, cHpiMarshal *m, const void *buffer, void *p1, void *p2 )
{
  void *param[2];
  param[0] = p1;
  param[1] = p2;

  return HpiDemarshalRequest( byte_order, m, buffer, param );
}


int
HpiDemarshalRequest3( int byte_order, cHpiMarshal *m, const void *buffer, void *p1, void *p2, void *p3 )
{
  void *param[3];
  param[0] = p1;
  param[1] = p2;
  param[2] = p3;

  return HpiDemarshalRequest( byte_order, m, buffer, param );
}


int
HpiDemarshalRequest4( int byte_order, cHpiMarshal *m, const void *buffer, void *p1, void *p2, void *p3, void *p4 )
{
  void *param[4];
  param[0] = p1;
  param[1] = p2;
  param[2] = p3;
  param[3] = p4;

  return HpiDemarshalRequest( byte_order, m, buffer, param );
}


int
HpiDemarshalRequest5( int byte_order, cHpiMarshal *m, const void *buffer, void *p1, void *p2, void *p3, void *p4, void *p5 )
{
  void *param[5];
  param[0] = p1;
  param[1] = p2;
  param[2] = p3;
  param[3] = p4;
  param[4] = p5;

  return HpiDemarshalRequest( byte_order, m, buffer, param );
}


int
HpiDemarshalRequest6( int byte_order, cHpiMarshal *m, const void *buffer, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6 )
{
  void *param[6];
  param[0] = p1;
  param[1] = p2;
  param[2] = p3;
  param[3] = p4;
  param[4] = p5;
  param[5] = p6;

  return HpiDemarshalRequest( byte_order, m, buffer, param );
}


int
HpiMarshalReply( cHpiMarshal *m, void *buffer, const void **params )
{
  // the first value is the result.
  SaErrorT err = *(const SaErrorT *)params[0];

  if ( err == SA_OK )
       return MarshalArray( m->m_reply, params, buffer );

  return Marshal( &SaErrorType, &err, buffer );
}


int
HpiMarshalReply0( cHpiMarshal *m, void *buffer, const void *result )
{
  const void *param[1];
  param[0] = result;

  return HpiMarshalReply( m, buffer, param );
}


int
HpiMarshalReply1( cHpiMarshal *m, void *buffer, const void *result, const void *p1  )
{
  const void *param[2];
  param[0] = result;
  param[1] = p1;

  return HpiMarshalReply( m, buffer, param );
}


int
HpiMarshalReply2( cHpiMarshal *m, void *buffer, const void *result,
		  const void *p1, const void *p2 )
{
  const void *param[3];
  param[0] = result;
  param[1] = p1;
  param[2] = p2;

  return HpiMarshalReply( m, buffer, param );
}


int
HpiMarshalReply3( cHpiMarshal *m, void *buffer, const void *result, const void *p1,
		  const void *p2, const void *p3 )
{
  const void *param[4];
  param[0] = result;
  param[1] = p1;
  param[2] = p2;
  param[3] = p3;

  return HpiMarshalReply( m, buffer, param );
}


int
HpiMarshalReply4( cHpiMarshal *m, void *buffer, const void *result, const void *p1,
		  const void *p2, const void *p3, const void *p4 )
{
  const void *param[5];
  param[0] = result;
  param[1] = p1;
  param[2] = p2;
  param[3] = p3;
  param[4] = p4;

  return HpiMarshalReply( m, buffer, param );
}


int
HpiMarshalReply5( cHpiMarshal *m, void *buffer, const void *result, const void *p1,
		  const void *p2, const void *p3, const void *p4, const void *p5 )
{
  const void *param[6];
  param[0] = result;
  param[1] = p1;
  param[2] = p2;
  param[3] = p3;
  param[4] = p4;
  param[5] = p5;

  return HpiMarshalReply( m, buffer, param );
}


int
HpiDemarshalReply( int byte_order, cHpiMarshal *m, const void *buffer, void **params )
{
  // the first value is the error code
  int rv = Demarshal( byte_order, &SaErrorType, params[0], buffer );

  if ( rv < 0 )
       return -1;

  SaErrorT err = *(SaErrorT *)params[0];

  if ( err == SA_OK )
       return DemarshalArray( byte_order, m->m_reply, params, buffer );

  return rv;
}


int
HpiDemarshalReply0( int byte_order, cHpiMarshal *m, const void *buffer, void *result )
{
  void *param[1];
  param[0] = result;

  return HpiDemarshalReply( byte_order, m, buffer, param );
}


int
HpiDemarshalReply1( int byte_order, cHpiMarshal *m, const void *buffer, void *result, void *p1 )
{
  void *param[2];
  param[0] = result;
  param[1] = p1;

  return HpiDemarshalReply( byte_order, m, buffer, param );
}


int
HpiDemarshalReply2( int byte_order, cHpiMarshal *m, const void *buffer, void *result, void *p1, void *p2 )
{
  void *param[3];
  param[0] = result;
  param[1] = p1;
  param[2] = p2;

  return HpiDemarshalReply( byte_order, m, buffer, param );
}


int
HpiDemarshalReply3( int byte_order, cHpiMarshal *m, const void *buffer, void *result, void *p1, void *p2, void *p3 )
{
  void *param[4];
  param[0] = result;
  param[1] = p1;
  param[2] = p2;
  param[3] = p3;

  return HpiDemarshalReply( byte_order, m, buffer, param );
}


int
HpiDemarshalReply4( int byte_order, cHpiMarshal *m, const void *buffer, void *result, void *p1, void *p2, void *p3, void *p4 )
{
  void *param[5];
  param[0] = result;
  param[1] = p1;
  param[2] = p2;
  param[3] = p3;
  param[4] = p4;

  return HpiDemarshalReply( byte_order, m, buffer, param );
}


int
HpiDemarshalReply5( int byte_order, cHpiMarshal *m, const void *buffer, void *result, void *p1, void *p2, void *p3, void *p4, void *p5 )
{
  void *param[6];
  param[0] = result;
  param[1] = p1;
  param[2] = p2;
  param[3] = p3;
  param[4] = p4;
  param[5] = p5;

  return HpiDemarshalReply( byte_order, m, buffer, param );
}
