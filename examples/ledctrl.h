/*
 *     ____             _________                __                _
 *    / __ \___  ____ _/ /_  __(_)___ ___  ___  / /   ____  ____ _(_)____
 *   / /_/ / _ \/ __ `/ / / / / / __ `__ \/ _ \/ /   / __ \/ __ `/ / ___/
 *  / _, _/  __/ /_/ / / / / / / / / / / /  __/ /___/ /_/ / /_/ / / /__
 * /_/ |_|\___/\__,_/_/ /_/ /_/_/ /_/ /_/\___/_____/\____/\__, /_/\___/
 *                                                       /____/
 *
 *                 SharkSSL Embedded SSL/TLS Stack
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

#ifndef _ledctrl_h
#define _ledctrl_h

#include <TargConfig.h>


/* Each LED is registered with the following information */
typedef struct {
   const char* name; /* LED name shown in the browser */
   const char* color; /* The color of this particular LED */
   int id; /* A unique ID for the LED. */
} LedInfo;


void mainTask(void); /* Ref-Ta */
const LedInfo* getLedInfo(int* len);
const char* getDevName(void);
void setLed(int ledId, int on);
int setLedFromDevice(int* ledId, int* on);


typedef enum {
   ProgramStatus_Starting,
   ProgramStatus_Restarting,
   ProgramStatus_Connecting,
   ProgramStatus_SslHandshake,
   ProgramStatus_DeviceReady,
   ProgramStatus_CloseCommandReceived,

   ProgramStatus_SocketError,
   ProgramStatus_DnsError,
   ProgramStatus_ConnectionError,
   ProgramStatus_CertificateNotTrustedError,
   ProgramStatus_SslHandshakeError,
   ProgramStatus_WebServiceNotAvailError,
   ProgramStatus_PongResponseError,
   ProgramStatus_InvalidCommandError,
   ProgramStatus_MemoryError
} ProgramStatus;


void setProgramStatus(ProgramStatus s);

#endif
