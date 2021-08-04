# Concealed Position

Concealed Position is a local privilege escalation attack against Windows using the concept of "Bring Your Own Vulnerability". Specifically, Concealed Position (CP) uses the *as designed* package point and print logic in Windows that allows a low privilege user to stage and install printer drivers. CP specifically installs drivers with known vulnerabilities which are then exploited to escalate to SYSTEM. Concealed Position was first presented at DEF CON 29.

## What exploits are available
Concealed Position offers four exploits - all with equally dumb names:

* ACIDDAMAGE - [CVE-2021-35449](https://nvd.nist.gov/vuln/detail/CVE-2021-35449) - Lexmark Universal Print Driver LPE
* RADIANTDAMAGE - [CVE-2021-38085](https://nvd.nist.gov/vuln/detail/CVE-2021-38085) - Canon TR150 Print Driver LPE
* POISONDAMAGE - [CVE-2019-19363](https://nvd.nist.gov/vuln/detail/CVE-2019-19363) - Ricoh PCL6 Print Driver LPE
* SLASHINGDAMAGE - [CVE-2020-1300](https://nvd.nist.gov/vuln/detail/CVE-2020-1300) - Windows Print Spooler LPE

The exploits are neat because, besides SLASHINGDAMAGE, they will continue working even after the issues are patched. The only mechanism Windows has to stop users from using old drivers is to revoke the driver's certificate - something that is not(?) historically done.

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
Arguably, yes. The driver store is a ["trusted collection of ... third-party driver packages"](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/driver-store) that requires administrator access to modify. Using `GetPrinterDriver` a low privileged attacker can stage arbitrary drivers into the store. This, to me, is a clear security boundary.

Microsoft seemed to agree when they issued [CVE-2021-34481](https://msrc.microsoft.com/update-guide/vulnerability/CVE-2021-34481).

Although... it's arguable that this is simply a feature of the system and not a vulnerability at all. It really doesn't matter all that much. An attacker can escalate to SYSTEM on standard Windows installs.

### Other things

One thing to note is that the inject_me dll is actually embedded in the cp_client as a C array. If you update inject_me, you'll need to manually update the C array as well (just use xxd to generate the array).
