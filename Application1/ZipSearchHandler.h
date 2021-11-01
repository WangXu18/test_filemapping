#pragma once
#include <atlstr.h>
#include "zip/unzip.h"
#include "FileMapping.h"
#include "ApplicationData.h"

class ZipSearchHandler
{
public:
    ZipSearchHandler();
    ~ZipSearchHandler();
    BOOL HandleSearch(const CString& strZipFilePath);

private:
    HZIP OpenZipFile(const CString& strZipFilePath);
    BOOL UnZip(HZIP hZip, int nWaitUnZipCount = 1, const CString& strParentPath = L"");
    BOOL IsZipFile(const CString& strFileName);
    BOOL GetZipInfo(HZIP hZip, int& nTotalFileCountRet, int& nZipFileCountRet);

private:
    int m_nTotalUnZipFile = 0;
    int m_nWaitUnZipCount = 1;
    int m_nResultType = 1;
};