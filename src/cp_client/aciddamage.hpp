#pragma once

#include "exploit.h"

class AcidDamage : public Exploit {
public:

    AcidDamage();

    virtual ~AcidDamage();

    virtual bool do_exploit();

private:


private:

    const std::string m_target_directory;
    const std::string m_target_dll;
};