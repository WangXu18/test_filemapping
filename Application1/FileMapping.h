#pragma once
#include <vector>
#include <string>

#define DEF_MAX_STRING_LEN  64

struct Address {
  UINT size;
  UINT address;
  bool operator < (const Address& b) {
    return size < b.size;
  }
};

struct Record {
  UINT len;
  char sh1[40];
  char name[DEF_MAX_STRING_LEN];
};

class FileMapping
{
public:
  FileMapping(std::vector<Address> &address);

  bool SetMemoryRange(UINT start, UINT end);
  bool FindRecord(UINT size, UINT address, const char* sh1, std::string &name);
protected:
  HANDLE file_;
  HANDLE file_mapping_;
  DWORD page_size_;
  UINT align_start_;
  UINT align_end_;
  UINT start_;
  UINT end_;
  std::vector<Address>& vec_address_;
  std::vector<UINT> vec_aver_address;
  char *data_;
};

