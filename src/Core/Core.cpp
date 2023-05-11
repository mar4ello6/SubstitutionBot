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

TgBot::Bot* g_bot = NULL;

long long GetCurrentTime()
{
	using namespace std::chrono;
	return (duration_cast<seconds>(system_clock::now().time_since_epoch())).count();
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