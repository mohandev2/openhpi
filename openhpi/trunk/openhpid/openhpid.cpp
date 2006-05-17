/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2004-2005
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *      W. David Ashley <dashley@us.ibm.com>
 *
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>
#include <errno.h>
#include <getopt.h>
extern "C"
{
#include <SaHpi.h>
#include <oHpi.h>
#include <oh_error.h>
#include <oh_threaded.h>


#include <oh_init.h>

/*
#include <oh_config.h>
#include <oh_plugin.h>
#include <oh_domain.h>
#include <oh_session.h>
#include <oh_threaded.h>
#include <oh_error.h>
#include <oh_lock.h>
#include <oh_utils.h>

#include <oh_config.h>
#include <oh_lock.h>
#include <oh_plugin.h>
#include <oh_domain.h>
#include <oh_session.h>
*/

}

#include "strmsock.h"
#include "marshal_hpi.h"



/*--------------------------------------------------------------------*/
/* Local definitions                                                  */
/*--------------------------------------------------------------------*/

extern "C"
{

enum tResult
{
   eResultOk,
   eResultError,
   eResultReply,
   eResultClose
};

static bool morph2daemon(void);
static void service_thread(gpointer data, gpointer user_data);
static void HandleInvalidRequest(psstrmsock thrdinst);
static tResult HandleMsg(psstrmsock thrdinst, char *data, GHashTable **ht,
                         SaHpiSessionIdT * sid);
static void hashtablefreeentry(gpointer key, gpointer value, gpointer data);

}

#define CLIENT_TIMEOUT 1800  // 30 minutes
#define PID_FILE "/var/run/openhpid.pid"

static bool stop_server = FALSE;
static bool runasdaemon = TRUE;
static int sock_timeout = CLIENT_TIMEOUT;
static int max_threads = -1;  // unlimited

/* options set by the command line */
static char *pid_file = NULL;
static int verbose_flag = 0;
static struct option long_options[] = {
        {"verbose",   no_argument,       NULL, 'v'},
        {"nondaemon", no_argument,       NULL, 'n'},
        {"config",    required_argument, NULL, 'c'},
        {"port",      required_argument, NULL, 'p'},
        {"pidfile",   required_argument, NULL, 'f'},
        {"timeout",   required_argument, NULL, 's'},
        {"threads",   required_argument, NULL, 't'},
        {0, 0, 0, 0}
};

/* verbose macro */
#define PVERBOSE1(msg) if (verbose_flag) printf(msg); else
#define PVERBOSE2(msg, arg1) if (verbose_flag) printf(msg, arg1); else
#define PVERBOSE3(msg, arg1, arg2) if (verbose_flag) printf(msg, arg1, arg2); else

/*--------------------------------------------------------------------*/
/* Function: display_help                                             */
/*--------------------------------------------------------------------*/

void display_help(void)
{
        printf("Help for openhpid:\n\n");
        printf("   openhpid -c conf_file [-v] [-p port] [-f pidfile]\n\n");
        printf("   -c conf_file  conf_file is the path/name of the configuration file.\n");
        printf("                 This option is required unless the environment\n");
        printf("                 variable OPENHPI_CONF has been set to a valid\n");
        printf("                 configuration file.\n");
        printf("   -v            This option causes the daemon to display verbose\n");
        printf("                 messages. This option is optional.\n");
        printf("   -p port       This overrides the default listening port (4743) of\n");
        printf("                 the daemon. This option is optional.\n");
        printf("   -f pidfile    This overrides the default name/location for the daemon.\n");
        printf("                 pid file. This option is optional.\n");
        printf("   -s seconds    This overrides the default socket read timeout of 30\n");
        printf("                 minutes. This option is optional.\n");
        printf("   -t threads    This sets the maximum number of connection threads.\n");
        printf("                 The default is umlimited. This option is optional.\n");
        printf("   -n            This forces the code to run as a foreground process\n");
        printf("                 and NOT as a daemon. The default is to run as\n");
        printf("                 a daemon. This option is optional.\n\n");
        printf("A typical invocation might be\n\n");
        printf("   ./openhpid -c /etc/openhpi/openhpi.conf\n\n");
}

//static int initialized = FALSE;
/*--------------------------------------------------------------------*/
/* Function: main                                                     */
/*--------------------------------------------------------------------*/

