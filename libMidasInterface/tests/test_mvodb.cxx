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
#include <stdlib.h> // exit()

#ifdef HAVE_MIDAS
#include "TMidasOnline.h"
#endif
#include "TMidasFile.h"
#include "TMidasEvent.h"
#include "mvodb.h"

std::string toString(int i)
{
  char buf[256];
  sprintf(buf, "%d", i);
  return buf;
}

void print_ia(const std::vector<int> &ia)
{
   int size = ia.size();
   printf("int[%d] has [", size);
   for (int i=0; i<size; i++) {
      if (i>0)
         printf(", ");
      printf("%d", ia[i]);
   }
   printf("]");
}

void print_da(const std::vector<double> &da)
{
   int size = da.size();
   printf("int[%d] has [", size);
   for (int i=0; i<size; i++) {
      if (i>0)
         printf(", ");
      printf("%f", da[i]);
   }
   printf("]");
}

static int gCountFail = 0;

void report_fail(const char* text)
{
   printf("FAIL: %s\n", text);
   gCountFail++;
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

#ifdef HAVE_MIDAS
   const char* hostname = NULL;
   const char* exptname = NULL;
#endif
   const char* filename = argv[1];
   bool online  = false;
   bool xmlfile = false;
   bool jsonfile = false;
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
     nullodb = true;

#ifdef HAVE_MIDAS
   TMidasOnline *midas = NULL;
#endif
   
   MVOdb* odb = NULL;
   MVOdbError odberror;

   if (nullodb)
     {
       printf("Using NullOdb\n");
       odb = MakeNullOdb();
     }
   else if (online)
     {
#ifdef HAVE_MIDAS
       printf("Using MidasOdb\n");
       midas = TMidasOnline::instance();

       int err = midas->connect(hostname, exptname, "test_mvodb");
       if (err != 0)
         {
           fprintf(stderr,"Cannot connect to MIDAS, error %d\n", err);
           return -1;
         }

       odb = MakeMidasOdb(midas->fDB, &odberror);
#else
       printf("Using MidasOdb: MIDAS support not available, sorry.\n");
       exit(1);
#endif
     }
   else if (xmlfile)
     {
       printf("Using XmlOdb\n");
       odb = MakeXmlFileOdb(filename, &odberror);
     }
   else if (jsonfile)
     {
       printf("Using JsonOdb\n");
       odb = MakeJsonFileOdb(filename, &odberror);
     }
   else
     {
       printf("Using FileDumpOdb\n");
       TMidasFile f;
       bool tryOpen = f.Open(filename);

       if (!tryOpen) {
         printf("Cannot open input file \"%s\"\n",filename);
         return -1;
       }

       while (1) {
         TMidasEvent event;
         if (!f.Read(&event))
           break;
         
         int eventId = event.GetEventId();
         //printf("Have an event of type %d\n",eventId);
         
         if ((eventId & 0xFFFF) != 0x8000)
           continue;
         
         // begin run
         //event.Print();
         
         odb = MakeFileDumpOdb(event.GetData(),event.GetDataSize(), &odberror);
         break;
       }

       if (!odb) {
         printf("Failed to load ODB from input file \"%s\"\n",filename);
         return -1;
       }
     }

   if (odberror.fError) {
      fprintf(stderr, "Cannot make MVOdb object, error: %s\n", odberror.fErrorString.c_str());
      exit(1);
   }

   printf("\n");
   printf("Starting tests:\n");
   printf("\n");

   int runno = 1234;

   odb->RI("runinfo/run number", &runno);

   printf("read run number (RI): %d\n", runno);

   MVOdb* test = odb->Chdir("test_mvodb", true);

   assert(test != NULL);

   test->SetPrintError(true);

   bool isreadonly = test->IsReadOnly();

   if (isreadonly) {
      printf("\n");
      printf("This ODB is read-only!\n");
   }

   printf("\n");
   printf("Test create-and-read of all data types (set default values):\n");
   printf("\n");

   int cycle  = 1;
   test->RI("cycle", &cycle, true);
   printf("RI() cycle: %d\n", cycle);

   int ivalue = 1;
   test->RI("int", &ivalue, true);
   printf("RI() int: %d\n", ivalue);

   float fvalue = 2.2;
   test->RF("float", &fvalue, true);
   printf("RF() float: %f\n", fvalue);

   double dvalue = 3.3;
   test->RD("double", &dvalue, true);
   printf("RD() double: %f\n", dvalue);

   bool bvalue_a = false;
   test->RB("bool_a", &bvalue_a, true);
   printf("bool_a: %d\n", bvalue_a);

   bool bvalue_b = true;
   test->RB("bool_b", &bvalue_b, true);
   printf("bool_b: %d\n", bvalue_b);

   uint16_t u16value = 0xabcd;
   test->RU16("u16", &u16value, true);
   printf("RU16() u16: 0x%04x\n", u16value);

   uint32_t u32value = 0x12345678;
   test->RU32("u32", &u32value, true);
   printf("RU32() u32: 0x%08x\n", u32value);

   std::string svalue = "test string";
   test->RS("string", &svalue, true);
   printf("RS() string: \"%s\"\n", svalue.c_str());

   printf("\n");
   printf("Test write of all data types (overwrite default values):\n");
   printf("\n");

   test->WI("cycle", cycle+1);

   int wi = 10 + 100*cycle;
   test->WI("int", wi);
   float wf = 11.1 + 100*cycle;
   test->WF("float", wf);
   double wd = 22.2 + 100*cycle;
   test->WD("double", wd);
   bool wba = true;
   test->WB("bool_a", wba);
   bool wbb = false;
   test->WB("bool_b", wbb);
   uint16_t wu16 = 0xcdef;
   test->WU16("u16", wu16);
   uint32_t wu32 = 0xdeadf00d;
   test->WU32("u32", wu32);
   std::string ws = "write test string cycle " + toString(cycle);
   test->WS("string", ws.c_str());

   printf("\n");
   printf("Test read of new values for all data types:\n");
   printf("\n");

   int ri = 1;
   test->RI("int", &ri);
   printf("int: %d -> %d -> %d\n", ivalue, wi, ri);

   float rf = 2.2;
   test->RF("float", &rf);
   printf("float: %f -> %f -> %f\n", fvalue, wf, rf);

   double rd = 3.3;
   test->RD("double", &rd);
   printf("double: %f -> %f -> %f\n", dvalue, wd, rd);

   bool rba = false;
   test->RB("bool_a", &rba);
   printf("bool_a: %d -> %d -> %d\n", bvalue_a, wba, rba);

   bool rbb = false;
   test->RB("bool_b", &rbb);
   printf("bool_b: %d -> %d -> %d\n", bvalue_b, wbb, rbb);

   uint16_t ru16 = 0x4444;
   test->RU16("u16", &ru16);
   printf("u16: 0x%04x -> 0x%04x -> 0x%04x\n", u16value, wu16, ru16);

   uint32_t ru32 = 0x55555555;
   test->RU32("u32", &ru32);
   printf("u32: 0x%08x -> 0x%08x -> 0x%08x\n", u32value, wu32, ru32);

   std::string rs = "rs";
   test->RS("string", &rs);
   printf("string: \"%s\" -> \"%s\" -> \"%s\"\n", svalue.c_str(), ws.c_str(), rs.c_str());

   printf("\n");
   printf("Test read arrays of all data types:\n");
   printf("\n");

   {
      printf("read int array ia:\n");
      std::vector<int> ia;
      ia.push_back(1);
      ia.push_back(2);
      ia.push_back(3);
      test->RIA("ia", &ia, true);
      printf("RIA() returned: ");
      print_ia(ia);
      printf("\n");
   }

   {
      printf("read non-existant array ia-noexist:\n");
      std::vector<int> ia;
      ia.push_back(1);
      ia.push_back(2);
      ia.push_back(3);
      test->RIA("ia-noexist", &ia);
      printf("RIA() returned: ");
      print_ia(ia);
      printf("\n");
   }

   {
      // create 10 element array, init to zero (ia is empty)
      std::vector<int> ia;
      test->RIA("ia10zero", &ia, true, 10);
      // create 10 element array, init from ia
      ia.clear();
      ia.push_back(11);
      ia.push_back(22);
      test->RIA("ia10", &ia, true, 10);
   }

   {
      // create 10 element array, init to zero (passed NULL instead of &ia)
      test->RIA("createia10", NULL, true, 10);
   }

   {
      std::vector<uint32_t> u32a;
      u32a.push_back(0x11110000);
      u32a.push_back(0x22220000);
      u32a.push_back(0x33330000);
      u32a.push_back(0x44440000);
      test->RU32A("dwa", &u32a, true);
   }

   {
      std::vector<double> da;
      da.push_back(1.1);
      da.push_back(1.2);
      da.push_back(1.3);
      da.push_back(1.4);
      test->RDA("da", &da, true, 0);
   }

   {
      std::vector<float> fa;
      fa.push_back(2.1);
      fa.push_back(2.2);
      fa.push_back(2.3);
      fa.push_back(20.3);
      fa.push_back(21.3);
      test->RFA("fa", &fa, true, 0);
   }

   {
      std::vector<bool> ba;
      ba.push_back(true);
      ba.push_back(false);
      ba.push_back(true);
      test->RBA("ba", &ba, true, 0);
   }

   {
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
   }

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
   printf("Test against subdirectory:\n");
   printf("\n");

   {
      printf("Creating subdir\n");
      MVOdb* subdir = test->Chdir("subdir", true);
      printf("Creating subdir/i\n");
      subdir->WI("i", 10); // write something into subdir
      int i = 1111;
      printf("RI(subdir)\n");
      test->RI("subdir", &i); // invalid read from a non-int
      printf("WI(subdir)\n");
      test->WI("subdir", 10); // invalid write into existing subdir
      std::vector<int> ia;
      ia.push_back(111);
      ia.push_back(222);
      ia.push_back(333);
      printf("WIA(subdir)\n");
      test->WIA("subdir", ia); // invalid write into existing subdir
      printf("RIA(subdir)\n");
      test->RIA("subdir", &ia); // invalid read from non-int-array
      printf("RIA() returned: ");
      print_ia(ia);
      printf("\n");
      printf("RIA(subdir, create=true)\n");
      test->RIA("subdir", &ia, true); // invalid read from non-int-array
      printf("RIA() returned: ");
      print_ia(ia);
      printf("\n");
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

     {
       std::vector<double> da;
       printf("test RDA() of integer array:\n");
       // wrong data type: ODB is INT, we ask for DOUBLE
       test->RDA("ia10", &da, false, 0);
       printf("RDA() returned: ");
       print_da(da);
       printf("\n");
     }
       
     {
       printf("test RD() of integer array:\n");
       // wrong data type: ODB is INT, we ask for DOUBLE
       double v = 999.9;
       test->RD("ia10", &v);
       printf("RD() returned %f\n", v);
       printf("\n");
     }

     {
       printf("test RI() of array ia10:\n");
       int ivalue = 999;
       test->RI("ia10", &ivalue);
       printf("RI() returned ivalue %d\n", ivalue);
       printf("\n");
     }

     {
       printf("test RIA() of non-array int:\n");
       std::vector<int> ia;
       test->RIA("int", &ia);
       printf("RIA() returned: ");
       print_ia(ia);
       printf("\n");
     }

     {
       printf("test index 0 of non-array int:\n");
       int ivalue = 999;
       test->RIAI("int", 0, &ivalue);
       printf("RIAI() returned ivalue %d\n", ivalue);
       printf("\n");
     }

     {
       printf("test index of non-array int:\n");
       int ivalue = 999;
       test->RIAI("int", 10, &ivalue);
       printf("RIAI() returned ivalue %d\n", ivalue);
       printf("\n");
     }

     {
       printf("test invalid index -1 of array ia10:\n");
       int ivalue = 999;
       test->RIAI("ia10", -1, &ivalue);
       printf("RIAI() returned ivalue %d\n", ivalue);
       printf("\n");
     }

     {
       printf("test invalid index 999 of array ia10:\n");
       int ivalue = 999;
       test->RIAI("ia10", 999, &ivalue);
       printf("RIAI() returned ivalue %d\n", ivalue);
       printf("\n");
     }

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

     printf("\n");
     printf("Test Chdir():\n");
     printf("\n");

     {
       printf("test Chdir(true):\n");
       MVOdb* dir = test->Chdir("test_subdir", true);
       printf("Chdir() returned %p\n", dir);
       if (dir == NULL) report_fail("Chdir(new directory, true)");
     }

     {
       printf("test Chdir(false):\n");
       MVOdb* dir = test->Chdir("non-existant-subdir", false);
       printf("Chdir() returned %p\n", dir);
       if (dir != NULL) report_fail("Chdir(new directory, false)");
     }

     {
       printf("test Chdir(\"not a dir\", true):\n");
       MVOdb* dir = test->Chdir("ia10", true);
       printf("Chdir() returned %p\n", dir);
       if (dir == NULL) report_fail("Chdir(not a directory, true)");
     }

     {
       printf("test Chdir(\"not a dir\", false):\n");
       MVOdb* dir = test->Chdir("ia10", false);
       printf(" returned %p\n", dir);
       if (dir != NULL) report_fail("Chdir(not a directory, false)");
     }
   }

#ifdef HAVE_MIDAS
   if (midas)
     midas->disconnect();
#endif

   printf("\n");
   printf("Number of FAILED tests: %d\n", gCountFail);
   printf("\n");
   
   return 0;
}

//end
/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
