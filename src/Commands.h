#pragma once
#include <tgbot/tgbot.h>

namespace TGCommands{
    void sendMyCommands();
    
    void info(TgBot::Message::Ptr message);

    void substitutions(TgBot::Message::Ptr message);
    void menu(TgBot::Message::Ptr message);
    void list(TgBot::Message::Ptr message);
    void listCallback(TgBot::CallbackQuery::Ptr query);
    void bdays(TgBot::Message::Ptr message);
    void tbdays(TgBot::Message::Ptr message);
    void everyone(TgBot::Message::Ptr message);
    void ping(TgBot::Message::Ptr message);
    void pingCallback(TgBot::CallbackQuery::Ptr query);
    void ssite(TgBot::Message::Ptr message);
    void courses(TgBot::Message::Ptr message);
    void holidays(TgBot::Message::Ptr message);
    void reminders(TgBot::Message::Ptr message);
    void addReminder(TgBot::Message::Ptr message);
    void addReminderCallback(TgBot::CallbackQuery::Ptr query);
    void addReminderToggleCallback(TgBot::CallbackQuery::Ptr query);
    void remReminder(TgBot::Message::Ptr message);
    void remReminderCallback(TgBot::CallbackQuery::Ptr query);
}