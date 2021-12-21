/*
 *     ____             _________                __                _     
 *    / __ \___  ____ _/ /_  __(_)___ ___  ___  / /   ____  ____ _(_)____
 *   / /_/ / _ \/ __ `/ / / / / / __ `__ \/ _ \/ /   / __ \/ __ `/ / ___/
 *  / _, _/  __/ /_/ / / / / / / / / / / /  __/ /___/ /_/ / /_/ / / /__  
 * /_/ |_|\___/\__,_/_/ /_/ /_/_/ /_/ /_/\___/_____/\____/\__, /_/\___/  
 *                                                       /____/          
 *
 ****************************************************************************
 *            PROGRAM MODULE
 *
 *   $Id: JDecoder.c 4914 2021-12-01 18:24:30Z wini $
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

*/

#include "JDecoder.h"
#include <string.h>

#define JDecoderV_val2Ix(o, val) (U16)(((U8*)val) - o->buf)
#define JDecoderV_ix2Val(o, ix) (JDecoderV*)(o->buf + ix)


static int
JDecoder_setStatus(JDecoder* o, JDecoderS s)
{
   o->status = s;
   return -1;
}


static int
JDecoder_buildValCB(JParserIntf* super, JParserVal* v, int reclevel)
{
   JDecoderV* dvp; /* parent */
   JDecoderV* dv; /* child */
   JDecoder* o = (JDecoder*)super;
   JDecoderStackNode* sn;
   int len;
   if(o->status)
	   return o->status;
   if(o->pIntf)
   {
      baAssert(reclevel >= o->startServiceLevel);
      if(JParserIntf_serviceCB(o->pIntf, v, reclevel - o->startServiceLevel))
         return JDecoder_setStatus(o, JDecoderS_ChainedErr);
      if((v->t == JParserT_EndObject || v->t == JParserT_EndArray) &&
         reclevel == o->startServiceLevel)
      {
         o->pIntf = 0;
      }
      return 0;
   }
   if(v->t == JParserT_EndObject || v->t == JParserT_EndArray)
   {
      sn = o->stack + reclevel;
      dvp = JDecoderV_ix2Val(o, sn->contIx);
      if(dvp->u.child.firstIx)
         return JDecoder_setStatus(o, JDecoderS_Overflow);
      return 0;
   }
   else
   {
      if( ! reclevel )
      {
         baAssert(v->t == JParserT_BeginObject || v->t == JParserT_BeginArray);
         return 0;
      }
      sn = o->stack + reclevel - 1;
      dvp = JDecoderV_ix2Val(o, sn->contIx); /* parent */
      if( ! dvp->u.child.firstIx )
         return JDecoder_setStatus(o, JDecoderS_Underflow);
   }
   dv = JDecoderV_ix2Val(o, dvp->u.child.firstIx); /* first child */
   if(*v->memberName) /* if member in object */
   {
      baAssert(sn->isObj);
      dvp=0;
      while( strcmp(v->memberName, dv->name) )
      {
         if( ! dv->nextIx )
            return JDecoder_setStatus(o, JDecoderS_NameNotFound);
         dvp = dv;
         dv = JDecoderV_ix2Val(o, dv->nextIx);
      }
   }
   else
   {
      baAssert( ! sn->isObj || reclevel == 0 );
   }
   switch(v->t)
   {
      case JParserT_BeginObject:
      case JParserT_BeginArray:
         if(dv->t != v->t)
            return JDecoder_setStatus(o, JDecoderS_FormatErr);
         baAssert(reclevel < o->stacklen);
         if(dv->u.child.firstIx)
         {
            JDecoderV* child = JDecoderV_ix2Val(o, dv->u.child.firstIx);
            if(child->t == 'X')
            {
               o->pIntf = child->u.pIntf;
               o->startServiceLevel = reclevel;
               if(JParserIntf_serviceCB(o->pIntf, v, 0))
                  return JDecoder_setStatus(o, JDecoderS_ChainedErr);
            }
         }
         o->stack[reclevel].contIx = JDecoderV_val2Ix(o, dv);
         o->stack[reclevel].isObj = v->t == JParserT_BeginObject;
         break;

      case JParserT_Boolean:
         if(dv->t != JParserT_Boolean)
            return JDecoder_setStatus(o, JDecoderS_FormatErr);
         *dv->u.b = v->v.b;
         break;

#ifdef NO_DOUBLE
      case JParserT_Double:
         return JDecoder_setStatus(o, JDecoderS_FormatErr);
#else
      case JParserT_Double:
         if(dv->t == JParserT_Double)
            *dv->u.f = (double)v->v.f;
         else
            return JDecoder_setStatus(o, JDecoderS_FormatErr);
         break;

#endif
      case JParserT_Int:
         if(dv->t == JParserT_Int)
            *dv->u.d = v->v.d;
         else if(dv->t == JParserT_Long)
            *dv->u.l = v->v.d;
#ifndef NO_DOUBLE
         else if(dv->t == JParserT_Double)
            *dv->u.f = v->v.d;
#endif
         else
            return JDecoder_setStatus(o, JDecoderS_FormatErr);
         break;

      case JParserT_Long:
         if(dv->t != JParserT_Long)
            return JDecoder_setStatus(o, JDecoderS_FormatErr);
         *dv->u.l = v->v.l;
         break;

      case JParserT_Null:
         if(dv->t != JParserT_String)
            return JDecoder_setStatus(o, JDecoderS_FormatErr);
         *dv->u.s = 0;
         break;

      case JParserT_String: /* Fall through */
         len = strlen(v->v.s);
         if( (len + 1) >= dv->sSize)
            return JDecoder_setStatus(o, JDecoderS_StringOverflow);
         strcpy(dv->u.s, v->v.s);
         break;

      default:
         baAssert(0);
   }

   /* unlink 'dv' */
   if(*v->memberName)
   {
      if(dvp)
         dvp->nextIx = dv->nextIx;
      else
      {
         dvp = JDecoderV_ix2Val(o, sn->contIx); /* parent */
         dvp->u.child.firstIx = dv->nextIx;
      }
   }
   else
   {
      dvp->u.child.firstIx = dv->nextIx;
   }
   return 0;
}


