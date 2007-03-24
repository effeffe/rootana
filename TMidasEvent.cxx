//
//  TMidasEvent.cxx.
//

#include <iostream>
#include <iomanip>
#include <assert.h>

#include "TMidasEvent.h"

using std::cout;
using std::cerr;
using std::endl;
using std::hex;
using std::dec;
using std::setw;
using std::setfill;
using std::showbase;
using std::bad_alloc;
using std::ios_base;

typedef uint8_t BYTE;

/// Byte swapping routine.
///
#define QWORD_SWAP(x) { BYTE _tmp;       \
_tmp= *((BYTE *)(x));                    \
*((BYTE *)(x)) = *(((BYTE *)(x))+7);     \
*(((BYTE *)(x))+7) = _tmp;               \
_tmp= *(((BYTE *)(x))+1);                \
*(((BYTE *)(x))+1) = *(((BYTE *)(x))+6); \
*(((BYTE *)(x))+6) = _tmp;               \
_tmp= *(((BYTE *)(x))+2);                \
*(((BYTE *)(x))+2) = *(((BYTE *)(x))+5); \
*(((BYTE *)(x))+5) = _tmp;               \
_tmp= *(((BYTE *)(x))+3);                \
*(((BYTE *)(x))+3) = *(((BYTE *)(x))+4); \
*(((BYTE *)(x))+4) = _tmp; }

/// Byte swapping routine.
///
#define DWORD_SWAP(x) { BYTE _tmp;       \
_tmp= *((BYTE *)(x));                    \
*((BYTE *)(x)) = *(((BYTE *)(x))+3);     \
*(((BYTE *)(x))+3) = _tmp;               \
_tmp= *(((BYTE *)(x))+1);                \
*(((BYTE *)(x))+1) = *(((BYTE *)(x))+2); \
*(((BYTE *)(x))+2) = _tmp; }

/// Byte swapping routine.
///
#define WORD_SWAP(x) { BYTE _tmp;        \
_tmp= *((BYTE *)(x));                    \
*((BYTE *)(x)) = *(((BYTE *)(x))+1);     \
*(((BYTE *)(x))+1) = _tmp; }

TMidasEvent::TMidasEvent()
  : fBankListMax(64), fStringBankListMax(fBankListMax * 4)
{
  fData = NULL;
  fAllocated = false;

  fBanksN = 0;
  fBankList = NULL;

  fEventHeader.fEventId      = 0;
  fEventHeader.fTriggerMask  = 0;
  fEventHeader.fSerialNumber = 0;
  fEventHeader.fTimeStamp    = 0;
  fEventHeader.fDataSize     = 0;
}

void TMidasEvent::Copy(const TMidasEvent& rhs)
{
  fEventHeader = rhs.fEventHeader;

  fData        = (char*)malloc(fEventHeader.fDataSize);
  assert(fData);
  memcpy(fData, rhs.fData, fEventHeader.fDataSize);
  fAllocated = true;

  fBanksN      = rhs.fBanksN;
  fBankList    = (char*)malloc(fBanksN + 1);
  assert(fBankList);
  memcpy(fBankList, rhs.fBankList, fBanksN + 1);
}

TMidasEvent::TMidasEvent(const TMidasEvent &rhs)
  : fBankListMax(64), fStringBankListMax(fBankListMax * 4)
{
  Copy(rhs);
}

TMidasEvent::~TMidasEvent()
{
  Clear();
}

TMidasEvent& TMidasEvent::operator=(const TMidasEvent &rhs)
{
  if (&rhs != this)
    Clear();

  this->Copy(rhs);
  return *this;
}

void TMidasEvent::Clear()
{
  if (fBankList)
    free(fBankList);
  fBankList = NULL;

  if (fData && fAllocated)
    free(fData);
  fData = NULL;

  fAllocated = false;
  fBanksN = 0;

  fEventHeader.fEventId      = 0;
  fEventHeader.fTriggerMask  = 0;
  fEventHeader.fSerialNumber = 0;
  fEventHeader.fTimeStamp    = 0;
  fEventHeader.fDataSize     = 0;
}

