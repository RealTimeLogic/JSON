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
 *   $Id: BufPrint.c 5029 2022-01-16 21:32:09Z wini $
 *
 *   COPYRIGHT:  Real Time Logic, 2002 - 2022
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

#include <BufPrint.h>

#include <limits.h> /* SHRT_MAX */

/****************************************************************************
 *                         basnprintf
 ***************************************************************************/

static int
BufPrint_defaultFlush(struct BufPrint* bp, int sizeRequired)
{
   bp->cursor=0; /* Reset */
   /* Buffer is full if sizeRequired set (if not a
    * BufPrint_flush() call)
    * This indicates a program error.
    */
   baAssert(sizeRequired == 0); /* Program error in code calling BufPrint_xxx */
   return sizeRequired ? -1 : 0;
}


BA_API int
basnprintf(char* buf, int len, const char* fmt, ...)
{
   int retv;
   va_list varg;
   BufPrint bufPrint;
   /* (len -1) -> -1 -> space for the string terminator */
   BufPrint_constructor2(&bufPrint, buf, (len -1), 0, BufPrint_defaultFlush);
   va_start(varg, fmt);
   retv = BufPrint_vprintf(&bufPrint, fmt, varg);
   if( retv >= 0 )
   {
      buf[bufPrint.cursor] = 0; /* Zero terminate string */
      retv = bufPrint.cursor;
   }
   va_end(varg);
   return retv;
}


BA_API int
basprintf(char* buf, const char* fmt, ...)
{
   int retv;
   va_list varg;
   BufPrint bufPrint;
   /* We have no idea what the size is so let's set it to fffff */
   BufPrint_constructor2(
      &bufPrint, buf, (int)((unsigned int)(~0)/2u), 0, BufPrint_defaultFlush);
   bufPrint.cursor = 0; /* Start position is beginning of buf */
   va_start(varg, fmt);
   retv = BufPrint_vprintf(&bufPrint, fmt, varg);
   if( retv >= 0 )
   {
      buf[bufPrint.cursor] = 0; /* Zero terminate string */
      retv = bufPrint.cursor;
   }
   va_end(varg);
   return retv;
}


/****************************************************************************
 *                         BufPrint
 ***************************************************************************/

#define XDBL_DIG 15

#define FLAG_LEFTADJUST 0x0001
#define FLAG_SIGN       0x0002
#define FLAG_ZEROPAD    0x0004
#define FLAG_ALTERNATE  0x0008
#define FLAG_SPACE      0x0010
#define FLAG_SHORT      0x0020
#define FLAG_LONG       0x0040
#define FLAG_LONG_LONG  0x0080
#define FLAG_LONGDOUBLE 0x0100
#define FLAG_POINTER    0x0200
#define MAXWIDTH ((SHRT_MAX - 9) / 10)



#ifdef B_BIG_ENDIAN
#define isNanOrInf(puVal) \
( \
  sizeof((*puVal).dVal) == sizeof((*puVal).aul) \
  ? /* We assume 64-bits double in IEEE 754 format */ \
      (((*puVal).aul[0] & 0x7ff00000UL) == 0x7ff00000UL) \
      ? /* Inf or Nan */ \
        (((*puVal).aul[0] & 0x000fffffUL) || (*puVal).aul[1]) \
        ? /* Nan */ \
          ((*puVal).aul[0] & 0x80000000UL) \
          ? 1 /* -Nan  */ \
          : 2 /*  Nan  */ \
        : /* Inf */ \
          ((*puVal).aul[0] & 0x80000000UL) \
          ? 3 /* -Inf  */ \
          : 4 /*  Inf  */ \
      : 0 \
  : 0 \
)
#elif defined(B_LITTLE_ENDIAN)
#define isNanOrInf(puVal) \
( \
  sizeof((*puVal).dVal) == sizeof((*puVal).aul) \
  ? /* We assume 64-bits double in IEEE 754 format */ \
      (((*puVal).aul[1] & 0x7ff00000UL) == 0x7ff00000UL) \
      ? /* Inf or Nan */ \
        (((*puVal).aul[1] & 0x000fffffUL) || (*puVal).aul[0]) \
        ? /* Nan */ \
          ((*puVal).aul[1] & 0x80000000UL) \
          ? 1 /* -Nan  */ \
          : 2 /*  Nan  */ \
        : /* Inf */ \
          ((*puVal).aul[1] & 0x80000000UL) \
          ? 3 /* -Inf  */ \
          : 4 /*  Inf  */ \
      : 0 \
  : 0 \
)
#else
#error ENDIAN_NEEDED_Define_one_of_B_BIG_ENDIAN_or_B_LITTLE_ENDIAN
#endif



