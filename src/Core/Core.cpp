#include <fstream>
#include <chrono>
#include <nlohmann/json.hpp>
#include "Core.h"

Cache g_cache;

Cache::Cache() : m_db("cache.db3", SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE) {
    try {
        if (!m_db.tableExists("SubstitutionCache")) { //checking if SubstitutionCache table exists, if not, create it
            SQLite::Statement query(m_db, "CREATE TABLE 'SubstitutionCache' ('date' DATETIME, 'latestHash' INTEGER)");
            int result = query.exec();
            if (result != 0) {
                printf("Crerating SubstitutionCache table returned %i.\n", result);
                exit(1);
            }
        }
    } catch (std::exception& e){
        printf("Caught an exception while trying to create SubstitutionCache table: %s\n", e.what());
        exit(1);
    }
    CleanUp();
}

void Cache::CleanUp(){
    try {
        SQLite::Statement query(m_db, "DELETE FROM \"SubstitutionCache\" WHERE \"date\"<\"" + GetDate() + "\"");
        if (query.exec() > 0){
            SQLite::Statement vacuum(m_db, "VACUUM");
            vacuum.exec();
        }
    } catch (std::exception& e){
        printf("Caught an exception while cleaning up: %s\n", e.what());
    }
}

bool Cache::hasSubstitutionChanged(tm * time, int& newHash, bool& isInDB, std::vector<Edupage::Substitution> subs){
    int latestHash = 0;
    try {
        SQLite::Statement query(m_db, "SELECT * FROM \"SubstitutionCache\" WHERE \"date\"=\"" + TimeToDate(time) + "\"");
        while (query.executeStep()){
            latestHash = query.getColumn(1).getInt();
            isInDB = true;
        }
    } catch (std::exception& e){
        printf("Caught an exception while getting substitution: %s\n", e.what());
        return false;
    }
    if (subs.size() > 0){
        int size = 0;
        for (auto& s : subs) size += s.GetSpaceForSerialize();
        std::unique_ptr<unsigned char> rawSubs(new unsigned char[size]);
        int mempos = 0;
        for (auto& s : subs) s.SerializeToMem(mempos, rawSubs.get());
        newHash = HashString(rawSubs.get(), size);
    }

    if (latestHash == newHash) return false;

    return true;
}

void Cache::SaveNewSubstitutions(std::vector<std::pair<time_t, std::vector<Edupage::Substitution>>> newSubs, std::map<time_t, std::pair<int,bool>>& hashesCache){
    for (auto& s : newSubs){
        try {
            std::string req = "UPDATE \"SubstitutionCache\" SET \"latestHash\"=" + std::to_string(hashesCache[s.first].first) + " WHERE `date`=\"" + TimeToDate(localtime(&s.first)) + "\"";
            if (!hashesCache[s.first].second) req = "INSERT INTO \"SubstitutionCache\" (\"date\", \"latestHash\") VALUES (\"" + TimeToDate(localtime(&s.first)) + "\", " + std::to_string(hashesCache[s.first].first) + ")";
            SQLite::Statement query(m_db, req);
            int result = query.exec();
            if (result != 1) {
                printf("Updating hash returned %i. (isInDB: %i)\n", result, hashesCache[s.first].second);
                continue;
            }
        } catch (std::exception& e){
            printf("Caught an exception while updating hash: %s\n", e.what());
            continue;
        }
    }
}

Config g_config;

Config::Config(){
    std::ifstream file("config.json");
    if (file.is_open()){
        try {
            nlohmann::json j;
            file >> j;
            //printf("%s\n", string(j).c_str());
            m_timetableURL = j["timetableURL"];
            m_class = j["class"];
            m_TGtoken = j["TGtoken"];
            m_targetChat = j["targetChat"];
            m_schoolSiteURL = j["schoolSiteURL"];
            m_schoolMenuLocation = j["schoolMenuLocation"];
            m_schoolMenuButtonLabel = j["schoolMenuButtonLabel"];
            return;
        }
        catch (std::exception& e){
            printf("Caught exception while loading config file: %s\n", e.what());
        }
    }
    else {
        printf("Couldn't open config.json. Is it in the right place?\n");
    }
    printf("Config loading failed\n");
    exit(1);
}

