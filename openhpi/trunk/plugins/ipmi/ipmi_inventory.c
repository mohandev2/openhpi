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
 *     Racing Guo <racing.guo@intel.com>
 *     Vadim Revyakin <vadim.a.revyakin@intel.com>
 */

#include "ipmi.h"
#include <oh_utils.h>
#include <string.h>

#define OHOI_IDR_DEFAULT_ID         0
#define OHOI_CHECK_RPT_CAP_IDR()                                     \
do{                                                                  \
	SaHpiRptEntryT           *rpt_entry;                         \
	rpt_entry = oh_get_resource_by_id(handler->rptcache, rid);   \
	if (!rpt_entry) {                                            \
		dbg("Resource %d No rptentry", rid);                 \
		return SA_ERR_HPI_INVALID_PARAMS;                    \
	}                                                            \
	if (!(rpt_entry->ResourceCapabilities &                      \
	    SAHPI_CAPABILITY_INVENTORY_DATA)) {                      \
		dbg("Resource %d no inventory capability", rid);     \
		return SA_ERR_HPI_INVALID_PARAMS;                    \
	}                                                            \
	if (idrid != OHOI_IDR_DEFAULT_ID) {                          \
		dbg("error id");                                     \
		return SA_ERR_HPI_NOT_PRESENT;                    \
	}                                                            \
}while(0)

struct ohoi_field_data {
	SaHpiIdrFieldTypeT fieldtype;
	SaHpiLanguageT lang;
	int (*get_len)(ipmi_entity_t *, unsigned int*);
	int (*get_data)(ipmi_entity_t *, char*, unsigned int*);
	int (*get_type)(ipmi_fru_t *fru, enum ipmi_str_type_e *type);
};

static struct ohoi_field_data chassis_fields[] = {
	{
		SAHPI_IDR_FIELDTYPE_SERIAL_NUMBER,
		SAHPI_LANG_ENGLISH, 
		ipmi_entity_get_chassis_info_serial_number_len,
		ipmi_entity_get_chassis_info_serial_number,
		ipmi_fru_get_chassis_info_serial_number_type,
	},
	{
		SAHPI_IDR_FIELDTYPE_PART_NUMBER,
		SAHPI_LANG_UNDEF,
		ipmi_entity_get_chassis_info_part_number_len,
		ipmi_entity_get_chassis_info_part_number,
		ipmi_fru_get_chassis_info_part_number_type,
	},
	{
		SAHPI_IDR_FIELDTYPE_CUSTOM,
		SAHPI_LANG_UNDEF,
		NULL,
		NULL,
		NULL,
	}
};

static struct ohoi_field_data board_fields[] = {
	{
		SAHPI_IDR_FIELDTYPE_MANUFACTURER,
		SAHPI_LANG_UNDEF,
		ipmi_entity_get_board_info_board_manufacturer_len, 
		ipmi_entity_get_board_info_board_manufacturer,
		ipmi_fru_get_board_info_board_manufacturer_type
	},
	{
		SAHPI_IDR_FIELDTYPE_PRODUCT_NAME,
		SAHPI_LANG_UNDEF, 
		ipmi_entity_get_board_info_board_product_name_len, 
		ipmi_entity_get_board_info_board_product_name,
		ipmi_fru_get_board_info_board_product_name_type,
	},
	{
		SAHPI_IDR_FIELDTYPE_SERIAL_NUMBER,
		SAHPI_LANG_ENGLISH, 
		ipmi_entity_get_board_info_board_serial_number_len, 
		ipmi_entity_get_board_info_board_serial_number,
		ipmi_fru_get_board_info_board_serial_number_type
	},
	{
		SAHPI_IDR_FIELDTYPE_PART_NUMBER,
		SAHPI_LANG_UNDEF, 
		ipmi_entity_get_board_info_board_part_number_len, 
		ipmi_entity_get_board_info_board_part_number,
		ipmi_fru_get_board_info_board_part_number_type
	},
	{
		SAHPI_IDR_FIELDTYPE_FILE_ID,
		SAHPI_LANG_ENGLISH, 
		ipmi_entity_get_board_info_fru_file_id_len, 
		ipmi_entity_get_board_info_fru_file_id,
		ipmi_fru_get_board_info_fru_file_id_type
	},
	{
		SAHPI_IDR_FIELDTYPE_CUSTOM,
		SAHPI_LANG_UNDEF,
		NULL,
		NULL,
		NULL,
	}
};

static int _ipmi_entity_get_product_info_custom_len(ipmi_entity_t *entity,
				      unsigned int  *len)
{
	return ipmi_entity_get_product_info_custom_len(entity,0,len);
}

static int _ipmi_entity_get_product_info_custom(ipmi_entity_t *entity,
				      char *data, unsigned int  *max_len)
{
	return ipmi_entity_get_product_info_custom(entity,0,data,max_len);
}

static int _ipmi_fru_get_product_info_custom_type(ipmi_fru_t *fru,
				      enum ipmi_str_type_e  *type)
{
	return ipmi_fru_get_product_info_custom_type(fru, 0, type);
}

static struct ohoi_field_data product_fields[] = {
	{
		SAHPI_IDR_FIELDTYPE_MANUFACTURER,
		SAHPI_LANG_UNDEF, 
		ipmi_entity_get_product_info_manufacturer_name_len, 
		ipmi_entity_get_product_info_manufacturer_name,
		ipmi_fru_get_product_info_manufacturer_name_type,
	},
	{
		SAHPI_IDR_FIELDTYPE_PRODUCT_NAME,
		SAHPI_LANG_UNDEF,
		ipmi_entity_get_product_info_product_name_len,
		ipmi_entity_get_product_info_product_name,
		ipmi_fru_get_product_info_product_name_type,
	},
	{
		SAHPI_IDR_FIELDTYPE_SERIAL_NUMBER,
		SAHPI_LANG_ENGLISH,
		ipmi_entity_get_product_info_product_serial_number_len,
		ipmi_entity_get_product_info_product_serial_number,
		ipmi_fru_get_product_info_product_serial_number_type,
	},
	{
		SAHPI_IDR_FIELDTYPE_FILE_ID,
		SAHPI_LANG_UNDEF,
		ipmi_entity_get_product_info_fru_file_id_len,
		ipmi_entity_get_product_info_fru_file_id,
		ipmi_fru_get_product_info_fru_file_id_type,
	},
	{
		SAHPI_IDR_FIELDTYPE_ASSET_TAG,
		SAHPI_LANG_UNDEF,
		ipmi_entity_get_product_info_asset_tag_len,
		ipmi_entity_get_product_info_asset_tag,
		ipmi_fru_get_product_info_asset_tag_type,
	},
	{
		SAHPI_IDR_FIELDTYPE_CUSTOM,
		SAHPI_LANG_UNDEF,
		_ipmi_entity_get_product_info_custom_len,
		_ipmi_entity_get_product_info_custom,
		_ipmi_fru_get_product_info_custom_type,
	}
};

