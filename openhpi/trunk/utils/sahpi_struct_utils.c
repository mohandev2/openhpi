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
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SaHpi.h>
#include <oh_utils.h>

static inline SaErrorT oh_init_bigtext(oh_big_textbuffer *big_buffer);
static inline SaErrorT oh_append_bigtext(oh_big_textbuffer *big_buffer, const char *from, size_t size);
static inline SaErrorT oh_copy_bigtext(oh_big_textbuffer *dest, const oh_big_textbuffer *from);

static SaErrorT oh_append_offset(oh_big_textbuffer *buffer, int offsets);
static SaErrorT oh_build_sensorrec(oh_big_textbuffer *buffer, const SaHpiSensorRecT *sensor, int offsets);
static SaErrorT oh_build_sensordataformat(oh_big_textbuffer *buffer, const SaHpiSensorDataFormatT *format, int offsets);
static SaErrorT oh_build_sensorthddefn(oh_big_textbuffer *buffer, const SaHpiSensorThdDefnT *tdef, int offsets);
static SaErrorT oh_build_threshold_mask(oh_big_textbuffer *buffer, const SaHpiSensorThdMaskT tmask, int offsets);

/************************************************************************
 * NOTES!
 *
 * - Several error checks can be removed if valid_xxx routines are defined
 *   for input structures. If this happens, several of the default switch
 *   statements should also return SA_ERR_HPI_INTERNAL_ERROR instead
 *   of SA_ERR_HPI_INVALID_PARMS.
 ************************************************************************/

/**
 * oh_lookup_manufacturerid:
 * @value: enum value of type SaHpiManufacturerIdT.
 * @buffer:  Location to store the string.
 *
 * Converts @value into a string based on @value's enum definition
 * in http://www.iana.org/assignments/enterprise-numbers.
 * String is stored in an SaHpiTextBufferT data structure.
 * 
 * Only a few of the manufacturers in that list have been defined.
 * For all others, this routine returns "Unknown Manufacturer".
 * Feel free to add your own favorite manufacturer to this routine.
 * 
 * Returns:
 * SA_OK - normal operation.
 * SA_ERR_HPI_INVALID_PARAMS - @buffer is NULL
 **/
SaErrorT oh_decode_manufacturerid(SaHpiManufacturerIdT value, SaHpiTextBufferT *buffer) 
{
	SaErrorT err;
	SaHpiTextBufferT working;

	if (!buffer) {
		return(SA_ERR_HPI_INVALID_PARAMS);
	}

	err = oh_init_textbuffer(&working);
	if (err) { return(err); }

	switch(value) {
	case SAHPI_MANUFACTURER_ID_UNSPECIFIED:
		err = oh_append_textbuffer(&working, 
					   "UNSPECIFIED Manufacturer",
					   strlen("UNSPECIFIED Manufacturer"));
		if (err) { return(err); }
		break;
	case 2:
		err = oh_append_textbuffer(&working, 
					   "IBM",
					   strlen("IBM"));
		if (err) { return(err); }
		break;
	default:
		err = oh_append_textbuffer(&working, 
					   "Unknown Manufacturer",
					   strlen("Unknown Manufacturer"));
		if (err) { return(err); }
		break;
	}

 	oh_copy_textbuffer(buffer, &working);

	return(SA_OK);
}

/**
 * oh_decode_sensorreading: 
 * @reading: SaHpiSensorReadingT to convert.
 * @format: SaHpiDataFormatT for the sensor reading.
 * @buffer: Location to store the converted string.
 *
 * Converts a sensor reading and format into a string. 
 * String is stored in an SaHpiTextBufferT data structure.
 * 
 * Returns: 
 * SA_OK - normal operation.
 * SA_ERR_HPI_INVALID_CMD - @format or @reading have IsSupported == FALSE.
 * SA_ERR_HPI_INVALID_DATA - @format and @reading types don't match.
 * SA_ERR_HPI_INVALID_PARAMS - @buffer, @reading, @format is NULL; Invalid @reading type
 * SA_ERR_HPI_OUT_OF_SPACE - @buffer not big enough to accomodate appended string
 **/
