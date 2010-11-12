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
 *      W. David Ashley <dashley@us.ibm.com>
 *      Anton Pak <anton.pak@pigeonpoint.com>
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <errno.h>
#include <glib.h>

#include <oh_error.h>

#include "strmsock.h"


// Local definitions
#ifndef INADDR_NONE
#define INADDR_NONE  0xffffffff
#endif


/*--------------------------------------------------------------------*/
/* Stream Sockets base class methods                   */
/*--------------------------------------------------------------------*/

void strmsock::Close(void)
{
	if (fOpen) {
		close(s);
		fOpen = FALSE;
	}
	errcode = 0;
	return;
}


int strmsock::GetErrcode(void)
{
	return(errcode);
}


void strmsock::SetDomain(
        int lNewDomain)                 // the new domain
{
	domain = lNewDomain;
	errcode = 0;
	return;
}


void strmsock::SetProtocol(
        int lNewProtocol)               // the new protocol
{
	protocol = lNewProtocol;
	errcode = 0;
	return;
}


void strmsock::SetType(
        int lNewType)                   // the new type
{
	type = lNewType;
	errcode = 0;
	return;
}


void strmsock::SetReadTimeout(
        int seconds)                    // the timeout
{
        struct timeval tv;

	tv.tv_sec = seconds;
	tv.tv_usec = 0;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	return;
}

bool strmsock::WriteMsg(const void *request)
{
	unsigned char data[dMaxMessageLength];
	unsigned int l = sizeof(cMessageHeader) + header.m_len;

      dbg("Message body length = %d.\n", header.m_len);
        errcode = 0;
	if (!fOpen) {
                err("Socket not open.\n");
		return true;
	}

	if (l > dMaxMessageLength) {
                err("Message too large.\n");
		return true;
	}

	memcpy(&data[0], &header, sizeof(cMessageHeader));
        if (request != NULL) {
                memcpy(&data[sizeof(cMessageHeader)], request, header.m_len);
        }
//      dbg("Size of message header is %d\n", sizeof(cMessageHeader));
//      dbg("Buffer header address is %p\n", &data[0]);
//      dbg("Buffer request address is %p\n", &data[sizeof(cMessageHeader)]);
//      dbg("Write request buffer (%d bytes) is\n", header.m_len);
//      for (unsigned int i = 0; i < header.m_len; i++) {
//              dbg("%02x ", *((unsigned char *)request + i));
//      }
//      dbg("\n");
//      dbg("Write buffer (%d bytes) is\n", l);
//      for (int i = 0; i < l; i++) {
//              dbg("%02x ", (unsigned char)data[i]);
//      }
//      dbg("\n");

	int rv = write(s, data, l);

	if ( (unsigned int)rv != l ) {
		return true;
	}

	return false;
}

bool strmsock::ReadMsg(char *data)
{
        errcode = 0;
	if (!fOpen) {
		return true;
	}

	int len = read( s, data, dMaxMessageLength);

	if (len < 0) {
                errcode = errno;
                err("Reading from socket returned and error: %d\n", errcode); // Debug
		return true;
	} else if (len == 0) {	//connection has been aborted by the peer
		Close();
		err("Connection has been aborted\n"); // Debug
		return true;
	} else if (len < (int)sizeof(cMessageHeader)) {
		err("Got corrupted header?\n"); // Debug
		return true;
	}
	memcpy(&header, data, sizeof(cMessageHeader));
    remote_byte_order = ( ( header.m_flags & dMhEndianBit ) != 0 ) ? G_LITTLE_ENDIAN : G_BIG_ENDIAN;
        // swap id and len if nessesary in the reply header
	if (remote_byte_order != G_BYTE_ORDER) {
		header.m_id  = GUINT32_SWAP_LE_BE(header.m_id);
		header.m_len = GUINT32_SWAP_LE_BE(header.m_len);
	}
	//dbg("header.m_flags: 0x%x\n", header.m_flags);

	if ( (header.m_flags >> 4) != dMhVersion ) {
		err("Wrong version? 0x%x != 0x%x\n", header.m_flags, dMhVersion); // Debug
		return true;
	}
//      dbg("Read buffer (%d bytes) is\n", len);
//      for (int i = 0; i < len; i++) {
//              dbg("%02x ", (unsigned char)data[i]);
//      }
//      dbg("\n");

	return false;
}

void strmsock::MessageHeaderInit(tMessageType mtype, unsigned char flags,
	               	         unsigned int id, unsigned int len )
{
	header.m_type    = mtype;
	header.m_flags   = flags;

        // set version
	header.m_flags &= 0x0f;
	header.m_flags |= dMhVersion << 4;

    // set byte order
    header.m_flags &= ~dMhEndianBit;
    if ( G_BYTE_ORDER == G_LITTLE_ENDIAN ) {
        header.m_flags |= dMhEndianBit;
    }

	header.m_id = id;
	header.m_len = len;
}

/*--------------------------------------------------------------------*/
/* Stream Sockets client class methods                                */
/*--------------------------------------------------------------------*/


cstrmsock::cstrmsock()
{
	fOpen = FALSE;
	ulBufSize = 4096;
	domain = AF_INET;
	type = SOCK_STREAM;
	protocol = 0;
	s = -1;
	next = NULL;
}


