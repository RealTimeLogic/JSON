/*
 *     ____             _________                __                _
 *    / __ \___  ____ _/ /_  __(_)___ ___  ___  / /   ____  ____ _(_)____
 *   / /_/ / _ \/ __ `/ / / / / / __ `__ \/ _ \/ /   / __ \/ __ `/ / ___/
 *  / _, _/  __/ /_/ / / / / / / / / / / /  __/ /___/ /_/ / /_/ / / /__
 * /_/ |_|\___/\__,_/_/ /_/ /_/_/ /_/ /_/\___/_____/\____/\__, /_/\___/
 *                                                       /____/
 *
 *                  Barracuda Application Server
 ****************************************************************************
 *            PROGRAM MODULE
 *
 *   $Id: JEncoder.c 4331 2018-12-06 20:41:28Z wini $
 *
 *   COPYRIGHT:  Real Time Logic, 2006 - 2018
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




/* 

Remove the dependency on the source file JVal.c by defining:

#define NO_JVAL_DEPENDENCY

The above construction, which is designed for resource limited
devices, enables the use of the JEncoder without requiring JVal.c

The above disables function JEncoder::setJV and the format flag 'J'
for function JEncoder_set/JEncoder_vSetJV

*/





#ifndef BA_LIB
#define BA_LIB 1
#endif

#include <JEncoder.h>
#include <ctype.h>



#define JEncoder_isObject(o)                            \
   ((o)->objectStack.data[((o)->objectStack.level/8)] & \
    (1 << ((o)->objectStack.level%8)))

#define JEncoder_setObject(o)                           \
   (o)->objectStack.data[((o)->objectStack.level/8)] |= \
      (1 << ((o)->objectStack.level%8));

#define JEncoder_clearObject(o)                         \
   (o)->objectStack.data[((o)->objectStack.level/8)] &= \
      ~(1 << ((o)->objectStack.level%8));

static int
JEncoder_setIoErr(JEncoder* o)
{
   JErr_setError(o->err, JErrT_IOErr, "Cannot write");
   return -1;
}

static int
JEncoder_setCommaSep(JEncoder* o)
{
   if(o->startNewObj)
      o->startNewObj = FALSE;
   else
   {
      if(BufPrint_printf(o->out,",")<0)
      {
         JEncoder_setIoErr(o);
         return -1;
      }
   }
   return 0;
}

static BaBool
JEncoder_beginValue(JEncoder* o, BaBool isMemberName)
{
   const char* eMsg=0;

   if(JErr_isError(o->err))
      return FALSE; /* Failed */
   if(JEncoder_isObject(o))
   {
      if(isMemberName) /* If called by method name */
      {
         if(JEncoder_setCommaSep(o)) return FALSE;
         if(o->objectMember)
            eMsg = "Duplicate call to name";
         else
            o->objectMember=1;
      }
      else if(o->objectMember) /* Do we have an object member name */
         o->objectMember = 0; /* reset */
      else
         eMsg = "Invalid fmt. Missing object member name";
   }
   else
   {
      if(JEncoder_setCommaSep(o)) return FALSE;
      if(isMemberName) /* If called by method name */
         eMsg="Invalid fmt in name. Not an object";
   }
   if(eMsg)
   {
      JErr_setError(o->err, JErrT_FmtValErr, eMsg);
      return FALSE;
   }
   return TRUE;
}


void
JEncoder_constructor(JEncoder* o, JErr* err, BufPrint* out)
{
   memset(o, 0, sizeof(JEncoder));
   o->err = err;
   o->out = out;
   o->startNewObj=TRUE;
}

int
JEncoder_flush(JEncoder* o)
{
   if(JErr_noError(o->err))
      return BufPrint_flush(o->out);
   return -1;
}


int
JEncoder_commit(JEncoder* o)
{
   o->startNewObj=TRUE;
   return JEncoder_flush(o);
}

int
JEncoder_setInt(JEncoder* o, S32 val)
{
   if(JEncoder_beginValue(o, FALSE))
   {
      if(BufPrint_printf(o->out, "%d", val)<0)
         return JEncoder_setIoErr(o);
      return 0;
   }
   return -1;
}


int
JEncoder_setLong(JEncoder* o, S64 val)
{
   if(JEncoder_beginValue(o, FALSE))
   {
      if(BufPrint_printf(o->out, "%lld", val)<0)
         return JEncoder_setIoErr(o);
      return 0;
   }
   return -1;
}


#ifndef NO_DOUBLE
int
JEncoder_setDouble(JEncoder* o, double val)
{
   if(JEncoder_beginValue(o, FALSE))
   {
      if(BufPrint_printf(o->out,"%f",val)<0)
         return JEncoder_setIoErr(o);
      return 0;
   }
   return -1;
}
#endif


int
JEncoder_setString(JEncoder* o, const char* val)
{
   if(JEncoder_beginValue(o, FALSE))
   {
      if(val)
      {
         if(BufPrint_jsonString(o->out,val)<0)
            return JEncoder_setIoErr(o);
      }
      else
      {
         if(BufPrint_write(o->out,"null", -1)<0)
            return JEncoder_setIoErr(o);
      }
      return 0;
   }
   return -1;
}