#ifndef NO_DOUBLE
union UIEEE_754
{
   unsigned int aul[2];
   double        dVal;
};
#endif

BA_API int
BufPrint_putc(BufPrint* o, int c)
{
   BufPrint_putcMacro(o, (char)c);
   return 0;
}


static int
BufPrint_fmtLongLong(char** bufPtr, int* len, U64 val64,
                     unsigned radix, int fmtAsSigned)
{
   int isNegative;
   if (fmtAsSigned && (S64)val64 < 0)
   {
      val64 = -(S64)val64;
      isNegative = TRUE;
   }
   else
      isNegative = FALSE;
   do
   {
      char r = (U8)(val64 % radix);
      val64 /= radix;
      *--(*bufPtr) = ('0' + r);
      (*len)++;
   } while (val64);
   return isNegative;
}


BA_API void
BufPrint_constructor(BufPrint* o, void* userData, BufPrint_Flush flush)
{
   memset(o, 0, sizeof(BufPrint));
   o->userData = userData;
   o->flushCB = flush ? flush : BufPrint_defaultFlush;
}

BA_API void
BufPrint_constructor2(
   BufPrint* o, char* buf,int size,void* userData,BufPrint_Flush flush)
{
   BufPrint_constructor(o, userData,flush);
   o->buf=buf;
   o->bufSize=size;
}


#define BufPrint_getSizeLeft(o) (o.bufSize - o.cursor)

#define BufPrint_padChar(o, c, count) do{ \
   int i; \
   for(i=0; i< (count) ; i++) \
      BufPrint_putcMacro(o, c); \
} while(0)


BA_API int
BufPrint_write(BufPrint* o, const void* buf, int len)
{
   int retVal=0;
   if(!o)
      return -1;
   if(len < 0)
      len = (int)strlen((const char*)buf);
   if(len)
   {
      if((o->cursor + len) > o->bufSize && o->cursor)
         if( (retVal=o->flushCB(o,o->cursor+len+1-o->bufSize))!= 0)
            return retVal;
      if((o->cursor + len) <= o->bufSize)
      {
         memcpy(o->buf + o->cursor, buf, len);
         o->cursor += len;
      }
      else
      {
         const U8* ptr = (const U8*)buf;
         while(len > o->bufSize)
         {
            o->cursor = o->bufSize;
            memcpy(o->buf, ptr, o->bufSize);
            len -= o->bufSize;
            ptr += o->bufSize;
            if( (retVal=o->flushCB(o, o->cursor+len-o->bufSize))!=0)
               return retVal;
         }
         o->cursor = len;
         memcpy(o->buf, ptr, len);
      }
      baAssert(o->cursor <= o->bufSize);
   }
   return retVal;
}