static const int TID_SIZE[] = {0, 1, 1, 1, 2, 2, 4, 4, 4, 4, 8, 1, 0, 0, 0, 0, 0};

int TMidasEvent::LocateBank(void *event, const char *name, void **pdata) const
{
  /// Locates a bank of given name inside an event.
  /// \param [in] event Pointer to current composed event.
  /// \param [in] name Bank name to look for.
  /// \param [out] pdata Pointer to data area of bank, NULL if bank not found.
  /// \returns Number of values inside the bank.
  ///
  Bank_t *pbk;
  Bank32_t *pbk32;
  uint32_t dname;

  if (((((BankHeader_t *) event)->fFlags & (1<<4)) > 0)) {
    pbk32 = (Bank32_t *) (((BankHeader_t *) event) + 1);
    memcpy(&dname, name, 4);
    do {
      if (*((uint32_t *) pbk32->fName) == dname) {
        *pdata = pbk32 + 1;
        if (TID_SIZE[pbk32->fType & 0xFF] == 0)
          return pbk32->fDataSize;
        return pbk32->fDataSize / TID_SIZE[pbk32->fType & 0xFF];
      }
      pbk32 = (Bank32_t *) ((char*)(pbk32 + 1)+(((pbk32->fDataSize)+7) & ~7));
    } while ((uint32_t) pbk32 - (uint32_t) event <
             ((BankHeader_t *) event)->fDataSize + sizeof(BankHeader_t));
  } else {
    pbk = (Bank_t *) (((BankHeader_t *) event) + 1);
    memcpy(&dname, name, 4);
    do {
      if (*((uint32_t *) pbk->fName) == dname) {
        *pdata = pbk + 1;
        if (TID_SIZE[pbk->fType & 0xFF] == 0)
          return pbk->fDataSize;
        return pbk->fDataSize / TID_SIZE[pbk->fType & 0xFF];
      }
      pbk = (Bank_t *) ((char*) (pbk + 1) + (((pbk->fDataSize)+7) & ~7));
    } while ((uint32_t) pbk - (uint32_t) event <
             ((BankHeader_t *) event)->fDataSize + sizeof(BankHeader_t));
  }
  //
  // bank not found
  //
  *pdata = NULL;
  return 0;
}

int TMidasEvent::FindBank(BankHeader_t *pbkh, const char* name, int *bklen, int *bktype, void **pdata) const
{
  /// Finds a bank of given name inside an event.
  /// \param [in] pbkh Pointer to current composed event.
  /// \param [in] name Bank name to look for.
  /// \param [out] bklen Number of elemtents in bank.
  /// \param [out] bktype Bank type.
  /// \param [in] pdata Pointer to data area of bank, NULL if bank not found.
  /// \returns 1 if bank found, 0 otherwise.
  ///
  Bank_t *pbk;
  Bank32_t *pbk32;
  uint32_t dname;

  if (((((BankHeader_t *) pbkh)->fFlags & (1<<4)) > 0)) {
    pbk32 = (Bank32_t *) (pbkh + 1);
    memcpy(&dname, name, 4);
    do {
      if (*((uint32_t *) pbk32->fName) == dname) {
        *pdata = pbk32 + 1;
        if (TID_SIZE[pbk32->fType & 0xFF] == 0)
          *bklen = pbk32->fDataSize;
        else
          *bklen = pbk32->fDataSize / TID_SIZE[pbk32->fType & 0xFF];

        *bktype = pbk32->fType;
        return 1;
      }
      pbk32 = (Bank32_t *) ((char*) (pbk32 + 1) +
                          (((pbk32->fDataSize)+7) & ~7));
    } while ((uint32_t) pbk32 -
             (uint32_t) pbkh < pbkh->fDataSize + sizeof(BankHeader_t));
  } else {
    pbk = (Bank_t *) (pbkh + 1);
    memcpy(&dname, name, 4);
    do {
      if (*((uint32_t *) pbk->fName) == dname) {
        *pdata = pbk + 1;
        if (TID_SIZE[pbk->fType & 0xFF] == 0)
          *bklen = pbk->fDataSize;
        else
          *bklen = pbk->fDataSize / TID_SIZE[pbk->fType & 0xFF];

        *bktype = pbk->fType;
        return 1;
      }
      pbk = (Bank_t *) ((char*) (pbk + 1) + (((pbk->fDataSize)+7) & ~7));
    } while ((uint32_t) pbk - (uint32_t) pbkh < pbkh->fDataSize +
             sizeof(BankHeader_t));
  }
  //
  // bank not found
  //
  *pdata = NULL;
  return 0;
}

