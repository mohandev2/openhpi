/* -*- linux-c -*-
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
 *     Steve Sherman <stevees@us.ibm.com>
 */

/**********************************************************************
 * Notes:
 *
 * All these test cases depend on values defined in errlog2event_hash and
 * sensor and resource definitions in snmp_bc_resources.c. These are real 
 * hardware events and sensors, which hopefully won't change much.
 **********************************************************************/

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

#include <snmp_bc_plugin.h>
#include <sim_resources.h>

#define SNMP_BC_ERROR_LOG_MSG_OID     ".1.3.6.1.4.1.2.3.51.2.3.4.2.1.2.1"
#define SNMP_BC_ERROR_LOG_MSG_OID_RSA ".1.3.6.1.4.1.2.3.51.1.3.4.2.1.2.1"
#define SNMP_BC_CHASSIS_VOLT_3_3_OID  ".1.3.6.1.4.1.2.3.51.2.2.2.1.2.0"
#define SNMP_BC_CHASSIS_TEMP_OID      ".1.3.6.1.4.1.2.3.51.2.2.1.5.1.0"

int main(int argc, char **argv)
{
        SaErrorT err;
	SaHpiRdrT rdr;
	SaHpiEntryIdT rptid, next_rptid;
	SaHpiRptEntryT rpt;
	SaHpiResourceIdT chassis_rid=0;
	SaHpiEventLogEntryT logentry;
	SaHpiEventLogEntryIdT prev_logid, next_logid;
        SaHpiSessionIdT sessionid;
	char *hash_key, *logstr;
	SnmpMibInfoT *hash_value;
	SnmpMibInfoT *hash_set_value;

        err = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID, &sessionid, NULL);
        if (err) {
	  printf("  Error! Testcase failed. Line=%d\n", __LINE__);
	  printf("  Received error=%s\n", oh_lookup_error(err));
	  return -1;
        }
 
        err = saHpiDiscover(sessionid);
        if (err) {
	  printf("  Error! Testcase failed. Line=%d\n", __LINE__);
	  printf("  Received error=%s\n", oh_lookup_error(err));
	  return -1;
        }

	/* Find first Event Log capable resource - assume its the chassis */
	rptid = SAHPI_FIRST_ENTRY;
	while ((err == SA_OK) && (rptid != SAHPI_LAST_ENTRY))
	{
		err = saHpiRptEntryGet(sessionid, rptid, &next_rptid, &rpt);
		if (err) {
			printf("  Error! Testcase failed. Line=%d\n", __LINE__);
			printf("  Received error=%s\n", oh_lookup_error(err));
			return -1;
		}
		
		if ((rpt.ResourceCapabilities & SAHPI_CAPABILITY_EVENT_LOG)) {
			chassis_rid = rpt.ResourceId;
			break;
		}
		else {
			rptid = next_rptid;
			continue;
		}
	}
	if (chassis_rid == 0) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Cannot find Chassis RID\n");
		return -1;
	}

	/* Determine platform */
	SnmpMibInfoT * hash_data = (SnmpMibInfoT *)g_hash_table_lookup(sim_hash, SNMP_BC_PLATFORM_OID_RSA);
	if (hash_data->value.integer == 255) {
		printf("Executing RSA event tests\n");
		goto RSA_TESTS;
	}
	else {
		printf("Executing BladeCenter event tests\n");
	}

	/* If test OID not already in sim hash table; create it */
	if (!g_hash_table_lookup_extended(sim_hash, 
					  SNMP_BC_ERROR_LOG_MSG_OID,
					  (gpointer)&hash_key, 
					  (gpointer)&hash_value)) {

		hash_key = g_strdup(SNMP_BC_ERROR_LOG_MSG_OID);
		if (!hash_key) {
			printf("  Error! Testcase failed. Line=%d\n", __LINE__);
			printf("  Cannot allocate memory for OID key=%s.\n", SNMP_BC_ERROR_LOG_MSG_OID);
			return -1;
		}
		
		hash_value = g_malloc0(sizeof(SnmpMibInfoT));
		if (!hash_value) {
			printf("  Error! Testcase failed. Line=%d\n", __LINE__);
			printf("  Cannot allocate memory for hash value for OID=%s.\n", SNMP_BC_ERROR_LOG_MSG_OID);
			return -1;
		}
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("Error! saHpiEventLogClear: line=%d; err=%d\n", __LINE__, err);
		return -1;
        }

	/************************************************************
	 * TestCase - Mapped Chassis Event (EN_CUTOFF_HI_FAULT_3_35V)
         * Event recovered in next testcase
	 ************************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:System shutoff due to +3.3v over voltage. Read value 3.5. Threshold value 3.4";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((logentry.Event.Source == chassis_rid) &&
	      (logentry.Event.EventType == SAHPI_ET_SENSOR) &&
	      (logentry.Event.Severity == SAHPI_CRITICAL) &&
	      (logentry.Event.EventDataUnion.SensorEvent.SensorType == SAHPI_VOLTAGE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_CRIT) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MINOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState == SAHPI_ES_UNSPECIFIED) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState == SAHPI_ES_UNSPECIFIED) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerReading.Value.SensorFloat64 == (double)3.5) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerThreshold.Value.SensorFloat64 == (double)3.4))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/**************************************************************
	 * TestCase - Chassis Recovery Event (EN_CUTOFF_HI_FAULT_3_35V)
         * Recover event in previous testcase.
	 **************************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:Recovery System shutoff due to +3.3v over voltage. Read value 3.5 Threshold value 3.4";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((logentry.Event.Source == chassis_rid) &&
	      (logentry.Event.EventType == SAHPI_ET_SENSOR) &&
	      (logentry.Event.Severity == SAHPI_CRITICAL) &&
	      (logentry.Event.EventDataUnion.SensorEvent.SensorType == SAHPI_VOLTAGE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.Assertion == SAHPI_FALSE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_CRIT) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MINOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState == SAHPI_ES_UNSPECIFIED) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState == SAHPI_ES_UNSPECIFIED) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerReading.Value.SensorFloat64 == (double)3.5) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerThreshold.Value.SensorFloat64 == (double)3.4))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/******************************************************
	 * TestCase - Chassis Event (EN_CUTOFF_HI_FAULT_3_35V)
         * Change sensor reading to LOWER MAJOR. Previous state 
         * depends upon previous testcase.
	 ******************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:System shutoff due to +3.3v over voltage. Read value 3.5 Threshold value 3.4";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

	/* Change sensor's simulator value */
	hash_set_value = (SnmpMibInfoT *)g_hash_table_lookup(sim_hash, SNMP_BC_CHASSIS_VOLT_3_3_OID);
	if (hash_set_value == NULL) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received Null hash value\n");
		return -1;
	}
	strcpy(hash_set_value->value.string, "2.9 Volts");

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((logentry.Event.Source == chassis_rid) &&
	      (logentry.Event.EventType == SAHPI_ET_SENSOR) &&
	      (logentry.Event.Severity == SAHPI_CRITICAL) &&
	      (logentry.Event.EventDataUnion.SensorEvent.SensorType == SAHPI_VOLTAGE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_CRIT) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MINOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState == SAHPI_ES_UNSPECIFIED) &&
	      (!(logentry.Event.EventDataUnion.SensorEvent.CurrentState & SAHPI_ES_LOWER_CRIT)) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState & SAHPI_ES_LOWER_MINOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState & SAHPI_ES_LOWER_MAJOR) &&      
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerReading.Value.SensorFloat64 == (double)3.5) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerThreshold.Value.SensorFloat64 == (double)3.4))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/* Set sensor's simulator value back to default */ 
	hash_set_value = (SnmpMibInfoT *)g_hash_table_lookup(sim_hash, SNMP_BC_CHASSIS_VOLT_3_3_OID);
	if (hash_set_value == NULL) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received Null hash value\n");
		return -1;
	}
	strcpy(hash_set_value->value.string, "3.3 Volts");

	/******************************************************
	 * TestCase - Chassis Event (EN_CUTOFF_HI_FAULT_3_35V)
         * Change sensor reading to UPPER MAJOR. Previous state  
         * depends upon previous testcase.
	 ******************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:System shutoff due to +3.3v over voltage. Read value 3.5 Threshold value 3.4";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

	/* Change sensor's simulator value */ 
	hash_set_value = (SnmpMibInfoT *)g_hash_table_lookup(sim_hash, SNMP_BC_CHASSIS_VOLT_3_3_OID);
	if (hash_set_value == NULL) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received Null hash value\n");
		return -1;
	}
	strcpy(hash_set_value->value.string, "3.5 Volts");

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((logentry.Event.Source == chassis_rid) &&
	      (logentry.Event.EventType == SAHPI_ET_SENSOR) &&
	      (logentry.Event.Severity == SAHPI_CRITICAL) &&
	      (logentry.Event.EventDataUnion.SensorEvent.SensorType == SAHPI_VOLTAGE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_CRIT) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MINOR) &&
	      (!(logentry.Event.EventDataUnion.SensorEvent.PreviousState & SAHPI_ES_LOWER_CRIT)) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState & SAHPI_ES_LOWER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState & SAHPI_ES_LOWER_MINOR) &&
	      (!(logentry.Event.EventDataUnion.SensorEvent.CurrentState & SAHPI_ES_UPPER_CRIT)) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState & SAHPI_ES_UPPER_MINOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerReading.Value.SensorFloat64 == (double)3.5) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerThreshold.Value.SensorFloat64 == (double)3.4))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/* Set sensor's simulator value back to default */ 
	hash_set_value = (SnmpMibInfoT *)g_hash_table_lookup(sim_hash, SNMP_BC_CHASSIS_VOLT_3_3_OID);
	if (hash_set_value == NULL) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received Null hash value\n");
		return -1;
	}
	strcpy(hash_set_value->value.string, "3.3 Volts");

	/************************************************************
	 * TestCase - Chassis Duplicate Event (EN_PFA_HI_FAULT_3_35V)
         * Previous state check depends on previous testcase!
	 ************************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:System over recommended voltage on +3.3v. Read value 3.5 Threshold value 3.4";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((logentry.Event.Source == chassis_rid) &&
	      (logentry.Event.EventType == SAHPI_ET_SENSOR) &&
	      (logentry.Event.Severity == SAHPI_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.SensorType == SAHPI_VOLTAGE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE) &&
	      (!(logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_CRIT)) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MINOR) &&
	      (!(logentry.Event.EventDataUnion.SensorEvent.PreviousState & SAHPI_ES_UPPER_CRIT)) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState & SAHPI_ES_UPPER_MINOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState == SAHPI_ES_UNSPECIFIED) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerReading.Value.SensorFloat64 == (double)3.5) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerThreshold.Value.SensorFloat64 == (double)3.4))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/**********************************************************
	 * TestCase - Blade Duplicate Event (EN_PFA_HI_FAULT_3_35V)
         * Same as previous testcase only for the blade not chassis
	 **********************************************************/
	logstr = "Severity:INFO  Source:BLADE_11  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:System over recommended voltage on +3.3v. Read value 3.5 Threshold value 3.4";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((!(logentry.Event.Source == chassis_rid)) &&
	      (logentry.Event.EventType == SAHPI_ET_SENSOR) &&
	      (logentry.Event.Severity == SAHPI_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.SensorType == SAHPI_VOLTAGE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE) &&
	      (!(logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_CRIT)) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MINOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState == SAHPI_ES_UNSPECIFIED) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState == SAHPI_ES_UNSPECIFIED) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerReading.Value.SensorFloat64 == (double)3.5) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerThreshold.Value.SensorFloat64 == (double)3.4))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/**************************************************************
	 * TestCase - Chassis Temperature (EN_PFA_HI_OVER_TEMP_AMBIENT)
         * Set at nominal temperature value.
	 **************************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:System over recommended ambient temperature.";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

	/* Change sensor's simulator value */ 
	hash_set_value = (SnmpMibInfoT *)g_hash_table_lookup(sim_hash, SNMP_BC_CHASSIS_TEMP_OID);
	if (hash_set_value == NULL) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received Null hash value\n");
		return -1;
	}
	strcpy(hash_set_value->value.string, "60 Centigrade");

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((logentry.Event.Source == chassis_rid) &&
	      (logentry.Event.EventType == SAHPI_ET_SENSOR) &&
	      (logentry.Event.Severity == SAHPI_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.SensorType == SAHPI_TEMPERATURE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE) &&
	      (!(logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_CRIT)) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MINOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState == SAHPI_ES_UNSPECIFIED) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState == SAHPI_ES_UNSPECIFIED))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/**************************************************************
	 * TestCase - Chassis Temperature (EN_PFA_HI_OVER_TEMP_AMBIENT)
         * Set above warning level.
	 **************************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:System over recommended ambient temperature.";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

	/* Change sensor's simulator value */ 
	hash_set_value = (SnmpMibInfoT *)g_hash_table_lookup(sim_hash, SNMP_BC_CHASSIS_TEMP_OID);
	if (hash_set_value == NULL) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received Null hash value\n");
		return -1;
	}
	strcpy(hash_set_value->value.string, "61 Centigrade");

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((logentry.Event.Source == chassis_rid) &&
	      (logentry.Event.EventType == SAHPI_ET_SENSOR) &&
	      (logentry.Event.Severity == SAHPI_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.SensorType == SAHPI_TEMPERATURE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE) &&
	      (!(logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_CRIT)) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MINOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState == SAHPI_ES_UNSPECIFIED) &&
	      (!(logentry.Event.EventDataUnion.SensorEvent.CurrentState & SAHPI_ES_UPPER_CRIT)) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState & SAHPI_ES_UPPER_MINOR))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/*************************************************************
	 * TestCase - Non-mapped Event (Severity=INFO)
	 *************************************************************/
	logstr = "Severity:INFO  Source:BLADE_01  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:Bogus message not in string to event table";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((!(logentry.Event.Source == chassis_rid)) &&
	      (logentry.Event.EventType == SAHPI_ET_OEM) &&
	      (logentry.Event.Severity == SAHPI_INFORMATIONAL))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/*************************************************************
	 * TestCase - Expansion Card Event (EN_PFA_HI_OVER_TEMP_DASD1)
	 *************************************************************/
	logstr = "Severity:INFO  Source:BLADE_07  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:BEM Option over recommended temperature. Read value 87 Threshold value 75";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((!(logentry.Event.Source == chassis_rid)) &&
	      (logentry.Event.EventType == SAHPI_ET_SENSOR) &&
	      (logentry.Event.Severity == SAHPI_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.SensorType == SAHPI_TEMPERATURE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE) &&
	      (!(logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_CRIT)) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MINOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState == SAHPI_ES_UNSPECIFIED) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState == SAHPI_ES_UNSPECIFIED) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerReading.Value.SensorFloat64 == (double)87) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerThreshold.Value.SensorFloat64 == (double)75))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/* Better to test a mapped login event - BC doesn't seem to have one defined yet */
	/*************************************************************
	 * TestCase - Non-mapped Login Event (Severity=WARN)
	 *************************************************************/
	logstr = "Severity:WARN  Source:SWITCH_4  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:Bogus login message Login ID:\'\'myid\' @ someaddress\'";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((!(logentry.Event.Source == chassis_rid)) &&
	      (logentry.Event.EventType == SAHPI_ET_OEM) &&
	      (logentry.Event.Severity == SAHPI_MINOR))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/***********************************************************
	 * TestCase - Power over temperature (EN_FAULT_PSx_OVR_TEMP)
         * This is a non-readable sensor. Recover in next testcase.
	 ***********************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:Power Supply 1 Temperature Fault";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((!(logentry.Event.Source == chassis_rid)) &&
	      (logentry.Event.EventType == SAHPI_ET_SENSOR) &&
	      (logentry.Event.Severity == SAHPI_CRITICAL) &&
	      (logentry.Event.EventDataUnion.SensorEvent.SensorType == SAHPI_TEMPERATURE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_CRITICAL) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState == SAHPI_ES_OK) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState == SAHPI_ES_CRITICAL))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/***********************************************************
	 * TestCase - Power over temperature (EN_FAULT_PSx_OVR_TEMP)
         * This is a non-readable sensor. Recover previous testcase.
	 ***********************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:Recovery Power Supply 1 Temperature Fault";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((!(logentry.Event.Source == chassis_rid)) &&
	      (logentry.Event.EventType == SAHPI_ET_SENSOR) &&
	      (logentry.Event.Severity == SAHPI_CRITICAL) &&
	      (logentry.Event.EventDataUnion.SensorEvent.SensorType == SAHPI_TEMPERATURE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.Assertion == SAHPI_FALSE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_CRITICAL) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState == SAHPI_ES_CRITICAL) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState == SAHPI_ES_MAJOR_FROM_LESS))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/*************************************************************
	 * TestCase - Hotswap switch installed (EN_SWITCH_3_INSTALLED) 
	 *************************************************************/
	logstr = "Severity:INFO  Source:SWITCH_3  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:I/O module 3 was installed.";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((!(logentry.Event.Source == chassis_rid)) &&
	      (logentry.Event.EventType == SAHPI_ET_HOTSWAP) &&
	      (logentry.Event.Severity == SAHPI_INFORMATIONAL) &&
	      (logentry.Event.EventDataUnion.HotSwapEvent.HotSwapState == SAHPI_HS_STATE_ACTIVE) && 
	      (logentry.Event.EventDataUnion.HotSwapEvent.PreviousHotSwapState == SAHPI_HS_STATE_ACTIVE))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/****************************************************************
	 * TestCase - hotswap Media Tray removal (EN_MEDIA_TRAY_REMOVED)
         * This event is recovered in the next testcase. Should be
         * MAJOR severity, since this is an unexpected failure.
	 ****************************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:The media tray was removed.";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((!(logentry.Event.Source == chassis_rid)) &&
	      (logentry.Event.EventType == SAHPI_ET_HOTSWAP) &&
	      (logentry.Event.Severity == SAHPI_MAJOR) &&
	      (logentry.Event.EventDataUnion.HotSwapEvent.HotSwapState == SAHPI_HS_STATE_NOT_PRESENT) && 
	      (logentry.Event.EventDataUnion.HotSwapEvent.PreviousHotSwapState == SAHPI_HS_STATE_ACTIVE))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/****************************************************************
	 * TestCase - hotswap Media Tray recovery (EN_MEDIA_TRAY_REMOVED)
         * Recovery of previous event.
	 ****************************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:Recovery The media tray was removed.";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((!(logentry.Event.Source == chassis_rid)) &&
	      (logentry.Event.EventType == SAHPI_ET_HOTSWAP) &&
	      (logentry.Event.Severity == SAHPI_INFORMATIONAL) &&
	      (logentry.Event.EventDataUnion.HotSwapEvent.HotSwapState == SAHPI_HS_STATE_ACTIVE) &&
	      (logentry.Event.EventDataUnion.HotSwapEvent.PreviousHotSwapState == SAHPI_HS_STATE_NOT_PRESENT))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	goto COMMON_TESTS;

 RSA_TESTS:

	/* If test OID not already in sim hash table; create it */
	if (!g_hash_table_lookup_extended(sim_hash, 
					  SNMP_BC_ERROR_LOG_MSG_OID_RSA,
					  (gpointer)&hash_key, 
					  (gpointer)&hash_value)) {

		hash_key = g_strdup(SNMP_BC_ERROR_LOG_MSG_OID_RSA);
		if (!hash_key) {
			printf("  Error! Testcase failed. Line=%d\n", __LINE__);
			printf("  Cannot allocate memory for OID key=%s.\n", SNMP_BC_ERROR_LOG_MSG_OID_RSA);
			return -1;
		}
		
		hash_value = g_malloc0(sizeof(SnmpMibInfoT));
		if (!hash_value) {
			printf("  Error! Testcase failed. Line=%d\n", __LINE__);
			printf("  Cannot allocate memory for hash value for OID=%s.\n", SNMP_BC_ERROR_LOG_MSG_OID_RSA);
			return -1;
		}
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("Error! saHpiEventLogClear: line=%d; err=%d\n", __LINE__, err);
		return -1;
        }

	/************************************************************
	 * TestCase - Mapped Chassis Event (EN_CUTOFF_HI_FAULT_3_35V)
	 ************************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN08032480  Date:10/11/03  Time:09:09:46  Text:System shutoff due to +3.3v over voltage.";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!((logentry.Event.Source == chassis_rid) &&
	      (logentry.Event.EventType == SAHPI_ET_SENSOR) &&
	      (logentry.Event.Severity == SAHPI_CRITICAL) &&
	      (logentry.Event.EventDataUnion.SensorEvent.SensorType == SAHPI_VOLTAGE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.Assertion == SAHPI_TRUE) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_CRIT) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MAJOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.EventState & SAHPI_ES_UPPER_MINOR) &&
	      (logentry.Event.EventDataUnion.SensorEvent.PreviousState == SAHPI_ES_UNSPECIFIED) &&
	      (logentry.Event.EventDataUnion.SensorEvent.CurrentState == SAHPI_ES_UNSPECIFIED) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerReading.Value.SensorFloat64 == (double)0) &&
	      (logentry.Event.EventDataUnion.SensorEvent.TriggerThreshold.Value.SensorFloat64 == (double)0) )) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

 COMMON_TESTS:
	/************************************
	 * Drive some error paths in the code
         ************************************/ 

	/******************************************************************
	 * TestCase - Bogus threshold strings
 	 ******************************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:System shutoff due to +3.3v over voltage. Bogus Read value 3.5 Bogus Threshold value 3.4";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }
	
	/* Check expected values */
	if (!(((logentry.Event.Source == chassis_rid)) &&
	      (logentry.Event.EventType == SAHPI_ET_OEM) &&
	      (logentry.Event.Severity == SAHPI_INFORMATIONAL))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/******************************************************************
	 * TestCase - Recovery string not first character of text string
         * (blank is first character). Should not treat as a recovery event
	 ******************************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text: Recovery System shutoff due to +3.3v over voltage. Read value 3.5 Threshold value 3.4";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/* Check expected values */
	if (!(((logentry.Event.Source == chassis_rid)) &&
	      (logentry.Event.EventType == SAHPI_ET_OEM) &&
	      (logentry.Event.Severity == SAHPI_INFORMATIONAL))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/******************************************************************
	 * TestCase - In string table but not mapped
         * Uses special defined Test event in bc_str2event.c
	 ******************************************************************/
	logstr = "Severity:INFO  Source:SERVPROC  Name:WMN315702424  Date:10/11/03  Time:09:09:46  Text:Bogus Test Event.";
	memset(&logentry, 0 , sizeof(SaHpiEventLogEntryT));
	strcpy(hash_value->value.string, logstr);
	g_hash_table_insert(sim_hash, hash_key, hash_value);

        err = saHpiEventLogEntryGet(sessionid, chassis_rid, SAHPI_NEWEST_ENTRY,
				    &prev_logid, &next_logid, &logentry, &rdr, &rpt);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/* Check expected values */
	if (!(((logentry.Event.Source == chassis_rid)) &&
	      (logentry.Event.EventType == SAHPI_ET_OEM) &&
	      (logentry.Event.Severity == SAHPI_INFORMATIONAL))) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		oh_print_event(&(logentry.Event), 1);
		return -1;
	}

	err = saHpiEventLogClear(sessionid, chassis_rid);
	if (err) {
		printf("  Error! Testcase failed. Line=%d\n", __LINE__);
		printf("  Received error=%s\n", oh_lookup_error(err));
		return -1;
        }

	/****************** 
	 * End of testcases 
         ******************/

        err = saHpiSessionClose(sessionid);
        if (err) {
	  printf("Error! saHpiSessionClose: err=%d\n", err);
	  return -1;
        }

        return 0;
}