int
JEncoder_b64enc(JEncoder* o, const void* source, S32 slen)
{
   if(JEncoder_beginValue(o, FALSE))
   {
      if(BufPrint_putc(o->out,'"') ||
         BufPrint_b64Encode(o->out,source, slen)<0 ||
            BufPrint_putc(o->out,'"'))
      {
         return JEncoder_setIoErr(o);
      }
   }
   return -1;
}


int
JEncoder_vFmtString(JEncoder* o, const char* fmt,va_list argList)
{
   if(JEncoder_beginValue(o, FALSE))
   {
      if(fmt)
      {
         if(BufPrint_putc(o->out,'"') ||
            BufPrint_vprintf(o->out,fmt,argList)<0 ||
            BufPrint_putc(o->out,'"'))
         {
            return JEncoder_setIoErr(o);
         }
      }
      else
      {
         if(BufPrint_write(o->out,"null", -1)<0)
            return JEncoder_setIoErr(o);
      }
      return 0;
   }
   return -1;
}



int
JEncoder_setBoolean(JEncoder* o, BaBool val)
{
   if(JEncoder_beginValue(o, FALSE))
   {
      if(BufPrint_printf(o->out,"%s",val?"true":"false")<0)
         return JEncoder_setIoErr(o);
      return 0;
   }
   return -1;
}


int
JEncoder_setNull(JEncoder* o)
{
   if(JEncoder_beginValue(o, FALSE))
   {
      if(BufPrint_write(o->out, "null", -1)<0)
         return JEncoder_setIoErr(o);
      return 0;
   }
   return -1;
}


#ifdef NO_JVAL_DEPENDENCY
#define JEncoder_setJV(o,val,x)                                         \
   JErr_setError(o->err, JErrT_FmtValErr, "Feature 'J' Disabled");return -1
#else
int
JEncoder_setJV(JEncoder* o, JVal* val, BaBool iterateNext)
{
   for(; val && JErr_noError(o->err); val=JVal_getNextElem(val))
   {
      if( JVal_isObjectMember(val) && ! o->objectMember )
      {
         JEncoder_setName(o, JVal_getName(val));
      }
      switch(JVal_getType(val))
      {
         case JVType_Null:
            JEncoder_setNull(o);
            break;

         case JVType_Boolean:
            JEncoder_setBoolean(o,JVal_getBoolean(val,o->err));
            break;

         case JVType_Double:
#ifdef NO_DOUBLE
            baAssert(0);
#else
            JEncoder_setDouble(o, JVal_getDouble(val, o->err));
#endif
            break;

         case JVType_Int:
            JEncoder_setInt(o, JVal_getInt(val, o->err));
            break;

         case JVType_Long:
            JEncoder_setLong(o, JVal_getLong(val, o->err));
            break;

         case JVType_String:
            JEncoder_setString(o, JVal_getString(val, o->err));
            break;

         case JVType_Object:
            JEncoder_beginObject(o);
            JEncoder_setJV(o, JVal_getObject(val, o->err),TRUE);
            JEncoder_endObject(o);
            break;

         case JVType_Array:
            JEncoder_beginArray(o);
            JEncoder_setJV(o, JVal_getArray(val, o->err), TRUE);
            JEncoder_endArray(o);
            break;

         default: baAssert(0);
      }
      if( !iterateNext )
         break;
   }
   return JErr_noError(o->err) ? 0 : -1;
}
#endif

int
JEncoder_setName(JEncoder* o, const char* name)
{
   if(JEncoder_beginValue(o, TRUE))
   {
      if(BufPrint_printf(o->out, "\"%s\":", name)<0)
         return JEncoder_setIoErr(o);
      return 0;
   }
   return -1;
}


int
JEncoder_beginObject(JEncoder* o)
{
   if(JEncoder_beginValue(o, FALSE))
   {
      if(o->objectStack.level < (sizeof(o->objectStack.data)*8-1))
      {
         /* Set current stack position to "isObject" */
         o->objectStack.level++;
         JEncoder_setObject(o);
         if(BufPrint_write(o->out, "{", -1)<0)
            return JEncoder_setIoErr(o);
         o->startNewObj=TRUE;
         return 0;
      }
      JErr_setError(o->err, JErrT_FmtValErr,
                    "Object nested too deep");
   }
   return -1;
}


int
JEncoder_endObject(JEncoder* o)
{
   if(JErr_noError(o->err))
   {
      if(o->objectStack.level > 0)
      {
         if(JEncoder_isObject(o))
         {
            if(BufPrint_printf(o->out, "}", -1)<0)
               return JEncoder_setIoErr(o);
            JEncoder_clearObject(o);
            o->objectStack.level--;
            o->startNewObj=FALSE;
            return 0;
         }
         JErr_setError(o->err, JErrT_FmtValErr,
                       "endObject: Not an object");
      }
      else
         JErr_setError(o->err, JErrT_FmtValErr,
                       "endObject: Unrolled too many times");
   }
   return -1;
}


