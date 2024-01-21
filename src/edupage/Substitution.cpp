#include "Substitution.h"
#include "Core/Core.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <gumbo.h>

int Edupage::Substitution::GetSpaceForSerialize(){
    return 7 + subject.size() + newSubject.size() + oTeacher.size() + sTeacher.size() + note.size();
}
void Edupage::Substitution::SerializeToMem(int& mempos, unsigned char* data){
    data[mempos] = type;
    mempos++;
    *(short*)(data + mempos) = period.first;
    mempos += 2;
    *(short*)(data + mempos) = period.second;
    mempos += 2;
    *(short*)(data + mempos) = movedFrom;
    mempos += 2;
    memcpy(data + mempos, subject.c_str(), subject.size());
    mempos += subject.size();
    memcpy(data + mempos, newSubject.c_str(), newSubject.size());
    mempos += newSubject.size();
    memcpy(data + mempos, oTeacher.c_str(), oTeacher.size());
    mempos += oTeacher.size();
    memcpy(data + mempos, sTeacher.c_str(), sTeacher.size());
    mempos += sTeacher.size();
    memcpy(data + mempos, note.c_str(), note.size());
    mempos += note.size();
}

std::string Edupage::Substitution::GetString(){
    std::string result = "";
    if (period.first == period.second) result += "#" + std::to_string(period.first);
    else result += "#" + std::to_string(period.first) + " - #" + std::to_string(period.second);
    if (type != Edupage::Substitution::Type::ABSENT) {
        if (newSubject.empty()) result += " (" + subject + "): ";
        else result += " (<s>" + subject + "</s>  ➡  " + newSubject + "): ";
    }
    switch (type){
        case Type::CHANGE:
            if (!oTeacher.empty() && !sTeacher.empty()){
                result += "<s>" + oTeacher + "</s>  ➡  " + sTeacher;
            }
            else result = result.substr(0, result.length() - 1);
        break;
        case Type::REMOVE:
            result += "🚫  Отменено!";
        break;
        case Type::ABSENT:
            result += ": Уходим!";
        break;
        case Type::ADDL:
            if (movedFrom != -999){
                result += "Вместо #" + std::to_string(movedFrom) + ", ";
            }
            result += sTeacher;
        break;
    }
    if (!note.empty()){
        result += " <i>(" + note + ")</i>";
    }
    return result;
}


