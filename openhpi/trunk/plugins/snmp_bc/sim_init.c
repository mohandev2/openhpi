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
 *      Steve Sherman  <stevees@us.ibm.com>
 */

#include <snmp_bc_session.h>
#include <snmp_bc.h>

int is_simulator(void);
int sim_banner(struct snmp_bc_hnd *);
int sim_init(void);
int sim_close(void);

int is_simulator()
{
  return(0);
}

int sim_banner(struct snmp_bc_hnd *custom_handle) 
{
  return(0);  
}

int sim_init() 
{
  return(0);  
}

int sim_close()
{
  return(0);
}
