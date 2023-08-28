#include "BirthdayChecker.h"
#include "Core/Core.h"

BirthdayChecker g_BDayChecker;

void BirthdayChecker::OnUpdate() {
    if (m_nextCheck > GetCurrentTime()) return;
    if (!m_bMidnightCheckDone){
        for (auto& c : g_classmates){
            c.m_daysUntilBirthday = DaysUntilBirthday(c.m_birthday);
        }
        m_bMidnightCheckDone = true;
        m_nextCheck += 25200; //next check will be at 7 o'clock, to send notifications
        return;
    }
    m_nextCheck = GetCurrentTime() + GetSecondsToMidnight();
    m_bMidnightCheckDone = false;

    unsigned short birthdaysToday = 0;
    std::string birthdaysMsg = "";

    for (auto& c : g_classmates){
        if (c.m_daysUntilBirthday == 0){
            birthdaysToday++;
            birthdaysMsg += "🎂 " + c.m_name + "\n";
        }
    }

    if (IsMonday()) { //Who will have birthday this week
        unsigned short birthdaysThisWeek = 0;
        std::string messageStr = "";
        for (auto& c : g_classmates){
            if (c.m_daysUntilBirthday <= 6) {
                birthdaysThisWeek++;
                messageStr += "🎂 " + c.m_name + ": ";
                switch (c.m_daysUntilBirthday){
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