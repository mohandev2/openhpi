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
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <errno.h>
#include <glib.h>
#include <byteswap.h>

#include "strmsock.h"
#include "marshal.h"


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

cMessageHeader* strmsock::GetHeader(void)
{
	return(&header);
}

void strmsock::SetBufferSize(
        unsigned long ulNewSize)        // the new buffer size
{
	if (ulNewSize > 4096)
		ulBufSize = ulNewSize;
	pBuf = g_realloc(pBuf, ulBufSize);
	if (pBuf == NULL)
		errcode = errno;
	else
		errcode = 0;
	return;
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

void strmsock::ConWriteMsg(const void *msg)
{
	if (!fOpen) {
		errcode = -1;
		return;
	}

	 int l = sizeof( cMessageHeader ) + header.m_len;

	if ( l > dMaxMessageLength ) {
		errcode = -1;
		return;
	}

	 char data[dMaxMessageLength];

	memcpy( data, &header, sizeof( cMessageHeader ) );

	if ( header.m_len ) {
		memcpy( data + sizeof( cMessageHeader ), msg, header.m_len );
	}

	int rv = write( s, data, l);

	if ( rv != l ) {
		errcode = errno;
		return;
	}

	errcode = 0;
	return;
}

void* strmsock::ConReadMsg(void)
{

	if (!fOpen) {
		errcode = -1;
		return NULL;
	}

	unsigned int len = read( s, pBuf, ulBufSize);

	if (len < 0) {
		errcode = errno;
		return NULL;
	} else if (len == 0) {	//connection has been aborted by the peer
		errcode = 0;
		Close();
		return NULL;
	} else if ( len < sizeof( cMessageHeader) ) {
		errcode = -1;
		return NULL;
	}

	memcpy( &header, pBuf, sizeof( cMessageHeader ) );

	if ( (header.m_flags >> 4) != dMhVersion ) {
		errcode = -1;
		return NULL;
	}

// swap id and len if nessesary
	if ( (header.m_flags & dMhEndianBit) != MarshalByteOrder() ) {
		header.m_id  = bswap_32( header.m_id );
		header.m_len = bswap_32( header.m_len );
	}

	if (header.m_len != 0) {
		while (len - sizeof( cMessageHeader) < header.m_len) {
			unsigned int add_len = read( s, (char *)pBuf + len, ulBufSize - len);
				if (add_len <= 0) {
					errcode = errno;
					return NULL;
				}
			len += add_len;
		}
	}

	errcode = 0;
	return ((char *)pBuf + sizeof( cMessageHeader));
}

void strmsock::MessageHeaderInit(tMessageType, unsigned char flags,
				unsigned int id, unsigned int len )
{
	header.m_type    = type;
	header.m_flags   = flags;

// set version
	header.m_flags &= 0x0f;
	header.m_flags |= dMhVersion << 4;

// set endian
	header.m_flags &= ~dMhEndianBit;
	header.m_flags |= MarshalByteOrder();

	header.m_id = id;
	header.m_len = len;
}

/*--------------------------------------------------------------------*/
/* Stream Sockets client class methods                   */
/*--------------------------------------------------------------------*/


cstrmsock::cstrmsock()
{
	fOpen = FALSE;
	ulBufSize = 4096;
	domain = AF_INET;
	type = SOCK_STREAM;
	protocol = 0;
	s = -1;
	bzero(&header, sizeof(cMessageHeader));
	pBuf = g_malloc(ulBufSize);
	if (pBuf == NULL) {
		errcode = -1;
	} else {
		errcode = 0;
	}
}


cstrmsock::~cstrmsock()
{
	if (fOpen)
		Close();
	if (pBuf)
		g_free(pBuf);
}


bool cstrmsock::Open(
		const char * pszHost,	// the remote host
		const int lPort)		// the remote port
{
	struct sockaddr_in  addr;		// address structure
	struct hostent     *phe;		 // pointer to a host entry

// get a socket
	s = socket(domain, type, protocol);
	if (s == -1) {
		errcode = errno;
		return(TRUE);
	}
// convert the host entry/name to an address
	phe = gethostbyname(pszHost);
	if (phe)
		bcopy(phe -> h_addr, (char *) &addr.sin_addr, phe -> h_length);
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

void cstrmsock::ClientWriteMsg(const void *msg)
{
	return ConWriteMsg(msg);
}

void* cstrmsock::ClientReadMsg(void)
{
	return ConReadMsg();
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
	bzero(&header, sizeof(cMessageHeader));
	s = -1;
	ss = -1;
	pBuf = g_malloc(ulBufSize);
	if (pBuf == NULL) {
		errcode = -1;
	} else {
		errcode = 0;
	}
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
	pBuf = g_malloc(ulBufSize); // each copy needs its own buffer
	errcode = 0;
}


sstrmsock::~sstrmsock()
{
// Note: do NOT close the server socket by default! This will cause
// an error in a multi-threaded environment.
	if (fOpen)
		Close();
	if (pBuf)
		g_free(pBuf);
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
	memset(&addr, sizeof (addr), 0);
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

void sstrmsock::ServerWriteMsg(const void *msg)
{
	return ConWriteMsg(msg);
}

void* sstrmsock::ServerReadMsg(void)
{
	return ConReadMsg();
}
