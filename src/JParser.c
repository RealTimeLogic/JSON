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
 *   $Id: JParser.c 4110 2017-09-13 16:53:07Z wini $
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

/*
  JSON stream parser.
  http://json.org/
*/
#ifndef BA_LIB
#define BA_LIB 1
#endif

#include <stdlib.h>  /* atof */
#include <JParser.h>
#include <string.h>
#include <ctype.h>

#ifndef bTolower
#define bTolower tolower
#endif
#ifndef bIsspace
#define bIsspace isspace 
#endif


static const char jsonNumberChars[] = {"0123456789.+-eE"};

static const U8 trueString[] = {"rue"};
static const U8 falseString[] = {"alse"};
static const U8 nullString[] = {"ull"};

#define hexdigit(x) (((x) <= '9') ? (x) - '0' : ((x) & 7) + 9)


/****************************************************************************
                                JErr
 JSON error message container
 ****************************************************************************/

int
JErr_setTooFewParams(JErr* o)
{
   return JErr_setError(o,JErrT_InvalidMethodParams,"Too few parameters");
}

int
JErr_setTypeErr(JErr* o, JVType expT, JVType recT)
{
   if(o && JErr_noError(o))
   {
      o->expType=expT;
      o->recType=recT;
      o->err=JErrT_WrongType;
      o->msg="Type not expected";
      return 0;
   }
   return -1;
}

int
JErr_setError(JErr* o,JErrT err,const char* msg)
{
   if(JErr_noError(o))
   {
      o->err = err;
      o->msg=msg;
      return 0;
   }
   return -1;
}



/****************************************************************************
                                JDBuf
 JSON Dynamic Buffer
 ****************************************************************************/

#define JDBuf_reset(o) do {                     \
      (o)->index=0;                             \
   } while(0)

#define JDBuf_expandIfNeeded(o, neededSize)                     \
   ((o)->index+neededSize) > (o)->size && JDBuf_expand(o)


#define JDBuf_destructor(o) do {                                    \
      if((o)->buf) AllocatorIntf_free((o)->alloc, (o)->buf);        \
   }while(0)


static void
JDBuf_constructor(JDBuf* o, AllocatorIntf* alloc)
{
   memset(o,0,sizeof(JDBuf));
   o->alloc=alloc;
}

/* Used exclusively by macro expandIfNeeded */
static int
JDBuf_expand(JDBuf* o)
{
   /*  Must be large enough to hold the largest number possible as a string */
   size_t expandSize = 256;
   if( ! o->alloc )
      return -1;
   if(o->buf)
   {
      U8* ptr;
      o->size += expandSize;
      ptr = AllocatorIntf_realloc(o->alloc, o->buf, &o->size);
      if(ptr)
      {
         o->buf = ptr;
         return 0;
      }
      AllocatorIntf_free(o->alloc, o->buf);
   }
   else
   {
      o->buf = AllocatorIntf_malloc(o->alloc, &expandSize);
      if(o->buf)
      {
         o->size = expandSize;
         return 0;
      }
   }
   o->buf=0;
   o->size=0;
   o->index=0;
   return -1;
}


/****************************************************************************
                                JLexer
 JSON Lexer
 ****************************************************************************/



static void
JLexer_setString(JLexer* o, JParserVal* v)
{
   v->v.s = (char*)o->asmB->buf;
   v->t = JParserT_String;
}


static void
JLexer_setNumber(JLexer* o, JParserVal* v)
{
   JDBuf* asmB = o->asmB;
   baAssert(asmB->buf);
   if(o->isDouble)
   {
#ifdef NO_DOUBLE
      U8* ptr = asmB->buf;
      while(ptr && *ptr!='.' && *ptr!='e' && *ptr=='E')
         ptr++;
      *ptr=0;
      asmB->index = ptr - asmB->buf;
      goto L_int;
#else
      v->v.f=atof((char*)asmB->buf);
      v->t=JParserT_Double;
      if(o->sn)
         v->v.f = -v->v.f;
#endif
   }
   else
   {
#ifdef NO_DOUBLE
     L_int:
#endif
      v->t = JParserT_Int;
      if(asmB->index > 9)
      {
         S64 l = S64_atoll((char*)asmB->buf);
         S32 lsw = (S32)l;
         if(0xFFFFFFFF00000000 & l || lsw < 0)
         {
             v->v.l = o->sn ? -l : l;
             v->t=JParserT_Long;
         }
         else
            v->v.d = (S32)l;
      }
      else
         v->v.d = (S32)U32_atoi((char*)asmB->buf);
      if(v->t == JParserT_Int && o->sn)
            v->v.d = -v->v.d;
   }
   o->isDouble=0;
}



