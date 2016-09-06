// midasio.h

class TMReaderInterface
{
 public:
   virtual int Read(void* buf, int count) = 0;
   virtual int Close() = 0;
   virtual ~TMReaderInterface() {};
};

TMReaderInterface* TMNewReader(const char* source);

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

