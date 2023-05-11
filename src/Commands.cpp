#include "Commands.h"
#include "Core/Core.h"
#include <vector>

void TGCommands::sendMyCommands(){
    std::vector<TgBot::BotCommand::Ptr> commands;

    TgBot::BotCommand::Ptr cmdArray(new TgBot::BotCommand);
    cmdArray->command = "info";
    cmdArray->description = "Посмотреть информацию про бота.";
    commands.push_back(cmdArray);

    cmdArray = TgBot::BotCommand::Ptr(new TgBot::BotCommand);
    cmdArray->command = "subs";
    cmdArray->description = "Посмотреть замены на неделю.";
    commands.push_back(cmdArray);

    g_bot->getApi().setMyCommands(commands);
}

void TGCommands::info(TgBot::Message::Ptr message){
    std::string messageStr = "Создатель бота: @mar4e_M\n"
                             "Этот бот с открытым исходным кодом! (<a href=\"https://github.com/mar4ello6/SubstitutionBot\">Репозиторий</a>)\n"
                             "В данный момент этот бот настроен для " + g_config.m_class + " класса.\n";

    try {
        g_bot->getApi().sendMessage(message->chat->id, messageStr, false, 0, nullptr, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending info (command): %s\n", e.what());
    }
}

void TGCommands::substitutions(TgBot::Message::Ptr message){
    std::string messageStr = "";

    std::vector<std::pair<time_t, std::vector<Edupage::Substitution>>> subs = g_subChecker.getSubsForAWeek();

    if (subs.size() < 1) messageStr = "Замен на ближайшую неделю нет!";
    else {
        messageStr = "Замены на ближайшую неделю:\n\n";
        for (auto& n : subs){
            if (n.second.size() < 1) continue; //that's strange, it shouldn't really be here

            messageStr += TimeToDate(localtime(&n.first)) + ":\n";
            for (auto& s : n.second) messageStr += s.GetString() + "\n";
            messageStr += "\n";
        }
    }

    try {
        g_bot->getApi().sendMessage(message->chat->id, messageStr, false, 0, nullptr, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending substitutions (command): %s\n", e.what());
    }
}