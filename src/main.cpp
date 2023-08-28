#include <stdio.h>
#include "Core/Core.h"
#include "Checkers/SubstitutionChecker.h"
#include "Checkers/BirthdayChecker.h"
#include "Commands.h"

int main(){
    //loading classmates and their birthdays
    LoadClassmates();

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

    printf("Starting event poll...\n");
    TgBot::TgLongPoll tgPoll(*g_bot);
    while (true){
        g_subChecker.OnUpdate();
        g_BDayChecker.OnUpdate();

        //this timeouts in around 10 seconds, looks good enough...
        tgPoll.start();
    }

    delete g_bot;
    return 0;
}