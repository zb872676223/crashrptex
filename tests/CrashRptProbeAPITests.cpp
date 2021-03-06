/************************************************************************************* 
This file is a part of CrashRpt library.

Copyright (c) 2003, Michael Carruth
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this 
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution.

* Neither the name of the author nor the names of its contributors 
may be used to endorse or promote products derived from this software without 
specific prior written permission.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************/

#include "stdafx.h"
#include "Tests.h"
#include "CrashRptProbe.h"
#include "CrashRpt.h"
#include "Utility.h"
#include "strconv.h"

// A helper function that creates a error report for testing
BOOL CreateErrorReport(CString sTmpFolder, CString& sErrorReportName, CString& sMD5Hash)
{
    BOOL bStatus = FALSE;
    CString sReportFolder;
    DWORD dwExitCode = 1;
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    CString sSearchPattern = sTmpFolder + "\\*.zip";
    CString sMD5FileName;
    FILE* f = NULL;
    TCHAR szHashBuff[256] = _T("");
    HKEY hKey = NULL;
    LONG lResult = -1;
    CString sKeyName = _T("Software\\CrashRpt&&#4216wer\\应用程序名称");
    CString sKeyName2 = _T("HKEY_CURRENT_USER\\") + sKeyName;

    lResult = RegCreateKey(HKEY_CURRENT_USER, sKeyName, &hKey); 
    if(lResult!=ERROR_SUCCESS)
        goto cleanup;

    DWORD dwVal = 12345;
    lResult = RegSetValueEx(hKey, _T("Value$%^!@#&123fer"), 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(DWORD));
    if(lResult!=ERROR_SUCCESS)
        goto cleanup;

    CR_INSTALL_INFOW infoW;
    memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
    infoW.cb = sizeof(CR_INSTALL_INFOW);  
    infoW.pszAppName = L"My& app Name & ' 应用程序名称"; // Use Chineese characters for app name
    // Use appname with restricted XML characters
    infoW.pszAppVersion = L"1.0.0 &<'a应> \"<"; 
    infoW.pszErrorReportSaveDir = sTmpFolder;
    infoW.dwFlags = CR_INST_NO_GUI|CR_INST_DONT_SEND_REPORT|CR_INST_STORE_ZIP_ARCHIVES;  

    int nInstallResult = crInstallW(&infoW);
    if(nInstallResult!=0)
        goto cleanup;

    crAddScreenshot(CR_AS_MAIN_WINDOW);
    crAddPropertyW(L"CustomProp", L"Property Value");
    crAddRegKey(sKeyName2, L"regkey.xml", 0);

    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(ei);
    ei.exctype = CR_SEH_EXCEPTION;
    ei.code = 0x123;

    // Generate error report
    int nGenResult = crGenerateErrorReport(&ei);
    if(nGenResult!=0)
        goto cleanup;

    // Wait until CrashSender process exits
    WaitForSingleObject(ei.hSenderProcess, INFINITE);

    // Check exit code  
    GetExitCodeProcess(ei.hSenderProcess, &dwExitCode);
    if(dwExitCode!=0)
        goto cleanup;

    // Get ZIP name  
    hFind = FindFirstFile(sSearchPattern, &ffd);
    if(hFind==INVALID_HANDLE_VALUE)
        goto cleanup;

    sErrorReportName = sTmpFolder + _T("\\") + CString(ffd.cFileName);

    FindClose(hFind);
    hFind = NULL;

    // Get MD5 name
    sSearchPattern = sTmpFolder + "\\*.md5";
    hFind = FindFirstFile(sSearchPattern, &ffd);
    if(hFind==INVALID_HANDLE_VALUE)
        goto cleanup;

    sMD5FileName = sTmpFolder + _T("\\") + CString(ffd.cFileName);

#if _MSC_VER < 1400
    f = _tfopen(sMD5FileName, _T("rt"));
#else
    _tfopen_s(&f, sMD5FileName, _T("rt"));
#endif
    if(f==NULL)
        goto cleanup;

    TCHAR* szHash = _fgetts(szHashBuff, 256, f);
    if(szHash==NULL)
        goto cleanup;

    sMD5Hash = szHash;

    if(sMD5Hash.GetLength()!=32)
        goto cleanup; // Hash must be 32 characters in length

    bStatus = TRUE;

cleanup:

    crUninstall();

    if(f!=NULL)
        fclose(f);

    if(hFind!=INVALID_HANDLE_VALUE)
        FindClose(hFind);

    if(hKey)    
        RegCloseKey(hKey);

    RegDeleteKey(HKEY_CURRENT_USER, sKeyName);

    return bStatus;
}


