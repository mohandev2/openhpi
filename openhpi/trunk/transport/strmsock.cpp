/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2004
 * (C) Copyright Pigeon Point Systems. 2010
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

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <glib.h>

#include <oh_error.h>

#include "strmsock.h"


/***************************************************************
 * Initialization/Finalization
 **************************************************************/
static void Initialize( void )__attribute__ ((constructor));

static void Initialize( void )
{
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup( MAKEWORD( 2, 2), &wsa_data );
#endif
}

static void Finalize( void )__attribute__ ((destructor));

static void Finalize( void )
{
#ifdef _WIN32
    WSACleanup();
#endif
}

/***************************************************************
 * Helper functions
 **************************************************************/
static void SelectAddresses( int hintflags,
                             const char * node,
                             uint16_t port,
                             struct addrinfo * & selected )
{
    selected = 0;

    struct addrinfo hints;
    memset( &hints, 0, sizeof(hints) );
    hints.ai_flags    = hintflags;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char service[32];
    snprintf( service, sizeof(service), "%u", port );

    struct addrinfo * items;
    int cc = getaddrinfo( node, service, &hints, &items );
    if ( cc != 0 ) {
        CRIT( "Transport: getaddrinfo failed." );
        return;
    }

    struct addrinfo * in = 0;
    struct addrinfo * in_tail = 0;
    struct addrinfo * out = 0;
    struct addrinfo * out_tail = 0;
    for ( struct addrinfo * item = items; item != 0; item = item->ai_next ) {
        if ( ( item->ai_family == AF_INET ) || ( item->ai_family == AF_INET6 ) ) {
            if ( in_tail ) {
                in_tail->ai_next = item;
                in_tail = item;
            } else {
                in = in_tail = item;
            }
        } else {
            if ( out_tail ) {
                out_tail->ai_next = item;
                out_tail = item;
            } else {
                out = out_tail = item;
            }
        }
    }
    if ( in_tail ) {
        in_tail->ai_next = 0;
    }
    if ( out_tail ) {
        out_tail->ai_next = 0;
        freeaddrinfo( out );
    }

    selected = in;
}


/***************************************************************
 * Base Stream Socket class
 **************************************************************/
cStreamSock::cStreamSock( SockFdT sockfd )
    : m_sockfd( sockfd )
{
    // empty
}

cStreamSock::~cStreamSock()
{
    Close();
}

bool cStreamSock::ReadMsg( uint8_t& type,
                           uint32_t& id,
                           void * payload,
                           uint32_t& payload_len,
                           int& payload_byte_order )
{
    union {
        MessageHeader hdr;
        char rawhdr[sizeof(MessageHeader)];
    };

    char * dst = rawhdr;
    size_t got  = 0;
    size_t need = sizeof(MessageHeader);
    while ( got < need ) {
        ssize_t len = recv( m_sockfd, dst + got, need - got, 0 );
        if ( len < 0 ) {
            CRIT( "Transport: error while reading message." );
            return false;
        } else if ( len == 0 ) {
            CRIT( "Transport: peer closed connection." );
            return false;
        }
        got += len;

        if ( ( got == need ) && ( dst == rawhdr ) ) {
            // we got header
            uint8_t ver = hdr.flags >> 4;
            if ( ver != dMhRpcVersion ) {
                CRIT( "Transport: unsupported version 0x%x != 0x%x.",
                     ver,
                     dMhRpcVersion );
                return false;
            }
            payload_byte_order = ( ( hdr.flags & dMhEndianBit ) != 0 ) ?
                                 G_LITTLE_ENDIAN : G_BIG_ENDIAN;
            // swap id and len if nessesary in the reply header
            if ( payload_byte_order != G_BYTE_ORDER ) {
                hdr.id  = GUINT32_SWAP_LE_BE( hdr.id );
                hdr.len = GUINT32_SWAP_LE_BE( hdr.len );
            }
            // now prepare to get payload
            dst  = reinterpret_cast<char *>(payload);
            got  = 0;
            need = hdr.len;
        }
    }

    type = hdr.type;
    id   = hdr.id;
    payload_len = got;

/*
    printf( "Transport: got message of %u bytes in buffer %p:\n",
            sizeof(MessageHeader) + got,
            payload );
    for ( size_t i = 0; i < sizeof(MessageHeader); ++i ) {
        union {
            char c;
            unsigned char uc;
        };
        c = rawhdr[i];
        printf( "%02x ", uc );
    }
    for ( size_t i = 0; i < got; ++i ) {
        union {
            char c;
            unsigned char uc;
        };
        c = (reinterpret_cast<char *>(payload))[i];
        printf( "%02x ", uc );
    }
    printf( "\n" );
*/

    return true;
}

bool cStreamSock::WriteMsg( uint8_t type,
                            uint32_t id,
                            const void * payload,
                            uint32_t payload_len )
{
    if ( ( payload_len > 0 ) && ( payload == 0 ) ) {
        return false;
    }
    if ( payload_len > dMaxPayloadLength ) {
        CRIT("Transport: message payload too large.");
        return false;
    }

    union {
        MessageHeader hdr;
        char msg[dMaxMessageLength];
    };

    hdr.type  = type;
    hdr.flags = dMhRpcVersion << 4;
    if ( G_BYTE_ORDER == G_LITTLE_ENDIAN ) {
        hdr.flags |= dMhEndianBit;
    }
    hdr.id    = id;
    hdr.len   = payload_len;

    if ( payload ) {
        memcpy( &msg[sizeof(MessageHeader)], payload, hdr.len );
    }

    size_t msg_len = sizeof(MessageHeader) + hdr.len;

/*
    printf("Transport: sending message of %d bytes:\n", msg_len );
    for ( size_t i = 0; i < msg_len; ++i ) {
        union {
            char c;
            unsigned char uc;
        };
        c = msg[i];
        printf( "%02x ", uc );
    }
    printf( "\n" );
*/

    ssize_t cc = send( m_sockfd, &msg[0], msg_len, 0 );
    if ( cc != (ssize_t)msg_len ) {
        CRIT( "Transport: error while sending message." );
        return false;
    }

    return true;
}

