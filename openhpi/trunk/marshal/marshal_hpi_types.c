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
 */

#include "marshal_hpi_types.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>


// text buffer
static cMarshalType SaHpiTextBufferDataArray = dVarArray( SaHpiUint8Type, 2 );

static cMarshalType SaHpiTextBufferElements[] =
{
  dStructElement( SaHpiTextBufferT, DataType, SaHpiTextTypeType ),
  dStructElement( SaHpiTextBufferT, Language, SaHpiLanguageType ),
  dStructElement( SaHpiTextBufferT, DataLength, SaHpiUint8Type ),
  dStructElement( SaHpiTextBufferT, Data, SaHpiTextBufferDataArray ),
  dStructElementEnd()
};

cMarshalType SaHpiTextBufferType = dStruct( SaHpiTextBufferT, SaHpiTextBufferElements );


// entity
static cMarshalType SaHpiEntityElements[] =
{
  dStructElement( SaHpiEntityT, EntityType, SaHpiEntityTypeType ),
  dStructElement( SaHpiEntityT, EntityLocation, SaHpiEntityLocationType ),
  dStructElementEnd()
};

cMarshalType SaHpiEntityType = dStruct( SaHpiEntityT, SaHpiEntityElements );

// entity path
static cMarshalType SaHpiEntityPathEntryArray = dArray( SaHpiEntityType, SAHPI_MAX_ENTITY_PATH );


static cMarshalType SaHpiEntityPathElements[] =
{
  dStructElement( SaHpiEntityPathT, Entry, SaHpiEntityPathEntryArray ),
  dStructElementEnd()
};

cMarshalType SaHpiEntityPathType = dStruct( SaHpiEntityPathT, SaHpiEntityPathElements );


// sensors
static cMarshalType SaHpiSensorInterpretedUnionBufferArray = dArray( SaHpiUint8Type, SAHPI_SENSOR_BUFFER_LENGTH );

static cMarshalType SaHpiSensorReadingUnionElements[] =
{
  dUnionElement( SAHPI_SENSOR_READING_TYPE_INT64,   SaHpiInt64Type ),
  dUnionElement( SAHPI_SENSOR_READING_TYPE_UINT64,  SaHpiUint64Type ),
  dUnionElement( SAHPI_SENSOR_READING_TYPE_FLOAT64, SaHpiFloat64Type ),
  dUnionElement( SAHPI_SENSOR_READING_TYPE_BUFFER , SaHpiSensorInterpretedUnionBufferArray ),
  dUnionElementEnd()
};

static cMarshalType SaHpiSensorReadingUnionType = dUnion( 0, 
							  SaHpiSensorReadingUnionT, 
                                                          SaHpiSensorReadingUnionElements );

// sensor status

// sensor reading
static cMarshalType SaHpiSensorReadingElements[] =
{
  dStructElement( SaHpiSensorReadingT, IsSupported, SaHpiBoolType ),
  dStructElement( SaHpiSensorReadingT, Type, SaHpiSensorReadingTypeType ),
  dStructElement( SaHpiSensorReadingT, Value, SaHpiSensorReadingUnionType ),
  dStructElementEnd()
};

cMarshalType SaHpiSensorReadingType = dStruct( SaHpiSensorReadingT, SaHpiSensorReadingElements );


static cMarshalType SaHpiSensorThresholdsElements[] =
{
  dStructElement( SaHpiSensorThresholdsT, LowCritical     , SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorThresholdsT, LowMajor        , SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorThresholdsT, LowMinor        , SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorThresholdsT, UpCritical      , SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorThresholdsT, UpMajor         , SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorThresholdsT, UpMinor         , SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorThresholdsT, PosThdHysteresis, SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorThresholdsT, NegThdHysteresis, SaHpiSensorReadingType ),
  dStructElementEnd()
};

cMarshalType SaHpiSensorThresholdsType = dStruct( SaHpiSensorThresholdsT, SaHpiSensorThresholdsElements );


static cMarshalType SaHpiSensorRangeElements[] =
{
  dStructElement( SaHpiSensorRangeT, Flags    , SaHpiSensorRangeFlagsType ),
  dStructElement( SaHpiSensorRangeT, Max      , SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorRangeT, Min      , SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorRangeT, Nominal  , SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorRangeT, NormalMax, SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorRangeT, NormalMin, SaHpiSensorReadingType ),
  dStructElementEnd()
};


cMarshalType SaHpiSensorRangeType = dStruct( SaHpiSensorRangeT, SaHpiSensorRangeElements );


static cMarshalType SaHpiSensorDataFormatElements[] =
{
	dStructElement( SaHpiSensorDataFormatT, IsSupported, SaHpiBoolType ),

	dStructElement( SaHpiSensorDataFormatT, ReadingType, SaHpiReadingTypeType ),

   	dStructElement( SaHpiSensorDataFormatT, BaseUnits, SaHpiSensorUnitsType ),
	dStructElement( SaHpiSensorDataFormatT, ModifierUnits, SaHpiSensorUnitsType ),
	dStructElement( SaHpiSensorDataFormatT, ModifierUse, SaHpiSensorModUnitUseType ),
	dStructElement( SaHpiSensorDataFormatT, Percentage, SaHpiBoolType ),
        dStructElement( SaHpiSensorDataFormatT, Range, SaHpiSensorRangeType ),
        dStructElement( SaHpiSensorDataFormatT, AccuracyFactor,SaHpiFloat64Type ),

	dStructElementEnd()
};