static int _ipmi_entity_get_internal_use_data(ipmi_entity_t *entity,
					      char *data,
					      unsigned int  *max_len)
{
	return ipmi_entity_get_internal_use_data(entity, data, max_len);
}
static int _ipmi_fru_get_multi_record_type(ipmi_fru_t    *fru,
						enum ipmi_str_type_e *type)
{
	return ipmi_fru_get_multi_record_type(fru, 0, (char *)type);
}

static struct ohoi_field_data internal_fields[] = {
	{
		SAHPI_IDR_FIELDTYPE_CUSTOM,
		SAHPI_LANG_UNDEF,
		ipmi_entity_get_internal_use_length,
		_ipmi_entity_get_internal_use_data,
		_ipmi_fru_get_multi_record_type
	},
	{
		SAHPI_IDR_FIELDTYPE_CUSTOM,
		SAHPI_LANG_UNDEF,
		NULL,
		NULL,
		NULL,
	}
};

static struct ohoi_area_data {
	int field_num;
	SaHpiIdrAreaTypeT areatype;
	SaHpiBoolT read_only;
	struct ohoi_field_data *fields;
} areas [] = {
	{
		sizeof(internal_fields)/sizeof(internal_fields[0]),
		SAHPI_IDR_AREATYPE_INTERNAL_USE,
		SAHPI_TRUE,
		internal_fields
	},
	{	sizeof(chassis_fields)/sizeof(chassis_fields[0]),
		SAHPI_IDR_AREATYPE_CHASSIS_INFO,
		SAHPI_FALSE,
		chassis_fields
	}, 
	{
		sizeof(board_fields)/sizeof(board_fields[0]),
		SAHPI_IDR_AREATYPE_BOARD_INFO,
		SAHPI_FALSE,
		board_fields
	},
	{
		sizeof(product_fields)/sizeof(product_fields[0]),
		SAHPI_IDR_AREATYPE_PRODUCT_INFO,
		SAHPI_FALSE,
		product_fields
	},
	{
		0,
		SAHPI_IDR_AREATYPE_OEM,
		SAHPI_TRUE,
		NULL
	}
};

#define OHOI_AREA_NUM        (sizeof(areas)/sizeof(areas[0]))
#define OHOI_AREA_EMPTY_ID   0
#define OHOI_AREA_FIRST_ID   1
#define OHOI_AREA_LAST_ID    OHOI_AREA_NUM

#define OHOI_FIELD_NUM(area)         (area->field_num)
#define OHOI_FIELD_EMPTY_ID          0
#define OHOI_FIELD_FIRST_ID          1
#define OHOI_FIELD_LAST_ID(area)     (area->field_num)

static int get_first_area(SaHpiIdrAreaTypeT areatype)
{
	int i;
	for (i = 0; i < OHOI_AREA_NUM; i++)
		if (areas[i].areatype == areatype) {
			return (i + 1);
		}
	return OHOI_AREA_EMPTY_ID;
}

static int get_first_field(struct ohoi_area_data *area,
			   struct ohoi_inventory_info *fru)
{
	int i;
	unsigned int *msk;

	switch (area->areatype) {
	case SAHPI_IDR_AREATYPE_INTERNAL_USE:
		return 1;
	case SAHPI_IDR_AREATYPE_CHASSIS_INFO:
		msk = &fru->ci_fld_msk;
		break;
	case SAHPI_IDR_AREATYPE_BOARD_INFO:
		msk = &fru->bi_fld_msk;
		break;	
	case SAHPI_IDR_AREATYPE_PRODUCT_INFO:
		msk = &fru->pi_fld_msk;
		break;
	default:
		return OHOI_FIELD_EMPTY_ID;	
	}

	for (i = 0; i < OHOI_FIELD_NUM(area); i++) {
		if ((1 << area->fields[i].fieldtype) & *msk) {
			return (i + 1);
		}
	}
	return OHOI_FIELD_EMPTY_ID;
}

static int get_first_typed_field(struct ohoi_area_data *area,
			   SaHpiIdrFieldTypeT fieldtype,
			   struct ohoi_inventory_info *fru)
{
	int i;
	unsigned int *msk;
	
	switch (area->areatype) {
	case SAHPI_IDR_AREATYPE_INTERNAL_USE:
		return 1;
	case SAHPI_IDR_AREATYPE_CHASSIS_INFO:
		msk = &fru->ci_fld_msk;
		break;
	case SAHPI_IDR_AREATYPE_BOARD_INFO:
		msk = &fru->bi_fld_msk;
		break;	
	case SAHPI_IDR_AREATYPE_PRODUCT_INFO:
		msk = &fru->pi_fld_msk;
		break;
	default:
		return OHOI_FIELD_EMPTY_ID;	
	}
	if (!((1 << fieldtype) & *msk)) {
		return OHOI_FIELD_EMPTY_ID;
	}
	for (i = 0; i < OHOI_FIELD_NUM(area); i++) {
		if (area->fields[i].fieldtype ==  fieldtype) {
			return (i + 1);
		}
	}
	return OHOI_FIELD_EMPTY_ID;
}

SaHpiTextTypeT convert_to_hpi_data_type(enum ipmi_str_type_e type)
{
	switch (type) {
	case IPMI_ASCII_STR:
		return SAHPI_TL_TYPE_TEXT;
	case IPMI_UNICODE_STR:
		return SAHPI_TL_TYPE_UNICODE;
	case IPMI_BINARY_STR:
		return SAHPI_TL_TYPE_BINARY;
	}
	return SAHPI_TL_TYPE_BINARY;
}
 