bool cStreamSock::Create( const struct addrinfo * info )
{
    bool rc = Close();
    if ( !rc ) {
        return false;
    }

    SockFdT new_sock;
    new_sock = socket( info->ai_family, info->ai_socktype, info->ai_protocol );
    if ( new_sock == InvalidSockFd ) {
        CRIT( "Transport: cannot create stream socket." );
        return false;
    }

    m_sockfd = new_sock;

    return true;
}

bool cStreamSock::Close()
{
    if ( m_sockfd == InvalidSockFd ) {
        return true;
    }

    int cc;
#ifdef _WIN32
    cc = closesocket( m_sockfd );
#else
    cc = close( m_sockfd );
#endif
    if ( cc != 0 ) {
        CRIT( "Transport: cannot close stream socket." );
        return false;
    }

    m_sockfd = InvalidSockFd;

    return true;
}


/***************************************************************
 * Client Stream Socket class
 **************************************************************/
cClientStreamSock::cClientStreamSock()
    : cStreamSock()
{
    // empty
}

cClientStreamSock::~cClientStreamSock()
{
    // empty
}

bool cClientStreamSock::Create( const char * host, uint16_t port )
{
    struct addrinfo * infos;
    SelectAddresses( 0, host, port, infos );
    if ( infos == 0 ) {
        CRIT( "Transport: failed to find sockaddr." );
        return false;
    }

    bool connected = false;
    struct addrinfo * info;
    for ( info = infos; info != 0; info = info->ai_next ) {
        connected = Create( info );
        if ( connected ) {
            break;
        }
    }

    freeaddrinfo( infos );

    return connected;
}

bool cClientStreamSock::EnableKeepAliveProbes( int keepalive_time,
                                               int keepalive_intvl,
                                               int keepalive_probes )
{
#ifdef __linux__
    int rc;
    int val;

    val = 1;
    rc = setsockopt( SockFd(), SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val) );
    if ( rc != 0 ) {
        CRIT( "Transport: failed to set SO_KEEPALIVE option." );
        return false;
    }
    val = keepalive_time;
    rc = setsockopt( SockFd(), SOL_TCP, TCP_KEEPIDLE, &val, sizeof(val) );
    if ( rc != 0 ) {
        CRIT( "Transport: failed to set TCP_KEEPIDLE option." );
        return false;
    }
    val = keepalive_intvl;
    rc = setsockopt( SockFd(), SOL_TCP, TCP_KEEPINTVL, &val, sizeof(val) );
    if ( rc != 0 ) {
        CRIT( "Transport: failed to set TCP_KEEPINTVL option." );
        return false;
    }
    val = keepalive_probes;
    rc = setsockopt( SockFd(), SOL_TCP, TCP_KEEPCNT, &val, sizeof(val) );
    if ( rc != 0 ) {
        CRIT( "Transport: failed to set TCP_KEEPCNT option." );
        return false;
    }

    return true;

#else

    CRIT( "Transport: TCP Keep-Alive Probes are not supported." );

    return false;

#endif /* __linux__ */
}

bool cClientStreamSock::Create( const struct addrinfo * info )
{
    bool rc = cStreamSock::Create( info );
    if ( !rc ) {
        return false;
    }

    int cc = connect( SockFd(), info->ai_addr, info->ai_addrlen );
    if ( cc != 0 ) {
        Close();
        CRIT( "Transport: connect failed." );
        return false;
    }

    return true;
}


/***************************************************************
 * Server Stream Socket class
 **************************************************************/
cServerStreamSock::cServerStreamSock()
    : cStreamSock()
{
    // empty
}

cServerStreamSock::~cServerStreamSock()
{
    // empty
}

bool cServerStreamSock::Create( uint16_t port )
{
    struct addrinfo * infos;
    SelectAddresses( AI_PASSIVE, 0, port, infos );
    if ( infos == 0 ) {
        CRIT( "Transport: failed to find sockaddr." );
        return false;
    }

    bool bound = false;
    struct addrinfo * info;
    for ( info = infos; info != 0; info = info->ai_next ) {
        bound = Create( info );
        if ( bound ) {
            break;
        }
    }

    freeaddrinfo( infos );

    return bound;
}

bool cServerStreamSock::Create( const struct addrinfo * info )
{
    bool rc = cStreamSock::Create( info );
    if ( !rc ) {
        return false;
    }

    int cc;

    int val = 1;
#ifdef _WIN32
    cc = setsockopt( SockFd(),
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     (const char *)&val, sizeof(val) );
#else
    cc = setsockopt( SockFd(),
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     &val,
                     sizeof(val) );
#endif
    if ( cc != 0 ) {
        Close();
        CRIT( "Transport: failed to set SO_REUSEADDR option." );
        return false;
    }
    cc = bind( SockFd(), info->ai_addr, info->ai_addrlen );
    if ( cc != 0 ) {
        Close();
        CRIT( "Transport: bind failed." );
        return false;
    }
    cc = listen( SockFd(), 5 /* TODO */ );
    if ( cc != 0 ) {
        Close();
        CRIT( "Transport: listen failed." );
        return false;
    }

    return true;
}

cStreamSock * cServerStreamSock::Accept()
{
    SockFdT sock = accept( SockFd(), 0, 0 );
    if ( sock == InvalidSockFd ) {
        CRIT( "Transport: accept failed." );
        return 0;
    }

    return new cStreamSock( sock );
}