cMarshalType SaHpiSensorDataFormatType = dStruct( SaHpiSensorDataFormatT, SaHpiSensorDataFormatElements );


static cMarshalType SaHpiSensorThdDefnElements[] =
{
  dStructElement( SaHpiSensorThdDefnT, IsAccessible, SaHpiBoolType ),
  dStructElement( SaHpiSensorThdDefnT, ReadThold, SaHpiSensorThdMaskType ),
  dStructElement( SaHpiSensorThdDefnT, WriteThold, SaHpiSensorThdMaskType ),
  dStructElement( SaHpiSensorThdDefnT, Nonlinear, SaHpiSensorThdMaskType ),
  dStructElementEnd()
};

cMarshalType SaHpiSensorThdDefnType = dStruct( SaHpiSensorThdDefnT, SaHpiSensorThdDefnElements );


static cMarshalType SaHpiSensorRecElements[] =
{
	dStructElement( SaHpiSensorRecT, Num, SaHpiSensorNumType ),
	dStructElement( SaHpiSensorRecT, Type, SaHpiSensorTypeType ),
	dStructElement( SaHpiSensorRecT, Category, SaHpiEventCategoryType ),
	dStructElement( SaHpiSensorRecT, EnableCtrl,SaHpiBoolType ),
	
	dStructElement( SaHpiSensorRecT, EventCtrl, SaHpiSensorEventCtrlType ),
	dStructElement( SaHpiSensorRecT, Events, SaHpiEventStateType ),

	dStructElement( SaHpiSensorRecT, DataFormat, SaHpiSensorDataFormatType ),
	dStructElement( SaHpiSensorRecT, ThresholdDefn, SaHpiSensorThdDefnType ),
	dStructElement( SaHpiSensorRecT, Oem, SaHpiUint32Type ),
	  
	dStructElementEnd()
};

cMarshalType SaHpiSensorRecType = dStruct( SaHpiSensorRecT, SaHpiSensorRecElements );


static cMarshalType SaHpiCtrlStateStreamArray = dArray( SaHpiUint8Type, SAHPI_CTRL_MAX_STREAM_LENGTH );


static cMarshalType SaHpiCtrlStateStreamElements[] =
{
  dStructElement( SaHpiCtrlStateStreamT, Repeat, SaHpiBoolType ),
  dStructElement( SaHpiCtrlStateStreamT, StreamLength, SaHpiUint32Type ),
  dStructElement( SaHpiCtrlStateStreamT, Stream, SaHpiCtrlStateStreamArray ),
  dStructElementEnd()
};

cMarshalType SaHpiCtrlStateStreamType = dStruct( SaHpiCtrlStateStreamT, SaHpiCtrlStateStreamElements );


static cMarshalType SaHpiCtrlStateTextElements[] = 
{
  dStructElement( SaHpiCtrlStateTextT, Line, SaHpiTxtLineNumType ),
  dStructElement( SaHpiCtrlStateTextT, Text, SaHpiTextBufferType ),
  dStructElementEnd()
};

cMarshalType SaHpiCtrlStateTextType = dStruct( SaHpiCtrlStateTextT, SaHpiCtrlStateTextElements );


static cMarshalType SaHpiCtrlStateOemBodyArray = dArray( SaHpiUint8Type, SAHPI_CTRL_MAX_OEM_BODY_LENGTH );

static cMarshalType SaHpiCtrlStateOemElements[] =
{
  dStructElement( SaHpiCtrlStateOemT, MId, SaHpiManufacturerIdType ),
  dStructElement( SaHpiCtrlStateOemT, BodyLength, SaHpiUint8Type ),
  dStructElement( SaHpiCtrlStateOemT, Body, SaHpiCtrlStateOemBodyArray ),
  dStructElementEnd()
};

cMarshalType SaHpiCtrlStateOemType = dStruct( SaHpiCtrlStateOemT, SaHpiCtrlStateOemElements );


static cMarshalType SaHpiCtrlStateUnionElements[] = 
{
  dUnionElement( SAHPI_CTRL_TYPE_DIGITAL, SaHpiCtrlStateDigitalType ),
  dUnionElement( SAHPI_CTRL_TYPE_DISCRETE, SaHpiCtrlStateDiscreteType ),
  dUnionElement( SAHPI_CTRL_TYPE_ANALOG, SaHpiCtrlStateAnalogType ),
  dUnionElement( SAHPI_CTRL_TYPE_STREAM, SaHpiCtrlStateStreamType ),
  dUnionElement( SAHPI_CTRL_TYPE_TEXT, SaHpiCtrlStateTextType ),
  dUnionElement( SAHPI_CTRL_TYPE_OEM, SaHpiCtrlStateOemType ),
  dUnionElementEnd()
};


cMarshalType SaHpiCtrlStateUnionType = dUnion( 0, SaHpiCtrlStateUnionT, SaHpiCtrlStateUnionElements );

static cMarshalType SaHpiCtrlStateElements[] =
{
  dStructElement( SaHpiCtrlStateT, Type, SaHpiCtrlTypeType ),
  dStructElement( SaHpiCtrlStateT, StateUnion, SaHpiCtrlStateUnionType ),
  dStructElementEnd()
};

