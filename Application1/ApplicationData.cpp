#include "ApplicationData.h"
#include "KDubaPath.h"

ApplicationData* ApplicationData::GetInstance()
{
    static ApplicationData obj;
    return &obj;
}

void ApplicationData::SetInputDir(const CString& strDir)
{
    m_strInputDir = strDir;
    KDubaPath::PathAddBackslash(m_strInputDir);
}

CString ApplicationData::GetInputDir()
{
    return m_strInputDir;
}

void ApplicationData::SetOutputDir(const CString& strDir)
{
    m_strOutputDir = strDir;
    KDubaPath::PathAddBackslash(m_strOutputDir);
}

CString ApplicationData::GetOutputDir()
{
    return m_strOutputDir;
}

void ApplicationData::AppendZipFileInfoList(std::vector<ZipFileInfo>& list)
{
    std::lock_guard<std::mutex> locker(m_lock);
    m_zipFileInfoList.insert(m_zipFileInfoList.end(), list.begin(), list.end());
}

const std::vector<ZipFileInfo>& ApplicationData::GetZipFileInfoList()
{
    std::lock_guard<std::mutex> locker(m_lock);
    return m_zipFileInfoList;
}

ApplicationData::ApplicationData()
{

}

ApplicationData::ApplicationData(const ApplicationData& obj)
{

}

