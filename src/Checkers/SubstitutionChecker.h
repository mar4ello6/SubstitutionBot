#pragma once
#include <string>
#include <vector>
#include "edupage/Substitution.h"

#define SUBSTITUTION_CHECK_DELAY 60 //in seconds

class SubstitutionChecker{
public:

    void OnUpdate();

    std::vector<std::pair<time_t, std::vector<Edupage::Substitution>>> getSubsForAWeek() {return m_subsForAWeek;}

private:

    void CheckSubstitutions();

    std::vector<std::pair<time_t, std::vector<Edupage::Substitution>>> m_subsForAWeek;

    long long m_nextCheck = 0;

    bool OnNew(std::vector<std::pair<time_t, std::vector<Edupage::Substitution>>> newSubs);

};
extern SubstitutionChecker g_subChecker;