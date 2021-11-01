// ConsoleApplication1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <intsafe.h>
#include <iosfwd>
#include <fstream>
#include <string>
#include <list>
#include <algorithm>
#include <vector>
#include "KDubaPath.h"
#include "ZipSearchHandler.h"
#include <xutility>
#include <windows.h>

#include "FileMapping.h"
#include "CmdLineParser.h"
#include "ApplicationData.h"
#include <atlconv.h>
#include <future>

std::list<Record> rd_list_;

bool sort_by_size(const Record& rd1, const Record& rd2) {
  return rd1.len < rd2.len;
}

bool zip_sort_by_size(const ZipFileInfo& f1, const ZipFileInfo& f2) {
  return f1.size < f2.size;
}

std::vector<Address> vec_address_;

void prepare() {
  CString strPath = ApplicationData::GetInstance()->GetInputDir();
  strPath += L"\\input_pattern.txt";
  std::ifstream file_in;
  file_in.open((LPCTSTR)strPath, std::ios::in);
  while (!file_in.is_open())
    return ;

  CString strMemPath = ApplicationData::GetInstance()->GetOutputDir();
  CString strMemRecord = strMemPath + L"records.mem";
  std::ofstream file_out;
  file_out.open((LPCTSTR)strMemRecord, std::ios::out | std::ios::binary);
  if (!file_out.is_open())
    return ;

  Record record;
  std::string line;
  while (std::getline(file_in, line)) {
    int pos = line.find(":");
    if (pos == std::string::npos)
      continue;
    std::string str = line.substr(0, pos);
    record.len = std::stoul(str);

    str = line.substr(pos + 1, 40);
    memcpy(record.sh1, str.c_str(), 40);
    //strcpy_s(record.sh1, 41, str.c_str());

    pos = line.find(":", pos + 1 + 40);
    if (pos == std::string::npos)
      continue;

    str = line.substr(pos + 1);
    strcpy_s(record.name, DEF_MAX_STRING_LEN, str.c_str());

    rd_list_.push_back(record);
  }

  // 设计缺陷,加一条数据方便时算大小
  Record rd{ UINT(-1),"e2eee67d73255b8ffea8ed9acf73a8a5a8d275a","test" };
  rd_list_.push_back(rd);

  std::vector<Address> vec_address;
  UINT pre_size = (UINT)-1;
  UINT addr_offset = 0;
  rd_list_.sort(sort_by_size);
  for (auto it : rd_list_) {
    Record &record = it;

    if (pre_size != record.len) {
      pre_size = record.len;
      Address address{ record.len, addr_offset };
      vec_address.emplace_back(address);
    }

    int stru_size = 4 + 40 + strlen(record.name) + 1;
    file_out.write((char*)&record, stru_size);
    addr_offset += stru_size;
  }
  file_out.close();

  CString strMemIndex = strMemPath + L"index.mem";
  std::ofstream file_out_index;
  file_out_index.open((LPCTSTR)strMemIndex, std::ios::trunc | std::ios::binary);
  if (!file_out_index.is_open())
    return;

  int index_size = vec_address.size() * sizeof(Address);
  file_out_index.write((char*)&index_size, sizeof(int));
  file_out_index.write((char*)&vec_address[0], index_size);
}

bool LoadMem() {

  CString strMemPath = ApplicationData::GetInstance()->GetOutputDir();
  CString strMemIndex = strMemPath + L"index.mem";
  std::ifstream file_in;
  file_in.open((LPCTSTR)strMemIndex, std::ios::in | std::ios::binary);
  while (!file_in.is_open())
    return false;

  int total_size = 0;
  file_in.read((char*)&total_size, sizeof(int));

  int count = total_size / sizeof(Address);
  vec_address_.resize(count);
  file_in.read((char*)&vec_address_[0], total_size);

  return true;
}

UINT SearchAddress(UINT size) {
  Address address{ size, 0 };
  auto it = std::lower_bound(vec_address_.begin(), vec_address_.end(), address);
  if(it == vec_address_.end())
    return UINT(-1);
  return it->address;
}

void handleSearch() 
{
    std::vector<CString> vecFileList;
    CString strInputDir = ApplicationData::GetInstance()->GetInputDir();
    vecFileList.push_back(strInputDir + L"input1.zip");
    vecFileList.push_back(strInputDir + L"input2.zip");

    std::vector< std::future<void> > futers(vecFileList.size());
    for (size_t i = 0; i < futers.size(); ++i) {
      futers[i] = std::async(std::launch::async, [&,i] {
        ZipSearchHandler handler;
        handler.HandleSearch(vecFileList[i]);
      });
    }

    for (auto& it : futers) {
      if (it.valid())
        it.get();
    }

    auto vecFileInfo = ApplicationData::GetInstance()->GetZipFileInfoList();
    std::sort(vecFileInfo.begin(), vecFileInfo.end(), zip_sort_by_size);
    std::vector<ZipFileInfo> result;
    FileMapping file_map(vec_address_);
    for (auto fileInfo : vecFileInfo)
    {
        if (file_map.FindRecord(fileInfo.size, fileInfo.address, fileInfo.sha1.c_str(), fileInfo.name))
        {
            result.push_back(fileInfo);
        }
    }

    CString strPath = ApplicationData::GetInstance()->GetOutputDir();

    std::wofstream file_out1;
    file_out1.open((LPCTSTR)(strPath+L"\\result_1.txt"), std::ios::trunc);
    if (!file_out1.is_open())
      return;

    std::wofstream file_out2;
    file_out2.open((LPCTSTR)(strPath + L"\\result_2.txt"), std::ios::trunc);
    if (!file_out2.is_open())
      return;

    for (auto fileInfo : result) {
      CString content = fileInfo.path + L":" + CA2W(fileInfo.name.c_str());

      if(fileInfo.resultType == 1)
        file_out1 << (LPCTSTR)content << L"\n";
      else
        file_out2 << (LPCTSTR)content << L"\n";
    }
}

int __stdcall WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd)
{
    CCmdLineParser cmdParser(::GetCommandLineW());
    ApplicationData::GetInstance()->SetInputDir(cmdParser.GetVal(L"input"));
    ApplicationData::GetInstance()->SetOutputDir(cmdParser.GetVal(L"output"));
    int nPrepare = cmdParser.GetIntVal(L"prepare", 0);
    bool bPrepare = nPrepare != 0;
    if (bPrepare) 
    {
        prepare();
    }
    else
    {
        if(!LoadMem())
          return 0;

        handleSearch();
    }
    return 0;
}