static unsigned char get_area_presence(struct ohoi_inventory_info *i_info,
				   SaHpiIdrAreaTypeT areatype)
{
	switch (areatype) {
	case SAHPI_IDR_AREATYPE_BOARD_INFO:
		return i_info->bi;
	case SAHPI_IDR_AREATYPE_PRODUCT_INFO:
		return i_info->pi;
	case SAHPI_IDR_AREATYPE_CHASSIS_INFO:
		return i_info->ci;
	case SAHPI_IDR_AREATYPE_OEM:
		return i_info->oem;
	case SAHPI_IDR_AREATYPE_INTERNAL_USE:
		return i_info->iu;
	default:
		return (unsigned char)0;
	}
}

#if 0
static SaHpiBoolT valid_area_type(SaHpiIdrAreaTypeT areatype)
{
	switch(areatype) {
	case SAHPI_IDR_AREATYPE_INTERNAL_USE:
	case SAHPI_IDR_AREATYPE_CHASSIS_INFO:
	case SAHPI_IDR_AREATYPE_BOARD_INFO:
	case SAHPI_IDR_AREATYPE_PRODUCT_INFO:
	case SAHPI_IDR_AREATYPE_OEM:
	case SAHPI_IDR_AREATYPE_UNSPECIFIED:
		return SAHPI_TRUE;
	default:
		return SAHPI_FALSE;
	}
}
#endif

static SaHpiLanguageT get_language(struct ohoi_inventory_info *i_info,
				   SaHpiIdrAreaTypeT areatype)
{
	switch (areatype) {
	case SAHPI_IDR_AREATYPE_BOARD_INFO:
		return i_info->bi;
	case SAHPI_IDR_AREATYPE_PRODUCT_INFO:
		return i_info->pi;
	default:
		return SAHPI_LANG_UNDEF;
	}
}



struct ohoi_get_field {
	struct ohoi_field_data *data;
	SaHpiIdrFieldT *field;
	int done;
	SaErrorT rv;
};

static void get_field(ipmi_entity_t *ent,
		      void          *cb_data)
{
	int rv;
	int len;
	enum ipmi_str_type_e type;
	ipmi_fru_t *fru;
	int (*get_len)(ipmi_entity_t *, unsigned int*);
	int (*get_data)(ipmi_entity_t *, char*, unsigned int*);
	struct ohoi_get_field *gf = cb_data;
	struct ohoi_field_data *data = gf->data;
	
	SaHpiIdrFieldT *field = gf->field;
	get_len = data->get_len;
	get_data = data->get_data;
	
	field->Type = data->fieldtype;
	field->ReadOnly = SAHPI_FALSE;

	gf->done = 1;
	fru = ipmi_entity_get_fru(ent);
	if (fru == NULL) {
		dbg("Bug: entity without fru");
		gf->rv = SA_ERR_HPI_INTERNAL_ERROR;
		return;
	}
	
	rv = data->get_type(fru, &type);
	if (rv) {
		dbg("Could not get data type = %d. set SAHPI_TL_TYPE_BINARY", rv);
		field->Field.DataType = SAHPI_TL_TYPE_BINARY;
	} else {
		field->Field.DataType = convert_to_hpi_data_type(type);
	}
	field->Field.Language = SAHPI_LANG_ENGLISH;
	field->Field.DataLength = 0;

	rv = get_len(ent, &len);
	if (rv) {
		dbg("Error on get_len: %d", rv);
		gf->rv = SA_ERR_HPI_NOT_PRESENT;
		return;
	}
	len++;  /* if ASCII string (yes), add one for NULL char. */

	if (len > SAHPI_MAX_TEXT_BUFFER_LENGTH)
		len = SAHPI_MAX_TEXT_BUFFER_LENGTH;

	rv = get_data(ent, &field->Field.Data[0], &len);
	if (!rv) {
		field->Field.DataLength = len;
	} else {
		dbg("Error on  get_data: %d", rv);
		gf->rv = SA_ERR_HPI_INTERNAL_ERROR;
	}
}

struct oem_idr_field {
	SaHpiIdrFieldT *field;
	SaErrorT rv;
	int done;
};

static void get_oem_idr_field_cb(ipmi_entity_t *ent, void *cbdata)
{
	struct oem_idr_field *oif = cbdata;
	int rv;
	unsigned int len;
	unsigned char ver, type;
	unsigned int f_id = oif->field->FieldId - 1;
	
	oif->done = 1;
	rv = ipmi_entity_get_multi_record_data_len(ent, f_id, &len);
	if (rv) {
		dbg("ipmi_entity_get_multi_record_data_len = %d", rv);
		oif->rv = SA_ERR_HPI_NOT_PRESENT;
		return;
	}
	rv = ipmi_entity_get_multi_record_type(ent, f_id, &type);
	if (rv) {
		dbg("ipmi_entity_get_multi_record_type = %d", rv);
		oif->rv = SA_ERR_HPI_NOT_PRESENT;
		return;
	}
	rv = ipmi_entity_get_multi_record_format_version(ent, f_id, &ver);
	if (rv) {
		dbg("ipmi_entity_get_multi_record_format_version = %d", rv);
		oif->rv = SA_ERR_HPI_NOT_PRESENT;
		return;
	}
	if (len > SAHPI_MAX_TEXT_BUFFER_LENGTH - 2) {
		len = SAHPI_MAX_TEXT_BUFFER_LENGTH - 2;
	}
	rv = ipmi_entity_get_multi_record_data(ent, f_id,
		&oif->field->Field.Data[2], &len);
	if (rv) {
		dbg("ipmi_entity_get_multi_record_data = %d", rv);
		oif->rv = SA_ERR_HPI_NOT_PRESENT;
		return;
	}
	oif->field->Field.Data[0] = type;
	oif->field->Field.Data[1] = ver;
	oif->field->Field.DataLength = len + 2;
	oif->field->Field.DataType = SAHPI_TL_TYPE_BINARY;
	oif->rv = SA_OK;
}


