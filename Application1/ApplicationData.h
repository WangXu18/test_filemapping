#pragma once
#include <atlstr.h>
#include <mutex>
#include <vector>

struct ZipFileInfo
{
    int resultType = 1;
    UINT address = 0;
    UINT size = 0;
    std::string sha1;
    std::string name;
    CString path;
};

class ApplicationData
{
public:
    static ApplicationData* GetInstance();

public:
    void SetInputDir(const CString& strDir);
    CString GetInputDir();
    void SetOutputDir(const CString& strDir);
    CString GetOutputDir();

    void AppendZipFileInfoList(std::vector<ZipFileInfo>& list);
    const std::vector<ZipFileInfo>& GetZipFileInfoList();

private:
    ApplicationData();
    ApplicationData(const ApplicationData& obj);

private:
    std::mutex m_lock;
    CString m_strInputDir;
    CString m_strOutputDir;
    std::vector<ZipFileInfo> m_zipFileInfoList;
};