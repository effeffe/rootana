//
// test_mvodb.cxx --- test ODB access functions
//
// K.Olchanski
//

#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <assert.h>
#include <signal.h>
#include <string.h>

#include "TMidasOnline.h"
#include "TMidasFile.h"
#include "TMidasEvent.h"
#include "mvodb.h"

std::string toString(int i)
{
  char buf[256];
  sprintf(buf, "%d", i);
  return buf;
}

// Main function call

int main(int argc, char *argv[])
{
   setbuf(stdout,NULL);
   setbuf(stderr,NULL);
 
   signal(SIGILL,  SIG_DFL);
   signal(SIGBUS,  SIG_DFL);
   signal(SIGSEGV, SIG_DFL);
   signal(SIGPIPE, SIG_DFL);
 
   const char* hostname = NULL;
   const char* exptname = NULL;
   const char* filename = argv[1];
   bool online  = false;
   bool xmlfile = false;
   bool jsonfile = false;
   bool httpfile = false;
   bool nullodb = false;

   if (!filename)
     online = true;
   else if (strstr(filename, ".xml")!=0)
     xmlfile = true;
   else if (strstr(filename, ".json")!=0)
     jsonfile = true;
   else if (strstr(filename, "null")!=0)
     nullodb = true;
   else
     httpfile = true;

   TMidasOnline *midas = NULL;
   
   MVOdb* odb = NULL;

   if (nullodb)
     {
       printf("Using NullOdb\n");
       odb = MakeNullOdb();
     }
   else if (online)
     {
       printf("Using MidasOdb\n");
       midas = TMidasOnline::instance();

       int err = midas->connect(hostname, exptname, "test_mvodb");
       if (err != 0)
         {
           fprintf(stderr,"Cannot connect to MIDAS, error %d\n", err);
           return -1;
         }

       odb = MakeMidasOdb(midas->fDB);
     }
   else if (xmlfile)
     {
       odb = MakeXmlFileOdb(filename);
     }
#if 0
   else if (jsonfile)
     {
#ifdef HAVE_ROOT_XML
       XmlOdb* odb = new XmlOdb(filename);
       //odb->DumpTree();
       gOdb = odb;
#else
       printf("This program is compiled without support for XML ODB access\n");
       return -1;
#endif
     }
   else if (httpfile)
     {
#ifdef HAVE_ROOT
       HttpOdb* odb = new HttpOdb(filename);
       //odb->DumpTree();
       gOdb = odb;
#else
       printf("This program is compiled without support for HTTP ODB access\n");
       return -1;
#endif
     }
   else
     {
#ifdef HAVE_ROOT
       TMidasFile f;
       bool tryOpen = f.Open(filename);

       if (!tryOpen)
         {
           printf("Cannot open input file \"%s\"\n",filename);
           return -1;
         }

       while (1)
         {
           TMidasEvent event;
           if (!f.Read(&event))
             break;

           int eventId = event.GetEventId();
           //printf("Have an event of type %d\n",eventId);

           if ((eventId & 0xFFFF) == 0x8000)
             {
               // begin run
               //event.Print();

               //
               // Load ODB contents from the ODB XML file
               //
               if (gOdb) {
                 delete gOdb;
                 gOdb = NULL;
               }
#ifdef HAVE_ROOT_XML
               gOdb = new XmlOdb(event.GetData(),event.GetDataSize());
#else
               printf("This program is compiled without support for XML ODB access\n");
#endif
               break;
             }
         }

       if (!gOdb)
         {
           printf("Failed to load ODB from input file \"%s\"\n",filename);
           return -1;
         }
#else
       printf("This program is compiled without support for XML ODB access\n");
       return -1;
#endif
     }
#endif

   int runno = 1234;

   odb->RI("runinfo/run number", &runno, false);

   printf("read run number (RI): %d\n", runno);

   MVOdb* test = odb->Chdir("test_mvodb", true);

   assert(test != NULL);

   test->SetPrintError(true);

   int ivalue = 1;
   float fvalue = 2.2;
   double dvalue = 3.3;
   bool bvalue0 = false;
   bool bvalue1 = true;
   uint16_t u16value = 0xabcd;
   uint32_t u32value = 0x12345678;
   std::string svalue = "test string";

   printf("\n");
   printf("Test read of all data types:\n");
   printf("\n");

   test->RI("int", &ivalue, true);
   test->RF("float", &fvalue, true);
   test->RD("double", &dvalue, true);
   test->RB("bool0", &bvalue0, true);
   test->RB("bool1", &bvalue1, true);
   test->RU16("u16", &u16value, true);
   test->RU32("u32", &u32value, true);
   test->RS("string", &svalue, true);

   printf("int: %d\n", ivalue);
   printf("float: %f\n", fvalue);
   printf("double: %f\n", dvalue);
   printf("bool0: %d\n", bvalue0);
   printf("bool1: %d\n", bvalue1);
   printf("u16: 0x%04x\n", u16value);
   printf("u32: 0x%08x\n", u32value);
   printf("string: \"%s\"\n", svalue.c_str());

   printf("\n");
   printf("Test write of all data types:\n");
   printf("\n");

   test->WI("int", 10);
   test->WF("float", 11.1);
   test->WD("double", 22.2);
   test->WB("bool0", true);
   test->WB("bool1", false);
   test->WU16("u16", 0xcdef);
   test->WU32("u32", 0xdeadf00d);
   test->WS("string", "write test string");

   printf("\n");
   printf("Test read after write of all data types:\n");
   printf("\n");

   test->RI("int", &ivalue, true);
   test->RF("float", &fvalue, true);
   test->RD("double", &dvalue, true);
   test->RB("bool0", &bvalue0, true);
   test->RB("bool1", &bvalue1, true);
   test->RU16("u16", &u16value, true);
   test->RU32("u32", &u32value, true);
   test->RS("string", &svalue, true);

   printf("int: %d\n", ivalue);
   printf("float: %f\n", fvalue);
   printf("double: %f\n", dvalue);
   printf("bool0: %d\n", bvalue0);
   printf("bool1: %d\n", bvalue1);
   printf("u16: 0x%04x\n", u16value);
   printf("u32: 0x%08x\n", u32value);
   printf("string: \"%s\"\n", svalue.c_str());

   printf("\n");
   printf("Test read arrays of all data types:\n");
   printf("\n");

   std::vector<int> ia;
   ia.push_back(1);
   ia.push_back(2);
   ia.push_back(3);
   test->RIA("ia", &ia, true);
   // read non-existant array
   test->RIA("ia-noexist", &ia);
   // create 10 element array, init to zero (ia is empty)
   ia.clear();
   test->RIA("ia10zero", &ia, true, 10);
   // create 10 element array, init from ia
   ia.clear();
   ia.push_back(11);
   ia.push_back(22);
   test->RIA("ia10", &ia, true, 10);
   // create 10 element array, init to zero (passed NULL instead of &ia)
   test->RIA("createia10", NULL, true, 10);

   std::vector<double> da;
   da.push_back(1.1);
   da.push_back(1.2);
   da.push_back(1.3);
   da.push_back(1.4);
   test->RDA("da", &da, true, 0);

   std::vector<float> fa;
   fa.push_back(2.1);
   fa.push_back(2.2);
   fa.push_back(2.3);
   fa.push_back(20.3);
   fa.push_back(21.3);
   test->RFA("fa", &fa, true, 0);

   std::vector<bool> ba;
   ba.push_back(true);
   ba.push_back(false);
   ba.push_back(true);
   test->RBA("ba", &ba, true, 0);

   std::vector<std::string> sa;
   sa.push_back("line1");
   sa.push_back("line2");
   sa.push_back("line3");
   sa.push_back("line4");
   test->RSA("sa", &sa, true, 0, 32);

   // create 10 element array, init from ia
   sa.clear();
   sa.push_back("xx1");
   sa.push_back("xx2");
   test->RSA("sa10", &sa, true, 10, 32);
   // create 10 element array, init to zero (passed NULL instead of &sa)
   test->RSA("createsa10", NULL, true, 10, 32);

   printf("\n");
   printf("Test string sizes:\n");
   printf("\n");

   {
     test->WS("test_size", "12345");
     std::string s;
     test->RS("test_size", &s);
     printf("read test_size     [%s]\n", s.c_str());
   }
   {
     test->WS("test_size_32", "1234567890", 32);
     std::string s;
     test->RS("test_size_32", &s);
     printf("read test_size_32  [%s]\n", s.c_str());
   }
   {
     test->WS("test_size_5", "1234567890", 5);
     std::string s;
     test->RS("test_size_5", &s);
     printf("read test_size_5   [%s]\n", s.c_str());
   }
   {
     std::string s = "1234567890";
     test->RS("test_rsize", &s, true);
     printf("read test_rsize    [%s]\n", s.c_str());
   }
   {
     std::string s = "123456";
     test->RS("test_size_r32", &s, true, 32);
     printf("read test_size_r32 [%s]\n", s.c_str());
   }
   {
     std::string s = "1234567890";
     test->RS("test_size_r8", &s, true, 8);
     printf("read test_size_r8  [%s]\n", s.c_str());
   }
   {
     test->RSA("test_size_a8", NULL, true, 2, 8);
     std::string s = "1234567890";
     test->WSAI("test_size_a8", 0, s.c_str());
     test->RSAI("test_size_a8", 0, &s);
     printf("read test_size_a8  [%s]\n", s.c_str());
   }

   printf("\n");
   printf("Test creating and resizing arrays:\n");
   printf("\n");

   {
     test->RIA("create_ia15", NULL, true, 15);
     test->RBA("create_ba12", NULL, true, 12);
     test->RSA("create_sa5", NULL, true, 5, 32);
   }
   {
     test->RIA("resize_ia10", NULL, true, 5);
     test->RIA("resize_ia10", NULL, true, 10);
     test->RSA("resize_sa3", NULL, true, 5, 32);
     test->WSAI("resize_sa3", 0, "00000000");
     test->WSAI("resize_sa3", 1, "11111111");
     test->WSAI("resize_sa3", 2, "22222222");
     test->WSAI("resize_sa3", 3, "33333333");
     test->WSAI("resize_sa3", 4, "44444444");
     test->RSA("resize_sa3", NULL, true, 3, 5);
   }

   printf("\n");
   printf("Test array index access:\n");
   printf("\n");

   {
     std::vector<int> ia;
     ia.push_back(10);
     ia.push_back(11);
     ia.push_back(12);
     test->WIA("index_ia", ia);
     for (int i=0; i<5; i++) {
       int ivalue = 999;
       test->RIAI("index_ia", i, &ivalue);
       printf("index %d value %d\n", i, ivalue);
     }
     for (int i=0; i<4; i++) {
       int ivalue = 20+i;
       test->WIAI("index_ia", i, ivalue);
     }
     for (int i=0; i<5; i++) {
       int ivalue = 999;
       test->RIAI("index_ia", i, &ivalue);
       printf("index %d value %d\n", i, ivalue);
     }
   }

   printf("\n");
   printf("Test string array index access:\n");
   printf("\n");

   {
     std::vector<std::string> sa;
     sa.push_back("sa0");
     sa.push_back("sa1");
     sa.push_back("sa2");
     test->WSA("index_sa", sa, 32);
     for (int i=0; i<5; i++) {
       std::string s = "aaa";
       test->RSAI("index_sa", i, &s);
       printf("index %d value [%s]\n", i, s.c_str());
     }
     for (int i=0; i<4; i++) {
       std::string s = "sa_qqq";
       s += toString(i);
       test->WSAI("index_sa", i, s.c_str());
     }
     for (int i=0; i<5; i++) {
       std::string s = "bbb";
       test->RSAI("index_sa", i, &s);
       printf("index %d value [%s]\n", i, s.c_str());
     }
   }

   printf("\n");
   printf("Test string truncation:\n");
   printf("\n");

   {
     std::vector<std::string> sa;
     sa.push_back("1234567890");
     sa.push_back("aaa1");
     sa.push_back("aaa2");
     test->WSA("trunc_sa5", sa, 5);
     test->WSAI("trunc_sa5", 1, "1234567890");
     for (int i=0; i<5; i++) {
       std::string s = "bbb";
       test->RSAI("trunc_sa5", i, &s);
       printf("index %d value [%s]\n", i, s.c_str());
     }
   }

   printf("\n");
   printf("Test special cases:\n");
   printf("\n");

   {
     printf("test RI() of non existant variable:\n");
     int ivalue = 999;
     test->RI("nonexistant", &ivalue);
     printf("RI() returned ivalue %d\n", ivalue);
     printf("\n");

     printf("test RDA() of integer array:\n");
     // wrong data type: ODB is INT, we ask for DOUBLE
     test->RDA("ia10", &da, false, 0);
     printf("RDA() returned array [%d]\n", (int)da.size());
     printf("\n");

     printf("test RI() of array ia10:\n");
     ivalue = 999;
     test->RI("ia10", &ivalue);
     printf("RI() returned ivalue %d\n", ivalue);
     printf("\n");

     printf("test index of non-array:\n");
     ivalue = 999;
     test->RIAI("int", 10, &ivalue);
     printf("RIAI() returned ivalue %d\n", ivalue);
     printf("\n");

     printf("test invalid index -1:\n");
     ivalue = 999;
     test->RIAI("ia10", -1, &ivalue);
     printf("RIAI() returned ivalue %d\n", ivalue);
     printf("\n");

     printf("test invalid index 999:\n");
     ivalue = 999;
     test->RIAI("ia10", 999, &ivalue);
     printf("RIAI() returned ivalue %d\n", ivalue);
     printf("\n");

     printf("test string array invalid index -1:\n");
     svalue = "aaa";
     test->RSAI("sa10", -1, &svalue);
     printf("RSAI() returned [%s]\n", svalue.c_str());
     printf("\n");

     printf("test string array invalid index 999:\n");
     svalue = "aaa";
     test->RSAI("sa10", 999, &svalue);
     printf("RSAI() returned [%s]\n", svalue.c_str());
     printf("\n");

     printf("test write invalid index -1:\n");
     test->WIAI("ia10", -1, 10);
     printf("\n");

     printf("test string write invalid index -1:\n");
     test->WSAI("sa10", -1, "aaa");
     printf("\n");

     {
       printf("test Chdir(true):\n");
       MVOdb* dir = test->Chdir("subdir", true);
       printf(" returned %p\n", dir);
     }

     {
       printf("test Chdir(false):\n");
       MVOdb* dir = test->Chdir("non-existant-subdir", false);
       printf(" returned %p\n", dir);
     }

     {
       printf("test Chdir(\"not a dir\", true):\n");
       MVOdb* dir = test->Chdir("ia10", true);
       printf(" returned %p\n", dir);
     }

     {
       printf("test Chdir(\"not a dir\", false):\n");
       MVOdb* dir = test->Chdir("ia10", false);
       printf(" returned %p\n", dir);
     }
   }

   if (midas)
     midas->disconnect();
   
   return 0;
}

//end