cMarshalType SaHpiCtrlStateType = dStruct( SaHpiCtrlStateT, SaHpiCtrlStateElements );


static cMarshalType SaHpiCtrlRecDigitalElements[] =
{
  dStructElement( SaHpiCtrlRecDigitalT, Default, SaHpiCtrlStateDigitalType ),
  dStructElementEnd()
};

cMarshalType SaHpiCtrlRecDigitalType = dStruct( SaHpiCtrlRecDigitalT, SaHpiCtrlRecDigitalElements );


static cMarshalType SaHpiCtrlRecDiscreteElements[] =
{
  dStructElement( SaHpiCtrlRecDiscreteT, Default, SaHpiCtrlStateDiscreteType ),
  dStructElementEnd()
};

cMarshalType SaHpiCtrlRecDiscreteType = dStruct( SaHpiCtrlRecDiscreteT, SaHpiCtrlRecDiscreteElements );


static cMarshalType SaHpiCtrlRecAnalogElements[] =
{
  dStructElement( SaHpiCtrlRecAnalogT, Min, SaHpiCtrlStateAnalogType ),
  dStructElement( SaHpiCtrlRecAnalogT, Max, SaHpiCtrlStateAnalogType ),
  dStructElement( SaHpiCtrlRecAnalogT, Default, SaHpiCtrlStateAnalogType ),
  dStructElementEnd()
};

cMarshalType SaHpiCtrlRecAnalogType = dStruct( SaHpiCtrlRecAnalogT, SaHpiCtrlRecAnalogElements );


static cMarshalType SaHpiCtrlRecStreamElements[] =
{
  dStructElement( SaHpiCtrlRecStreamT, Default, SaHpiCtrlStateStreamType ),
  dStructElementEnd()
};

cMarshalType SaHpiCtrlRecStreamType = dStruct( SaHpiCtrlRecStreamT, SaHpiCtrlRecStreamElements );


static cMarshalType SaHpiCtrlRecTextElements[] =
{
  dStructElement( SaHpiCtrlRecTextT, MaxChars, SaHpiUint8Type ),
  dStructElement( SaHpiCtrlRecTextT, MaxLines, SaHpiUint8Type ),
  dStructElement( SaHpiCtrlRecTextT, Language, SaHpiLanguageType ),
  dStructElement( SaHpiCtrlRecTextT, DataType, SaHpiTextTypeType ),
  dStructElement( SaHpiCtrlRecTextT, Default, SaHpiCtrlStateTextType ),
  dStructElementEnd()
};

cMarshalType SaHpiCtrlRecTextType = dStruct( SaHpiCtrlRecTextT, SaHpiCtrlRecTextElements );


static cMarshalType SaHpiCtrlRecOemConfigDataArray = dArray( SaHpiUint8Type, SAHPI_CTRL_OEM_CONFIG_LENGTH );

static cMarshalType SaHpiCtrlRecOemElements[] =
{
  dStructElement( SaHpiCtrlRecOemT, MId, SaHpiManufacturerIdType ),
  dStructElement( SaHpiCtrlRecOemT, ConfigData, SaHpiCtrlRecOemConfigDataArray ),
  dStructElement( SaHpiCtrlRecOemT, Default, SaHpiCtrlStateOemType ),
  dStructElementEnd()
};

cMarshalType SaHpiCtrlRecOemType = dStruct( SaHpiCtrlRecOemT, SaHpiCtrlRecOemElements );


static cMarshalType SaHpiCtrlRecUnionElements[] =
{
	dUnionElement( SAHPI_CTRL_TYPE_DIGITAL, SaHpiCtrlRecDigitalType ),
	dUnionElement( SAHPI_CTRL_TYPE_DISCRETE, SaHpiCtrlRecDiscreteType ),
	dUnionElement( SAHPI_CTRL_TYPE_ANALOG, SaHpiCtrlRecAnalogType ),
	dUnionElement( SAHPI_CTRL_TYPE_STREAM, SaHpiCtrlRecStreamType ),
	dUnionElement( SAHPI_CTRL_TYPE_TEXT, SaHpiCtrlRecTextType ),
	dUnionElement( SAHPI_CTRL_TYPE_OEM, SaHpiCtrlRecOemType ),
	dUnionElementEnd()
};

static cMarshalType SaHpiCtrlRecUnionType = dUnion( 3, SaHpiCtrlRecUnionT, SaHpiCtrlRecUnionElements );


static cMarshalType SaHpiCtrlDefaultModeElements[] =
{
	dStructElement( SaHpiCtrlDefaultModeT, Mode, SaHpiCtrlModeType ),
	dStructElement( SaHpiCtrlDefaultModeT, ReadOnly, SaHpiBoolType),
	dStructElementEnd()
};

cMarshalType SaHpiCtrlDefaultModeType = dStruct( SaHpiCtrlDefaultModeT, SaHpiCtrlDefaultModeElements );


static cMarshalType SaHpiCtrlRecElements[] =
{
	dStructElement( SaHpiCtrlRecT, Num, SaHpiCtrlNumType ),

	dStructElement( SaHpiCtrlRecT, OutputType, SaHpiCtrlOutputTypeType ),
	dStructElement( SaHpiCtrlRecT, Type, SaHpiCtrlTypeType ),
	dStructElement( SaHpiCtrlRecT, TypeUnion, SaHpiCtrlRecUnionType ),
	dStructElement( SaHpiCtrlRecT, DefaultMode, SaHpiCtrlDefaultModeType ),

	dStructElement( SaHpiCtrlRecT, WriteOnly, SaHpiBoolType ),

	dStructElement( SaHpiCtrlRecT, Oem, SaHpiUint32Type),
	dStructElementEnd()
};

