#include "CoursesChecker.h"
#include "Core/Core.h"

CoursesChecker g_coursesChecker;

void CoursesChecker::OnUpdate() {
    if (m_nextCheck > GetCurrentTime()) return;
    if (!m_bMidnightCheckDone){
        LoadCourses();
        m_bMidnightCheckDone = true;
        m_nextCheck += 25200; //next check will be at 7 o'clock, to send notifications
        return;
    }
    m_nextCheck = GetCurrentTime() + GetSecondsToMidnight();
    m_bMidnightCheckDone = false;

    std::string msg = "";
    for (auto& c : g_courses){
        if (c.m_daysToEnd == 7 || c.m_daysToEnd == 14){
            if (!msg.empty()) msg += "\n\n";
            msg += "Через ";
            if (c.m_daysToEnd == 7) msg += "неделю ";
            else if (c.m_daysToEnd == 14) msg += "2 недели ";
            else msg += "??? ";
            msg += "закончатся следующие курсы: ";
            for (auto& s : c.m_subjects) msg += s + ", ";
            msg = msg.substr(0, msg.length() - 2);
            msg += ".";
        }
    }

    if (!msg.empty()){
        try {
            g_bot->getApi().sendMessage(g_config.m_targetChat, msg, false, 0, nullptr, "HTML");
        } catch (std::exception& e) { 
            printf("Caught exception while sending ending courses: %s\n", e.what());
        }
    }
}