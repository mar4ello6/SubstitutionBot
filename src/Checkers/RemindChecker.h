#pragma once
#include <string>
#include <vector>
#include <ctime>

struct Reminder{
    Reminder();

    std::string m_text = "";
    tm m_time; //when
    short m_daysTo = 0;

    std::vector<unsigned char> m_daysBeforeReminder;

    bool operator<(const Reminder& compr) { return m_daysTo < compr.m_daysTo; }
};

class RemindChecker{
public:
    RemindChecker();
    void OnUpdate();

    std::vector<Reminder> GetReminders() { return m_reminders; }
    void AddReminder(Reminder reminder);
    void RemoveReminder(size_t index);

private:
    void SaveReminders();

    long long m_nextCheck = 0;
    bool m_bMidnightCheckDone = false;

    std::vector<Reminder> m_reminders;

};
extern RemindChecker g_remindChecker;