cMarshalType SaHpiCtrlRecType = dStruct( SaHpiCtrlRecT, SaHpiCtrlRecElements );


// inventory data
// TODO DJ
#if 0
static int
SaHpiInventInternalUseDataMarshaller( SaHpiUint32T len, 
				      const SaHpiInventInternalUseDataT *iu,
				      unsigned char *b )
{
  memcpy( b, iu->Data, len );

  return len;
}


static int
SaHpiInventGeneralDataMarshaller( const SaHpiInventGeneralDataT *gd,
				  unsigned char *b )
{
  int size = 0;
  int s;

  s = Marshal( &SaHpiTimeType, &gd->MfgDateTime, b );

  if ( s < 0 )
       return -1;

  size += s;
  b    += s;

  // placeholder for number of entries
  tUint32 *num = (tUint32 *)b;
  b += sizeof( tUint32 );
  size += sizeof( tUint32 );

  SaHpiTextBufferT * const *tbp = &gd->Manufacturer;
  int i;

  tUint8 found     = 1;
  tUint8 not_found = 0;

  int n = 0;

  for( i = 0; ;i++, tbp++ )
     {
       const SaHpiTextBufferT *tb = *tbp;

       s = Marshal( &Uint8Type, tb ? &found : &not_found, b );

       if ( s < 0 )
	    return -1;

       size += s;
       b    += s;

       if ( tb == 0 )
	  {
	    if ( i > 7 )
		 break;

	    n++;

	    continue;
	  }

       n++;

       s = Marshal( &SaHpiTextBufferType, tb, b );

       if ( s < 0 )
	    return -1;

       size += s;
       b    += s;
     }

  // fill in the placeholder for the number of entries
  if ( Marshal( &Uint32Type, &n, num ) < 0 )
       return -1;

  return size;
}


static int
SaHpiInventChassisInfoMarshaller( SaHpiUint32T len, 
				  const SaHpiInventChassisDataT *iu,
				  unsigned char *b )
{
  int size = Marshal( &SaHpiInventChassisTypeType, &iu->Type, b );

  if ( size < 0 )
       return -1;

  b += size;

  int s = SaHpiInventGeneralDataMarshaller( &iu->GeneralData, b );

  if ( s < 0 )
       return -1;

  return size + s;
}


static int
SaHpiInventOemMarshaller( SaHpiUint32T len, 
			  const SaHpiInventOemDataT *od,
			  unsigned char *b )
{
  int size = Marshal( &SaHpiManufacturerIdType, &od->MId, b );

  if ( size < 0 )
       return -1;

  b += size;

  int s = len - sizeof( SaHpiManufacturerIdT );
  memcpy( b, od->Data, s );

  size += s;

  return size;
}


static int
SaHpiInventDataRecordMarshaller( const SaHpiInventDataRecordT *r,
				 unsigned char *b )
{
  int size = 0;
  int s;

  s = Marshal( &SaHpiInventDataRecordTypeType, &r->RecordType, b );

  if ( s < 0 )
       return -1;

  size += s;
  b    += s;

  tUint32 *len = (tUint32 *)b;
  size += sizeof( tUint32 );
  b += sizeof( tUint32 );

  int l;

  switch( r->RecordType )
     {
       case SAHPI_INVENT_RECTYPE_INTERNAL_USE:
	    l = SaHpiInventInternalUseDataMarshaller( r->DataLength, 
						      &r->RecordData.InternalUse,
						      b );
	    break;

       case SAHPI_INVENT_RECTYPE_CHASSIS_INFO:
	    l = SaHpiInventChassisInfoMarshaller( r->DataLength, 
						  &r->RecordData.ChassisInfo,
						  b );
	    break;

       case SAHPI_INVENT_RECTYPE_BOARD_INFO:
	    l = SaHpiInventGeneralDataMarshaller( &r->RecordData.BoardInfo,
						  b );
	    break;

       case SAHPI_INVENT_RECTYPE_PRODUCT_INFO:
	    l = SaHpiInventGeneralDataMarshaller( &r->RecordData.ProductInfo,
						  b );

	    break;

       case SAHPI_INVENT_RECTYPE_OEM:
	    l = SaHpiInventOemMarshaller( r->DataLength, 
					  &r->RecordData.OemData,
					  b );

	    break;

       default:
	    assert( 0 );
	    return -1;
     }

  if ( l < 0 )
       return -1;

  if ( Marshal( &Uint32Type, &l, len ) < 0 )
       return -1;

  size += l;

  return size;
}


static int
SaHpiInventoryDataMarshaller( const cMarshalType *type, const void *data,
			      void *buffer, void *user_data )
{
  const SaHpiInventoryDataT *id = (const SaHpiInventoryDataT *)data;
  unsigned char *b = buffer;
  int size = 0;
  int s;
  tUint32 i;

  s = Marshal( &SaHpiInventDataValidityType, &id->Validity, b );

  if ( s < 0 )
       return -1;

  size += s;
  b    += s;

  tUint32 num = 0;
  for( i = 0; id->DataRecords[i]; i++ )
       num++;

  s = Marshal( &Uint32Type, &num, b );

  if ( s < 0 )
       return -1;

  size += s;
  b    += s;

  for( i = 0; id->DataRecords[i]; i++ )
     {
       s = SaHpiInventDataRecordMarshaller( id->DataRecords[i], b );

       if ( s < 0 )
	    return -1;

       b    += s;
       size += s;
     }

  return size;
}