static SaErrorT get_oem_idr_field(struct oh_handler_state  *handler,
				struct ohoi_resource_info   *ohoi_res_info,
				SaHpiIdrFieldTypeT fieldtype,
				SaHpiEntryIdT fieldid,
				SaHpiEntryIdT *nextfieldid,
				SaHpiIdrFieldT *field)
{
	struct ohoi_inventory_info *i_info = ohoi_res_info->fru;
	struct oem_idr_field oif;
	int rv;
	
	if (fieldtype != SAHPI_IDR_FIELDTYPE_UNSPECIFIED) {
		return SA_ERR_HPI_INVALID_PARAMS;
	}
	if (fieldid == SAHPI_FIRST_ENTRY) {
		fieldid = 1;
	}
	if (fieldid > i_info->oem_fields_num) {
		dbg("fieldid(%d) > i_info->oem_fields_num(%d)",
			fieldid, i_info->oem_fields_num);
		return SA_ERR_HPI_NOT_PRESENT;
	}
	if (fieldid < 1) {
		dbg("fieldid(%d) < 1", fieldid);
		return SA_ERR_HPI_NOT_PRESENT;
	}
	field->FieldId = fieldid;
	field->Type = SAHPI_IDR_FIELDTYPE_UNSPECIFIED;
	oif.done = 0;
	oif.rv = SA_OK;
	oif.field = field;
	rv = ipmi_entity_pointer_cb(ohoi_res_info->u.entity_id,
		get_oem_idr_field_cb, &oif);
	if (rv) {
		dbg("ipmi_entity_pointer_cb returned %d", rv);
		oif.rv = SA_ERR_HPI_INTERNAL_ERROR;
	} else {
		oif.rv = ohoi_loop(&oif.done, handler->data);
	}
	if (oif.rv != SA_OK) {
		dbg("get_oem_idr_field. rv = %d", oif.rv);
	} else if (fieldid < i_info->oem_fields_num) {
		*nextfieldid = fieldid + 1;
	} else {
		*nextfieldid = SAHPI_LAST_ENTRY;
	}
		
	return oif.rv; 
}

struct ohoi_custom_field {
	int (*get_len)(ipmi_fru_t *, unsigned int, unsigned int*);
	int (*get_data)(ipmi_fru_t *, unsigned int, char*, unsigned int*);
	SaHpiIdrFieldT *field;
	unsigned int num;
	SaErrorT rv;
	int done;
};

static void get_custom_field_cb(ipmi_entity_t *ent, void *cbdata)
{
	struct ohoi_custom_field *cf = cbdata;
	ipmi_fru_t *fru;
	SaHpiIdrFieldT *field = cf->field;
	unsigned int len;
	int rv;
	
	cf->done = 1;
	fru = ipmi_entity_get_fru(ent);
	if (fru == NULL) {
		dbg("Bug: entity without fru");
		cf->rv = SA_ERR_HPI_INTERNAL_ERROR;
		return;
	}
	field->Field.DataType = SAHPI_TL_TYPE_BINARY;
	
	field->Field.Language = SAHPI_LANG_ENGLISH;
	field->Field.DataLength = 0;

	rv = cf->get_len(fru, cf->num, &len);
	if (rv) {
		dbg("Error on get_len: %d", rv);
		cf->rv = SA_ERR_HPI_NOT_PRESENT;
		return;
	}

	if (len > SAHPI_MAX_TEXT_BUFFER_LENGTH)
		len = SAHPI_MAX_TEXT_BUFFER_LENGTH;

	rv = cf->get_data(fru, cf->num, &field->Field.Data[0], &len);
	if (!rv) {
		field->Field.DataLength = len;
	} else {
		dbg("Error on  get_data: %d", rv);
		cf->rv = SA_ERR_HPI_INTERNAL_ERROR;
	}
}

		
static SaErrorT get_custom_field(struct oh_handler_state  *handler,
				struct ohoi_resource_info   *ohoi_res_info,
				SaHpiEntryIdT lastid,
				SaHpiEntryIdT fieldid,
				SaHpiEntryIdT *nextfieldid,
				SaHpiIdrFieldT *field)
{
	unsigned int num;
	struct ohoi_custom_field cf;
	int rv;
	
	switch(areas[field->AreaId -1].areatype) {
	case SAHPI_IDR_AREATYPE_CHASSIS_INFO:
		cf.get_len = ipmi_fru_get_chassis_info_custom_len;
		cf.get_data = ipmi_fru_get_chassis_info_custom;
		num = ohoi_res_info->fru->ci_custom_num;
		break;
	case SAHPI_IDR_AREATYPE_BOARD_INFO:
		cf.get_len = ipmi_fru_get_board_info_custom_len;
		cf.get_data = ipmi_fru_get_board_info_custom;
		num = ohoi_res_info->fru->bi_custom_num;
		break;
	case SAHPI_IDR_AREATYPE_PRODUCT_INFO:
		cf.get_len =ipmi_fru_get_product_info_custom_len;
		cf.get_data =ipmi_fru_get_product_info_custom;
		//get_type = ipmi_fru_get_product_info_custom_type
		num = ohoi_res_info->fru->pi_custom_num;
		break;
	default:
		dbg("bug: areaa %d; wrong areatype %x",
			field->AreaId, areas[field->AreaId -1].areatype);
		return SA_ERR_HPI_NOT_PRESENT;
	}
	
	if (fieldid - lastid >= num) {
		dbg("fieldid(%d) - lastid(%d) >= num(%d)", fieldid, lastid, num);
		return SA_ERR_HPI_NOT_PRESENT;
	}
	
	cf.done = 0;
	cf.rv = SA_OK;
	cf.num = fieldid - lastid;
	cf.field = field;
	
	rv = ipmi_entity_pointer_cb(ohoi_res_info->u.entity_id,
		get_custom_field_cb, &cf);
	if (rv) {
		dbg("ipmi_entity_pointer_cb returned %d", rv);
		cf.rv = SA_ERR_HPI_INTERNAL_ERROR;
	} else {
		cf.rv = ohoi_loop(&cf.done, handler->data);
	}
	if (cf.rv != SA_OK) {
		dbg("error after get_custom_field_cb cf.rv =%d", cf.rv);
		return cf.rv;
	}
	field->Field.DataType = SAHPI_TL_TYPE_TEXT; // FIXME
	field->Field.Language = SAHPI_LANG_ENGLISH;   // FIXME
	if (fieldid + 1 - lastid < num) {
		*nextfieldid = fieldid + 1;
	} else {
		*nextfieldid = SAHPI_LAST_ENTRY;
	}
	return SA_OK;	 
}


SaErrorT ohoi_get_idr_info(void *hnd, SaHpiResourceIdT rid, SaHpiIdrIdT idrid,
                           SaHpiIdrInfoT *idrinfo)
{
	struct oh_handler_state  *handler = hnd;

	OHOI_CHECK_RPT_CAP_IDR();

	idrinfo->IdrId = OHOI_IDR_DEFAULT_ID;
	idrinfo->UpdateCount = 0;
	idrinfo->ReadOnly = SAHPI_FALSE;

	idrinfo->NumAreas = OHOI_AREA_NUM;

	return SA_OK;
}