class CrashRptProbeAPITests : public CTestSuite
{
    BEGIN_TEST_MAP(CrashRptProbeAPITests, "CrashRptProbe API function tests")
        REGISTER_TEST(Test_crpOpenErrorReportA)
        REGISTER_TEST(Test_crpOpenErrorReportW)
        REGISTER_TEST(Test_crpCloseErrorReport)    
        REGISTER_TEST(Test_crpExtractFileW)
        REGISTER_TEST(Test_crpExtractFileA)
        REGISTER_TEST(Test_crpGetLastErrorW)
        REGISTER_TEST(Test_crpGetLastErrorA)
        REGISTER_TEST(Test_crpGetPropertyW)
        REGISTER_TEST(Test_crpGetPropertyA)
        REGISTER_TEST(Test_crpGetProperty)
#ifndef CRASHRPT_LIB
        REGISTER_TEST(Test_crashrptprobe_dll_file_version)
#endif //!CRASHRPT_LIB
    END_TEST_MAP()

public:

    void SetUp();
    void TearDown();

    void Test_crpOpenErrorReportA();
    void Test_crpOpenErrorReportW();
    void Test_crpCloseErrorReport();
    void Test_crpExtractFileW();
    void Test_crpExtractFileA();
    void Test_crpGetLastErrorW();
    void Test_crpGetLastErrorA();
    void Test_crpGetPropertyW();
    void Test_crpGetPropertyA();
    void Test_crpGetProperty();
#ifndef CRASHRPT_LIB
    void Test_crashrptprobe_dll_file_version();
#endif //!CRASHRPT_LIB

    CString m_sTmpFolderW;
    CString m_sErrorReportNameW;
    CString m_sMD5HashW;

    CString m_sTmpFolderA;
    CString m_sErrorReportNameA;
    CString m_sMD5HashA;
};

REGISTER_TEST_SUITE( CrashRptProbeAPITests );

void CrashRptProbeAPITests::SetUp()
{
    CString sAppDataFolder;

    // Create a temporary folder for wide-char test
    Utility::GetSpecialFolder(CSIDL_APPDATA, sAppDataFolder);
    m_sTmpFolderW = sAppDataFolder+_T("\\CrashRpt 应用程序名称");
    BOOL bCreate = Utility::CreateFolder(m_sTmpFolderW);
    TEST_ASSERT(bCreate);

    // Create error report ZIP
    BOOL bCreateReport = CreateErrorReport(m_sTmpFolderW, m_sErrorReportNameW, m_sMD5HashW);
    TEST_ASSERT(bCreateReport);

    // Create a temporary folder for ANSI test 
    Utility::GetSpecialFolder(CSIDL_APPDATA, sAppDataFolder);
    m_sTmpFolderA = sAppDataFolder+_T("\\CrashRpt");
    BOOL bCreate2 = Utility::CreateFolder(m_sTmpFolderA);
    TEST_ASSERT(bCreate2);

    // Create error report ZIP
    BOOL bCreateReport2 = CreateErrorReport(m_sTmpFolderA, m_sErrorReportNameA, m_sMD5HashA);
    TEST_ASSERT(bCreateReport2);

    __TEST_CLEANUP__;
}

void CrashRptProbeAPITests::TearDown()
{
    // Delete tmp folder
    Utility::RecycleFile(m_sTmpFolderW, TRUE);

    Utility::RecycleFile(m_sTmpFolderA, TRUE);
}