static int
SaHpiInventInternalUseDataDemarshaller( int byte_order, SaHpiUint32T *len,
 					SaHpiInventInternalUseDataT *iu,
					const unsigned char *b, tUint32 *data_size )
{
  memcpy( iu->Data, b, *len );
  *data_size += *len;

  return *len;
}


static int
SaHpiInventGeneralDataDemarshaller( int byte_order, SaHpiUint32T *len,
 				    SaHpiInventGeneralDataT *gd,
				    const unsigned char *b,
				    tUint32 *data_size )
{
  int size = 0;
  int s;

  s = Demarshal( byte_order, &SaHpiTimeType, &gd->MfgDateTime, b );

  if ( s < 0 )
       return -1;

  size += s;
  b    += s;

  // get the number of entries
  tUint32 num;
  s = Demarshal( byte_order, &Uint32Type, &num, b );

  if ( s < 0 )
       return -1;

  size += s;
  b += s;

  int offset = sizeof( SaHpiInventGeneralDataT ) + (num-8) * sizeof( SaHpiTextBufferT * );
  *data_size += offset;
  unsigned char *data = (unsigned char *)gd + offset;

  int i;
  tUint8 found;

  SaHpiTextBufferT **tbp = &gd->Manufacturer;

  for( i = 0; i < num + 1; i++, tbp++ )
     {
       s = Demarshal( byte_order, &Uint8Type, &found, b );

       if ( s < 0 )
	    return -1;

       size += s;
       b    += s;

       if ( !found )
	  {
	    *tbp = 0;
	    continue;
	  }

       s = Demarshal( byte_order, &SaHpiTextBufferType, data, b );

       if ( s < 0 )
	    return -1;

       size += s;
       b    += s;

       *tbp = (SaHpiTextBufferT *)data;

       data += sizeof( SaHpiTextBufferT );
       *data_size += sizeof( SaHpiTextBufferT );
     }

  return size;
}


static int
SaHpiInventChassisInfoDemarshaller( int byte_order, SaHpiUint32T *len, 
 				    SaHpiInventChassisDataT *iu,
				    const unsigned char *b, tUint32 *data_size )
{
  int size = Demarshal( byte_order, &SaHpiInventChassisTypeType, &iu->Type, b );

  if ( size < 0 )
       return -1;

  b += size;

  *data_size += sizeof( SaHpiInventChassisDataT ) - sizeof( SaHpiInventGeneralDataT );

  int s = SaHpiInventGeneralDataDemarshaller( byte_order, len, &iu->GeneralData, b,
					      data_size );

  if ( s < 0 )
       return -1;

  return size + s;
}


static int
SaHpiInventOemDemarshaller( int byte_order, SaHpiUint32T *len, 
 			    SaHpiInventOemDataT *od,
	 		    const unsigned char *b,
			    tUint32 *data_size )
{
  int size = Demarshal( byte_order, &SaHpiManufacturerIdType, &od->MId, b );

  if ( size < 0 )
       return -1;

  b += size;

  int s = *len - sizeof( SaHpiManufacturerIdT );
  memcpy( od->Data, b, s );

  size += s;

  *data_size += *len - sizeof( SaHpiManufacturerIdT );

  return size;
}


static int
SaHpiInventDataRecordDemarshaller( int byte_order, 
				   SaHpiInventDataRecordT *r,
				   const unsigned char *b,
				   tUint32 *data_size )
{
  int size = 0;
  int s;

  s = Demarshal( byte_order, &SaHpiInventDataRecordTypeType,
		 &r->RecordType, b );

  if ( s < 0 )
       return -1;

  size += s;
  b    += s;

  tUint32 record_len;
  s = Demarshal( byte_order, &Uint32Type, &record_len, b );

  if ( s < 0 )
       return -1;

  size += s;
  b    += s;

  r->DataLength = record_len;

  *data_size = sizeof( SaHpiInventDataRecordT ) - sizeof( SaHpiInventDataUnionT );

  switch( r->RecordType )
     {
       case SAHPI_INVENT_RECTYPE_INTERNAL_USE:
	    s = SaHpiInventInternalUseDataDemarshaller( byte_order, &r->DataLength,
		 					&r->RecordData.InternalUse,
							b, data_size );
	    break;

       case SAHPI_INVENT_RECTYPE_CHASSIS_INFO:
	    s = SaHpiInventChassisInfoDemarshaller( byte_order, &r->DataLength, 
						    &r->RecordData.ChassisInfo,
						    b, data_size );
  	    break;

       case SAHPI_INVENT_RECTYPE_BOARD_INFO:
	    s = SaHpiInventGeneralDataDemarshaller( byte_order, &r->DataLength,
						    &r->RecordData.BoardInfo,
						    b, data_size );
	    break;

       case SAHPI_INVENT_RECTYPE_PRODUCT_INFO:
	    s = SaHpiInventGeneralDataDemarshaller( byte_order, &r->DataLength,
		 				    &r->RecordData.ProductInfo,
						    b, data_size );

	    break;

       case SAHPI_INVENT_RECTYPE_OEM:
	    s = SaHpiInventOemDemarshaller( byte_order, &r->DataLength, 
					    &r->RecordData.OemData,
					    b, data_size );

	    break;

       default:
	    assert( 0 );
	    break;
     }

  if ( s < 0 )
       return -1;

  size += s;

  return size;
}


