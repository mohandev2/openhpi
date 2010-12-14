/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2004-2008
 * (C) Copyright Pigeon Point Systems. 2010
 * (C) Copyright Nokia Siemens Networks 2010
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *     W. David Ashley <dashley@us.ibm.com>
 *     David Judkoivcs <djudkovi@us.ibm.com>
 *     Renier Morales <renier@openhpi.org>
 *     Anton Pak <anton.pak@pigeonpoint.com>
 *     Ulrich Kleber <ulikleber@users.sourceforge.net>
 *
 */

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>

#include <SaHpi.h>
#include <oHpi.h>

#include <marshal_hpi.h>
#include <oh_domain.h>
#include <oh_error.h>
#include <oh_init.h>
#include <oh_rpc_params.h>
#include <strmsock.h>


/*--------------------------------------------------------------------*/
/* Local definitions                                                  */
/*--------------------------------------------------------------------*/

extern "C"
{

static void service_thread(gpointer sock_ptr, gpointer /* user_data */);
static SaErrorT ProcessMsg(cHpiMarshal * hm,
                           int rq_byte_order,
                           char * data,
                           uint32_t& data_len,
                           SaHpiSessionIdT& changed_sid);
}

#define CLIENT_TIMEOUT 0  // Unlimited
#define PID_FILE "/var/run/openhpid.pid"

static bool stop_server = false;
static int sock_timeout = CLIENT_TIMEOUT;
static int verbose_flag = 0;

/* verbose macro */
#define PVERBOSE1(msg, ...) if (verbose_flag) dbg(msg, ## __VA_ARGS__)
#define PVERBOSE2(msg, ...) if (verbose_flag) err(msg, ## __VA_ARGS__)
#define PVERBOSE3(msg, ...) if (verbose_flag) printf("CRITICAL: "msg, ## __VA_ARGS__)

/*--------------------------------------------------------------------*/
/* Function to dehash handler cfg for oHpiHandlerInfo                 */
/*--------------------------------------------------------------------*/
static void dehash_handler_cfg(gpointer key, gpointer value, gpointer data)
{
    oHpiHandlerConfigT *hc = (oHpiHandlerConfigT *)data;

    strncpy((char *)hc->Params[hc->NumberOfParams].Name,
            (const char *)key,
            SAHPI_MAX_TEXT_BUFFER_LENGTH);
    strncpy((char *)hc->Params[hc->NumberOfParams].Value,
            (const char *)value,
            SAHPI_MAX_TEXT_BUFFER_LENGTH);

    ++hc->NumberOfParams;
}

/*--------------------------------------------------------------------*/
/* Function: display_help                                             */
/*--------------------------------------------------------------------*/

void display_help(void)
{
    printf("Help for openhpid:\n\n");
    printf("   openhpid -c conf_file [-v] [-p port] [-f pidfile]\n\n");
    printf("   -c conf_file  Sets path/name of the configuration file.\n");
    printf("                 This option is required unless the environment\n");
    printf("                 variable OPENHPI_CONF has been set to a valid\n");
    printf("                 configuration file.\n");
    printf("   -v            This option causes the daemon to display verbose\n");
    printf("                 messages. This option is optional.\n");
    printf("   -p port       Overrides the default listening port (%d) of\n",
           OPENHPI_DEFAULT_DAEMON_PORT);
    printf("                 the daemon. The option is optional.\n");
    printf("   -f pidfile    Overrides the default path/name for the daemon.\n");
    printf("                 pid file. The option is optional.\n");
    printf("   -s seconds    Overrides the default socket read timeout of 30\n");
    printf("                 minutes. The option is optional.\n");
    printf("   -t threads    Sets the maximum number of connection threads.\n");
    printf("                 The default is umlimited. The option is optional.\n");
    printf("   -n            Forces the code to run as a foreground process\n");
    printf("                 and NOT as a daemon. The default is to run as\n");
    printf("                 a daemon. The option is optional.\n\n");
    printf("A typical invocation might be\n\n");
    printf("   ./openhpid -c /etc/openhpi/openhpi.conf\n\n");
}

/*--------------------------------------------------------------------*/
/* PID File Unility Functions                                         */
/*--------------------------------------------------------------------*/
bool check_pidfile(const char *pidfile)
{
    // TODO add more checks here
    if (!pidfile) {
        return false;
    }

    int fd = open(pidfile, O_RDONLY);
    if (fd >= 0) {
        char buf[32];
        memset(buf, 0, sizeof(buf));
        ssize_t len = read(fd, buf, sizeof(buf) - 1);
        if (len < 0) {
            err("Error: Cannot read from PID file.\n");
            return false;
        }
        close(fd);
        int pid = atoi(buf);
        if ((pid > 0) && (pid == getpid() || (kill(pid, 0) < 0))) {
            unlink(pidfile);
        } else {
            err("Error: There is another active OpenHPI daemon.\n");
            return false;
        }
    }

    return true;
}