int main (int argc, char *argv[])
{
	GThreadPool *thrdpool;
        int port, c, option_index = 0;
        char *portstr;
        char * configfile = NULL;
        char pid_buf[256];
        int pfile, len, pid = 0;
        SaHpiUint64T version = 0;

        /* get the command line options */
        while (1) {
                c = getopt_long(argc, argv, "nvc:p:f:s:t:", long_options,
                                &option_index);
                /* detect when done scanning options */
                if (c == -1) {
                        break;
                }
                switch (c) {
                case 0:
                        /* no need to do anything here */
                        break;
                case 'c':
                        setenv("OPENHPI_CONF", optarg, 1);
                        break;
                case 'p':
                        setenv("OPENHPI_DAEMON_PORT", optarg, 1);
                        break;
                case 'v':
                        verbose_flag = 1;
                        break;
                case 'f':
                        pid_file = (char *)malloc(strlen(optarg) + 1);
                        strcpy(pid_file, optarg);
                        break;
                case 's':
                        sock_timeout = atoi(optarg);
                        if (sock_timeout < 0) {
                                sock_timeout = CLIENT_TIMEOUT;
                        }
                        break;
                case 't':
                        max_threads = atoi(optarg);
                        if (max_threads < -1 || max_threads == 0) {
                                max_threads = -1;
                        }
                        break;
                case 'n':
                        runasdaemon = FALSE;
                        break;
                case '?':
                        display_help();
                        exit(0);
                default:
			/* they obviously need it */
			display_help();
                        exit(-1);
                }
        }

        if (pid_file == NULL) {
                pid_file = (char *)malloc(strlen(PID_FILE) + 1);
                strcpy(pid_file, PID_FILE);
        }

        if (optind < argc) {
                printf("Error: Unknown command line option specified .\n");
                printf("       Aborting execution.\n\n");
		display_help();
                exit(-1);
        }

        // see if we are already running
        if ((pfile = open(pid_file, O_RDONLY)) > 0) {
                len = read(pfile, pid_buf, sizeof(pid_buf) - 1);
                close(pfile);
                pid_buf[len] = '\0';
                pid = atoi(pid_buf);
                if (pid && (pid == getpid() || kill(pid, 0) < 0)) {
                        unlink(pid_file);
                } else {
                        // there is already a server running
                        printf("Error: There is already a server running .\n");
                        printf("       Aborting execution.\n");
                        exit(1);
                }
        }

        // see if we are trying to use the wrong library
        version = oHpiVersionGet();
        if((version & 0x000000000000ffff) != 0) {
                // we're trying to run against the client lib
                // danger will robinson!
                printf("Error: Trying to run with the OpenHPI client library.\n");
                printf("You DON'T WANT THIS!!!\n");
                printf("Read openhpi-switcher man page to set the environment\n");
                printf("for the daemon to use the standard lib\n");
                exit(1);
        }

        // write the pid file
        pfile = open(pid_file, O_WRONLY | O_CREAT, 0640);
        if (pfile == -1) {
                // there is already a server running
                printf("Error: Cannot open PID file .\n");
                printf("       Aborting execution.\n\n");
                display_help();
                exit(1);
        }
        snprintf(pid_buf, sizeof(pid_buf), "%d\n", (int)getpid());
        write(pfile, pid_buf, strlen(pid_buf));
        close(pfile);

        // see if we have a valid configuration file
        char *cfgfile = getenv("OPENHPI_CONF");
        if (cfgfile == NULL) {
                printf("Error: Configuration file not specified .\n");
                printf("       Aborting execution.\n\n");
		display_help();
                exit(-1);
        }
        if (!g_file_test(cfgfile, G_FILE_TEST_EXISTS)) {
                printf("Error: Configuration file does not exist.\n");
                printf("       Aborting execution.\n\n");
		display_help();
                exit(-1);
        }
        configfile = (char *) malloc(strlen(cfgfile) + 1);
        strcpy(configfile, cfgfile);

        // get our listening port
        portstr = getenv("OPENHPI_DAEMON_PORT");
        if (portstr == NULL) {
                port =  4743;
        }
        else {
                port =  atoi(portstr);
        }

        // become a daemon
        if (!morph2daemon()) {
		exit(8);
	}
#if 1
//printf("_initialize called\n");
//_initialize();
//_init2(); 
_initialize(TRUE);
//printf("_initialize returned\n");
#endif
        // as a daemon we do NOT inherit the environment!
        setenv("OPENHPI_CONF", configfile, 1);

        // create the thread pool
        if (!g_thread_supported()) {
                g_thread_init(NULL);
        }

	thrdpool = g_thread_pool_new(service_thread, NULL, max_threads, FALSE, NULL);

        // create the server socket
	psstrmsock servinst = new sstrmsock;
	if (servinst->Create(port)) {
		printf("Error creating server socket.\n");
		g_thread_pool_free(thrdpool, FALSE, TRUE);
                	delete servinst;
		return 8;
	}


#if 0
        /* if we are in threaded mode and runnning as a daemon */
        oHpiGlobalParamT my_global_param;
        my_global_param.Type = OHPI_DAEMON_MODE;
        oHpiGlobalParamGet(&my_global_param);
        if ((my_global_param.u.Daemon) && (initialized == FALSE)) {
            initialized = TRUE;
            oh_threaded_start();
            trace(" ### we are running as a daemon my_global_param.u.Daemon[%d]###\n", my_global_param.u.Daemon);
        } else {
            trace(" ### we are not running as a daemon my_global_param.u.Daemon[%d]###\n", my_global_param.u.Daemon);
        }
#endif
        // announce ourselves
        printf("%s started.\n", argv[0]);
        printf("OPENHPI_CONF = %s\n", configfile);
        printf("OPENHPI_DAEMON_PORT = %d\n\n", port);

        // wait for a connection and then service the connection
	while (TRUE) {

		if (stop_server) {
			break;
		}

		if (servinst->Accept()) {
			PVERBOSE1("Error accepting server socket.\n");
			break;
		}

		PVERBOSE1("### Spawning thread to handle connection. ###\n");
		psstrmsock thrdinst = new sstrmsock(*servinst);
		g_thread_pool_push(thrdpool, (gpointer)thrdinst, NULL);

        
	}

	servinst->CloseSrv();
	PVERBOSE1("Server socket closed.\n");

        // ensure all threads are complete
	g_thread_pool_free(thrdpool, FALSE, TRUE);

	delete servinst;
	return 0;
}


/*--------------------------------------------------------------------*/
/* Function: morph2daemon                                             */
/*--------------------------------------------------------------------*/

static bool morph2daemon(void)
{
        char pid_buf[256];
        int pfile;

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		return false;
	}

	if (runasdaemon) {
		pid_t pid = fork();
		if (pid < 0) {
			return false;
		}
                // parent process
		if (pid != 0) {
			exit( 0 );
		}

        // become the session leader
		setsid();
        // second fork to become a real daemon
		pid = fork();
		if (pid < 0) {
			return false;
		}

        // parent process
		if (pid != 0) {
			exit(0);
		}

        // create the pid file (overwrite of old pid file is ok)
        unlink(pid_file);
        pfile = open(pid_file, O_WRONLY | O_CREAT, 0640);
        snprintf(pid_buf, sizeof(pid_buf), "%d\n", (int)getpid());
        write(pfile, pid_buf, strlen(pid_buf));
        close(pfile);

        // housekeeping
		chdir("/");
		umask(0);
		for(int i = 0; i < 1024; i++) {
			close(i);
		}
	}

	return true;
}


/*--------------------------------------------------------------------*/
/* Function: service_thread                                           */
/*--------------------------------------------------------------------*/

static void service_thread(gpointer data, gpointer user_data)
{
	psstrmsock thrdinst = (psstrmsock) data;
        bool stop = false;
	char buf[dMaxMessageLength];
        tResult result;
        GHashTable *thrdhashtable = NULL;
        gpointer thrdid = g_thread_self();
        SaHpiSessionIdT session_id = 0;

	PVERBOSE2("%p Servicing connection.\n", thrdid);

        /* set the read timeout for the socket */
        thrdinst->SetReadTimeout(sock_timeout);

    PVERBOSE2("### service_thread, thrdid [%p] ###\n", (void *)thrdid); 

	while (stop == false) {
                if (thrdinst->ReadMsg(buf)) {
                        if (thrdinst->GetErrcode() == EWOULDBLOCK) {
                                PVERBOSE2("%p Timeout reading socket.\n", thrdid);
                        } else {
                                PVERBOSE2("%p Error reading socket.\n", thrdid);
                        }
                        goto thrd_cleanup;
                }
                else {
                        switch( thrdinst->header.m_type ) {
                        case eMhMsg:
                                result = HandleMsg(thrdinst, buf,
                                                   &thrdhashtable, &session_id);
                                // marshal error ?
                                if (result == eResultError) {
                                        PVERBOSE2("%p Invalid API found.\n", thrdid);
                                        HandleInvalidRequest(thrdinst);
                                }
                                // done ?
                                if (result == eResultClose) {
                                        stop = true;
                                }
                                break;
                        default:
                                PVERBOSE2("%p Error in socket read buffer data.\n", thrdid);
                                HandleInvalidRequest(thrdinst);
                                break;
                        }
                }
	}

        thrd_cleanup:
        // if necessary, clean up HPI lib data
        if (session_id != 0) {
                saHpiSessionClose( session_id );
        }
        delete thrdinst; // cleanup thread instance data

	PVERBOSE2("%p Connection ended.\n", thrdid);
	return; // do NOT use g_thread_exit here!
}