static int 
SaHpiInventoryDataDemarshaller( int byte_order, const cMarshalType *type, void *data,
				const void *buffer, void *user_data )
{
  SaHpiInventoryDataT *id = (SaHpiInventoryDataT *)data;
  const unsigned char *b = buffer;
  int size = 0;
  int s;

  s = Demarshal( byte_order, &SaHpiInventDataValidityType, &id->Validity, b );

  if ( s < 0 )
    return -1;

  size += s;
  b    += s;

  tUint32 num;
  s = Demarshal( byte_order, &Uint32Type, &num, b );

  if ( s < 0 )
       return -1;

  size += s;
  b    += s;

  unsigned char *data_ptr = (unsigned char *)&id->DataRecords;
  data_ptr += ( num + 1 ) * sizeof( SaHpiInventDataRecordT * );

  tUint32 i;
  for( i = 0; i < num; i++ )
     {
       tUint32 data_size;

       id->DataRecords[i] = (SaHpiInventDataRecordT *)data_ptr;
       s = SaHpiInventDataRecordDemarshaller( byte_order, id->DataRecords[i], b, &data_size );

       if ( s < 0 )
	    return -1;

       b    += s;
       size += s;

       data_ptr += data_size;
     }

  id->DataRecords[num] = 0;

  return size;
}


cMarshalType SaHpiInventoryDataType = dUserDefined( SaHpiInventoryDataMarshaller, SaHpiInventoryDataDemarshaller, 0 );


// inventory resource data records
static cMarshalType SaHpiInventoryRecElements[] =
{
  dStructElement( SaHpiInventoryRecT, EirId, SaHpiEirIdType ),
  dStructElement( SaHpiInventoryRecT, Oem, SaHpiUint32Type ),
  dStructElementEnd()
};

cMarshalType SaHpiInventoryRecType = dStruct( SaHpiInventoryRecT, SaHpiInventoryRecElements );
#endif

static cMarshalType SaHpiWatchdogElements[] =
{
  dStructElement( SaHpiWatchdogT, Log, SaHpiBoolType ),
  dStructElement( SaHpiWatchdogT, Running, SaHpiBoolType ),
  dStructElement( SaHpiWatchdogT, TimerUse, SaHpiWatchdogTimerUseType ),
  dStructElement( SaHpiWatchdogT, TimerAction, SaHpiWatchdogActionType ),
  dStructElement( SaHpiWatchdogT, PretimerInterrupt, SaHpiWatchdogPretimerInterruptType ),
  dStructElement( SaHpiWatchdogT, PreTimeoutInterval, SaHpiUint32Type ),
  dStructElement( SaHpiWatchdogT, TimerUseExpFlags, SaHpiWatchdogExpFlagsType ),
  dStructElement( SaHpiWatchdogT, InitialCount, SaHpiUint32Type ),
  dStructElement( SaHpiWatchdogT, PresentCount, SaHpiUint32Type ),
  dStructElementEnd()
};

cMarshalType SaHpiWatchdogType = dStruct( SaHpiWatchdogT, SaHpiWatchdogElements );


// watchdog resource data records
static cMarshalType SaHpiWatchdogRecElements[] =
{
  dStructElement( SaHpiWatchdogRecT, WatchdogNum, SaHpiWatchdogNumType ),
  dStructElement( SaHpiWatchdogRecT, Oem, SaHpiUint32Type ),
  dStructElementEnd()
};

cMarshalType SaHpiWatchdogRecType = dStruct( SaHpiWatchdogRecT, SaHpiWatchdogRecElements );

static cMarshalType SaHpiAnnunciatorRecElements[] =
{
	dStructElement( SaHpiAnnunciatorRecT, AnnunciatorNum, SaHpiAnnunciatorNumType ),
	dStructElement( SaHpiAnnunciatorRecT, AnnunciatorType, SaHpiAnnunciatorTypeType ),
	dStructElement( SaHpiAnnunciatorRecT, ModeReadOnly, SaHpiBoolType ),
	dStructElement( SaHpiAnnunciatorRecT, MaxConditions, SaHpiUint32Type ),
	dStructElement( SaHpiAnnunciatorRecT, Oem, SaHpiUint32Type ),
	dStructElementEnd()
};

cMarshalType SaHpiAnnunciatorRecType = dStruct( SaHpiAnnunciatorRecT, SaHpiAnnunciatorRecElements );

static cMarshalType SaHpiRdrTypeUnionElements[] =
{
	dUnionElement( SAHPI_NO_RECORD, VoidType ),
	dUnionElement( SAHPI_CTRL_RDR, SaHpiCtrlRecType ),
	dUnionElement( SAHPI_SENSOR_RDR, SaHpiSensorRecType ),
	dUnionElement( SAHPI_INVENTORY_RDR, VoidType ),/* TODO this void type is just a place holder */
	dUnionElement( SAHPI_WATCHDOG_RDR, SaHpiWatchdogRecType ),
	dUnionElement( SAHPI_ANNUNCIATOR_RDR, SaHpiAnnunciatorRecType ),
	dUnionElementEnd()
};