bool update_pidfile(const char *pidfile)
{
    // TODO add more checks here
    if (!pidfile) {
        return false;
    }

    int fd = open(pidfile, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP);
    if (fd < 0) {
        err("Error: Cannot open PID file.\n");
        return false;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d\n", (int)getpid());
    write(fd, buf, strlen(buf));
    close(fd);

    return true;
}

/*--------------------------------------------------------------------*/
/* Function: daemonize                                                */
/*--------------------------------------------------------------------*/

static bool daemonize(const char *pidfile)
{
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        return false;
    }

    pid_t pid;
    pid = fork();
    if (pid < 0) {
        return false;
    } else if (pid > 0) {
        exit(0);
    }
    // Become the session leader
    setsid();
    // Second fork to make sure we are detached
    // from any controlling terminal.
    pid = fork();
    if (pid < 0) {
        return false;
    } else if (pid > 0) {
        exit(0);
    }

    update_pidfile(pidfile);

    //chdir("/");
    umask(0); // Reset default file permissions

    // Close unneeded inherited file descriptors
    // Keep stdout and stderr open if they already are.
#ifdef NR_OPEN
    for (int i = 3; i < NR_OPEN; i++) {
#else
    for (int i = 3; i < 1024; i++) {
#endif
        close(i);
    }

    return true;
}

/*--------------------------------------------------------------------*/
/* Function: main                                                     */
/*--------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    struct option options[] = {
        {"verbose",   no_argument,       0, 'v'},
        {"nondaemon", no_argument,       0, 'n'},
        {"cfg",       required_argument, 0, 'c'},
        {"port",      required_argument, 0, 'p'},
        {"pidfile",   required_argument, 0, 'f'},
        {"timeout",   required_argument, 0, 's'},
        {"threads",   required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    char *cfgfile    = 0;
    int  port        = OPENHPI_DEFAULT_DAEMON_PORT;
    char *pidfile    = 0;
    int  max_threads = -1;  // unlimited
    bool runasdaemon = true;

    /* get the command line options */
    int c;
    while (1) {
        c = getopt_long(argc, argv, "nvc:p:f:s:t:", options, 0);
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
                cfgfile = g_new0(char, strlen(optarg) + 1);
                strcpy(cfgfile, optarg);
                break;
            case 'p':
                setenv("OPENHPI_DAEMON_PORT", optarg, 1);
                port = atoi(optarg);
                break;
            case 'v':
                verbose_flag = 1;
                break;
            case 'f':
                pidfile = g_new0(char, strlen(optarg) + 1);
                strcpy(pidfile, optarg);
                break;
            case 's':
                sock_timeout = atoi(optarg);
                if (sock_timeout < 0) {
                    sock_timeout = CLIENT_TIMEOUT;
                }
                break;
            case 't':
                max_threads = atoi(optarg);
                if (max_threads <= 0) {
                    max_threads = -1;
                }
                break;
            case 'n':
                runasdaemon = false;
                break;
            case '?':
            default:
                /* they obviously need it */
                display_help();
                exit(-1);
        }
    }
    if (optind < argc) {
        fprintf(stderr, "Error: Unknown command line option specified. Exiting.\n");
        display_help();
        exit(-1);
    }

    // see if we have a valid configuration file
    if ((!cfgfile) || (!g_file_test(cfgfile, G_FILE_TEST_EXISTS))) {
        fprintf(stderr, "Error: Cannot find configuration file. Exiting.\n");
        display_help();
        exit(-1);
    }

    if (!pidfile) {
        pidfile = g_new0(char, strlen(PID_FILE) + 1);
        strcpy(pidfile, PID_FILE);
    }
    // see if we are already running
    if (!check_pidfile(pidfile)) {
        fprintf(stderr, "Error: PID file check failed. Exiting.\n");
        display_help();
        exit(1);
    }
    if (!update_pidfile(pidfile)) {
        fprintf(stderr, "Error: Cannot update PID file. Exiting.\n");
        display_help();
        exit(1);
    }

    if (runasdaemon) {
        if (!daemonize(pidfile)) {
            exit(8);
        }
    }

    if (!g_thread_supported()) {
        g_thread_init(0);
    }

    if (oh_init()) { // Initialize OpenHPI
        err("There was an error initializing OpenHPI. Exiting.\n");
        return 8;
    }

    // create the server socket
    cServerStreamSock * ssock = new cServerStreamSock;
    if (!ssock->Create(port)) {
        err("Error creating server socket. Exiting.\n");
        delete ssock;
        return 8;
    }

    // announce ourselves
    dbg("%s started.\n", argv[0]);
    dbg("OPENHPI_CONF = %s.\n", cfgfile);
    dbg("OPENHPI_DAEMON_PORT = %d.\n\n", port);

    // create the thread pool
    GThreadPool *pool;
    pool = g_thread_pool_new(service_thread, 0, max_threads, FALSE, 0);

    // wait for a connection and then service the connection
    while (!stop_server) {
        cStreamSock * sock = ssock->Accept();
        if (!sock) {
            PVERBOSE1("Error accepting server socket.\n");
            break;
        }
        PVERBOSE1("### Spawning thread to handle connection. ###\n");
        g_thread_pool_push(pool, (gpointer)sock, 0);
    }

    // ensure all threads are complete
    g_thread_pool_free(pool, FALSE, TRUE);

    delete ssock;
    PVERBOSE1("Server socket closed.\n");

    return 0;
}




/*--------------------------------------------------------------------*/
/* Function: service_thread                                           */
/*--------------------------------------------------------------------*/

static void service_thread(gpointer sock_ptr, gpointer /* user_data */)
{
    cStreamSock * sock = (cStreamSock *)sock_ptr;
    gpointer thrdid = g_thread_self();
    // TODO several sids for one connection
    SaHpiSessionIdT my_sid = 0;

    PVERBOSE1("%p Servicing connection.\n", thrdid);

    /* set the read timeout for the socket */
    // TODO
    //sock->SetReadTimeout(sock_timeout);

    PVERBOSE1("### service_thread, thrdid [%p] ###\n", (void *)thrdid);

    while (true) {
        bool     rc;
        char     data[dMaxPayloadLength];
        uint32_t data_len;
        uint8_t  type;
        uint32_t id;
        int      rq_byte_order;

        rc = sock->ReadMsg(type, id, data, data_len, rq_byte_order);
        if (!rc) {
            PVERBOSE3("%p Error or Timeout while reading socket.\n", thrdid);
            break;
        } else if (type != eMhMsg) {
            PVERBOSE3("%p Unsupported message type. Discarding.\n", thrdid);
            sock->WriteMsg(eMhError, id, 0, 0);
        } else {
            cHpiMarshal *hm = HpiMarshalFind(id);
            SaErrorT process_rv;
            SaHpiSessionIdT changed_sid = 0;
            process_rv = ProcessMsg(hm, rq_byte_order, data, data_len, changed_sid);
            if (process_rv != SA_OK) {
                int mr = HpiMarshalReply0(hm, data, &process_rv);
                if (mr < 0) {
                    PVERBOSE2("%p Marshal failed.\n", thrdid);
                    break;
                }
                data_len = (uint32_t)mr;
            }
            rc = sock->WriteMsg(eMhMsg, id, data, data_len);
            if (!rc) {
                PVERBOSE2("%p Socket write failed.\n", thrdid);
                break;
            }
            if ((process_rv == SA_OK) && (changed_sid != 0)) {
                if (id == eFsaHpiSessionOpen) {
                    my_sid = changed_sid;
                } else if (id == eFsaHpiSessionClose) {
                    my_sid = 0;
                    break;
                }
            }
        }
    }

    // if necessary, clean up HPI lib data
    if (my_sid != 0) {
        saHpiSessionClose(my_sid);
    }
    delete sock; // cleanup thread instance data

    PVERBOSE1("%p Connection closed.\n", thrdid);
    // TODO why?
    return; // do NOT use g_thread_exit here!
}


