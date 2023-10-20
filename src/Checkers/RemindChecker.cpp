#include "RemindChecker.h"
#include "Core/Core.h"
#include <nlohmann/json.hpp>

Reminder::Reminder(){
    memset(&m_time, 0, sizeof(m_time));
}

RemindChecker g_remindChecker;

RemindChecker::RemindChecker(){
    std::ifstream file("reminders.json");
    if (file.is_open()){
        try {
            nlohmann::json j;
            file >> j;
            nlohmann::json reminders = j["reminders"];
            time_t currentTime = time(NULL);
            tm* currentTm = localtime(&currentTime);
            currentTm->tm_hour = 0;
            currentTm->tm_min = 0;
            currentTm->tm_sec = 0;
            currentTime = mktime(currentTm);
            for (auto& r : reminders){
                Reminder reminder;
                reminder.m_text = r["text"];
                std::vector<std::string> birthday = Explode(r["time"], "-");
                if (birthday.size() != 3) continue;
                reminder.m_time.tm_year = atoi(birthday[0].c_str()) - 1900;
                reminder.m_time.tm_mon = atoi(birthday[1].c_str()) - 1;
                reminder.m_time.tm_mday = atoi(birthday[2].c_str());
                reminder.m_daysTo = round(difftime(mktime(&reminder.m_time), currentTime) / 86400);
                reminder.m_daysBeforeReminder = r["daysBeforeReminder"].get<std::vector<unsigned char>>();
                m_reminders.push_back(reminder);
            }
        }
        catch (...) {} //i don't really care if this fails
        std::sort(m_reminders.begin(), m_reminders.end());
    }
}

void RemindChecker::OnUpdate() {
    if (m_nextCheck > GetCurrentTime()) return;
    if (!m_bMidnightCheckDone){
        time_t currentTime = time(NULL);
        tm* currentTm = localtime(&currentTime);
        currentTm->tm_hour = 0;
        currentTm->tm_min = 0;
        currentTm->tm_sec = 0;
        currentTime = mktime(currentTm);
        bool removeOldOnes = false;
        for (auto& r : m_reminders){
            r.m_daysTo = round(difftime(mktime(&r.m_time), currentTime) / 86400);
            if (r.m_daysTo < 0) removeOldOnes = true;
        }
        if (removeOldOnes){
            for (size_t i = 0; i < m_reminders.size(); i++){
                if (m_reminders[i].m_daysTo < 0){
                    m_reminders.erase(m_reminders.begin() + i);
                    i--;
                }
            }
            SaveReminders();
        }
        m_bMidnightCheckDone = true;
        m_nextCheck += 25200; //next check will be at 7 o'clock, to send notifications
        return;
    }
    m_nextCheck = GetCurrentTime() + GetSecondsToMidnight();
    m_bMidnightCheckDone = false;

    std::string msg = "";
    for (auto& r : m_reminders){
        bool shouldNotify = r.m_daysTo == 0;
        if (!shouldNotify){
            for (auto& d : r.m_daysBeforeReminder){
                if (d == r.m_daysTo){
                    shouldNotify = true;
                    break;
                }
            }
        }

        if (shouldNotify){
            if (msg.empty()) msg += "Напоминания:\n";
            msg += "Через " + DaysToMonthsText(r.m_daysTo) + ": " + r.m_text + "\n\n";
        }
    }

    if (!msg.empty()){
        msg = msg.substr(0, msg.length() - 2);
        try {
            g_bot->getApi().sendMessage(g_config.m_targetChat, msg, false, 0, nullptr, "HTML");
        } catch (std::exception& e) { 
            printf("Caught exception while sending reminders: %s\n", e.what());
        }
    }
}

void RemindChecker::AddReminder(Reminder reminder){
    time_t currentTime = time(NULL);
    tm* currentTm = localtime(&currentTime);
    currentTm->tm_hour = 0;
    currentTm->tm_min = 0;
    currentTm->tm_sec = 0;
    currentTime = mktime(currentTm);
    reminder.m_daysTo = round(difftime(mktime(&reminder.m_time), currentTime) / 86400);
    m_reminders.push_back(reminder);
    std::sort(m_reminders.begin(), m_reminders.end());
    SaveReminders();
}

void RemindChecker::RemoveReminder(size_t index){
    if (m_reminders.size() <= index) return;
    m_reminders.erase(m_reminders.begin() + index);
    SaveReminders();
}

void RemindChecker::SaveReminders(){
    nlohmann::json j;
    j["reminders"] = nlohmann::json::array();
    for (auto& r : m_reminders){
        nlohmann::json jr;
        jr["text"] = r.m_text;
        jr["time"] = TimeToDate(&r.m_time);
        jr["daysBeforeReminder"] = r.m_daysBeforeReminder;
        j["reminders"].push_back(jr);
    }
    std::ofstream file("reminders.json", std::ofstream::out | std::ofstream::trunc);
    file << std::setw(4) << j << std::endl;
    file.close();
}