SaErrorT ohoi_get_idr_area_header(void *hnd, SaHpiResourceIdT rid,
				  SaHpiIdrIdT idrid,
                                  SaHpiIdrAreaTypeT areatype,
				  SaHpiEntryIdT areaid,
                                  SaHpiEntryIdT *nextareaid,
				  SaHpiIdrAreaHeaderT *header)
{
	struct oh_handler_state  *handler = hnd;
	struct ohoi_area_data *area;
	struct ohoi_resource_info   *ohoi_res_info;
	SaHpiEntryIdT tmp_id;

	OHOI_CHECK_RPT_CAP_IDR();
	ohoi_res_info = oh_get_resource_data(handler->rptcache, rid);
	if (ohoi_res_info->fru == NULL) {
		dbg("bug: resource without fru?");
		return SA_ERR_HPI_CAPABILITY;
	}

	
	if ((areatype == SAHPI_IDR_AREATYPE_UNSPECIFIED) &&
	    (areaid == SAHPI_FIRST_ENTRY)) {
		for (tmp_id = OHOI_AREA_FIRST_ID;
				tmp_id <= OHOI_AREA_LAST_ID; tmp_id++) {
			if (get_area_presence(ohoi_res_info->fru,
					areas[tmp_id - 1].areatype)) {
				break;
			}
		}
		if (tmp_id > OHOI_AREA_LAST_ID) {
			dbg("tmp_id > OHOI_AREA_LAST_ID");
			return SA_ERR_HPI_NOT_PRESENT;
		}
		areaid = tmp_id;
	} else if ((areatype != SAHPI_IDR_AREATYPE_UNSPECIFIED) &&
	    (areaid == SAHPI_FIRST_ENTRY)) {
		areaid = get_first_area(areatype);
		if (areaid == OHOI_AREA_EMPTY_ID) {
			dbg("areaid == OHOI_AREA_EMPTY_ID");
			return SA_ERR_HPI_NOT_PRESENT;
		}
	} else if ((areatype == SAHPI_IDR_AREATYPE_UNSPECIFIED) &&
	    (areaid != SAHPI_FIRST_ENTRY)) {
		if (areaid > OHOI_AREA_LAST_ID) {
			dbg("areaid(%d) > OHOI_AREA_LAST_ID(%d)",
					areaid, OHOI_AREA_LAST_ID);
			return SA_ERR_HPI_NOT_PRESENT;
		}
	} else if ((areatype != SAHPI_IDR_AREATYPE_UNSPECIFIED) && 
	    (areaid != SAHPI_FIRST_ENTRY)) {
		if (areaid > OHOI_AREA_LAST_ID) {
			dbg("areaid(%d) > OHOI_AREA_LAST_ID(%d)",
					areaid, OHOI_AREA_LAST_ID);
			return SA_ERR_HPI_NOT_PRESENT;
		}
	}
	
	if (areatype != SAHPI_IDR_AREATYPE_UNSPECIFIED) {
		if (areas[areaid - 1].areatype != areatype) {
			dbg("areas[areaid - 1].areatype(%d) != areatype(%d)",
				areas[areaid - 1].areatype, areatype);
			return SA_ERR_HPI_INVALID_PARAMS;
		}
		if (!get_area_presence(ohoi_res_info->fru, areatype)) {
			dbg("area %d not present", areatype);
			return SA_ERR_HPI_NOT_PRESENT;
		}
		*nextareaid = SAHPI_LAST_ENTRY;
	} else {
		for (tmp_id = areaid + 1; tmp_id <= OHOI_AREA_LAST_ID; tmp_id++) {
			if (get_area_presence(ohoi_res_info->fru,
					areas[tmp_id - 1].areatype)) {
				break;
			}
		}
		if (tmp_id > OHOI_AREA_LAST_ID) {
			*nextareaid = SAHPI_LAST_ENTRY;
		} else {
			*nextareaid = tmp_id;
		}
	}

	area = &areas[areaid -1];

	header->AreaId = areaid;
	header->Type = area->areatype;

	header->ReadOnly = (area->areatype == SAHPI_IDR_AREATYPE_INTERNAL_USE) ?
						SAHPI_TRUE : SAHPI_FALSE;
	if (SAHPI_IDR_AREATYPE_OEM != area->areatype) {
		header->NumFields = area->field_num;
	} else {
		header->NumFields = ohoi_res_info->fru->oem_fields_num; // FIXME
	}

	return SA_OK;
}

SaErrorT ohoi_add_idr_area(void *hnd, SaHpiResourceIdT rid, SaHpiIdrIdT idrid,
                           SaHpiIdrAreaTypeT areatype, SaHpiEntryIdT *areaid)
{
	return SA_ERR_HPI_INVALID_REQUEST;
}

SaErrorT ohoi_del_idr_area(void *hnd, SaHpiResourceIdT rid, SaHpiIdrIdT idrid,
                           SaHpiEntryIdT areaid)
{
	return SA_ERR_HPI_INVALID_REQUEST;
}



static  SaHpiEntryIdT get_nextfield(struct ohoi_inventory_info *i_info,
				    struct ohoi_area_data  *area_data,
				    SaHpiEntryIdT fieldid)
{
	unsigned int msk;
	unsigned int num;
	SaHpiEntryIdT i;

	switch (area_data->areatype) {
	case SAHPI_IDR_AREATYPE_INTERNAL_USE:
		return SAHPI_LAST_ENTRY;
	case SAHPI_IDR_AREATYPE_CHASSIS_INFO:
		msk = i_info->ci_fld_msk;
		num = i_info->ci_custom_num;
		break;
	case SAHPI_IDR_AREATYPE_BOARD_INFO:
		msk = i_info->bi_fld_msk;
		num = i_info->bi_custom_num;
		break;
	case SAHPI_IDR_AREATYPE_PRODUCT_INFO:
		msk = i_info->pi_fld_msk;
		num = i_info->pi_custom_num;
		break;
	case SAHPI_IDR_AREATYPE_OEM:
		msk = 0;
		num = 0;
		break;
	default:
		dbg("bug: wrong areatype %x", area_data->areatype);
		return SA_ERR_HPI_INTERNAL_ERROR;
	}
	
	dbg("area = %x; fieldid = %d; msk = %x; num = %x", area_data->areatype,
			fieldid, msk, num);

	for (i = 1; fieldid + i - 1 < OHOI_FIELD_LAST_ID(area_data); i++) {
		if (area_data->fields[fieldid + i - 1].fieldtype ==
				SAHPI_IDR_FIELDTYPE_CUSTOM) {
			break;
		}
		if ((1 << (area_data->fields[fieldid + i - 1].fieldtype)) & msk) {
			dbg("return %d for not custom field %d",
				fieldid + i,
				area_data->fields[fieldid + i - 1].fieldtype);
			return fieldid + i;
		}
	}
	if (((1 << SAHPI_IDR_FIELDTYPE_CUSTOM) & msk) && 
		(fieldid + 1 < OHOI_FIELD_LAST_ID(area_data) + num)) {
		dbg("return %d for custom field", fieldid + 1);
		return fieldid + 1;
	}
	dbg("return SAHPI_LAST_ENTRY");
	return SAHPI_LAST_ENTRY;
}
			
	

