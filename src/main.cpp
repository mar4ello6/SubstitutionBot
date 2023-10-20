#include <stdio.h>
#include "Core/Core.h"
#include "Checkers/SubstitutionChecker.h"
#include "Checkers/BirthdayChecker.h"
#include "Checkers/CoursesChecker.h"
#include "Checkers/RemindChecker.h"
#include "Commands.h"

int main(){
    //loading classmates, their birthdays and groups
    LoadClassmates();
    LoadGroups();
    LoadCourses();

    g_bot = new TgBot::Bot(g_config.m_TGtoken);

    //run some checks that all settings are valid...
    try {
        printf("TG Bot's username is %s\n", g_bot->getApi().getMe()->username.c_str());
    } catch (std::exception& e){
        printf("Caught exception while getting bot's username: %s; make sure your token is right\n", e.what());
        exit(1);
    }
    //check checker

    TGCommands::sendMyCommands();

    //setting callbacks
    g_bot->getEvents().onCommand("info", TGCommands::info);
    g_bot->getEvents().onCommand("subs", TGCommands::substitutions);
    g_bot->getEvents().onCommand("menu", TGCommands::menu);
    g_bot->getEvents().onCommand("list", TGCommands::list);
    g_bot->getEvents().onCommand("bdays", TGCommands::bdays);
    g_bot->getEvents().onCommand("everyone", TGCommands::everyone);
    g_bot->getEvents().onCommand("ping", TGCommands::ping);
    g_bot->getEvents().onCallbackQuery(TGCommands::pingCallback);
    g_bot->getEvents().onCommand("ssite", TGCommands::ssite);
    g_bot->getEvents().onCommand("courses", TGCommands::courses);
    g_bot->getEvents().onCommand("holidays", TGCommands::holidays);
    g_bot->getEvents().onCommand("reminders", TGCommands::reminders);
    g_bot->getEvents().onCommand("addreminder", TGCommands::addReminder);
    g_bot->getEvents().onCallbackQuery(TGCommands::addReminderCallback);
    g_bot->getEvents().onCallbackQuery(TGCommands::addReminderToggleCallback);
    g_bot->getEvents().onCommand("remreminder", TGCommands::remReminder);
    g_bot->getEvents().onCallbackQuery(TGCommands::remReminderCallback);

    printf("Starting event poll...\n");
    TgBot::TgLongPoll tgPoll(*g_bot);
    while (true){
        g_subChecker.OnUpdate();
        g_BDayChecker.OnUpdate();
        g_coursesChecker.OnUpdate();
        g_remindChecker.OnUpdate();

        //this timeouts in around 10 seconds, looks good enough...
        try {
            tgPoll.start();
        }
        catch (std::exception& e){
            printf("TG poll failed: %s\n", e.what());
        }
    }

    delete g_bot;
    return 0;
}