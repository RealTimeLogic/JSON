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
 *   $Id: JVal.c 4331 2018-12-06 20:41:28Z wini $
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
 *               http://www.realtimelogic.com
 ****************************************************************************
 *
 */

#ifndef BA_LIB
#define BA_LIB 1
#endif
#include <JVal.h>
#include <string.h>


/****************************************************************************
                                JVal
 JSON value
 ****************************************************************************/

static void JVal_extractArray(
   JVal* o,JErr* err,const char flag,void* array,int len);
static void JVal_extractObject(
   JVal* o,JErr* err,const char** fmt,va_list* argList);
static JVal* JVal_extract(
   JVal* o,JErr* err,const char** fmt, va_list* argList);



static int
JVal_init(JVal* o, JVal* parent, JParserVal* pv, AllocatorIntf* dAlloc)
{
   memset(o, 0, sizeof(JVal));
   if(*pv->memberName)
   {
      o->memberName = baStrdup2(dAlloc, pv->memberName);
      if( ! o->memberName )
         return -1;
   }
   switch(pv->t)
   {
      case JParserT_String:
         o->type = JVType_String;
         o->v.s = (U8*)baStrdup2(dAlloc, (char*)pv->v.s);
         if( ! o->v.s )
         {
            if(o->memberName)
               AllocatorIntf_free(dAlloc, o->memberName);
            return -1;
         }
         break;
      case JParserT_Double:
#ifdef NO_DOUBLE
         baAssert(0);
#else
         o->v.f=pv->v.f;
#endif
         o->type = JVType_Double;
         break;
      case JParserT_Int:
         o->v.d=pv->v.d;
         o->type = JVType_Int;
         break;
      case JParserT_Long:
         o->v.l=pv->v.l;
         o->type = JVType_Long;
         break;
      case JParserT_Boolean:
         o->v.b=pv->v.b;
         o->type = JVType_Boolean;
         break;
      case JParserT_Null:
         o->type = JVType_Null;
         break;
      case JParserT_BeginObject:
         o->type = JVType_Object;
         break;
      case JParserT_BeginArray:
         o->type = JVType_Array;
         break;
     default:
         baAssert(0);
   }
   if(parent)
   {
      if(parent->v.firstChild)
      {
         JVal* iter = parent->v.firstChild;
         while(iter->next)
            iter = iter->next;
         iter->next = o;
      }
      else
         parent->v.firstChild = o;
   }
   return 0;
}


static int
JVal_extractValue(JVal* o,
                  JErr* err,
                  const char** fmt,
                  va_list* argList)
{
   union
   {
         BaBool* b;
         S32* d;
         S64* l;
         double* f;
         const char** s;
         void* p;
         JVal** j;
   } u;
   switch(**fmt)
   {
      case 'b':
         u.b = va_arg(*argList, BaBool*);
         *u.b = JVal_getBoolean(o, err);
         return 0;
      case 'd':
         u.d = va_arg(*argList, S32*);
         *u.d = JVal_getInt(o, err);
         return 0;
      case 'l':
         u.l = va_arg(*argList, S64*);
         *u.l = JVal_getLong(o, err);
         return 0;
#ifndef NO_DOUBLE
      case 'f':
         u.f = va_arg(*argList, double*);
         *u.f = JVal_getDouble(o, err);
         return 0;
#endif
      case 's':
         u.s = va_arg(*argList, const char**);
         *u.s = JVal_getString(o, err);
         return 0;
      case 'J':
         u.j = va_arg(*argList, JVal**);
         *u.j = o;
         return 0;
      case 'A':
         u.p = va_arg(*argList, void*);
         JVal_extractArray(JVal_getArray(o,err),
                           err,
                           *++(*fmt),
                           u.p,
                           va_arg(*argList, int));
         return 0;
      case '[':
         (*fmt)++;
         JVal_extract(JVal_getArray(o,err),err,fmt,argList);
         if(**fmt != ']')
         {
            JErr_setError(err, JErrT_FmtValErr,
                          "End of array flag ']' not found");
            return 1;
         }
         return 0;
      case ']':
         return 1;
      case '{':
         (*fmt)++;
         JVal_extractObject(
            JVal_getObject(o,err),err,fmt,argList);
         return 0;
      case '}':
         return 1;

      default:
         JErr_setError(err, JErrT_FmtValErr,
                       "Unknown format flag");
   }
   return -1;
}


