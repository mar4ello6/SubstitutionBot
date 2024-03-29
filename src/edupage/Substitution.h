#pragma once
#include <string>
#include <vector>

namespace Edupage{
    struct Substitution{
        enum Type{
            CHANGE,
            REMOVE,
            ABSENT,
            ADDL
        } type;
        std::pair<short, short> period;
        std::string subject;
        std::string newSubject; //sometimes subject is changed too...
        std::string oTeacher; //original teacher who's being replaced
        std::string sTeacher; //teacher who will replace
        std::string note; //some times there are some notes and info
        short movedFrom = -999; //for ADDL, when moved from some period

        int GetSpaceForSerialize();
        /**
         * This is not made for use after! It's only to get the hash of this struct!
        */
        void SerializeToMem(int& mempos, unsigned char* data);

        /**
         * Make a string with info about substitution. (Uses some HTML markdown)
        */
        std::string GetString();
    };

    /**
     * Get substitutions for some class. You need to specify class, but not URL. It takes URL from g_config.
     * 
     * @param className Class whose substitutions we need
     * @param time Day to get substitutions from
     * @throw 1 - when function fails
    */
    std::vector<Edupage::Substitution> GetSubstitutions(std::string className, tm * time);
}