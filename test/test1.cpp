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

#include <JDecoder.h>
#include <JEncoder.h>

typedef struct {
   S32 s32Var;
   S64 s64Var;
   char buf[20];
   double dVar;
} T1;

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

JParser* p;



int
write2file(const char* fn, void* data, int len)
{
   FILE* fp = fopen(fn,"wb");
   if(!fp)
      return -1;
   fwrite(data, 1, len, fp);
   fclose(fp);
   return 0;
}


int parse(const char* fn)
{
   size_t size;
   U8 buf[1024];
   FILE* fp = fopen(fn,"rb");
   if(!fp)
      return -1;
   size=fread(buf, 1, sizeof(buf), fp);
   fclose(fp);
   if(size == 0)
      return -1;
   if(p->parse(buf, size) != 1 || p->getStatus() != JParsStat_DoneEOS)
   {
      printf("Parse err\n");
	  return -1;
   }
   return 0;
}



/* Parser callback */
static int
buildValCB(JParserIntf* super, JParserVal* v, int recLevel)
{
   int i;
   for(i=0 ; i < recLevel ; i++) putchar('\t');
   if(*v->memberName) /* if parent is an object */
      printf("Name: %s, ", v->memberName);
   switch(v->t)
   {
      case JParserT_Boolean:
         printf("Boolean %s",v->v.b ? "TRUE" : "FALSE");
         break;
      case JParserT_Double:
         printf("Float64 %f",v->v.f);
         break;
      case JParserT_Int:
         printf("Int32 %d (%X)",(unsigned int)v->v.d,(unsigned int)v->v.d);
         break;
      case JParserT_Long:
#ifdef _WIN32
         printf("Int64 %I64d (%I64X)",v->v.l,v->v.l);
#else
         printf("Int64 %lld (%llX)",(long long)v->v.l,(long long)v->v.l);
#endif
         break;

      case JParserT_Null:
         printf("Null");
         break;

      case JParserT_String:
         printf("String: ");
         printf("%s",v->v.s);
         break;

      case JParserT_BeginObject:
         printf("BeginObject: level: %d",recLevel);
         break;
      case JParserT_BeginArray:
         printf("BeginArray: level: %d",recLevel);
         break;
      case JParserT_EndObject:
         printf("EndObject");
         break;
      case JParserT_EndArray:
         printf("EndArray");
         break;

	  default:
		  baAssert(0);

   }
   printf("\n");
   return 0;
}


T1 t1Out = {
   1234556335,
   6434523484939242478,
   "hello",
   1.2345
};


T2 t2Out[2] = {{125,"Hello"}, {-125,"World"}};
T3 t3Out = {{1,"t3.one"}, {2,"t3.two"}};
T4 t4Out = {{{1,"t4.one"}, {2,"t4.two"}},{{3,"t4.three"}, {4,"t4.four"}}};

T1 t1In;
T2 t2In[2];
T3 t3In;
T4 t4In;