static cMarshalType SaHpiRdrTypeUnionType = dUnion( 1, SaHpiRdrTypeUnionT, SaHpiRdrTypeUnionElements );


static cMarshalType SaHpiRdrElements[] =
{
	dStructElement( SaHpiRdrT, RecordId, SaHpiEntryIdType ),
	dStructElement( SaHpiRdrT, RdrType, SaHpiRdrTypeType ),
	dStructElement( SaHpiRdrT, Entity, SaHpiEntityPathType ),

	dStructElement( SaHpiRdrT, Entity, SaHpiBoolType ),

	dStructElement( SaHpiRdrT, RdrTypeUnion, SaHpiRdrTypeUnionType ),
	dStructElement( SaHpiRdrT, IdString, SaHpiTextBufferType ),
	dStructElementEnd()
};

cMarshalType SaHpiRdrType = dStruct( SaHpiRdrT, SaHpiRdrElements );


static cMarshalType SaHpiSensorEventElements[] =
{
  dStructElement( SaHpiSensorEventT, SensorNum, SaHpiSensorNumType ),
  dStructElement( SaHpiSensorEventT, SensorType, SaHpiSensorTypeType ),
  dStructElement( SaHpiSensorEventT, EventCategory, SaHpiEventCategoryType ),
  dStructElement( SaHpiSensorEventT, Assertion, SaHpiBoolType ),
  dStructElement( SaHpiSensorEventT, EventState, SaHpiEventStateType ),
  dStructElement( SaHpiSensorEventT, OptionalDataPresent, SaHpiSensorOptionalDataType ),
  dStructElement( SaHpiSensorEventT, TriggerReading, SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorEventT, TriggerThreshold, SaHpiSensorReadingType ),
  dStructElement( SaHpiSensorEventT, PreviousState, SaHpiEventStateType ),
  dStructElement( SaHpiSensorEventT, Oem, SaHpiUint32Type ),
  dStructElement( SaHpiSensorEventT, SensorSpecific, SaHpiUint32Type ),
  dStructElementEnd()
};

cMarshalType SaHpiSensorEventType = dStruct( SaHpiSensorEventT, SaHpiSensorEventElements );


static cMarshalType SaHpiHotSwapEventElements[] =
{
  dStructElement( SaHpiHotSwapEventT, HotSwapState, SaHpiHsStateType ),
  dStructElement( SaHpiHotSwapEventT, PreviousHotSwapState, SaHpiHsStateType ),
  dStructElementEnd()
};

cMarshalType SaHpiHotSwapEventType = dStruct( SaHpiHotSwapEventT, SaHpiHotSwapEventElements );


static cMarshalType SaHpiWatchdogEventElements[] =
{
  dStructElement( SaHpiWatchdogEventT, WatchdogNum, SaHpiWatchdogNumType ),
  dStructElement( SaHpiWatchdogEventT, WatchdogAction, SaHpiWatchdogActionEventType ),
  dStructElement( SaHpiWatchdogEventT, WatchdogPreTimerAction, SaHpiWatchdogPretimerInterruptType ),
  dStructElement( SaHpiWatchdogEventT, WatchdogUse, SaHpiWatchdogTimerUseType ),
  dStructElementEnd()
};

cMarshalType SaHpiWatchdogEventType = dStruct( SaHpiWatchdogEventT, SaHpiWatchdogEventElements );


static cMarshalType SaHpiOemEventElements[] =
{
  dStructElement( SaHpiOemEventT, MId, SaHpiManufacturerIdType ),
  dStructElement( SaHpiOemEventT, OemEventData, SaHpiTextBufferType ),
  dStructElementEnd()
};

cMarshalType SaHpiOemEventType = dStruct( SaHpiOemEventT, SaHpiOemEventElements );


static cMarshalType SaHpiUserEventElements[] =
{
  dStructElement( SaHpiUserEventT, UserEventData, SaHpiTextBufferType ),
  dStructElementEnd()
};

cMarshalType SaHpiUserEventType = dStruct( SaHpiUserEventT, SaHpiUserEventElements );


static cMarshalType SaHpiEventUnionElements[] =
{
  dUnionElement( SAHPI_ET_SENSOR, SaHpiSensorEventType ),
  dUnionElement( SAHPI_ET_HOTSWAP, SaHpiHotSwapEventType ),
  dUnionElement( SAHPI_ET_WATCHDOG, SaHpiWatchdogEventType ),
  dUnionElement( SAHPI_ET_OEM, SaHpiOemEventType ),
  dUnionElement( SAHPI_ET_USER, SaHpiUserEventType ),
  dUnionElementEnd()
};

static cMarshalType SaHpiEventUnionType = dUnion( 1, SaHpiEventUnionT, SaHpiEventUnionElements );

static cMarshalType SaHpiEventElements[] =
{
  dStructElement( SaHpiEventT, Source, SaHpiResourceIdType ),
  dStructElement( SaHpiEventT, EventType, SaHpiEventTypeType ),
  dStructElement( SaHpiEventT, Timestamp, SaHpiTimeType ),
  dStructElement( SaHpiEventT, Severity, SaHpiSeverityType ),
  dStructElement( SaHpiEventT, EventDataUnion, SaHpiEventUnionType ),
  dStructElementEnd()
};