static void
JVal_extractArray(JVal* o,
                  JErr* err,
                  const char flag,
                  void* array,
                  int len)
{
   int i;
   if(!o)
      return;
   for(i = 0 ; i < len ; i++)
   {
      if(!o)
         JErr_setTooFewParams(err);
      if(JErr_isError(err))
         return;
      switch(flag)
      {
         case 'b':
            ((BaBool*)array)[i] = JVal_getBoolean(o, err);
            break;
         case 'd':
            ((S32*)array)[i] = JVal_getInt(o, err);
            break;
         case 'l':
            ((S64*)array)[i] = JVal_getLong(o, err);
            break;
#ifndef NO_DOUBLE
         case 'f':
            ((double*)array)[i] = JVal_getDouble(o, err);
            break;
#endif
         case 's':
            ((const char**)array)[i] = JVal_getString(o, err);
            break;
         case 'J':
            ((JVal**)array)[i] = o;
            break;

        default:
            JErr_setError(
               err, JErrT_FmtValErr,
               "Unknown or illegal format flag in array flag 'A'");
      }
      o = JVal_getNextElem(o);
   }
}


static void
JVal_extractObject(JVal* o,JErr* err,const char** fmt,va_list* argList)
{
   if(!o)
      return;
   for( ; **fmt && **fmt != '}' && JErr_noError(err) ; (*fmt)++)
   {
      const char* n;
      const char* name = va_arg(*argList, const char*);
      JVal* iter = o;
      while(iter && (n = JVal_getName(iter))!=0 && strcmp(name,n) )
         iter = JVal_getNextElem(iter);
      if(!iter)
      {
         JErr_setError(err, JErrT_InvalidMethodParams,
                       "Member name not found in object");
         return;
      }
      if(JVal_extractValue(iter,
                           err,
                           fmt,
                           argList))
      {
         JErr_setError(
            err, JErrT_FmtValErr,
            "Detected mismatched object and array format flags");
         return;
      }

   }
   if(**fmt != '}')
   {
      JErr_setError(
         err, JErrT_FmtValErr,
         "Format error: End of object '}' not found");
   }
}


static JVal*
JVal_extract(JVal* o,JErr* err,const char** fmt, va_list* argList)
{
   for( ; **fmt ; (*fmt)++)
   {
      if(JErr_isError(err))
         return 0;
      if(JVal_extractValue(o, err, fmt, argList))
         break;
      if(!o)
      {
         JErr_setTooFewParams(err);
         return 0;
      }
      o = JVal_getNextElem(o);
   }
   return o;
}


JVal*
JVal_vget(JVal* o,JErr* err,const char** fmt, va_list* argList)
{
   o = JVal_extract(o,err,fmt,argList);
   if(**fmt)
   {
      JErr_setTooFewParams(err);
      return 0;
   }
   return o;
}



JVal*
JVal_get(JVal* o, JErr* err, const char* fmt, ...)
{
   JVal* retVal;
   va_list varg;
   va_start(varg, fmt);
   retVal = JVal_vget(o, err, &fmt, &varg);
   va_end(varg);
   return retVal;
}

S32
JVal_getInt(JVal* o, JErr* e)
{
   if(o)
   {
      if(o->type == JVType_Int)
         return o->v.d;
#ifndef NO_DOUBLE
      if(o->type == JVType_Double)
         return (S32)o->v.f;
#endif
      if(o->type == JVType_Long)
         return (S32)o->v.l;
      else if(o->type == JVType_Boolean)
         return (S32)o->v.b;
      else if(o->type == JVType_Null)
         return 0;
      JErr_setTypeErr(e, JVType_Int, o->type);
   }
   else
      JErr_setTooFewParams(e);
   return 0;
}