BA_API int
BufPrint_b64Encode(BufPrint* o, const void* source, S32 slen)
{
   static const char b64alpha[] = {
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
   };

   const U8* src = (const U8*)source;

   while( slen >= 3 )
   {
      if(BufPrint_putc(o,b64alpha[*src>>2]) < 0 ||
         BufPrint_putc(o,b64alpha[(*src&0x03)<<4 | src[1]>>4])  < 0 ||
         BufPrint_putc(o,b64alpha[(src[1]&0x0F)<<2 | src[2]>>6])  < 0 ||
         BufPrint_putc(o, b64alpha[src[2] & 0x3F])  < 0)
      {
         return -1;
      }
      src += 3;
      slen -= 3;
   }
   switch(slen)
   {
      case 2:
         if(BufPrint_putc(o, b64alpha[src[0]>>2]) < 0 ||
            BufPrint_putc(o, b64alpha[(src[0] & 0x03)<<4 | src[1]>>4]) < 0 ||
            BufPrint_putc(o, b64alpha[(src[1] & 0x0F)<<2]) < 0 ||
            BufPrint_putc(o, (U8)'=') < 0)
         {
            return -1;
         }
         break;

      case 1:
         if(BufPrint_putc(o, b64alpha[src[0]>>2]) < 0 ||
            BufPrint_putc(o, b64alpha[(src[0] & 0x03)<<4]) < 0 ||
            BufPrint_write(o,"==",2) < 0)
         {
            return -1;
         }
         break;

      default:
         baAssert(slen == 0);
   }
   return 0;
}


BA_API int
BufPrint_b64urlEncode(BufPrint* o, const void* source, S32 slen, BaBool padding)
{
   static const char b64alpha[] = {
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
   };

   const U8* src = (const U8*)source;

   while( slen >= 3 )
   {
      if(BufPrint_putc(o,b64alpha[*src>>2]) < 0 ||
         BufPrint_putc(o,b64alpha[(*src&0x03)<<4 | src[1]>>4])  < 0 ||
         BufPrint_putc(o,b64alpha[(src[1]&0x0F)<<2 | src[2]>>6])  < 0 ||
         BufPrint_putc(o, b64alpha[src[2] & 0x3F])  < 0)
      {
         return -1;
      }
      src += 3;
      slen -= 3;
   }
   switch(slen)
   {
      case 2:
         if(BufPrint_putc(o, b64alpha[src[0]>>2]) < 0 ||
            BufPrint_putc(o, b64alpha[(src[0] & 0x03)<<4 | src[1]>>4]) < 0 ||
            BufPrint_putc(o, b64alpha[(src[1] & 0x0F)<<2]) < 0 ||
            (padding ? BufPrint_putc(o, (U8)'=') : 0) < 0)
         {
            return -1;
         }
         break;

      case 1:
         if(BufPrint_putc(o, b64alpha[src[0]>>2]) < 0 ||
            BufPrint_putc(o, b64alpha[(src[0] & 0x03)<<4]) < 0 ||
            (padding ? BufPrint_write(o,"==",2) : 0) < 0)
         {
            return -1;
         }
         break;

      default:
         baAssert(slen == 0);
   }
   return 0;
}



