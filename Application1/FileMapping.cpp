#include <windows.h>
#include "FileMapping.h"
#include <sysinfoapi.h>
#include <assert.h>
#include "CmdLineParser.h"
#include "ApplicationData.h"

const DWORD kFileShareAll =
FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

FileMapping::FileMapping(std::vector<Address>& address)
  : file_(INVALID_HANDLE_VALUE)
  , file_mapping_(INVALID_HANDLE_VALUE)
  , page_size_(0)
  , align_start_(0)
  , align_end_(0)
  , start_(0)
  , end_(0)
  , data_(nullptr)
  , vec_address_(address){

  SYSTEM_INFO system_info;
  GetSystemInfo(&system_info);

  page_size_ = system_info.dwAllocationGranularity;

  CString strMemPath = ApplicationData::GetInstance()->GetOutputDir();
  CString strMemRecord = strMemPath + L"records.mem";
  file_ =
    ::CreateFile((LPCTSTR)strMemRecord,
      GENERIC_READ,
      kFileShareAll,
      NULL,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      NULL);

  if (file_ == NULL)
    return;

  file_mapping_ = ::CreateFileMapping(file_, NULL, PAGE_READONLY,
    0, 0, NULL);

  UINT n = 10;
  if (vec_address_.size() > 10000)
    n = 100;
  UINT aver = vec_address_.size() / n;
  for (size_t i = 1; i < n; ++i) {
    UINT address = vec_address_[i * aver].address;
    vec_aver_address.push_back(address);
  }
  if(vec_aver_address.back() != vec_address_.back().address)
    vec_aver_address.push_back(vec_address_.back().address);
}

bool FileMapping::SetMemoryRange(UINT start, UINT end) {
  if(data_ != nullptr)
    UnmapViewOfFile(data_);
  align_start_ = start / page_size_ * page_size_;
  //DWORD size = (end - start + page_size_) / page_size_ * page_size_;
  data_ = (char*)MapViewOfFile(file_mapping_, FILE_MAP_READ, 0, align_start_, end - align_start_);
  start_ = start;
  end_ = end;

  return data_ != nullptr;
}

bool FileMapping::FindRecord(UINT size, UINT address, const char* sh1, std::string& name) {
  assert(address >= start_);
  if (address < start_ || address >= end_ || data_ == nullptr) {
    for (auto it : vec_aver_address) {
      UINT size = it;
      if (address < it) {
        SetMemoryRange(address, it);
        break;
      }
    }
  }
  
  UINT rec_offset = address;
  while (rec_offset < end_) {
    Record* record = (Record*)(data_ + (rec_offset - align_start_));
    if(record->len != size)
      return false;
    int len = strlen(record->name);
    rec_offset += sizeof(int) + sizeof(record->sh1) + len + 1;

    if (memcmp(record->sh1, sh1, sizeof(record->sh1)) == 0) {
      name = record->name;
      return true;
    }
  }

  return false;
}