SaErrorT oh_decode_sensorreading(SaHpiSensorReadingT reading,
				 SaHpiSensorDataFormatT format,
				 SaHpiTextBufferT *buffer)
{
	SaErrorT err;
	SaHpiTextBufferT working;
        char text[SAHPI_MAX_TEXT_BUFFER_LENGTH];
        size_t text_size = 0;

	if (!buffer) {
		return(SA_ERR_HPI_INVALID_PARAMS);
	}
        if (!reading.IsSupported || !format.IsSupported) {
                return(SA_ERR_HPI_INVALID_CMD);
        }
        if (reading.Type != format.ReadingType) {
                return(SA_ERR_HPI_INVALID_DATA);
        }
	
	oh_init_textbuffer(&working);
	memset(text, 0, SAHPI_MAX_TEXT_BUFFER_LENGTH);

        switch(reading.Type) {
        case SAHPI_SENSOR_READING_TYPE_INT64:
                text_size = snprintf(text, SAHPI_MAX_TEXT_BUFFER_LENGTH,
				     "%lld", reading.Value.SensorInt64);
		err = oh_append_textbuffer(&working, text, text_size);
		if (err != SA_OK) { return(err); }
                break;
        case SAHPI_SENSOR_READING_TYPE_UINT64:
		text_size = snprintf(text, SAHPI_MAX_TEXT_BUFFER_LENGTH,
				     "%llu", reading.Value.SensorUint64);
		err = oh_append_textbuffer(&working, text, text_size);
		if (err != SA_OK) { return(err); }
                break;
	case SAHPI_SENSOR_READING_TYPE_FLOAT64:
                text_size = snprintf(text, SAHPI_MAX_TEXT_BUFFER_LENGTH, 
				     "%le", reading.Value.SensorFloat64);
		err = oh_append_textbuffer(&working, text, text_size);
		if (err != SA_OK) { return(err); }
 		break;
        case SAHPI_SENSOR_READING_TYPE_BUFFER:
		err = oh_append_textbuffer(&working, reading.Value.SensorBuffer, 
					   SAHPI_SENSOR_BUFFER_LENGTH);
		if (err != SA_OK) { return(err); }
                break;
	default:
		return(SA_ERR_HPI_INVALID_PARAMS);
        }
        
        if (format.Percentage) {
		err = oh_append_textbuffer(&working, "%", 1); 
		if (err != SA_OK) { return(err); }
        }
	else {
		const char *str;

		/* Add units */
		err = oh_append_textbuffer(&working, " ", 1); 		
		if (err != SA_OK) { return(err); }
		str = oh_lookup_sensorunits(format.BaseUnits);
 		if (str == NULL) { return(SA_ERR_HPI_INVALID_PARAMS); }
		err = oh_append_textbuffer(&working, str, strlen(str)); 		
		if (err != SA_OK) { return(err); }
		
		/* Add modifier units, if appropriate */
		if (format.BaseUnits != SAHPI_SU_UNSPECIFIED && 
		    format.ModifierUse != SAHPI_SMUU_NONE) {

			switch(format.ModifierUse) {
			case SAHPI_SMUU_BASIC_OVER_MODIFIER:
				err = oh_append_textbuffer(&working, " / ", 3); 
				if (err != SA_OK) { return(err); }
				break;
			case SAHPI_SMUU_BASIC_TIMES_MODIFIER:
				err = oh_append_textbuffer(&working, " * ", 3); 
				if (err != SA_OK) { return(err); }
				break;
			default:
				return(SA_ERR_HPI_INVALID_PARAMS);
			}
			str = oh_lookup_sensorunits(format.ModifierUnits);
			if (str == NULL) { return(SA_ERR_HPI_INVALID_PARAMS); }
			err = oh_append_textbuffer(&working, str, strlen(str)); 		
			if (err != SA_OK) { return(err); }
		}
	}

 	oh_copy_textbuffer(buffer, &working);

        return(SA_OK);
}

/**
 * oh_fprint_text:
 * @stream: File handle.
 * @buffer: Pointer to SaHpiTextBufferT to be printed.
 * 
 * Prints the text data contained in SaHpiTextBufferT to a file. Data must
 * be of type SAHPI_TL_TYPE_TEXT. @buffer->DataLength is ignored.
 * The MACRO oh_print_text(), uses this function to print to STDOUT. 
 *
 * Returns:
 * SA_OK - normal operation.
 * SA_ERR_HPI_INVALID_DATA - @buffer->DataType not SAHPI_TL_TYPE_TEXT.
 **/
SaErrorT oh_fprint_text(FILE *stream, const SaHpiTextBufferT *buffer)
{
	int err;

	/* FIXME:: Why not support the other types - most represented as 8-bit 
	   ASCII; so can print without a problem */

        if (buffer->DataType == SAHPI_TL_TYPE_TEXT) {
                err = fprintf(stream, "%s\n", buffer->Data);
		if (err < 0) {
			return(SA_ERR_HPI_INVALID_PARAMS);
		}
        }
	else {
		return(SA_ERR_HPI_INVALID_DATA);
	}
	
	return(SA_OK);
}