BA_API int
BufPrint_jsonString(BufPrint* o, const char* str)
{
   BufPrint_putcMacro(o,'"');
   while(*str)
   {
      if(*str < ' ' ||  *str == '"')
      {
         if(*str > 0)
         {
            switch(*str)
            {
               case '\b': BufPrint_write(o, "\\b",2); break;
               case '\t': BufPrint_write(o, "\\t",2); break;
               case '\n': BufPrint_write(o, "\\n",2); break;
               case '\v': BufPrint_write(o, "\\v",2); break;
               case '\f': BufPrint_write(o, "\\f",2); break;
               case '\r': BufPrint_write(o, "\\r",2); break;
               case '\"': BufPrint_write(o, "\\\"",2);break;
               case '\'': BufPrint_write(o, "\\'",2); break;
               default: BufPrint_printf(o,"\\u%04x",(unsigned)*str);
            }
         }
         else
         {
            unsigned char c = str[0];
            unsigned long uc = 0;
            if (c < 0xc0)
               uc = c;
            else if (c < 0xe0)
            {
               if ((str[1] & 0xc0) == 0x80)
               {
                  uc = ((c & 0x1f) << 6) | (str[1] & 0x3f);
                  ++str;
               }
               else
                  uc = c;
            }
            else if (c < 0xf0)
            {
               if ((str[1] & 0xc0) == 0x80 &&
                   (str[2] & 0xc0) == 0x80)
               {
                  uc = ((c & 0x0f) << 12) |
                     ((str[1] & 0x3f) << 6) | (str[2] & 0x3f);
                  str += 2;
               }
               else
                  uc = c;
            }
            else if (c < 0xf8)
            {
               if ((str[1] & 0xc0) == 0x80 &&
                   (str[2] & 0xc0) == 0x80 &&
                   (str[3] & 0xc0) == 0x80)
               {
                  uc = ((c & 0x03) << 18) |
                     ((str[1] & 0x3f) << 12) |
                     ((str[2] & 0x3f) << 6) |
                     (str[4] & 0x3f);
                  str += 3;
               }
               else
                  uc = c;
            }
            else if (c < 0xfc)
            {
               if ((str[1] & 0xc0) == 0x80 &&
                   (str[2] & 0xc0) == 0x80 &&
                   (str[3] & 0xc0) == 0x80 &&
                   (str[4] & 0xc0) == 0x80)
               {
                  uc = ((c & 0x01) << 24) |
                     ((str[1] & 0x3f) << 18) |
                     ((str[2] & 0x3f) << 12) |
                     ((str[4] & 0x3f) << 6) |
                     (str[5] & 0x3f);
                  str += 4;
               }
               else
                  uc = c;
            }
            else
               ++str; /* ignore */
            if (uc < 0x10000)
               BufPrint_printf(o,"\\u%04x", uc);
            else
            {
               uc -= 0x10000;
               BufPrint_printf(o,"\\u%04x", 0xdc00 | ((uc >> 10) & 0x3ff));
               BufPrint_printf(o,"\\u%04x", 0xd800 | (uc & 0x3ff));
            }
         }
      }
      else if(*str == '\\')
         BufPrint_write(o,"\\\\",2);
      else if(*str == '/')
         BufPrint_write(o, "\\/",2);
      else if(*str == 0x7f)
         BufPrint_printf(o,"\\u%04x",(unsigned)*str);
      else
         BufPrint_putcMacro(o, *str);
      str++;
   }
   BufPrint_putcMacro(o,'"');
   return 0;
}


