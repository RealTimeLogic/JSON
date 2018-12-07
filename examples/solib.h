/*
 *     ____             _________                __                _
 *    / __ \___  ____ _/ /_  __(_)___ ___  ___  / /   ____  ____ _(_)____
 *   / /_/ / _ \/ __ `/ / / / / / __ `__ \/ _ \/ /   / __ \/ __ `/ / ___/
 *  / _, _/  __/ /_/ / / / / / / / / / / /  __/ /___/ /_/ / /_/ / / /__
 * /_/ |_|\___/\__,_/_/ /_/ /_/_/ /_/ /_/\___/_____/\____/\__, /_/\___/
 *                                                       /____/
 *
 ****************************************************************************
 *   PROGRAM MODULE
 *
 *   $Id$
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2014
 *
 *   This software is copyrighted by and is the sole property of Real
 *   Time Logic LLC.  All rights, title, ownership, or other interests in
 *   the software remain the property of Real Time Logic LLC.  This
 *   software may only be used in accordance with the terms and
 *   conditions stipulated in the corresponding license agreement under
 *   which the software has been supplied.  Any unauthorized use,
 *   duplication, transmission, distribution, or disclosure of this
 *   software is expressly forbidden.
 *
 *   This Copyright notice may not be removed or modified without prior
 *   written consent of Real Time Logic LLC.
 *
 *   Real Time Logic LLC. reserves the right to modify this software
 *   without notice.
 *
 *               http://realtimelogic.com
 ****************************************************************************
 *
 */

#ifndef _solib_h
#define _solib_h

#include <TargConfig.h>

#ifndef HOST_PLATFORM
#if defined(__GNUC__) || defined(_WIN32)
/** Assume it's running on a host with a std in/out console for VC++ and GCC
*/
#define HOST_PLATFORM 1
#ifndef XPRINTF
#define XPRINTF 1
#endif
#else
#define HOST_PLATFORM 0
#ifndef XPRINTF
#define XPRINTF 0
#endif
#endif
#endif

#ifdef __CODEWARRIOR__
/* Assume MQX */
#include <rtcs.h>
#include <ipcfg.h>
#else   /* ! __CODEWARRIOR__  */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __GNUC__
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#elif defined(SharkSSLNetX)
#include <nx_api.h>
typedef struct {
   NX_TCP_SOCKET nxSock;
   NX_PACKET* nxFirstPacket;
   NX_PACKET* nxPacket;
   UCHAR* nxPktOffs;
} SOCKET;
#define NO_BSD_SOCK
#endif  /* __GNUC__ */

#endif  /* __CODEWARRIOR__  */

#include <stdarg.h>

/** Infinite wait time option for socket read functions.
 */
#define INFINITE_TMO (~((U32)0))

#ifndef NO_BSD_SOCK
/** The SOCKET object/handle is an 'int' when using a BSD compatible
    TCP/IP stack. Non BSD compatible TCP IP stacks must set the macro
    NO_BSD_SOCK and define the SOCKET object. See the header file
    solib.h for details.
*/
#define SOCKET int
#endif


#ifdef __cplusplus
extern "C" {
#endif

/** Initializes a SOCKET object connected to a remote host/address at
 * a given port.
 \return  Zero on success. 
   Error codes returned:
   \li \c -1 Cannot create socket: Fatal
   \li \c -2 Cannot resolve 'address'
   \li \c -3 Cannot connect
*/
int so_connect(SOCKET* sock, const char* address, U16 port);

/** Waits for remote connections on the server SOCKET object
   'listenSock', initialized by function so_bind, and initializes
   socket object 'outSock' to represent the new connection.

   \return
   \li \c 1 Success
   \li \c 0 timeout
   \li \c -1 error
*/
int so_accept(SOCKET* listenSock, U32 timeout, SOCKET* outSock);

/** Close a connected socket connection.
 */
void so_close(SOCKET* sock);

/** Returns TRUE if socket is valid (connected).
 */
int so_sockValid(SOCKET* sock);

/** Sends data to the connected peer.
 */
S32 so_send(SOCKET* sock, const void* buf, U32 len);

/** Waits for data sent by peer.

    \param sock the SOCKET object.
    \param buf is the data to send.
    \param len is the 'buf' length.
    \param timeout in milliseconds. The timeout can be set to #INFINITE_TMO.
    \returns the length of the data read, zero on timeout, or a
    negative value on error.
 */
S32 so_recv(SOCKET* sock, void* buf, U32 len, U32 timeout);

#if XPRINTF == 1
/** The macro xprintf expands to function _xprintf if the code is
    compiled with XPRINTF set to 1.
    \param data is standard printf arguments enclosed in parenthesis;
    thus you must use double parenthesis when using macro xprintf.
*/
#define xprintf(data) _xprintf data
/** The example code and macro xprintf requires this function when the
    code is compiled with macro XPRINTF set to 1.
    \param fmt the format string.
    \param ... variable arguments.
*/
void _xprintf(const char* fmt, ...);
#else
#define xprintf(data)
#endif

#ifdef __cplusplus
}
#endif


/** @} */ /* end group solib */
/** @} */ /* end group Examples */ 

#endif
