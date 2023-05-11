#pragma once
#include <tgbot/tgbot.h>

namespace TGCommands{
    void sendMyCommands();
    
    void info(TgBot::Message::Ptr message);

    void substitutions(TgBot::Message::Ptr message);
}