static int
JLexer_setValue(JLexer* o, JLexerT t, JParserVal* v)
{
   switch(t)
   {
      case JLexerT_Null:
         v->t = JParserT_Null;
         break;
      case JLexerT_Boolean:
         v->t = JParserT_Boolean;
         v->v.b = o->sn;
         break;
      case JLexerT_Number:
         JLexer_setNumber(o,v);
         break;
      case JLexerT_String:
         JLexer_setString(o,v);
         break;
      default:
         return -1;
   }
   JDBuf_reset(o->asmB);
   return 0;
}



#define  JLexer_constructor(o, asmBM) do { \
   (o)->asmB = asmBM; \
   (o)->state=JLexerSt_GetNextToken; \
} while(0)



#define JLexer_setBuf(o, buf, size) do {\
   (o)->tokenPtr=(o)->bufStart=buf;\
   (o)->bufEnd=(o)->bufStart+size;\
}while(0)


static BaBool
JLexer_hasMoreData(JLexer* o)
{
   while(o->tokenPtr != o->bufEnd)
   {
      baAssert(o->tokenPtr <  o->bufEnd);
      if( ! bIsspace(*o->tokenPtr) )
         return TRUE;
      o->tokenPtr++;
   }
   return FALSE;
}


static JLexerT
JLexer_nextToken(JLexer* o)
{
   JDBuf* asmB = o->asmB;
   for(;;)
   {
      baAssert(o->tokenPtr <=  o->bufEnd);
      if(o->tokenPtr == o->bufEnd)
         return JLexerT_NeedMoreData;

      switch(o->state)
      {
         case JLexerSt_StartComment:
            if(*o->tokenPtr == '*')
               o->state = JLexerSt_EatComment;
            else if(*o->tokenPtr == '/')
               o->state = JLexerSt_EatCppComment;
            else
               return JLexerT_ParseErr;
            o->tokenPtr++;
            break;

         case JLexerSt_EatComment:
            while(*o->tokenPtr++ != '*')
            {
               if(o->tokenPtr == o->bufEnd)
                  return JLexerT_NeedMoreData;
            }
            o->state = JLexerSt_EndComment;
            break;

         case JLexerSt_EndComment:
            if(*o->tokenPtr++ != '/')
               o->state = JLexerSt_EatComment;
            else
               o->state = JLexerSt_GetNextToken;
            break;

         case JLexerSt_EatCppComment:
            while(*o->tokenPtr++ != '\n')
            {
               if(o->tokenPtr == o->bufEnd)
                  return JLexerT_NeedMoreData;
            }
            o->state = JLexerSt_GetNextToken;
            break;

         case JLexerSt_TrueFalseNull:
            if(bTolower(*o->tokenPtr++) != *o->typeChkPtr++)
               return JLexerT_ParseErr;
            if( ! *o->typeChkPtr )
            {
               o->state = JLexerSt_GetNextToken;
               return (JLexerT)o->retVal;
            }
            break;

         case JLexerSt_String:
            if(JDBuf_expandIfNeeded(o->asmB, 2)) return JLexerT_MemErr;
            while(*o->tokenPtr != '\\')
            {
               if(*o->tokenPtr == o->sn) /* equal end of string: ' or "  */
               {
                  asmB->buf[asmB->index]=0;
                  o->tokenPtr++;
                  o->state = JLexerSt_GetNextToken;
                  return JLexerT_String;
               }
               asmB->buf[asmB->index++] = *o->tokenPtr++;
               if(JDBuf_expandIfNeeded(o->asmB, 1)) return JLexerT_MemErr;
               if(o->tokenPtr == o->bufEnd)
                  return JLexerT_NeedMoreData;
            }
            o->tokenPtr++;
            o->state = JLexerSt_StringEscape;
            break;

         case JLexerSt_StringEscape:
            switch(*o->tokenPtr)
            {
               case '"':
               case '/':
               case '\\':
               case 'b':
               case 'f':
               case 'n':
               case 'r':
               case 't':
               case 'v':
                  switch(*o->tokenPtr)
                  {
                     case '"':  asmB->buf[asmB->index]='"';  break;
                     case '/':  asmB->buf[asmB->index]='/';  break;
                     case '\\': asmB->buf[asmB->index]='\\'; break;
                     case 'b':  asmB->buf[asmB->index]='\b'; break;
                     case 'f':  asmB->buf[asmB->index]='\f'; break;
                     case 'n':  asmB->buf[asmB->index]='\n'; break;
                     case 'r':  asmB->buf[asmB->index]='\r'; break;
                     case 't':  asmB->buf[asmB->index]='\t'; break;
                     case 'v':  asmB->buf[asmB->index]='\v'; break;
                  }
                  asmB->index++;
                  o->tokenPtr++;
                  o->state = JLexerSt_String;
                  break;

               case 'u':
                  o->tokenPtr++;
                  o->state = JLexerSt_StringUnicode;
                  o->unicode=0;
                  o->unicodeShift=12;
                  break;

               default:
                     return JLexerT_ParseErr;
            }
            break;

         case JLexerSt_StringUnicode:
         {
            U32 hex;
            char c = *o->tokenPtr;
            if ( c >= '0' && c <= '9' )
               hex = c - '0';
            else if ( c >= 'a' && c <= 'f' )
               hex = c - 'a' + 10;
            else if ( c >= 'A' && c <= 'F' )
               hex = c - 'A' + 10;
            else
               return JLexerT_ParseErr;
            o->unicode |= (hex << o->unicodeShift);
            o->tokenPtr++;
            baAssert(o->unicodeShift >= 0);
            if( ! o->unicodeShift )
            {  /* Convert unicode to UTF8. */
               if(JDBuf_expandIfNeeded(o->asmB, 4)) return JLexerT_MemErr;
               if (o->unicode < 0x80)
               {
                  asmB->buf[asmB->index++] = (U8)o->unicode;
               }
               else if (o->unicode < 0x800)
               {
                  asmB->buf[asmB->index++]=(U8)(0xc0|(o->unicode >> 6));
                  asmB->buf[asmB->index++]=(U8)(0x80|(o->unicode & 0x3f));
               }
               else
               {
                  asmB->buf[asmB->index++]=
                     (U8)(0xe0 | (o->unicode >> 12));
                  asmB->buf[asmB->index++]=
                     (U8)(0x80 | ((o->unicode>>6)&0x3f));
                  asmB->buf[asmB->index++]=
                     (U8)(0x80 | (o->unicode & 0x3f));
               }
               o->state = JLexerSt_String;
            }
            o->unicodeShift -= 4;
            break;
         }

         case JLexerSt_Number:
            while(strchr(jsonNumberChars, *o->tokenPtr))
            {
               if(JDBuf_expandIfNeeded(o->asmB, 2))
                  return JLexerT_MemErr;
               if(*o->tokenPtr=='.' || *o->tokenPtr=='e' || *o->tokenPtr=='E')
                  o->isDouble=TRUE;
               asmB->buf[asmB->index++] = *o->tokenPtr++;
               if(o->tokenPtr == o->bufEnd)
                  return JLexerT_NeedMoreData;
            }
            asmB->buf[asmB->index]=0;
            o->state = JLexerSt_GetNextToken;
            return JLexerT_Number;

         case JLexerSt_GetNextToken:
            switch(*o->tokenPtr)
            {
               case '{':
                  o->tokenPtr++;
                  return JLexerT_BeginObject;

               case '}':
                  o->tokenPtr++;
                  return JLexerT_EndObject;

               case '[':
                  o->tokenPtr++;
                  return JLexerT_BeginArray;

               case ']':
                  o->tokenPtr++;
                  return JLexerT_EndArray;

               case ',':
                  o->tokenPtr++;
                  return JLexerT_Comma;

               case ':':
                  o->tokenPtr++;
                  return JLexerT_MemberSep;

               case 't':
               case 'T':
               case 'f':
               case 'F':
                  o->state = JLexerSt_TrueFalseNull;
                  o->retVal = JLexerT_Boolean;
                  if(*o->tokenPtr == 'f' || *o->tokenPtr == 'T')
                  {
                     o->typeChkPtr = falseString;
                     o->sn = FALSE;
                  }
                  else
                  {
                     o->typeChkPtr = trueString;
                     o->sn = TRUE;
                  }
                  o->tokenPtr++;
                  break;

               case 'n':
               case 'N':
                  o->tokenPtr++;
                  o->typeChkPtr = nullString;
                  o->retVal = JLexerT_Null;
                  o->state = JLexerSt_TrueFalseNull;
                  break;

               case '"':
               case '\'':
                  baAssert(asmB->index==0);
                  if(JDBuf_expandIfNeeded(o->asmB, 2))
                     return JLexerT_MemErr;
                  o->sn = *o->tokenPtr++;
                  o->state = JLexerSt_String;
                  break;

               case ' ':
               case '\t':
               case '\n':
               case '\r':
                  o->tokenPtr++;
                  break;

               case '-': /* negative number */
                  o->tokenPtr++;
                  o->sn = 255;
                  o->state = JLexerSt_Number;
                  baAssert(asmB->index==0);
                  if(JDBuf_expandIfNeeded(o->asmB, 256))
                     return JLexerT_MemErr;
                  break;

               case '/':
                  o->tokenPtr++;
                  o->state = JLexerSt_StartComment;
                  break;

               default:
                  baAssert(asmB->index==0);
                  if(isdigit(*o->tokenPtr))
                  {
                     o->sn = 0;
                     o->state = JLexerSt_Number;
                     if(JDBuf_expandIfNeeded(o->asmB, 256))
                        return JLexerT_MemErr;
                     break;
                  }
                  else
                     return JLexerT_ParseErr;
            }
      }
   }
}