//Things to work with webpage
GumboNode* GetClassesInNode(GumboNode* node){
    if (node->type != GUMBO_NODE_ELEMENT) return NULL;
    
    if (gumbo_get_attribute(&node->v.element.attributes, "data-date")) return node;

    GumboVector* children = &node->v.element.children;
    for (int i = 0; i < children->length; ++i){
        GumboNode* res = GetClassesInNode((GumboNode*)children->data[i]);
        if (res != NULL) return res;
    }

    return NULL;
}
bool IsThisClassNode(GumboNode* node, std::string className){
    if (node->type != GUMBO_NODE_ELEMENT) return false; //it's not the node we need
    
    GumboAttribute* attribute = gumbo_get_attribute(&node->v.element.attributes, "class");
    if (!attribute) return false;
    if (strcmp(attribute->value, "section print-nobreak") != 0) return false; //it's not class node

    GumboVector* children = &node->v.element.children;
    for (int i = 0; i < children->length; ++i){
        GumboNode* child = (GumboNode*)children->data[i];
        if (child->type != GUMBO_NODE_ELEMENT) continue;

        //checking if this is header
        GumboAttribute* chAttr = gumbo_get_attribute(&child->v.element.attributes, "class");
        if (!chAttr) continue;
        if (strcmp(chAttr->value, "header") != 0) continue;
        
        if (child->v.element.children.length < 1) continue; //we need to get the first child from here...
        GumboNode* classNode = (GumboNode*)child->v.element.children.data[0];
        if (classNode->v.element.children.length < 1) continue; //we need to get the first child again
        GumboNode* classNameNode = (GumboNode*)classNode->v.element.children.data[0];
        if (classNameNode->type != GUMBO_NODE_TEXT) continue; //something is wrong here

        return (strcmp(classNameNode->v.text.text, className.c_str()) == 0);
    }

    return false;
}
std::vector<Edupage::Substitution> GetSubstitutionsFromNode(GumboNode* node){
    std::vector<Edupage::Substitution> result;

    if (node->type != GUMBO_NODE_ELEMENT) return result;
    
    GumboAttribute* attribute = gumbo_get_attribute(&node->v.element.attributes, "class");
    if (!attribute) return result;
    if (strcmp(attribute->value, "section print-nobreak") != 0) return result; //it's not class node

    GumboVector* children = &node->v.element.children;
    for (int i = 0; i < children->length; ++i){
        GumboNode* rows = (GumboNode*)children->data[i];
        if (rows->type != GUMBO_NODE_ELEMENT) continue;

        //checking if this is rows (changes)
        GumboAttribute* chAttr = gumbo_get_attribute(&rows->v.element.attributes, "class");
        if (!chAttr) continue;
        if (strcmp(chAttr->value, "rows") != 0) continue;
        
        if (rows->v.element.children.length < 1) continue; //no changes?
        GumboVector* rowsVector = &rows->v.element.children;

        for (int r = 0; r < rowsVector->length; ++r) { //going through all rows
            GumboNode* row = (GumboNode*)rowsVector->data[r];
            if (row->type != GUMBO_NODE_ELEMENT) continue;
            
            GumboAttribute* rowAttr = gumbo_get_attribute(&row->v.element.attributes, "class");
            if (!rowAttr) continue;
            Edupage::Substitution sub;
            if (strcmp(rowAttr->value, "row change") == 0) { //teacher changed
                sub.type = Edupage::Substitution::CHANGE;
            }
            else if (strcmp(rowAttr->value, "row remove") == 0) { //lesson cancelled
                sub.type = Edupage::Substitution::REMOVE;
            }
            else if (strcmp(rowAttr->value, "row absent") == 0) { //class is gone
                sub.type = Edupage::Substitution::ABSENT;
            }
            else if (strcmp(rowAttr->value, "row add") == 0) { //lesson added
                sub.type = Edupage::Substitution::ADDL;
            }
            else continue;

            if (rows->v.element.children.length < 1) continue; //no info, not inserting it into vector

            GumboVector* infoVector = &row->v.element.children;
            for (int ri = 0; ri < infoVector->length; ++ri){
                GumboNode* info = (GumboNode*)infoVector->data[ri];
                if (info->type != GUMBO_NODE_ELEMENT) continue;

                GumboAttribute* infoAttr = gumbo_get_attribute(&info->v.element.attributes, "class");
                if (!infoAttr) continue;

                std::string valueStr = "";
                if (info->v.element.children.length < 1) continue;
                GumboNode* valueNode = (GumboNode*)info->v.element.children.data[0];
                if (valueNode->type != GUMBO_NODE_ELEMENT) continue;
                if (valueNode->v.element.children.length < 1) continue;
                if (((GumboNode*)(valueNode->v.element.children.data[0]))->type != GUMBO_NODE_TEXT) continue;
                valueStr = ((GumboNode*)(valueNode->v.element.children.data[0]))->v.text.text;

                if (strcmp(infoAttr->value, "period") == 0) {
                    if (valueStr.find('(') != std::string::npos) valueStr = valueStr.substr(1, valueStr.length() - 2); //getting rid of 
                    if (valueStr.find('-') != std::string::npos) { //it's period... more than 1 lesson
                        std::string firstNum = valueStr.substr(0, valueStr.find('.')); //found first num
                        std::size_t secondNumPos = valueStr.find('-') + 2;
                        std::string secondNum = valueStr.substr(secondNumPos, valueStr.length() - secondNumPos - 1);

                        sub.period.first = atoi(firstNum.c_str());
                        sub.period.second = atoi(secondNum.c_str());
                    }
                    else { //just 1 lesson replaced
                        valueStr = valueStr.substr(0, valueStr.length() - 1); //getting rid of a dot
                        sub.period.first = atoi(valueStr.c_str());
                        sub.period.second = sub.period.first;
                    }
                }
                if (strcmp(infoAttr->value, "info") == 0 && sub.type != Edupage::Substitution::ABSENT) { //when class is absent, there's nothing we would need
                    std::size_t subjectDelim = valueStr.find('-');
                    sub.subject = valueStr.substr(0, subjectDelim - 1);
                    std::size_t newSubjectDelim = sub.subject.find("➔");
                    if (newSubjectDelim != std::string::npos){ //subject is changed
                        sub.newSubject = sub.subject.substr(newSubjectDelim + 4);
                        sub.subject = sub.subject.substr(1, newSubjectDelim - 3); //we need to remove brackets too
                    }
                    valueStr = valueStr.substr(subjectDelim);

                    if (sub.type == Edupage::Substitution::CHANGE){
                        std::size_t teachersSeparator = valueStr.find("➔"); //This is 3 symbols!!!
                        std::size_t oTeacherStart = std::string::npos;

                        std::size_t infoSeparator = valueStr.find(',', (teachersSeparator == std::string::npos ? 0 : teachersSeparator)); //probably it has some info
                        if (infoSeparator != std::string::npos){
                            sub.note = valueStr.substr(infoSeparator + 2);
                        }
                        else infoSeparator = 0;

                        if (teachersSeparator != std::string::npos || infoSeparator == 0){ //else it's not really a change, probably just some info
                            oTeacherStart = valueStr.find('(') + 1;
                            sub.oTeacher = valueStr.substr(oTeacherStart, valueStr.find(')') - oTeacherStart);
                        }
                        if (oTeacherStart != std::string::npos){
                            if (teachersSeparator != std::string::npos){
                                std::size_t sTeacherLen = valueStr.length() - teachersSeparator - 4;
                                if (infoSeparator > 0){
                                    sTeacherLen -= sub.note.length() + 2;
                                }
                                sub.sTeacher = valueStr.substr(teachersSeparator + 4, sTeacherLen);
                            }
                            else sub.sTeacher = "???"; //Another teacher is not announced yet...
                        }
                    }
                    if (sub.type == Edupage::Substitution::REMOVE){
                        std::size_t oTeacherStart = valueStr.find('-') + 2;
                        std::size_t oTeacherEnd = valueStr.find(',');
                        if (oTeacherEnd != std::string::npos){
                            sub.oTeacher = valueStr.substr(oTeacherStart, oTeacherEnd - oTeacherStart);
                            std::size_t noteStart = valueStr.find(',', oTeacherEnd + 1);
                            if (noteStart == std::string::npos && valueStr.find(' ', oTeacherEnd + 2) != std::string::npos){ //probably there's just a note there
                                sub.note = valueStr.substr(oTeacherEnd + 2);
                            }
                            else {
                                if (sub.oTeacher.find(' ') == std::string::npos){ //not a teacher, teachers usually have space in their names ('Cancelled' text)
                                    sub.oTeacher.clear();
                                    noteStart = oTeacherEnd;
                                }
                                if (noteStart != std::string::npos){
                                    sub.note = valueStr.substr(noteStart + 2);
                                }
                            }
                        }
                    }
                    if (sub.type == Edupage::Substitution::ADDL){
                        std::size_t movedFromPos = valueStr.find("Перемещено из периода: "); //hard coding this... maybe i will change this later
                        if (movedFromPos != std::string::npos){
                            sub.movedFrom = atoi(valueStr.substr(movedFromPos + 42, valueStr.find('.', movedFromPos) - movedFromPos).c_str());
                        }
                        std::size_t teacherPos = valueStr.find("Учитель: "); //hard coding this... maybe i will change this later
                        std::size_t notePos = valueStr.find(',', teacherPos);
                        if (notePos != std::string::npos){
                            sub.sTeacher = valueStr.substr(teacherPos + 16, notePos - teacherPos - 16);
                            sub.note = valueStr.substr(notePos + 2);
                        }
                        else sub.sTeacher = valueStr.substr(teacherPos + 16);
                    }
                }
            }
            result.push_back(sub);
        }

        return result;
    }

    return result;
}

