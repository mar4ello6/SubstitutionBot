#pragma once
#include <string>
#include <vector>
#include <memory>
#include <tgbot/tgbot.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include "Checkers/SubstitutionChecker.h"

class Cache{
public:

    Cache();

    /**
     * Cleans some stuff (removes outdated items in cache).
    */
    void CleanUp();
    /**
     * Checks if substitution has changed since last check.
    */
    bool hasSubstitutionChanged(tm * time, int& newHash, bool& isInDB, std::vector<Edupage::Substitution> subs);
    /**
     * Saves new substitutions' hash.
    */
    void SaveNewSubstitutions(std::vector<std::pair<time_t, std::vector<Edupage::Substitution>>> newSubs, std::map<time_t, std::pair<int,bool>>& hashesCache);

private:

    SQLite::Database m_db;

};
extern Cache g_cache;

struct Config{

    Config();

    std::string m_timetableURL = ""; //Timetable's (and substitutions') root url
    std::string m_class = ""; //Class as it's written in substitutions
    std::string m_TGtoken = ""; //Telegram bot token
    int64_t m_targetChat = 0; //Where will it send messages about substitutions. Right now we will need to do it manually

};
extern Config g_config;

extern TgBot::Bot* g_bot;

long long GetCurrentTime();
std::string GetDate(int dayOffset = 0);
std::string TimeToDate(tm* time);

uint32_t HashString(unsigned char* str, int len);


template<typename ... Args>
std::string string_format( const std::string& format, Args ... args ) {
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    if( size_s <= 0 ){ return ""; }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 );
}