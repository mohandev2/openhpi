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
 *     Racing Guo <racing.guo@intel.com>
 * Changes:
 *	11.30.2004 - Kouzmich: porting to HPI-B
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "hpi_cmd.h"

int main(int argc, char **argv)
{
	int	c, eflag = 0;
	char	*val;

	while ( (c = getopt( argc, argv,"e?")) != EOF )
		switch(c)  {
			case 'e':
				eflag = 1;
				break;
			default:
				printf("Usage: %s [-e]\n", argv[0]);
				printf("   -e - show short events, discover after subscribe\n");
				return(1);
		}

	val = getenv("OPENHPI_THREADED");
	if ((val == (char *)NULL) || (strcmp(val, "YES") != 0)) {
		printf("Please, set OPENHPI_THREADED environment!\n");
	};
	if (open_session(eflag) == -1)
		return(1);
	cmd_shell();
	close_session();
	return 0;
}