SaErrorT ohoi_get_idr_field(void *hnd, SaHpiResourceIdT rid, SaHpiIdrIdT idrid,
		            SaHpiEntryIdT areaid, SaHpiIdrFieldTypeT fieldtype,
                            SaHpiEntryIdT fieldid, SaHpiEntryIdT *nextfieldid,
                            SaHpiIdrFieldT *field)
{
	struct oh_handler_state  *handler = hnd;
	struct ohoi_resource_info   *ohoi_res_info;
	ipmi_entity_id_t         ent_id;
	struct ohoi_field_data   *field_data;
	struct ohoi_area_data  *area_data;
	struct ohoi_get_field gf;
	int rv;

	OHOI_CHECK_RPT_CAP_IDR();

	ohoi_res_info = oh_get_resource_data(handler->rptcache, rid);
	if (ohoi_res_info->type != OHOI_RESOURCE_ENTITY) {
		dbg("Bug: try to get fru in unsupported resource");
		return SA_ERR_HPI_INVALID_CMD;
	}
	ent_id = ohoi_res_info->u.entity_id;
	
	if (areaid == SAHPI_FIRST_ENTRY)
		areaid = OHOI_AREA_FIRST_ID;

	if (areaid > OHOI_AREA_LAST_ID) {
		dbg("areaid > OHOI_AREA_LAST_ID");
		return SA_ERR_HPI_NOT_PRESENT;
	}

	area_data = &areas[areaid - 1];
	field->AreaId = areaid;
	
	if (area_data->areatype == SAHPI_IDR_AREATYPE_OEM) {
		// oem area is handled by special alghorithm
		return get_oem_idr_field(handler, ohoi_res_info,
				fieldtype, fieldid, nextfieldid, field);
	}

//	if (fieldid > OHOI_FIELD_LAST_ID(area_data)) {
//		dbg("fieldid > OHOI_FIELD_LAST_ID(area_data)");
//		return SA_ERR_HPI_NOT_PRESENT;
//	}

	if ((fieldtype == SAHPI_IDR_FIELDTYPE_UNSPECIFIED) &&
	    (fieldid == SAHPI_FIRST_ENTRY)) {
		fieldid = get_first_field(area_data, ohoi_res_info->fru);	
		if (fieldid == OHOI_FIELD_EMPTY_ID) {
			dbg("fieldid == OHOI_FIELD_EMPTY_ID");
			return SA_ERR_HPI_NOT_PRESENT;
		}
	} else if ((fieldtype != SAHPI_IDR_FIELDTYPE_UNSPECIFIED) &&
					(fieldid == SAHPI_FIRST_ENTRY)) {
		fieldid = get_first_typed_field(area_data, fieldtype,
			ohoi_res_info->fru);
		if (fieldid == OHOI_FIELD_EMPTY_ID) {
			dbg("fieldid == OHOI_FIELD_EMPTY_ID");
			return SA_ERR_HPI_NOT_PRESENT;
		}
	} else if ((fieldtype == SAHPI_IDR_FIELDTYPE_UNSPECIFIED) &&
					(fieldid != SAHPI_FIRST_ENTRY)) {
		if (fieldid >= OHOI_FIELD_LAST_ID(area_data)) {
			dbg("area %x; fieldid(%d) >= "
			    "OHOI_FIELD_LAST_ID(area_data)(%d)",
				area_data->areatype, fieldid,
				OHOI_FIELD_LAST_ID(area_data));
				field->FieldId = fieldid;
				field->Type = SAHPI_IDR_FIELDTYPE_CUSTOM;
				field->ReadOnly = area_data->read_only;
				return get_custom_field(handler, ohoi_res_info,
				OHOI_FIELD_LAST_ID(area_data),
				fieldid, nextfieldid, field);
		}	
	} else if ((fieldtype != SAHPI_IDR_FIELDTYPE_UNSPECIFIED) && 
					(fieldid != SAHPI_FIRST_ENTRY)) {
		if (fieldid >= OHOI_FIELD_LAST_ID(area_data)) {
			if (fieldtype != SAHPI_IDR_FIELDTYPE_CUSTOM) {
				dbg("fieldtype(%d) != SAHPI_IDR_FIELDTYPE_CUSTOM",
					fieldtype);
				return SA_ERR_HPI_NOT_PRESENT;
			}
			return get_custom_field(handler, ohoi_res_info,
				fieldid - OHOI_FIELD_LAST_ID(area_data),
				fieldid, nextfieldid, field);
		}
		if (area_data->fields[fieldid - 1].fieldtype != fieldtype) {
			dbg("area_data->fields[fieldid - 1].fieldtype != fieldtype(%d != %d)",
				area_data->fields[fieldid - 1].fieldtype, fieldtype);
			return SA_ERR_HPI_INVALID_PARAMS;
		}
	}

	field_data = &area_data->fields[fieldid - 1];

	if (area_data->areatype == SAHPI_IDR_AREATYPE_OEM) {
		if (fieldid < ohoi_res_info->fru->oem_fields_num - 1) {
			*nextfieldid = fieldid + 1;
		} else {
			*nextfieldid = SAHPI_LAST_ENTRY;
		}
	} else {
		*nextfieldid = get_nextfield(ohoi_res_info->fru,
			area_data, fieldid);
	}

	field->FieldId = fieldid;
	field->Type = field_data->fieldtype;

	gf.field = field;
	gf.data = field_data;
	gf.rv = SA_OK;
	gf.done = 0;

	rv = ipmi_entity_pointer_cb(ent_id, get_field, &gf);
	if (rv) {
		return SA_ERR_HPI_INTERNAL_ERROR;
	}
	gf.rv = ohoi_loop(&gf.done, handler->data);
	if (gf.rv != SA_OK) {
		return gf.rv;
	}
	field->ReadOnly = area_data->read_only;
	if (field_data->lang == SAHPI_LANG_UNDEF) {
		field->Field.Language = get_language(ohoi_res_info->fru,
						area_data->areatype);
	} else {
		field->Field.Language = field_data->lang;
	}
	return SA_OK;
}