/**
 * oh_fprint_bigtext:
 * @stream: File handle.
 * @big_buffer: Pointer to oh_big_textbuffer to be printed.
 * 
 * Prints the text data contained in oh_big_textbuffer to a file. Data must
 * be of type SAHPI_TL_TYPE_TEXT. @big_buffer->DataLength is ignored.
 * The MACRO oh_print_bigtext(), uses this function to print to STDOUT.
 * 
 * Returns:
 * SA_OK - normal operation.
 * SA_ERR_HPI_INVALID_DATA - @big_buffer->DataType not SAHPI_TL_TYPE_TEXT.
 **/
SaErrorT oh_fprint_bigtext(FILE *stream, const oh_big_textbuffer *big_buffer)
{
	int err;
	
	/* FIXME:: Why not support the other types - most represented as 8-bit 
	   ASCII; so can print without a problem */

        if (big_buffer->DataType == SAHPI_TL_TYPE_TEXT) {
                err = fprintf(stream, "%s\n", big_buffer->Data);
		if (err < 0) {
			return(SA_ERR_HPI_INVALID_PARAMS);
		}
        }
	else {
		return(SA_ERR_HPI_INVALID_DATA);
	}
	
	return(SA_OK);
}

/**
 * oh_init_textbuffer:
 * @buffer: Pointer to an SaHpiTextBufferT.
 * 
 * Initializes an SaHpiTextBufferT. Assumes an english language set.
 * 
 * Returns:
 * SA_OK - normal operation.
 * SA_ERR_HPI_INVALID_PARAMS - @buffer is NULL.
 **/
SaErrorT oh_init_textbuffer(SaHpiTextBufferT *buffer)
{
	if (!buffer) {
		return(SA_ERR_HPI_INVALID_PARAMS);
	}

	memset(buffer, 0, sizeof(*buffer));
        buffer->DataType = SAHPI_TL_TYPE_TEXT;
        buffer->Language = SAHPI_LANG_ENGLISH;
        buffer->DataLength = 0;
        return(SA_OK);
}

static inline SaErrorT oh_init_bigtext(oh_big_textbuffer *big_buffer)
{
	if (!big_buffer) {
		return(SA_ERR_HPI_INVALID_PARAMS);
	}

	memset(big_buffer, 0, sizeof(*big_buffer));
        big_buffer->DataType = SAHPI_TL_TYPE_TEXT;
        big_buffer->Language = SAHPI_LANG_ENGLISH;
        big_buffer->DataLength = 0;
        return(SA_OK);
}

/**
 * oh_copy_textbuffer:
 * @dest: SaHpiTextBufferT to copy into.
 * @from:SaHpiTextBufferT to copy from.
 *
 * Initializes an SaHpiTextBufferT. Assumes an english language set.
 * 
 * Returns:
 * SA_OK - normal operation.
 * SA_ERR_HPI_INVALID_PARAMS - @dest or @from is NULL.
 **/
SaErrorT oh_copy_textbuffer(SaHpiTextBufferT *dest, const SaHpiTextBufferT *from)
{
	if (!dest || !from) {
		return(SA_ERR_HPI_INVALID_PARAMS);
	}

        dest->DataType = from->DataType;
        dest->Language = from->Language;
        dest->DataLength = from->DataLength;
        memcpy(dest->Data, from->Data, SAHPI_MAX_TEXT_BUFFER_LENGTH);
        return(SA_OK);
}

static inline SaErrorT oh_copy_bigtext(oh_big_textbuffer *dest, const oh_big_textbuffer *from)
{
	if (!dest || !from) {
		return(SA_ERR_HPI_INVALID_PARAMS);
	}

        dest->DataType = from->DataType;
        dest->Language = from->Language;
        dest->DataLength = from->DataLength;
        memcpy(dest->Data, from->Data, OH_MAX_TEXT_BUFFER_LENGTH);
        return(SA_OK);
}

/**
 * oh_append_textbuffer:
 * @buffer: SaHpiTextBufferT to append to.
 * @from: String to be appended.
 * @size: Size of string.
 *
 * Appends a string to the @buffer->Data.
 * 
 * Returns:
 * SA_OK - normal operation.
 * SA_ERR_HPI_INVALID_PARAMS - @buffer or @from is NULL. Or
 *                             @from is NULL and size is non-zero.
 * SA_ERR_HPI_OUT_OF_SPACE - @buffer not big enough to accomodate appended string
 **/
SaErrorT oh_append_textbuffer(SaHpiTextBufferT *buffer, const char *from, size_t size)
{
        SaHpiUint8T *p;

	/* FIXME:: Add a valid_textbuffer check when routine is available */
	if (size == 0) { 
		return(SA_OK); 
	}
	if (!buffer || !from) {
		return(SA_ERR_HPI_INVALID_PARAMS);
	}
        if ((size + buffer->DataLength) >= SAHPI_MAX_TEXT_BUFFER_LENGTH) {
		dbg("Cannot append to text buffer. Bufsize=%d, size=%d",
		    buffer->DataLength, size);
                return(SA_ERR_HPI_OUT_OF_SPACE);
        }

        /* Can't trust NULLs to be right, so use a targeted strncpy instead */
        p = buffer->Data;
        p += buffer->DataLength;
        strncpy(p, from, size);
        buffer->DataLength += size;
        
        return(SA_OK);
}