static int
JDecoder_expandBuf(JDecoder* o)
{
   (void)o;
   return -1; /* not implemented */
}


static void
JDecoder_link(JDecoder* o, JDecoderStackNode* sn, JDecoderV* v)
{
   JDecoderV* parent = JDecoderV_ix2Val(o, sn->contIx);
   baAssert(parent->t == '{' || parent->t == '[');
   if(parent->u.child.firstIx)
   {
      JDecoderV* vtmp = JDecoderV_ix2Val(o, parent->u.child.lastIx);
      parent->u.child.lastIx = vtmp->nextIx = JDecoderV_val2Ix(o,v);
   }
   else
      parent->u.child.firstIx = parent->u.child.lastIx = JDecoderV_val2Ix(o,v);
}



int
JDecoder_vget(JDecoder* o, const char* fmt, va_list* argList)
{
   int stackIx = -1;
   JDecoderV* v;
   JDecoderStackNode* sn = o->stack;
   o->status = JDecoderS_OK;
   o->pIntf=0;
   o->bufIx=0;

   if(J_POINTER_NOT_ALIGNED(o->buf))
      return JDecoder_setStatus(o, JDecoderS_BufNotAligned);

   if(*fmt != '{' && *fmt != '[')
      return JDecoder_setStatus(o, JDecoderS_Unbalanced);
   sn->isObj = *fmt == '{';
   while(*fmt)
   {
      if(*fmt == '}' || *fmt == ']')
      {
         if(stackIx == 0)
         {
            if(fmt[1])
               return JDecoder_setStatus(o, JDecoderS_Unbalanced);
            break;
         }
         stackIx--;
         sn--;
         fmt++;
         continue;
      }
      v = JDecoderV_ix2Val(o, o->bufIx);
      o->bufIx += sizeof(JDecoderV);
      if(o->bufIx >= o->bufSize)
      {
         if(stackIx == 0 && (*fmt == '}' || *fmt == ']'))
            return JDecoder_setStatus(o, JDecoderS_Unbalanced);
         if(JDecoder_expandBuf(o))
            return -1;
         sn = o->stack+stackIx;
      }
      v->t = *fmt;
      v->name = stackIx >= 0 && sn->isObj && *fmt != 'X' ?
         va_arg(*argList, const char*) : "";
      v->nextIx = 0;
      switch(*fmt)
      {
         case 'X': /* A child JParserIntf_Service */
            if(fmt[1] != '}' && fmt[1] != ']')
               return JDecoder_setStatus(o, JDecoderS_FormatErr);
         case 'b': /* bool */
         case 'd': /* d */
         case 'l': /* l */
         case 'f': /* f */
         case 's': /* s */
            v->u.b = va_arg(*argList, U8*);
            if(v->t == 's')
               v->sSize = (S32)va_arg(*argList, size_t);
            JDecoder_link(o, sn, v);
            break;
            
         case '{':
         case '[':
            if(stackIx >= 0)
               JDecoder_link(o, sn, v);
            if(++stackIx >= o->stacklen)
               return JDecoder_setStatus(o, JDecoderS_Overflow);
            v->u.child.firstIx = 0;
            sn = o->stack+stackIx;
            sn->isObj = *fmt == '{';
            sn->contIx = JDecoderV_val2Ix(o, v);
            break;

         default:
            return JDecoder_setStatus(o, JDecoderS_Unknown);
      }
      fmt++;
   }
   if( stackIx || fmt[1] )
      return JDecoder_setStatus(o, JDecoderS_Unbalanced);
   return 0;
}



int
JDecoder_get(JDecoder* o, const char* fmt, ...)
{
   int status;
   va_list argList;
   va_start(argList, fmt);
   status = JDecoder_vget(o, fmt, &argList);
   va_end(argList);
   return status;
}


void
JDecoder_constructor(JDecoder* o, U8* buf, int bufSize, int extraStackLen)
{
   JParserIntf_constructor((JParserIntf*)o, JDecoder_buildValCB);
   o->buf=buf;
   o->bufSize = bufSize;
   o->stacklen = JPARSER_STACK_LEN + extraStackLen;
}