cstrmsock::~cstrmsock()
{
	if (fOpen)
		Close();
}


bool cstrmsock::Open(
		const char * pszHost,		// the remote host
		unsigned short lPort)		// the remote port
{
	struct sockaddr_in  addr;		// address structure
	struct hostent     *phe;		// pointer to a host entry

        // get a socket
	s = socket(domain, type, protocol);
	if (s == -1) {
		errcode = errno;
		return(TRUE);
	}
        // convert the host entry/name to an address
	phe = gethostbyname(pszHost);
	if (phe)
		memcpy((char *) &addr.sin_addr, phe -> h_addr, phe -> h_length);
	else
		addr.sin_addr.s_addr = inet_addr(pszHost);
	if (addr.sin_addr.s_addr == INADDR_NONE) {
		errcode = 67;  // bad network name
		close(s);
		return(TRUE);
	}
        // connect to the remote host
	addr.sin_family = domain;
	addr.sin_port = htons(lPort);
	errcode = connect(s, (struct sockaddr *) &addr, sizeof(addr));
	if (errcode == -1) {
		errcode = errno;
		close(s);
		return(TRUE);
	}

	errcode = 0;
	fOpen = TRUE;
	return(FALSE);
}

bool cstrmsock::EnableKeepAliveProbes( int keepalive_time,
                                       int keepalive_intvl,
                                       int keepalive_probes )
{
#ifdef __linux__
    int rc;
    int optval;

    optval = 1;
    rc = setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    if ( rc != 0 ) {
        err( "failed to set SO_KEEPALIVE option, errno = %d\n", errno );
        return true;
    }
    optval = keepalive_time;
    rc = setsockopt(s, SOL_TCP, TCP_KEEPIDLE, &optval, sizeof(optval));
    if ( rc != 0 ) {
        err( "failed to set TCP_KEEPIDLE option, errno = %d\n", errno );
        return true;
    }
    optval = keepalive_intvl;
    rc = setsockopt(s, SOL_TCP, TCP_KEEPINTVL, &optval, sizeof(optval));
    if ( rc != 0 ) {
        err( "failed to set TCP_KEEPINTVL option, errno = %d\n", errno );
        return true;
    }
    optval = keepalive_probes;
    rc = setsockopt(s, SOL_TCP, TCP_KEEPCNT, &optval, sizeof(optval));
    if ( rc != 0 ) {
        err( "failed to set TCP_KEEPCNT option, errno = %d\n", errno );
        return true;
    }

    return false;

#else

    err( "TCP Keep-Alive Probes are not supported\n" );

    return true;

#endif /* __linux__ */
}


/*--------------------------------------------------------------------*/
/* Stream Sockets server class methods                 */
/*--------------------------------------------------------------------*/


sstrmsock::sstrmsock()
{
	fOpen = FALSE;
	fOpenS = FALSE;
	ulBufSize = 4096;
	domain = AF_INET;
	type = SOCK_STREAM;
	protocol = 0;
	backlog = 20;
	s = -1;
	ss = -1;
}


sstrmsock::sstrmsock(const sstrmsock &copy)
{
	if (this == &copy) {
		return;
	}
	fOpen = copy.fOpen;
	fOpenS = copy.fOpenS;
	ulBufSize = copy.ulBufSize;
	domain = copy.domain;
	type = copy.type;
	protocol = copy.protocol;
	backlog = copy.backlog;
	s = copy.s;
	ss = copy.ss;
	errcode = 0;
}


sstrmsock::~sstrmsock()
{
// Note: do NOT close the server socket by default! This will cause
// an error in a multi-threaded environment.
	if (fOpen)
		Close();
}


bool sstrmsock::Accept(void)
{
	socklen_t sz = sizeof(addr);

	if (!fOpenS) {
		return(TRUE);
	}
// accept the connection and obtain the connection socket
	sz = sizeof (struct sockaddr);
	s = accept(ss, (struct sockaddr *) &addr, &sz);
	if (s == -1) {
		errcode = errno;
		fOpen = FALSE;
		return(TRUE);
	}

	fOpen = TRUE;
	return(FALSE);
}

void sstrmsock::CloseSrv(void)
{
	if (fOpenS) {
		close(ss);
		fOpenS = FALSE;
	}
	errcode = 0;
	return;
}


bool sstrmsock::Create(
        int    Port)                    // the local port
{
	int    Rc;				// return code
	int    so_reuseaddr = TRUE;	// socket reuse flag

        // get a server socket
	ss = socket(domain, type, protocol);
	if (ss == -1) {
		errcode = errno;
		return(TRUE);
	}
        // set the socket option to reuse the address
	setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
							sizeof(so_reuseaddr));
        // bind the server socket to a port
	memset(&addr, 0, sizeof (addr));
	addr.sin_family = domain;
	addr.sin_port = htons(Port);
	addr.sin_addr.s_addr = INADDR_ANY;
	Rc = bind(ss, (struct sockaddr *) &addr, sizeof(addr));
	if (Rc == -1) {
		errcode = errno;
		return (TRUE);
	}
        // listen for a client at the port
	Rc = listen(ss, backlog);
	if (Rc == -1) {
		errcode = errno;
		return(TRUE);
	}

	errcode = 0;
	fOpenS = TRUE;
	return(FALSE);
}