static inline SaErrorT oh_append_bigtext(oh_big_textbuffer *big_buffer, const char *from, size_t size)
{
        SaHpiUint8T *p;

	if (size == 0 ) {
		return(SA_OK);
	}
	if (!big_buffer || !from) {
		dbg("Invalid parameters");
		return(SA_ERR_HPI_INVALID_PARAMS);
	}
        if ((size + big_buffer->DataLength) >= OH_MAX_TEXT_BUFFER_LENGTH) {
		dbg("Cannot append to buffer. Bufsize=%d, size=%d",
		    big_buffer->DataLength, size);
                return(SA_ERR_HPI_INTERNAL_ERROR);
        }
        
        /* Can't trust NULLs to be right, so use a targeted strncpy instead */
        p = big_buffer->Data;
        p += big_buffer->DataLength;
        strncpy(p, from, size);
        big_buffer->DataLength += size;
        
        return(SA_OK);
}


/* Append an arbitrary number of fixed offset strings to a big text buffer */
static SaErrorT oh_append_offset(oh_big_textbuffer *buffer, int offsets)
{
	int i;
	
	for (i=0; i < offsets; i++) {
		oh_append_bigtext(buffer, OH_PRINT_OFFSET, strlen(OH_PRINT_OFFSET));
	}

	return(SA_OK);
}

/**
 * oh_fprint_sensorrec:
 * @stream: File handle.
 * @sensor: Pointer to SaHpiSensorRecT to be printed.
 * 
 * Prints a sensor's SaHpiSensorRecT data to a file. 
 * The MACRO oh_print_sensorrec(), uses this function to print to STDOUT. 
 *
 * Returns:
 * SA_OK - normal operation.
 * SA_ERR_HPI_INVALID_PARAMS - \@sensor or \@stream is NULL
 **/
SaErrorT oh_fprint_sensorrec(FILE *stream, const SaHpiSensorRecT *sensor)
{
	int err;
	oh_big_textbuffer buffer;
	
	if (!stream || !sensor) {
		return(SA_ERR_HPI_INVALID_PARAMS);
	}
	
	oh_init_bigtext(&buffer);
	err = oh_build_sensorrec(&buffer, sensor, 0);
	if (err) { return(err); }

	err = oh_fprint_bigtext(stream, &buffer);
	if (err) { return(err); }
	
	return(SA_OK);
}

static SaErrorT oh_build_sensorrec(oh_big_textbuffer *buffer, const SaHpiSensorRecT *sensor, int offsets)
{
	char str[SAHPI_MAX_TEXT_BUFFER_LENGTH];
	SaErrorT err;

	/* Sensor Num */
	oh_append_offset(buffer, offsets);
	snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "Sensor Num: %d\n", sensor->Num);
	oh_append_bigtext(buffer, str, strlen(str));
	offsets++;

	/* Sensor Type */
	oh_append_offset(buffer, offsets);
	snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "Type: %s\n", 
		 oh_lookup_sensortype(sensor->Type));
	oh_append_bigtext(buffer, str, strlen(str));

	/* Sensor Category */
	oh_append_offset(buffer, offsets);
	snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "Category: %s\n", 
		 oh_lookup_eventcategory(sensor->Category));
	oh_append_bigtext(buffer, str, strlen(str));

	/* Sensor Enable Control */
	oh_append_offset(buffer, offsets);
	snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "EnableCtrl: %s\n",
		(sensor->EnableCtrl == SAHPI_TRUE) ? "TRUE" : "FALSE");
	oh_append_bigtext(buffer, str, strlen(str));

	/* Sensor Event Control */
	oh_append_offset(buffer, offsets);
	snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "EventCtrl: %s\n", 
		 oh_lookup_sensoreventctrl(sensor->EventCtrl));
	oh_append_bigtext(buffer, str, strlen(str));

	/* Sensor Supported Events */
	{
		SaHpiTextBufferT evt_buffer;

		oh_append_offset(buffer, offsets);
	        oh_append_bigtext(buffer, "Events: ", strlen("Events: "));
		err = oh_decode_eventstate(sensor->Events, sensor->Category, &evt_buffer);
		if (err != SA_OK) { return(err); }
		oh_append_bigtext(buffer, evt_buffer.Data, strlen(evt_buffer.Data));
		oh_append_bigtext(buffer, "\n", strlen("\n"));
	}
	
	/* Sensor Data Format */
	err = oh_build_sensordataformat(buffer, &(sensor->DataFormat), offsets);
	if (err != SA_OK) { return(err); }

	/* Sensor Threshold Definition */
	err = oh_build_sensorthddefn(buffer, &(sensor->ThresholdDefn), offsets);
	if (err != SA_OK) { return(err); }

	/* Sensor OEM Data */
	oh_append_offset(buffer, offsets);
	snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "OEM: %x\n", sensor->Oem);
	oh_append_bigtext(buffer, str, strlen(str));

	/* printf("SENSOR LENGTH = %d\n", strlen(buffer->Data)); */
	return(SA_OK);
}