/*--------------------------------------------------------------------*/
/* Function: HandleInvalidRequest                                     */
/*--------------------------------------------------------------------*/

void HandleInvalidRequest(psstrmsock thrdinst) {
        gpointer thrdid = g_thread_self();

       /* create and deliver a pong message */
       PVERBOSE2("%p Invalid request.\n", thrdid);
       thrdinst->MessageHeaderInit(eMhError, 0, thrdinst->header.m_id, 0 );
       thrdinst->WriteMsg(NULL);

       return;
}


/*--------------------------------------------------------------------*/
/* Function: HandleMsg                                                */
/*--------------------------------------------------------------------*/

static tResult HandleMsg(psstrmsock thrdinst, char *data, GHashTable **ht,
                         SaHpiSessionIdT * sid)
{
  cHpiMarshal *hm;
  SaErrorT ret;
  tResult result = eResultReply;
  char *pReq = data + sizeof(cMessageHeader);
  gpointer thrdid = g_thread_self();

  hm = HpiMarshalFind(thrdinst->header.m_id);

  // init reply header
  thrdinst->MessageHeaderInit((tMessageType) thrdinst->header.m_type, 0,
                                 thrdinst->header.m_id, hm->m_reply_len );

  switch( thrdinst->header.m_id ) {
       case eFsaHpiVersionGet: {
	      SaHpiVersionT ver;

              PVERBOSE2("%p Processing saHpiVersionGet.\n", thrdid);

	      ver = saHpiVersionGet( );

	      thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ver );
              result = eResultClose;
       }
       break;

       case eFsaHpiSessionOpen: {
              SaHpiDomainIdT  domain_id;
              SaHpiSessionIdT session_id = 0;
              void            *securityparams = NULL;

              PVERBOSE2("%p Processing saHpiSessionOpen.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &domain_id ) < 0 )
                   return eResultError;

              ret = saHpiSessionOpen( domain_id, &session_id, securityparams );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &session_id );

              // this is used in case the connection ever breaks!
              *sid = session_id;
       }
       break;

       case eFsaHpiSessionClose: {
	      SaHpiSessionIdT session_id;

              PVERBOSE2("%p Processing saHpiSessionClose.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id ) < 0 )
		   return eResultError;

	      ret = saHpiSessionClose( session_id );

	      thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
              result = eResultClose;
              *sid = 0;
       }
       break;

       case eFsaHpiDiscover: {
	      SaHpiSessionIdT session_id;

              PVERBOSE2("%p Processing saHpiDiscover.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id ) < 0 )
		   return eResultError;

	      ret = saHpiDiscover( session_id );

	      thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiDomainInfoGet: {
              SaHpiSessionIdT  session_id;
              SaHpiDomainInfoT domain_info;

              PVERBOSE2("%p Processing saHpiDomainInfoGet.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id ) < 0 )
                   return eResultError;

              ret = saHpiDomainInfoGet( session_id, &domain_info );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &domain_info );
       }
       break;

       case eFsaHpiDrtEntryGet: {
              SaHpiSessionIdT session_id;
              SaHpiEntryIdT   entry_id;
              SaHpiEntryIdT   next_entry_id = 0;
              SaHpiDrtEntryT  drt_entry;

              PVERBOSE2("%p Processing saHpiDrtEntryGet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &entry_id ) < 0 )
                   return eResultError;

              ret = saHpiDrtEntryGet( session_id, entry_id, &next_entry_id,
                                      &drt_entry );

              thrdinst->header.m_len = HpiMarshalReply2( hm, pReq, &ret, &next_entry_id, &drt_entry );
       }
       break;

       case eFsaHpiDomainTagSet: {
              SaHpiSessionIdT  session_id;
              SaHpiTextBufferT domain_tag;

              PVERBOSE2("%p Processing saHpiDomainTagSet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &domain_tag ) < 0 )
                   return eResultError;

              ret = saHpiDomainTagSet( session_id, &domain_tag );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiRptEntryGet: {
              SaHpiSessionIdT session_id;
              SaHpiEntryIdT   entry_id;
              SaHpiEntryIdT   next_entry_id = 0; // for valgring
              SaHpiRptEntryT  rpt_entry;

              PVERBOSE2("%p Processing saHpiRptEntryGet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &entry_id ) < 0 )
                   return eResultError;

              ret = saHpiRptEntryGet( session_id, entry_id, &next_entry_id, &rpt_entry );

              thrdinst->header.m_len = HpiMarshalReply2( hm, pReq, &ret, &next_entry_id, &rpt_entry );
       }
       break;

       case eFsaHpiRptEntryGetByResourceId: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiRptEntryT   rpt_entry;

              PVERBOSE2("%p Processing saHpiRptEntryGetByResourceId.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiRptEntryGetByResourceId( session_id, resource_id, &rpt_entry );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &rpt_entry );
       }
       break;

       case eFsaHpiResourceSeveritySet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiSeverityT   severity;

              PVERBOSE2("%p Processing saHpiResourceSeveritySet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &severity ) < 0 )
                   return eResultError;

              ret = saHpiResourceSeveritySet( session_id,
                                              resource_id, severity );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiResourceTagSet:
            {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiTextBufferT resource_tag;

              PVERBOSE2("%p Processing saHpiResourceTagSet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &resource_tag ) < 0 )
                   return eResultError;

              ret = saHpiResourceTagSet( session_id, resource_id, &resource_tag );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }

       break;

       case eFsaHpiResourceIdGet: {
              SaHpiSessionIdT session_id;
              SaHpiResourceIdT resource_id = 0;

              PVERBOSE2("%p Processing saHpiResourceIdGet.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id ) < 0 )
                   return eResultError;

              ret = saHpiResourceIdGet( session_id, &resource_id );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &resource_id );
       }

       break;

       case eFsaHpiEventLogInfoGet: {
              SaHpiSessionIdT    session_id;
              SaHpiResourceIdT   resource_id;
              SaHpiEventLogInfoT info;

              PVERBOSE2("%p Processing saHpiEventLogInfoGet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiEventLogInfoGet( session_id, resource_id, &info );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &info );
       }
       break;

       case eFsaHpiEventLogEntryGet: {
              SaHpiSessionIdT       session_id;
              SaHpiResourceIdT      resource_id;
              SaHpiEventLogEntryIdT entry_id;
              SaHpiEventLogEntryIdT prev_entry_id = 0;
              SaHpiEventLogEntryIdT next_entry_id = 0;
              SaHpiEventLogEntryT   event_log_entry;
              SaHpiRdrT             rdr;
              SaHpiRptEntryT        rpt_entry;

              PVERBOSE2("%p Processing saHpiEventLogEntryGet.\n", thrdid);

              memset( &rdr, 0, sizeof( SaHpiRdrT ) );
              memset( &rpt_entry, 0, sizeof( SaHpiRptEntryT ) );

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &entry_id ) < 0 )
                   return eResultError;

              ret = saHpiEventLogEntryGet( session_id, resource_id, entry_id,
                                           &prev_entry_id, &next_entry_id,
                                           &event_log_entry, &rdr, &rpt_entry );

              thrdinst->header.m_len = HpiMarshalReply5( hm, pReq, &ret, &prev_entry_id, &next_entry_id,
                                                         &event_log_entry, &rdr, &rpt_entry );
       }
       break;

       case eFsaHpiEventLogEntryAdd: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiEventT      evt_entry;

              PVERBOSE2("%p Processing saHpiEventLogEntryAdd.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &evt_entry ) < 0 )
                   return eResultError;

              ret = saHpiEventLogEntryAdd( session_id, resource_id,
                                           &evt_entry );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiEventLogClear: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;

              PVERBOSE2("%p Processing saHpiEventLogClear.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiEventLogClear( session_id, resource_id );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiEventLogTimeGet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiTimeT       ti;

              PVERBOSE2("%p Processing saHpiEventLogTimeGet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiEventLogTimeGet( session_id, resource_id, &ti );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &ti );
       }
       break;

       case eFsaHpiEventLogTimeSet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiTimeT       ti;

              PVERBOSE2("%p Processing saHpiEventLogTimeSet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &ti ) < 0 )
                   return eResultError;

              ret = saHpiEventLogTimeSet( session_id, resource_id, ti );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiEventLogStateGet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiBoolT       enable;

              PVERBOSE2("%p Processing saHpiEventLogStateGet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiEventLogStateGet( session_id, resource_id, &enable );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &enable );
       }
       break;

       case eFsaHpiEventLogStateSet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiBoolT       enable;

              PVERBOSE2("%p Processing saHpiEventLogStateSet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &enable ) < 0 )
                   return eResultError;

              ret = saHpiEventLogStateSet( session_id, resource_id, enable );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiEventLogOverflowReset: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;

              PVERBOSE2("%p Processing saHpiEventLogOverflowReset.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiEventLogOverflowReset( session_id, resource_id );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiSubscribe: {
              SaHpiSessionIdT session_id;

              PVERBOSE2("%p Processing saHpiSubscribe.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id ) < 0 )
                   return eResultError;

              ret = saHpiSubscribe( session_id );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiUnsubscribe: {
              SaHpiSessionIdT session_id;

              PVERBOSE2("%p Processing saHpiUnsubscribe.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id ) < 0 )
                   return eResultError;

              ret = saHpiUnsubscribe( session_id );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiEventGet: {
              SaHpiSessionIdT      session_id;
              SaHpiTimeoutT        timeout;
              SaHpiEventT          event;
              SaHpiRdrT            rdr;
              SaHpiRptEntryT       rpt_entry;
              SaHpiEvtQueueStatusT status;

              PVERBOSE2("%p Processing saHpiEventGet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &timeout ) < 0 )
                   return eResultError;

              ret = saHpiEventGet( session_id, timeout, &event, &rdr,
                                   &rpt_entry, &status );

              thrdinst->header.m_len = HpiMarshalReply4( hm, pReq, &ret, &event, &rdr, &rpt_entry, &status );
       }
       break;

       case eFsaHpiEventAdd: {
              SaHpiSessionIdT session_id;
              SaHpiEventT     event;

              PVERBOSE2("%p Processing saHpiEventAdd.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &event ) < 0 )
                   return eResultError;

              ret = saHpiEventAdd( session_id, &event );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiAlarmGetNext: {
              SaHpiSessionIdT session_id;
              SaHpiSeverityT  severity;
              SaHpiBoolT      unack;
              SaHpiAlarmT     alarm;

              PVERBOSE2("%p Processing saHpiAlarmGetNext.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &severity,
                                         &unack, &alarm ) < 0 )
                   return eResultError;

              ret = saHpiAlarmGetNext( session_id, severity, unack, &alarm );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &alarm );
       }
       break;

       case eFsaHpiAlarmGet: {
              SaHpiSessionIdT session_id;
              SaHpiAlarmIdT   alarm_id;
              SaHpiAlarmT     alarm;

              PVERBOSE2("%p Processing saHpiAlarmGet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &alarm_id ) < 0 )
                   return eResultError;

              ret = saHpiAlarmGet( session_id, alarm_id, &alarm );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &alarm );
       }
       break;

       case eFsaHpiAlarmAcknowledge: {
              SaHpiSessionIdT session_id;
              SaHpiAlarmIdT   alarm_id;
              SaHpiSeverityT  severity;

              PVERBOSE2("%p Processing saHpiAlarmAcknowledge.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &alarm_id,
                                         &severity ) < 0 )
                   return eResultError;

              ret = saHpiAlarmAcknowledge( session_id, alarm_id, severity );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiAlarmAdd: {
              SaHpiSessionIdT session_id;
              SaHpiAlarmT     alarm;

              PVERBOSE2("%p Processing saHpiAlarmAdd.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &alarm ) < 0 )
                   return eResultError;

              ret = saHpiAlarmAdd( session_id, &alarm );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &alarm );
       }
       break;

       case eFsaHpiAlarmDelete: {
              SaHpiSessionIdT session_id;
              SaHpiAlarmIdT   alarm_id;
              SaHpiSeverityT  severity;

              PVERBOSE2("%p Processing saHpiAlarmDelete.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &alarm_id,
                                         &severity ) < 0 )
                   return eResultError;

              ret = saHpiAlarmDelete( session_id, alarm_id, severity );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiRdrGet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiEntryIdT    entry_id;
              SaHpiEntryIdT    next_entry_id;
              SaHpiRdrT        rdr;

              PVERBOSE2("%p Processing saHpiRdrGet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &entry_id ) < 0 )
                   return eResultError;

              ret = saHpiRdrGet( session_id, resource_id, entry_id,
                                 &next_entry_id, &rdr );

              thrdinst->header.m_len = HpiMarshalReply2( hm, pReq, &ret, &next_entry_id, &rdr );
       }
       break;

       case eFsaHpiRdrGetByInstrumentId: {
              SaHpiSessionIdT    session_id;
              SaHpiResourceIdT   resource_id;
              SaHpiRdrTypeT      rdr_type;
              SaHpiInstrumentIdT inst_id;
              SaHpiRdrT          rdr;

              PVERBOSE2("%p Processing saHpiRdrGetByInstrumentId.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &rdr_type, &inst_id ) < 0 )
                   return eResultError;

              ret = saHpiRdrGetByInstrumentId( session_id, resource_id, rdr_type,
                                               inst_id, &rdr );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &rdr );
       }
       break;

       case eFsaHpiSensorReadingGet: {
              SaHpiSessionIdT     session_id;
              SaHpiResourceIdT    resource_id;
              SaHpiSensorNumT     sensor_num;
              SaHpiSensorReadingT reading;
              SaHpiEventStateT    state;

              PVERBOSE2("%p Processing saHpiSensorReadingGet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &sensor_num ) < 0 )
                   return eResultError;

              ret = saHpiSensorReadingGet( session_id, resource_id,
                                           sensor_num, &reading, &state );

              thrdinst->header.m_len = HpiMarshalReply2( hm, pReq, &ret, &reading, &state );
       }
       break;

       case eFsaHpiSensorThresholdsGet: {
              SaHpiSessionIdT        session_id;
              SaHpiResourceIdT       resource_id;
              SaHpiSensorNumT        sensor_num;
              SaHpiSensorThresholdsT sensor_thresholds;

              PVERBOSE2("%p Processing saHpiSensorThresholdsGet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &sensor_num ) < 0 )
                   return eResultError;

              ret = saHpiSensorThresholdsGet( session_id,
                                              resource_id, sensor_num,
                                              &sensor_thresholds);

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &sensor_thresholds );
       }
       break;

       case eFsaHpiSensorThresholdsSet: {
              SaHpiSessionIdT        session_id;
              SaHpiResourceIdT       resource_id;
              SaHpiSensorNumT        sensor_num;
              SaHpiSensorThresholdsT sensor_thresholds;

              PVERBOSE2("%p Processing saHpiSensorThresholdsSet.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &sensor_num, &sensor_thresholds ) < 0 )
                   return eResultError;

              ret = saHpiSensorThresholdsSet( session_id, resource_id,
                                              sensor_num,
                                              &sensor_thresholds );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiSensorTypeGet: {
              SaHpiSessionIdT     session_id;
              SaHpiResourceIdT    resource_id;
              SaHpiSensorNumT     sensor_num;
              SaHpiSensorTypeT    type;
              SaHpiEventCategoryT category;

              PVERBOSE2("%p Processing saHpiSensorTypeGet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &sensor_num ) < 0 )
                   return eResultError;

              ret = saHpiSensorTypeGet( session_id, resource_id,
                                        sensor_num, &type, &category );

              thrdinst->header.m_len = HpiMarshalReply2( hm, pReq, &ret, &type, &category );
       }
       break;

       case eFsaHpiSensorEnableGet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiSensorNumT  sensor_num;
              SaHpiBoolT       enabled;

              PVERBOSE2("%p Processing saHpiSensorEnableGet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &sensor_num ) < 0 )
                   return eResultError;

              ret = saHpiSensorEnableGet( session_id, resource_id,
                                          sensor_num, &enabled );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &enabled );
       }
       break;

       case eFsaHpiSensorEnableSet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiSensorNumT  sensor_num;
              SaHpiBoolT       enabled;

              PVERBOSE2("%p Processing saHpiSensorEnableSet.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &sensor_num, &enabled ) < 0 )
                   return eResultError;

              ret = saHpiSensorEnableSet( session_id, resource_id,
                                          sensor_num, enabled );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiSensorEventEnableGet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiSensorNumT  sensor_num;
              SaHpiBoolT       enables;

              PVERBOSE2("%p Processing saHpiSensorEventEnableGet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &sensor_num ) < 0 )
                   return eResultError;

              ret = saHpiSensorEventEnableGet( session_id, resource_id,
                                               sensor_num, &enables );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &enables );
       }
       break;

       case eFsaHpiSensorEventEnableSet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiSensorNumT  sensor_num;
              SaHpiBoolT       enables;

              PVERBOSE2("%p Processing saHpiSensorEventEnableSet.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &sensor_num, &enables ) < 0 )
                   return eResultError;

              ret = saHpiSensorEventEnableSet( session_id, resource_id,
                                               sensor_num, enables );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiSensorEventMasksGet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiSensorNumT  sensor_num;
              SaHpiEventStateT assert_mask;
              SaHpiEventStateT deassert_mask;

              PVERBOSE2("%p Processing saHpiSensorEventMasksGet.\n", thrdid);

              if ( HpiDemarshalRequest5( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &sensor_num, &assert_mask,
                                         &deassert_mask ) < 0 )
                   return eResultError;

              ret = saHpiSensorEventMasksGet( session_id, resource_id, sensor_num,
                                              &assert_mask, &deassert_mask );

              thrdinst->header.m_len = HpiMarshalReply2( hm, pReq, &ret, &assert_mask, &deassert_mask );
       }
       break;

       case eFsaHpiSensorEventMasksSet: {
              SaHpiSessionIdT             session_id;
              SaHpiResourceIdT            resource_id;
              SaHpiSensorNumT             sensor_num;
              SaHpiSensorEventMaskActionT action;
              SaHpiEventStateT            assert_mask;
              SaHpiEventStateT            deassert_mask;

              PVERBOSE2("%p Processing saHpiSensorEventMasksSet.\n", thrdid);

              if ( HpiDemarshalRequest6( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &sensor_num, &action, &assert_mask,
                                         &deassert_mask ) < 0 )
                   return eResultError;

              ret = saHpiSensorEventMasksSet( session_id, resource_id, sensor_num,
                                              action, assert_mask, deassert_mask );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiControlTypeGet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiCtrlNumT    ctrl_num;
              SaHpiCtrlTypeT   type;

              PVERBOSE2("%p Processing saHpiControlTypeGet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &ctrl_num ) < 0 )
                   return eResultError;

              ret = saHpiControlTypeGet( session_id, resource_id, ctrl_num,
                                         &type );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &type );
       }
       break;

       case eFsaHpiControlGet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiCtrlNumT    ctrl_num;
              SaHpiCtrlModeT   ctrl_mode;
              SaHpiCtrlStateT  ctrl_state;

              PVERBOSE2("%p Processing saHpiControlGet.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &ctrl_num, &ctrl_state ) < 0 )
                      return eResultError;

              ret = saHpiControlGet( session_id, resource_id, ctrl_num,
                                     &ctrl_mode, &ctrl_state );

              thrdinst->header.m_len = HpiMarshalReply2( hm, pReq, &ret, &ctrl_mode, &ctrl_state );
       }
       break;

       case eFsaHpiControlSet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiCtrlNumT    ctrl_num;
              SaHpiCtrlModeT   ctrl_mode;
              SaHpiCtrlStateT  ctrl_state;

              PVERBOSE2("%p Processing saHpiControlSet.\n", thrdid);

              if ( HpiDemarshalRequest5( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &ctrl_num, &ctrl_mode, &ctrl_state ) < 0 )
                   return eResultError;

              ret = saHpiControlSet( session_id, resource_id,
                                     ctrl_num, ctrl_mode, &ctrl_state );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiIdrInfoGet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiIdrIdT      idr_id;
              SaHpiIdrInfoT    info;

              PVERBOSE2("%p Processing saHpiIdrInfoGet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &idr_id ) < 0 )
                   return eResultError;

              ret = saHpiIdrInfoGet( session_id, resource_id,
                                     idr_id, &info );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &info );
       }
       break;

       case eFsaHpiIdrAreaHeaderGet: {
              SaHpiSessionIdT     session_id;
              SaHpiResourceIdT    resource_id;
              SaHpiIdrIdT         idr_id;
              SaHpiIdrAreaTypeT   area;
              SaHpiEntryIdT       area_id;
              SaHpiEntryIdT       next;
              SaHpiIdrAreaHeaderT header;

              PVERBOSE2("%p Processing saHpiIdrAreaHeaderGet.\n", thrdid);

              if ( HpiDemarshalRequest5( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &idr_id, &area, &area_id ) < 0 )
                   return eResultError;

              ret = saHpiIdrAreaHeaderGet( session_id, resource_id, idr_id,
                                           area, area_id, &next, &header );

              thrdinst->header.m_len = HpiMarshalReply2( hm, pReq, &ret, &next, &header );
       }
       break;

       case eFsaHpiIdrAreaAdd: {
              SaHpiSessionIdT     session_id;
              SaHpiResourceIdT    resource_id;
              SaHpiIdrIdT         idr_id;
              SaHpiIdrAreaTypeT   area;
              SaHpiEntryIdT       area_id;

              PVERBOSE2("%p Processing saHpiIdrAreaAdd.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &idr_id, &area ) < 0 )
                   return eResultError;

              ret = saHpiIdrAreaAdd( session_id, resource_id, idr_id,
                                     area, &area_id  );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &area_id );
       }
       break;

       case eFsaHpiIdrAreaDelete: {
              SaHpiSessionIdT     session_id;
              SaHpiResourceIdT    resource_id;
              SaHpiIdrIdT         idr_id;
              SaHpiEntryIdT       area_id;

              PVERBOSE2("%p Processing saHpiIdrAreaAdd.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &idr_id, &area_id ) < 0 )
                   return eResultError;

              ret = saHpiIdrAreaDelete( session_id, resource_id, idr_id,
                                        area_id  );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiIdrFieldGet: {
              SaHpiSessionIdT    session_id;
              SaHpiResourceIdT   resource_id;
              SaHpiIdrIdT        idr_id;
              SaHpiEntryIdT      area_id;
              SaHpiIdrFieldTypeT type;
              SaHpiEntryIdT      field_id;
              SaHpiEntryIdT      next;
              SaHpiIdrFieldT     field;

              PVERBOSE2("%p Processing saHpiIdrFieldGet.\n", thrdid);

              if ( HpiDemarshalRequest6( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &idr_id, &area_id, &type, &field_id ) < 0 )
                   return eResultError;

              ret = saHpiIdrFieldGet( session_id, resource_id, idr_id, area_id,
                                      type, field_id, &next, &field );

              thrdinst->header.m_len = HpiMarshalReply2( hm, pReq, &ret, &next, &field );
       }
       break;

       case eFsaHpiIdrFieldAdd: {
              SaHpiSessionIdT    session_id;
              SaHpiResourceIdT   resource_id;
              SaHpiIdrIdT        idr_id;
              SaHpiIdrFieldT     field;

              PVERBOSE2("%p Processing saHpiIdrFieldAdd.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &idr_id, &field ) < 0 )
                   return eResultError;

              ret = saHpiIdrFieldAdd( session_id, resource_id, idr_id,
                                      &field );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &field );
       }
       break;

       case eFsaHpiIdrFieldSet: {
              SaHpiSessionIdT    session_id;
              SaHpiResourceIdT   resource_id;
              SaHpiIdrIdT        idr_id;
              SaHpiIdrFieldT     field;

              PVERBOSE2("%p Processing saHpiIdrFieldSet.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &idr_id, &field ) < 0 )
                   return eResultError;

              ret = saHpiIdrFieldSet( session_id, resource_id, idr_id,
                                      &field );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiIdrFieldDelete: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiIdrIdT      idr_id;
              SaHpiEntryIdT    area_id;
              SaHpiEntryIdT    field_id;

              PVERBOSE2("%p Processing saHpiIdrFieldSet.\n", thrdid);

              if ( HpiDemarshalRequest5( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &idr_id, &area_id, &field_id ) < 0 )
                   return eResultError;

              ret = saHpiIdrFieldDelete( session_id, resource_id, idr_id,
                                         area_id, field_id );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiWatchdogTimerGet: {
              SaHpiSessionIdT   session_id;
              SaHpiResourceIdT  resource_id;
              SaHpiWatchdogNumT watchdog_num;
              SaHpiWatchdogT    watchdog;

              PVERBOSE2("%p Processing saHpiWatchdogTimerGet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &watchdog_num ) < 0 )
                   return eResultError;

              ret = saHpiWatchdogTimerGet( session_id, resource_id,
                                           watchdog_num, &watchdog );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &watchdog );
       }
       break;

       case eFsaHpiWatchdogTimerSet: {
              SaHpiSessionIdT   session_id;
              SaHpiResourceIdT  resource_id;
              SaHpiWatchdogNumT watchdog_num;
              SaHpiWatchdogT    watchdog;

              PVERBOSE2("%p Processing saHpiWatchdogTimerSet.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &watchdog_num, &watchdog ) < 0 )
                   return eResultError;

              ret = saHpiWatchdogTimerSet( session_id, resource_id,
                                           watchdog_num, &watchdog );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiWatchdogTimerReset: {
              SaHpiSessionIdT   session_id;
              SaHpiResourceIdT  resource_id;
              SaHpiWatchdogNumT watchdog_num;

              PVERBOSE2("%p Processing saHpiWatchdogTimerReset.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &watchdog_num ) < 0 )
                   return eResultError;

              ret = saHpiWatchdogTimerReset( session_id, resource_id,
                                             watchdog_num );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiAnnunciatorGetNext: {
              SaHpiSessionIdT      session_id;
              SaHpiResourceIdT     resource_id;
              SaHpiAnnunciatorNumT annun_num;
              SaHpiSeverityT       severity;
              SaHpiBoolT           unack;
              SaHpiAnnouncementT   announcement;

              PVERBOSE2("%p Processing saHpiAnnunciatorGetNext.\n", thrdid);

              if ( HpiDemarshalRequest6( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &annun_num, &severity, &unack,
                                         &announcement ) < 0 )
                   return eResultError;

              ret = saHpiAnnunciatorGetNext( session_id, resource_id, annun_num,
                                             severity, unack, &announcement );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &announcement );
       }
       break;

       case eFsaHpiAnnunciatorGet: {
              SaHpiSessionIdT      session_id;
              SaHpiResourceIdT     resource_id;
              SaHpiAnnunciatorNumT annun_num;
              SaHpiEntryIdT        entry_id;
              SaHpiAnnouncementT   announcement;

              PVERBOSE2("%p Processing saHpiAnnunciatorGet.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &annun_num, &entry_id ) < 0 )
                   return eResultError;

              ret = saHpiAnnunciatorGet( session_id, resource_id, annun_num,
                                         entry_id, &announcement );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &announcement );
       }
       break;

       case eFsaHpiAnnunciatorAcknowledge: {
              SaHpiSessionIdT      session_id;
              SaHpiResourceIdT     resource_id;
              SaHpiAnnunciatorNumT annun_num;
              SaHpiEntryIdT        entry_id;
              SaHpiSeverityT       severity;

              PVERBOSE2("%p Processing saHpiAnnunciatorAcknowledge.\n", thrdid);

              if ( HpiDemarshalRequest5( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &annun_num, &entry_id, &severity ) < 0 )
                   return eResultError;

              ret = saHpiAnnunciatorAcknowledge( session_id, resource_id, annun_num,
                                                 entry_id, severity );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiAnnunciatorAdd: {
              SaHpiSessionIdT      session_id;
              SaHpiResourceIdT     resource_id;
              SaHpiAnnunciatorNumT annun_num;
              SaHpiAnnouncementT   announcement;

              PVERBOSE2("%p Processing saHpiAnnunciatorAdd.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &annun_num, &announcement ) < 0 )
                   return eResultError;

              ret = saHpiAnnunciatorAdd( session_id, resource_id, annun_num,
                                         &announcement );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &announcement );
       }
       break;

       case eFsaHpiAnnunciatorDelete: {
              SaHpiSessionIdT      session_id;
              SaHpiResourceIdT     resource_id;
              SaHpiAnnunciatorNumT annun_num;
              SaHpiEntryIdT        entry_id;
              SaHpiSeverityT       severity;

              PVERBOSE2("%p Processing saHpiAnnunciatorAdd.\n", thrdid);

              if ( HpiDemarshalRequest5( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &annun_num, &entry_id, &severity ) < 0 )
                   return eResultError;

              ret = saHpiAnnunciatorDelete( session_id, resource_id, annun_num,
                                            entry_id, severity );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiAnnunciatorModeGet: {
              SaHpiSessionIdT       session_id;
              SaHpiResourceIdT      resource_id;
              SaHpiAnnunciatorNumT  annun_num;
              SaHpiAnnunciatorModeT mode;

              PVERBOSE2("%p Processing saHpiAnnunciatorModeGet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &annun_num ) < 0 )
                   return eResultError;

              ret = saHpiAnnunciatorModeGet( session_id, resource_id, annun_num,
                                             &mode );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &mode );
       }
       break;

       case eFsaHpiAnnunciatorModeSet: {
              SaHpiSessionIdT       session_id;
              SaHpiResourceIdT      resource_id;
              SaHpiAnnunciatorNumT  annun_num;
              SaHpiAnnunciatorModeT mode;

              PVERBOSE2("%p Processing saHpiAnnunciatorModeSet.\n", thrdid);

              if ( HpiDemarshalRequest4( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &annun_num, &mode ) < 0 )
                   return eResultError;

              ret = saHpiAnnunciatorModeSet( session_id, resource_id, annun_num,
                                             mode );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiHotSwapPolicyCancel: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;

              PVERBOSE2("%p Processing saHpiHotSwapPolicyCancel.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiHotSwapPolicyCancel( session_id, resource_id );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiResourceActiveSet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;

              PVERBOSE2("%p Processing saHpiResourceActiveSet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiResourceActiveSet( session_id, resource_id );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiResourceInactiveSet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;

              PVERBOSE2("%p Processing saHpiResourceInactiveSet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiResourceInactiveSet( session_id, resource_id );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiAutoInsertTimeoutGet: {
              SaHpiSessionIdT session_id;
              SaHpiTimeoutT   timeout;

              PVERBOSE2("%p Processing saHpiAutoInsertTimeoutGet.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id ) < 0 )
                   return eResultError;

              ret = saHpiAutoInsertTimeoutGet( session_id, &timeout );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &timeout );
       }
       break;

       case eFsaHpiAutoInsertTimeoutSet: {
              SaHpiSessionIdT session_id;
              SaHpiTimeoutT   timeout;

              PVERBOSE2("%p Processing saHpiAutoInsertTimeoutSet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &timeout ) < 0 )
                   return eResultError;

              ret = saHpiAutoInsertTimeoutSet( session_id, timeout );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiAutoExtractTimeoutGet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiTimeoutT    timeout;

              PVERBOSE2("%p Processing saHpiAutoExtractTimeoutGet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiAutoExtractTimeoutGet( session_id, resource_id, &timeout );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &timeout );
       }
       break;

       case eFsaHpiAutoExtractTimeoutSet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiTimeoutT    timeout;

              PVERBOSE2("%p Processing saHpiAutoExtractTimeoutSet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &timeout ) < 0 )
                   return eResultError;

              ret = saHpiAutoExtractTimeoutSet( session_id, resource_id, timeout );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiHotSwapStateGet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiHsStateT    state;

              PVERBOSE2("%p Processing saHpiHotSwapStateGet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiHotSwapStateGet( session_id, resource_id, &state );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &state );
       }
       break;

       case eFsaHpiHotSwapActionRequest: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiHsActionT   action;

              PVERBOSE2("%p Processing saHpiHotSwapActionRequest.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &action ) < 0 )
                   return eResultError;

              ret = saHpiHotSwapActionRequest( session_id, resource_id, action );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiHotSwapIndicatorStateGet: {
              SaHpiSessionIdT        session_id;
              SaHpiResourceIdT       resource_id;
              SaHpiHsIndicatorStateT state;

              PVERBOSE2("%p Processing saHpiHotSwapIndicatorStateGet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiHotSwapIndicatorStateGet( session_id, resource_id, &state );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &state );
       }
       break;

       case eFsaHpiHotSwapIndicatorStateSet: {
              SaHpiSessionIdT        session_id;
              SaHpiResourceIdT       resource_id;
              SaHpiHsIndicatorStateT state;

              PVERBOSE2("%p Processing saHpiHotSwapIndicatorStateSet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &state ) < 0 )
                   return eResultError;

              ret = saHpiHotSwapIndicatorStateSet( session_id, resource_id, state );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiParmControl: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiParmActionT action;

              PVERBOSE2("%p Processing saHpiParmControl.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &action ) < 0 )
                   return eResultError;

              ret = saHpiParmControl( session_id, resource_id, action );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiResourceResetStateGet: {
              SaHpiSessionIdT   session_id;
              SaHpiResourceIdT  resource_id;
              SaHpiResetActionT action;

              PVERBOSE2("%p Processing saHpiResourceResetStateGet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiResourceResetStateGet( session_id, resource_id, &action );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &action );
       }
       break;

       case eFsaHpiResourceResetStateSet: {
              SaHpiSessionIdT   session_id;
              SaHpiResourceIdT  resource_id;
              SaHpiResetActionT action;

              PVERBOSE2("%p Processing saHpiResourceResetStateSet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &action ) < 0 )
                   return eResultError;

              ret = saHpiResourceResetStateSet( session_id, resource_id, action );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFsaHpiResourcePowerStateGet:
            {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiPowerStateT state;

              PVERBOSE2("%p Processing saHpiResourcePowerStateGet.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id ) < 0 )
                   return eResultError;

              ret = saHpiResourcePowerStateGet( session_id, resource_id, &state );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &state );
       }
       break;

       case eFsaHpiResourcePowerStateSet: {
              SaHpiSessionIdT  session_id;
              SaHpiResourceIdT resource_id;
              SaHpiPowerStateT state;

              PVERBOSE2("%p Processing saHpiResourcePowerStateGet.\n", thrdid);

              if ( HpiDemarshalRequest3( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &session_id, &resource_id,
                                         &state  ) < 0 )
                   return eResultError;

              ret = saHpiResourcePowerStateSet( session_id, resource_id, state );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
       }
       break;

       case eFoHpiPluginLoad: {
              oHpiTextBufferT buf;

              PVERBOSE2("%p Processing oHpiPluginLoad.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &buf ) < 0 )
                   return eResultError;

              buf.Data[buf.DataLength] = '\0'; // insurance
              ret = oHpiPluginLoad( (char *)buf.Data );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
              result = eResultClose;
       }
       break;

       case eFoHpiPluginUnload: {
              oHpiTextBufferT buf;

              PVERBOSE2("%p Processing oHpiPluginUnload.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &buf ) < 0 )
                   return eResultError;

              buf.Data[buf.DataLength] = '\0'; // insurance
              ret = oHpiPluginUnload( (char *)buf.Data );

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
              result = eResultClose;
       }
       break;

       case eFoHpiPluginInfo: {
              oHpiTextBufferT buf;
              oHpiPluginInfoT info;

              PVERBOSE2("%p Processing oHpiPluginInfo.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &buf ) < 0 )
                   return eResultError;

              buf.Data[buf.DataLength] = '\0'; // insurance
              ret = oHpiPluginInfo( (char *)buf.Data, &info );

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &info );
              result = eResultClose;
       }
       break;

       case eFoHpiPluginGetNext: {
              oHpiTextBufferT buf, retbuf;

              PVERBOSE2("%p Processing oHpiPluginGetNext.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &buf ) < 0 )
                   return eResultError;

              buf.Data[buf.DataLength] = '\0'; // insurance
              ret = oHpiPluginGetNext( (char *)buf.Data, (char *)retbuf.Data,
                                       SAHPI_MAX_TEXT_BUFFER_LENGTH );

              retbuf.DataLength = strlen((char *)retbuf.Data);

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &retbuf );
              result = eResultClose;
       }
       break;

       case eFoHpiHandlerCreateInit: {

              PVERBOSE2("%p Processing oHpiHandlerCreateInit.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &ret ) < 0 )
                   return eResultError;

              if (*ht) {
                      // first free the table entries
                      g_hash_table_foreach(*ht, hashtablefreeentry, NULL);
                      // now destroy the table
                      g_hash_table_destroy(*ht);
                      *ht = NULL;
              }
              *ht = g_hash_table_new(g_str_hash, g_str_equal);
              if (*ht == NULL) {
                      ret = SA_ERR_HPI_OUT_OF_MEMORY;
              }
              else {
                      ret = SA_OK;
              }

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
              result = eResultClose;
       }
       break;

       case eFoHpiHandlerCreateAddTEntry: {
              oHpiTextBufferT key, value;
              char *newkey, *newvalue;

              PVERBOSE2("%p Processing oHpiHandlerCreateAddTEntry.\n", thrdid);

              if ( HpiDemarshalRequest2( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &key, &value ) < 0 )
                   return eResultError;

              newkey = (char *)g_malloc(key.DataLength + 1);
              newvalue = (char *)g_malloc(value.DataLength + 1);
              if (newkey == NULL || newvalue == NULL) {
                      ret = SA_ERR_HPI_OUT_OF_MEMORY;
              }
              else {
                      g_hash_table_insert(*ht, newkey, newvalue);
                      ret = SA_OK;
              }

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
              result = eResultClose;
       }
       break;

       case eFoHpiHandlerCreate: {
              oHpiHandlerIdT id;

              PVERBOSE2("%p Processing oHpiHandlerCreate.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &id ) < 0 )
                   return eResultError;

              ret = oHpiHandlerCreate(*ht, &id);

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &id );
              result = eResultClose;
       }
       break;

       case eFoHpiHandlerDestroy: {
              oHpiHandlerIdT id;

              PVERBOSE2("%p Processing oHpiHandlerDestroy.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &id ) < 0 )
                   return eResultError;

              ret = oHpiHandlerDestroy(id);

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
              result = eResultClose;
       }
       break;

       case eFoHpiHandlerInfo: {
              oHpiHandlerIdT id;
              oHpiHandlerInfoT info;

              PVERBOSE2("%p Processing oHpiHandlerInfo.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &id ) < 0 )
                   return eResultError;

              ret = oHpiHandlerInfo(id, &info);

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &info );
              result = eResultClose;
       }
       break;

       case eFoHpiHandlerGetNext: {
              oHpiHandlerIdT id, next_id;

              PVERBOSE2("%p Processing oHpiHandlerGetNext.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &id ) < 0 )
                   return eResultError;

              ret = oHpiHandlerGetNext(id, &next_id);

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &next_id );
              result = eResultClose;
       }
       break;

       case eFoHpiGlobalParamGet: {
              oHpiGlobalParamT param;

              PVERBOSE2("%p Processing oHpiGlobalParamGet.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &param ) < 0 )
                   return eResultError;

              ret = oHpiGlobalParamGet(&param);

              thrdinst->header.m_len = HpiMarshalReply1( hm, pReq, &ret, &param );
              result = eResultClose;
       }
       break;

       case eFoHpiGlobalParamSet: {
              oHpiGlobalParamT param;

              PVERBOSE2("%p Processing oHpiGlobalParamSet.\n", thrdid);

              if ( HpiDemarshalRequest1( thrdinst->header.m_flags & dMhEndianBit,
                                         hm, pReq, &param ) < 0 )
                   return eResultError;

              ret = oHpiGlobalParamSet(&param);

              thrdinst->header.m_len = HpiMarshalReply0( hm, pReq, &ret );
              result = eResultClose;
       }
       break;

       default:
              PVERBOSE2("%p Function not found\n", thrdid);
              return eResultError;
       }

       // send the reply
       bool wrt_result = thrdinst->WriteMsg(pReq);
       if (wrt_result) {
               PVERBOSE2("%p Socket write failed\n", thrdid);
               return eResultError;
       }

       PVERBOSE3("%p Return code = %d\n", thrdid, ret);

       return result;
}


static void hashtablefreeentry(gpointer key, gpointer value, gpointer data) {
        g_free(key);
        g_free(value);
}