void CrashRptProbeAPITests::Test_crpOpenErrorReportW()
{ 
    strconv_t strconv;
    CrpHandle hReport = 0;  

    // Open NULL report - should fail
    int nOpenResult = crpOpenErrorReportW(NULL, NULL, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult!=0);

    // Open report - should succeed
    LPCWSTR szReportName = strconv.t2w(m_sErrorReportNameW);
    int nOpenResult2 = crpOpenErrorReportW(szReportName, NULL, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult2==0 && hReport!=0);

    // Close report - should succeed
    int nCloseResult = crpCloseErrorReport(hReport);
    TEST_ASSERT(nCloseResult==0);
    hReport = 0;

    // Open report and check MD5 - should succeed
    LPCWSTR szMD5Hash = strconv.t2w(m_sMD5HashW);
    int nOpenResult3 = crpOpenErrorReportW(szReportName, szMD5Hash, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult3==0 && hReport!=0);

    // Close report - should succeed
    int nCloseResult2 = crpCloseErrorReport(hReport);
    TEST_ASSERT(nCloseResult2==0);
    hReport = 0;

    // Open report with incorrect MD5 - should fail
    LPCWSTR szInvalidMD5 = L"1234567890123456";
    int nOpenResult4 = crpOpenErrorReportW(szReportName, szInvalidMD5, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult4!=0);

    // Open not existing file - should fail  
    int nOpenResult5 = crpOpenErrorReportW(L"NotExisting.zip", NULL, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult5!=0);

    __TEST_CLEANUP__;

    crpCloseErrorReport(hReport);  
}

void CrashRptProbeAPITests::Test_crpOpenErrorReportA()
{   
    strconv_t strconv;
    CrpHandle hReport = 0;    

    // Open NULL report - should fail
    int nOpenResult = crpOpenErrorReportA(NULL, NULL, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult!=0);

    // Open report - should succeed
    LPCSTR szReportName = strconv.t2a(m_sErrorReportNameA);
    int nOpenResult2 = crpOpenErrorReportA(szReportName, NULL, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult2==0 && hReport!=0);

    // Close report - should succeed
    int nCloseResult = crpCloseErrorReport(hReport);
    TEST_ASSERT(nCloseResult==0);
    hReport = 0;

    // Open report and check MD5 - should succeed
    LPCSTR szMD5Hash = strconv.t2a(m_sMD5HashA);
    int nOpenResult3 = crpOpenErrorReportA(szReportName, szMD5Hash, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult3==0 && hReport!=0);

    // Close report - should succeed
    int nCloseResult2 = crpCloseErrorReport(hReport);
    TEST_ASSERT(nCloseResult2==0);
    hReport = 0;

    // Open report with incorrect MD5 - should fail
    LPCSTR szInvalidMD5 = "1234567890123456";
    int nOpenResult4 = crpOpenErrorReportA(szReportName, szInvalidMD5, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult4!=0);

    // Open not existing file - should fail  
    int nOpenResult5 = crpOpenErrorReportW(L"NotExisting.zip", NULL, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult5!=0);

    __TEST_CLEANUP__;

    crpCloseErrorReport(hReport);  
}

void CrashRptProbeAPITests::Test_crpCloseErrorReport()
{ 
    CrpHandle hReport = 3;

    // Close invalid report - should fail
    int nCloseResult = crpCloseErrorReport(hReport);
    TEST_ASSERT(nCloseResult!=0);

    __TEST_CLEANUP__;
}