static SaErrorT oh_build_sensordataformat(oh_big_textbuffer *buffer,
					  const SaHpiSensorDataFormatT *format, 
					  int offsets)
{
	char str[SAHPI_MAX_TEXT_BUFFER_LENGTH];
	SaErrorT err;
	SaHpiTextBufferT reading_buffer;

	/* Sensor Data Format Title */
	oh_append_offset(buffer, offsets);
	oh_append_bigtext(buffer, "Data Format:\n", strlen("Data Format:\n"));
	offsets++;
		
	/* Sensor Data Format IsSupported */
	oh_append_offset(buffer, offsets);
	snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "IsSupported: %s\n",
		 (format->IsSupported == SAHPI_TRUE) ? "TRUE" : "FALSE");
	oh_append_bigtext(buffer, str, strlen(str));
		
	if (format->IsSupported) {

		/* Sensor Data Format Reading Type */
		oh_append_offset(buffer, offsets);
		snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "Reading Type: %s\n", 
			 oh_lookup_sensorreadingtype(format->ReadingType));
		oh_append_bigtext(buffer, str, strlen(str));
		
		if (format->ReadingType != SAHPI_SENSOR_READING_TYPE_BUFFER) {

			/* Sensor Data Format Base Units */
			oh_append_offset(buffer, offsets);
			snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "Base Unit: %s\n", 
				 oh_lookup_sensorunits(format->BaseUnits));
			oh_append_bigtext(buffer, str, strlen(str));
			
			/* Sensor Data Format Modifier Units */
			if (format->ModifierUnits) {
				oh_append_offset(buffer, offsets);
				snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "Modifier Unit: %s\n", 
					 oh_lookup_sensorunits(format->ModifierUnits));
				oh_append_bigtext(buffer, str, strlen(str));
				/* Sensor Data Format Modifier Use */
				oh_append_offset(buffer, offsets);
				snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "Modifier Use: %s\n",
					 oh_lookup_sensormodunituse(format->ModifierUse));
				oh_append_bigtext(buffer, str, strlen(str));
			}
			
			/* Sensor Data Format Percentage */
			oh_append_offset(buffer, offsets);
			snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "Percentage: %s\n",
				 (format->Percentage == SAHPI_TRUE) ? "TRUE" : "FALSE");
			oh_append_bigtext(buffer, str, strlen(str));
		
			/* Sensor Data Format Max Range */
			if (format->Range.Flags & SAHPI_SRF_MAX && 
			    format->Range.Max.IsSupported) { 
				oh_append_offset(buffer, offsets);
				oh_append_bigtext(buffer, "Range Max: ", strlen("Range Max: "));
				
				err = oh_decode_sensorreading(format->Range.Max,
							      *format,
							      &reading_buffer);
				if (err) { return(err); }
				oh_append_bigtext(buffer, reading_buffer.Data, strlen(reading_buffer.Data));
				oh_append_bigtext(buffer, "\n", strlen("\n"));
			}

			/* Sensor Data Format Min Range */
			if (format->Range.Flags & SAHPI_SRF_MIN && 
			    format->Range.Min.IsSupported) { 
				oh_append_offset(buffer, offsets);
				oh_append_bigtext(buffer, "Range Min: ", strlen("Range Min: "));
				
				err = oh_decode_sensorreading(format->Range.Min,
							      *format,
							      &reading_buffer);
				if (err) { return(err); }
				oh_append_bigtext(buffer, reading_buffer.Data, strlen(reading_buffer.Data));
				oh_append_bigtext(buffer, "\n", strlen("\n"));
			}

			/* Sensor Data Format Nominal Range */
			if (format->Range.Flags & SAHPI_SRF_NOMINAL && 
			    format->Range.Nominal.IsSupported) { 
				oh_append_offset(buffer, offsets);
				oh_append_bigtext(buffer, "Range Nominal: ", strlen("Range Nominal: "));
				
				err = oh_decode_sensorreading(format->Range.Nominal,
							      *format,
							      &reading_buffer);
				if (err) { return(err); }
				oh_append_bigtext(buffer, reading_buffer.Data, strlen(reading_buffer.Data));
				oh_append_bigtext(buffer, "\n", strlen("\n"));
			}

			/* Sensor Data Format Normal Max Range */
			if (format->Range.Flags & SAHPI_SRF_NORMAL_MAX && 
			    format->Range.NormalMax.IsSupported) { 
				oh_append_offset(buffer, offsets);
				oh_append_bigtext(buffer, "Range Normal Max: ", strlen("Range Normal Max: "));
				
				err = oh_decode_sensorreading(format->Range.NormalMax,
							      *format,
							      &reading_buffer);
				if (err) { return(err); }
				oh_append_bigtext(buffer, reading_buffer.Data, strlen(reading_buffer.Data));
				oh_append_bigtext(buffer, "\n", strlen("\n"));
			}

			/* Sensor Data Format Normal Min Range */
			if (format->Range.Flags & SAHPI_SRF_NORMAL_MIN && 
			    format->Range.NormalMin.IsSupported) { 
				oh_append_offset(buffer, offsets);
				oh_append_bigtext(buffer, "Range Normal Min: ", strlen("Range Normal Min: "));
				
				err = oh_decode_sensorreading(format->Range.NormalMin,
							      *format,
							      &reading_buffer);
				if (err) { return(err); }
				oh_append_bigtext(buffer, reading_buffer.Data, strlen(reading_buffer.Data));
				oh_append_bigtext(buffer, "\n", strlen("\n"));
			}

			/* Sensor Data Format Accuracy Factor */
			oh_append_offset(buffer, offsets);
			snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "Accuracy: %le\n", format->AccuracyFactor);	
			oh_append_bigtext(buffer, str, strlen(str));
		}
	}
	
	return(SA_OK);
}

