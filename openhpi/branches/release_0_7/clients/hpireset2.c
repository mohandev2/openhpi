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
char *progver  = "1.0";
char fdebug = 0;

static void Usage(char *pname)
{
                printf("Usage: %s [-r -d -x]\n", pname);
                printf(" where -r  hard Resets the system\n");
                printf("       -d  powers Down the system\n");
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
  SaHpiVersionT hpiVer;
  SaHpiSessionIdT sessionid;
  SaHpiRptInfoT rptinfo;
  SaHpiRptEntryT rptentry;
  SaHpiEntryIdT rptentryid;
  SaHpiEntryIdT nextrptentryid;
  SaHpiEntryIdT entryid;
  SaHpiResourceIdT resourceid;
  uchar breset;
  uchar bopt;
  uchar fshutdown = 0;
 
  printf("%s ver %s\n", argv[0],progver);
  breset = 3; /* hard reset as default */
  bopt = 0;    /* Boot Options default */
  while ( (c = getopt( argc, argv,"rdx?")) != EOF )
     switch(c) {
          case 'd': breset = 0;     break;  /* power down */
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
  if (fshutdown) breset = 5;     /* soft shutdown option */

  rv = saHpiInitialize(&hpiVer);
  if (rv != SA_OK) {
	printf("saHpiInitialize error %d\n",rv);
	exit(-1);
	}
  rv = saHpiSessionOpen(SAHPI_DEFAULT_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
        if (rv == SA_ERR_HPI_ERROR)
           printf("saHpiSessionOpen: error %d, SpiLibd not running\n",rv);
        else
	   printf("saHpiSessionOpen error %d\n",rv);
	exit(-1);
	}
 
  rv = saHpiResourcesDiscover(sessionid);
  if (fdebug) printf("saHpiResourcesDiscover rv = %d\n",rv);
  rv = saHpiRptInfoGet(sessionid,&rptinfo);
  if (fdebug) printf("saHpiRptInfoGet rv = %d\n",rv);
  printf("RptInfo: UpdateCount = %x, UpdateTime = %lx\n",
         rptinfo.UpdateCount, (unsigned long)rptinfo.UpdateTimestamp);

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
        
	rv1 = saHpiResourceResetStateSet(sessionid, 
	     	resourceid, SAHPI_COLD_RESET);
        printf("ResetStateSet status = %d\n",rv1);
        
	rptentryid = nextrptentryid;
     }
  }
 
  rv = saHpiSessionClose(sessionid);
  rv = saHpiFinalize();

  exit(0);
  return(0);
}
 
/* end hpireset.c */
