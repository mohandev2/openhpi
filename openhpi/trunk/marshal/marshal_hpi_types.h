/*
 * marshaling/demarshaling of hpi data types
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

#ifndef dMarshalHpiTypes_h
#define dMarshalHpiTypes_h


#ifdef __cplusplus
extern "C" {
#endif

#include <SaHpi.h>
#include <oHpi.h>
#include <oh_utils.h>
#ifdef __cplusplus
}
#endif


#ifndef dMarshal_h
#include "marshal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define SaHpiVoidType    Marshal_Uint8Type
#define SaHpiUint8Type   Marshal_Uint8Type
#define SaHpiUint16Type  Marshal_Uint16Type
#define SaHpiUint32Type  Marshal_Uint32Type
#define SaHpiInt8Type    Marshal_Int8Type
#define SaHpiInt16Type   Marshal_Int16Type
#define SaHpiInt32Type   Marshal_Int32Type
#define SaHpiInt64Type   Marshal_Int64Type
#define SaHpiUint64Type  Marshal_Uint64Type
#define SaHpiFloat32Type Marshal_Float32Type
#define SaHpiFloat64Type Marshal_Float64Type

#define SaHpiBoolType    SaHpiUint8Type
#define SaHpiManufacturerIdType SaHpiUint32Type
#define SaHpiVersionType SaHpiUint32Type
#define SaErrorType SaHpiUint32Type

#define SaHpiDomainIdType   SaHpiUint32Type
#define SaHpiSessionIdType  SaHpiUint32Type
#define SaHpiResourceIdType SaHpiUint32Type
#define SaHpiHsCapabilitiesType  SaHpiUint32Type
#define SaHpiEntryIdType    SaHpiUint32Type			
#define SaHpiTimeType       SaHpiInt64Type
#define SaHpiTimeoutType    SaHpiInt64Type

// text buffer
#define SaHpiTextTypeType   SaHpiUint32Type
#define SaHpiLanguageType   SaHpiUint32Type
extern cMarshalType SaHpiTextBufferType;
#define SaHpiInstrumentIdType SaHpiUint32Type

// entity
#define SaHpiEntityTypeType SaHpiUint32Type
#define SaHpiEntityLocationType SaHpiUint32Type
extern cMarshalType SaHpiEntityType;
extern cMarshalType SaHpiEntityPathType;

// events
#define SaHpiEventCategoryType SaHpiUint8Type
#define SaHpiEventStateType SaHpiUint16Type

// sensors
#define SaHpiSensorNumType SaHpiUint32Type
#define SaHpiSensorTypeType SaHpiUint32Type

// sensor reading type
#define SaHpiSensorReadingTypeType SaHpiUint32Type
extern cMarshalType SaHpiSensorReadingType;

// sensor thresholds
extern cMarshalType SaHpiSensorThresholdsType;
#define SaHpiSensorRangeFlagsType SaHpiUint8Type
extern cMarshalType SaHpiSensorRangeType;

// sensor rdr sensor units
#define SaHpiSensorUnitsType SaHpiUint32Type
#define SaHpiSensorModUnitUseType SaHpiUint32Type
extern cMarshalType SaHpiSensorDataFormatType;

// sensor rdr threshold support
#define SaHpiSensorThdMaskType SaHpiUint8Type
extern cMarshalType SaHpiSensorThdDefnType;

// sensor rdr event control
#define SaHpiSensorEventCtrlType SaHpiUint32Type

// sensor rdr record
extern cMarshalType SaHpiSensorRecType;

// controls
#define SaHpiCtrlNumType SaHpiUint32Type
#define SaHpiCtrlTypeType SaHpiUint32Type
#define SaHpiCtrlStateDigitalType SaHpiUint32Type
#define SaHpiCtrlStateDiscreteType SaHpiUint32Type
#define SaHpiCtrlStateAnalogType SaHpiInt32Type
extern cMarshalType SaHpiCtrlStateStreamType;
#define SaHpiCtrlModeType SaHpiUint32Type

// control text controls
#define SaHpiTxtLineNumType SaHpiUint8Type
extern cMarshalType SaHpiCtrlStateTextType;
extern cMarshalType SaHpiCtrlStateOemType;
extern cMarshalType SaHpiCtrlStateType;

// control resource data records
#define SaHpiCtrlOutputTypeType SaHpiUint32Type
extern cMarshalType SaHpiCtrlRecDigitalType;
extern cMarshalType SaHpiCtrlRecDiscreteType;
extern cMarshalType SaHpiCtrlRecAnalogType;
extern cMarshalType SaHpiCtrlRecStreamType;
extern cMarshalType SaHpiCtrlRecTextType;
extern cMarshalType SaHpiCtrlRecOemType;
extern cMarshalType SaHpiCtrlDefaultModeType;
extern cMarshalType SaHpiCtrlRecType;

// entity inventory data
#define SaHpiIdrIdType SaHpiUint32Type
#define SaHpiIdrAreaTypeType SaHpiUint32Type
#define SaHpiIdrFieldTypeType SaHpiUint32Type
extern cMarshalType SaHpiIdrFieldType;
extern cMarshalType SaHpiIdrAreaHeaderType;
extern cMarshalType SaHpiIdrInfoType;

// inventory data resource records
extern cMarshalType SaHpiInventoryRecType;

// watchdogs
#define SaHpiWatchdogNumType SaHpiUint32Type
#define SaHpiWatchdogActionType SaHpiUint32Type
#define SaHpiWatchdogActionEventType SaHpiUint32Type
#define SaHpiWatchdogPretimerInterruptType SaHpiUint32Type
#define SaHpiWatchdogTimerUseType SaHpiUint32Type
#define SaHpiWatchdogExpFlagsType SaHpiUint8Type
extern cMarshalType SaHpiWatchdogType;

// watchdog resource data records
extern cMarshalType SaHpiWatchdogRecType;

// Annunciators
#define SaHpiAnnunciatorNumType SaHpiUint32Type
#define SaHpiStatusCondTypeType SaHpiUint32Type
extern cMarshalType SaHpiNameType;
extern cMarshalType SaHpiConditionType;
extern cMarshalType SaHpiAnnouncementType;
#define SaHpiAnnunciatorModeType SaHpiUint32Type

// annunciator resource data records
#define SaHpiAnnunciatorTypeType SaHpiUint32Type
extern cMarshalType SaHpiAnnunciatorRecType;


// resource data record
#define SaHpiRdrTypeType SaHpiUint32Type
extern cMarshalType SaHpiRdrType;

// hot swap
#define SaHpiHsIndicatorStateType SaHpiUint32Type
#define SaHpiHsActionType SaHpiUint32Type
#define SaHpiHsStateType SaHpiUint32Type

// events part 2
#define SaHpiSeverityType SaHpiUint32Type
#define SaHpiResourceEventTypeType SaHpiUint32Type
extern cMarshalType SaHpiResourceEventType;
#define SaHpiDomainEventTypeType SaHpiUint32Type
extern cMarshalType SaHpiDomainEventType;
#define SaHpiSensorOptionalDataType SaHpiUint8Type
extern cMarshalType SaHpiSensorEventType;
#define SaHpiSensorEnableOptDataType SaHpiUint8Type
extern cMarshalType SaHpiSensorEnableChangeEventType;
extern cMarshalType SaHpiHotSwapEventType;
extern cMarshalType SaHpiWatchdogEventType;
#define SaHpiSwEventTypeType SaHpiUint32Type
extern cMarshalType SaHpiSwEventType;
extern cMarshalType SaHpiOemEventType;
extern cMarshalType SaHpiUserEventType;
#define SaHpiEventTypeType SaHpiUint32Type
extern cMarshalType SaHpiEventType;
#define SaHpiEvtQueueStatusType SaHpiUint32Type

// param control
#define SaHpiParmActionType SaHpiUint32Type

// reset
#define SaHpiResetActionType SaHpiUint32Type

// power
#define SaHpiPowerStateType SaHpiUint32Type

// resource presence table
extern cMarshalType SaHpiResourceInfoType;

// resource capabilities
#define SaHpiCapabilitiesType SaHpiUint32Type

// resource hot swap capabilities
#define SaHpiHsCapabilitiesType SaHpiUint32Type

// rpt entry
extern cMarshalType SaHpiRptEntryType;

// domains
#define SaHpiDomainCapabilitiesType SaHpiUint32Type
extern cMarshalType SaHpiDomainInfoType;
extern cMarshalType SaHpiDrtEntryType;
#define SaHpiAlarmIdType SaHpiUint32Type
extern cMarshalType SaHpiAlarmType;

// system event log
#define SaHpiSelOverflowActionType SaHpiUint32Type
extern cMarshalType SaHpiEventLogInfoType;
#define SaHpiEventLogEntryIdType SaHpiUint32Type
extern cMarshalType SaHpiEventLogEntryType;

//----------------------------------------------------------------------------
// The following support the oHpi dynamic configuration APIs
//----------------------------------------------------------------------------

// plugin stuff
#define refcountType SaHpiInt32Type
extern cMarshalType oHpiPluginInfoType;

// handler stuff
#define oHpiHandlerIdType SaHpiUint32Type
extern cMarshalType oHpiHandlerInfoType;
#define oHpiGlobalParamTypeType SaHpiUint32Type
extern cMarshalType oHpiGlobalParamType;

typedef struct {
        SaHpiUint16T   DataLength;
        SaHpiUint8T    Data[OH_MAX_TEXT_BUFFER_LENGTH];  /* Data buffer */
} oHpiTextBufferT;

extern cMarshalType oHpiTextBufferType;







#ifdef __cplusplus
}
#endif

#endif