static SaErrorT oh_build_sensorthddefn(oh_big_textbuffer *buffer,
				       const SaHpiSensorThdDefnT *tdef, 
				       int offsets)
{
	char str[SAHPI_MAX_TEXT_BUFFER_LENGTH];
	SaErrorT err;

	/* Sensor Threshold Definition Title */
	oh_append_offset(buffer, offsets);
	oh_append_bigtext(buffer, "Threshold Definitions:\n", strlen("Threshold Definitions:\n"));
	offsets++;

	/* Sensor Threshold Definition IsAccessible */
	oh_append_offset(buffer, offsets);
	snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "IsAccessible: %s\n",
		 (tdef->IsAccessible == SAHPI_TRUE) ? "TRUE" : "FALSE");
	oh_append_bigtext(buffer, str, strlen(str));
	
	if (tdef->IsAccessible) {

		/* Sensor Threshold Read Threshold */
		if (tdef->ReadThold) {
			oh_append_offset(buffer, offsets);
			oh_append_bigtext(buffer, "Readable Thresholds:\n", 
						 strlen("Readable Thresholds:\n"));

			err = oh_build_threshold_mask(buffer, tdef->ReadThold, offsets + 1);
			if (err != SA_OK) { return(err); }
		}
		
		/* Sensor Threshold Write Threshold */
		if (tdef->WriteThold) {
			oh_append_offset(buffer, offsets);
			oh_append_bigtext(buffer, "Writeable Thresholds:\n", 
						 strlen("Writeable Thresholds:\n"));

			err = oh_build_threshold_mask(buffer, tdef->WriteThold, offsets + 1);
			if (err != SA_OK) { return(err); }
		}

		/* Sensor Threshold Nonlinear */
		oh_append_offset(buffer, offsets);
		snprintf(str, SAHPI_MAX_TEXT_BUFFER_LENGTH, "Nonlinear: %s\n",
			 (tdef->Nonlinear == SAHPI_TRUE) ? "TRUE" : "FALSE");
		oh_append_bigtext(buffer, str, strlen(str));
	}	

	return(SA_OK);
}