int main()
{
   static char memberName[10];

   static U8 buf[1024];

   T2* t3_m1 = &t3Out.member1;
   T2* t3_m2 = &t3Out.member2;

   T2* t4_x1_m1 = &t4Out.x1.member1;
   T2* t4_x1_m2 = &t4Out.x1.member2;
   T2* t4_x2_m1 = &t4Out.x2.member1;
   T2* t4_x2_m2 = &t4Out.x2.member2;

   JParserIntf pintf(buildValCB);

   JDecoder dec(buf, sizeof(buf));
   JParser parser(&dec, memberName, sizeof(memberName), AllocatorIntf_getDefault());
   p = &parser;

   memset(&t1In, 0, sizeof(t1In));
   memset(&t2In, 0, sizeof(t2In));
   memset(&t3In, 0, sizeof(t3In));
   memset(&t4In, 0, sizeof(t4In));

   BufPrint out;
   out.setBuf((char*)buf,sizeof(buf));
   JErr err;
   JEncoder enc(&err, &out);

   if(enc.set("{dlsf}",
            JE_MEMBER(&t1Out, s32Var),
            JE_MEMBER(&t1Out, s64Var),
            JE_MEMBER(&t1Out, buf),
            JE_MEMBER(&t1Out, dVar)))
      return 1;
   enc.commit();
   write2file("t1.json", buf, out.cursor);
   out.erase();

   if(dec.get("{dlsf}",
         JD_MNUM(&t1In, s32Var),
         JD_MNUM(&t1In, s64Var),
         JD_MSTR(&t1In, buf),
         JD_MNUM(&t1In, dVar)))
		 return 1;
   if(parse("t1.json"))
      return 1;
   if( memcmp(&t1Out, &t1In, sizeof(t1In)) )
       return 1;

   if(enc.set("[{ds}{ds}]",
            JE_MEMBER(&t2Out[0], s8),
            JE_MEMBER(&t2Out[0], buf),
            JE_MEMBER(&t2Out[1], s8),
            JE_MEMBER(&t2Out[1], buf)))
      return 1;
   enc.commit();
   write2file("t2.json", buf, out.cursor);
   out.erase();

   if(dec.get("[{ds}{ds}]",
            JD_MNUM(&t2In[0], s8),
            JD_MSTR(&t2In[0], buf),
            JD_MNUM(&t2In[1], s8),
            JD_MSTR(&t2In[1], buf)))
      return 1;
   if(parse("t2.json"))
      return 1;
   if( memcmp(&t2Out, &t2In, sizeof(t2In)) )
       return 1;

   if(enc.set("{{ds}{ds}}",
            "member1",
            JE_MEMBER(t3_m1, s8),
            JE_MEMBER(t3_m1, buf),
            "member2",
            JE_MEMBER(t3_m2, s8),
            JE_MEMBER(t3_m2, buf)))
      return 1;
   enc.commit();
   write2file("t3.json", buf, out.cursor);
   out.erase();

   t3_m1 = &t3In.member1;
   t3_m2 = &t3In.member2;
   if(dec.get("{{ds}{ds}}",
            "member1",
            JD_MNUM(t3_m1, s8),
            JD_MSTR(t3_m1, buf),
            "member2",
            JD_MNUM(t3_m2, s8),
            JD_MSTR(t3_m2, buf)))
      return 1;
   if(parse("t3.json"))
      return 1;
   if( memcmp(&t3Out, &t3In, sizeof(t3In)) )
       return 1;

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
      return 1;
   enc.commit();
   write2file("t4.json", buf, out.cursor);
   out.erase();

   t4_x1_m1 = &t4In.x1.member1;
   t4_x1_m2 = &t4In.x1.member2;
   t4_x2_m1 = &t4In.x2.member1;
   t4_x2_m2 = &t4In.x2.member2;

   if(dec.get("{{{ds}{ds}}{{ds}{ds}}}",
            "x1",
            "member1",
            JD_MNUM(t4_x1_m1, s8),
            JD_MSTR(t4_x1_m1, buf),
            "member2",
            JD_MNUM(t4_x1_m2, s8),
            JD_MSTR(t4_x1_m2, buf),
            "x2",
            "member1",
            JD_MNUM(t4_x2_m1, s8),
            JD_MSTR(t4_x2_m1, buf),
            "member2",
            JD_MNUM(t4_x2_m2, s8),
            JD_MSTR(t4_x2_m2, buf)))
      return 1;
   if(parse("t4.json"))
      return 1;
   if( memcmp(&t4Out, &t4In, sizeof(t4In)) )
       return 1;

   if(dec.get("{{X}{X}}",
            "x1",
            &pintf,
            "x2",
            &pintf))
      return 1;

   if(parse("t4.json"))
      return 1;


   enc.beginObject();
   enc.setName("array");
   enc.beginArray();
   for(int i = 0 ; i < 20 ; i++)
      enc.setInt(1+i*10);
   enc.endArray();
   if(enc.endObject())
      return 1;
   write2file("array.json", buf, out.cursor);


   if(dec.get("{[X]}",
            "array",
            &pintf))
      return 1;
   if(parse("array.json"))
      return 1;

   printf("\n\nOK\n");
   return 0;
}