S64
JVal_getLong(JVal* o, JErr* e)
{
   if(o)
   {
      if(o->type == JVType_Long)
         return o->v.l;
#ifndef NO_DOUBLE
      if(o->type == JVType_Double)
         return (S64)o->v.f;
#endif
      if(o->type == JVType_Int)
         return (S64)o->v.d;
      else if(o->type == JVType_Boolean)
         return (S64)o->v.b;
      else if(o->type == JVType_Null)
         return 0;
      JErr_setTypeErr(e, JVType_Long, o->type);
   }
   else
      JErr_setTooFewParams(e);
   return 0;
}

#ifndef NO_DOUBLE
double
JVal_getDouble(JVal* o, JErr* e)
{
   if(o)
   {
      if(o->type == JVType_Double)
         return o->v.f;
      if(o->type == JVType_Int)
         return (double)o->v.d;
      if(o->type == JVType_Long)
         return (double)o->v.l;
      else if(o->type == JVType_Boolean)
         return (double)o->v.b;
      else if(o->type == JVType_Null)
         return 0;
      JErr_setTypeErr(e, JVType_Double, o->type);
   }
   else
      JErr_setTooFewParams(e);
   return 0;
}
#endif

BaBool
JVal_getBoolean(JVal* o, JErr* e)
{
   if(o)
   {
      if(o->type == JVType_Boolean)
         return o->v.b;
      else if(o->type == JVType_Null)
         return FALSE;
      JErr_setTypeErr(e, JVType_Boolean, o->type);
   }
   else
      JErr_setTooFewParams(e);
   return FALSE;
}


const char*
JVal_getString(JVal* o, JErr* e)
{
   if(o)
   {
      if(o->type == JVType_String)
         return (char*)o->v.s;
      else if(o->type == JVType_Null)
         return 0;
      JErr_setTypeErr(e, JVType_String, o->type);
   }
   else
      JErr_setTooFewParams(e);
   return 0;
}


char*
JVal_manageString(JVal* o, JErr* e)
{
   if(o)
   {
      if(o->type == JVType_String)
      {
         char* ptr = (char*)o->v.s;
         o->v.s=0;
         return ptr;
      }
      JErr_setTypeErr(e, JVType_String, o->type);
   }
   else
      JErr_setTooFewParams(e);
   return 0;
}

const char*
JVal_getName(JVal* o)
{
   return o ? o->memberName : 0;
}

char*
JVal_manageName(JVal* o)
{
   if(o)
   {
      char* ptr = o->memberName;
      o->memberName=0;
      return ptr;
   }
   return 0;
}

JVal*
JVal_getObject(JVal* o, JErr* e)
{
   if(o)
   {
      if(o->type == JVType_Object)
         return o->v.firstChild;
      JErr_setTypeErr(e, JVType_Object, o->type);
   }
   else
      JErr_setTooFewParams(e);
   return 0;
}

JVal*
JVal_getArray(JVal* o, JErr* e)
{
   if(o)
   {
      if(o->type == JVType_Array)
         return o->v.firstChild;
      JErr_setTypeErr(e, JVType_Array, o->type);
   }
   else
      JErr_setTooFewParams(e);
   return 0;
}


JVal*
JVal_getJ(JVal* o, JErr* e)
{
   if(o)
   {
      if(o->type == JVType_Array || o->type == JVType_Object)
         return o->v.firstChild;
      JErr_setTypeErr(e, JVType_Object, o->type);
   }
   else
      JErr_setTooFewParams(e);
   return 0;
}


JVal*
JVal_manageJ(JVal* o, JErr* e)
{
   if(o)
   {
      if(o->type == JVType_Array || o->type == JVType_Object)
      {
         JVal* retVal = o->v.firstChild;
         o->v.firstChild=0;
         return retVal;
      }
      JErr_setTypeErr(e, JVType_Object, o->type);
   }
   else
      JErr_setTooFewParams(e);
   return 0;
}


