/*      -*- linux-c -*-
 *
 * Copyright (c) 2004 by Intel Corp.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Authors:
 *     Aaron  Chen <yukun.chen@intel.com>
 *     Nick   Yin  <hu.yin@intel.com>
 * Changes:
 *	11.30.2004 - Kouzmich: porting to HPI-B
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <oh_utils.h>
#include "hpi_cmd.h"
#include "resource.h"


static void print_rpt_entry(SaHpiRptEntryT rptentry)
{
	printf("EntryId: %d\n",rptentry.EntryId);
	printf("ResourceId: %d\n",rptentry.ResourceId);
	printf("Resource Capability: %d\n",rptentry.ResourceCapabilities);
	printf("Severity: %s\n",oh_lookup_severity(rptentry.ResourceSeverity));
	printf("ResourceTag: %s\n", (char *)rptentry.ResourceTag.Data);
}

static void print_sensor_rdr(SaHpiSensorRecT rdr)
{
	printf("Sensor Num:%d",rdr.Num);
	printf("Sensor Type:%s",oh_lookup_sensortype(rdr.Type));
	printf("Category:%d",rdr.Category);
}


static void print_control_rdr(SaHpiCtrlRecT rdr)
{
	printf("Control Num:%d\n",rdr.Num);
	printf("Control Output Type:%s\n",oh_lookup_ctrloutputtype(rdr.OutputType));
	printf("Control Type:%s\n",oh_lookup_ctrltype(rdr.OutputType));
}

static void print_inventory_rdr(SaHpiInventoryRecT rdr)
{
	printf("Oem:%d\n",rdr.Oem);
}

static void print_watchdog_rdr(SaHpiWatchdogRecT rdr)
{
	printf("WatchdogNum:%d\n",rdr.WatchdogNum);
	printf("Oem:%d\n",rdr.Oem);
}

static void print_rdr(SaHpiRdrT rdr)
{
	switch(rdr.RdrType){
		case SAHPI_CTRL_RDR:
			print_control_rdr(rdr.RdrTypeUnion.CtrlRec);
			break;
		case SAHPI_SENSOR_RDR:
			print_sensor_rdr(rdr.RdrTypeUnion.SensorRec);
			break;
		case SAHPI_INVENTORY_RDR:
			print_inventory_rdr(rdr.RdrTypeUnion.InventoryRec);
			break;
		case SAHPI_WATCHDOG_RDR:
			print_watchdog_rdr(rdr.RdrTypeUnion.WatchdogRec);
			break;
		default:
			printf("Unsupported Rdr Type");
	}
	printf("\n");
}

int sa_list_res(void)
{
        SaErrorT ret = 0;
        SaHpiRptEntryT rptentry;
        SaHpiEntryIdT rptentryid;
        SaHpiEntryIdT nextrptentryid;
        SaHpiResourceIdT resourceid;
	int astrisk = 0;

        // walk the RPT list 
        rptentryid = SAHPI_FIRST_ENTRY;
        while ((ret == SA_OK) && (rptentryid != SAHPI_LAST_ENTRY))
        {
                ret = saHpiRptEntryGet(sessionid,rptentryid,&nextrptentryid,&rptentry);
                                                
                if (ret == SA_OK) {
			if ( (rptentry.ResourceCapabilities & SAHPI_CAPABILITY_RDR) &&
					(rptentry.ResourceCapabilities & SAHPI_CAPABILITY_INVENTORY_DATA)){
				astrisk = 1;
				}

                        resourceid = rptentry.ResourceId;
			printf("Resource Id: %d, %sResource Tag: %s\n",rptentry.ResourceId, astrisk?"*":"", (char *)rptentry.ResourceTag.Data);
			astrisk = 0;
                        rptentryid = nextrptentryid;
                }
		else{
			printf("Fail to show RPT\n");
			return -1;
		}

        }

	return 0;
}


int sa_show_rpt(SaHpiResourceIdT resourceid)
{
        SaErrorT ret;
        SaHpiRptEntryT rptentry;

        ret = saHpiRptEntryGetByResourceId(sessionid,resourceid,&rptentry);
        if ( ret ==SA_OK ) 
		print_rpt_entry(rptentry);
	else{
		printf("Fail to show RPT entry of resource %d\n",resourceid); 
		return -1;
	}

	return SA_OK;
}

int sa_show_rdr(SaHpiResourceIdT resourceid)
{
        SaErrorT               rc = 0;
	SaHpiEntryIdT          entryid, next_entryid;
        SaHpiRdrT              rdr;

        entryid = SAHPI_FIRST_ENTRY;
        while ((rc == SA_OK) && (entryid != SAHPI_LAST_ENTRY)) {
                rc = saHpiRdrGet(sessionid, resourceid, entryid, &next_entryid, &rdr);
                if (rc == SA_OK) {
			print_rdr(rdr);
                        entryid = next_entryid;
                } 
                else {
        	        return -1;
                }
	}
	return SA_OK;
}

int sa_discover(void) 
{
	SaErrorT        ret;

        ret = saHpiDiscover(sessionid);
        if (SA_OK != ret)
                printf("saHpiResourcesDiscover failed\n");
        return ret;
}
