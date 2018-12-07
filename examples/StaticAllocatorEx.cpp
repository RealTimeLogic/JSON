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

/* 
   Static (buffer) allocator when using JParser and JParserValFact.
   For systems with no dynamic memory.
*/

#include <JDecoder.h>
#include <JEncoder.h>


/* JParser allocates one buffer only.
 */

#define MAX_STRING_LEN 1024

class JParserAllocator : public AllocatorIntf
{
   U8 buf[MAX_STRING_LEN];

   static void* malloc(AllocatorIntf* o, size_t* size);
   static void* realloc(AllocatorIntf* o, void* memblock, size_t* size);
   static void free(AllocatorIntf* o, void* memblock);

public:
   JParserAllocator();
};


void* JParserAllocator::malloc(AllocatorIntf* super, size_t* size)
{
   JParserAllocator* o = (JParserAllocator*)super;
   return *size < MAX_STRING_LEN ? o->buf : 0;
}


//Called when the one and only string buffer must grow.
void* JParserAllocator::realloc(
   AllocatorIntf* super, void* memblock, size_t* size)
{
   JParserAllocator* o = (JParserAllocator*)super;
   baAssert(memblock == o->buf);
   return *size < MAX_STRING_LEN ? o->buf : 0;
}


//Called when JParser terminates
void JParserAllocator::free(AllocatorIntf* super, void* memblock)
{
   // Do nothing
   (void)super;
   (void)memblock;
}

JParserAllocator::JParserAllocator() : AllocatorIntf(malloc,realloc,free)
{
}


/* JParserValFact allocator
 */

class JPValFactAllocator : public AllocatorIntf
{
   U8* buf;
   size_t bufsize; 
   size_t cursor;

   static void* malloc(AllocatorIntf* o, size_t* size);
   static void* realloc(AllocatorIntf* o, void* memblock, size_t* size);
   static void free(AllocatorIntf* o, void* memblock);

public:
   JPValFactAllocator(void* buf, size_t bufsize);
   void reset() {  cursor = 0; }
};


void* JPValFactAllocator::malloc(AllocatorIntf* super, size_t* size)
{
   JPValFactAllocator* o = (JPValFactAllocator*)super;
   if(*size + o->cursor < o->bufsize)
   {
      void* buf = o->buf+o->cursor;
      o->cursor += *size;
	  return buf;
   }
   return 0; // Memory exhausted
}


//Not used by JParserValFact
void* JPValFactAllocator::realloc(
   AllocatorIntf* super, void* memblock, size_t* size)
{
   (void)super;
   (void)memblock;
   (void)size;
   baAssert(0);
   return 0;
}


// Called when JParserValFact terminates or when
// JParserValFact::termFirstVal is called.
void JPValFactAllocator::free(AllocatorIntf* super, void* memblock)
{
   // Do nothing
   (void)super;
   (void)memblock;
}

JPValFactAllocator::JPValFactAllocator(void* b, size_t bsize) :
   AllocatorIntf(malloc,realloc,free)
{
   buf =(U8*)b;
   bufsize = bsize;
   reset();
}



typedef struct {
   S8  s8;
   char buf[20];
} T2;

typedef struct {
   T2 member1;
   T2 member2;
} T3;


typedef struct {
   T3 x1;
   T3 x2;
} T4;


static void
printPadChars(int pad)
{
   int i;
   for(i = 0 ; i < pad; i++)
      putchar(' ');
}


static void
printJVal(JVal* v, int recCntr)
{
   for( ; v ; v = JVal_getNextElem(v) )
   {
      const char* m;
      printPadChars(recCntr * 4);
      m = v->getName();
      if(m)
        printf("%-10s : ", m); /* v is a struct member */
      /* else: v is an array member */
      
      switch(v->getType())
      {
         case JVType_String:
            printf("%s\n", v->getString(0));
            break;
         case JVType_Double:
            printf("%g\n", v->getDouble(0));
            break;
         case JVType_Int:
            printf("%d\n", (int)v->getInt(0));
            break;
         case JVType_Long:
#ifdef _WIN32
            printf("%I64d\n", v->getLong(0));
#else
/* Assume GCC */
            printf("%lld\n", (long long)v->getLong(0));
#endif
            break;
         case JVType_Boolean:
            printf("%s\n", v->getBoolean(0) ? "true" : "false");
            break;
         case JVType_Null:
            printf("NULL\n");
            break;

         case JVType_Object:
         case JVType_Array:
            if(m)
               putchar('\n');
            printJVal(v->getJ(0), recCntr+1);
            break;

         default:
            baAssert(0);
      }
   }
}

/* 64 bit requires more memory for pointers */
#ifdef __LP64__
#define XSIZE 2
#else
#define XSIZE 1
#endif


// Using lots of stack space since all objects are on the 'main' stack.
int main()
{
   char memberName[10];
   U8 buf[1024];
   T4 t4 = {{{1,"t4.one"}, {2,"t4.two"}},{{3,"t4.three"}, {4,"t4.four"}}};

   U8 vBuf[380*XSIZE];
   U8 dBuf[360];
   JPValFactAllocator vAlloc(vBuf,sizeof(vBuf));
   JPValFactAllocator dAlloc(dBuf,sizeof(dBuf));
   JParserValFact vFact(&vAlloc, &dAlloc);
   JParserAllocator jpa;
   JParser parser(&vFact, memberName, sizeof(memberName),&jpa);

   BufPrint out;
   out.setBuf((char*)buf,sizeof(buf));
   JErr err;
   JEncoder enc(&err, &out);

   T2* t4_x1_m1 = &t4.x1.member1;
   T2* t4_x1_m2 = &t4.x1.member2;
   T2* t4_x2_m1 = &t4.x2.member1;
   T2* t4_x2_m2 = &t4.x2.member2;

   for(int i=0 ; i < 2 ; i++)
   {
      if(enc.set("{{{ds}{ds}}{{ds}{ds}}}",
                 "x1",
                 "member1",
                 JE_MEMBER(t4_x1_m1, s8),
                 JE_MEMBER(t4_x1_m1, buf),
                 "member2",
                 JE_MEMBER(t4_x1_m2, s8),
                 JE_MEMBER(t4_x1_m2, buf),
                 "x2",
                 "member1",
                 JE_MEMBER(t4_x2_m1, s8),
                 JE_MEMBER(t4_x2_m1, buf),
                 "member2",
                 JE_MEMBER(t4_x2_m2, s8),
                 JE_MEMBER(t4_x2_m2, buf)))
         return 1; // Failed

      if(parser.parse((U8*)out.buf, out.cursor) != 1)
         return 1; //Failed

      enc.commit(); // Ready for next object

      printf("-----------------------------\n");
      printJVal(vFact.getFirstVal(), 0);

      /* Must reset JParserValFact. Note: termFirstVal calls our
       * allocator::free, which is not doing anything.
       */
      vFact.termFirstVal();
      //Make allocators ready for next object
      vAlloc.reset();
      dAlloc.reset();
   }

   printf("OK\n");
   return 0;
}
