/*
 *     ____             _________                __                _     
 *    / __ \___  ____ _/ /_  __(_)___ ___  ___  / /   ____  ____ _(_)____
 *   / /_/ / _ \/ __ `/ / / / / / __ `__ \/ _ \/ /   / __ \/ __ `/ / ___/
 *  / _, _/  __/ /_/ / / / / / / / / / / /  __/ /___/ /_/ / /_/ / / /__  
 * /_/ |_|\___/\__,_/_/ /_/ /_/_/ /_/ /_/\___/_____/\____/\__, /_/\___/  
 *                                                       /____/          
 *
 *                  Barracuda Embedded Web-Server 
 ****************************************************************************
 *            PROGRAM MODULE
 *
 *   $Id: BaServerLib.c 3455 2014-08-18 21:22:21Z wini $
 *
 *   COPYRIGHT:  Real Time Logic, 2002 - 2014
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
 *               http://www.realtimelogic.com
 ****************************************************************************
 *
 */

#ifndef BA_LIB
#define BA_LIB 1
#endif

#include <BaAtoi.h>
#include <string.h>


BA_API U32
U32_negate(U32 n)
{
   return (U32)(-(S32)n);
}


BA_API U32
U32_atoi2(const char* s, const char* e)
{
   U32 n = 0;
   BaBool isNegative;
   if(!s) return 0;
   if(*s == '-')
   {
      ++s;
      isNegative = TRUE;
   }
   else
      isNegative = FALSE;
   for ( ; s < e && *s != '.' ; ++s ) /* '.' for floating point */
      n = 10 * n + (*s-'0');
   if(*s == '.' && s[1])
   {
      if(s[1] >= '5')
         n++;
   }
   return isNegative ? U32_negate(n) : n;
}


BA_API U32
U32_atoi(const char* s)
{
   if(!s) return 0;
   return U32_atoi2(s, s+strlen(s));
}

BA_API U32
U32_hextoi(const char *str)
{
   U32 number = 0;
   U32 i;
   if(!str) return 0;
   for(i = 0 ; i<8 && *str!=0 ; i++)
   {
      U8 c = *str++ ;
      if(c>='0' && c<='9') c -= '0' ; /* 0..9 */
      else if(c>='a' && c<='f') c = c-'a'+10 ; /* 10..15 */
      else if(c>='A' && c<='F') c = c-'A'+10 ; /* 10..15 */
      else break; /* Invalid character. */
      number = (number << 4) | c ;
   }
   return *str==0 ? number : 0 ;
}


BA_API U64
U64_atoll2(const char* s, const char* e)
{
   U64 n = 0;
   BaBool isNegative;
   if(!s) return 0;
   if(*s == '-')
   {
      ++s;
      isNegative = TRUE;
   }
   else
      isNegative = FALSE;
   for ( ; s < e ; ++s )
      n = 10 * n + (*s-'0');
   return isNegative ? (U64)(-(S64)n) : n;
}


BA_API U64
U64_atoll(const char* s)
{
   if(!s) return 0;
   return U64_atoll2(s, s+strlen(s));
}