S32
JVal_getLength(struct JVal* o, JErr* e)
{
   o = JVal_getJ(o, e);
   if(o)
   {
         S32 len=1;
         while( (o = JVal_getNextElem(o)) != 0)
            len++;
         return len;
   }
   return 0; /* NULL means array/object is zero in length */
}


void
JVal_setX(JVal* o, JErr* e, JVType t, void* v)
{
  if(o->type == JVType_Array || o->type == JVType_Object)
  {
    JErr_setTypeErr(e, o->type, t);
    return;
  }
  if(o->type == JVType_String && o->v.s)
  {
    JErr_setError(e,JErrT_MemErr, "JVal::setX string must be managed");
    return;
  }
  o->type = t;
  switch(t)
  {
         case JVType_String:
            o->v.s=(U8*)v;
            break;
         case JVType_Double:
#ifdef NO_DOUBLE
            o->v.l=*((S64*)v);
#else
            o->v.f=*((double*)v);
#endif
            break;
         case JVType_Int:
            o->v.d=*((S32*)v);
            break;
         case JVType_Long:
            o->v.l=*((S64*)v);
            break;
         case JVType_Boolean:
            o->v.b = *((BaBool*)v) ? TRUE : FALSE;
            break;
         case JVType_Null:
            break;
         default:
            baAssert(0);

   }
}


int
JVal_unlink(JVal* o, JVal* child)
{
   if( (o->type == JVType_Object || o->type == JVType_Array) &&
       o->v.firstChild )
   {
      JVal* iter;
      if(child == o->v.firstChild)
      {
         o->v.firstChild = child->next;
         child->next=0;
         return 0;
      }
      iter = o->v.firstChild;
      while(iter->next && iter->next != child)
        iter = iter->next;
      if(iter->next)
      {
         iter->next = child->next;
         child->next=0;
         return 0;
      }
   }
   return -1;
}

static int
JVal_addChild(JVal* o, JVal* child)
{
   if(child->next)
      return -1;
   child->next=o->v.firstChild;
   o->v.firstChild=child;
   return 0;
}


int
JVal_addMember(JVal* o, JErr* e, const char* memberName,
               JVal* child, AllocatorIntf* dAlloc)
{
   if(JErr_noError(e))
   {
      if(o->type == JVType_Object)
      {
         if( ! child->memberName )
         {
            child->memberName =
               dAlloc ? baStrdup2(dAlloc, memberName) : (char*)memberName;
         }
         if(child->memberName)
            return JVal_addChild(o, child);
         JErr_setError(e,JErrT_MemErr,0);
      }
      else
         JErr_setTypeErr(e, JVType_Int, o->type);
   }
   return -1;
}


int
JVal_add(JVal* o, JErr* e, JVal* child)
{
   if(JErr_noError(e))
   {
      if(o->type == JVType_Array)
         return JVal_addChild(o, child);
      JErr_setTypeErr(e, JVType_Int, o->type);
   }
   return -1;
}


void
JVal_terminate(JVal* o, AllocatorIntf* vAlloc, AllocatorIntf* dAlloc)
{
   while(o)
   {
      JVal* next = o->next;
      if(o->type == JVType_Object || o->type == JVType_Array)
         JVal_terminate(o->v.firstChild, vAlloc, dAlloc);
      else if(o->type == JVType_String)
         AllocatorIntf_free(dAlloc, o->v.s);
      if(o->memberName)
         AllocatorIntf_free(dAlloc, o->memberName);
      AllocatorIntf_free(vAlloc, o);
      o = next;
   }
}



/****************************************************************************
                                JParserValFact
 JSON Value Factory for JParser
 ****************************************************************************/

static int
JParserValFact_expandStack(JParserValFact* o)
{
   size_t newSize = (o->vStackSize + 32) * sizeof(void*);
   JVal** v = o->vStack ?
      AllocatorIntf_realloc(o->dAlloc, o->vStack, &newSize) :
      AllocatorIntf_malloc(o->dAlloc, &newSize);
   if(v)
   {
      o->vStack = v;
      o->vStackSize=(int)(newSize/sizeof(void*));
      return 0;
   }
   o->status=JParserValFactStat_DMemErr;
   return -1;
}


