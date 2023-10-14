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
            m_studentSite = j["studentSite"];
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

Classmate::Classmate(){
    memset(&m_birthday, 0, sizeof(m_birthday));
}
std::vector<Classmate> g_classmates;

bool Classmate::IsInGroup(unsigned char groupID){
    for (auto& g : m_groups) if (g == groupID) return true;
    return false;
}

void LoadClassmates(){
    g_classmates.clear();
    std::ifstream file("classmates.json");
    if (file.is_open()){
        try {
            nlohmann::json j;
            file >> j;
            nlohmann::json classmates = j["classmates"];
            time_t rawtime = time(NULL);
            tm *time = localtime(&rawtime);
            for (auto& c : classmates){
                Classmate mate;
                mate.m_name = c["name"];
                mate.m_tgID = c["tgID"];
                mate.m_groups = c["groups"].get<std::vector<unsigned char>>();
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
                mate.m_age = time->tm_year - mate.m_birthday.tm_year;
                if (mate.m_birthday.tm_yday > time->tm_yday) mate.m_age--;
                mate.m_bdayAge = time->tm_year - mate.m_birthday.tm_year;
                if (mate.m_birthday.tm_yday < time->tm_yday) mate.m_bdayAge++;
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

std::vector<Group> g_groups;

void LoadGroups(){
    g_groups.clear();
    std::ifstream file("groups.json");
    if (file.is_open()){
        try {
            nlohmann::json j;
            file >> j;
            nlohmann::json groups = j["groups"];
            for (auto& g : groups){
                Group group;
                group.m_id = g["id"];
                group.m_name = g["name"];
                g_groups.push_back(group);
            }
            return;
        }
        catch (std::exception& e){
            printf("Caught exception while loading groups: %s\n", e.what());
        }
    }
    else {
        printf("Couldn't open groups.json. Is it in the right place?\n");
    }
    printf("Groups loading failed\n");
}

Course::Course(){
    memset(&m_end, 0, sizeof(m_end));
}
std::vector<Course> g_courses;

void LoadCourses(){
    g_courses.clear();
    std::ifstream file("courses.json");
    if (file.is_open()){
        try {
            nlohmann::json j;
            file >> j;
            nlohmann::json courses = j["courses"];
            time_t currentTime = time(NULL);
            tm* currentTm = localtime(&currentTime);
            currentTm->tm_hour = 0;
            currentTm->tm_min = 0;
            currentTm->tm_sec = 0;
            currentTime = mktime(currentTm);
            for (auto& c : courses){
                Course course;
                course.m_subjects = c["subjects"].get<std::vector<std::string>>();
                if (course.m_subjects.size() < 1) continue;
                std::vector<std::string> periods = c["ends"].get<std::vector<std::string>>();
                bool firstPeriod = true;
                for (auto& p : periods){
                    tm end;
                    memset(&end, 0, sizeof(end));
                    std::vector<std::string> endVec = Explode(p, "-");
                    if (endVec.size() == 3){
                        end.tm_year = atoi(endVec[0].c_str()) - 1900;
                        end.tm_mon = atoi(endVec[1].c_str()) - 1;
                        end.tm_mday = atoi(endVec[2].c_str());
                    }
                    else {
                        printf("Some of the courses' end date is %u in size (should be 3)\n", endVec.size());
                        continue;
                    }
                    if (firstPeriod){
                        firstPeriod = false;
                        course.m_end = end;
                        course.m_daysToEnd = round(difftime(mktime(&end), currentTime) / 86400);
                    }
                    else {
                        short daysToEnd = round(difftime(mktime(&end), currentTime) / 86400);
                        if (course.m_daysToEnd > daysToEnd || (course.m_daysToEnd < 0 && daysToEnd >= 0)){
                            course.m_end = end;
                            course.m_daysToEnd = daysToEnd;
                        }
                    }
                }
                if (course.m_daysToEnd >= 0) g_courses.push_back(course);
            }
            std::sort(g_courses.begin(), g_courses.end());
            return;
        }
        catch (std::exception& e){
            printf("Caught exception while loading courses: %s\n", e.what());
        }
    }
    else {
        printf("Couldn't open courses.json. Is it in the right place?\n");
    }
    printf("Courses loading failed\n");
}

Holiday::Holiday(){
    memset(&m_start, 0, sizeof(m_start));
    memset(&m_end, 0, sizeof(m_end));
}
std::vector<Holiday> g_holidays;

void LoadHolidays(){
    g_holidays.clear();
    std::ifstream file("holidays.json");
    if (file.is_open()){
        try {
            nlohmann::json j;
            file >> j;
            nlohmann::json holidays = j["holidays"];
            time_t currentTime = time(NULL);
            tm* currentTm = localtime(&currentTime);
            currentTm->tm_hour = 0;
            currentTm->tm_min = 0;
            currentTm->tm_sec = 0;
            currentTime = mktime(currentTm);
            for (auto& h : holidays){
                Holiday holiday;
                std::vector<std::string> start = Explode(h["start"], "-");
                if (start.size() == 3){
                    holiday.m_start.tm_year = atoi(start[0].c_str()) - 1900;
                    holiday.m_start.tm_mon = atoi(start[1].c_str()) - 1;
                    holiday.m_start.tm_mday = atoi(start[2].c_str());
                    time_t startTime = mktime(&holiday.m_start);
                    holiday.m_daysTo = round(difftime(startTime, currentTime) / 86400);
                    if (holiday.m_daysTo <= 0){
                        holiday.m_daysTo = 0;
                        holiday.m_bStarted = true;
                    }
                }
                else {
                    printf("Some of the holidays' start is %u in size\n", start.size());
                    continue;
                }
                std::vector<std::string> end = Explode(h["end"], "-");
                if (end.size() == 3){
                    holiday.m_end.tm_year = atoi(end[0].c_str()) - 1900;
                    holiday.m_end.tm_mon = atoi(end[1].c_str()) - 1;
                    holiday.m_end.tm_mday = atoi(end[2].c_str());
                    time_t endTime = mktime(&holiday.m_end);
                    if (holiday.m_bStarted){
                        holiday.m_daysTo = round(difftime(endTime, currentTime) / 86400);
                    }
                }
                else {
                    printf("Some of the holidays' end is %u in size\n", end.size());
                    continue;
                }

                if (holiday.m_daysTo >= 0) g_holidays.push_back(holiday);
            }
            return;
        }
        catch (std::exception& e){
            printf("Caught exception while loading holidays: %s\n", e.what());
        }
    }
    else {
        printf("Couldn't open holidays.json. Is it in the right place?\n");
    }
    printf("Holidays loading failed\n");
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

std::string TimeToDate(tm* time, bool addDayName){
    std::stringstream dateStr;
    dateStr << std::put_time(time, "%Y-%m-%d");
    if (addDayName){
        std::string result = dateStr.str() + " (";
        switch (time->tm_wday){
            case 1:
                result += "Понедельник";
            break;
            case 2:
                result += "Вторник";
            break;
            case 3:
                result += "Среда";
            break;
            case 4:
                result += "Четверг";
            break;
            case 5:
                result += "Пятница";
            break;
            case 6:
                result += "Суббота";
            break;
            case 0:
                result += "Воскресенье";
            break;
            default:
                result += "Что-то";
            break;
        }
        return result + ")";
    }
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