static SaErrorT oh_build_threshold_mask(oh_big_textbuffer *buffer,
					const SaHpiSensorThdMaskT tmask, 
					int offsets)
{
	int i;

	oh_append_offset(buffer, offsets);

	if (tmask & SAHPI_STM_LOW_MINOR) {
		oh_append_bigtext(buffer, "LOW_MINOR", strlen("LOW_MINOR"));
		oh_append_bigtext(buffer, OH_ENCODE_DELIMITER, OH_ENCODE_DELIMITER_LENGTH);
	}
	if (tmask & SAHPI_STM_LOW_MAJOR) {
		oh_append_bigtext(buffer, "LOW_MAJOR", strlen("LOW_MAJOR"));
		oh_append_bigtext(buffer, OH_ENCODE_DELIMITER, OH_ENCODE_DELIMITER_LENGTH);
	}
	if (tmask & SAHPI_STM_LOW_CRIT) {
		oh_append_bigtext(buffer, "LOW_CRIT", strlen("LOW_CRIT"));
		oh_append_bigtext(buffer, OH_ENCODE_DELIMITER, OH_ENCODE_DELIMITER_LENGTH);
	}
	if (tmask & SAHPI_STM_LOW_HYSTERESIS) {
		oh_append_bigtext(buffer, "LOW_HYSTERESIS", strlen("LOW_HYSTERESIS"));
		oh_append_bigtext(buffer, OH_ENCODE_DELIMITER, OH_ENCODE_DELIMITER_LENGTH);
	}
	if (tmask & SAHPI_STM_UP_MINOR) {
		oh_append_bigtext(buffer, "UP_MINOR", strlen("UP_MINOR"));
		oh_append_bigtext(buffer, OH_ENCODE_DELIMITER, OH_ENCODE_DELIMITER_LENGTH);
	}
	if (tmask & SAHPI_STM_UP_MAJOR) {
		oh_append_bigtext(buffer, "UP_MAJOR", strlen("UP_MAJOR"));
		oh_append_bigtext(buffer, OH_ENCODE_DELIMITER, OH_ENCODE_DELIMITER_LENGTH);
	}
	if (tmask & SAHPI_STM_UP_CRIT) {
		oh_append_bigtext(buffer, "UP_CRIT", strlen("UP_CRIT"));
		oh_append_bigtext(buffer, OH_ENCODE_DELIMITER, OH_ENCODE_DELIMITER_LENGTH);
	}
	if (tmask & SAHPI_STM_UP_HYSTERESIS) {
		oh_append_bigtext(buffer, "UP_HYSTERESIS", strlen("UP_HYSTERESIS"));
		oh_append_bigtext(buffer, OH_ENCODE_DELIMITER, OH_ENCODE_DELIMITER_LENGTH);
	}

	/* Remove last delimiter; add NL */
	for (i=0; i<OH_ENCODE_DELIMITER_LENGTH + 1; i++) {
		buffer->Data[buffer->DataLength - i] = 0x00;
	}
	buffer->DataLength = buffer->DataLength - (i-1);
	oh_append_bigtext(buffer, "\n", strlen("\n"));

	return(SA_OK);
}

/**
 * oh_fprint_idrfield:
 * @stream: File handle.
 * @thisfield: Pointer to SaHpiIdrFieldT to be printed.
 * @space:  Number of blank space to skip (from column 1) before printing
 * 
 * Prints the member data contained in SaHpiIdrFieldT struct to a file.
 * The MACRO oh_print_idrfield(), uses this function to print to STDOUT. 
 *
 * Returns:
 * SA_OK - normal operation.
 **/
SaErrorT oh_fprint_idrfield(FILE *stream, const SaHpiIdrFieldT *thisfield, int space)
{
	int err;
	SaErrorT rv = SA_OK;

	if (!stream || !thisfield) 
		return(SA_ERR_HPI_INVALID_PARAMS);
			
	put_spacing(space);
	err = fprintf(stream, "Field Id:\t%d\n", thisfield->FieldId);
	check_err(err);
	
	put_spacing(space);
        err = fprintf(stream, "Field Type:\t%s\n", oh_lookup_idrfieldtype(thisfield->Type));
	check_err(err);
	
	put_spacing(space);
        err = fprintf(stream, "ReadOnly:\t%d\n", thisfield->ReadOnly); 
	check_err(err);
	
	put_spacing(space);
        err = fprintf(stream, "DataType:\t%s\n", oh_lookup_texttype(thisfield->Field.DataType));
	check_err(err);
	
	put_spacing(space);
        err = fprintf(stream, "Language:\t%s\n", oh_lookup_language(thisfield->Field.Language));;
	check_err(err);
	
	put_spacing(space);
	err = fprintf(stream, "Content:\t");
	rv =  oh_fprint_text(stream, &thisfield->Field);
	err = fprintf(stream, "\n");
	check_err(err);
			
	return(rv);
}

/**
 * oh_fprint_idrareaheader:
 * @stream: File handle.
 * @areaheader: Pointer to SaHpiIdrAreaHeaderT to be printed.
 * @space:  Number of blank space to skip (from column 1) before printing
 * 
 * Prints the member data contained in SaHpiIdrAreaHeaderT struct to a file.
 * The MACRO oh_print_idrareaheader(), uses this function to print to STDOUT. 
 *
 * Returns:       
 * SA_OK - normal operation.
 **/