std::vector<Classmate> g_classmates;

void LoadClassmates(){
    g_classmates.clear();
    std::ifstream file("classmates.json");
    if (file.is_open()){
        try {
            nlohmann::json j;
            file >> j;
            nlohmann::json classmates = j["classmates"];
            for (auto& c : classmates){
                Classmate mate;
                mate.m_name = c["name"];
                memset(&mate.m_birthday, 0, sizeof(mate.m_birthday));
                std::vector<std::string> birthday = Explode(c["birthday"], "-");
                if (birthday.size() == 3){
                    mate.m_birthday.tm_year = atoi(birthday[0].c_str()) - 1900;
                    mate.m_birthday.tm_mon = atoi(birthday[1].c_str()) - 1;
                    mate.m_birthday.tm_mday = atoi(birthday[2].c_str());
                    mktime(&mate.m_birthday);
                }
                else {
                    printf("%s's birthday is %u in size\n", mate.m_name.c_str(), birthday.size());
                }
                mate.m_daysUntilBirthday = DaysUntilBirthday(mate.m_birthday);
                g_classmates.push_back(mate);
            }
            return;
        }
        catch (std::exception& e){
            printf("Caught exception while loading classmates: %s\n", e.what());
        }
    }
    else {
        printf("Couldn't open classmates.json. Is it in the right place?\n");
    }
    printf("Classmates loading failed\n");
}

unsigned short DaysUntilBirthday(tm birthday){
    time_t rawtime = time(NULL);
    tm *time = localtime(&rawtime);
    if (time->tm_yday <= birthday.tm_yday) {
        birthday.tm_year = time->tm_year;
        mktime(&birthday);
        return birthday.tm_yday - time->tm_yday;
    }
    else {
        birthday.tm_year = time->tm_year + 1;
        mktime(&birthday);
        return 365 - time->tm_yday + birthday.tm_yday;
    }
    return 365;
}

TgBot::Bot* g_bot = NULL;

long long GetCurrentTime() {
	using namespace std::chrono;
	return (duration_cast<seconds>(system_clock::now().time_since_epoch())).count();
}

int GetSecondsToMidnight(){
    time_t rawtime = time(NULL);
    tm *time = localtime(&rawtime);
	int seconds = 86400 - ((time->tm_hour * 3600) + (time->tm_min * 60) + (time->tm_sec));
	return seconds;
}

bool IsMonday(){
    time_t rawtime = time(NULL);
    tm *time = localtime(&rawtime);
    return time->tm_wday == 1;
}

std::string GetDate(int dayOffset){
    time_t rawtime = time(NULL);
    tm *time = localtime(&rawtime);
    time->tm_mday += dayOffset;
    mktime(time);
    std::stringstream dateStr;
    dateStr << std::put_time(time, "%Y-%m-%d");
    return dateStr.str();
}

std::string TimeToDate(tm* time){
    std::stringstream dateStr;
    dateStr << std::put_time(time, "%Y-%m-%d");
    return dateStr.str();
}

uint32_t HashString(unsigned char* str, int len)
{
	if (!str) return 0;

	unsigned char* n = (unsigned char*)str;
	uint32_t acc = 0x55555555;

	if (len == 0)
	{
		while (*n)
			acc = (acc >> 27) + (acc << 5) + *n++;
	}
	else
	{
		for (int i = 0; i < len; i++)
		{
			acc = (acc >> 27) + (acc << 5) + *n++;
		}
	}
	return acc;

}

std::vector<std::string> Explode(std::string str, std::string token){
    std::vector<std::string> result;
    while(str.size()){
        int index = str.find(token);
        if(index!=std::string::npos){
            result.push_back(str.substr(0,index));
            str = str.substr(index+token.size());
            if(str.size()==0)result.push_back(str);
        }else{
            result.push_back(str);
            str = "";
        }
    }
    return result;
}