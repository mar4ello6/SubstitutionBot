#include "Commands.h"
#include "Core/Core.h"
#include <vector>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
#include <gumbo.h>

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

    cmdArray = TgBot::BotCommand::Ptr(new TgBot::BotCommand);
    cmdArray->command = "menu";
    cmdArray->description = "Достать ссылку на меню в столовой.";
    commands.push_back(cmdArray);

    g_bot->getApi().setMyCommands(commands);
}

void TGCommands::info(TgBot::Message::Ptr message){
    std::string messageStr = "Создатель бота: @mar4e_M\n"
                             "Этот бот с открытым исходным кодом! (<a href=\"https://github.com/mar4ello6/SubstitutionBot\">Репозиторий</a>)\n"
                             "В данный момент этот бот настроен для <b>" + g_config.m_class + "</b> класса.\n";

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


GumboNode* GetMenuLinkNode(GumboNode* node){
    if (node->type == GUMBO_NODE_ELEMENT){ //we're trying to find all the text nodes and check if it's menu button...
        GumboVector* children = &node->v.element.children;
        for (int i = 0; i < children->length; ++i){
            GumboNode* res = GetMenuLinkNode((GumboNode*)children->data[i]);
            if (res != NULL) return res;
        }
    }
    if (node->type == GUMBO_NODE_TEXT){
        if (strcmp(node->v.text.text, g_config.m_schoolMenuButtonLabel.c_str()) == 0) return node->parent;
    }
    return NULL;
}
void sendMenuFailed(TgBot::Message::Ptr message){
    g_bot->getApi().sendMessage(message->chat->id, "Не получилось достать ссылку на меню.", false, 0, nullptr, "HTML");
}
void TGCommands::menu(TgBot::Message::Ptr message){
    httplib::Client cli(g_config.m_schoolSiteURL);

    auto result = cli.Get(g_config.m_schoolMenuLocation);
    if (!result) {
        printf("Failed to get menu page (HTTP error: %i)\n", result.error());
        sendMenuFailed(message);
        return;
    }
    if (result->status != 200){
        printf("Failed to get menu page (HTTP status: %i)\n", result->status);
        sendMenuFailed(message);
        return;
    }

    GumboOutput* output = gumbo_parse(result->body.c_str());
    GumboNode* menuNode = GetMenuLinkNode(output->root);
    if (!menuNode){
        sendMenuFailed(message);
        gumbo_destroy_output(&kGumboDefaultOptions, output);
        return;
    }

    GumboAttribute* attribute = gumbo_get_attribute(&menuNode->v.element.attributes, "href");
    if (!attribute){
        sendMenuFailed(message);
        gumbo_destroy_output(&kGumboDefaultOptions, output);
        return;
    }
    g_bot->getApi().sendMessage(message->chat->id, string_format("<a href=\"%s\">Меню на неделю</a>", attribute->value), false, 0, nullptr, "HTML");

    gumbo_destroy_output(&kGumboDefaultOptions, output);
}