int TMidasEvent::SwapBytes(bool force)
{
  /// Swaps bytes from little endian to big endian or vice versa for a
  /// whole event.  An event contains a flag which is set by bk_init()
  /// to identify the endian format of an event. If force is FALSE,
  /// this flag is evaluated and the event is only swapped if it is in
  /// the "wrong" format for this system. An event can be swapped to
  /// the "wrong" format on purpose for example by a front-end which
  /// wants to produce events in a "right" format for a back-end
  /// analyzer which has different byte ordering.
  /// \param [in] force If true, the event is always swapped, if
  /// false, the event is only swapped if it is in the wrong format.
  /// \returns 1==event has been swap, 0==event has not been swapped.
  ///
  BankHeader_t *pbh;
  Bank_t *pbk;
  Bank32_t *pbk32;
  void *pdata;
  uint16_t type;

  pbh = (BankHeader_t *) fData;
  //
  // only swap if flags in high 16-bit
  //
  if (pbh->fFlags < 0x10000 && ! force)
    return 0;

  if (pbh->fDataSize == 0x3c3f786d) // string "<xml..."
    return 1;

  //
  // swap bank header
  //
  DWORD_SWAP(&pbh->fDataSize);
  DWORD_SWAP(&pbh->fFlags);
  //
  // check for 32-bit banks
  //
  bool b32 = ((pbh->fFlags & (1<<4)) > 0);

  pbk = (Bank_t *) (pbh + 1);
  pbk32 = (Bank32_t *) pbk;
  //
  // scan event
  //
  while ((int) pbk - (int) pbh < (int) pbh->fDataSize +
         (int) sizeof(BankHeader_t)) {
    //
    // swap bank header
    //
    if (b32) {
      DWORD_SWAP(&pbk32->fType);
      DWORD_SWAP(&pbk32->fDataSize);
      pdata = pbk32 + 1;
      type = (uint16_t) pbk32->fType;
    } else {
      WORD_SWAP(&pbk->fType);
      WORD_SWAP(&pbk->fDataSize);
      pdata = pbk + 1;
      type = pbk->fType;
    }
    //
    // pbk points to next bank
    //
    if (b32) {
      pbk32 = (Bank32_t *) ((char*) (pbk32 + 1) +
                          (((pbk32->fDataSize)+7) & ~7));
      pbk = (Bank_t *) pbk32;
    } else {
      pbk = (Bank_t *) ((char*) (pbk + 1) +  (((pbk->fDataSize)+7) & ~7));
      pbk32 = (Bank32_t *) pbk;
    }

    switch (type) {
    case 4:
    case 5:
      while ((int) pdata < (int) pbk) {
        WORD_SWAP(pdata);
        pdata = (void *) (((uint16_t *) pdata) + 1);
      }
      break;
    case 6:
    case 7:
    case 8:
    case 9:
      while ((int) pdata < (int) pbk) {
        DWORD_SWAP(pdata);
        pdata = (void *) (((uint32_t *) pdata) + 1);
      }
      break;
    case 10:
      while ((int) pdata < (int) pbk) {
        QWORD_SWAP(pdata);
        pdata = (void *) (((double*) pdata) + 1);
      }
      break;
    }
  }
  return 1;
}

