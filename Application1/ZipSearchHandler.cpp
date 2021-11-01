#include "ZipSearchHandler.h"
#include <memory>
#include "KDubaPath.h"
#include "sha1.h"

extern UINT SearchAddress(UINT size);

namespace
{
    const int MAX_WAIT_UNZIP_COUNT = 1000;
    const long MAX_FILE_SIZE = 500 * 1024 * 1024L;
}

ZipSearchHandler::ZipSearchHandler()
{
}

ZipSearchHandler::~ZipSearchHandler()
{
    
}

BOOL ZipSearchHandler::HandleSearch(const CString& strZipFilePath)
{
    m_nResultType = strZipFilePath.Find(L"input1") > 0;
    HZIP hZip = OpenZipFile(strZipFilePath);
    return UnZip(hZip);
}

HZIP ZipSearchHandler::OpenZipFile(const CString& strZipFilePath)
{
    if (!KDubaPath::IsFileExist(strZipFilePath))
    {
        return NULL;
    }

    return OpenZip(strZipFilePath.GetString(), NULL);
}

BOOL ZipSearchHandler::UnZip(HZIP hZip, int nWaitUnZipCount, const CString& strParentPath)
{
    m_nTotalUnZipFile++;
    int nFileCount = 0;
    int nZipFileCount = 0;
    if (!GetZipInfo(hZip, nFileCount, nZipFileCount) || nFileCount <= 0)
    {
        return FALSE;
    }

    if (nZipFileCount > 0)
    {
        nWaitUnZipCount = nWaitUnZipCount * nZipFileCount;
    }

    if (nWaitUnZipCount >= MAX_WAIT_UNZIP_COUNT)
    {
        return FALSE;
    }

    std::vector<ZipFileInfo> vecFileInfo;
    ZIPENTRY itemEntry = { 0 };
    ZipFileInfo info;
    for (int nIndex = 0; nIndex < nFileCount; nIndex++)
    {
        ZRESULT result = GetZipItem(hZip, nIndex, &itemEntry);
        if (result != ZR_OK)
        {
            continue;
        }

        long nFileSize = itemEntry.unc_size;
        if (nFileSize <= 0 || nFileSize >= MAX_FILE_SIZE)
        {
            continue;
        }

        CString strPath = itemEntry.name;
        if (!strParentPath.IsEmpty())
        {
            strPath.Format(L"%s//%s", strParentPath, CString(itemEntry.name));
        }

        UINT address = SearchAddress(nFileSize);
        if (!IsZipFile(itemEntry.name))
        {
            //check file is hit size cache
            if (address == (UINT)-1)
            {
                continue;
            }
        }

        std::shared_ptr<char> pBuffer(new char[nFileSize], std::default_delete<char[]>());
        result = UnzipItem(hZip, nIndex, pBuffer.get(), nFileSize);
        if (result != ZR_OK)
        {
            continue;
        }

        HZIP hSubZip = OpenZip(pBuffer.get(), nFileSize, NULL);
        if (hSubZip != NULL)
        {
            if (strPath.GetLength() < MAX_PATH)
            {
                UnZip(hSubZip, nWaitUnZipCount, strPath);
            }
        }
        
        SHA1 sha1;
        sha1.add(pBuffer.get(), itemEntry.unc_size);
        info.sha1 = sha1.getHash().c_str();
        info.size = itemEntry.unc_size;
        info.address = address;
        info.path = strPath;
        info.resultType = m_nResultType;
        vecFileInfo.push_back(info);
    }

    ApplicationData::GetInstance()->AppendZipFileInfoList(vecFileInfo);
    CloseZip(hZip);
    return TRUE;
}

BOOL ZipSearchHandler::IsZipFile(const CString& strFileName)
{
    return KDubaPath::GetFileExtend(strFileName).CompareNoCase(L".zip") == 0;
}

BOOL ZipSearchHandler::GetZipInfo(HZIP hZip, int& nFileCountRet, int& nZipFileCountRet)
{
    if (hZip == NULL)
    {
        return FALSE;
    }

    ZIPENTRY zipEntry = { 0 };
    ZRESULT result = GetZipItem(hZip, -1, &zipEntry);
    if (result != ZR_OK)
    {
        CloseZip(hZip);
        return FALSE;
    }

    int nFileCount = zipEntry.index;
    int nZipFileCount = 0;
    for (int nIndex = 0; nIndex < nFileCount; nIndex++)
    {
        ZRESULT result = GetZipItem(hZip, nIndex, &zipEntry);
        if (result != ZR_OK)
        {
            continue;
        }

        if (IsZipFile(zipEntry.name))
        {
            ++nZipFileCount;
        }
    }

    nFileCountRet = nFileCount;
    nZipFileCountRet = nZipFileCount;
    return TRUE;
}