SaErrorT oh_fprint_idrareaheader(FILE *stream, const SaHpiIdrAreaHeaderT *areaheader, int space)
{
	int err;

	if (!stream || !areaheader) 
		return(SA_ERR_HPI_INVALID_PARAMS);
			
	
	put_spacing(space);
        err = fprintf(stream, "AreaId:\t%d\n", areaheader->AreaId);
	check_err(err);
			
	put_spacing(space);
        err = fprintf(stream, "AreaType:\t%s\n", oh_lookup_idrareatype(areaheader->Type));
	check_err(err);
			
	put_spacing(space);
        err = fprintf(stream, "ReadOnly:\t%d\n", areaheader->ReadOnly);
	check_err(err);

	put_spacing(space);
        err = fprintf(stream, "NumFields:\t%d\n",areaheader->NumFields);	
	check_err(err);
			
	return(SA_OK);
}

/**
 * oh_fprint_idrinfo:
 * @stream: File handle.
 * @idrinfo: Pointer to SaHpiIdrInfoT to be printed.
 * @space:  Number of blank space to skip (from column 1) before printing
 * 
 * Prints the member data contained in SaHpiIdrInfoT struct to a file.
 * The MACRO oh_print_idrinfo(), uses this function to print to STDOUT. 
 *
 * Returns:       
 * SA_OK - normal operation.
 **/
SaErrorT oh_fprint_idrinfo(FILE *stream, const SaHpiIdrInfoT *idrinfo, int space)
{
	int err;
	
	if (!stream || !idrinfo) 
		return(SA_ERR_HPI_INVALID_PARAMS);
			
	put_spacing(space);
        err = fprintf(stream, "IdrId:\t%d\n", idrinfo->IdrId);
	check_err(err);
			
	put_spacing(space);
        err = fprintf(stream, "UpdateCount:\t%d\n", idrinfo->UpdateCount);
	check_err(err);
			
	put_spacing(space);
        err = fprintf(stream, "ReadOnly:\t%d\n",idrinfo->ReadOnly);
	check_err(err);
			
	put_spacing(space);
        err = fprintf(stream, "NumAreas:\t%d\n", idrinfo->NumAreas);
	check_err(err);
			
	return(SA_OK);
}

#if 0
SaHpiBoolT oh_valid_textbuffer(SaHpiTextBufferT text_buffer) {

	if (!valid_SaHpiTextTypeT) { return(SAHPI_FALSE); }
	if (!valid_SaHpiLanguageT) { return(SAHPI_FALSE); }
	/* Compiler checks DataLength <= SAHPI_MAX_TEXT_BUFFER_LENGTH */

	switch(text_buffer.DataType){
	case SAHPI_TL_TYPE_UNICODE:
		
		/* FIXME:: Verify unicode characters in Data */
		/* Check for even number of bytes */
		/* g_unichar_validate(gunichar ch); ???? */
		break;
	case SAHPI_TL_TYPE_BCDPLUS:

		/* FIXME:: Verify BCDPLUS characters in Data */
		/* 8-bit ASCII, '0'-'9' or space, dash, period, colon, comma, or
		   underscore only.(*/
		break;
	case SAHPI_TL_TYPE_ASCII6:
		/* FIXME:: Verify reduced character set in Data */
		/* reduced set, 0x20-0x5f only. */
		break;
	case SAHPI_TL_TYPE_TEXT:
		/* FIXME:: Any check possible for 8-bit ASCII + Latin 1? */
		break;
	case SAHPI_TL_TYPE_BINARY: /* No check possible */
		break;
	default: /* Impossible state */
		return(SA_ERR_HPI_INTERNAL_ERROR);
	}

	return(SAHPI_TRUE);
}

SaErrorT oh_print_text(SaHpiTextBufferT text_buffer) {

	/* Check for valid */
        /* Support arbitrary tab/indents offsets??*/

	SaHpiUint8T Data[SAHPI_MAX_TEXT_BUFFER_LENGTH];
	
	memset(Data, 0, SAHPI_MAX_TEXT_BUFFER_LENGTH);
        memcpy(Data, text_buffer.Data, text_buffer.DataLength);

	printf("TextBuffer:\n");
	printf("\tDataType: %s\n", SaHpiTextTypeT2str(text_buffer.DataType));
	printf("\tLanguage: %s\n", SaHpiLanguageT2str(text_buffer.Language));
	printf("\tDataLength: %d\n", text_buffer.DataLength);

	switch (text_buffer.DataType) {	
	case SAHPI_TL_TYPE_UNICODE:
		/* FIXME:: print unicode */
		break;
	case SAHPI_TL_TYPE_BCDPLUS:
	case SAHPI_TL_TYPE_ASCII6:
	case SAHPI_TL_TYPE_TEXT:
		printf("\tData: %s\n", Data);
		break;
	case SAHPI_TL_TYPE_BINARY:
		printf("\tData: %x\n", Data);
		break;
	default:
		printf("\tData: ERROR! Invalid HPI SaHpiTextTypeT type\n");
	}

	return SA_OK;
}

#endif
