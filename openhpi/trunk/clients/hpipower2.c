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
 *     Andy Cress <arcress@users.sourceforge.net>
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "SaHpi.h"

#define  uchar  unsigned char
char *progver  = "1.1";
char fdebug = 0;

static void Usage(char *pname)
{
                printf("Usage: %s [-r -d -x]\n", pname);
                printf(" where -r  hard Resets the system\n");
                printf("       -d  powers Down the system\n");
                printf("       -u  powers Up the system\n");
#ifdef MAYBELATER
/*++++ not implemented in HPI 1.0 ++++
                printf("       -c  power Cycles the system\n");
                printf("       -n  sends NMI to the system\n");
                printf("       -o  soft-shutdown OS\n");
                printf("       -s  reboots to Service Partition\n");
 ++++*/
#endif
                printf("       -x  show eXtra debug messages\n");
}

int
main(int argc, char **argv)
{
  int c;
  SaErrorT rv;
  SaHpiSessionIdT sessionid;
  SaHpiRptEntryT rptentry;
  SaHpiEntryIdT rptentryid;
  SaHpiEntryIdT nextrptentryid;
  SaHpiEntryIdT entryid;
  SaHpiResourceIdT resourceid;
  uchar breset;
  uchar bopt;
 
  printf("%s ver %s\n", argv[0],progver);
  breset = 3; /* hard reset as default */
  bopt = 0;    /* Boot Options default */
  while ( (c = getopt( argc, argv,"rdux?")) != EOF )
     switch(c) {
          case 'd': breset = 0;     break;  /* power down SAHPI_POWER_OFF */
          case 'u': breset = 5;     break;  /* power up SAHPI_POWER_ON */
          case 'r': breset = 3;     break;  /* hard reset */
          case 'x': fdebug = 1;     break;  /* debug messages */
#ifdef MAYBELATER
          case 'c': breset = 2;     break;  /* power cycle */
          case 'o': fshutdown = 1;  break;  /* shutdown OS */
          case 'n': breset = 4;     break;  /* interrupt (NMI) */
          case 's': bopt   = 1;     break;  /* hard reset to svc part */
#endif
          default:
		Usage(argv[0]);
                exit(1);
  }

  rv = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
        if (rv == SA_ERR_HPI_ERROR)
           printf("saHpiSessionOpen: error %d, SpiLibd not running\n",rv);
        else
	   printf("saHpiSessionOpen error %d\n",rv);
	exit(-1);
	}
 
  rv = saHpiDiscover(sessionid);
  if (fdebug) printf("saHpiResourcesDiscover rv = %d\n",rv);

  /* walk the RPT list */
  rptentryid = SAHPI_FIRST_ENTRY;
  while ((rv == SA_OK) && (rptentryid != SAHPI_LAST_ENTRY))
  {
     SaErrorT rv1;
     rv = saHpiRptEntryGet(sessionid,rptentryid,&nextrptentryid,&rptentry);
     if (rv != SA_OK) printf("RptEntryGet: rv = %d\n",rv);
     if (rv == SA_OK) {
	/* walk the RDR list for this RPT entry */
	entryid = SAHPI_FIRST_ENTRY;
	resourceid = rptentry.ResourceId;
	rptentry.ResourceTag.Data[rptentry.ResourceTag.DataLength] = 0;
	printf("rptentry[%d] resourceid=%d tag: %s\n",
		entryid,resourceid, rptentry.ResourceTag.Data);
        if (rptentry.ResourceCapabilities && SAHPI_CAPABILITY_POWER) {

            if (breset == 5) {
                rv1 = saHpiResourcePowerStateSet(sessionid, 
                                                 resourceid, SAHPI_POWER_ON);
            }

            if (breset == 0) {
                rv1 = saHpiResourcePowerStateSet(sessionid, 
                                                 resourceid, SAHPI_POWER_OFF);
            }
        }
        
	rptentryid = nextrptentryid;
     }
  }
 
  rv = saHpiSessionClose(sessionid);

  exit(0);
  return(0);
}
 
/* end hpireset.c */