int
JEncoder_beginArray(JEncoder* o)
{
   if(JEncoder_beginValue(o, FALSE))
   {
      if(o->objectStack.level < (sizeof(o->objectStack.data)*8))
      {
         o->objectStack.level++;
         if(BufPrint_printf(o->out, "[", -1)<0)
            return JEncoder_setIoErr(o);
         o->startNewObj=TRUE;
         return 0;
      }
      JErr_setError(o->err, JErrT_FmtValErr,
                    "Array nested too deep");
   }
   return -1;
}


int
JEncoder_endArray(JEncoder* o)
{
   if(JErr_noError(o->err))
   {

      if(o->objectStack.level > 0)
      {
         if( ! JEncoder_isObject(o) )
         {
            if(BufPrint_printf(o->out, "]", -1)<0)
               return JEncoder_setIoErr(o);
            o->objectStack.level--;
            o->startNewObj=FALSE;
            return 0;
         }
         JErr_setError(o->err, JErrT_FmtValErr,
                       "endArray: Is object");
      }
      else
         JErr_setError(o->err, JErrT_FmtValErr,
                       "endArray: Unrolled too many times");
   }
   return -1;
}


static int
JEncoder_setArray(JEncoder* o, const char flag, void* array, int len)
{
   int i;
   JEncoder_beginArray(o);
   for(i = 0 ; i < len ; i++)
   {
      if(JErr_isError(o->err))
         return -1;
      switch(flag)
      {
         case 'b':
            JEncoder_setBoolean(o, ((BaBool*)array)[i]);
            break;
         case 'd':
            JEncoder_setInt(o, ((S32*)array)[i]);
            break;
         case 'f':
            JEncoder_setDouble(o, ((double*)array)[i]);
            break;
         case 's':
            JEncoder_setString(o, ((const char**)array)[i]);
            break;
         case 'J':
            JEncoder_setJV(o, ((JVal**)array)[i], FALSE);
            break;

         default:
            JErr_setError(
               o->err, JErrT_FmtValErr,
               "Unknown or illegal format flag in array flag 'A'");
      }
   }
   JEncoder_endArray(o);
   return 0;
}


int
JEncoder_vSetJV(JEncoder* o, const char** fmt, va_list* argList)
{
   union
   {
      const void* p;
      int len;
   } u;

   for( ; **fmt ; (*fmt)++)
   {
      if(JErr_isError(o->err))
         return -1;
      if(**fmt == '}' || **fmt == ']')
      {
         if(o->objectStack.level == 0)
         {
            JErr_setError(o->err, JErrT_FmtValErr,
                          "JEncoder::set Mismatched ']' or '}'");
            return -1;
         }
         return 0;
      }
      if(JEncoder_isObject(o))
         JEncoder_setName(o,va_arg(*argList, char*));
      switch(**fmt)
      {
         case 'b':
            JEncoder_setBoolean(o, (BaBool)va_arg(*argList, int));
            break;
         case 'd':
            JEncoder_setInt(o, va_arg(*argList, S32));
            break;
         case 'l':
            JEncoder_setLong(o, va_arg(*argList, S64));
            break;
         case 'f':
            JEncoder_setDouble(o, va_arg(*argList, double));
            break;
         case 's':
            JEncoder_setString(o, va_arg(*argList, char*));
            break;
         case 'n':
            JEncoder_setNull(o);
            break;
         case 'J':
            JEncoder_setJV(o, va_arg(*argList, JVal*), FALSE);
            break;
         case 'A':
            u.len = va_arg(*argList, int);
            JEncoder_setArray(
               o,*++(*fmt),va_arg(*argList,void*),u.len);
            break;
         case '[':
            (*fmt)++;
            JEncoder_beginArray(o);
            JEncoder_vSetJV(o, fmt, argList);
            JEncoder_endArray(o);
            if(**fmt != ']')
            {
               JErr_setError(
                  o->err, JErrT_FmtValErr,
                  "JEncoder::set End of array flag ']' not found");
               return -1;
            }
            break;
         case '{':
            (*fmt)++;
            JEncoder_beginObject(o);
            JEncoder_vSetJV(o, fmt, argList);
            JEncoder_endObject(o);
            if(**fmt != '}')
            {
               JErr_setError(
                  o->err, JErrT_FmtValErr,
                  "JEncoder::set End of object flag '}' not found");
               return -1;
            }
            break;

         default:
            JErr_setError(o->err, JErrT_FmtValErr,
                          "JEncoder::set Unknown format flag");
      }

   }
   return JErr_noError(o->err) ? 0 : -1;
}


int
JEncoder_set(JEncoder* o, const char* fmt, ...)
{
   int retVal;
   va_list varg;
   va_start(varg, fmt);
   retVal = JEncoder_vSetJV(o, &fmt, &varg);
   if(retVal) /* Can only set error once. Just in case not set */
      JErr_setError(o->err, JErrT_FmtValErr, "?");
   va_end(varg);
   return retVal;
}