void TMidasEvent::Print(const char *option) const
{
  /// Print data held in this class.
  /// \param [in] option If 'a' (for "all") then the raw data will be
  /// printed out too.
  ///
  cout.setf(ios_base::showbase | ios_base::internal);
  cout.setf(ios_base::hex, ios_base::basefield);
  cout.fill('0');
  cout << endl << "Event start:" << endl
       << "  event id:       " << setw(6)  << fEventHeader.fEventId      << endl
       << "  trigger mask:   " << setw(6)  << fEventHeader.fTriggerMask  << endl
       << "  serial number:  " << setw(10) << fEventHeader.fSerialNumber << endl
       << "  time stamp:     " << setw(10) << fEventHeader.fTimeStamp
       << " ==> "
       << ctime(reinterpret_cast<time_t const *>(&fEventHeader.fTimeStamp))
       << "  data size:      " << setw(10) << fEventHeader.fDataSize << endl;
  if (fBanksN == 0) {
    if ((fEventHeader.fEventId & 0xffff) == 0x8000) {
      const char *midasMagic =
        reinterpret_cast<const char *>(&fEventHeader.fTriggerMask);
      cout << "No banks, start of run:"      << endl << "  midas magic="
           << midasMagic[0] << midasMagic[1] << endl << "  run number="
           << dec << fEventHeader.fSerialNumber << endl;
    } else if ((fEventHeader.fEventId & 0xffff) == 0x8001)
      cout << "No banks, end of run:" << endl;
    else
      cout << "Print: Use SetBankList() before Print() to print bank data"
           << endl;
  } else {
    cout << "Banks: " << fBankList << endl;
    for (int i = 0; i < fBanksN * 4; i += 4) {
      int bankLength = 0;
      int bankType = 0;
      void *pdata = 0;
      int found = FindBank(reinterpret_cast<BankHeader_t *>(fData),
			   &fBankList[i], &bankLength, &bankType,  &pdata);

      cout << "Bank="          << fBankList[i]     << fBankList[i + 1]
           << fBankList[i + 2] << fBankList[i + 3] << " length="   << dec
           << bankLength       << " type="         << bankType     << endl;
      if (option[0] == 'a' && found) {
        cout << "Bank data: " << endl;
        switch (bankType) {
          //
          // TID_WORD
          //
        case 4:
          for (int j = 0; j < bankLength; j++) {
            cout << setw(6) << hex << showbase
                 << static_cast<uint16_t>((static_cast<uint16_t *>(pdata))[j])
                 << " ";
            if (j % 11 == 10)
              cout << endl;
          }
          cout << endl;
          break;
          //
          // TID_DWORD
          //
        case 6:
          for (int j = 0; j < bankLength; j++) {
            cout << setw(10) << hex << showbase
                 << static_cast<uint32_t>((static_cast<uint32_t *>(pdata))[j]) << " ";
            if (j % 7 == 6)
              cout << endl;
          }
          cout << endl;
          break;
          //
          // TID_FLOAT
          //
        case 9:
          for (int j = 0; j < bankLength; j++) {
            cout << setw(6) << dec
                 << static_cast<float>((static_cast<float*>(pdata))[j])
                 << " ";
            if (j % 7 == 6)
              cout << endl;
          }
          cout << endl;
          break;
        default:
          cerr << "Print: ERROR - unknown bank type " << bankType << endl;
        }
      }
    }
  }
}

void TMidasEvent::AllocateData()
{
  fData = (char*)malloc(fEventHeader.fDataSize);
  assert(fData);
  fAllocated = true;
}

