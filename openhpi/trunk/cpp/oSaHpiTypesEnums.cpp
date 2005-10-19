/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2005
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *    W. David Ashley <dashley@us.ibm.com>
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
extern "C"
{
#include <SaHpi.h>
}
#include "oSaHpiTypesEnums.hpp"


/**
 * Translates the boolean true or false into a string.
 *
 * @param f       The boolean value.
 *
 * @return "SAHPI_TRUE" if the boolean is true, otherwise returns "SAHPI_FALSE".
 */
const char * oSaHpiTypesEnums::torf2str(SaHpiBoolT f) {
    if (f) {
        return "SAHPI_TRUE";
    }
	return "SAHPI_FALSE";
}


/**
 * Translates the string value to true or false.
 *
 * @param str     The string to be translated.
 *
 * @return SAHPI_TRUE if the string is true, otherwise returns SAHPI_FALSE.
 */
SaHpiBoolT oSaHpiTypesEnums::str2torf(const char *str) {
    if (strcmp(str, "SAHPI_FALSE") == 0 || strcmp(str, "FALSE") == 0 ||
     strcmp(str, "0") == 0) {
        return SAHPI_FALSE;
    }
	return SAHPI_FALSE;
}


static struct language_map {
    SaHpiLanguageT type;
    const char     *str;
} language_strings[] = {
   {SAHPI_LANG_UNDEF,          "SAHPI_LANG_UNDEF"},
   {SAHPI_LANG_AFAR,           "SAHPI_LANG_AFAR"},
   {SAHPI_LANG_ABKHAZIAN,      "SAHPI_LANG_ABKHAZIAN"},
   {SAHPI_LANG_AFRIKAANS,      "SAHPI_LANG_AFRIKAANS"},
   {SAHPI_LANG_AMHARIC,        "SAHPI_LANG_AMHARIC"},
   {SAHPI_LANG_ARABIC,         "SAHPI_LANG_ARABIC"},
   {SAHPI_LANG_ASSAMESE,       "SAHPI_LANG_ASSAMESE"},
   {SAHPI_LANG_AYMARA,         "SAHPI_LANG_AYMARA"},
   {SAHPI_LANG_AZERBAIJANI,    "SAHPI_LANG_AZERBAIJANI"},
   {SAHPI_LANG_BASHKIR,        "SAHPI_LANG_BASHKIR"},
   {SAHPI_LANG_BYELORUSSIAN,   "SAHPI_LANG_BYELORUSSIAN"},
   {SAHPI_LANG_BULGARIAN,      "SAHPI_LANG_BULGARIAN"},
   {SAHPI_LANG_BIHARI,         "SAHPI_LANG_BIHARI"},
   {SAHPI_LANG_BISLAMA,        "SAHPI_LANG_BISLAMA"},
   {SAHPI_LANG_BENGALI,        "SAHPI_LANG_BENGALI"},
   {SAHPI_LANG_TIBETAN,        "SAHPI_LANG_TIBETAN"},
   {SAHPI_LANG_BRETON,         "SAHPI_LANG_BRETON"},
   {SAHPI_LANG_CATALAN,        "SAHPI_LANG_CATALAN"},
   {SAHPI_LANG_CORSICAN,       "SAHPI_LANG_CORSICAN"},
   {SAHPI_LANG_CZECH,          "SAHPI_LANG_CZECH"},
   {SAHPI_LANG_WELSH,          "SAHPI_LANG_WELSH"},
   {SAHPI_LANG_DANISH,         "SAHPI_LANG_DANISH"},
   {SAHPI_LANG_GERMAN,         "SAHPI_LANG_GERMAN"},
   {SAHPI_LANG_BHUTANI,        "SAHPI_LANG_BHUTANI"},
   {SAHPI_LANG_GREEK,          "SAHPI_LANG_GREEK"},
   {SAHPI_LANG_ENGLISH,        "SAHPI_LANG_ENGLISH"},
   {SAHPI_LANG_ESPERANTO,      "SAHPI_LANG_ESPERANTO"},
   {SAHPI_LANG_SPANISH,        "SAHPI_LANG_SPANISH"},
   {SAHPI_LANG_ESTONIAN,       "SAHPI_LANG_ESTONIAN"},
   {SAHPI_LANG_BASQUE,         "SAHPI_LANG_BASQUE"},
   {SAHPI_LANG_PERSIAN,        "SAHPI_LANG_PERSIAN"},
   {SAHPI_LANG_FINNISH,        "SAHPI_LANG_FINNISH"},
   {SAHPI_LANG_FIJI,           "SAHPI_LANG_FIJI"},
   {SAHPI_LANG_FAEROESE,       "SAHPI_LANG_FAEROESE"},
   {SAHPI_LANG_FRENCH,         "SAHPI_LANG_FRENCH"},
   {SAHPI_LANG_FRISIAN,        "SAHPI_LANG_FRISIAN"},
   {SAHPI_LANG_IRISH,          "SAHPI_LANG_IRISH"},
   {SAHPI_LANG_SCOTSGAELIC,    "SAHPI_LANG_SCOTSGAELIC"},
   {SAHPI_LANG_GALICIAN,       "SAHPI_LANG_GALICIAN"},
   {SAHPI_LANG_GUARANI,        "SAHPI_LANG_GUARANI"},
   {SAHPI_LANG_GUJARATI,       "SAHPI_LANG_GUJARATI"},
   {SAHPI_LANG_HAUSA,          "SAHPI_LANG_HAUSA"},
   {SAHPI_LANG_HINDI,          "SAHPI_LANG_HINDI"},
   {SAHPI_LANG_CROATIAN,       "SAHPI_LANG_CROATIAN"},
   {SAHPI_LANG_HUNGARIAN,      "SAHPI_LANG_HUNGARIAN"},
   {SAHPI_LANG_ARMENIAN,       "SAHPI_LANG_ARMENIAN"},
   {SAHPI_LANG_INTERLINGUA,    "SAHPI_LANG_INTERLINGUA"},
   {SAHPI_LANG_INTERLINGUE,    "SAHPI_LANG_INTERLINGUE"},
   {SAHPI_LANG_INUPIAK,        "SAHPI_LANG_INUPIAK"},
   {SAHPI_LANG_INDONESIAN,     "SAHPI_LANG_INDONESIAN"},
   {SAHPI_LANG_ITALIAN,        "SAHPI_LANG_ITALIAN"},
   {SAHPI_LANG_HEBREW,         "SAHPI_LANG_HEBREW"},
   {SAHPI_LANG_JAPANESE,       "SAHPI_LANG_JAPANESE"},
   {SAHPI_LANG_YIDDISH,        "SAHPI_LANG_YIDDISH"},
   {SAHPI_LANG_JAVANESE,       "SAHPI_LANG_JAVANESE"},
   {SAHPI_LANG_GEORGIAN,       "SAHPI_LANG_GEORGIAN"},
   {SAHPI_LANG_KAZAKH,         "SAHPI_LANG_KAZAKH"},
   {SAHPI_LANG_GREENLANDIC,    "SAHPI_LANG_GREENLANDIC"},
   {SAHPI_LANG_CAMBODIAN,      "SAHPI_LANG_CAMBODIAN"},
   {SAHPI_LANG_KANNADA,        "SAHPI_LANG_KANNADA"},
   {SAHPI_LANG_KOREAN,         "SAHPI_LANG_KOREAN"},
   {SAHPI_LANG_KASHMIRI,       "SAHPI_LANG_KASHMIRI"},
   {SAHPI_LANG_KURDISH,        "SAHPI_LANG_KURDISH"},
   {SAHPI_LANG_KIRGHIZ,        "SAHPI_LANG_KIRGHIZ"},
   {SAHPI_LANG_LATIN,          "SAHPI_LANG_LATIN"},
   {SAHPI_LANG_LINGALA,        "SAHPI_LANG_LINGALA"},
   {SAHPI_LANG_LAOTHIAN,       "SAHPI_LANG_LAOTHIAN"},
   {SAHPI_LANG_LITHUANIAN,     "SAHPI_LANG_LITHUANIAN"},
   {SAHPI_LANG_LATVIANLETTISH, "SAHPI_LANG_LATVIANLETTISH"},
   {SAHPI_LANG_MALAGASY,       "SAHPI_LANG_MALAGASY"},
   {SAHPI_LANG_MAORI,          "SAHPI_LANG_MAORI"},
   {SAHPI_LANG_MACEDONIAN,     "SAHPI_LANG_MACEDONIAN"},
   {SAHPI_LANG_MALAYALAM,      "SAHPI_LANG_MALAYALAM"},
   {SAHPI_LANG_MONGOLIAN,      "SAHPI_LANG_MONGOLIAN"},
   {SAHPI_LANG_MOLDAVIAN,      "SAHPI_LANG_MOLDAVIAN"},
   {SAHPI_LANG_MARATHI,        "SAHPI_LANG_MARATHI"},
   {SAHPI_LANG_MALAY,          "SAHPI_LANG_MALAY"},
   {SAHPI_LANG_MALTESE,        "SAHPI_LANG_MALTESE"},
   {SAHPI_LANG_BURMESE,        "SAHPI_LANG_BURMESE"},
   {SAHPI_LANG_NAURU,          "SAHPI_LANG_NAURU"},
   {SAHPI_LANG_NEPALI,         "SAHPI_LANG_NEPALI"},
   {SAHPI_LANG_DUTCH,          "SAHPI_LANG_DUTCH"},
   {SAHPI_LANG_NORWEGIAN,      "SAHPI_LANG_NORWEGIAN"},
   {SAHPI_LANG_OCCITAN,        "SAHPI_LANG_OCCITAN"},
   {SAHPI_LANG_AFANOROMO,      "SAHPI_LANG_AFANOROMO"},
   {SAHPI_LANG_ORIYA,          "SAHPI_LANG_ORIYA"},
   {SAHPI_LANG_PUNJABI,        "SAHPI_LANG_PUNJABI"},
   {SAHPI_LANG_POLISH,         "SAHPI_LANG_POLISH"},
   {SAHPI_LANG_PASHTOPUSHTO,   "SAHPI_LANG_PASHTOPUSHTO"},
   {SAHPI_LANG_PORTUGUESE,     "SAHPI_LANG_PORTUGUESE"},
   {SAHPI_LANG_QUECHUA,        "SAHPI_LANG_QUECHUA"},
   {SAHPI_LANG_RHAETOROMANCE,  "SAHPI_LANG_RHAETOROMANCE"},
   {SAHPI_LANG_KIRUNDI,        "SAHPI_LANG_KIRUNDI"},
   {SAHPI_LANG_ROMANIAN,       "SAHPI_LANG_ROMANIAN"},
   {SAHPI_LANG_RUSSIAN,        "SAHPI_LANG_RUSSIAN"},
   {SAHPI_LANG_KINYARWANDA,    "SAHPI_LANG_KINYARWANDA"},
   {SAHPI_LANG_SANSKRIT,       "SAHPI_LANG_SANSKRIT"},
   {SAHPI_LANG_SINDHI,         "SAHPI_LANG_SINDHI"},
   {SAHPI_LANG_SANGRO,         "SAHPI_LANG_SANGRO"},
   {SAHPI_LANG_SERBOCROATIAN,  "SAHPI_LANG_SERBOCROATIAN"},
   {SAHPI_LANG_SINGHALESE,     "SAHPI_LANG_SINGHALESE"},
   {SAHPI_LANG_SLOVAK,         "SAHPI_LANG_SLOVAK"},
   {SAHPI_LANG_SLOVENIAN,      "SAHPI_LANG_SLOVENIAN"},
   {SAHPI_LANG_SAMOAN,         "SAHPI_LANG_SAMOAN"},
   {SAHPI_LANG_SHONA,          "SAHPI_LANG_SHONA"},
   {SAHPI_LANG_SOMALI,         "SAHPI_LANG_SOMALI"},
   {SAHPI_LANG_ALBANIAN,       "SAHPI_LANG_ALBANIAN"},
   {SAHPI_LANG_SERBIAN,        "SAHPI_LANG_SERBIAN"},
   {SAHPI_LANG_SISWATI,        "SAHPI_LANG_SISWATI"},
   {SAHPI_LANG_SESOTHO,        "SAHPI_LANG_SESOTHO"},
   {SAHPI_LANG_SUDANESE,       "SAHPI_LANG_SUDANESE"},
   {SAHPI_LANG_SWEDISH,        "SAHPI_LANG_SWEDISH"},
   {SAHPI_LANG_SWAHILI,        "SAHPI_LANG_SWAHILI"},
   {SAHPI_LANG_TAMIL,          "SAHPI_LANG_TAMIL"},
   {SAHPI_LANG_TELUGU,         "SAHPI_LANG_TELUGU"},
   {SAHPI_LANG_TAJIK,          "SAHPI_LANG_TAJIK"},
   {SAHPI_LANG_THAI,           "SAHPI_LANG_THAI"},
   {SAHPI_LANG_TIGRINYA,       "SAHPI_LANG_TIGRINYA"},
   {SAHPI_LANG_TURKMEN,        "SAHPI_LANG_TURKMEN"},
   {SAHPI_LANG_TAGALOG,        "SAHPI_LANG_TAGALOG"},
   {SAHPI_LANG_SETSWANA,       "SAHPI_LANG_SETSWANA"},
   {SAHPI_LANG_TONGA,          "SAHPI_LANG_TONGA"},
   {SAHPI_LANG_TURKISH,        "SAHPI_LANG_TURKISH"},
   {SAHPI_LANG_TSONGA,         "SAHPI_LANG_TSONGA"},
   {SAHPI_LANG_TATAR,          "SAHPI_LANG_TATAR"},
   {SAHPI_LANG_TWI,            "SAHPI_LANG_TWI"},
   {SAHPI_LANG_UKRAINIAN,      "SAHPI_LANG_UKRAINIAN"},
   {SAHPI_LANG_URDU,           "SAHPI_LANG_URDU"},
   {SAHPI_LANG_UZBEK,          "SAHPI_LANG_UZBEK"},
   {SAHPI_LANG_VIETNAMESE,     "SAHPI_LANG_VIETNAMESE"},
   {SAHPI_LANG_VOLAPUK,        "SAHPI_LANG_VOLAPUK"},
   {SAHPI_LANG_WOLOF,          "SAHPI_LANG_WOLOF"},
   {SAHPI_LANG_XHOSA,          "SAHPI_LANG_XHOSA"},
   {SAHPI_LANG_YORUBA,         "SAHPI_LANG_YORUBA"},
   {SAHPI_LANG_CHINESE,        "SAHPI_LANG_CHINESE"},
   {SAHPI_LANG_ZULU,           "SAHPI_LANG_ZULU"},
   {SAHPI_LANG_UNDEF,          NULL}
};


