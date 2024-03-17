#pragma once
#include <string>
#include <vector>

class BirthdayChecker{
public:
    void OnUpdate();

private:
    template<typename T> void MidnightCheck(std::vector<T>& ppl);
    template<typename T> void AddTodayBdayText(unsigned short& birthdaysToday, std::string& birthdaysMsg, const std::vector<T>& ppl);
    template<typename T> void AddWeekBdayText(unsigned short& birthdaysThisWeek, std::string& messageStr, const std::vector<T>& ppl);

    long long m_nextCheck = 0;
    bool m_bMidnightCheckDone = false;

};
extern BirthdayChecker g_BDayChecker;