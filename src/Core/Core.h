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

    std::string m_schoolSiteURL = "";
    std::string m_schoolMenuLocation = "";
    std::string m_schoolMenuButtonLabel = "";

    int64_t m_targetChat = 0; //Where will it send messages about substitutions. Right now we will need to do it manually

    std::string m_studentSite = ""; //Site with some info for students

};
extern Config g_config;

struct Classmate{
    Classmate();

    std::string m_name = "";
    int64_t m_tgID = 0;
    std::vector<unsigned char> m_groups;
    tm m_birthday;
    unsigned short m_daysUntilBirthday = 0;
    unsigned char m_age = 0;
    unsigned char m_bdayAge = 0;

    bool operator<(const Classmate& compr) { return m_daysUntilBirthday < compr.m_daysUntilBirthday; }
    bool IsInGroup(unsigned char groupID);
};
extern std::vector<Classmate> g_classmates;
void LoadClassmates();
unsigned short DaysUntilBirthday(tm birthday);

struct Group{
    std::string m_name = "";
    unsigned char m_id = 0;
};
extern std::vector<Group> g_groups;
void LoadGroups();

struct Course{
    Course();

    tm m_end;
    short m_daysToEnd = 0;
    std::vector<std::string> m_subjects;

    bool operator<(const Course& compr) { return m_daysToEnd < compr.m_daysToEnd; }
};
extern std::vector<Course> g_courses;
void LoadCourses();

struct Holiday{
    Holiday();

    tm m_start;
    tm m_end;
    short m_daysTo = 0; //days to start or end
    bool m_bStarted = false;
};
extern std::vector<Holiday> g_holidays;
void LoadHolidays(); //it's loaded when someone uses command /holidays

extern TgBot::Bot* g_bot;

long long GetCurrentTime();
int GetSecondsToMidnight();
bool IsMonday();
std::string GetDate(int dayOffset = 0);
std::string TimeToDate(tm* time, bool addDayName = false);
std::string DaysToMonthsText(unsigned short days);

int RandomInt(int min, int max);

uint32_t HashString(unsigned char* str, int len);
std::vector<std::string> Explode(std::string str, std::string token);


template<typename ... Args>
std::string string_format( const std::string& format, Args ... args ) {
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    if( size_s <= 0 ){ return ""; }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 );
}