/*----------------------------------------------------------------------------*/
/* RPC Call Processing                                                        */
/*----------------------------------------------------------------------------*/

#define DEMARSHAL_RQ(rq_byte_order, hm, data, iparams) \
{ \
    int mr = HpiDemarshalRequest(rq_byte_order, hm, data, iparams.array); \
    if (mr < 0) { \
        return SA_ERR_HPI_INVALID_PARAMS; \
    } \
}

#define MARSHAL_RP(hm, data, data_len, oparams) \
{ \
    int mr = HpiMarshalReply(hm, data, oparams.const_array); \
    if (mr < 0) { \
        return SA_ERR_HPI_INTERNAL_ERROR; \
    } \
    data_len = (uint32_t)mr; \
}


/*--------------------------------------------------------------------*/
/* Function: ProcessMsg                                                */
/*--------------------------------------------------------------------*/

static SaErrorT ProcessMsg(cHpiMarshal * hm,
                           int rq_byte_order,
                           char * data,
                           uint32_t& data_len,
                           SaHpiSessionIdT& changed_sid)
{
    gpointer thrdid;
    thrdid = g_thread_self();

    PVERBOSE1("%p Processing RPC request %d.\n", thrdid, hm->m_id);

    changed_sid = 0;

    // These vars are used in many places
    SaErrorT             rv;
    SaHpiSessionIdT      sid;
    SaHpiResourceIdT     rid;
    SaHpiSensorNumT      snum;
    SaHpiCtrlNumT        cnum;
    SaHpiIdrIdT          iid;
    SaHpiWatchdogNumT    wnum;
    SaHpiAnnunciatorNumT anum;
    SaHpiDimiNumT        dnum;
    SaHpiFumiNumT        fnum;
    SaHpiRdrT            rdr;
    SaHpiRptEntryT       rpte;

    switch(hm->m_id) {
        case eFsaHpiSessionOpen: {
            SaHpiDomainIdT  did;
            void            *security = 0;

            RpcParams iparams(&did);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSessionOpen(OH_DEFAULT_DOMAIN_ID, &sid, security);
            if (rv == SA_OK) {
                changed_sid = sid;
            }

            RpcParams oparams(&rv, &sid);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiSessionClose: {

            RpcParams iparams(&sid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSessionClose(sid);
            if (rv == SA_OK) {
                changed_sid = sid;
            }

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiDiscover: {

            RpcParams iparams(&sid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiDiscover(sid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiDomainInfoGet: {
            SaHpiDomainInfoT info;

            RpcParams iparams(&sid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiDomainInfoGet(sid, &info);

            RpcParams oparams(&rv, &info);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiDrtEntryGet: {
            SaHpiEntryIdT   eid;
            SaHpiEntryIdT   next_eid;
            SaHpiDrtEntryT  drte;

            RpcParams iparams(&sid, &eid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiDrtEntryGet(sid, eid, &next_eid, &drte);

            RpcParams oparams(&rv, &next_eid, &drte);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiDomainTagSet: {
            SaHpiTextBufferT tag;

            RpcParams iparams(&sid, &tag);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiDomainTagSet(sid, &tag);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiRptEntryGet: {
            SaHpiEntryIdT   eid;
            SaHpiEntryIdT   next_eid;

            RpcParams iparams(&sid, &eid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiRptEntryGet(sid, eid, &next_eid, &rpte);

            RpcParams oparams(&rv, &next_eid, &rpte);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiRptEntryGetByResourceId: {

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiRptEntryGetByResourceId(sid, rid, &rpte);

            RpcParams oparams(&rv, &rpte);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiResourceSeveritySet: {
            SaHpiSeverityT   sev;

            RpcParams iparams(&sid, &rid, &sev);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiResourceSeveritySet(sid, rid, sev);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiResourceTagSet: {
            SaHpiTextBufferT tag;

            RpcParams iparams(&sid, &rid, &tag);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiResourceTagSet(sid, rid, &tag);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiMyEntityPathGet: {
            SaHpiEntityPathT ep;

            RpcParams iparams(&sid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiMyEntityPathGet(sid, &ep);

            RpcParams oparams(&rv, &ep);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiResourceIdGet: {

            RpcParams iparams(&sid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiResourceIdGet(sid, &rid);

            RpcParams oparams(&rv, &rid);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiGetIdByEntityPath: {
            SaHpiEntityPathT   ep;
            SaHpiRdrTypeT      instr_type;
            SaHpiUint32T       instance;
            SaHpiInstrumentIdT instr_id;
            SaHpiUint32T       rpt_update_cnt;

            RpcParams iparams(&sid, &ep, &instr_type, &instance);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiGetIdByEntityPath(sid, ep, instr_type, &instance, &rid, &instr_id, &rpt_update_cnt);

            RpcParams oparams(&rv, &instance, &rid, &instr_id, &rpt_update_cnt);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiGetChildEntityPath: {
            SaHpiEntityPathT parent_ep;
            SaHpiUint32T     instance;
            SaHpiEntityPathT child_ep;
            SaHpiUint32T     rpt_update_cnt;

            RpcParams iparams(&sid, &parent_ep, &instance);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiGetChildEntityPath(sid, parent_ep, &instance, &child_ep, &rpt_update_cnt);

            RpcParams oparams(&rv, &instance, &child_ep, &rpt_update_cnt);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiResourceFailedRemove: {

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiResourceFailedRemove(sid, rid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiEventLogInfoGet: {
            SaHpiEventLogInfoT info;

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiEventLogInfoGet(sid, rid, &info);

            RpcParams oparams(&rv, &info);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiEventLogCapabilitiesGet: {
            SaHpiEventLogCapabilitiesT caps;

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiEventLogCapabilitiesGet(sid, rid, &caps);

            RpcParams oparams(&rv, &caps);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiEventLogEntryGet: {
            SaHpiEventLogEntryIdT eid;
            SaHpiEventLogEntryIdT prev_eid;
            SaHpiEventLogEntryIdT next_eid;
            SaHpiEventLogEntryT   ele;

            RpcParams iparams(&sid, &rid, &eid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiEventLogEntryGet(sid, rid, eid, &prev_eid, &next_eid, &ele, &rdr, &rpte);

            RpcParams oparams(&rv, &prev_eid, &next_eid, &ele, &rdr, &rpte);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiEventLogEntryAdd: {
            SaHpiEventT evt;

            RpcParams iparams(&sid, &rid, &evt);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiEventLogEntryAdd(sid, rid, &evt);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiEventLogClear: {

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiEventLogClear(sid, rid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiEventLogTimeGet: {
            SaHpiTimeT time;

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiEventLogTimeGet(sid, rid, &time);

            RpcParams oparams(&rv, &time);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiEventLogTimeSet: {
            SaHpiTimeT time;

            RpcParams iparams(&sid, &rid, &time);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiEventLogTimeSet(sid, rid, time);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiEventLogStateGet: {
            SaHpiBoolT enable;

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiEventLogStateGet(sid, rid, &enable);

            RpcParams oparams(&rv, &enable);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiEventLogStateSet: {
            SaHpiBoolT enable;

            RpcParams iparams(&sid, &rid, &enable);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiEventLogStateSet(sid, rid, enable);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiEventLogOverflowReset: {

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiEventLogOverflowReset(sid, rid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiSubscribe: {

            RpcParams iparams(&sid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSubscribe(sid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiUnsubscribe: {

            RpcParams iparams(&sid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiUnsubscribe(sid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiEventGet: {
            SaHpiTimeoutT        timeout;
            SaHpiEventT          evt;
            SaHpiEvtQueueStatusT status;

            RpcParams iparams(&sid, &timeout);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiEventGet(sid, timeout, &evt, &rdr, &rpte, &status);

            RpcParams oparams(&rv, &evt, &rdr, &rpte, &status);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiEventAdd: {
            SaHpiEventT evt;

            RpcParams iparams(&sid, &evt);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiEventAdd(sid, &evt);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAlarmGetNext: {
            SaHpiSeverityT sev;
            SaHpiBoolT     unack;
            SaHpiAlarmT    alarm;

            RpcParams iparams(&sid, &sev, &unack, &alarm);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAlarmGetNext(sid, sev, unack, &alarm);

            RpcParams oparams(&rv, &alarm);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAlarmGet: {
            SaHpiAlarmIdT aid;
            SaHpiAlarmT   alarm;

            RpcParams iparams(&sid, &aid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAlarmGet(sid, aid, &alarm);

            RpcParams oparams(&rv, &alarm);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAlarmAcknowledge: {
            SaHpiAlarmIdT  aid;
            SaHpiSeverityT sev;

            RpcParams iparams(&sid, &aid, &sev);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAlarmAcknowledge(sid, aid, sev);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAlarmAdd: {
            SaHpiAlarmT alarm;

            RpcParams iparams(&sid, &alarm);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAlarmAdd(sid, &alarm);

            RpcParams oparams(&rv, &alarm);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAlarmDelete: {
            SaHpiAlarmIdT  aid;
            SaHpiSeverityT sev;

            RpcParams iparams(&sid, &aid, &sev);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAlarmDelete(sid, aid, sev);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiRdrGet: {
            SaHpiEntryIdT eid;
            SaHpiEntryIdT next_eid;

            RpcParams iparams(&sid, &rid, &eid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiRdrGet(sid, rid, eid, &next_eid, &rdr);

            RpcParams oparams(&rv, &next_eid, &rdr);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiRdrGetByInstrumentId: {
            SaHpiRdrTypeT      rdr_type;
            SaHpiInstrumentIdT instr_id;

            RpcParams iparams(&sid, &rid, &rdr_type, &instr_id);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiRdrGetByInstrumentId(sid, rid, rdr_type, instr_id, &rdr);

            RpcParams oparams(&rv, &rdr);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiRdrUpdateCountGet: {
            SaHpiUint32T rdr_update_cnt;

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiRdrUpdateCountGet(sid, rid, &rdr_update_cnt);

            RpcParams oparams(&rv, &rdr_update_cnt);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiSensorReadingGet: {
            SaHpiSensorReadingT reading;
            SaHpiEventStateT    state;

            RpcParams iparams(&sid, &rid, &snum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSensorReadingGet(sid, rid, snum, &reading, &state);

            RpcParams oparams(&rv, &reading, &state);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiSensorThresholdsGet: {
            SaHpiSensorThresholdsT tholds;

            RpcParams iparams(&sid, &rid, &snum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSensorThresholdsGet(sid, rid, snum, &tholds);

            RpcParams oparams(&rv, &tholds);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiSensorThresholdsSet: {
            SaHpiSensorThresholdsT tholds;

            RpcParams iparams(&sid, &rid, &snum, &tholds);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSensorThresholdsSet(sid, rid, snum, &tholds);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiSensorTypeGet: {
            SaHpiSensorTypeT    type;
            SaHpiEventCategoryT cat;

            RpcParams iparams(&sid, &rid, &snum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSensorTypeGet(sid, rid, snum, &type, &cat);

            RpcParams oparams(&rv, &type, &cat);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiSensorEnableGet: {
            SaHpiBoolT enabled;

            RpcParams iparams(&sid, &rid, &snum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSensorEnableGet(sid, rid, snum, &enabled);

            RpcParams oparams(&rv, &enabled);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiSensorEnableSet: {
            SaHpiBoolT enabled;

            RpcParams iparams(&sid, &rid, &snum, &enabled);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSensorEnableSet(sid, rid, snum, enabled);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiSensorEventEnableGet: {
            SaHpiBoolT enabled;

            RpcParams iparams(&sid, &rid, &snum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSensorEventEnableGet(sid, rid, snum, &enabled);

            RpcParams oparams(&rv, &enabled);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiSensorEventEnableSet: {
            SaHpiBoolT enabled;

            RpcParams iparams(&sid, &rid, &snum, &enabled);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSensorEventEnableSet(sid, rid, snum, enabled);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiSensorEventMasksGet: {
            SaHpiEventStateT amask;
            SaHpiEventStateT dmask;

            RpcParams iparams(&sid, &rid, &snum, &amask, &dmask);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSensorEventMasksGet(sid, rid, snum, &amask, &dmask);

            RpcParams oparams(&rv, &amask, &dmask);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiSensorEventMasksSet: {
            SaHpiSensorEventMaskActionT action;
            SaHpiEventStateT            amask;
            SaHpiEventStateT            dmask;

            RpcParams iparams(&sid, &rid, &snum, &action, &amask, &dmask);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiSensorEventMasksSet(sid, rid, snum, action, amask, dmask);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiControlTypeGet: {
            SaHpiCtrlTypeT type;

            RpcParams iparams(&sid, &rid, &cnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiControlTypeGet(sid, rid, cnum, &type);

            RpcParams oparams(&rv, &type);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiControlGet: {
            SaHpiCtrlModeT  mode;
            SaHpiCtrlStateT state;

            RpcParams iparams(&sid, &rid, &cnum, &state);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiControlGet(sid, rid, cnum, &mode, &state);

            RpcParams oparams(&rv, &mode, &state);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiControlSet: {
            SaHpiCtrlModeT  mode;
            SaHpiCtrlStateT state;

            RpcParams iparams(&sid, &rid, &cnum, &mode, &state);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiControlSet(sid, rid, cnum, mode, &state);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiIdrInfoGet: {
            SaHpiIdrInfoT info;

            RpcParams iparams(&sid, &rid, &iid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiIdrInfoGet(sid, rid, iid, &info);

            RpcParams oparams(&rv, &info);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiIdrAreaHeaderGet: {
            SaHpiIdrAreaTypeT   area;
            SaHpiEntryIdT       aid;
            SaHpiEntryIdT       next_aid;
            SaHpiIdrAreaHeaderT hdr;

            RpcParams iparams(&sid, &rid, &iid, &area, &aid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiIdrAreaHeaderGet(sid, rid, iid, area, aid, &next_aid, &hdr);

            RpcParams oparams(&rv, &next_aid, &hdr);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiIdrAreaAdd: {
            SaHpiIdrAreaTypeT area;
            SaHpiEntryIdT     aid;

            RpcParams iparams(&sid, &rid, &iid, &area);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiIdrAreaAdd(sid, rid, iid, area, &aid);

            RpcParams oparams(&rv, &aid);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiIdrAreaAddById: {
            SaHpiIdrAreaTypeT type;
            SaHpiEntryIdT     aid;

            RpcParams iparams(&sid, &rid, &iid, &type, &aid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiIdrAreaAddById(sid, rid, iid, type, aid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiIdrAreaDelete: {
            SaHpiEntryIdT aid;

            RpcParams iparams(&sid, &rid, &iid, &aid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiIdrAreaDelete(sid, rid, iid, aid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiIdrFieldGet: {
            SaHpiEntryIdT      aid;
            SaHpiIdrFieldTypeT type;
            SaHpiEntryIdT      fid;
            SaHpiEntryIdT      next_fid;
            SaHpiIdrFieldT     field;

            RpcParams iparams(&sid, &rid, &iid, &aid, &type, &fid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiIdrFieldGet(sid, rid, iid, aid, type, fid, &next_fid, &field);

            RpcParams oparams(&rv, &next_fid, &field);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiIdrFieldAdd: {
            SaHpiIdrFieldT field;

            RpcParams iparams(&sid, &rid, &iid, &field);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiIdrFieldAdd(sid, rid, iid, &field);

            RpcParams oparams(&rv, &field);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiIdrFieldAddById: {
            SaHpiIdrFieldT field;

            RpcParams iparams(&sid, &rid, &iid, &field);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiIdrFieldAddById(sid, rid, iid, &field);

            RpcParams oparams(&rv, &field);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiIdrFieldSet: {
            SaHpiIdrFieldT field;

            RpcParams iparams(&sid, &rid, &iid, &field);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiIdrFieldSet(sid, rid, iid, &field);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiIdrFieldDelete: {
            SaHpiEntryIdT aid;
            SaHpiEntryIdT fid;

            RpcParams iparams(&sid, &rid, &iid, &aid, &fid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiIdrFieldDelete(sid, rid, iid, aid, fid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiWatchdogTimerGet: {
            SaHpiWatchdogT wdt;

            RpcParams iparams(&sid, &rid, &wnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiWatchdogTimerGet(sid, rid, wnum, &wdt);

            RpcParams oparams(&rv, &wdt);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiWatchdogTimerSet: {
            SaHpiWatchdogT wdt;

            RpcParams iparams(&sid, &rid, &wnum, &wdt);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiWatchdogTimerSet(sid, rid, wnum, &wdt);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiWatchdogTimerReset: {

            RpcParams iparams(&sid, &rid, &wnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiWatchdogTimerReset(sid, rid, wnum);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAnnunciatorGetNext: {
            SaHpiSeverityT     sev;
            SaHpiBoolT         unack;
            SaHpiAnnouncementT ann;

            RpcParams iparams(&sid, &rid, &anum, &sev, &unack, &ann);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAnnunciatorGetNext(sid, rid, anum, sev, unack, &ann);

            RpcParams oparams(&rv, &ann);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAnnunciatorGet: {
            SaHpiEntryIdT      eid;
            SaHpiAnnouncementT ann;

            RpcParams iparams(&sid, &rid, &anum, &eid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAnnunciatorGet(sid, rid, anum, eid, &ann);

            RpcParams oparams(&rv, &ann);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAnnunciatorAcknowledge: {
            SaHpiEntryIdT  eid;
            SaHpiSeverityT sev;

            RpcParams iparams(&sid, &rid, &anum, &eid, &sev);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAnnunciatorAcknowledge(sid, rid, anum, eid, sev);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAnnunciatorAdd: {
            SaHpiAnnouncementT ann;

            RpcParams iparams(&sid, &rid, &anum, &ann);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAnnunciatorAdd(sid, rid, anum, &ann);

            RpcParams oparams(&rv, &ann);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAnnunciatorDelete: {
            SaHpiEntryIdT  eid;
            SaHpiSeverityT sev;

            RpcParams iparams(&sid, &rid, &anum, &eid, &sev);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAnnunciatorDelete(sid, rid, anum, eid, sev);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAnnunciatorModeGet: {
            SaHpiAnnunciatorModeT mode;

            RpcParams iparams(&sid, &rid, &anum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAnnunciatorModeGet(sid, rid, anum, &mode);

            RpcParams oparams(&rv, &mode);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAnnunciatorModeSet: {
            SaHpiAnnunciatorModeT mode;

            RpcParams iparams(&sid, &rid, &anum, &mode);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAnnunciatorModeSet(sid, rid, anum, mode);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiDimiInfoGet: {
            SaHpiDimiInfoT info;

            RpcParams iparams(&sid, &rid, &dnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiDimiInfoGet(sid, rid, dnum, &info);

            RpcParams oparams(&rv, &info);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiDimiTestInfoGet: {
            SaHpiDimiTestNumT tnum;
            SaHpiDimiTestT    test;

            RpcParams iparams(&sid, &rid, &dnum, &tnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiDimiTestInfoGet(sid, rid, dnum, tnum, &test);

            RpcParams oparams(&rv, &test);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiDimiTestReadinessGet: {
            SaHpiDimiTestNumT tnum;
            SaHpiDimiReadyT   ready;

            RpcParams iparams(&sid, &rid, &dnum, &tnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiDimiTestReadinessGet(sid, rid, dnum, tnum, &ready);

            RpcParams oparams(&rv, &ready);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiDimiTestStart: {
            SaHpiDimiTestNumT                tnum;
            SaHpiDimiTestVariableParamsListT pl;
            SaHpiUint8T&                     ntestparams = pl.NumberOfParams;
            SaHpiDimiTestVariableParamsT*&   testparams  = pl.ParamsList;

            RpcParams iparams(&sid, &rid, &dnum, &tnum, &pl);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiDimiTestStart(sid, rid, dnum, tnum, ntestparams, testparams);
            g_free(pl.ParamsList);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiDimiTestCancel: {
            SaHpiDimiTestNumT tnum;

            RpcParams iparams(&sid, &rid, &dnum, &tnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiDimiTestCancel(sid, rid, dnum, tnum);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiDimiTestStatusGet: {
            SaHpiDimiTestNumT              tnum;
            SaHpiDimiTestPercentCompletedT percent;
            SaHpiDimiTestRunStatusT        status;

            RpcParams iparams(&sid, &rid, &dnum, &tnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiDimiTestStatusGet(sid, rid, dnum, tnum, &percent, &status);

            RpcParams oparams(&rv, &percent, &status);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiDimiTestResultsGet: {
            SaHpiDimiTestNumT     tnum;
            SaHpiDimiTestResultsT results;

            RpcParams iparams(&sid, &rid, &dnum, &tnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiDimiTestResultsGet(sid, rid, dnum, tnum, &results);

            RpcParams oparams(&rv, &results);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiSpecInfoGet: {
            SaHpiFumiSpecInfoT info;

            RpcParams iparams(&sid, &rid, &fnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiSpecInfoGet(sid, rid, fnum, &info);

            RpcParams oparams(&rv, &info);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiServiceImpactGet: {
            SaHpiFumiServiceImpactDataT impact;

            RpcParams iparams(&sid, &rid, &fnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiServiceImpactGet(sid, rid, fnum, &impact);

            RpcParams oparams(&rv, &impact);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiSourceSet: {
            SaHpiBankNumT    bnum;
            SaHpiTextBufferT uri;

            RpcParams iparams(&sid, &rid, &fnum, &bnum, &uri);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiSourceSet(sid, rid, fnum, bnum, &uri);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiSourceInfoValidateStart: {
            SaHpiBankNumT bnum;

            RpcParams iparams(&sid, &rid, &fnum, &bnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiSourceInfoValidateStart(sid, rid, fnum, bnum);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiSourceInfoGet: {
            SaHpiBankNumT        bnum;
            SaHpiFumiSourceInfoT info;

            RpcParams iparams(&sid, &rid, &fnum, &bnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiSourceInfoGet(sid, rid, fnum, bnum, &info);

            RpcParams oparams(&rv, &info);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiSourceComponentInfoGet: {
            SaHpiBankNumT           bnum;
            SaHpiEntryIdT           eid;
            SaHpiEntryIdT           next_eid;
            SaHpiFumiComponentInfoT info;


            RpcParams iparams(&sid, &rid, &fnum, &bnum, &eid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiSourceComponentInfoGet(sid, rid, fnum, bnum, eid, &next_eid, &info);

            RpcParams oparams(&rv, &next_eid, &info);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiTargetInfoGet: {
            SaHpiBankNumT      bnum;
            SaHpiFumiBankInfoT info;

            RpcParams iparams(&sid, &rid, &fnum, &bnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiTargetInfoGet(sid, rid, fnum, bnum, &info);

            RpcParams oparams(&rv, &info);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiTargetComponentInfoGet: {
            SaHpiBankNumT           bnum;
            SaHpiEntryIdT           eid;
            SaHpiEntryIdT           next_eid;
            SaHpiFumiComponentInfoT info;

            RpcParams iparams(&sid, &rid, &fnum, &bnum, &eid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiTargetComponentInfoGet(sid, rid, fnum, bnum, eid, &next_eid, &info);

            RpcParams oparams(&rv, &next_eid, &info);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiLogicalTargetInfoGet: {
            SaHpiFumiLogicalBankInfoT info;

            RpcParams iparams(&sid, &rid, &fnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiLogicalTargetInfoGet(sid, rid, fnum, &info);

            RpcParams oparams(&rv, &info);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiLogicalTargetComponentInfoGet: {
            SaHpiEntryIdT                  eid;
            SaHpiEntryIdT                  next_eid;
            SaHpiFumiLogicalComponentInfoT info;

            RpcParams iparams(&sid, &rid, &fnum, &eid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiLogicalTargetComponentInfoGet(sid, rid, fnum, eid, &next_eid, &info);

            RpcParams oparams(&rv, &next_eid, &info);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiBackupStart: {

            RpcParams iparams(&sid, &rid, &fnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiBackupStart(sid, rid, fnum);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiBankBootOrderSet: {
            SaHpiBankNumT bnum;
            SaHpiUint32T  pos;

            RpcParams iparams(&sid, &rid, &fnum, &bnum, &pos);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiBankBootOrderSet(sid, rid, fnum, bnum, pos);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiBankCopyStart: {
            SaHpiBankNumT src_bnum;
            SaHpiBankNumT dst_bnum;

            RpcParams iparams(&sid, &rid, &fnum, &src_bnum, &dst_bnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiBankCopyStart(sid, rid, fnum, src_bnum, dst_bnum);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiInstallStart: {
            SaHpiBankNumT bnum;

            RpcParams iparams(&sid, &rid, &fnum, &bnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiInstallStart(sid, rid, fnum, bnum);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiUpgradeStatusGet: {
            SaHpiBankNumT           bnum;
            SaHpiFumiUpgradeStatusT status;

            RpcParams iparams(&sid, &rid, &fnum, &bnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiUpgradeStatusGet(sid, rid, fnum, bnum, &status);

            RpcParams oparams(&rv, &status);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiTargetVerifyStart: {
            SaHpiBankNumT bnum;

            RpcParams iparams(&sid, &rid, &fnum, &bnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiTargetVerifyStart(sid, rid, fnum, bnum);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiTargetVerifyMainStart: {

            RpcParams iparams(&sid, &rid, &fnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiTargetVerifyMainStart(sid, rid, fnum);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiUpgradeCancel: {
            SaHpiBankNumT bnum;

            RpcParams iparams(&sid, &rid, &fnum, &bnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiUpgradeCancel(sid, rid, fnum, bnum);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiAutoRollbackDisableGet: {
            SaHpiBoolT disable;

            RpcParams iparams(&sid, &rid, &fnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiAutoRollbackDisableGet(sid, rid, fnum, &disable);

            RpcParams oparams(&rv, &disable);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiAutoRollbackDisableSet: {
            SaHpiBoolT disable;

            RpcParams iparams(&sid, &rid, &fnum, &disable);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiAutoRollbackDisableSet(sid, rid, fnum, disable);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiRollbackStart: {

            RpcParams iparams(&sid, &rid, &fnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiRollbackStart(sid, rid, fnum);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiActivate: {

            RpcParams iparams(&sid, &rid, &fnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiActivate(sid, rid, fnum);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiActivateStart: {
            SaHpiBoolT logical;

            RpcParams iparams(&sid, &rid, &fnum, &logical);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiActivateStart(sid, rid, fnum, logical);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiFumiCleanup: {
            SaHpiBankNumT bnum;

            RpcParams iparams(&sid, &rid, &fnum, &bnum);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiFumiCleanup(sid, rid, fnum, bnum);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiHotSwapPolicyCancel: {

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiHotSwapPolicyCancel(sid, rid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiResourceActiveSet: {

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiResourceActiveSet(sid, rid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiResourceInactiveSet: {

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiResourceInactiveSet(sid, rid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAutoInsertTimeoutGet: {
            SaHpiTimeoutT timeout;

            RpcParams iparams(&sid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAutoInsertTimeoutGet(sid, &timeout);

            RpcParams oparams(&rv, &timeout);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAutoInsertTimeoutSet: {
            SaHpiTimeoutT timeout;

            RpcParams iparams(&sid, &timeout);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAutoInsertTimeoutSet(sid, timeout);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAutoExtractTimeoutGet: {
            SaHpiTimeoutT timeout;

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAutoExtractTimeoutGet(sid, rid, &timeout);

            RpcParams oparams(&rv, &timeout);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiAutoExtractTimeoutSet: {
            SaHpiTimeoutT timeout;

            RpcParams iparams(&sid, &rid, &timeout);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiAutoExtractTimeoutSet(sid, rid, timeout);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiHotSwapStateGet: {
            SaHpiHsStateT state;

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiHotSwapStateGet(sid, rid, &state);

            RpcParams oparams(&rv, &state);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiHotSwapActionRequest: {
            SaHpiHsActionT action;

            RpcParams iparams(&sid, &rid, &action);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiHotSwapActionRequest(sid, rid, action);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiHotSwapIndicatorStateGet: {
            SaHpiHsIndicatorStateT state;

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiHotSwapIndicatorStateGet(sid, rid, &state);

            RpcParams oparams(&rv, &state);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiHotSwapIndicatorStateSet: {
            SaHpiHsIndicatorStateT state;

            RpcParams iparams(&sid, &rid, &state);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiHotSwapIndicatorStateSet(sid, rid, state);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiParmControl: {
            SaHpiParmActionT action;

            RpcParams iparams(&sid, &rid, &action);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiParmControl(sid, rid, action);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiResourceLoadIdGet: {
            SaHpiLoadIdT lid;

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiResourceLoadIdGet(sid, rid, &lid);

            RpcParams oparams(&rv, &lid);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiResourceLoadIdSet: {
            SaHpiLoadIdT lid;

            RpcParams iparams(&sid, &rid, &lid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiResourceLoadIdSet(sid, rid, &lid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiResourceResetStateGet: {
            SaHpiResetActionT action;

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiResourceResetStateGet(sid, rid, &action);

            RpcParams oparams(&rv, &action);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiResourceResetStateSet: {
            SaHpiResetActionT action;

            RpcParams iparams(&sid, &rid, &action);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiResourceResetStateSet(sid, rid, action);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiResourcePowerStateGet: {
            SaHpiPowerStateT state;

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiResourcePowerStateGet(sid, rid, &state);

            RpcParams oparams(&rv, &state);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFsaHpiResourcePowerStateSet: {
            SaHpiPowerStateT state;

            RpcParams iparams(&sid, &rid, &state);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = saHpiResourcePowerStateSet(sid, rid, state);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFoHpiHandlerCreate: {
            oHpiHandlerIdT     hid;
            oHpiHandlerConfigT cfg;

            RpcParams iparams(&sid, &cfg);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            GHashTable *cfg_tbl;
            cfg_tbl = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
            for (int n = 0; n < cfg.NumberOfParams; n++) {
                g_hash_table_insert(cfg_tbl,
                                    g_strdup((const gchar *)cfg.Params[n].Name),
                                    g_strdup((const gchar *)cfg.Params[n].Value));
            }
            g_free(cfg.Params);

            rv = oHpiHandlerCreate(sid, cfg_tbl, &hid);
            g_hash_table_destroy(cfg_tbl);

            RpcParams oparams(&rv, &hid);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFoHpiHandlerDestroy: {
            oHpiHandlerIdT hid;

            RpcParams iparams(&sid, &hid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = oHpiHandlerDestroy(sid, hid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFoHpiHandlerInfo: {
            oHpiHandlerIdT hid;
            oHpiHandlerInfoT info;
            oHpiHandlerConfigT cfg;
            GHashTable *cfg_tbl;

            RpcParams iparams(&sid, &hid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            cfg_tbl = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

            rv = oHpiHandlerInfo(sid, hid, &info, cfg_tbl);

            cfg.NumberOfParams = 0;
            cfg.Params = g_new0(oHpiHandlerConfigParamT, g_hash_table_size(cfg_tbl));
            // add each hash tbl entry to the marshable handler_cfg
            g_hash_table_foreach(cfg_tbl, dehash_handler_cfg, &cfg);

            RpcParams oparams(&rv, &info, &cfg);
            MARSHAL_RP(hm, data, data_len, oparams);
// TODO memory leak
            // cleanup
            g_hash_table_destroy(cfg_tbl);
        }
        break;

        case eFoHpiHandlerGetNext: {
            oHpiHandlerIdT hid, next_hid;

            RpcParams iparams(&sid, &hid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = oHpiHandlerGetNext(sid, hid, &next_hid);

            RpcParams oparams(&rv, &next_hid);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFoHpiHandlerFind: {
            oHpiHandlerIdT hid;

            RpcParams iparams(&sid, &rid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = oHpiHandlerFind(sid, rid, &hid);

            RpcParams oparams(&rv, &hid);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFoHpiHandlerRetry: {
            oHpiHandlerIdT hid;

            RpcParams iparams(&sid, &hid);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = oHpiHandlerRetry(sid, hid);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFoHpiGlobalParamGet: {
            oHpiGlobalParamT param;

            RpcParams iparams(&sid, &param);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = oHpiGlobalParamGet(sid, &param);

            RpcParams oparams(&rv, &param);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFoHpiGlobalParamSet: {
            oHpiGlobalParamT param;

            RpcParams iparams(&sid, &param);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = oHpiGlobalParamSet(sid, &param);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        case eFoHpiInjectEvent: {
            oHpiHandlerIdT  hid;
            SaHpiEventT     evt;

            RpcParams iparams(&sid, &hid, &evt, &rpte, &rdr);
            DEMARSHAL_RQ(rq_byte_order, hm, data, iparams);

            rv = oHpiInjectEvent(sid, hid, &evt, &rpte, &rdr);

            RpcParams oparams(&rv);
            MARSHAL_RP(hm, data, data_len, oparams);
        }
        break;

        default:
            PVERBOSE2("%p Function not found\n", thrdid);
            return SA_ERR_HPI_UNSUPPORTED_API; 
    }

    PVERBOSE1("%p Return code = %d\n", thrdid, rv);

    return SA_OK;
}

