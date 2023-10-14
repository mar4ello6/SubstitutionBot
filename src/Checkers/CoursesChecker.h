#pragma once
#include <string>
#include <vector>

class CoursesChecker{
public:
    void OnUpdate();

private:
    long long m_nextCheck = 0;
    bool m_bMidnightCheckDone = false;

};
extern CoursesChecker g_coursesChecker;