int TMidasEvent::SetBankList()
{
  /// Set the bank list.
  /// \returns The number of banks in the event.
  ///
  if (fEventHeader.fEventId <= 0)
    return 0;
  char bankList[fStringBankListMax];
  char *bklist = bankList;
  void *event = reinterpret_cast<void *>(fData);
  int size;
  Bank_t *pmbk = NULL;
  Bank32_t *pmbk32 = NULL;
  char *pdata;
  //
  // compose bank list
  //
  bklist[0] = 0;
  fBanksN = 0;
  do {
    //
    // scan all banks for bank name only
    //
    if ((((BankHeader_t *) event)->fFlags & (1 << 4)) > 0) {
      size = IterateBank32(event, &pmbk32, &pdata);
      if (pmbk32 == NULL)
        break;
    } else {
      size = IterateBank(event, &pmbk, &pdata);
      if (pmbk == NULL)
        break;
    }
    fBanksN++;

    if (fBanksN > fBankListMax) {
      cerr << "bk_list: Over " << fBankListMax << " banks -> truncated" << endl;
      return (fBanksN - 1);
    }
    if ((((BankHeader_t *) event)->fFlags & (1 << 4)) > 0)
      strncat(bklist, (char*) pmbk32->fName, 4);
    else
      strncat(bklist, (char*) pmbk->fName, 4);
  }
  while (1);
  fBankList = new char[4 * fBanksN + 1];
  strncpy(fBankList, bankList, 4 * fBanksN + 1);
  return fBanksN;
}

int TMidasEvent::IterateBank(void *event, Bank_t **pbk, char **pdata)
{
  /// Iterates through banks inside an event. The function can be used
  /// to enumerate all banks of an event.
  /// \param [in] event Pointer to data area of event.
  /// \param [in] pbk Pointer to the bank header, must be NULL for the
  /// first call to this function.
  /// \param [in] pdata Pointer to data area of bank, NULL if bank not
  /// found.
  /// \returns Size of bank in bytes.
  ///
  if (*pbk == NULL)
    *pbk = (Bank_t *) (((BankHeader_t *) event) + 1);
  else
    *pbk = (Bank_t *) ((char*) (*pbk + 1) + ((((*pbk)->fDataSize)+7) & ~7));

  *pdata = (char*)((*pbk) + 1);

  if ((uint32_t) * pbk - (uint32_t) event >=
      ((BankHeader_t *) event)->fDataSize + sizeof(BankHeader_t)) {
    *pbk = *((Bank_t **) pdata) = NULL;
    return 0;
  }
  return (*pbk)->fDataSize;
}

int TMidasEvent::IterateBank32(void *event, Bank32_t **pbk, char **pdata)
{
  /// Iterates through banks inside an event. The function can be used
  /// to enumerate all banks of an event.
  /// \param [in] event Pointer to data area of event.
  /// \param [in] pbk Pointer to the bank header, must be NULL for the
  /// first call to this function.
  /// \param [in] pdata Pointer to data area of bank, NULL if bank not
  /// found.
  /// \returns Size of bank in bytes.
  ///
  if (*pbk == NULL)
    *pbk = (Bank32_t *) (((BankHeader_t *) event) + 1);
  else
    *pbk = (Bank32_t *) ((char*) (*pbk + 1) +
                         ((((*pbk)->fDataSize)+7) & ~7));

  *pdata = (char*)((*pbk) + 1);

  if ((uint32_t) * pbk - (uint32_t) event >=
      ((BankHeader_t *) event)->fDataSize + sizeof(BankHeader_t)) {
    *pbk = *((Bank32_t **) pdata) = NULL;
    return 0;
  }
  return (*pbk)->fDataSize;
}

void TMidasEvent::SwapBytesEventHeader()
{
  /// The event header may need byte-swapping.
  ///
  WORD_SWAP(&fEventHeader.fEventId);
  WORD_SWAP(&fEventHeader.fTriggerMask);
  DWORD_SWAP(&fEventHeader.fSerialNumber);
  DWORD_SWAP(&fEventHeader.fTimeStamp);
  DWORD_SWAP(&fEventHeader.fDataSize);
}