BA_API int
BufPrint_vprintf(BufPrint* o, const char* fmt, va_list argList)
{
   int retVal;
   int outCount = 0;
   if(!o)
      return -1;
   if( ! o->buf )
   { /* buffer not created. Call flush to create buffer */
      if(o->flushCB(o, 0))
         return -1;
   }
   while (*fmt)
   {
      if(*fmt != '%')
      {
         const char* start = fmt;
         while (*fmt && *fmt != '%')
            ++fmt;
         if( (retVal=BufPrint_write(o, start, (int)(fmt - start))) != 0)
            return retVal;
         continue;
      }
      else
      {
         const char *ptr;
         const char *prefix = "";
         unsigned int flags;
         int width, precision, bufLen, prefixLen, noOfLeadingZeros;
         int tmp;
         char buf[50];
         bufLen = prefixLen = noOfLeadingZeros = 0;
         flags = 0;
         ++fmt;
         while ((*fmt == '-') || (*fmt == '+') ||
                (*fmt == '0') || (*fmt == '#') || (*fmt == ' '))
         {
            switch (*fmt++)
            {
               case '-': flags |= FLAG_LEFTADJUST; break;
               case '+': flags |= FLAG_SIGN;       break;
               case '0': flags |= FLAG_ZEROPAD;    break;
               case '#': flags |= FLAG_ALTERNATE;  break;
               case ' ': flags |= FLAG_SPACE;      break;
            }
         }
         if (*fmt == '*')
         {
            ++fmt;
            width = va_arg(argList, int);
            if (width < 0)
            {
               width = -width;
               flags |= FLAG_LEFTADJUST;
            }
         }
         else
         {
            for (width = 0 ; (*fmt >= '0') && (*fmt <= '9') ; fmt++)
            {
               if (width < MAXWIDTH)
                  width = width * 10 + (*fmt - '0');
            }
         }
         if (*fmt == '.')
         {
            if (*++fmt == '*')
            {
               ++fmt;
               precision = va_arg(argList, int);
               if (precision < 0)
                  precision = 0;

            }
            else
            {
               for (precision = 0 ; (*fmt >= '0') && (*fmt <= '9') ; fmt++)
               {
                  if (precision < MAXWIDTH)
                     precision = precision * 10 + (*fmt - '0');
               }
            }
         }
         else
            precision = -1;
         switch (*fmt)
         {
            case 'h': flags |= FLAG_SHORT;      ++fmt; break;
            case 'l':
               if(*++fmt == 'l')
               {
                  flags |= FLAG_LONG_LONG;
                  ++fmt;
               }
               else
                  flags |= FLAG_LONG;
               break;
            case 'L': flags |= FLAG_LONGDOUBLE; ++fmt; break;
         }
         switch (*fmt)
         {
            case 'p': flags |= FLAG_POINTER;
               /* fall through */
            case 'x': case 'X':
            case 'o': case 'u':
            case 'd': case 'i':
            {
               size_t val=0;
               U64 longLongVal=0;
               char *bufPtr = (char *)&buf[(sizeof buf) - 1];
               *bufPtr = 0;
               if (flags & FLAG_LONG_LONG)
               {
                  longLongVal = va_arg(argList, U64);
                  if (precision == 0 && longLongVal == 0)
                  {
                     fmt++;
                     continue;
                  }
               }
               else
               {
                  if (flags & FLAG_POINTER)
                     val = (size_t)va_arg(argList, void *);
                  else if (flags & FLAG_SHORT)
                     val = (unsigned long)va_arg(argList, int);
                  else if (flags & FLAG_LONG)
                     val = (unsigned long)va_arg(argList, long);
                  else
                     val = (unsigned long)va_arg(argList, int);
                  if (precision == 0 && val == 0)
                  {
                     fmt++;
                     continue;
                  }
               }
               if (*fmt == 'd' || *fmt == 'i')
               {
                  if((flags & FLAG_LONG_LONG) &&
                     BufPrint_fmtLongLong(&bufPtr,&bufLen,longLongVal,10,TRUE))
                  {
                     prefix = "-";
                     prefixLen = 1;
                  }
                  else if ( ! (flags & FLAG_LONG_LONG) && (long)val < 0)
                  {
                     val = -(long)val;
                     prefix = "-";
                     prefixLen = 1;
                  }
                  else if (flags & FLAG_SIGN)
                  {
                     prefix = "+";
                     prefixLen = 1;
                  }
                  else if (flags & FLAG_SPACE)
                  {
                     prefix = " ";
                     prefixLen = 1;
                  }
                  if ( ! (flags & FLAG_LONG_LONG) )
                  {
                     do
                     {
                        int r = val % 10U;
                        val /= 10U;
                        *--bufPtr = (char)('0' + r);
                        bufLen++;
                     } while (val);
                  }
               }
               else
               {
                  const char *acDigits = (*fmt == 'X')
                     ? "0123456789ABCDEF"
                     : "0123456789abcdef";

                  if (flags & FLAG_LONG_LONG)
                  {
                     if(*fmt == 'x' || *fmt == 'X')
                     {
                        unsigned long mval =
                           (unsigned long)((U32)(0xFFFFFFFF & ((longLongVal) >> 32) ));
                        /* LSW */
                        val = (unsigned long)((U32)(longLongVal));
                        /* reusing retVal */
                        if(mval)
                        {
                           bufLen+=8;
                           for(retVal=0; retVal < 8 ; retVal++)
                           {
                              *--bufPtr = acDigits[val & 0xF];
                              if(val)
                                 val >>= 4;
                           }
                           /* MSW */
                           while(mval)
                           {
                              *--bufPtr = acDigits[mval & 0xF];
                              mval >>= 4;
                              bufLen++;
                           }
                        }
                        else
                        {
                           do
                           {
                              *--bufPtr = acDigits[val & 0xF];
                              val >>= 4;
                              bufLen++;
                           } while(val);
                        }
                     }
                     else
                     {  /* Assume 'u' */
                        BufPrint_fmtLongLong(
                           &bufPtr,&bufLen,longLongVal,
                           (*fmt == 'u') ? 10U : (*fmt == 'o') ? 8U : 16U,
                           FALSE);
                     }
                  }
                  else
                  {
                     const unsigned radix = (*fmt == 'u')
                        ? 10U
                        : (*fmt == 'o') ? 8U : 16U;
                     do
                     {
                        *--bufPtr = acDigits[val % radix];
                        val /= radix;
                        bufLen++;
                     } while (val);
                  }

                  if ((flags & FLAG_ALTERNATE) && *bufPtr != '0')
                  {
                     if (*fmt == 'o')
                     {
                        bufLen++;
                        *--bufPtr = '0';
                     }
                     else if (*fmt == 'X')
                     {
                        prefix = "0X";
                        prefixLen = 2;
                     }
                     else
                     {
                        prefix = "0x";
                        prefixLen = 2;
                     }
                  }
               }
               ptr = bufPtr;
            }
            noOfLeadingZeros = bufLen < precision ? precision - bufLen : 0;
            if (precision < 0 &&
                ((flags & (FLAG_ZEROPAD | FLAG_LEFTADJUST)) == FLAG_ZEROPAD))
            {
               tmp = width - prefixLen - noOfLeadingZeros - bufLen;
               if (tmp > 0)
                  noOfLeadingZeros += tmp;
            }
            break;

            case 'f':
            case 'e': case 'E':
            case 'g': case 'G':
#ifdef NO_DOUBLE
               ptr = "(no float support)";
               goto L_s;
#else
               {
                  double value = (flags & FLAG_LONGDOUBLE)
                     ? (double)va_arg(argList, long double)
                     : va_arg(argList, double);
                  int strLen;
                  int decZeros, expZeros, decPoint, fracDigs;
                  int trailZeros, expLen, exp, savedPrec;
                  char *fracStr, *expStr;
                  char cFmt = *fmt;
                  static const char *nanInf[] = { "-NaN", "NaN",
                                                  "-Inf", "Inf" };
                  decZeros   = expZeros = fracDigs = 0;
                  trailZeros = expLen   = exp = 0;
                  savedPrec  = precision;
                  expStr = (char *)"";
                  tmp = isNanOrInf((union UIEEE_754 *)&value);
                  if (tmp)
                  {
                     ptr = nanInf[tmp - 1];
                     goto L_s;
                  }
                  if (value < 0.0)
                  {
                     value = -value;
                     prefix = "-";
                     prefixLen = 1;
                  }
                  else if (flags & FLAG_SIGN)
                  {
                     prefix = "+";
                     prefixLen = 1;
                  }
                  else if (flags & FLAG_SPACE)
                  {
                     prefix = " ";
                     prefixLen = 1;
                  }
                  if (value >= 10.0)
                  {
                     if (value >= 1e16)
                     {
                        while (value >=  1e64) { value /= 1e64; exp += 64; }
                        while (value >=  1e32) { value /= 1e32; exp += 32; }
                        while (value >=  1e16) { value /= 1e16; exp += 16; }
                     }
                     while (value >=  1e08) { value /= 1e08; exp += 8; }
                     while (value >=  1e04) { value /= 1e04; exp += 4; }
                     while (value >=  1e01) { value /= 1e01; exp += 1; }
                  }
                  else if (value < 1.0 && value > 0.0)
                  {
                     if (value < 1e-15)
                     {
                        while (value <  1e-63) { value *= 1e64; exp -= 64; }
                        while (value <  1e-31) { value *= 1e32; exp -= 32; }
                        while (value <  1e-15) { value *= 1e16; exp -= 16; }
                     }
                     while (value <  1e-7) { value *= 1e8; exp -= 8; }
                     while (value <  1e-3) { value *= 1e4; exp -= 4; }
                     while (value <  1e-0) { value *= 1e1; exp -= 1; }
                     if    (value >= 10.0) { value /= 1e1; exp += 1; }
                  }
                  if (precision < 0 )
                     precision = savedPrec = 6;

                  if (cFmt == 'g' || cFmt == 'G')
                  {
                     if (exp < -4  || exp >= precision)
                     {
                        cFmt = (char)(cFmt == 'g' ? 'e' : 'E');
                        --precision;
                     }
                     else
                     {
                        cFmt = 'f';
                        if (precision == 0)
                           precision = 1;
                        if (exp >= 0)
                        {
                           precision -= exp + 1;
                        }
                        else
                        {
                           precision += -exp - 1;
                        }
                     }
                     if (precision < 0)
                        precision = 0;
                  }
                  ptr = (char *)buf;
                  strLen = 0;
                  if (cFmt == 'e' || cFmt == 'E')
                  {
                     tmp = (int)value;
                     buf[strLen++] = (unsigned char)(tmp + '0');
                     value -= tmp;
                     value *= 10.;
                  }
                  else if (exp < 0)
                  {
                     buf[strLen++] = '0';
                  }
                  else
                  {
                     if (exp > XDBL_DIG + 1)
                     {
                        expZeros = exp - (XDBL_DIG + 1);
                        exp = XDBL_DIG + 1;
                     }

                     do
                     {
                        tmp = (int)value;
                        buf[strLen++] = (unsigned char)(tmp + '0');
                        value -= tmp;
                        value *= 10.;
                     } while (exp--);
                  }

                  fracStr = (char *)&buf[strLen];
                  if (cFmt == 'f')
                  {
                     while (decZeros < precision && exp < -1)
                     {
                        ++decZeros;
                        ++exp;
                     }
                  }
                  if (!expZeros)
                  {
                     while (fracDigs + decZeros < precision &&
                            strLen + fracDigs < XDBL_DIG + 1)
                     {
                        tmp = (int)value;
                        fracStr[fracDigs++] = (char)(tmp + '0');
                        value -= tmp;
                        value *= 10.;
                        if (cFmt == 'f')
                           --exp;
                     }
                     if (!fracDigs && decZeros)
                     {
                        fracStr[fracDigs++] = (char)'0';
                        --decZeros;
                        --exp;
                     }
                     if (value >= 5.0 && (fracDigs || !precision))
                     {
                        tmp = strLen + fracDigs - 1;
                        while (buf[tmp] == '9' && tmp >= 0)
                           buf[tmp--] = '0';
                        if (tmp == 0 && decZeros)
                        {
                           for (tmp = strLen + fracDigs - 1 ;
                                tmp > 0 ;
                                tmp--)
                           {
                              buf[tmp] = buf[tmp - 1];
                           }
                           buf[1]++;
                           decZeros--;
                        }
                        else if (tmp >= 0)
                        {
                           if (cFmt == 'f')
                           {
                              if (precision + 1 + exp >= 0)
                              {
                                 buf[tmp]++;
                              }
                           }
                           else if (precision == 0 || fracDigs == precision)
                           {
                              buf[tmp]++;
                           }
                        }
                        else
                        {
                           /* tmp == -1 */
                           buf[0] = '1';
                           buf[strLen + fracDigs] = '0';
                           if (cFmt == 'f')
                           {
                              if (strLen >= savedPrec &&
                                  (*fmt == 'g' || *fmt == 'G'))
                              {
                                 cFmt = (char)(*fmt == 'g' ? 'e' : 'E');
                                 exp = strLen;
                                 strLen = 1;
                              }
                              else
                                 strLen++;
                           }
                           else
                              exp++;
                        }
                     }
                  }
                  if (*fmt == 'g' || *fmt == 'G')
                  {
                     if (flags & FLAG_ALTERNATE)
                     {
                        decPoint = 1;
                     }
                     else
                     {
                        tmp = strLen + fracDigs - 1;
                        while (buf[tmp--] == '0' && fracDigs > 0)
                        {
                           fracDigs--;
                        }
                        if (!fracDigs)
                           decZeros = 0;
                        trailZeros = 0;
                        decPoint = fracDigs ? 1 : 0;
                     }
                     if (exp == 0)
                        goto SkipExp;
                  }
                  else
                  {
                     trailZeros = precision - fracDigs - decZeros;
                     if (trailZeros < 0)
                        trailZeros = 0;
                     decPoint = precision || (flags & FLAG_ALTERNATE) ? 1 : 0;
                  }

                  if (cFmt == 'e' || cFmt == 'E')
                  {
                     expStr = &fracStr[fracDigs];
                     expStr[expLen++] = cFmt;
                     expStr[expLen++] =
                        (char)((exp < 0) ? (exp = -exp), '-':'+');
                     if (exp >= 100)
                     {
                        expStr[expLen++] = (char)(exp / 100 + '0');
                        exp %= 100;
                     }
                     expStr[expLen++] = (char)(exp / 10 + '0');
                     expStr[expLen++] = (char)(exp % 10 + '0');
                  }

                 SkipExp:
                  if ((flags & FLAG_ZEROPAD) && !(flags & FLAG_LEFTADJUST) )
                  {
                     noOfLeadingZeros = width -
                        (prefixLen  + strLen   + expZeros +
                         decPoint   + decZeros + fracDigs +
                         trailZeros + expLen);
                     if (noOfLeadingZeros < 0)
                        noOfLeadingZeros = 0;
                  }
                  else
                     noOfLeadingZeros = 0;

                  width -= prefixLen  + noOfLeadingZeros + strLen   +
                     expZeros   + decPoint     + decZeros +
                     fracDigs   + trailZeros   + expLen;

                  if ( !(flags & FLAG_LEFTADJUST))
                     BufPrint_padChar(o, ' ', width);
                  BufPrint_write(o, prefix, prefixLen);
                  BufPrint_padChar(o, '0', noOfLeadingZeros);
                  BufPrint_write(o, ptr, strLen);
                  BufPrint_padChar(o, '0', expZeros);
                  BufPrint_write(o, ".", decPoint);
                  BufPrint_padChar(o, '0', decZeros);
                  BufPrint_write(o, fracStr, fracDigs);
                  BufPrint_padChar(o, '0', trailZeros);
                  if( (retVal=BufPrint_write(o, expStr, expLen)) != 0)
                     return retVal;
                  if (flags & FLAG_LEFTADJUST)
                     BufPrint_padChar(o, ' ', width);
                  ++fmt;
                  continue;
               }
#endif
            case 'c':
               buf[0] = (unsigned char)va_arg(argList, int);
               ptr = buf;
               bufLen = 1;
               break;
            case 'j':
            case 's':
               ptr = va_arg(argList, char *);
               if (ptr == NULL)
                  ptr = "(null)";
           L_s:
               bufLen = 0;
               while (ptr[bufLen] != '\0')
                  bufLen++;
               if (precision >= 0 && precision < bufLen)
                  bufLen = precision;
               break;
            case '%':
               ptr = "%";
               bufLen = 1;
               break;
            case 'n':
               if (flags & FLAG_SHORT)
                  *va_arg(argList, S16 *) = (S16)outCount;
               else if (flags & FLAG_LONG)
                  *va_arg(argList, long *) = (long)outCount;
               else
                  *va_arg(argList, int *) = (int)outCount;
               fmt++;
               continue;
            default:
               fmt++;
               continue;
         }
         width -= prefixLen + noOfLeadingZeros + bufLen;
         if( !(flags & FLAG_LEFTADJUST))
            BufPrint_padChar(o, ' ', width);
         if( (retVal=BufPrint_write(o, prefix, prefixLen)) !=0 )
            return retVal;
         BufPrint_padChar(o, '0', noOfLeadingZeros);

         if(*fmt++ == 'j')
         {
            BufPrint_jsonString(o, ptr);
         }
         else
         {
            if( (retVal=BufPrint_write(o, ptr, bufLen)) !=0 )
               return retVal;
         }
         if (flags & FLAG_LEFTADJUST)
            BufPrint_padChar(o, ' ', width);
      }
   }
   return outCount;
}


BA_API int
BufPrint_printf(BufPrint* o, const char* fmt, ...)
{
    int retv;
    va_list varg;
    va_start(varg, fmt);
    retv = BufPrint_vprintf(o, fmt, varg);
    va_end(varg);
    return retv;
}

BA_API int
BufPrint_flush(BufPrint* o)
{
   if(!o)
      return -1;
   if(o->cursor)
   {
      int rsp = o->flushCB(o, 0);
      o->cursor=0;
      return rsp;
   }
   return 0;
}