/****************************************************************************
                                JParser
 JSON Parser
 ****************************************************************************/

static int
JParser_setStatus(JParser* o, JParsStat s, int retVal)
{
   o->status = s;
   if(retVal)
   {
      o->stackIx=0;
      o->state = JParserSt_StartObj;
   }
   return retVal;
}

static int
JParser_service(JParser* o)
{
   int retVal = JParserIntf_serviceCB(o->intf, &o->val, o->stackIx);
   if(retVal)
      JParser_setStatus(o, JParsStat_IntfErr, -1);
   o->val.memberName[0]=0;
   return retVal;
}



void
JParser_constructor(JParser* o, JParserIntf* intf, char* nameBuf,
                    int namebufSize, AllocatorIntf* alloc, int extraStackLen)
{
   memset(o, 0, sizeof(JParser));
   JDBuf_constructor(&o->mnameB, 0);
   o->val.memberName = nameBuf;
   o->mnameB.buf = (U8*)nameBuf;
   nameBuf[0]=0;
   o->mnameB.size = (size_t)namebufSize;
   JDBuf_constructor(&o->asmB, alloc);
   JLexer_constructor(&o->lexer,&o->asmB);
   o->intf = intf;
   o->status = JParsStat_DoneEOS;
   o->state = JParserSt_StartObj;
   o->stackIx = 0;
   o->stackSize = (U16)(JPARSER_STACK_LEN + extraStackLen);
}


