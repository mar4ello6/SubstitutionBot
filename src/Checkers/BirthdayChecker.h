#pragma once
#include <string>
#include <vector>

class BirthdayChecker{
public:
    void OnUpdate();

private:
    long long m_nextCheck = 0;

};
extern BirthdayChecker g_BDayChecker;