cMarshalType SaHpiEventType = dStruct( SaHpiEventT, SaHpiEventElements );


// resource presence table
// TODO DJ
#if 0
static cMarshalType SaHpiRptInfoElements[] =
{
  dStructElement( SaHpiRptInfoT, UpdateCount, SaHpiUint32Type ),
  dStructElement( SaHpiRptInfoT, UpdateTimestamp, SaHpiTimeType ),
  dStructElementEnd()
};

cMarshalType SaHpiRptInfoType = dStruct( SaHpiRptInfoT, SaHpiRptInfoElements );
#endif


static cMarshalType GuidDataArray = dVarArray( SaHpiUint8Type, 16 );

static cMarshalType SaHpiResourceInfoElements[] =
{
	dStructElement( SaHpiResourceInfoT, ResourceRev, SaHpiUint8Type ),
	dStructElement( SaHpiResourceInfoT, SpecificVer, SaHpiUint8Type ),
	dStructElement( SaHpiResourceInfoT, DeviceSupport, SaHpiUint8Type ),
	dStructElement( SaHpiResourceInfoT, ManufacturerId, SaHpiManufacturerIdType ),
	dStructElement( SaHpiResourceInfoT, ProductId, SaHpiUint16Type ),
	dStructElement( SaHpiResourceInfoT, FirmwareMajorRev, SaHpiUint8Type ),
	dStructElement( SaHpiResourceInfoT, FirmwareMinorRev, SaHpiUint8Type ),
	dStructElement( SaHpiResourceInfoT, AuxFirmwareRev, SaHpiUint8Type ),
	dStructElement( SaHpiResourceInfoT, AuxFirmwareRev, SaHpiUint8Type ),
	dStructElement( SaHpiResourceInfoT, Guid, GuidDataArray ),
	dStructElementEnd()
};

cMarshalType SaHpiResourceInfoType = dStruct( SaHpiResourceInfoT, SaHpiResourceInfoElements );


static cMarshalType SaHpiRptEntryElements[] =
{
	dStructElement( SaHpiRptEntryT, EntryId, SaHpiEntryIdType ),
	dStructElement( SaHpiRptEntryT, ResourceId, SaHpiResourceIdType ),
	dStructElement( SaHpiRptEntryT, ResourceInfo, SaHpiResourceInfoType ),	
	dStructElement( SaHpiRptEntryT, ResourceEntity, SaHpiEntityPathType ),
	dStructElement( SaHpiRptEntryT, ResourceCapabilities,SaHpiCapabilitiesType ),

	dStructElement( SaHpiRptEntryT, HotSwapCapabilities, SaHpiHsCapabilitiesType ),

	dStructElement( SaHpiRptEntryT, ResourceSeverity, SaHpiSeverityType ), 
	dStructElement( SaHpiRptEntryT, ResourceFailed, SaHpiBoolType ),
	dStructElement( SaHpiRptEntryT, ResourceTag, SaHpiTextBufferType ),	
	dStructElementEnd()
};

cMarshalType SaHpiRptEntryType = dStruct( SaHpiRptEntryT, SaHpiRptEntryElements );


#if 0
typedef struct {
    SaHpiUint32T                   Entries;        
    SaHpiUint32T                   Size;      
    SaHpiUint32T                   UserEventMaxSize;
    SaHpiTimeT                     UpdateTimestamp;
    SaHpiTimeT                     CurrentTime;
    SaHpiBoolT                     Enabled;
    SaHpiBoolT                     OverflowFlag;
    SaHpiBoolT                     OverflowResetable;
    SaHpiEventLogOverflowActionT   OverflowAction;
} SaHpiEventLogInfoT;
#endif
static cMarshalType SaHpiEventLogInfoTElements[] =
{
	dStructElement( SaHpiEventLogInfoT, Entries, SaHpiUint32Type ),
	dStructElement( SaHpiEventLogInfoT, Size, SaHpiUint32Type ),
	dStructElement( SaHpiEventLogInfoT, UserEventMaxSize, SaHpiUint32Type ),
	dStructElement( SaHpiEventLogInfoT, UpdateTimestamp, SaHpiTimeType ),
	dStructElement( SaHpiEventLogInfoT, CurrentTime, SaHpiTimeType ),
	dStructElement( SaHpiEventLogInfoT, Enabled, SaHpiBoolType ),
	dStructElement( SaHpiEventLogInfoT, OverflowFlag, SaHpiBoolType ),
	dStructElement( SaHpiEventLogInfoT, OverflowResetable, SaHpiBoolType ),
	dStructElement( SaHpiEventLogInfoT, OverflowAction, SaHpiSelOverflowActionType ),
	dStructElementEnd()
};

cMarshalType SaHpiEventLogInfoType = dStruct( SaHpiEventLogInfoT, SaHpiEventLogInfoTElements );


static cMarshalType SaHpiEventLogEntryElements[] =
{
  dStructElement( SaHpiEventLogEntryT, EntryId, SaHpiEventLogEntryIdType ),
  dStructElement( SaHpiEventLogEntryT, Timestamp, SaHpiTimeType ),
  dStructElement( SaHpiEventLogEntryT, Event, SaHpiEventType ),
  dStructElementEnd()
};

cMarshalType SaHpiEventLogEntryType = dStruct( SaHpiEventLogEntryT, SaHpiEventLogEntryElements );