void
JParser_destructor(JParser* o)
{
   JDBuf_destructor(&o->asmB);
}


int
JParser_parse(JParser* o, const U8* buf, U32 size)
{
   JLexerT lexerT;
   if(o->status == JParsStat_DoneEOS || o->status == JParsStat_NeedMoreData)
      JLexer_setBuf(&o->lexer,buf,size);
   else if(o->status != JParsStat_Done)
   {
      baAssert(o->status == JParsStat_ParseErr ||
               o->status == JParsStat_MemErr ||
               o->status == JParsStat_IntfErr);
      return -1;
   }

   for(;;)
   {
      lexerT = JLexer_nextToken(&o->lexer);
      if(lexerT == JLexerT_NeedMoreData)
         return JParser_setStatus(o, JParsStat_NeedMoreData, 0);
      if(lexerT == JLexerT_ParseErr)
         return JParser_setStatus(o, JParsStat_ParseErr, -1);
      if(lexerT == JLexerT_MemErr)
         return JParser_setStatus(o, JParsStat_MemErr, -1);

      switch(o->state)
      {
         case JParserSt_StartObj:
        L_startObj:
            if( (o->stackIx + 1) >= o->stackSize)
               return JParser_setStatus(o, JParsStat_StackOverflow, -1);
            o->stack[o->stackIx] = lexerT; /* Assume that it is ok */
            if(lexerT == JLexerT_BeginObject)
            {
               o->val.t = JParserT_BeginObject;
               o->lexer.asmB = &o->mnameB;
               o->state = JParserSt_MemberName;
            }
            else if(lexerT == JLexerT_BeginArray)
            {
               o->val.t = JParserT_BeginArray;
               o->state = JParserSt_BeginArray;
            }
            else
               return JParser_setStatus(o, JParsStat_ParseErr, -1);
            if(JParser_service(o)) return -1;
            o->stackIx++;
            break;

         case JParserSt_BeginArray:
            if(lexerT == JLexerT_EndArray)
               goto L_endArray;
            else
               goto L_value;

         case JParserSt_MemberName:
            JDBuf_reset(&o->mnameB);
            o->lexer.asmB = &o->asmB;
            if(lexerT == JLexerT_EndObject)
               goto L_endObj;
            if(lexerT != JLexerT_String)
               return JParser_setStatus(o, JParsStat_ParseErr, -1);
            o->state = JParserSt_MemberSep;
            break;

         case JParserSt_MemberSep:
            if(lexerT != JLexerT_MemberSep)
               return JParser_setStatus(o, JParsStat_ParseErr, -1);
            o->state = JParserSt_Value;
            break;

         case JParserSt_Value:
        L_value:
            if(lexerT == JLexerT_BeginObject ||
               lexerT == JLexerT_BeginArray)
            {
               goto L_startObj;
            }
            if(JLexer_setValue(&o->lexer, lexerT, &o->val))
               return JParser_setStatus(o, JParsStat_ParseErr, -1);
            if(JParser_service(o)) return -1;
            o->state = JParserSt_Comma;
            break;

         case JParserSt_Comma:
            if(o->stack[o->stackIx-1] == JLexerT_BeginObject)
            {
               if(lexerT == JLexerT_Comma)
               {
                  o->lexer.asmB = &o->mnameB;
                  o->state = JParserSt_MemberName;
               }
               else if(lexerT == JLexerT_EndObject)
               {
                 L_endObj:
                  if(o->stack[o->stackIx-1] != JLexerT_BeginObject)
                     return JParser_setStatus(o, JParsStat_ParseErr, -1);
                  o->val.t = JParserT_EndObject;
                  o->stackIx--;
                  if(JParser_service(o)) return -1;
                  if(o->stackIx == 0)
                  {
L_endParse:
                     return JParser_setStatus(
                        o, JLexer_hasMoreData(&o->lexer) ?
                        JParsStat_Done : JParsStat_DoneEOS, 1);
                  }
                  o->state = JParserSt_Comma;
               }
               else
                  return JParser_setStatus(o, JParsStat_ParseErr, -1);
            }
            else
            {
               baAssert(o->stack[o->stackIx-1] == JLexerT_BeginArray);
               if(lexerT == JLexerT_Comma)
                  o->state = JParserSt_Value;
               else if(lexerT == JLexerT_EndArray)
               {
                 L_endArray:
                  if(o->stack[o->stackIx-1] != JLexerT_BeginArray)
                     return JParser_setStatus(o, JParsStat_ParseErr, -1);
                  o->val.t = JParserT_EndArray;
                  o->stackIx--;
                  if(JParser_service(o)) return -1;
                  if(o->stackIx == 0)
                     goto L_endParse;
                  o->state = JParserSt_Comma;
               }
               else
                  return JParser_setStatus(o, JParsStat_ParseErr, -1);
            }
            break;

         default:
            baAssert(0);
      } /* end switch */
   } /* end for(;;) */
}
