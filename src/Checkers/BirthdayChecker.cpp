#include "BirthdayChecker.h"
#include "Core/Core.h"

BirthdayChecker g_BDayChecker;

void BirthdayChecker::OnUpdate() {
    if (m_nextCheck > GetCurrentTime()) return;
    if (!m_bMidnightCheckDone){
        MidnightCheck<Classmate>(g_classmates);
        MidnightCheck<Teacher>(g_teachers);
        m_bMidnightCheckDone = true;
        m_nextCheck += 25200; //next check will be at 7 o'clock, to send notifications
        return;
    }
    m_nextCheck = GetCurrentTime() + GetSecondsToMidnight();
    m_bMidnightCheckDone = false;

    unsigned short birthdaysToday = 0;
    std::string birthdaysMsg = "";
    AddTodayBdayText<Classmate>(birthdaysToday, birthdaysMsg, g_classmates);
    AddTodayBdayText<Teacher>(birthdaysToday, birthdaysMsg, g_teachers);

    if (IsMonday()) { //Who will have birthday this week
        unsigned short birthdaysThisWeek = 0;
        std::string messageStr = "";
        AddWeekBdayText<Classmate>(birthdaysThisWeek, messageStr, g_classmates);
        AddWeekBdayText<Teacher>(birthdaysThisWeek, messageStr, g_teachers);

        if (birthdaysThisWeek > 0){
            if (birthdaysThisWeek == 1) messageStr = "На этой неделе будет день рождения:\n" + messageStr.substr(0, messageStr.length() - 1);
            else messageStr = "На этой неделе будут дни рождения:\n" + messageStr.substr(0, messageStr.length() - 1);

            try {
                g_bot->getApi().sendMessage(g_config.m_targetChat, messageStr, false, 0, nullptr, "HTML");
            } catch (std::exception& e) { 
                printf("Caught exception while sending birthdays this week: %s\n", e.what());
            }
        }
    }

    if (birthdaysToday > 0){
        birthdaysMsg = "Сегодня день рождения у:\n" + birthdaysMsg.substr(0, birthdaysMsg.length() - 1);
        try {
            g_bot->getApi().sendMessage(g_config.m_targetChat, birthdaysMsg, false, 0, nullptr, "HTML");
        } catch (std::exception& e) { 
            printf("Caught exception while sending birthdays today: %s\n", e.what());
        }
    }
}

template<typename T> void BirthdayChecker::MidnightCheck(std::vector<T>& ppl) {
    time_t rawtime = time(NULL);
    tm *time = localtime(&rawtime);
    for (auto& p : ppl){
        //we need to do this in case it is leap year
        tm bdayTime = p.m_birthday;
        bdayTime.tm_year = time->tm_year;
        mktime(&bdayTime);

        p.m_daysUntilBirthday = DaysUntilBirthday(p.m_birthday);
        if (p.m_daysUntilBirthday == 0){
            p.m_age = time->tm_year - p.m_birthday.tm_year;
        }
        if (p.m_daysUntilBirthday >= 364){
            p.m_bdayAge = time->tm_year - p.m_birthday.tm_year;
            if (bdayTime.tm_yday < time->tm_yday) p.m_bdayAge++;
        }
    }
}

template<typename T> void BirthdayChecker::AddTodayBdayText(unsigned short& birthdaysToday, std::string& birthdaysMsg, const std::vector<T>& ppl){
    std::string emoji = "🎂";
    bool needSpacer = false;
    if (std::is_same<T, Teacher>::value) {
        emoji = "🧑‍🏫🎂";
        needSpacer = birthdaysToday > 0;
    }

    for (auto& p : ppl){
        if (p.m_daysUntilBirthday == 0){
            if (needSpacer) {
                birthdaysMsg += "\n";
                needSpacer = false;
            }
            birthdaysToday++;
            birthdaysMsg += emoji + " " + p.m_name + " (" + std::to_string(p.m_bdayAge) + ")\n";
        }
    }
}

template<typename T> void BirthdayChecker::AddWeekBdayText(unsigned short& birthdaysThisWeek, std::string& messageStr, const std::vector<T>& ppl){
    std::string emoji = "🎂";
    bool needSpacer = false;
    if (std::is_same<T, Teacher>::value) {
        emoji = "🧑‍🏫🎂";
        needSpacer = birthdaysThisWeek > 0;
    }

    for (auto& p : ppl){
        if (p.m_daysUntilBirthday <= 6) {
            if (needSpacer) {
                messageStr += "\n";
                needSpacer = false;
            }
            birthdaysThisWeek++;
            messageStr += "🎂 " + p.m_name + " (" + std::to_string(p.m_bdayAge) + "): ";
            switch (p.m_daysUntilBirthday){
                case 0:
                    messageStr += "СЕГОДНЯ!";
                break;
                case 1:
                    messageStr += "завтра.";
                break;
                case 2:
                    messageStr += "в среду.";
                break;
                case 3:
                    messageStr += "в четверг.";
                break;
                case 4:
                    messageStr += "в пятницу.";
                break;
                case 5:
                    messageStr += "в субботу.";
                break;
                case 6:
                    messageStr += "в воскресенье.";
                break;
            }
            messageStr += "\n";
        }
    }
}