/**
 * Translates a string to a valid SaHpiLanguageT type.
 *
 * @param strtype The language type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiLanguageT oSaHpiTypesEnums::str2language(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_LANG_ENGLISH;
    }
	for (i = 0; language_strings[i].str != NULL; i++) {
		if (strcmp(strtype, language_strings[i].str) == 0) {
            return language_strings[i].type;
		}
	}
	return SAHPI_LANG_ENGLISH;
}


/**
 * Translates a language type to a string.
 *
 * @param value  The SaHpiLanguageT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::language2str(SaHpiLanguageT value) {
	int i;

	for (i = 0; language_strings[i].str != NULL; i++) {
		if (value == language_strings[i].type) {
			return language_strings[i].str;
		}
	}
    return "Unknown";
}


static struct texttype_map {
    SaHpiTextTypeT type;
    const char     *typestr;
} texttype_strings[] = {
   {SAHPI_TL_TYPE_UNICODE, "SAHPI_TL_TYPE_UNICODE"},
   {SAHPI_TL_TYPE_BCDPLUS, "SAHPI_TL_TYPE_BCDPLUS"},
   {SAHPI_TL_TYPE_ASCII6,  "SAHPI_TL_TYPE_ASCII6"},
   {SAHPI_TL_TYPE_TEXT,    "SAHPI_TL_TYPE_TEXT"},
   {SAHPI_TL_TYPE_BINARY,  "SAHPI_TL_TYPE_BINARY"},
   {SAHPI_TL_TYPE_BINARY,  NULL}
};


/**
 * Translate a string into a valid SaHpiTextTypeT value.
 *
 * @param type   The string to be translated.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiTextTypeT oSaHpiTypesEnums::str2texttype(const char *type) {
	int i;

    if (type == NULL) {
        return SAHPI_TL_TYPE_TEXT;
    }
	for (i = 0; texttype_strings[i].typestr != NULL; i++) {
		if (strcmp(type, texttype_strings[i].typestr) == 0) {
            return texttype_strings[i].type;
		}
	}
	return SAHPI_TL_TYPE_TEXT;
}


/**
 * Translates a text type to a string.
 *
 * @param value  The SaHpiTextType to be converted.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
const char * oSaHpiTypesEnums::texttype2str(SaHpiTextTypeT value) {
	int i;

	for (i = 0; texttype_strings[i].typestr != NULL; i++) {
		if (value == texttype_strings[i].type) {
			return texttype_strings[i].typestr;
		}
	}
    return "Unknown";
}


static struct entitytype_map {
    SaHpiEntityTypeT type;
    const char       *str;
} entitytype_strings[] = {
   {SAHPI_ENT_UNSPECIFIED,              "SAHPI_ENT_UNSPECIFIED"},
   {SAHPI_ENT_OTHER,                    "SAHPI_ENT_OTHER"},
   {SAHPI_ENT_UNKNOWN,                  "SAHPI_ENT_UNKNOWN"},
   {SAHPI_ENT_PROCESSOR,                "SAHPI_ENT_PROCESSOR"},
   {SAHPI_ENT_DISK_BAY,                 "SAHPI_ENT_DISK_BAY"},
   {SAHPI_ENT_PERIPHERAL_BAY,           "SAHPI_ENT_PERIPHERAL_BAY"},
   {SAHPI_ENT_SYS_MGMNT_MODULE,         "SAHPI_ENT_SYS_MGMNT_MODULE"},
   {SAHPI_ENT_SYSTEM_BOARD,             "SAHPI_ENT_SYSTEM_BOARD"},
   {SAHPI_ENT_MEMORY_MODULE,            "SAHPI_ENT_MEMORY_MODULE"},
   {SAHPI_ENT_PROCESSOR_MODULE,         "SAHPI_ENT_PROCESSOR_MODULE"},
   {SAHPI_ENT_POWER_SUPPLY,             "SAHPI_ENT_POWER_SUPPLY"},
   {SAHPI_ENT_ADD_IN_CARD,              "SAHPI_ENT_ADD_IN_CARD"},
   {SAHPI_ENT_FRONT_PANEL_BOARD,        "SAHPI_ENT_FRONT_PANEL_BOARD"},
   {SAHPI_ENT_BACK_PANEL_BOARD,         "SAHPI_ENT_BACK_PANEL_BOARD"},
   {SAHPI_ENT_POWER_SYSTEM_BOARD,       "SAHPI_ENT_POWER_SYSTEM_BOARD"},
   {SAHPI_ENT_DRIVE_BACKPLANE,          "SAHPI_ENT_DRIVE_BACKPLANE"},
   {SAHPI_ENT_SYS_EXPANSION_BOARD,      "SAHPI_ENT_SYS_EXPANSION_BOARD"},
   {SAHPI_ENT_OTHER_SYSTEM_BOARD,       "SAHPI_ENT_OTHER_SYSTEM_BOARD"},
   {SAHPI_ENT_PROCESSOR_BOARD,          "SAHPI_ENT_PROCESSOR_BOARD"},
   {SAHPI_ENT_POWER_UNIT,               "SAHPI_ENT_POWER_UNIT"},
   {SAHPI_ENT_POWER_MODULE,             "SAHPI_ENT_POWER_MODULE"},
   {SAHPI_ENT_POWER_MGMNT,              "SAHPI_ENT_POWER_MGMNT"},
   {SAHPI_ENT_CHASSIS_BACK_PANEL_BOARD, "SAHPI_ENT_CHASSIS_BACK_PANEL_BOARD"},
   {SAHPI_ENT_SYSTEM_CHASSIS,           "SAHPI_ENT_SYSTEM_CHASSIS"},
   {SAHPI_ENT_SUB_CHASSIS,              "SAHPI_ENT_SUB_CHASSIS"},
   {SAHPI_ENT_OTHER_CHASSIS_BOARD,      "SAHPI_ENT_OTHER_CHASSIS_BOARD"},
   {SAHPI_ENT_DISK_DRIVE_BAY,           "SAHPI_ENT_DISK_DRIVE_BAY"},
   {SAHPI_ENT_PERIPHERAL_BAY_2,         "SAHPI_ENT_PERIPHERAL_BAY_2"},
   {SAHPI_ENT_DEVICE_BAY,               "SAHPI_ENT_DEVICE_BAY"},
   {SAHPI_ENT_COOLING_DEVICE,           "SAHPI_ENT_COOLING_DEVICE"},
   {SAHPI_ENT_COOLING_UNIT,             "SAHPI_ENT_COOLING_UNIT"},
   {SAHPI_ENT_INTERCONNECT,             "SAHPI_ENT_INTERCONNECT"},
   {SAHPI_ENT_MEMORY_DEVICE,            "SAHPI_ENT_MEMORY_DEVICE"},
   {SAHPI_ENT_SYS_MGMNT_SOFTWARE,       "SAHPI_ENT_SYS_MGMNT_SOFTWARE"},
   {SAHPI_ENT_BIOS,                     "SAHPI_ENT_BIOS"},
   {SAHPI_ENT_OPERATING_SYSTEM,         "SAHPI_ENT_OPERATING_SYSTEM"},
   {SAHPI_ENT_SYSTEM_BUS,               "SAHPI_ENT_SYSTEM_BUS"},
   {SAHPI_ENT_GROUP,                    "SAHPI_ENT_GROUP"},
   {SAHPI_ENT_REMOTE,                   "SAHPI_ENT_REMOTE"},
   {SAHPI_ENT_EXTERNAL_ENVIRONMENT,     "SAHPI_ENT_EXTERNAL_ENVIRONMENT"},
   {SAHPI_ENT_BATTERY,                  "SAHPI_ENT_BATTERY"},
   {SAHPI_ENT_CHASSIS_SPECIFIC,         "SAHPI_ENT_CHASSIS_SPECIFIC"},
   {SAHPI_ENT_BOARD_SET_SPECIFIC,       "SAHPI_ENT_BOARD_SET_SPECIFIC"},
   {SAHPI_ENT_OEM_SYSINT_SPECIFIC,      "SAHPI_ENT_OEM_SYSINT_SPECIFIC"},
   {SAHPI_ENT_ROOT,                     "SAHPI_ENT_ROOT"},
   {SAHPI_ENT_RACK,                     "SAHPI_ENT_RACK"},
   {SAHPI_ENT_SUBRACK,                  "SAHPI_ENT_SUBRACK"},
   {SAHPI_ENT_COMPACTPCI_CHASSIS,       "SAHPI_ENT_COMPACTPCI_CHASSIS"},
   {SAHPI_ENT_ADVANCEDTCA_CHASSIS,      "SAHPI_ENT_ADVANCEDTCA_CHASSIS"},
   {SAHPI_ENT_RACK_MOUNTED_SERVER,      "SAHPI_ENT_RACK_MOUNTED_SERVER"},
   {SAHPI_ENT_SYSTEM_BLADE,             "SAHPI_ENT_SYSTEM_BLADE"},
   {SAHPI_ENT_SWITCH,                   "SAHPI_ENT_SWITCH"},
   {SAHPI_ENT_SWITCH_BLADE,             "SAHPI_ENT_SWITCH_BLADE"},
   {SAHPI_ENT_SBC_BLADE,                "SAHPI_ENT_SBC_BLADE"},
   {SAHPI_ENT_IO_BLADE,                 "SAHPI_ENT_IO_BLADE"},
   {SAHPI_ENT_DISK_BLADE,               "SAHPI_ENT_DISK_BLADE"},
   {SAHPI_ENT_DISK_DRIVE,               "SAHPI_ENT_DISK_DRIVE"},
   {SAHPI_ENT_FAN,                      "SAHPI_ENT_FAN"},
   {SAHPI_ENT_POWER_DISTRIBUTION_UNIT,  "SAHPI_ENT_POWER_DISTRIBUTION_UNIT"},
   {SAHPI_ENT_SPEC_PROC_BLADE,          "SAHPI_ENT_SPEC_PROC_BLADE"},
   {SAHPI_ENT_IO_SUBBOARD,              "SAHPI_ENT_IO_SUBBOARD"},
   {SAHPI_ENT_SBC_SUBBOARD,             "SAHPI_ENT_SBC_SUBBOARD"},
   {SAHPI_ENT_ALARM_MANAGER,            "SAHPI_ENT_ALARM_MANAGER"},
   {SAHPI_ENT_SHELF_MANAGER,            "SAHPI_ENT_SHELF_MANAGER"},
   {SAHPI_ENT_DISPLAY_PANEL,            "SAHPI_ENT_DISPLAY_PANEL"},
   {SAHPI_ENT_SUBBOARD_CARRIER_BLADE,   "SAHPI_ENT_SUBBOARD_CARRIER_BLADE"},
   {SAHPI_ENT_PHYSICAL_SLOT,            "SAHPI_ENT_PHYSICAL_SLOT"},
   {SAHPI_ENT_ROOT,                     NULL}
};


/**
 * Translates a string to a valid SaHpiEntityTypeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiEntityTypeT oSaHpiTypesEnums::str2entitytype(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_ENT_ROOT;
    }
	for (i = 0; entitytype_strings[i].str != NULL; i++) {
		if (strcmp(strtype, entitytype_strings[i].str) == 0) {
            return entitytype_strings[i].type;
		}
	}
	return SAHPI_ENT_ROOT;
}


/**
 * Translates an entity type to a string.
 *
 * @param value  The SaHpiEntityTypeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::entitytype2str(SaHpiEntityTypeT value) {
	int i;

	for (i = 0; entitytype_strings[i].str != NULL; i++) {
		if (value == entitytype_strings[i].type) {
			return entitytype_strings[i].str;
		}
	}
    return "Unknown";
}


static struct sensorreadingtype_map {
    SaHpiSensorReadingTypeT type;
    const char              *str;
} sensorreadingtype_strings[] = {
       {SAHPI_SENSOR_READING_TYPE_INT64,   "SAHPI_SENSOR_READING_TYPE_INT64"},
       {SAHPI_SENSOR_READING_TYPE_UINT64,  "SAHPI_SENSOR_READING_TYPE_UINT64"},
       {SAHPI_SENSOR_READING_TYPE_FLOAT64, "SAHPI_SENSOR_READING_TYPE_FLOAT64"},
       {SAHPI_SENSOR_READING_TYPE_BUFFER,  "SAHPI_SENSOR_READING_TYPE_BUFFER"},
       {SAHPI_SENSOR_READING_TYPE_BUFFER,  NULL},
};


/**
 * Translates a string to a valid SaHpiSensorReadingTypeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiSensorReadingTypeT oSaHpiTypesEnums::str2sensorreadingtype(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_SENSOR_READING_TYPE_INT64;
    }
	for (i = 0; sensorreadingtype_strings[i].str != NULL; i++) {
		if (strcmp(strtype, sensorreadingtype_strings[i].str) == 0) {
            return sensorreadingtype_strings[i].type;
		}
	}
	return SAHPI_SENSOR_READING_TYPE_INT64;
}


/**
 * Translates an sensor reading type to a string.
 *
 * @param value  The SaHpiSensorReadingTypeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::sensorreadingtype2str(SaHpiSensorReadingTypeT value) {
	int i;

	for (i = 0; sensorreadingtype_strings[i].str != NULL; i++) {
		if (value == sensorreadingtype_strings[i].type) {
			return sensorreadingtype_strings[i].str;
		}
	}
    return "Unknown";
}


static struct sensorunits_map {
    SaHpiSensorUnitsT type;
    const char        *str;
} sensorunits_strings[] = {
   {SAHPI_SU_UNSPECIFIED,          "SAHPI_SU_UNSPECIFIED"},
   {SAHPI_SU_DEGREES_C,            "SAHPI_SU_DEGREES_C"},
   {SAHPI_SU_DEGREES_F,            "SAHPI_SU_DEGREES_F"},
   {SAHPI_SU_DEGREES_K,            "SAHPI_SU_DEGREES_K"},
   {SAHPI_SU_VOLTS,                "SAHPI_SU_VOLTS"},
   {SAHPI_SU_AMPS,                 "SAHPI_SU_AMPS"},
   {SAHPI_SU_WATTS,                "SAHPI_SU_WATTS"},
   {SAHPI_SU_JOULES,               "SAHPI_SU_JOULES"},
   {SAHPI_SU_COULOMBS,             "SAHPI_SU_COULOMBS"},
   {SAHPI_SU_VA,                   "SAHPI_SU_VA"},
   {SAHPI_SU_NITS,                 "SAHPI_SU_NITS"},
   {SAHPI_SU_LUMEN,                "SAHPI_SU_LUMEN"},
   {SAHPI_SU_LUX,                  "SAHPI_SU_LUX"},
   {SAHPI_SU_CANDELA,              "SAHPI_SU_CANDELA"},
   {SAHPI_SU_KPA,                  "SAHPI_SU_KPA"},
   {SAHPI_SU_PSI,                  "SAHPI_SU_PSI"},
   {SAHPI_SU_NEWTON,               "SAHPI_SU_NEWTON"},
   {SAHPI_SU_CFM,                  "SAHPI_SU_CFM"},
   {SAHPI_SU_RPM,                  "SAHPI_SU_RPM"},
   {SAHPI_SU_HZ,                   "SAHPI_SU_HZ"},
   {SAHPI_SU_MICROSECOND,          "SAHPI_SU_MICROSECOND"},
   {SAHPI_SU_MILLISECOND,          "SAHPI_SU_MILLISECOND"},
   {SAHPI_SU_SECOND,               "SAHPI_SU_SECOND"},
   {SAHPI_SU_MINUTE,               "SAHPI_SU_MINUTE"},
   {SAHPI_SU_HOUR,                 "SAHPI_SU_HOUR"},
   {SAHPI_SU_DAY,                  "SAHPI_SU_DAY"},
   {SAHPI_SU_WEEK,                 "SAHPI_SU_WEEK"},
   {SAHPI_SU_MIL,                  "SAHPI_SU_MIL"},
   {SAHPI_SU_INCHES,               "SAHPI_SU_INCHES"},
   {SAHPI_SU_FEET,                 "SAHPI_SU_FEET"},
   {SAHPI_SU_CU_IN,                "SAHPI_SU_CU_IN"},
   {SAHPI_SU_CU_FEET,              "SAHPI_SU_CU_FEET"},
   {SAHPI_SU_MM,                   "SAHPI_SU_MM"},
   {SAHPI_SU_CM,                   "SAHPI_SU_CM"},
   {SAHPI_SU_M,                    "SAHPI_SU_M"},
   {SAHPI_SU_CU_CM,                "SAHPI_SU_CU_CM"},
   {SAHPI_SU_CU_M,                 "SAHPI_SU_CU_M"},
   {SAHPI_SU_LITERS,               "SAHPI_SU_LITERS"},
   {SAHPI_SU_FLUID_OUNCE,          "SAHPI_SU_FLUID_OUNCE"},
   {SAHPI_SU_RADIANS,              "SAHPI_SU_RADIANS"},
   {SAHPI_SU_STERADIANS,           "SAHPI_SU_STERADIANS"},
   {SAHPI_SU_REVOLUTIONS,          "SAHPI_SU_REVOLUTIONS"},
   {SAHPI_SU_CYCLES,               "SAHPI_SU_CYCLES"},
   {SAHPI_SU_GRAVITIES,            "SAHPI_SU_GRAVITIES"},
   {SAHPI_SU_OUNCE,                "SAHPI_SU_OUNCE"},
   {SAHPI_SU_POUND,                "SAHPI_SU_POUND"},
   {SAHPI_SU_FT_LB,                "SAHPI_SU_FT_LB"},
   {SAHPI_SU_OZ_IN,                "SAHPI_SU_OZ_IN"},
   {SAHPI_SU_GAUSS,                "SAHPI_SU_GAUSS"},
   {SAHPI_SU_GILBERTS,             "SAHPI_SU_GILBERTS"},
   {SAHPI_SU_HENRY,                "SAHPI_SU_HENRY"},
   {SAHPI_SU_MILLIHENRY,           "SAHPI_SU_MILLIHENRY"},
   {SAHPI_SU_FARAD,                "SAHPI_SU_FARAD"},
   {SAHPI_SU_MICROFARAD,           "SAHPI_SU_MICROFARAD"},
   {SAHPI_SU_OHMS,                 "SAHPI_SU_OHMS"},
   {SAHPI_SU_SIEMENS,              "SAHPI_SU_SIEMENS"},
   {SAHPI_SU_MOLE,                 "SAHPI_SU_MOLE"},
   {SAHPI_SU_BECQUEREL,            "SAHPI_SU_BECQUEREL"},
   {SAHPI_SU_PPM,                  "SAHPI_SU_PPM"},
   {SAHPI_SU_RESERVED,             "SAHPI_SU_RESERVED"},
   {SAHPI_SU_DECIBELS,             "SAHPI_SU_DECIBELS"},
   {SAHPI_SU_DBA,                  "SAHPI_SU_DBA"},
   {SAHPI_SU_DBC,                  "SAHPI_SU_DBC"},
   {SAHPI_SU_GRAY,                 "SAHPI_SU_GRAY"},
   {SAHPI_SU_SIEVERT,              "SAHPI_SU_SIEVERT"},
   {SAHPI_SU_COLOR_TEMP_DEG_K,     "SAHPI_SU_COLOR_TEMP_DEG_K"},
   {SAHPI_SU_BIT,                  "SAHPI_SU_BIT"},
   {SAHPI_SU_KILOBIT,              "SAHPI_SU_KILOBIT"},
   {SAHPI_SU_MEGABIT,              "SAHPI_SU_MEGABIT"},
   {SAHPI_SU_GIGABIT,              "SAHPI_SU_GIGABIT"},
   {SAHPI_SU_BYTE,                 "SAHPI_SU_BYTE"},
   {SAHPI_SU_KILOBYTE,             "SAHPI_SU_KILOBYTE"},
   {SAHPI_SU_MEGABYTE,             "SAHPI_SU_MEGABYTE"},
   {SAHPI_SU_GIGABYTE,             "SAHPI_SU_GIGABYTE"},
   {SAHPI_SU_WORD,                 "SAHPI_SU_WORD"},
   {SAHPI_SU_DWORD,                "SAHPI_SU_DWORD"},
   {SAHPI_SU_QWORD,                "SAHPI_SU_QWORD"},
   {SAHPI_SU_LINE,                 "SAHPI_SU_LINE"},
   {SAHPI_SU_HIT,                  "SAHPI_SU_HIT"},
   {SAHPI_SU_MISS,                 "SAHPI_SU_MISS"},
   {SAHPI_SU_RETRY,                "SAHPI_SU_RETRY"},
   {SAHPI_SU_RESET,                "SAHPI_SU_RESET"},
   {SAHPI_SU_OVERRUN,              "SAHPI_SU_OVERRUN"},
   {SAHPI_SU_UNDERRUN,             "SAHPI_SU_UNDERRUN"},
   {SAHPI_SU_COLLISION,            "SAHPI_SU_COLLISION"},
   {SAHPI_SU_PACKETS,              "SAHPI_SU_PACKETS"},
   {SAHPI_SU_MESSAGES,             "SAHPI_SU_MESSAGES"},
   {SAHPI_SU_CHARACTERS,           "SAHPI_SU_CHARACTERS"},
   {SAHPI_SU_ERRORS,               "SAHPI_SU_ERRORS"},
   {SAHPI_SU_CORRECTABLE_ERRORS,   "SAHPI_SU_CORRECTABLE_ERRORS"},
   {SAHPI_SU_UNCORRECTABLE_ERRORS, "SAHPI_SU_UNCORRECTABLE_ERRORS"},
   {SAHPI_SU_UNSPECIFIED,          NULL}
};


/**
 * Translates a string to a valid SaHpiSensorUnitsT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiSensorUnitsT oSaHpiTypesEnums::str2sensorunits(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_SU_UNSPECIFIED;
    }
	for (i = 0; sensorunits_strings[i].str != NULL; i++) {
		if (strcmp(strtype, sensorunits_strings[i].str) == 0) {
            return sensorunits_strings[i].type;
		}
	}
    return SAHPI_SU_UNSPECIFIED;
}


/**
 * Translates an sensor reading type to a string.
 *
 * @param value  The SaHpiSensorUnitsT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::sensorunits2str(SaHpiSensorUnitsT value) {
	int i;

	for (i = 0; sensorunits_strings[i].str != NULL; i++) {
		if (value == sensorunits_strings[i].type) {
			return sensorunits_strings[i].str;
		}
	}
    return "Unknown";
}


static struct sensormodunituse_map {
    SaHpiSensorModUnitUseT type;
    const char             *str;
} sensormodunituse_strings[] = {
   {SAHPI_SMUU_NONE,                 "SAHPI_SMUU_NONE"},
   {SAHPI_SMUU_BASIC_OVER_MODIFIER,  "SAHPI_SMUU_BASIC_OVER_MODIFIER"},
   {SAHPI_SMUU_BASIC_TIMES_MODIFIER, "SAHPI_SMUU_BASIC_TIMES_MODIFIER"},
   {SAHPI_SMUU_NONE,                 NULL}
};


/**
 * Translates a string to a valid SaHpiSensorModUnitsUseT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiSensorModUnitUseT oSaHpiTypesEnums::str2sensoruse(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_SMUU_NONE;
    }
	for (i = 0; sensormodunituse_strings[i].str != NULL; i++) {
		if (strcmp(strtype, sensormodunituse_strings[i].str) == 0) {
            return sensormodunituse_strings[i].type;
		}
	}
    return SAHPI_SMUU_NONE;
}


/**
 * Translates an sensor reading type to a string.
 *
 * @param value  The SaHpiSensorModUnitUseT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::sensoruse2str(SaHpiSensorModUnitUseT value) {
	int i;

	for (i = 0; sensormodunituse_strings[i].str != NULL; i++) {
		if (value == sensormodunituse_strings[i].type) {
			return sensormodunituse_strings[i].str;
		}
	}
    return "Unknown";
}


static struct sensorthdmasktype_map {
    SaHpiSensorThdMaskT type;
    const char          *str;
} sensorthdmask_strings[] = {
       {SAHPI_STM_LOW_MINOR,       "SAHPI_STM_LOW_MINOR"},
       {SAHPI_STM_LOW_MAJOR,       "SAHPI_STM_LOW_MAJOR"},
       {SAHPI_STM_LOW_CRIT,        "SAHPI_STM_LOW_CRIT"},
       {SAHPI_STM_UP_MAJOR,        "SAHPI_STM_UP_MAJOR"},
       {SAHPI_STM_UP_MINOR,        "SAHPI_STM_UP_MINOR"},
       {SAHPI_STM_UP_CRIT,         "SAHPI_STM_UP_CRIT"},
       {SAHPI_STM_UP_HYSTERESIS,   "SAHPI_STM_UP_HYSTERESIS"},
       {SAHPI_STM_LOW_HYSTERESIS,  "SAHPI_STM_LOW_HYSTERESIS"},
       {0,                         NULL},
};


/**
 * Translates a string to a valid SaHpiSensorThdMaskT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiSensorThdMaskT oSaHpiTypesEnums::str2sensorthdmask(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return 0;
    }
	for (i = 0; sensorthdmask_strings[i].str != NULL; i++) {
		if (strcmp(strtype, sensorthdmask_strings[i].str) == 0) {
            return sensorthdmask_strings[i].type;
		}
	}
	return 0;
}


/**
 * Translates an sensor reading type to a string.
 *
 * @param value  The SaHpiSensorThdMaskT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::sensorthdmask2str(SaHpiSensorThdMaskT value) {
	int i;

	for (i = 0; sensorthdmask_strings[i].str != NULL; i++) {
		if (value == sensorthdmask_strings[i].type) {
			return sensorthdmask_strings[i].str;
		}
	}
    return "Unknown";
}


static struct sensoreventctrl_map {
    SaHpiSensorEventCtrlT type;
    const char            *str;
} sensoreventctrl_strings[] = {
    {SAHPI_SEC_PER_EVENT,       "SAHPI_SEC_PER_EVENT"},
    {SAHPI_SEC_READ_ONLY_MASKS, "SAHPI_SEC_READ_ONLY_MASKS"},
    {SAHPI_SEC_READ_ONLY,       "SAHPI_SEC_READ_ONLY"},
    {SAHPI_SEC_READ_ONLY,       NULL}
};


/**
 * Translates a string to a valid SaHpiSensorEventCtrlT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiSensorEventCtrlT oSaHpiTypesEnums::str2sensoreventctrl(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_SEC_PER_EVENT;
    }
	for (i = 0; sensoreventctrl_strings[i].str != NULL; i++) {
		if (strcmp(strtype, sensoreventctrl_strings[i].str) == 0) {
            return sensoreventctrl_strings[i].type;
		}
	}
    return SAHPI_SEC_PER_EVENT;
}


/**
 * Translates an sensor reading type to a string.
 *
 * @param value  The SaHpiSensorEventCtrlT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::sensoreventctrl2str(SaHpiSensorEventCtrlT value) {
	int i;

	for (i = 0; sensoreventctrl_strings[i].str != NULL; i++) {
		if (value == sensoreventctrl_strings[i].type) {
			return sensoreventctrl_strings[i].str;
		}
	}
    return "Unknown";
}


static struct sensortype_map {
    SaHpiSensorTypeT type;
    const char       *str;
} sensortype_strings[] = {
   {SAHPI_TEMPERATURE,                 "SAHPI_TEMPERATURE"},
   {SAHPI_VOLTAGE,                     "SAHPI_VOLTAGE"},
   {SAHPI_CURRENT,                     "SAHPI_CURRENT"},
   {SAHPI_FAN,                         "SAHPI_FAN"},
   {SAHPI_PHYSICAL_SECURITY,           "SAHPI_PHYSICAL_SECURITY"},
   {SAHPI_PLATFORM_VIOLATION,          "SAHPI_PLATFORM_VIOLATION"},
   {SAHPI_PROCESSOR,                   "SAHPI_PROCESSOR"},
   {SAHPI_POWER_SUPPLY,                "SAHPI_POWER_SUPPLY"},
   {SAHPI_POWER_UNIT,                  "SAHPI_POWER_UNIT"},
   {SAHPI_COOLING_DEVICE,              "SAHPI_COOLING_DEVICE"},
   {SAHPI_OTHER_UNITS_BASED_SENSOR,    "SAHPI_OTHER_UNITS_BASED_SENSOR"},
   {SAHPI_MEMORY,                      "SAHPI_MEMORY"},
   {SAHPI_DRIVE_SLOT,                  "SAHPI_DRIVE_SLOT"},
   {SAHPI_POST_MEMORY_RESIZE,          "SAHPI_POST_MEMORY_RESIZE"},
   {SAHPI_SYSTEM_FW_PROGRESS,          "SAHPI_SYSTEM_FW_PROGRESS"},
   {SAHPI_EVENT_LOGGING_DISABLED,      "SAHPI_EVENT_LOGGING_DISABLED"},
   {SAHPI_RESERVED1,                   "SAHPI_RESERVED1"},
   {SAHPI_SYSTEM_EVENT,                "SAHPI_SYSTEM_EVENT"},
   {SAHPI_CRITICAL_INTERRUPT,          "SAHPI_CRITICAL_INTERRUPT"},
   {SAHPI_BUTTON,                      "SAHPI_BUTTON"},
   {SAHPI_MODULE_BOARD,                "SAHPI_MODULE_BOARD"},
   {SAHPI_MICROCONTROLLER_COPROCESSOR, "SAHPI_MICROCONTROLLER_COPROCESSOR"},
   {SAHPI_ADDIN_CARD,                  "SAHPI_ADDIN_CARD"},
   {SAHPI_CHASSIS,                     "SAHPI_CHASSIS"},
   {SAHPI_CHIP_SET,                    "SAHPI_CHIP_SET"},
   {SAHPI_OTHER_FRU,                   "SAHPI_OTHER_FRU"},
   {SAHPI_CABLE_INTERCONNECT,          "SAHPI_CABLE_INTERCONNECT"},
   {SAHPI_TERMINATOR,                  "SAHPI_TERMINATOR"},
   {SAHPI_SYSTEM_BOOT_INITIATED,       "SAHPI_SYSTEM_BOOT_INITIATED"},
   {SAHPI_BOOT_ERROR,                  "SAHPI_BOOT_ERROR"},
   {SAHPI_OS_BOOT,                     "SAHPI_OS_BOOT"},
   {SAHPI_OS_CRITICAL_STOP,            "SAHPI_OS_CRITICAL_STOP"},
   {SAHPI_SLOT_CONNECTOR,              "SAHPI_SLOT_CONNECTOR"},
   {SAHPI_SYSTEM_ACPI_POWER_STATE,     "SAHPI_SYSTEM_ACPI_POWER_STATE"},
   {SAHPI_RESERVED2,                   "SAHPI_RESERVED2"},
   {SAHPI_PLATFORM_ALERT,              "SAHPI_PLATFORM_ALERT"},
   {SAHPI_ENTITY_PRESENCE,             "SAHPI_ENTITY_PRESENCE"},
   {SAHPI_MONITOR_ASIC_IC,             "SAHPI_MONITOR_ASIC_IC"},
   {SAHPI_LAN,                         "SAHPI_LAN"},
   {SAHPI_MANAGEMENT_SUBSYSTEM_HEALTH, "SAHPI_MANAGEMENT_SUBSYSTEM_HEALTH"},
   {SAHPI_BATTERY,                     "SAHPI_BATTERY"},
   {SAHPI_OPERATIONAL,                 "SAHPI_OPERATIONAL"},
   {SAHPI_OEM_SENSOR,                  "SAHPI_OEM_SENSOR"},
   {SAHPI_OEM_SENSOR,                  NULL}
};


/**
 * Translates a string to a valid SaHpiSensorTypeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiSensorTypeT oSaHpiTypesEnums::str2sensortype(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_TEMPERATURE;
    }
	for (i = 0; sensortype_strings[i].str != NULL; i++) {
		if (strcmp(strtype, sensortype_strings[i].str) == 0) {
            return sensortype_strings[i].type;
		}
	}
    return SAHPI_TEMPERATURE;
}


/**
 * Translates an sensor reading type to a string.
 *
 * @param value  The SaHpiSensorTypeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::sensortype2str(SaHpiSensorTypeT value) {
	int i;

	for (i = 0; sensortype_strings[i].str != NULL; i++) {
		if (value == sensortype_strings[i].type) {
			return sensortype_strings[i].str;
		}
	}
    return "Unknown";
}


static struct eventcategory_map {
    SaHpiEventCategoryT type;
    const char          *str;
} eventcategory_strings[] = {
   {SAHPI_EC_UNSPECIFIED,     "SAHPI_EC_UNSPECIFIED"},
   {SAHPI_EC_THRESHOLD,       "SAHPI_EC_THRESHOLD"},
   {SAHPI_EC_USAGE,           "SAHPI_EC_USAGE"},
   {SAHPI_EC_STATE,           "SAHPI_EC_STATE"},
   {SAHPI_EC_PRED_FAIL,       "SAHPI_EC_PRED_FAIL"},
   {SAHPI_EC_LIMIT,           "SAHPI_EC_LIMIT"},
   {SAHPI_EC_PERFORMANCE,     "SAHPI_EC_PERFORMANCE"},
   {SAHPI_EC_SEVERITY,        "SAHPI_EC_SEVERITY"},
   {SAHPI_EC_PRESENCE,        "SAHPI_EC_PRESENCE"},
   {SAHPI_EC_ENABLE,          "SAHPI_EC_ENABLE"},
   {SAHPI_EC_AVAILABILITY,    "SAHPI_EC_AVAILABILITY"},
   {SAHPI_EC_REDUNDANCY,      "SAHPI_EC_REDUNDANCY"},
   {SAHPI_EC_SENSOR_SPECIFIC, "SAHPI_EC_SENSOR_SPECIFIC"},
   {SAHPI_EC_GENERIC,         "SAHPI_EC_GENERIC"},
   {SAHPI_EC_GENERIC,         NULL},
};


/**
 * Translates a string to a valid SaHpiEventCategoryT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiEventCategoryT oSaHpiTypesEnums::str2eventcategory(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_EC_UNSPECIFIED;
    }
	for (i = 0; eventcategory_strings[i].str != NULL; i++) {
		if (strcmp(strtype, eventcategory_strings[i].str) == 0) {
            return eventcategory_strings[i].type;
		}
	}
    return SAHPI_EC_UNSPECIFIED;
}


/**
 * Translates an sensor reading type to a string.
 *
 * @param value  The SaHpiEventCategoryT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::eventcategory2str(SaHpiEventCategoryT value) {
	int i;

	for (i = 0; eventcategory_strings[i].str != NULL; i++) {
		if (value == eventcategory_strings[i].type) {
			return eventcategory_strings[i].str;
		}
	}
    return "Unknown";
}


static struct eventstate_map {
    SaHpiEventStateT type;
    const char       *str;
} eventstate_strings[] = {
   {SAHPI_ES_UNSPECIFIED,                          "SAHPI_ES_UNSPECIFIED"},
   {SAHPI_ES_LOWER_MINOR,                          "SAHPI_ES_LOWER_MINOR"},
   {SAHPI_ES_LOWER_MAJOR,                          "SAHPI_ES_LOWER_MAJOR"},
   {SAHPI_ES_LOWER_CRIT,                           "SAHPI_ES_LOWER_CRIT"},
   {SAHPI_ES_UPPER_MINOR,                          "SAHPI_ES_UPPER_MINOR"},
   {SAHPI_ES_UPPER_MAJOR,                          "SAHPI_ES_UPPER_MAJOR"},
   {SAHPI_ES_UPPER_CRIT,                           "SAHPI_ES_UPPER_CRIT"},
   {SAHPI_ES_IDLE,                                 "SAHPI_ES_IDLE"},
   {SAHPI_ES_ACTIVE,                               "SAHPI_ES_ACTIVE"},
   {SAHPI_ES_BUSY,                                 "SAHPI_ES_BUSY"},
   {SAHPI_ES_STATE_DEASSERTED,                     "SAHPI_ES_STATE_DEASSERTED"},
   {SAHPI_ES_STATE_ASSERTED,                       "SAHPI_ES_STATE_ASSERTED"},
   {SAHPI_ES_PRED_FAILURE_DEASSERT,                "SAHPI_ES_PRED_FAILURE_DEASSERT"},
   {SAHPI_ES_PRED_FAILURE_ASSERT,                  "SAHPI_ES_PRED_FAILURE_ASSERT"},
   {SAHPI_ES_LIMIT_NOT_EXCEEDED,                   "SAHPI_ES_LIMIT_NOT_EXCEEDED"},
   {SAHPI_ES_LIMIT_EXCEEDED,                       "SAHPI_ES_LIMIT_EXCEEDED"},
   {SAHPI_ES_PERFORMANCE_MET,                      "SAHPI_ES_PERFORMANCE_MET"},
   {SAHPI_ES_PERFORMANCE_LAGS,                     "SAHPI_ES_PERFORMANCE_LAGS"},
   {SAHPI_ES_OK,                                   "SAHPI_ES_OK"},
   {SAHPI_ES_MINOR_FROM_OK,                        "SAHPI_ES_MINOR_FROM_OK"},
   {SAHPI_ES_MAJOR_FROM_LESS,                      "SAHPI_ES_MAJOR_FROM_LESS"},
   {SAHPI_ES_CRITICAL_FROM_LESS,                   "SAHPI_ES_CRITICAL_FROM_LESS"},
   {SAHPI_ES_MINOR_FROM_MORE,                      "SAHPI_ES_MINOR_FROM_MORE"},
   {SAHPI_ES_MAJOR_FROM_CRITICAL,                  "SAHPI_ES_MAJOR_FROM_CRITICAL"},
   {SAHPI_ES_CRITICAL,                             "SAHPI_ES_CRITICAL"},
   {SAHPI_ES_MONITOR,                              "SAHPI_ES_MONITOR"},
   {SAHPI_ES_INFORMATIONAL,                        "SAHPI_ES_INFORMATIONAL"},
   {SAHPI_ES_ABSENT,                               "SAHPI_ES_ABSENT"},
   {SAHPI_ES_PRESENT,                              "SAHPI_ES_PRESENT"},
   {SAHPI_ES_DISABLED,                             "SAHPI_ES_DISABLED"},
   {SAHPI_ES_ENABLED,                              "SAHPI_ES_ENABLED"},
   {SAHPI_ES_RUNNING,                              "SAHPI_ES_RUNNING"},
   {SAHPI_ES_TEST,                                 "SAHPI_ES_TEST"},
   {SAHPI_ES_POWER_OFF,                            "SAHPI_ES_POWER_OFF"},
   {SAHPI_ES_ON_LINE,                              "SAHPI_ES_ON_LINE"},
   {SAHPI_ES_OFF_LINE,                             "SAHPI_ES_OFF_LINE"},
   {SAHPI_ES_OFF_DUTY,                             "SAHPI_ES_OFF_DUTY"},
   {SAHPI_ES_DEGRADED,                             "SAHPI_ES_DEGRADED"},
   {SAHPI_ES_POWER_SAVE,                           "SAHPI_ES_POWER_SAVE"},
   {SAHPI_ES_INSTALL_ERROR,                        "SAHPI_ES_INSTALL_ERROR"},
   {SAHPI_ES_FULLY_REDUNDANT,                      "SAHPI_ES_FULLY_REDUNDANT"},
   {SAHPI_ES_REDUNDANCY_LOST,                      "SAHPI_ES_REDUNDANCY_LOST"},
   {SAHPI_ES_REDUNDANCY_DEGRADED,                  "SAHPI_ES_REDUNDANCY_DEGRADED"},
   {SAHPI_ES_REDUNDANCY_LOST_SUFFICIENT_RESOURCES, "SAHPI_ES_REDUNDANCY_LOST_SUFFICIENT_RESOURCES"},
   {SAHPI_ES_NON_REDUNDANT_SUFFICIENT_RESOURCES,   "SAHPI_ES_NON_REDUNDANT_SUFFICIENT_RESOURCES"},
   {SAHPI_ES_NON_REDUNDANT_INSUFFICIENT_RESOURCES, "SAHPI_ES_NON_REDUNDANT_INSUFFICIENT_RESOURCES"},
   {SAHPI_ES_REDUNDANCY_DEGRADED_FROM_FULL,        "SAHPI_ES_REDUNDANCY_DEGRADED_FROM_FULL"},
   {SAHPI_ES_REDUNDANCY_DEGRADED_FROM_NON,         "SAHPI_ES_REDUNDANCY_DEGRADED_FROM_NON"},
   {SAHPI_ES_STATE_00,                             "SAHPI_ES_STATE_00"},
   {SAHPI_ES_STATE_01,                             "SAHPI_ES_STATE_01"},
   {SAHPI_ES_STATE_02,                             "SAHPI_ES_STATE_02"},
   {SAHPI_ES_STATE_03,                             "SAHPI_ES_STATE_03"},
   {SAHPI_ES_STATE_04,                             "SAHPI_ES_STATE_04"},
   {SAHPI_ES_STATE_05,                             "SAHPI_ES_STATE_05"},
   {SAHPI_ES_STATE_06,                             "SAHPI_ES_STATE_06"},
   {SAHPI_ES_STATE_07,                             "SAHPI_ES_STATE_07"},
   {SAHPI_ES_STATE_08,                             "SAHPI_ES_STATE_08"},
   {SAHPI_ES_STATE_09,                             "SAHPI_ES_STATE_09"},
   {SAHPI_ES_STATE_10,                             "SAHPI_ES_STATE_10"},
   {SAHPI_ES_STATE_11,                             "SAHPI_ES_STATE_11"},
   {SAHPI_ES_STATE_12,                             "SAHPI_ES_STATE_12"},
   {SAHPI_ES_STATE_13,                             "SAHPI_ES_STATE_13"},
   {SAHPI_ES_STATE_14,                             "SAHPI_ES_STATE_14"},
   {SAHPI_ES_STATE_14,                             NULL}
};


/**
 * Translates a string to a valid SaHpiEventStateT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiEventStateT oSaHpiTypesEnums::str2eventstate(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_ES_UNSPECIFIED;
    }
	for (i = 0; eventstate_strings[i].str != NULL; i++) {
		if (strcmp(strtype, eventstate_strings[i].str) == 0) {
            return eventstate_strings[i].type;
		}
	}
    return SAHPI_ES_UNSPECIFIED;
}


/**
 * Translates an sensor reading type to a string.
 *
 * @param value  The SaHpiEventStateT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::eventstate2str(SaHpiEventStateT value) {
	int i;

	for (i = 0; eventstate_strings[i].str != NULL; i++) {
		if (value == eventstate_strings[i].type) {
			return eventstate_strings[i].str;
		}
	}
    return "Unknown";
}


static struct ctrltype_map {
    SaHpiCtrlTypeT type;
    const char     *str;
} ctrltype_strings[] = {
   {SAHPI_CTRL_TYPE_DIGITAL,  "SAHPI_CTRL_TYPE_DIGITAL"},
   {SAHPI_CTRL_TYPE_DISCRETE, "SAHPI_CTRL_TYPE_DISCRETE"},
   {SAHPI_CTRL_TYPE_ANALOG,   "SAHPI_CTRL_TYPE_ANALOG"},
   {SAHPI_CTRL_TYPE_STREAM,   "SAHPI_CTRL_TYPE_STREAM"},
   {SAHPI_CTRL_TYPE_TEXT,     "SAHPI_CTRL_TYPE_TEXT"},
   {SAHPI_CTRL_TYPE_OEM,      "SAHPI_CTRL_TYPE_OEM"},
   {SAHPI_CTRL_TYPE_OEM,      NULL}
};


/**
 * Translates a string to a valid SaHpiCtrlTypeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiCtrlTypeT oSaHpiTypesEnums::str2ctrltype(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_CTRL_TYPE_DIGITAL;
    }
	for (i = 0; ctrltype_strings[i].str != NULL; i++) {
		if (strcmp(strtype, ctrltype_strings[i].str) == 0) {
            return ctrltype_strings[i].type;
		}
	}
    return SAHPI_CTRL_TYPE_DIGITAL;
}


/**
 * Translates an sensor reading type to a string.
 *
 * @param value  The SaHpiCtrlTypeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::ctrltype2str(SaHpiCtrlTypeT value) {
	int i;

	for (i = 0; ctrltype_strings[i].str != NULL; i++) {
		if (value == ctrltype_strings[i].type) {
			return ctrltype_strings[i].str;
		}
	}
    return "Unknown";
}


static struct ctrlstatedigital_map {
    SaHpiCtrlStateDigitalT type;
    const char             *str;
} ctrlstatedigital_strings[] = {
   {SAHPI_CTRL_STATE_OFF,       "SAHPI_CTRL_STATE_OFF"},
   {SAHPI_CTRL_STATE_ON,        "SAHPI_CTRL_STATE_ON"},
   {SAHPI_CTRL_STATE_PULSE_OFF, "SAHPI_CTRL_STATE_PULSE_OFF"},
   {SAHPI_CTRL_STATE_PULSE_ON,  "SAHPI_CTRL_STATE_PULSE_ON"},
   {SAHPI_CTRL_STATE_PULSE_ON,  NULL}
};


/**
 * Translates a string to a valid SaHpiCtrlStateDigitalT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiCtrlStateDigitalT oSaHpiTypesEnums::str2ctrlstatedigital(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_CTRL_STATE_OFF;
    }
	for (i = 0; ctrlstatedigital_strings[i].str != NULL; i++) {
		if (strcmp(strtype, ctrlstatedigital_strings[i].str) == 0) {
            return ctrlstatedigital_strings[i].type;
		}
	}
    return SAHPI_CTRL_STATE_OFF;
}


/**
 * Translates an sensor reading type to a string.
 *
 * @param value  The SaHpiCtrlStateDigitalT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::ctrlstatedigital2str(SaHpiCtrlStateDigitalT value) {
	int i;

	for (i = 0; ctrlstatedigital_strings[i].str != NULL; i++) {
		if (value == ctrlstatedigital_strings[i].type) {
			return ctrlstatedigital_strings[i].str;
		}
	}
    return "Unknown";
}


static struct aggregatestatus_map {
    SaHpiUint32T type;
    const char   *str;
} aggregatestatus_strings[] = {
   {SAHPI_DEFAGSENS_OPER, "SAHPI_DEFAGSENS_OPRR"},
   {SAHPI_DEFAGSENS_PWR,  "SAHPI_DEFAGSENS_PWR"},
   {SAHPI_DEFAGSENS_TEMP, "SAHPI_DEFAGSENS_TEMP"},
   {SAHPI_DEFAGSENS_MIN,  "SAHPI_DEFAGSENS_MIN"},
   {SAHPI_DEFAGSENS_MAX,  "SAHPI_DEFAGSENS_MAX"},
   {SAHPI_DEFAGSENS_MAX,  NULL}
};


/**
 * Translates a string to a valid SaHpiUint32T type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiUint32T oSaHpiTypesEnums::str2aggregatestatus(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return 0;
    }
	for (i = 0; aggregatestatus_strings[i].str != NULL; i++) {
		if (strcmp(strtype, aggregatestatus_strings[i].str) == 0) {
            return aggregatestatus_strings[i].type;
		}
	}
    return 0;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiUint32T to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::aggregatestatus2str(SaHpiUint32T value) {
	int i;

	for (i = 0; aggregatestatus_strings[i].str != NULL; i++) {
		if (value == aggregatestatus_strings[i].type) {
			return aggregatestatus_strings[i].str;
		}
	}
    return "Unknown";
}


static struct ctrloutputtype_map {
    SaHpiCtrlOutputTypeT type;
    const char           *str;
} ctrloutputtype_strings[] = {
    {SAHPI_CTRL_GENERIC,              "SAHPI_CTRL_GENERIC"},
    {SAHPI_CTRL_LED,                  "SAHPI_CTRL_LED"},
    {SAHPI_CTRL_FAN_SPEED,            "SAHPI_CTRL_FAN_SPEED"},
    {SAHPI_CTRL_DRY_CONTACT_CLOSURE,  "SAHPI_CTRL_DRY_CONTACT_CLOSURE"},
    {SAHPI_CTRL_POWER_SUPPLY_INHIBIT, "SAHPI_CTRL_POWER_SUPPLY_INHIBIT"},
    {SAHPI_CTRL_AUDIBLE,              "SAHPI_CTRL_AUDIBLE"},
    {SAHPI_CTRL_FRONT_PANEL_LOCKOUT,  "SAHPI_CTRL_FRONT_PANEL_LOCKOUT"},
    {SAHPI_CTRL_POWER_INTERLOCK,      "SAHPI_CTRL_POWER_INTERLOCK"},
    {SAHPI_CTRL_POWER_STATE,          "SAHPI_CTRL_POWER_STATE"},
    {SAHPI_CTRL_LCD_DISPLAY,          "SAHPI_CTRL_LCD_DISPLAY"},
    {SAHPI_CTRL_OEM,                  "SAHPI_CTRL_OEM"},
    {SAHPI_CTRL_OEM,                  NULL}
};


/**
 * Translates a string to a valid SaHpiCtrlOutputTypeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiCtrlOutputTypeT oSaHpiTypesEnums::str2ctrloutputtype(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_CTRL_GENERIC;
    }
	for (i = 0; ctrloutputtype_strings[i].str != NULL; i++) {
		if (strcmp(strtype, ctrloutputtype_strings[i].str) == 0) {
            return ctrloutputtype_strings[i].type;
		}
	}
    return SAHPI_CTRL_GENERIC;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiCtrlOutputTypeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::ctrloutputtype2str(SaHpiCtrlOutputTypeT value) {
	int i;

	for (i = 0; ctrloutputtype_strings[i].str != NULL; i++) {
		if (value == ctrloutputtype_strings[i].type) {
			return ctrloutputtype_strings[i].str;
		}
	}
    return "Unknown";
}


static struct ctrlmode_map {
    SaHpiCtrlModeT type;
    const char     *str;
} ctrlmode_strings[] = {
    {SAHPI_CTRL_MODE_AUTO,   "SAHPI_CTRL_MODE_AUTO"},
    {SAHPI_CTRL_MODE_MANUAL, "SAHPI_CTRL_MODE_MANUAL"},
    {SAHPI_CTRL_MODE_MANUAL, NULL}
};


/**
 * Translates a string to a valid SaHpiCtrlModeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiCtrlModeT oSaHpiTypesEnums::str2ctrlmode(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_CTRL_MODE_AUTO;
    }
	for (i = 0; ctrlmode_strings[i].str != NULL; i++) {
		if (strcmp(strtype, ctrlmode_strings[i].str) == 0) {
            return ctrlmode_strings[i].type;
		}
	}
    return SAHPI_CTRL_MODE_AUTO;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiCtrlModeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::ctrlmode2str(SaHpiCtrlModeT value) {
	int i;

	for (i = 0; ctrlmode_strings[i].str != NULL; i++) {
		if (value == ctrlmode_strings[i].type) {
			return ctrlmode_strings[i].str;
		}
	}
    return "Unknown";
}


static struct idrareatype_map {
    SaHpiIdrAreaTypeT type;
    const char        *str;
} idrareatype_strings[] = {
    {SAHPI_IDR_AREATYPE_INTERNAL_USE, "SAHPI_IDR_AREATYPE_INTERNAL_USE"},
    {SAHPI_IDR_AREATYPE_CHASSIS_INFO, "SAHPI_IDR_AREATYPE_CHASSIS_INFO"},
    {SAHPI_IDR_AREATYPE_BOARD_INFO,   "SAHPI_IDR_AREATYPE_BOARD_INFO"},
    {SAHPI_IDR_AREATYPE_PRODUCT_INFO, "SAHPI_IDR_AREATYPE_PRODUCT_INFO"},
    {SAHPI_IDR_AREATYPE_OEM,          "SAHPI_IDR_AREATYPE_OEM"},
    {SAHPI_IDR_AREATYPE_UNSPECIFIED,  "SAHPI_IDR_AREATYPE_UNSPECIFIED"},
    {SAHPI_IDR_AREATYPE_UNSPECIFIED,  NULL}
};


/**
 * Translates a string to a valid SaHpiIdrAreaTypeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiIdrAreaTypeT oSaHpiTypesEnums::str2idrareatype(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_IDR_AREATYPE_UNSPECIFIED;
    }
	for (i = 0; idrareatype_strings[i].str != NULL; i++) {
		if (strcmp(strtype, idrareatype_strings[i].str) == 0) {
            return idrareatype_strings[i].type;
		}
	}
    return SAHPI_IDR_AREATYPE_UNSPECIFIED;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiIdrAreaTypeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::idrareatype2str(SaHpiIdrAreaTypeT value) {
	int i;

	for (i = 0; idrareatype_strings[i].str != NULL; i++) {
		if (value == idrareatype_strings[i].type) {
			return idrareatype_strings[i].str;
		}
	}
    return "Unknown";
}


static struct idrfieldtype_map {
    SaHpiIdrFieldTypeT type;
    const char         *str;
} idrfieldtype_strings[] = {
    {SAHPI_IDR_FIELDTYPE_CHASSIS_TYPE,    "SAHPI_IDR_FIELDTYPE_CHASSIS_TYPE"},
    {SAHPI_IDR_FIELDTYPE_MFG_DATETIME,    "SAHPI_IDR_FIELDTYPE_MFG_DATETIME"},
    {SAHPI_IDR_FIELDTYPE_MANUFACTURER,    "SAHPI_IDR_FIELDTYPE_MANUFACTURER"},
    {SAHPI_IDR_FIELDTYPE_PRODUCT_NAME,    "SAHPI_IDR_FIELDTYPE_PRODUCT_NAME"},
    {SAHPI_IDR_FIELDTYPE_PRODUCT_VERSION, "SAHPI_IDR_FIELDTYPE_PRODUCT_VERSION"},
    {SAHPI_IDR_FIELDTYPE_SERIAL_NUMBER,   "SAHPI_IDR_FIELDTYPE_SERIAL_NUMBER"},
    {SAHPI_IDR_FIELDTYPE_PART_NUMBER,     "SAHPI_IDR_FIELDTYPE_PART_NUMBER"},
    {SAHPI_IDR_FIELDTYPE_FILE_ID,         "SAHPI_IDR_FIELDTYPE_FILE_ID"},
    {SAHPI_IDR_FIELDTYPE_ASSET_TAG,       "SAHPI_IDR_FIELDTYPE_ASSET_TAG"},
    {SAHPI_IDR_FIELDTYPE_CUSTOM,          "SAHPI_IDR_FIELDTYPE_CUSTOM"},
    {SAHPI_IDR_FIELDTYPE_UNSPECIFIED,     "SAHPI_IDR_FIELDTYPE_UNSPECIFIED"},
    {SAHPI_IDR_FIELDTYPE_UNSPECIFIED,     NULL}
};


/**
 * Translates a string to a valid SaHpiIdrFieldTypeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiIdrFieldTypeT oSaHpiTypesEnums::str2idrfieldtype(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_IDR_FIELDTYPE_UNSPECIFIED;
    }
	for (i = 0; idrfieldtype_strings[i].str != NULL; i++) {
		if (strcmp(strtype, idrfieldtype_strings[i].str) == 0) {
            return idrfieldtype_strings[i].type;
		}
	}
    return SAHPI_IDR_FIELDTYPE_UNSPECIFIED;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiIdrFieldTypeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::idrfieldtype2str(SaHpiIdrFieldTypeT value) {
	int i;

	for (i = 0; idrfieldtype_strings[i].str != NULL; i++) {
		if (value == idrfieldtype_strings[i].type) {
			return idrfieldtype_strings[i].str;
		}
	}
    return "Unknown";
}


static struct watchdogaction_map {
    SaHpiWatchdogActionT type;
    const char           *str;
} watchdogaction_strings[] = {
    {SAHPI_WA_NO_ACTION,   "SAHPI_WA_NO_ACTION"},
    {SAHPI_WA_RESET,       "SAHPI_WA_RESET"},
    {SAHPI_WA_POWER_DOWN,  "SAHPI_WA_POWER_DOWN"},
    {SAHPI_WA_POWER_CYCLE, "SAHPI_WA_POWER_CYCLE"},
    {SAHPI_WA_POWER_CYCLE, NULL}
};


/**
 * Translates a string to a valid SaHpiWatchdogActionT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiWatchdogActionT oSaHpiTypesEnums::str2watchdogaction(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_WA_NO_ACTION;
    }
	for (i = 0; watchdogaction_strings[i].str != NULL; i++) {
		if (strcmp(strtype, watchdogaction_strings[i].str) == 0) {
            return watchdogaction_strings[i].type;
		}
	}
    return SAHPI_WA_NO_ACTION;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiWatchdogActionT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::watchdogaction2str(SaHpiWatchdogActionT value) {
	int i;

	for (i = 0; watchdogaction_strings[i].str != NULL; i++) {
		if (value == watchdogaction_strings[i].type) {
			return watchdogaction_strings[i].str;
		}
	}
    return "Unknown";
}


static struct watchdogactionevent_map {
    SaHpiWatchdogActionEventT type;
    const char                *str;
} watchdogactionevent_strings[] = {
    {SAHPI_WAE_NO_ACTION,   "SAHPI_WAE_NO_ACTION"},
    {SAHPI_WAE_RESET,       "SAHPI_WAE_RESET"},
    {SAHPI_WAE_POWER_DOWN,  "SAHPI_WAE_POWER_DOWN"},
    {SAHPI_WAE_POWER_CYCLE, "SAHPI_WAE_POWER_CYCLE"},
    {SAHPI_WAE_TIMER_INT,   "SAHPI_WAE_TIMER_INT"},
    {SAHPI_WAE_TIMER_INT,   NULL}
};


/**
 * Translates a string to a valid SaHpiWatchdogActionEventT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiWatchdogActionEventT oSaHpiTypesEnums::str2watchdogactionevent(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_WAE_NO_ACTION;
    }
	for (i = 0; watchdogactionevent_strings[i].str != NULL; i++) {
		if (strcmp(strtype, watchdogactionevent_strings[i].str) == 0) {
            return watchdogactionevent_strings[i].type;
		}
	}
    return SAHPI_WAE_NO_ACTION;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiWatchdogActionEventT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::watchdogactionevent2str(SaHpiWatchdogActionEventT value) {
	int i;

	for (i = 0; watchdogactionevent_strings[i].str != NULL; i++) {
		if (value == watchdogactionevent_strings[i].type) {
			return watchdogactionevent_strings[i].str;
		}
	}
    return "Unknown";
}


static struct watchdogpretimerinterrupt_map {
    SaHpiWatchdogPretimerInterruptT type;
    const char                      *str;
} watchdogpretimerinterrupt_strings[] = {
    {SAHPI_WPI_NONE,              "SAHPI_WPI_NONE"},
    {SAHPI_WPI_SMI,               "SAHPI_WPI_SMI"},
    {SAHPI_WPI_NMI,               "SAHPI_WPI_NMI"},
    {SAHPI_WPI_MESSAGE_INTERRUPT, "SAHPI_WPI_MESSAGE_INTERRUPT"},
    {SAHPI_WPI_OEM,               "SAHPI_WPI_OEM"},
    {SAHPI_WPI_OEM,               NULL}
};


/**
 * Translates a string to a valid SaHpiWatchdogPretimerInterruptT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiWatchdogPretimerInterruptT oSaHpiTypesEnums::str2watchdogpretimerinterrupt(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_WPI_NONE;
    }
	for (i = 0; watchdogpretimerinterrupt_strings[i].str != NULL; i++) {
		if (strcmp(strtype, watchdogpretimerinterrupt_strings[i].str) == 0) {
            return watchdogpretimerinterrupt_strings[i].type;
		}
	}
    return SAHPI_WPI_NONE;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiWatchdogPretimerInterruptT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::watchdogpretimerinterrupt2str(SaHpiWatchdogPretimerInterruptT value) {
	int i;

	for (i = 0; watchdogpretimerinterrupt_strings[i].str != NULL; i++) {
		if (value == watchdogpretimerinterrupt_strings[i].type) {
			return watchdogpretimerinterrupt_strings[i].str;
		}
	}
    return "Unknown";
}


static struct watchdogtimeruse_map {
    SaHpiWatchdogTimerUseT type;
    const char             *str;
} watchdogtimeruse_strings[] = {
    {SAHPI_WTU_NONE,        "SAHPI_WTU_NONE"},
    {SAHPI_WTU_BIOS_FRB2,   "SAHPI_WTU_BIOS_FRB2"},
    {SAHPI_WTU_BIOS_POST,   "SAHPI_WTU_BIOS_POST"},
    {SAHPI_WTU_OS_LOAD,     "SAHPI_WTU_OS_LOAD"},
    {SAHPI_WTU_SMS_OS,      "SAHPI_WTU_SMS_OS"},
    {SAHPI_WTU_OEM,         "SAHPI_WTU_OEM"},
    {SAHPI_WTU_UNSPECIFIED, "SAHPI_WTU_UNSPECIFIED"},
    {SAHPI_WTU_UNSPECIFIED, NULL}
};


/**
 * Translates a string to a valid SaHpiWatchdogTimerUseT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiWatchdogTimerUseT oSaHpiTypesEnums::str2watchdogtimeruse(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_WTU_NONE;
    }
	for (i = 0; watchdogtimeruse_strings[i].str != NULL; i++) {
		if (strcmp(strtype, watchdogtimeruse_strings[i].str) == 0) {
            return watchdogtimeruse_strings[i].type;
		}
	}
    return SAHPI_WTU_NONE;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiWatchdogTimerUseT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::watchdogtimeruse2str(SaHpiWatchdogTimerUseT value) {
	int i;

	for (i = 0; watchdogtimeruse_strings[i].str != NULL; i++) {
		if (value == watchdogtimeruse_strings[i].type) {
			return watchdogtimeruse_strings[i].str;
		}
	}
    return "Unknown";
}


static struct watchdogexpflags_map {
    SaHpiWatchdogExpFlagsT type;
    const char             *str;
} watchdogexpflags_strings[] = {
    {SAHPI_WATCHDOG_EXP_BIOS_FRB2, "SAHPI_WATCHDOG_EXP_BIOS_FRB2"},
    {SAHPI_WATCHDOG_EXP_BIOS_POST, "SAHPI_WATCHDOG_EXP_BIOS_POST"},
    {SAHPI_WATCHDOG_EXP_OS_LOAD,   "SAHPI_WATCHDOG_EXP_OS_LOAD"},
    {SAHPI_WATCHDOG_EXP_SMS_OS,    "SAHPI_WATCHDOG_EXP_SMS_OS"},
    {SAHPI_WATCHDOG_EXP_OEM,       "SAHPI_WATCHDOG_EXP_OEM"},
    {SAHPI_WATCHDOG_EXP_OEM,       NULL}
};


/**
 * Translates a string to a valid SaHpiWatchdogExpFlagsT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiWatchdogExpFlagsT oSaHpiTypesEnums::str2watchdogexpflags(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_WTU_NONE;
    }
	for (i = 0; watchdogexpflags_strings[i].str != NULL; i++) {
		if (strcmp(strtype, watchdogexpflags_strings[i].str) == 0) {
            return watchdogexpflags_strings[i].type;
		}
	}
    return SAHPI_WTU_NONE;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiWatchdogExpFlagsT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::watchdogexpflags2str(SaHpiWatchdogExpFlagsT value) {
	int i;

	for (i = 0; watchdogexpflags_strings[i].str != NULL; i++) {
		if (value == watchdogexpflags_strings[i].type) {
			return watchdogexpflags_strings[i].str;
		}
	}
    return "Unknown";
}


static struct statuscondtype_map {
    SaHpiStatusCondTypeT type;
    const char           *str;
} statuscondtype_strings[] = {
    {SAHPI_STATUS_COND_TYPE_SENSOR,   "SAHPI_STATUS_COND_TYPE_SENSOR"},
    {SAHPI_STATUS_COND_TYPE_RESOURCE, "SAHPI_STATUS_COND_TYPE_RESOURCE"},
    {SAHPI_STATUS_COND_TYPE_OEM,      "SAHPI_STATUS_COND_TYPE_OEM"},
    {SAHPI_STATUS_COND_TYPE_USER,     "SAHPI_STATUS_COND_TYPE_USER"},
    {SAHPI_STATUS_COND_TYPE_USER,     NULL}
};


/**
 * Translates a string to a valid SaHpiStatusCondTypeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiStatusCondTypeT oSaHpiTypesEnums::str2statuscondtype(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_STATUS_COND_TYPE_SENSOR;
    }
	for (i = 0; statuscondtype_strings[i].str != NULL; i++) {
		if (strcmp(strtype, statuscondtype_strings[i].str) == 0) {
            return statuscondtype_strings[i].type;
		}
	}
    return SAHPI_STATUS_COND_TYPE_SENSOR;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiStatusCondTypeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::statuscondtype2str(SaHpiStatusCondTypeT value) {
	int i;

	for (i = 0; statuscondtype_strings[i].str != NULL; i++) {
		if (value == statuscondtype_strings[i].type) {
			return statuscondtype_strings[i].str;
		}
	}
    return "Unknown";
}


static struct annunciatormode_map {
    SaHpiAnnunciatorModeT type;
    const char           *str;
} annunciatormode_strings[] = {
    {SAHPI_ANNUNCIATOR_MODE_AUTO,   "SAHPI_ANNUNCIATOR_MODE_AUTO"},
    {SAHPI_ANNUNCIATOR_MODE_USER,   "SAHPI_ANNUNCIATOR_MODE_USER"},
    {SAHPI_ANNUNCIATOR_MODE_SHARED, "SAHPI_ANNUNCIATOR_MODE_SHARED"},
    {SAHPI_ANNUNCIATOR_MODE_SHARED, NULL}
};


/**
 * Translates a string to a valid SaHpiAnnunciatorModeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiAnnunciatorModeT oSaHpiTypesEnums::str2annunciatormode(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_ANNUNCIATOR_MODE_AUTO;
    }
	for (i = 0; annunciatormode_strings[i].str != NULL; i++) {
		if (strcmp(strtype, annunciatormode_strings[i].str) == 0) {
            return annunciatormode_strings[i].type;
		}
	}
    return SAHPI_ANNUNCIATOR_MODE_AUTO;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiAnnunciatorModeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::annunciatormode2str(SaHpiAnnunciatorModeT value) {
	int i;

	for (i = 0; annunciatormode_strings[i].str != NULL; i++) {
		if (value == annunciatormode_strings[i].type) {
			return annunciatormode_strings[i].str;
		}
	}
    return "Unknown";
}


static struct severity_map {
    SaHpiSeverityT type;
    const char     *str;
} severity_strings[] = {
    {SAHPI_CRITICAL,       "SAHPI_CRITICAL"},
    {SAHPI_MAJOR,          "SAHPI_MAJOR"},
    {SAHPI_MINOR,          "SAHPI_MINOR"},
    {SAHPI_INFORMATIONAL,  "SAHPI_INFORMATIONAL"},
    {SAHPI_OK,             "SAHPI_OK"},
    {SAHPI_DEBUG,          "SAHPI_DEBUG"},
    {SAHPI_ALL_SEVERITIES, "SAHPI_ALL_SEVERITIES"},
    {SAHPI_ALL_SEVERITIES, NULL}
};


/**
 * Translates a string to a valid SaHpiSeverityT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiSeverityT oSaHpiTypesEnums::str2severity(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_OK;
    }
	for (i = 0; severity_strings[i].str != NULL; i++) {
		if (strcmp(strtype, severity_strings[i].str) == 0) {
            return severity_strings[i].type;
		}
	}
    return SAHPI_OK;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiSeverityT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::severity2str(SaHpiSeverityT value) {
	int i;

	for (i = 0; severity_strings[i].str != NULL; i++) {
		if (value == severity_strings[i].type) {
			return severity_strings[i].str;
		}
	}
    return "Unknown";
}


static struct annunciatortype_map {
    SaHpiAnnunciatorTypeT type;
    const char     *str;
} annunciatortype_strings[] = {
    {SAHPI_ANNUNCIATOR_TYPE_LED,                 "SAHPI_ANNUNCIATOR_TYPE_LED"},
    {SAHPI_ANNUNCIATOR_TYPE_DRY_CONTACT_CLOSURE, "SAHPI_ANNUNCIATOR_TYPE_DRY_CONTACT_CLOSURE"},
    {SAHPI_ANNUNCIATOR_TYPE_AUDIBLE,             "SAHPI_ANNUNCIATOR_TYPE_AUDIBLE"},
    {SAHPI_ANNUNCIATOR_TYPE_LCD_DISPLAY,         "SAHPI_ANNUNCIATOR_TYPE_LCD_DISPLAY"},
    {SAHPI_ANNUNCIATOR_TYPE_MESSAGE,             "SAHPI_ANNUNCIATOR_TYPE_MESSAGE"},
    {SAHPI_ANNUNCIATOR_TYPE_COMPOSITE,           "SAHPI_ANNUNCIATOR_TYPE_COMPOSITE"},
    {SAHPI_ANNUNCIATOR_TYPE_OEM,                 "SAHPI_ANNUNCIATOR_TYPE_OEM"},
    {SAHPI_ANNUNCIATOR_TYPE_OEM,                 NULL}
};


/**
 * Translates a string to a valid SaHpiAnnunciatorTypeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiAnnunciatorTypeT oSaHpiTypesEnums::str2annunciatortype(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_ANNUNCIATOR_TYPE_LED;
    }
	for (i = 0; annunciatortype_strings[i].str != NULL; i++) {
		if (strcmp(strtype, annunciatortype_strings[i].str) == 0) {
            return annunciatortype_strings[i].type;
		}
	}
    return SAHPI_ANNUNCIATOR_TYPE_LED;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiAnnunciatorTypeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::annunciatortype2str(SaHpiAnnunciatorTypeT value) {
	int i;

	for (i = 0; annunciatortype_strings[i].str != NULL; i++) {
		if (value == annunciatortype_strings[i].type) {
			return annunciatortype_strings[i].str;
		}
	}
    return "Unknown";
}


static struct rdrtype_map {
    SaHpiRdrTypeT type;
    const char     *str;
} rdrtype_strings[] = {
    {SAHPI_NO_RECORD,       "SAHPI_NO_RECORD"},
    {SAHPI_CTRL_RDR,        "SAHPI_CTRL_RDR"},
    {SAHPI_SENSOR_RDR,      "SAHPI_SENSOR_RDR"},
    {SAHPI_INVENTORY_RDR,   "SAHPI_INVENTORY_RDR"},
    {SAHPI_WATCHDOG_RDR,    "SAHPI_WATCHDOG_RDR"},
    {SAHPI_ANNUNCIATOR_RDR, "SAHPI_ANNUNCIATOR_RDR"},
    {SAHPI_ANNUNCIATOR_RDR, NULL}
};


/**
 * Translates a string to a valid SaHpiRdrTypeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiRdrTypeT oSaHpiTypesEnums::str2rdrtype(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_NO_RECORD;
    }
	for (i = 0; rdrtype_strings[i].str != NULL; i++) {
		if (strcmp(strtype, rdrtype_strings[i].str) == 0) {
            return rdrtype_strings[i].type;
		}
	}
    return SAHPI_NO_RECORD;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiRdrTypeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::rdrtype2str(SaHpiRdrTypeT value) {
	int i;

	for (i = 0; rdrtype_strings[i].str != NULL; i++) {
		if (value == rdrtype_strings[i].type) {
			return rdrtype_strings[i].str;
		}
	}
    return "Unknown";
}


static struct hsindicatorstate_map {
    SaHpiHsIndicatorStateT type;
    const char             *str;
} hsindicatorstate_strings[] = {
    {SAHPI_HS_INDICATOR_OFF, "SAHPI_HS_INDICATOR_OFF"},
    {SAHPI_HS_INDICATOR_ON,  "SAHPI_HS_INDICATOR_ON"},
    {SAHPI_HS_INDICATOR_ON,  NULL}
};


/**
 * Translates a string to a valid SaHpiHsIndicatorStateT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiHsIndicatorStateT oSaHpiTypesEnums::str2hsindicatorstate(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_HS_INDICATOR_OFF;
    }
	for (i = 0; hsindicatorstate_strings[i].str != NULL; i++) {
		if (strcmp(strtype, hsindicatorstate_strings[i].str) == 0) {
            return hsindicatorstate_strings[i].type;
		}
	}
    return SAHPI_HS_INDICATOR_OFF;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiHsIndicatorStateT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::hsindicatorstate2str(SaHpiHsIndicatorStateT value) {
	int i;

	for (i = 0; hsindicatorstate_strings[i].str != NULL; i++) {
		if (value == hsindicatorstate_strings[i].type) {
			return hsindicatorstate_strings[i].str;
		}
	}
    return "Unknown";
}


static struct hsaction_map {
    SaHpiHsActionT type;
    const char     *str;
} hsaction_strings[] = {
    {SAHPI_HS_ACTION_INSERTION,  "SAHPI_HS_ACTION_INSERTION"},
    {SAHPI_HS_ACTION_EXTRACTION, "SAHPI_HS_ACTION_EXTRACTION"},
    {SAHPI_HS_ACTION_EXTRACTION, NULL}
};


/**
 * Translates a string to a valid SaHpiHsActionT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiHsActionT oSaHpiTypesEnums::str2hsaction(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_HS_ACTION_INSERTION;
    }
	for (i = 0; hsaction_strings[i].str != NULL; i++) {
		if (strcmp(strtype, hsaction_strings[i].str) == 0) {
            return hsaction_strings[i].type;
		}
	}
    return SAHPI_HS_ACTION_INSERTION;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiHsActionT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::hsaction2str(SaHpiHsActionT value) {
	int i;

	for (i = 0; hsaction_strings[i].str != NULL; i++) {
		if (value == hsaction_strings[i].type) {
			return hsaction_strings[i].str;
		}
	}
    return "Unknown";
}


static struct hsstate_map {
    SaHpiHsStateT type;
    const char    *str;
} hsstate_strings[] = {
    {SAHPI_HS_STATE_INACTIVE,           "SAHPI_HS_STATE_INACTIVE"},
    {SAHPI_HS_STATE_INSERTION_PENDING,  "SAHPI_HS_STATE_INSERTION_PENDING"},
    {SAHPI_HS_STATE_ACTIVE,             "SAHPI_HS_STATE_ACTIVE"},
    {SAHPI_HS_STATE_EXTRACTION_PENDING, "SAHPI_HS_STATE_EXTRACTION_PENDING"},
    {SAHPI_HS_STATE_NOT_PRESENT,        "SAHPI_HS_STATE_NOT_PRESENT"},
    {SAHPI_HS_STATE_NOT_PRESENT,        NULL}
};


/**
 * Translates a string to a valid SaHpiHsStateT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiHsStateT oSaHpiTypesEnums::str2hsstate(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_HS_STATE_INACTIVE;
    }
	for (i = 0; hsstate_strings[i].str != NULL; i++) {
		if (strcmp(strtype, hsstate_strings[i].str) == 0) {
            return hsstate_strings[i].type;
		}
	}
    return SAHPI_HS_STATE_INACTIVE;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiHsStateT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::hsstate2str(SaHpiHsStateT value) {
	int i;

	for (i = 0; hsstate_strings[i].str != NULL; i++) {
		if (value == hsstate_strings[i].type) {
			return hsstate_strings[i].str;
		}
	}
    return "Unknown";
}


static struct resourceeventtype_map {
    SaHpiResourceEventTypeT type;
    const char              *str;
} resourceeventtype_strings[] = {
    {SAHPI_RESE_RESOURCE_FAILURE,  "SAHPI_RESE_RESOURCE_FAILURE"},
    {SAHPI_RESE_RESOURCE_RESTORED, "SAHPI_RESE_RESOURCE_RESTORED"},
    {SAHPI_RESE_RESOURCE_ADDED,    "SAHPI_RESE_RESOURCE_ADDED"},
    {SAHPI_RESE_RESOURCE_ADDED,    NULL}
};


/**
 * Translates a string to a valid SaHpiResourceEventTypeT type.
 *
 * @param strtype The entity type expressed as a string.
 *
 * @return SAHPI_OK on success, otherwise an HPI error code.
 */
SaHpiResourceEventTypeT oSaHpiTypesEnums::str2resourceeventtype(const char *strtype) {
	int i;

    if (strtype == NULL) {
        return SAHPI_RESE_RESOURCE_FAILURE;
    }
	for (i = 0; resourceeventtype_strings[i].str != NULL; i++) {
		if (strcmp(strtype, resourceeventtype_strings[i].str) == 0) {
            return resourceeventtype_strings[i].type;
		}
	}
    return SAHPI_RESE_RESOURCE_FAILURE;
}


/**
 * Translates an sensor aggregate status type to a string.
 *
 * @param value  The SaHpiResourceEventTypeT to be converted.
 *
 * @return The string value of the type.
 */
const char * oSaHpiTypesEnums::resourceeventtype2str(SaHpiResourceEventTypeT value) {
	int i;

	for (i = 0; resourceeventtype_strings[i].str != NULL; i++) {
		if (value == resourceeventtype_strings[i].type) {
			return resourceeventtype_strings[i].str;
		}
	}
    return "Unknown";
}