void CrashRptProbeAPITests::Test_crpExtractFileW()
{   
    strconv_t strconv;
    CrpHandle hReport = 0;    
    const int BUFF_SIZE = 1024;
    WCHAR szBuffer[BUFF_SIZE] = L"";
    int nDmpFileCount = 0;
    int nXmlFileCount = 0;
    int nPngFileCount = 0;
    int nOtherFileCount = 0;

    // Open report - should succeed
    LPCWSTR szReportName = strconv.t2w(m_sErrorReportNameW);
    int nOpenResult = crpOpenErrorReportW(szReportName, NULL, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult==0 && hReport!=0);

    // Enumerate files contained in error report and extract each one

    int nRowCount = crpGetPropertyW(hReport, CRP_TBL_XMLDESC_FILE_ITEMS, CRP_META_ROW_COUNT, 0, NULL, 0, NULL); 
    TEST_ASSERT(nRowCount>0); // Ensure there are files in the report

    int i;
    for(i=0; i<nRowCount; i++)
    {
        // Get file name
        int nResult = crpGetPropertyW(hReport, CRP_TBL_XMLDESC_FILE_ITEMS, CRP_COL_FILE_ITEM_NAME, i, szBuffer, BUFF_SIZE, NULL);
        TEST_ASSERT(nResult==0);

        CString sDstFile = m_sTmpFolderW + _T("\\") + CString(szBuffer);
        strconv_t strconv;
        LPCWSTR szDstFile = strconv.t2w(sDstFile);

        // Check file extension
        CString sExt;
        int nDotPos = sDstFile.ReverseFind('.');
        if(nDotPos>=0)
            sExt = sDstFile.Mid(nDotPos);

        if(sExt.CompareNoCase(_T(".XML"))==0)
            nXmlFileCount++;
        else if(sExt.CompareNoCase(_T(".DMP"))==0)
            nDmpFileCount++;
        else if(sExt.CompareNoCase(_T(".PNG"))==0)
            nPngFileCount++;
        else
            nOtherFileCount++;

        // Extract file - should succeed
        int nExtract = crpExtractFileW(hReport, szBuffer, szDstFile, FALSE);
        TEST_ASSERT(nExtract==0);

        // Check that file exists
        DWORD dwAttrs = GetFileAttributesW(szDstFile);
        TEST_ASSERT(dwAttrs!=INVALID_FILE_ATTRIBUTES);

        // Extract file the second time - should fail, because it already exists
        int nExtract2 = crpExtractFileW(hReport, szBuffer, szDstFile, FALSE);
        TEST_ASSERT(nExtract2!=0);

        // Extract file the second time and overwrite existing - should succeed
        int nExtract3 = crpExtractFileW(hReport, szBuffer, szDstFile, TRUE);
        TEST_ASSERT(nExtract3==0);

        // Check that file exists
        DWORD dwAttrs2 = GetFileAttributesW(szDstFile);
        TEST_ASSERT(dwAttrs2!=INVALID_FILE_ATTRIBUTES);

        // Extract file that doesnt exist - should fail
        int nExtract4 = crpExtractFileW(hReport, L"NotExisting.txt", szDstFile, TRUE);
        TEST_ASSERT(nExtract4!=0);
    }   

    // Enusure there are two XML file (assume it is crashrpt.xml and regkey.xml)
    TEST_ASSERT(nXmlFileCount==2);

    // Enusure there is exactly one DMP file (assume it is crashdump.dmp)
    TEST_ASSERT(nDmpFileCount>=1);

    // Enusure there is at least one PNG file (desktop screenshot)
    TEST_ASSERT(nPngFileCount>=1);

    __TEST_CLEANUP__;

    crpCloseErrorReport(hReport);  
}

