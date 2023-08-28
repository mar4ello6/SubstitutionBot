#include "SubstitutionChecker.h"
#include "Core/Core.h"
#include <vector>

SubstitutionChecker g_subChecker;


void SubstitutionChecker::OnUpdate(){
    //we don't want to ddos edupage, right? so we have some delay between requests
    if (m_nextCheck > GetCurrentTime()) return;
    m_nextCheck = GetCurrentTime() + SUBSTITUTION_CHECK_DELAY;

    CheckSubstitutions();
}

void SubstitutionChecker::CheckSubstitutions(){
    g_cache.CleanUp();
    m_subsForAWeek.clear(); //I'm too lazy to run checks if substitutions are still valid...
    std::vector<std::pair<time_t, std::vector<Edupage::Substitution>>> newSubs;
    std::map<time_t, std::pair<int,bool>> hashesCache;
    for (int i = 0; i < 7; i++){
        time_t rawtime = time(NULL);
        tm *time = localtime(&rawtime);
        time->tm_mday += i;
        time_t rawSubTime = mktime(time);

        std::vector<Edupage::Substitution> subs;
        try {
            subs = Edupage::GetSubstitutions(g_config.m_class, time);
        } catch (...) { continue; }
        if (subs.size() > 0) m_subsForAWeek.push_back({rawSubTime, subs});

        int newHash = 0;
        bool isInDB = false;
        if (!g_cache.hasSubstitutionChanged(time, newHash, isInDB, subs)) continue;

        hashesCache[rawSubTime] = {newHash, isInDB};
        newSubs.push_back({rawSubTime, subs});
    }

    if (newSubs.size() < 1) return; //no subs, no sending

    if (OnNew(newSubs)) g_cache.SaveNewSubstitutions(newSubs, hashesCache);

}

bool SubstitutionChecker::OnNew(std::vector<std::pair<time_t, std::vector<Edupage::Substitution>>> newSubs){
    if (g_config.m_targetChat == 0) {
        printf("Target chat is unset!\n");
        exit(1);
        return false;
    }
    if (!g_bot){
        printf("Bot is NULL?! How is this even possible???\n");
        exit(1);
        return false;
    }

    std::string message = "Замены обновились!\n\n";
    for (auto& n : newSubs){
        message += TimeToDate(localtime(&n.first)) + ":\n";
        if (n.second.size() < 1){
            message += "Замен нет. 😢\n";
        }
        else {
            for (auto& s : n.second) message += s.GetString() + "\n";
        }
        message += "\n";
    }

    try {
        g_bot->getApi().sendMessage(g_config.m_targetChat, message, false, 0, nullptr, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending new substitutions: %s\n", e.what());
        return false;
    }

    return true;
}