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
 * Authors:
 *      W. David Ashley <dashley@us.ibm.com>
 */

/*************************************************************************
 * RESTRICTIONS!!!
 *
 * - If IsThreshold=SAHPI_TRUE for an interpreted sensor, 
 *   Range.Max.Interpreted.Type must be defined for snmp_rsa.c 
 *   get_interpreted_thresholds to work.
 * - Digital controls must be integers and depend on SaHpiStateDigitalT
 *************************************************************************/

#ifndef __RSA_RESOURCES_H
#define __RSA_RESOURCES_H

/* HPI Spec dependencies */
#define ELEMENTS_IN_SaHpiStateDigitalT 5

/* Resource indexes to snmp_rpt array below */
typedef enum {
	RSA_RPT_ENTRY_CHASSIS = 0,
	RSA_RPT_ENTRY_CPU,
	RSA_RPT_ENTRY_DASD
} RSARptEntryT;

/* Start HPI Instance numbers from 1 */
#define RSA_HPI_INSTANCE_BASE 1
/* Character for blanking out normalized oid elements */
#define OID_BLANK_CHAR 'x'
#define OID_BLANK_STR "x"

/* Maximum number of resources  */
#define RSA_MAX_CPU    8
#define RSA_MAX_FAN    8
#define RSA_MAX_DASD   4

/*************************************************************************
 *                   Resource Definitions
 *************************************************************************/
struct ResourceMibInfo {
	const char* OidHealth;
	int   HealthyValue;
	const char* OidReset;
	const char* OidPowerState;
	const char* OidPowerOnOff;
};

struct snmp_rpt {
        SaHpiRptEntryT rpt;
	struct ResourceMibInfo mib;
        const  char* comment;
};

extern struct snmp_rpt snmp_rpt_array[];

/******************************************************************************
 *                      Sensor Definitions
 ******************************************************************************/
struct SNMPRawThresholdsOIDs {
	const char *OidLowMinor;
	const char *OidLowMajor;
	const char *OidLowCrit;
	const char *OidUpMinor;
	const char *OidUpMajor;
	const char *OidUpCrit;
	const char *OidLowHysteresis;
	const char *OidUpHysteresis;
};

struct SNMPInterpretedThresholdsOIDs {
	const char *OidLowMinor;
	const char *OidLowMajor;
	const char *OidLowCrit;
	const char *OidUpMinor;
	const char *OidUpMajor;
	const char *OidUpCrit;
	const char *OidLowHysteresis;
	const char *OidUpHysteresis;
};

struct SnmpSensorThresholdOids {
	struct SNMPRawThresholdsOIDs RawThresholds;
	struct SNMPInterpretedThresholdsOIDs InterpretedThresholds;
};

struct SensorMibInfo {
        unsigned int not_avail_indicator_num; /* 0 for none, n>0 otherwise */
	int write_only; /* Write-only SNMP command; 0 no; 1 yes  */
	int convert_snmpstr; /* -1 no conversion; else use SaHpiSensorInterpretedTypeT values */
	const char* oid;
        struct SnmpSensorThresholdOids threshold_oids;
};

struct snmp_rsa_sensor {
        SaHpiSensorRecT sensor;
	struct SensorMibInfo mib;
        const char* comment;
};

extern struct snmp_rsa_sensor snmp_rsa_chassis_sensors[];
extern struct snmp_rsa_sensor snmp_rsa_power_sensors[];
extern struct snmp_rsa_sensor snmp_rsa_cpu_sensors[];
extern struct snmp_rsa_sensor snmp_rsa_dasd_sensors[];
extern struct snmp_rsa_sensor snmp_rsa_fan_sensors[];

/*************************************************************************
 *                   Control Definitions
 *************************************************************************/
struct ControlMibInfo {
        unsigned int not_avail_indicator_num; /* 0 for none, n>0 otherwise */
        int write_only; /* Write-only SNMP command; 0 no; 1 yes  */
	const char* oid;
	int digitalmap[ELEMENTS_IN_SaHpiStateDigitalT];
};

struct snmp_rsa_control {
        SaHpiCtrlRecT control;
        struct ControlMibInfo mib;
        const char* comment;
};

extern struct snmp_rsa_control snmp_rsa_chassis_controls[];
extern struct snmp_rsa_control snmp_rsa_power_controls[];
extern struct snmp_rsa_control snmp_rsa_fan_controls[];
 
/*************************************************************************
 *                   Inventory Definitions
 *************************************************************************/
struct SnmpInventoryOids {
        const char *OidMfgDateTime;
        const char *OidManufacturer;
        const char *OidProductName;
        const char *OidProductVersion;
        const char *OidModelNumber;
        const char *OidSerialNumber;
        const char *OidPartNumber;
        const char *OidFileId;
        const char *OidAssetTag;
};

struct InventoryMibInfo {
        unsigned int not_avail_indicator_num; /* 0 for none, n>0 otherwise */
        int write_only; /* Write-only SNMP command; 0 no; 1 yes  */
        SaHpiInventDataRecordTypeT  inventory_type;
	SaHpiInventChassisTypeT chassis_type; /* Ignore if inventory_type not CHASSIS */
	struct SnmpInventoryOids oid;
};

struct snmp_rsa_inventory {
	SaHpiInventoryRecT  inventory;
        struct InventoryMibInfo mib;
        const char* comment;
};

extern struct snmp_rsa_inventory snmp_rsa_chassis_inventories[];
extern struct snmp_rsa_inventory snmp_rsa_fan_inventories[];
extern struct snmp_rsa_inventory snmp_rsa_power_inventories[];

#endif  /* End __RSA_RESOURCES.H */
