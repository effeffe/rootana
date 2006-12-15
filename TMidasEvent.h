//
// TMidasEvent.h
//

#ifndef TMIDASEVENT_H
#define TMIDASEVENT_H

#include "TMidasBanks.h"

///
/// C++ class representing one midas event.
///
/// Objects of this class are created by reading
/// midas events from a file, by reading
/// them from a midas shared memory buffer or by
/// receiving them through the mserver
///

/// MIDAS event

class TMidasEvent
{
 public:
  TMidasEvent(); ///< default constructor
  TMidasEvent(const TMidasEvent &); ///< copy constructor
  ~TMidasEvent(); ///< destructor
  TMidasEvent& operator=(const TMidasEvent &); ///< assignement operator
  void Clear(); ///< prepare object for destruction
  void Print(const char* = "") const;
  int FindBank(BankHeader_t*, const char* bankName, int* bankLength, int* bankType, void **) const;
  int LocateBank(void *event, const char* bankName, void **pdata) const;
  int SwapBytes(bool);
  char* GetData() const;
  const char* GetBankList() const;
  uint16_t GetEventId() const;
  uint16_t GetTriggerMask() const;
  uint32_t GetSerialNumber() const;
  uint32_t GetTimeStamp() const;
  uint32_t GetDataSize() const;
  EventHeader_t* GetEventHeader();
  void AllocateData();
  int SetBankList();
  void SetData(char* dataBuffer);
  void SetEventId(uint16_t eventId);
  void SetTriggerMask(uint16_t triggerMask);
  void SetSerialNumber(uint32_t serial);
  void SetTimeStamp(uint32_t timestamp);
  void SetDataSize(uint32_t dataSize);
  bool IsGoodSize() const;
  void SwapBytesEventHeader();

protected:
  const int fBankListMax; ///< maximum number of banks allowed in an event, currently 64
  const int fStringBankListMax; ///< maximum bank string size allowed in an event

  char* fData;     ///< event data buffer
  char* fBankList; ///< pointer to the event bank list
  int  fBanksN;    ///< number of banks in this event
  bool fAllocated; ///< "true" if we own and should delete the data buffer
  EventHeader_t fEventHeader; ///< event header

private:
  void Copy(const TMidasEvent &); ///< copy helper
  int IterateBank(void *event, Bank_t **, char **pdata);
  int IterateBank32(void *event, Bank32_t **, char **pdata);
};

inline void TMidasEvent::SetEventId(uint16_t eventId)
{ 
  /// Set the event identification-number.
  /// \param [in] eventId The event identification number.
  ///
  fEventHeader.fEventId = eventId;
}

inline void TMidasEvent::SetTriggerMask(uint16_t triggerMask)
{
  /// Set the event trigger-mask.
  /// \param [in] triggerMask The event trigger mask.
  ///
  fEventHeader.fTriggerMask = triggerMask;
}

inline void TMidasEvent::SetSerialNumber(uint32_t serialNumber)
{
  /// Set the event serial-number.
  /// \param [in] serialNumber The event serial number.
  ///
  fEventHeader.fSerialNumber = serialNumber;
}

inline void TMidasEvent::SetTimeStamp(uint32_t timeStamp)
{
  /// Set the event time-stamp.
  /// \param [in] timeStamp The event time stamp.
  ///
  fEventHeader.fTimeStamp = timeStamp;
}

inline void TMidasEvent::SetDataSize(uint32_t dataSize)
{
  /// Set the event data-size.
  /// \param [in] dataSize The size of the event in bytes.
  ///
  fEventHeader.fDataSize = dataSize;
}

inline void TMidasEvent::SetData(char* data)
{
  /// Set the event data - useful for online sorting when the data
  /// does not need to be copied into the class.
  /// \param [in] data A pointer to the event data.
  ///
  fData = data;
  fAllocated = false;
}

inline const char* TMidasEvent::GetBankList() const
{
  /// Get a list of the banks within the event.
  /// \returns A pointer to the list of banks.
  ///
  return fBankList;
}

inline uint16_t TMidasEvent::GetEventId() const
{
  /// Get the event identification number.
  /// \returns The event identification number.
  ///
  return fEventHeader.fEventId;
}

inline uint16_t TMidasEvent::GetTriggerMask() const
{
  /// Get the event trigger mask.
  /// \returns The event trigger mask.
  ///
  return fEventHeader.fTriggerMask;
}

inline uint32_t TMidasEvent::GetSerialNumber() const
{
  /// Get the event serial number (incremented separately for each
  /// type of event).
  /// \returns The event serial number.
  ///
  return fEventHeader.fSerialNumber;
}

inline uint32_t TMidasEvent::GetTimeStamp() const
{
  /// Get the event time stamp.
  /// \returns The event time stamp.
  ///
  return fEventHeader.fTimeStamp;
}

inline uint32_t TMidasEvent::GetDataSize() const
{
  /// Get the event data-size.
  /// \returns The event data-size in bytes.
  ///
  return fEventHeader.fDataSize;
}

inline char* TMidasEvent::GetData() const
{
  /// Get the event data.
  /// \returns A pointer to the data.
  ///
  return fData;
}

inline EventHeader_t * TMidasEvent::GetEventHeader()
{
  /// Get the event header (usually to fill in the fields).
  /// \returns The event header.
  ///
  return &fEventHeader;
}

inline bool TMidasEvent::IsGoodSize() const
{
  /// Check if the event is a good size.
  /// \returns True if the data has a sensible size, false otherwise.
  ///
  return fEventHeader.fDataSize > 0 || fEventHeader.fDataSize <= 1024 * 1024;
}

#endif // TMidasEvent.h