void CrashRptProbeAPITests::Test_crpExtractFileA()
{ 
    strconv_t strconv;
    CrpHandle hReport = 0;
    const int BUFF_SIZE = 1024;
    char szBuffer[BUFF_SIZE] = "";

    // Open report - should succeed
    LPCSTR szReportName = strconv.t2a(m_sErrorReportNameA);
    int nOpenResult = crpOpenErrorReportA(szReportName, NULL, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult==0 && hReport!=0);

    // Enumerate files contained in error report and extract each one

    int nRowCount = crpGetPropertyA(hReport, "XmlDescFileItems", "RowCount", 0, NULL, 0, NULL); 
    TEST_ASSERT(nRowCount>0);

    int i;
    for(i=0; i<nRowCount; i++)
    {
        // Get file name
        int nResult = crpGetPropertyA(hReport, "XmlDescFileItems", "FileItemName", i, szBuffer, BUFF_SIZE, NULL);
        TEST_ASSERT(nResult==0);

        CString sDstFile = m_sTmpFolderA + _T("\\") + CString(szBuffer);
        strconv_t strconv;
        LPCSTR szDstFile = strconv.t2a(sDstFile);

        // Extract file - should succeed
        int nExtract = crpExtractFileA(hReport, szBuffer, szDstFile, FALSE);
        TEST_ASSERT(nExtract==0);

        // Check that file exists
        DWORD dwAttrs = GetFileAttributesA(szDstFile);
        TEST_ASSERT(dwAttrs!=INVALID_FILE_ATTRIBUTES);

        // Extract file the second time - should fail, because it already exists
        int nExtract2 = crpExtractFileA(hReport, szBuffer, szDstFile, FALSE);
        TEST_ASSERT(nExtract2!=0);

        // Extract file the second time and overwrite existing - should succeed
        int nExtract3 = crpExtractFileA(hReport, szBuffer, szDstFile, TRUE);
        TEST_ASSERT(nExtract3==0);

        // Check that file exists
        DWORD dwAttrs2 = GetFileAttributesA(szDstFile);
        TEST_ASSERT(dwAttrs2!=INVALID_FILE_ATTRIBUTES);

        // Extract file that doesnt exist - should fail
        int nExtract4 = crpExtractFileA(hReport, "NotExisting.txt", szDstFile, TRUE);
        TEST_ASSERT(nExtract4!=0);
    }   

    __TEST_CLEANUP__;

    crpCloseErrorReport(hReport);
}

void CrashRptProbeAPITests::Test_crpGetLastErrorW()
{   
    // Get error message
    WCHAR szErrMsg[256] = L"";
    int nResult = crpGetLastErrorMsgW(szErrMsg, 256);
    TEST_ASSERT(nResult>0);

    // Get error message to NULL buffer - must fail  
    int nResult2 = crpGetLastErrorMsgW(NULL, 256);
    TEST_ASSERT(nResult2<0);

    // Get error message to a buffer, but zero length - must fail  
    WCHAR szErrMsg2[256] = L"";
    int nResult3 = crpGetLastErrorMsgW(szErrMsg2, 0);
    TEST_ASSERT(nResult3<0);

    // Get error message to a single-char buffer, must trunkate message and succeed
    WCHAR szErrMsg3[1] = L"";
    int nResult4 = crpGetLastErrorMsgW(szErrMsg3, 1);
    TEST_ASSERT(nResult4==0);

    // Get error message to a small buffer, must trunkate message and succeed
    WCHAR szErrMsg6[2] = L"";
    int nResult6 = crpGetLastErrorMsgW(szErrMsg6, 2);
    TEST_ASSERT(nResult6>0);

    __TEST_CLEANUP__;

}

void CrashRptProbeAPITests::Test_crpGetLastErrorA()
{   
    // Get error message
    char szErrMsg[256] = "";
    int nResult = crpGetLastErrorMsgA(szErrMsg, 256);
    TEST_ASSERT(nResult>0);

    // Get error message to NULL buffer - must fail  
    int nResult2 = crpGetLastErrorMsgA(NULL, 256);
    TEST_ASSERT(nResult2<0);

    // Get error message to a buffer, but zero length - must fail  
    char szErrMsg2[256] = "";
    int nResult3 = crpGetLastErrorMsgA(szErrMsg2, 0);
    TEST_ASSERT(nResult3<0);

    // Get error message to a single-char buffer, must trunkate message and succeed
    char szErrMsg3[1] = "";
    int nResult4 = crpGetLastErrorMsgA(szErrMsg3, 1);
    TEST_ASSERT(nResult4==0);

    // Get error message to a small buffer, must trunkate message and succeed
    char szErrMsg6[2] = "";
    int nResult6 = crpGetLastErrorMsgA(szErrMsg6, 2);
    TEST_ASSERT(nResult6>0);

    __TEST_CLEANUP__;

}

