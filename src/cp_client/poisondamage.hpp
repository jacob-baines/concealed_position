#pragma once

#include "exploit.h"

class PoisonDamage : public Exploit {
public:

	PoisonDamage();

	virtual ~PoisonDamage();

	virtual bool do_exploit();

private:


private:

	const std::string m_target_directory;
	const std::string m_target_dll;
	const std::string m_malicious_dll;
};

