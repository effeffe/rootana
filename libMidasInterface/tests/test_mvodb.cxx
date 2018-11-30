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

#include "TMidasOnline.h"
#include "TMidasFile.h"
#include "TMidasEvent.h"
#include "mvodb.h"

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

   MVOdb* odb = NULL;

   if (nullodb)
     {
       printf("Using NullOdb\n");
       odb = MakeNullOdb();
     }
   else if (online)
     {
       printf("Using MidasOdb\n");
       TMidasOnline *midas = TMidasOnline::instance();

       int err = midas->connect(hostname, exptname, "test_mvodb");
       if (err != 0)
         {
           fprintf(stderr,"Cannot connect to MIDAS, error %d\n", err);
           return -1;
         }

       odb = MakeMidasOdb(midas->fDB);
     }
#if 0
   else if (xmlfile)
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

   odb->RI("runinfo/run number", 0, &runno, false);

   printf("read run number (RI): %d\n", runno);

   MVOdb* test = odb->Chdir("test_mvodb", true);

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

   test->RI("int", 0, &ivalue, true);
   test->RF("float", 0, &fvalue, true);
   test->RD("double", 0, &dvalue, true);
   test->RB("bool0", 0, &bvalue0, true);
   test->RB("bool1", 0, &bvalue1, true);
   test->RU16("u16", 0, &u16value, true);
   test->RU32("u32", 0, &u32value, true);
   test->RS("string", 0, &svalue, true);

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

   test->WI("int", 0, 10);
   test->WF("float", 0, 11.1);
   test->WD("double", 0, 22.2);
   test->WB("bool0", 0, true);
   test->WB("bool1", 0, false);
   test->WU16("u16", 0, 0xcdef);
   test->WU32("u32", 0, 0xdeadf00d);
   test->WS("string", 0, "write test string");

   printf("\n");
   printf("Test read arrays of all data types:\n");
   printf("\n");

   std::vector<int> ia;
   ia.push_back(1);
   ia.push_back(2);
   ia.push_back(3);
   test->RIA("ia", &ia, true, 0);
   // read non-existant array
   test->RIA("ia-noexist", &ia, false, 0);
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
   printf("Test special cases:\n");
   printf("\n");

   test->RI("nonexistant", 0, &ivalue, false);
   test->RI("nonexistant_with_index", 1, &ivalue, true);
   // wrong data type: ODB is INT, we ask for DOUBLE
   test->RDA("ia10", &da, false, 0);

#if 0
   printf("read array size of /test: %d\n", gOdb->odbReadArraySize("/test"));
   printf("read array size of /runinfo/run number: %d\n", gOdb->odbReadArraySize("/runinfo/Run number"));
   printf("read array size of /test/intarr: %d\n", gOdb->odbReadArraySize("/test/intarr"));
   printf("read array values:\n");
   int size = gOdb->odbReadArraySize("/test/intarr");
   for (int i=0; i<size; i++)
     printf("  intarr[%d] = %d\n", i, gOdb->odbReadInt("/test/intarr", i));
   printf("read double value: %f\n", gOdb->odbReadDouble("/test/dblvalue"));
   printf("read uint32 value: %d\n", gOdb->odbReadUint32("/test/dwordvalue"));
   printf("read bool value: %d\n", gOdb->odbReadBool("/test/boolvalue"));
   const char* s = gOdb->odbReadString("/test/stringvalue");
   int len=0;
   if (s)
     len = strlen(s);
   printf("read string value: [%s] length %d\n", s, len);

   printf("\nTry non-existent entries...\n\n");

   printf("read array size: %d\n", gOdb->odbReadArraySize("/test/doesnotexist"));
   printf("read uint32 value: %d\n", gOdb->odbReadUint32("/test/doesnotexist", 0, -9999));

   printf("\nTry wrong types...\n\n");

   printf("read float value: %f\n", gOdb->odbReadDouble("/test/fltvalue"));
   printf("read uint32 value: %d\n", gOdb->odbReadUint32("/test/wordvalue"));
   printf("read int value: %d\n", gOdb->odbReadInt("/test/wordvalue"));

   printf("\nTry wrong array indices...\n\n");

   printf("try to index a non-array: %f\n", gOdb->odbReadDouble("/test/dblvalue", 10, -9999));
   printf("try invalid index -1: %d\n", gOdb->odbReadInt("/test/intarr", -1, -9999));
   printf("try invalid index 999: %d\n", gOdb->odbReadInt("/test/intarr", 999, -9999));
   printf("try invalid index 1 for double value: %f\n", gOdb->odbReadDouble("/test/dblvalue", 1, -9999));
#endif
   
   return 0;
}

//end