void CrashRptProbeAPITests::Test_crpGetPropertyW()
{   
    strconv_t strconv;
    CrpHandle hReport = 0;  
    const int BUFF_SIZE = 1024;
    WCHAR szBuffer[BUFF_SIZE];
    ULONG uCount = 0;

    // Get property from unopened report - should fail
    int nResult = crpGetPropertyW(hReport, CRP_TBL_XMLDESC_MISC, CRP_COL_APP_NAME, 
        0, szBuffer, BUFF_SIZE, &uCount);
    TEST_ASSERT(nResult!=0 && uCount==0);

    // Open report - should succeed
    LPCWSTR szReportName = strconv.t2w(m_sErrorReportNameW);
    int nOpenResult = crpOpenErrorReportW(szReportName, NULL, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult==0 && hReport!=0);

    // Get property from opened report - should succeed
    int nResult2 = crpGetPropertyW(hReport, CRP_TBL_XMLDESC_MISC, CRP_COL_APP_NAME, 
        0, szBuffer, BUFF_SIZE, &uCount);
    TEST_ASSERT(nResult2==0 && uCount>0);

    __TEST_CLEANUP__;

    crpCloseErrorReport(hReport);
}

void CrashRptProbeAPITests::Test_crpGetPropertyA()
{   
    strconv_t strconv;
    CrpHandle hReport = 0;
    const int BUFF_SIZE = 1024;
    char szBuffer[BUFF_SIZE];
    ULONG uCount = 0;

    // Get property from unopened report - should fail
    int nResult = crpGetPropertyA(hReport, "XmlDescMisc", "AppName", 
        0, szBuffer, BUFF_SIZE, &uCount);
    TEST_ASSERT(nResult!=0 && uCount==0);

    // Open report - should succeed
    LPCSTR szReportName = strconv.t2a(m_sErrorReportNameA);
    int nOpenResult = crpOpenErrorReportA(szReportName, NULL, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult==0 && hReport!=0);

    // Get property from opened report - should succeed
    int nResult2 = crpGetPropertyA(hReport, "XmlDescMisc", "AppName", 
        0, szBuffer, BUFF_SIZE, &uCount);
    TEST_ASSERT(nResult2==0 && uCount>0);

    __TEST_CLEANUP__;

    crpCloseErrorReport(hReport);

}


