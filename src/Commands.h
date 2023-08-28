#pragma once
#include <tgbot/tgbot.h>

namespace TGCommands{
    void sendMyCommands();
    
    void info(TgBot::Message::Ptr message);

    void substitutions(TgBot::Message::Ptr message);
    void menu(TgBot::Message::Ptr message);
    void list(TgBot::Message::Ptr message);
    void bdays(TgBot::Message::Ptr message);
}