httplib::Client httpCli("https://" + g_config.m_timetableURL);

std::vector<Edupage::Substitution> Edupage::GetSubstitutions(std::string className, tm * time){
    std::vector<Edupage::Substitution> subs;

    nlohmann::json options;

    options["date"] = TimeToDate(time);
    options["mode"] = "classes";

    //making JSON params needed for api...
    nlohmann::json params;
    nlohmann::json args;
    args[0] = NULL;
    args[1] = options;
    params["__args"] = args;
    params["__gsh"] = "00000000";

    auto result = httpCli.Post("/substitution/server/viewer.js?__func=getSubstViewerDayDataHtml", params.dump(), "application/json");
    if (!result) {
        printf("Failed to get substitution page (HTTP error: %i)\n", result.error());
        throw 1;
    }
    if (result->status != 200){
        printf("Failed to get substitution page (HTTP status: %i)\n", result->status);
        throw 1;
    }

    std::string raw;
    try {
        raw = nlohmann::json::parse(result->body)["r"];
    } catch (std::exception& e) {
        printf("Failed to get substitution (%s)\n", e.what());
        throw 1;
    }
    if (raw.find(g_config.m_class) == std::string::npos) { //there's no target class here
        return subs;
    }

    GumboOutput* output = gumbo_parse(raw.c_str());
    GumboNode* classesRaw = GetClassesInNode(output->root);
    if (!classesRaw) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
        throw 1;
    }

    GumboVector* classes = &classesRaw->v.element.children;
    for (int i = 0; i < classes->length; i++){
        if (!IsThisClassNode((GumboNode*)classes->data[i], g_config.m_class)) continue;
        subs = GetSubstitutionsFromNode((GumboNode*)classes->data[i]);
        break;
    }

    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return subs;
}