static int
JParserValFact_service(JParserIntf* super, JParserVal* pv, int recLevel)
{
   JVal* v;
   size_t jValSize = sizeof(JVal);
   JParserValFact* o = (JParserValFact*)super; /* upcast from base class */
   if(pv->t == JParserT_EndObject || pv->t == JParserT_EndArray)
      return 0; /* Nothing to do */
   if(++o->nodeCounter >= o->maxNodes)
   {
      o->status=JParserValFactStat_MaxNodes;
      return -1;
   }
   if(recLevel >= o->vStackSize && JParserValFact_expandStack(o))
      return -1;
   if( (v = AllocatorIntf_malloc(o->vAlloc, &jValSize)) == 0 )
   {
      o->status=JParserValFactStat_VMemErr;
      return -1;
   }
   if(JVal_init(v, recLevel ? o->vStack[recLevel-1] : 0, pv, o->dAlloc))
   {
      o->status=JParserValFactStat_VMemErr;
      AllocatorIntf_free(o->vAlloc, v);
      return -1;
   }
   if(pv->t == JParserT_BeginObject || pv->t == JParserT_BeginArray)
      o->vStack[recLevel] = v;
   return 0;
}


void
JParserValFact_constructor(
   JParserValFact* o, AllocatorIntf* vAlloc, AllocatorIntf* dAlloc)
{
   memset(o, 0, sizeof(JParserValFact));
   o->vAlloc=vAlloc;
   o->dAlloc=dAlloc;
   JParserIntf_constructor((JParserIntf*)o, JParserValFact_service);
   o->maxNodes=~(U32)0;
}


JVal*
JParserValFact_manageFirstVal(JParserValFact* o)
{
   if(o->vStack && *o->vStack)
   {
      JVal* v = *o->vStack;
      *o->vStack = 0;
      o->nodeCounter=0;
      return v;
   }
   return 0;
}


void
JParserValFact_termFirstVal(JParserValFact* o)
{
   if(o->vStack)
   {
      if(o->vStack)
      {
         JVal_terminate(*o->vStack, o->vAlloc, o->dAlloc);
         *o->vStack=0;
         o->nodeCounter=0;
      }
      AllocatorIntf_free(o->dAlloc, o->vStack);
      o->vStack=0;
      o->vStackSize=0;
   }
}


void
JParserValFact_destructor(JParserValFact* o)
{
   JParserValFact_termFirstVal(o);
}



/****************************************************************************
                                JValFact
 JSON Value Factory
 ****************************************************************************/

void
JValFact_constructor(JValFact* o, AllocatorIntf* vAlloc, AllocatorIntf* dAlloc)
{
   memset(o, 0, sizeof(JValFact));
   o->vAlloc=vAlloc;
   o->dAlloc=dAlloc;
}


JVal*
JValFact_mkVal(JValFact* o, JVType t, const void* uv)
{
   size_t size=sizeof(JVal);
   JVal* v = (JVal*)AllocatorIntf_malloc(o->vAlloc,&size);
   if(v)
   {
      memset(v,0,sizeof(JVal));
      v->type = t;
      switch(t)
      {
         case JVType_String:
            v->v.s=(U8*)baStrdup2(o->dAlloc, (const char*)uv);
            if(!v->v.s)
               t = JVType_InvalidType;
            break;
         case JVType_Double:
#ifdef NO_DOUBLE
            v->v.l=*((S64*)uv);
#else
            v->v.f=*((double*)uv);
#endif
            break;
         case JVType_Int:
            v->v.d=*((S32*)uv);
            break;
         case JVType_Long:
            v->v.l=*((S64*)uv);
            break;
         case JVType_Boolean:
            v->v.b = *((BaBool*)uv) ? TRUE : FALSE;
            break;
         case JVType_Null:
         case JVType_Object:
         case JVType_Array:
            break;
         default:
            baAssert(0);
            t = JVType_InvalidType;
      }
      if(t != JVType_InvalidType)
         return v;
      AllocatorIntf_free(o->vAlloc, v);
   }
   return 0;
}
