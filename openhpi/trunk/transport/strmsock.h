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

#ifndef STRMSOCK_H_INCLUDED
#define STRMSOCK_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif


/***************************************************************
 * IPv4 / IPv6
 **************************************************************/
typedef enum
{
    FlagIPv4   = 1,
    FlagIPv6   = 2
} IPvFlags;


/***************************************************************
 * Message Header
 **************************************************************/
typedef struct
{
	uint8_t	 type;
    uint8_t  flags; // bits 0-3 : flags, bit 4-7 : OpenHPI RPC version
	uint32_t id;
	uint32_t len;
} MessageHeader;

const uint8_t eMhMsg   = 1;
const uint8_t eMhError = 2;

// message flags
// if bit is set the byte order is Little Endian
const uint8_t dMhEndianBit  = 1;
const uint8_t dMhRpcVersion = 1;


const size_t dMaxMessageLength = 0xFFFF;
const size_t dMaxPayloadLength = dMaxMessageLength - sizeof(MessageHeader);


/***************************************************************
 * Base Stream Socket class
 **************************************************************/
class cStreamSock
{
protected:

#ifdef _WIN32
    typedef SOCKET SockFdT;
    static const SockFdT InvalidSockFd = INVALID_SOCKET;
#else
    typedef int SockFdT;
    static const SockFdT InvalidSockFd = -1;
#endif

public:

    explicit cStreamSock( SockFdT sockfd = InvalidSockFd );
    virtual ~cStreamSock();

    bool ReadMsg( uint8_t& type,
                  uint32_t& id,
                  void * payload,
                  uint32_t& payload_len,
                  int& payload_byte_order );

    bool WriteMsg( uint8_t type,
                   uint32_t id,
                   const void * payload,
                   uint32_t payload_len );

protected:

    SockFdT SockFd() const
    {
        return m_sockfd;
    }

    bool Create( const struct addrinfo * ainfo );
    bool Close();

private:

    cStreamSock( const cStreamSock& );
    cStreamSock& operator =( const cStreamSock& );

private:

    SockFdT m_sockfd;
};


/***************************************************************
 * Client Stream Socket class
 **************************************************************/
class cClientStreamSock : public cStreamSock
{
public:

    explicit cClientStreamSock();
    ~cClientStreamSock();

    bool Create( const char * host, uint16_t port );

    /***********************
     * TCP Keep-Alive
     *
     * keepalive_time   - interval(sec) between the last data packet sent and
     *                    the first keepalive probe
     * keepalive_intvl  - interval(sec) between subsequential keepalive probes
     * keepalive_probes - number of unacknowledged probes to send before
     *                    considering the connection dead
     **********************/
    bool EnableKeepAliveProbes( int keepalive_time,
                                int keepalive_intvl,
                                int keepalive_probes );

private:

    cClientStreamSock( const cClientStreamSock& );
    cClientStreamSock& operator =( const cClientStreamSock& );

    bool Create( const struct addrinfo * ainfo );
};


/***************************************************************
 * Server Stream Socket class
 **************************************************************/
class cServerStreamSock : public cStreamSock
{
public:

    explicit cServerStreamSock();
    ~cServerStreamSock();

    bool Create( int ipvflags, const char * bindaddr, uint16_t port );

    cStreamSock * Accept();

private:

    cServerStreamSock( const cServerStreamSock& );
    cServerStreamSock& operator =( const cServerStreamSock& );

    bool Create( const struct addrinfo * ainfo );
};


#endif  // STRMSOCK_H_INCLUDED__