void CrashRptProbeAPITests::Test_crpGetProperty()
{   
    strconv_t strconv;
    CrpHandle hReport = 0;  
    const int BUFF_SIZE = 1024;
    TCHAR szBuffer[BUFF_SIZE];
    ULONG uCount = 0;

    // Open report - should succeed
    int nOpenResult = crpOpenErrorReport(m_sErrorReportNameW, NULL, NULL, 0, &hReport);
    TEST_ASSERT(nOpenResult==0 && hReport!=0);

    // Get row count in CRP_TBL_XMLDESC_MISC table - should return 1 (this table always has single row)
    int nResult = crpGetProperty(hReport, CRP_TBL_XMLDESC_MISC, CRP_META_ROW_COUNT, 
        0, szBuffer, BUFF_SIZE, &uCount);
    TEST_ASSERT(nResult==1 && uCount==0);

    // Get row count in CRP_TBL_XMLDESC_FILE_ITEMS table - should return >0 
    int nResult2 = crpGetProperty(hReport, CRP_TBL_XMLDESC_FILE_ITEMS, CRP_META_ROW_COUNT, 
        0, szBuffer, BUFF_SIZE, &uCount);
    TEST_ASSERT(nResult2>1 && uCount==0);

    // Get row count in CRP_TBL_XMLDESC_FILE_ITEMS table - should return >0 
    int nResult3 = crpGetProperty(hReport, CRP_TBL_XMLDESC_FILE_ITEMS, CRP_META_ROW_COUNT, 
        0, szBuffer, BUFF_SIZE, &uCount);
    TEST_ASSERT(nResult3>1 && uCount==0);

    // Get row count in CRP_TBL_XMLDESC_CUSTOM_PROPS table - should return 1 (added one custom property) 
    int nResult4 = crpGetProperty(hReport, CRP_TBL_XMLDESC_CUSTOM_PROPS, CRP_META_ROW_COUNT, 
        0, szBuffer, BUFF_SIZE, &uCount);
    TEST_ASSERT(nResult4==1 && uCount==0);

    // Get row count in CRP_TBL_MDMP_MISC table - should return 1 (always has one row) 
    int nResult5 = crpGetProperty(hReport, CRP_TBL_MDMP_MISC, CRP_META_ROW_COUNT, 
        0, szBuffer, BUFF_SIZE, &uCount);
    TEST_ASSERT(nResult5==1 && uCount==0);

    // Get row count in CRP_TBL_MDMP_MODULES table - should return >0 
    int nResult6 = crpGetProperty(hReport, CRP_TBL_MDMP_MODULES, CRP_META_ROW_COUNT, 
        0, szBuffer, BUFF_SIZE, &uCount);
    TEST_ASSERT(nResult6>0 && uCount==0);

    // Get row count in CRP_TBL_MDMP_THREADS table - should return >0 
    int nResult7 = crpGetProperty(hReport, CRP_TBL_MDMP_THREADS, CRP_META_ROW_COUNT, 
        0, szBuffer, BUFF_SIZE, &uCount);
    TEST_ASSERT(nResult7>0 && uCount==0);

    __TEST_CLEANUP__;

    crpCloseErrorReport(hReport);

}

#ifndef CRASHRPT_LIB
void CrashRptProbeAPITests::Test_crashrptprobe_dll_file_version()
{
    // Check that CrashRpt.dll file version equals to CRASHRPT_VERSION constant

    HMODULE hModule = NULL;
    TCHAR szModuleName[_MAX_PATH] = _T("");
    DWORD dwBuffSize = 0;
    LPBYTE pBuff = NULL;
    VS_FIXEDFILEINFO* fi = NULL;
    UINT uLen = 0;

    // Load CrashRpt.dll dynamically
    CString sDllPath;
#ifdef _DEBUG    
    sDllPath.Format(_T("%s\\CrashRptProbe%dd.dll"), (LPCTSTR) Utility::GetModulePath(NULL), CRASHRPT_VER);    
#else
    sDllPath.Format(_T("%s\\CrashRptProbe%d.dll"), (LPCTSTR) Utility::GetModulePath(NULL), CRASHRPT_VER);
#endif
    hModule = LoadLibrary(sDllPath);
    TEST_ASSERT(hModule!=NULL);

    // Get module file name
    GetModuleFileName(hModule, szModuleName, _MAX_PATH);

    // Get module version 
    dwBuffSize = GetFileVersionInfoSize(szModuleName, 0);
    TEST_ASSERT(dwBuffSize!=0);    

    pBuff = (LPBYTE)GlobalAlloc(GPTR, dwBuffSize);
    TEST_ASSERT(pBuff!=NULL);   

    TEST_ASSERT(0!=GetFileVersionInfo(szModuleName, 0, dwBuffSize, pBuff));    

    VerQueryValue(pBuff, _T("\\"), (LPVOID*)&fi, &uLen);

    WORD dwVerMajor = HIWORD(fi->dwProductVersionMS);
    WORD dwVerMinor = LOWORD(fi->dwProductVersionMS);  
    WORD dwVerBuild = LOWORD(fi->dwProductVersionLS);

    DWORD dwModuleVersion = dwVerMajor*1000+dwVerMinor*100+dwVerBuild;

    TEST_ASSERT(CRASHRPT_VER==dwModuleVersion);

    __TEST_CLEANUP__

    if(pBuff)
    {
        // Free buffer
        GlobalFree((HGLOBAL)pBuff);
        pBuff = NULL;
    }

    FreeLibrary(hModule);
}
#endif //!CRASHRPT_LIB