SaErrorT ohoi_add_idr_field(void *hnd, SaHpiResourceIdT rid, SaHpiIdrIdT idrid,
                            SaHpiIdrFieldT *field )
{
	return SA_ERR_HPI_INVALID_REQUEST;
}

static SaErrorT modify_inventory(SaHpiIdrFieldT *field, ipmi_entity_t *ent,
					struct ohoi_resource_info *res_info);

struct ohoi_mod_field {
	SaHpiIdrFieldT *field;
	struct ohoi_resource_info *res_info;
	struct oh_handler_state  *hnd;
	SaErrorT rv;
	int done;
};

struct ohoi_fru_write {
	int done;
	SaErrorT rv;
};

static void fru_write_done_cb(ipmi_domain_t *domain,
			ipmi_fru_t    *fru,
			int           err,
			void          *cb_data)
{
	struct ohoi_fru_write *fw = cb_data;
	fw->done = 1;
	fw->rv = err ? SA_ERR_HPI_INTERNAL_ERROR : SA_OK;
}

static void modify_inventoty_field_cb(ipmi_entity_t *ent, void *cbdata)
{
	struct ohoi_mod_field *mf = cbdata;
	int rv;
	struct ohoi_fru_write fw;
	
	dbg("modify_inventoty_field_cb started");
	mf->rv = modify_inventory(mf->field, ent, mf->res_info);
	mf->done = 1;
	if (mf->rv != SA_OK) {
		return;
	}
	if (((struct ohoi_handler *)mf->hnd->data)->real_write_fru) {
		fw.done = 0;
		fw.rv = SA_OK;
		rv = ipmi_fru_write(ipmi_entity_get_fru(ent),
					fru_write_done_cb, &fw);
		if (rv) {
			mf->rv = SA_ERR_HPI_INTERNAL_ERROR;
		} else {
			mf->rv = ohoi_loop(&fw.done, mf->hnd->data);
		}
	}
}

SaErrorT ohoi_set_idr_field(void *hnd, SaHpiResourceIdT rid, SaHpiIdrIdT idrid,
                            SaHpiIdrFieldT *field)
{
	struct oh_handler_state  *handler = hnd;
	struct ohoi_resource_info   *ohoi_res_info;
	struct ohoi_mod_field mf;
	int rv;

	OHOI_CHECK_RPT_CAP_IDR();

	ohoi_res_info = oh_get_resource_data(handler->rptcache, rid);
	if (ohoi_res_info->type != OHOI_RESOURCE_ENTITY) {
		dbg("Bug: try to get fru in unsupported resource");
		return SA_ERR_HPI_INVALID_CMD;
	}

	mf.field = field;
	mf.res_info = ohoi_res_info;
	mf.hnd = hnd;
	mf.rv = SA_OK;
	mf.done = 0;
	
	rv = ipmi_entity_pointer_cb(ohoi_res_info->u.entity_id,
					modify_inventoty_field_cb, &mf);
	if (rv) {
		dbg("ipmi_entity_pointer_cb returned %d", rv);
		mf.rv = SA_ERR_HPI_INTERNAL_ERROR;
	} else {
		mf.rv = ohoi_loop(&mf.done, handler->data);
	}
	if (mf.rv != SA_OK) {
		dbg("ohoi_set_idr_field failed. rv = %d", mf.rv);
	}
	return mf.rv; 

}

SaErrorT ohoi_del_idr_field(void *hnd, SaHpiResourceIdT rid, SaHpiIdrIdT idrid,
                            SaHpiEntryIdT areaid, SaHpiEntryIdT fieldid)
{
	return SA_ERR_HPI_INVALID_REQUEST;
}
 
  
static SaErrorT get_str_type(SaHpiTextBufferT *tb,
		      SaHpiLanguageT lang,
		      enum ipmi_str_type_e *type)
{
	switch (tb->DataType) {
	case SAHPI_TL_TYPE_BINARY:
		*type = IPMI_BINARY_STR;
		return SA_OK;
	case SAHPI_TL_TYPE_TEXT:
	case SAHPI_TL_TYPE_ASCII6:
	case SAHPI_TL_TYPE_BCDPLUS:
		*type = IPMI_ASCII_STR;
		break;
	case SAHPI_TL_TYPE_UNICODE:
		*type = IPMI_UNICODE_STR;
		break;
	default:
		return SA_ERR_HPI_INVALID_DATA;
	}
	if (lang && (tb->Language != lang)) {
		return SA_ERR_HPI_INVALID_DATA;
	}
	return SA_OK;
}

  
static SaErrorT modify_inventory(SaHpiIdrFieldT *field,
				ipmi_entity_t *ent,
				struct ohoi_resource_info   *ohoi_res_info)
{
	struct ohoi_inventory_info *i_info = ohoi_res_info->fru;
	SaHpiEntryIdT areaid = field->AreaId;
	SaHpiEntryIdT fieldid = field->FieldId;
	SaHpiTextBufferT *tb = &field->Field;
	SaHpiIdrFieldTypeT f_type;
	SaHpiIdrAreaTypeT  a_type;
	struct ohoi_area_data  *area_data;
	enum ipmi_str_type_e type;
	unsigned char lang;
	SaErrorT ret = SA_OK;
	ipmi_fru_t *fru;
	int rv = 0;
	int (* set_func)(ipmi_fru_t   *fru,
			enum ipmi_str_type_e type,
			char         *str,
			unsigned int len) = NULL;


	if (areaid > OHOI_AREA_LAST_ID && areaid < 1) {
		dbg("areaid(%d) > OHOI_AREA_LAST_ID(%d) && areaid < 1",
			areaid, OHOI_AREA_LAST_ID);
		return SA_ERR_HPI_NOT_PRESENT;
	}
	area_data = &areas[areaid - 1];
	a_type = area_data->areatype;
	if (fieldid > OHOI_FIELD_LAST_ID(area_data)) {
		dbg("fieldid(%d) >= OHOI_FIELD_LAST_ID(area_data)(%d)",
			fieldid, OHOI_FIELD_LAST_ID(area_data));
		return SA_ERR_HPI_NOT_PRESENT;
	}
		
