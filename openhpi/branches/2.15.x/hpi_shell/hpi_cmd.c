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
 *
 * Changes:
 *	11.30.2004 - Kouzmich: porting to HPI-B
 *  28.10.2010 - Anton Pak: fixed -c command line argument
 *  28.10.2010 - Anton Pak: added -D command line argument
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "hpi_cmd.h"

int	debug_flag = 0;

int main(int argc, char **argv)
{
	int	c, eflag = 0;
    SaHpiBoolT printusage = SAHPI_FALSE;
    SaHpiDomainIdT domainId = SAHPI_UNSPECIFIED_DOMAIN_ID;

	while ( (c = getopt( argc, argv,"D:c:ef:xn:?")) != EOF ) {
		switch(c)  {
            case 'D':
                if (optarg) {
                    domainId = atoi(optarg);
                } else {
                    printusage = SAHPI_TRUE;
                }
                break;
			case 'c':
				setenv("OPENHPICLIENT_CONF", optarg, 1);
				break;
			case 'e':
				eflag = 1;
				break;
			case 'f':
				open_file(optarg);
				break;
			case 'x':
				debug_flag = 1;
				break;
            case 'n':
                setenv("OPENHPI_DAEMON_HOST", optarg, 1);
                break;
			default:
                printusage = SAHPI_TRUE;
		}
    }

    if (printusage != SAHPI_FALSE) {
        printf("Usage: %s [-c <cfgfile>][-e][-f <file>][-n <hostname>]\n", argv[0]);
        printf("   -D <did> - select domain id\n");
        printf("   -c <cfgfile> - use passed file as client configuration file\n");
        printf("   -e - show short events, discover after subscribe\n");
        printf("   -f <file> - execute command file\n");
        printf("   -x - display debug messages\n");
        printf("   -n <hostname> - use passed hostname as OpenHPI daemon host\n");
        return 1;
    }

	domainlist = (GSList *)NULL;
	if (open_session(domainId, eflag) == -1)
		return(1);
	cmd_shell();
	close_session();
	return 0;
}

ret_code_t ask_entity(SaHpiEntityPathT *ret)
{
	term_def_t	*term;
	int	res;
    SaErrorT rv;
    char buf[256];
    const char * epstr;

	term = get_next_term();
	if (term == NULL) {
        SaHpiEntityPathT root;
        root.Entry[0].EntityType = SAHPI_ENT_ROOT;
        root.Entry[0].EntityLocation = 0;

		if (read_file) return(HPI_SHELL_PARM_ERROR);
		rv = show_entity_tree(Domain, &root, 0, ui_print);
		if (rv != SA_OK) {
			printf("NO entities!\n");
			return(HPI_SHELL_CMD_ERROR);
		};
		res = get_string_param("Entity Path ==> ", buf, sizeof(buf));
		if (res != 0) {
            printf("Invalid entity path");
            return(HPI_SHELL_PARM_ERROR);
        }
        epstr = buf;
	} else {
        epstr = term->term;
	};

    rv = oh_encode_entitypath(epstr, ret); 
    if ( rv != SA_OK ) {
        printf("Invalid entity path");
        return(HPI_SHELL_PARM_ERROR);
    }

	return(HPI_SHELL_OK);
}

ret_code_t ask_rpt(SaHpiResourceIdT *ret)
{
	term_def_t	*term;
	int		i, res;

	term = get_next_term();
	if (term == NULL) {
		if (read_file) return(HPI_SHELL_PARM_ERROR);
		i = show_rpt_list(Domain, SHOW_ALL_RPT, 0, SHORT_LSRES, ui_print);
		if (i == 0) {
			printf("NO rpts!\n");
			return(HPI_SHELL_CMD_ERROR);
		};
		i = get_int_param("RPT ID ==> ", &res);
		if (i == 1) *ret = (SaHpiResourceIdT)res;
		else return(HPI_SHELL_PARM_ERROR);
	} else {
		*ret = (SaHpiResourceIdT)atoi(term->term);
	};
	return(HPI_SHELL_OK);
}

ret_code_t ask_rdr(SaHpiResourceIdT rptid, SaHpiRdrTypeT type, SaHpiInstrumentIdT *ret)
{
	term_def_t	*term;
	int		i, res;
	char            buf[64];

	strncpy(buf, oh_lookup_rdrtype(type), 64);
	buf[strlen(buf)-4] = '\0';
	strncat(buf, " NUM ==> ", 64-strlen(buf));
	term = get_next_term();
	if (term == NULL) {
		if (read_file) return(HPI_SHELL_CMD_ERROR);
		i = show_rdr_list(Domain, rptid, type, ui_print);
		if (i == 0) {
			printf("No rdrs for rpt: %d\n", rptid);
			return(HPI_SHELL_CMD_ERROR);
		};
		i = get_int_param(buf, &res);
		if (i != 1) return(HPI_SHELL_PARM_ERROR);
		*ret = (SaHpiInstrumentIdT)res;
	} else {
		*ret = (SaHpiInstrumentIdT)atoi(term->term);
	};
	return(HPI_SHELL_OK);
}

ret_code_t open_file(char *path)
{
	if (add_input_file(path) != 0) {
		printf("Can not run file: %s\n", path);
		return(HPI_SHELL_PARM_ERROR);
	};
	read_file = 1;
	read_stdin = 0;
	return(HPI_SHELL_OK);
}
