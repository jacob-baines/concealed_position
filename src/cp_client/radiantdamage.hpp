#pragma once

#include "exploit.h"

class RadiantDamage : public Exploit {
public:

	RadiantDamage();

	virtual ~RadiantDamage();

	virtual bool do_exploit();

private:


private:

	const std::string m_target_directory;
	const std::string m_target_dll;
	const std::string m_malicious_dll;
};