	f_type = area_data->fields[fieldid - 1].fieldtype;
	dbg("modify_inventory: area = 0x%x; field = %i", a_type, f_type);
	if (i_info == NULL) {
		dbg("Bug: ohoi_res_info->fru == NULL");
		return SA_ERR_HPI_CAPABILITY;
	}
	fru = ipmi_entity_get_fru(ent);
	if (fru == NULL) {
		dbg("Bug: resource without fru");
		return SA_ERR_HPI_INTERNAL_ERROR;
	}
	switch (a_type) {
	case SAHPI_IDR_AREATYPE_CHASSIS_INFO:
		if (i_info->ci == 0) {
			dbg("CHASSIS_INFO area not present");
			ret = SA_ERR_HPI_NOT_PRESENT;
			goto out;
		}
		switch (f_type) {
		case SAHPI_IDR_FIELDTYPE_CHASSIS_TYPE:
			if ((tb->DataType != SAHPI_TL_TYPE_BINARY) ||
					(tb->DataLength != 1)) {
				dbg("CHASSIS_TYPE: DataType = %d; DataLength = %d",
					tb->DataType, tb->DataLength);
				return SA_ERR_HPI_INVALID_DATA;
			}
			rv = ipmi_fru_set_chassis_info_type(fru,
				*(unsigned char *)tb->Data);
			break;
		case SAHPI_IDR_FIELDTYPE_PART_NUMBER:
			lang = 0;
			set_func = ipmi_fru_set_chassis_info_part_number;
			break;
		case SAHPI_IDR_FIELDTYPE_SERIAL_NUMBER:
			lang = SAHPI_LANG_ENGLISH;
			set_func = ipmi_fru_set_chassis_info_serial_number;
			break;
		default:
			dbg("CHASSIS_INFO: field %d not present", fieldid);
			ret = SA_ERR_HPI_NOT_PRESENT;
			goto out;
		}
		break;
	case SAHPI_IDR_AREATYPE_BOARD_INFO:
		if (i_info->bi == 0) {
			lang = SAHPI_LANG_ENGLISH;
//			dbg("BOARD_INFO area not present");
//			ret = SA_ERR_HPI_NOT_PRESENT;
//			goto out;
		} else
		lang = i_info->bi;
		switch (f_type) {
		case SAHPI_IDR_FIELDTYPE_MFG_DATETIME:
			if ((tb->DataType != SAHPI_TL_TYPE_BINARY) ||
					(tb->DataLength != sizeof(time_t))) {
				dbg("BOARD_INFO/MFG_DATETIME: DataType = %d; "
					"DataLength = %d",
					tb->DataType, tb->DataLength);	
				return SA_ERR_HPI_INVALID_DATA;
			}
			rv = ipmi_fru_set_chassis_info_type(fru,
				*(time_t *)tb->Data);
			break;
		case SAHPI_IDR_FIELDTYPE_MANUFACTURER:
			set_func = ipmi_fru_set_board_info_board_manufacturer;
			break;
		case SAHPI_IDR_FIELDTYPE_PRODUCT_NAME:
			set_func = ipmi_fru_set_board_info_board_product_name;
			break;
		case SAHPI_IDR_FIELDTYPE_PART_NUMBER:
			set_func = ipmi_fru_set_board_info_board_part_number;
			break;
		case SAHPI_IDR_FIELDTYPE_SERIAL_NUMBER:
			lang = SAHPI_LANG_ENGLISH;
			 set_func = ipmi_fru_set_board_info_board_serial_number;
			break;
		case SAHPI_IDR_FIELDTYPE_FILE_ID:
			lang = SAHPI_LANG_ENGLISH;
			set_func = ipmi_fru_set_board_info_fru_file_id;
			break;
		default:
			dbg("BOARD_INFO: field %d not present", fieldid);
			ret = SA_ERR_HPI_NOT_PRESENT;
			goto out;
		}
		break;
	case SAHPI_IDR_AREATYPE_PRODUCT_INFO:
		if (i_info->pi == 0) {
			dbg("PRODUCT_INFO area not present");
			return SA_ERR_HPI_NOT_PRESENT;
		}
		lang = i_info->pi;
		switch (f_type) {
		case SAHPI_IDR_FIELDTYPE_MANUFACTURER:
			set_func = ipmi_fru_set_product_info_manufacturer_name;
			break;
		case SAHPI_IDR_FIELDTYPE_PRODUCT_NAME:
			set_func = ipmi_fru_set_product_info_product_name;
			break;
		case SAHPI_IDR_FIELDTYPE_PART_NUMBER:
			set_func = ipmi_fru_set_product_info_product_part_model_number;
			break;
		case SAHPI_IDR_FIELDTYPE_PRODUCT_VERSION:
			set_func = ipmi_fru_set_product_info_product_version;
			break;
		case SAHPI_IDR_FIELDTYPE_SERIAL_NUMBER:
			lang = SAHPI_LANG_ENGLISH;
			set_func = ipmi_fru_set_product_info_product_serial_number;
			break;
		case SAHPI_IDR_FIELDTYPE_ASSET_TAG:
			set_func = ipmi_fru_set_product_info_asset_tag;
			break;
		case SAHPI_IDR_FIELDTYPE_FILE_ID:
			set_func = ipmi_fru_set_product_info_fru_file_id;
			break;
		default:
			dbg("PRODUCT_INFO: field %d not present", fieldid);
			ret = SA_ERR_HPI_NOT_PRESENT;
			goto out;
		}
		break;
	default:
		dbg("Unknown area type = 0x%x", areas[areaid].areatype);
		ret = SA_ERR_HPI_INVALID_PARAMS;
		goto out;
	}

	if (set_func != NULL) {
		ret = get_str_type(tb, lang, &type);
		if (ret != SA_OK) {
			goto out;
		}
		rv = set_func(fru, type, (char *)tb->Data,
					(unsigned int)tb->DataLength);
		if (rv) {
			dbg("Could not set FRU field %d of area %d. rv = %d\n",
				fieldid, areaid, rv);
			if (rv == ENOSPC) {
				ret = SA_ERR_HPI_OUT_OF_SPACE;
			} else {
				dbg("set_func for %x/%d returned error = %d",
					a_type, f_type, rv);
				ret = SA_ERR_HPI_INVALID_DATA;
			}
		} 
	}
out:
	return ret;
}

