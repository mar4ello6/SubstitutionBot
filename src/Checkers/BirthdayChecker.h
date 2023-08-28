#pragma once
#include <string>
#include <vector>

class BirthdayChecker{
public:
    void OnUpdate();

private:
    long long m_nextCheck = 0;
    bool m_bMidnightCheckDone = false;

};
extern BirthdayChecker g_BDayChecker;