#include "Commands.h"
#include "Core/Core.h"
#include "Checkers/RemindChecker.h"
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

    cmdArray = TgBot::BotCommand::Ptr(new TgBot::BotCommand);
    cmdArray->command = "list";
    cmdArray->description = "Посмотреть список класса или группы.";
    commands.push_back(cmdArray);

    cmdArray = TgBot::BotCommand::Ptr(new TgBot::BotCommand);
    cmdArray->command = "bdays";
    cmdArray->description = "Посмотреть дни рождения класса.";
    commands.push_back(cmdArray);

    cmdArray = TgBot::BotCommand::Ptr(new TgBot::BotCommand);
    cmdArray->command = "everyone";
    cmdArray->description = "Пингануть всех.";
    commands.push_back(cmdArray);

    cmdArray = TgBot::BotCommand::Ptr(new TgBot::BotCommand);
    cmdArray->command = "ping";
    cmdArray->description = "Пингануть группу.";
    commands.push_back(cmdArray);

    cmdArray = TgBot::BotCommand::Ptr(new TgBot::BotCommand);
    cmdArray->command = "ssite";
    cmdArray->description = "Получить ссылку на сайт ученика.";
    commands.push_back(cmdArray);

    cmdArray = TgBot::BotCommand::Ptr(new TgBot::BotCommand);
    cmdArray->command = "courses";
    cmdArray->description = "Посмотреть через сколько заканчиваются курсы.";
    commands.push_back(cmdArray);

    cmdArray = TgBot::BotCommand::Ptr(new TgBot::BotCommand);
    cmdArray->command = "holidays";
    cmdArray->description = "Посмотреть когда каникулы.";
    commands.push_back(cmdArray);

    cmdArray = TgBot::BotCommand::Ptr(new TgBot::BotCommand);
    cmdArray->command = "reminders";
    cmdArray->description = "Посмотреть текущие напоминалки.";
    commands.push_back(cmdArray);

    cmdArray = TgBot::BotCommand::Ptr(new TgBot::BotCommand);
    cmdArray->command = "addreminder";
    cmdArray->description = "Добавить новую напоминалку.";
    commands.push_back(cmdArray);

    cmdArray = TgBot::BotCommand::Ptr(new TgBot::BotCommand);
    cmdArray->command = "remreminder";
    cmdArray->description = "Удалить напоминалку.";
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

            messageStr += TimeToDate(localtime(&n.first), true) + ":\n";
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
        if (strcmp(node->v.text.text, g_config.m_schoolMenuButtonLabel.c_str()) == 0) {
            if (!gumbo_get_attribute(&node->parent->v.element.attributes, "href")) return NULL; //it should have some link...
            return node->parent;
        }
    }
    return NULL;
}
void sendMenuFailed(TgBot::Message::Ptr message){
    g_bot->getApi().sendMessage(message->chat->id, "Не получилось достать ссылку на меню.", false, 0, nullptr, "HTML");
}
void TGCommands::menu(TgBot::Message::Ptr message){
    std::string resultLink = "";
    if (g_config.m_schoolMenuLocation.empty() && g_config.m_schoolMenuButtonLabel.empty()) { //means we should just send this link
        resultLink = g_config.m_schoolSiteURL;
    }
    else {
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
        resultLink = attribute->value;
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    try {
        g_bot->getApi().sendMessage(message->chat->id, string_format("<a href=\"%s\">Меню на неделю</a>", resultLink.c_str()), false, 0, nullptr, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending menu: %s\n", e.what());
    }
}

void TGCommands::list(TgBot::Message::Ptr message){
    if (message->chat->id != g_config.m_targetChat) { //other people can (i think) dm our bot and get private info about our class... I don't want that
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Этой коммандой можно пользоваться только в чате класса!", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }
    if (g_classmates.size() < 1) {
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Список класса не загружен, попробуйте позже.", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }

    TgBot::InlineKeyboardMarkup::Ptr inlineKeyboard(new TgBot::InlineKeyboardMarkup);
    std::vector<TgBot::InlineKeyboardButton::Ptr> buttonsRow;
    TgBot::InlineKeyboardButton::Ptr inlineButton;

    inlineButton = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
    inlineButton->text = "Все";
    inlineButton->callbackData = "listAll";
    buttonsRow.push_back(inlineButton);
    inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
    buttonsRow.clear();
    for (auto& g : g_groups){
        inlineButton = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
        inlineButton->text = g.m_name;
        inlineButton->callbackData = "list" + std::to_string(g.m_id);
        buttonsRow.push_back(inlineButton);

        if (buttonsRow.size() >= 2){
            inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
            buttonsRow.clear();
        }
    }
    if (buttonsRow.size() > 0){
        inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
        buttonsRow.clear();
    }

    try {
        g_bot->getApi().sendMessage(message->chat->id, "Выберите список группы.", false, 0, inlineKeyboard, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending list command: %s\n", e.what());
    }
}

void TGCommands::listCallback(TgBot::CallbackQuery::Ptr query){
    if (query->data.substr(0, 4) != "list") return;
    if (query->message->chat->id != g_config.m_targetChat) { //other people can (i think) dm our bot and get private info about our class... I don't want that
        try {
            g_bot->getApi().answerCallbackQuery(query->id, "Этой коммандой можно пользоваться только в чате класса!", true);
        } catch (...) {}
        return;
    }
    if (g_classmates.size() < 1) {
        try {
            g_bot->getApi().answerCallbackQuery(query->id, "Список класса не загружен, попробуйте позже.", true);
        } catch (...) {}
        return;
    }

    if (query->data.substr(4) == "All"){
        std::string messageStr = "Список " + g_config.m_class + ":\n";
        uint32_t personNum = 0;
        for (auto& c : g_classmates){
            messageStr += std::to_string(++personNum) + ". " + c.m_name + "\n";
        }

        try {
            g_bot->getApi().editMessageText(messageStr.substr(0, messageStr.length() - 1), query->message->chat->id, query->message->messageId, "", "HTML");
        } catch (std::exception& e) { 
            printf("Caught exception while sending whole class list: %s\n", e.what());
        }
        return;
    }

    unsigned char gId = 0;
    try {
        gId = atoi(query->data.substr(4).c_str());
    } catch (...) { return; }
    Group group;
    for (auto& g : g_groups) {
        if (g.m_id == gId) {
            group = g;
            break;
        }
    }
    if (group.m_name.empty()) return;

    std::string messageStr = "Список " + group.m_name + ":\n";
    uint32_t personNum = 0;
    for (auto& c : g_classmates){
        if (!c.IsInGroup(group.m_id)) continue;
        messageStr += std::to_string(++personNum) + ". " + c.m_name + "\n";
    }

    try {
        g_bot->getApi().editMessageText(messageStr.substr(0, messageStr.length() - 1), query->message->chat->id, query->message->messageId, "", "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending group #%u's list: %s\n", gId, e.what());
    }
}

void TGCommands::bdays(TgBot::Message::Ptr message){
    if (message->chat->id != g_config.m_targetChat) { //other people can (i think) dm our bot and get private info about our class... I don't want that
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Этой коммандой можно пользоваться только в чате класса!", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }
    if (g_classmates.size() < 1) {
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Список класса не загружен, попробуйте позже.", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }

    std::string messageStr = "Список дней рождений " + g_config.m_class + " класса:\n";
    std::vector<Classmate> classmates = g_classmates;
    std::sort(classmates.begin(), classmates.end());
    for (auto& c : classmates){
        messageStr += string_format("- %s (%02i.%02i.%04i: <i>%i</i>, <b>", c.m_name.c_str(), c.m_birthday.tm_mday, c.m_birthday.tm_mon + 1, c.m_birthday.tm_year + 1900, c.m_age);
        if (c.m_daysUntilBirthday == 0) messageStr += "СЕГОДНЯ! 🎂";
        else if (c.m_daysUntilBirthday == 1) messageStr += "завтра";
        else {
            unsigned short daysLeft = c.m_daysUntilBirthday;
            messageStr += "через " + std::to_string(daysLeft) + " ";
            daysLeft %= 100;
            if (daysLeft / 10 != 1) daysLeft %= 10;
            if (daysLeft == 1) messageStr += "день";
            else if (daysLeft >= 2 && daysLeft <= 4) messageStr += "дня";
            else messageStr += "дней";
        }
        if (c.m_daysUntilBirthday >= 1){
            messageStr += string_format(" %i-летие", c.m_bdayAge);
        }
        messageStr += "</b>)\n";
    }

    try {
        g_bot->getApi().sendMessage(message->chat->id, messageStr.substr(0, messageStr.length() - 1), false, 0, nullptr, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending birthdays: %s\n", e.what());
    }
}

void TGCommands::everyone(TgBot::Message::Ptr message){
    if (message->chat->id != g_config.m_targetChat) { //other people can (i think) dm our bot and get private info about our class... I don't want that
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Этой коммандой можно пользоваться только в чате класса!", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }
    if (g_classmates.size() < 1) {
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Список класса не загружен, попробуйте позже.", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }

    std::string msg = "Пинг\\!";
    for (auto& c : g_classmates){
        if (!c.m_tgID) continue;
        msg += "[\u2009](tg://user?id=" + std::to_string(c.m_tgID) + ")";
    }

    try {
        g_bot->getApi().sendMessage(message->chat->id, msg, false, 0, nullptr, "MarkdownV2");
    } catch (std::exception& e) { 
        printf("Caught exception while sending everyone: %s\n", e.what());
    }
}

void TGCommands::ping(TgBot::Message::Ptr message){
    if (message->chat->id != g_config.m_targetChat) { //other people can (i think) dm our bot and get private info about our class... I don't want that
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Этой коммандой можно пользоваться только в чате класса!", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }
    if (g_classmates.size() < 1) {
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Список класса не загружен, попробуйте позже.", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }

    TgBot::InlineKeyboardMarkup::Ptr inlineKeyboard(new TgBot::InlineKeyboardMarkup);
    std::vector<TgBot::InlineKeyboardButton::Ptr> buttonsRow;
    TgBot::InlineKeyboardButton::Ptr inlineButton;

    buttonsRow.clear();
    for (auto& g : g_groups){
        inlineButton = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
        inlineButton->text = g.m_name;
        inlineButton->callbackData = "ping" + std::to_string(g.m_id);
        buttonsRow.push_back(inlineButton);

        if (buttonsRow.size() >= 2){
            inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
            buttonsRow.clear();
        }
    }
    if (buttonsRow.size() > 0){
        inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
        buttonsRow.clear();
    }

    try {
        g_bot->getApi().sendMessage(message->chat->id, "Кого пингануть?", false, 0, inlineKeyboard, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending ping command: %s\n", e.what());
    }
}

void TGCommands::pingCallback(TgBot::CallbackQuery::Ptr query){
    if (query->data.substr(0, 4) != "ping") return;
    if (query->message->chat->id != g_config.m_targetChat) { //other people can (i think) dm our bot and get private info about our class... I don't want that
        try {
            g_bot->getApi().answerCallbackQuery(query->id, "Этой коммандой можно пользоваться только в чате класса!", true);
        } catch (...) {}
        return;
    }
    if (g_classmates.size() < 1) {
        try {
            g_bot->getApi().answerCallbackQuery(query->id, "Список класса не загружен, попробуйте позже.", true);
        } catch (...) {}
        return;
    }

    unsigned char gId = 0;
    try {
        gId = atoi(query->data.substr(4).c_str());
    } catch (...) { return; }
    Group group;
    for (auto& g : g_groups) {
        if (g.m_id == gId) {
            group = g;
            break;
        }
    }
    if (group.m_name.empty()) return;
    std::string msg = "Пинг, " + group.m_name + "!";
    for (auto& c : g_classmates){
        if (!c.m_tgID) continue;
        if (!c.IsInGroup(group.m_id)) continue;
        msg += "<a href=\"tg://user?id=" + std::to_string(c.m_tgID) + "\">\u2009</a>";
    }

    try {
        g_bot->getApi().sendMessage(query->message->chat->id, msg, false, 0, nullptr, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending ping callback: %s\n", e.what());
        g_bot->getApi().answerCallbackQuery(query->id, "Произошла ошибка, попробуй ещё раз!", true);
        return;
    }

    try {
        g_bot->getApi().answerCallbackQuery(query->id, "Пинг группе \"" + group.m_name + "\" отправлен.");
    } catch (std::exception& e) { 
        printf("Caught exception while answering ping callback: %s\n", e.what());
    }
}

void TGCommands::ssite(TgBot::Message::Ptr message){
    try {
        g_bot->getApi().sendMessage(message->chat->id, "<a href=\"" + g_config.m_studentSite + "\">Сайт ученика</a>", false, 0, nullptr, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending student's site: %s\n", e.what());
    }
}

void TGCommands::courses(TgBot::Message::Ptr message){
    if (g_courses.size() < 1){
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Все курсы уже закончились. (Или они не загружены)", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }

    std::string msg = "";
    for (auto& c : g_courses){
        if (!msg.empty()) msg += "\n\n";
        msg += "Через ";
        msg += DaysToMonthsText(c.m_daysToEnd) + " ";
        msg += string_format("(%02i.%02i.%04i) ", c.m_end.tm_mday, c.m_end.tm_mon + 1, c.m_end.tm_year + 1900);

        msg += "закончатся следующие курсы: ";
        for (auto& s : c.m_subjects) msg += s + ", ";
        msg = msg.substr(0, msg.length() - 2);
        msg += ".";
    }

    try {
        g_bot->getApi().sendMessage(message->chat->id, msg, false, 0, nullptr, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending courses: %s\n", e.what());
    }
}

void TGCommands::holidays(TgBot::Message::Ptr message){
    static long long nextReload = 0;
    if (nextReload <= GetCurrentTime()){
        nextReload = GetCurrentTime() + GetSecondsToMidnight();
        LoadHolidays();
    }
    
    if (g_holidays.size() < 1){
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Каинкул не будет.", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }

    std::string msg = "Предстоящие каникулы:";
    bool first = true;
    for (auto& h : g_holidays){
        msg += "\n- ";
        if (first) msg += "<b>";
        if (h.m_bStarted){
            msg += "Закончатся через ";
        }
        else {
            msg += "Начнутся через ";
        }

        msg += DaysToMonthsText(h.m_daysTo) + " ";
        msg += string_format("(%02i.%02i.%04i - %02i.%02i.%04i)", h.m_start.tm_mday, h.m_start.tm_mon + 1, h.m_start.tm_year + 1900,
                                                                  h.m_end.tm_mday, h.m_end.tm_mon + 1, h.m_end.tm_year + 1900);
        if (first) msg += "</b>";
        first = false;
    }

    try {
        g_bot->getApi().sendMessage(message->chat->id, msg, false, 0, nullptr, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending holidays: %s\n", e.what());
    }
}

void TGCommands::reminders(TgBot::Message::Ptr message){
    if (message->chat->id != g_config.m_targetChat) { //other people can (i think) dm our bot and get private info about our class... I don't want that
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Этой коммандой можно пользоваться только в чате класса!", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }

    std::vector<Reminder> rems = g_remindChecker.GetReminders();
    std::string msg = "";
    if (rems.size() > 0){
        msg = "Текущие напоминания:\n";
        for (auto& r : rems){
            msg += "Через " + DaysToMonthsText(r.m_daysTo);
            msg += string_format(" (%02i.%02i.%04i): ", r.m_time.tm_mday, r.m_time.tm_mon + 1, r.m_time.tm_year + 1900);
            msg += r.m_text + "\n\n";
        }
        msg = msg.substr(0, msg.length() - 2);
    }
    else {
        msg = "Никаких напоминаний нет.";
    }

    try {
        g_bot->getApi().sendMessage(message->chat->id, msg, false, 0, nullptr, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending reminders: %s\n", e.what());
    }
}

std::unordered_map<int, std::string> reminderTexts; //putting it here, because callback data is too small for it
void TGCommands::addReminder(TgBot::Message::Ptr message){
    if (message->chat->id != g_config.m_targetChat) { //other people can (i think) dm our bot and get private info about our class... I don't want that
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Этой коммандой можно пользоваться только в чате класса!", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }

    std::string msg = "";
    TgBot::InlineKeyboardMarkup::Ptr inlineKeyboard(new TgBot::InlineKeyboardMarkup);

    std::string args;
    if (message->text.find(' ') != std::string::npos){
        args = message->text.substr(message->text.find(' ') + 1);
        std::size_t dateDelim = args.find(' ');
        if (dateDelim == std::string::npos || dateDelim + 1 >= args.length()) goto FAILED;
        std::string date = args.substr(0, dateDelim);
        std::string text = args.substr(dateDelim + 1);
        if (date.length() < 1 || text.length() < 1) goto FAILED;
        
        std::vector<std::string> dateParts = Explode(date, ".");
        if (dateParts.size() != 3) goto FAILED;
        tm targetDate;
        memset(&targetDate, 0, sizeof(targetDate));
        try {
            targetDate.tm_mday = stoi(dateParts[0]);
            targetDate.tm_mon = stoi(dateParts[1]) - 1;
            targetDate.tm_year = stoi(dateParts[2]) - 1900;
        } catch (...) { goto FAILED; }

        time_t currentTime = time(NULL);
        tm* currentTm = localtime(&currentTime);
        currentTm->tm_hour = 0;
        currentTm->tm_min = 0;
        currentTm->tm_sec = 0;
        currentTime = mktime(currentTm);
        double timeDiff = difftime(mktime(&targetDate), currentTime);
        if (timeDiff < 0) {
            msg = "Как только я научусь путешествовать во времени, я сразу установлю напоминание на этот день!";
            goto SEND_MSG;
        }
        if (timeDiff >= 63072000) {
            msg = "Я вряд ли столько проживу.";
            goto SEND_MSG;
        }
        timeDiff = round(timeDiff / 86400); //converting to days
        if (timeDiff < 1) {
            msg = "Напоминание необходимо установить как минимум за день до события.";
            goto SEND_MSG;
        }

        int textId = RandomInt(0, 999999999);
        while (reminderTexts.count(textId) > 0) textId = RandomInt(0, 999999999);
        reminderTexts[textId] = text;
        msg = "Детали напоминания:\n"
              "<u>Текст:</u> " + text + "\n"
              "<u>Срок</u>: через " + DaysToMonthsText(timeDiff) + string_format(" (%02i.%02i.%04i)", targetDate.tm_mday, targetDate.tm_mon + 1, targetDate.tm_year + 1900) + "\n\n" +
              "Если всё правильно, то выбери когда отправлять напоминание (можно выбрать несколько). Напоминание также будет отправлено в день события в 7 утра. Иначе нажми Отмена.";
        std::vector<TgBot::InlineKeyboardButton::Ptr> buttonsRow;
        TgBot::InlineKeyboardButton::Ptr inlineButton(new TgBot::InlineKeyboardButton);
        inlineButton->text = "❌ За день";
        inlineButton->callbackData = "addReminder_toggle|0";
        buttonsRow.push_back(inlineButton);
        inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
        buttonsRow.clear();
        inlineButton = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
        inlineButton->text = "❌ За 1 неделю";
        inlineButton->callbackData = "addReminder_toggle|1";
        buttonsRow.push_back(inlineButton);
        inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
        buttonsRow.clear();
        inlineButton = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
        inlineButton->text = "❌ За 2 недели";
        inlineButton->callbackData = "addReminder_toggle|2";
        buttonsRow.push_back(inlineButton);
        inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
        buttonsRow.clear();
        inlineButton = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
        inlineButton->text = "❌ За 3 недели";
        inlineButton->callbackData = "addReminder_toggle|3";
        buttonsRow.push_back(inlineButton);
        inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
        buttonsRow.clear();
        inlineButton = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
        inlineButton->text = "❌ За 4 недели";
        inlineButton->callbackData = "addReminder_toggle|4";
        buttonsRow.push_back(inlineButton);
        inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
        buttonsRow.clear();
        inlineButton = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
        inlineButton->text = "Сохранить";
        inlineButton->callbackData = "aRem|" + std::to_string(textId) + "|" + date + "|";
        buttonsRow.push_back(inlineButton);
        inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
        buttonsRow.clear();
        inlineButton = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
        inlineButton->text = "Отмена";
        inlineButton->callbackData = "aRem|";
        buttonsRow.push_back(inlineButton);
        inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
        buttonsRow.clear();
    }
    else { //no arguments at all
FAILED:
        msg = "Эта команда работает так: <code>/addreminder [дата] [сообщение]</code>\nНапример: <code>/addreminder ";
        srand(time(NULL));
        msg += std::to_string(rand() % 18 + 13) + "." + std::to_string(rand() % 3 + 10) + "." + std::to_string(2024 + rand() % 7) + " ";
        for (unsigned char i = 0; i < rand() % 6 + 5; i++) msg += char(rand() % 26 + 97);
        msg += "</code>";
    }

SEND_MSG:
    try {
        g_bot->getApi().sendMessage(message->chat->id, msg, false, 0, inlineKeyboard, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending add reminder: %s\n", e.what());
    }
}

void TGCommands::addReminderCallback(TgBot::CallbackQuery::Ptr query){
    if (query->data.substr(0, 5) != "aRem|") return;
    if (query->message->chat->id != g_config.m_targetChat) { //other people can (i think) dm our bot and get private info about our class... I don't want that
        try {
            g_bot->getApi().answerCallbackQuery(query->id, "Этой коммандой можно пользоваться только в чате класса!", true);
        } catch (...) {}
        return;
    }
    std::vector<std::string> args = Explode(query->data, "|");
    if (args.size() == 2){
        try {
            g_bot->getApi().editMessageText("Добавление напоминания отменено.", query->message->chat->id, query->message->messageId, "", "HTML");
        } catch (std::exception& e) {
            printf("Caught exception while editing add reminder message (cancel): %s\n", e.what());
        }
        return;
    }
    else if (args.size() != 4){
FAIL:
        try {
            g_bot->getApi().editMessageText("Что-то сломалось, попробуйте ещё раз.", query->message->chat->id, query->message->messageId, "", "HTML");
        } catch (std::exception& e) {
            printf("Caught exception while editing add reminder message (fail): %s\n", e.what());
        }
        return;
    }

    Reminder reminder;
    try {
        int textId = stoi(args[1]);
        if (reminderTexts.count(textId) < 1) goto FAIL;
        reminder.m_text = reminderTexts[textId];
        reminderTexts.erase(textId);
    } catch (...) { goto FAIL; }
    
    std::vector<std::string> dateParts = Explode(args[2], ".");
    if (dateParts.size() != 3) goto FAIL;
    try {
        reminder.m_time.tm_mday = stoi(dateParts[0]);
        reminder.m_time.tm_mon = stoi(dateParts[1]) - 1;
        reminder.m_time.tm_year = stoi(dateParts[2]) - 1900;
    } catch (...) { goto FAIL; }
    time_t currentTime = time(NULL);
    tm* currentTm = localtime(&currentTime);
    currentTm->tm_hour = 0;
    currentTm->tm_min = 0;
    currentTm->tm_sec = 0;
    currentTime = mktime(currentTm);
    double timeDiff = difftime(mktime(&reminder.m_time), currentTime);
    if (timeDiff < 0) {
        try {
            g_bot->getApi().editMessageText("Как только я научусь путешествовать во времени, я сразу установлю напоминание на этот день!", query->message->chat->id, query->message->messageId, "", "HTML");
        } catch (...) {}
        return;
    }
    if (timeDiff >= 63072000) {
        try {
            g_bot->getApi().editMessageText("Я вряд ли столько проживу.", query->message->chat->id, query->message->messageId, "", "HTML");
        } catch (...) {}
        return;
    }
    timeDiff = round(timeDiff / 86400); //converting to days
    if (timeDiff < 1) {
        try {
            g_bot->getApi().editMessageText("Напоминание необходимо установить как минимум за день до события.", query->message->chat->id, query->message->messageId, "", "HTML");
        } catch (...) {}
        return;
    }

    std::vector<std::string> days = Explode(args[3], ",");
    for (auto& d : days) reminder.m_daysBeforeReminder.push_back(atoi(d.c_str()));

    g_remindChecker.AddReminder(reminder);
    try {
        g_bot->getApi().editMessageText("Напоминание добавлено.", query->message->chat->id, query->message->messageId, "", "HTML");
    } catch (std::exception& e) {
        printf("Caught exception while editing add reminder message (added): %s\n", e.what());
    }
}

void TGCommands::addReminderToggleCallback(TgBot::CallbackQuery::Ptr query){
    if (query->data.substr(0, 19) != "addReminder_toggle|") return;
    if (query->message->chat->id != g_config.m_targetChat) { //other people can (i think) dm our bot and get private info about our class... I don't want that
        try {
            g_bot->getApi().answerCallbackQuery(query->id, "Этой коммандой можно пользоваться только в чате класса!", true);
        } catch (...) {}
        return;
    }

    if (query->data.length() != 20) {
FAIL:
        try {
            g_bot->getApi().editMessageText("Что-то сломалось, попробуйте ещё раз.", query->message->chat->id, query->message->messageId, "", "HTML");
        } catch (std::exception& e) {
            printf("Caught exception while editing add reminder message (toggle fail): %s\n", e.what());
        }
        return;
    }
    unsigned char daysAmount = atoi(query->data.substr(19).c_str()) * 7;
    if (daysAmount < 1) daysAmount = 1;
    bool add = false;

    TgBot::InlineKeyboardMarkup::Ptr inlineKeyboard = query->message->replyMarkup;
    for (auto& row : inlineKeyboard->inlineKeyboard){
        for (auto& b : row){
            if (b->callbackData == query->data){ //our button
                if (b->text.size() < 3) continue; //avoiding exception...
                bool toggledOn = b->text.substr(0, 3) == "✅";
                if (toggledOn) b->text.replace(0, 3, "❌");
                else {
                    b->text.replace(0, 3, "✅");
                    add = true;
                }
                goto DONE_TOGGLING;
            }
        }
    }
DONE_TOGGLING:
    for (auto& row : inlineKeyboard->inlineKeyboard){
        for (auto& b : row){
            if (b->callbackData.length() <= 5) continue; //filters out cancel button too
            if (b->callbackData.substr(0, 5) == "aRem|"){ //save button
                std::vector<std::string> args = Explode(b->callbackData, "|");
                if (args.size() != 4) continue; //not correct button...
                std::vector<std::string> days = Explode(args[3], ",");
                if (add) days.push_back(std::to_string(daysAmount));
                else {
                    for (size_t i = 0; i < days.size(); i++){
                        if (days[i] == std::to_string(daysAmount)) {
                            days.erase(days.begin() + i);
                            break;
                        }
                    }
                }
                args[3] = "";
                for (auto& d : days){
                    args[3] += d + ",";
                }
                args[3] = args[3].substr(0, args[3].length() - 1);
                b->callbackData = "";
                for (auto& a : args){
                    b->callbackData += a + "|";
                }
                b->callbackData = b->callbackData.substr(0, b->callbackData.length() - 1);
                goto DONE_ADDING;
            }
        }
    }
DONE_ADDING:

    try {
        g_bot->getApi().editMessageReplyMarkup(query->message->chat->id, query->message->messageId, "", inlineKeyboard);
    } catch (std::exception& e) {
        printf("Caught exception while editing add reminder inline keyboard: %s\n", e.what());
    }
}

void TGCommands::remReminder(TgBot::Message::Ptr message){
    if (message->chat->id != g_config.m_targetChat) { //other people can (i think) dm our bot and get private info about our class... I don't want that
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Этой коммандой можно пользоваться только в чате класса!", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }
    std::vector<Reminder> reminders = g_remindChecker.GetReminders();
    if (reminders.size() < 1) {
        try {
            g_bot->getApi().sendMessage(message->chat->id, "Нет напоминаний, которые можно было бы удалить.", false, 0, nullptr, "HTML");
        } catch (...) {}
        return;
    }

    TgBot::InlineKeyboardMarkup::Ptr inlineKeyboard(new TgBot::InlineKeyboardMarkup);
    std::vector<TgBot::InlineKeyboardButton::Ptr> buttonsRow;
    for (size_t i = 0; i < reminders.size() && i < 5; i++){
        Reminder& r = reminders[i];
        TgBot::InlineKeyboardButton::Ptr inlineButton(new TgBot::InlineKeyboardButton);
        inlineButton->text = string_format("%02i.%02i.%04i: ", r.m_time.tm_mday, r.m_time.tm_mon + 1, r.m_time.tm_year + 1900) + r.m_text;
        inlineButton->callbackData = "remReminder|" + std::to_string(i) + "|" + std::to_string(reminders.size());
        buttonsRow.push_back(inlineButton);
        inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
        buttonsRow.clear();
    }
    if (reminders.size() > 5){
        TgBot::InlineKeyboardButton::Ptr inlineButton(new TgBot::InlineKeyboardButton);
        inlineButton->text = "➡";
        inlineButton->callbackData = "remReminder|5";
        buttonsRow.push_back(inlineButton);
        inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
        buttonsRow.clear();
    }

    try {
        g_bot->getApi().sendMessage(message->chat->id, "Выберите, какое напоминание удалить.", false, 0, inlineKeyboard, "HTML");
    } catch (std::exception& e) { 
        printf("Caught exception while sending rem reminder: %s\n", e.what());
    }
}

void TGCommands::remReminderCallback(TgBot::CallbackQuery::Ptr query){
    if (query->data.substr(0, 12) != "remReminder|") return;
    if (query->message->chat->id != g_config.m_targetChat) { //other people can (i think) dm our bot and get private info about our class... I don't want that
        try {
            g_bot->getApi().answerCallbackQuery(query->id, "Этой коммандой можно пользоваться только в чате класса!", true);
        } catch (...) {}
        return;
    }
    
    std::vector<std::string> args = Explode(query->data, "|");
    std::vector<Reminder> reminders = g_remindChecker.GetReminders();
    if (args.size() == 2){ //switching pages
        size_t startFrom = 0;
        try {
            startFrom = stoul(args[1]);
        } catch (...) { goto FAIL; }
        if (startFrom >= reminders.size()) goto FAIL;

        TgBot::InlineKeyboardMarkup::Ptr inlineKeyboard(new TgBot::InlineKeyboardMarkup);
        std::vector<TgBot::InlineKeyboardButton::Ptr> buttonsRow;
        for (size_t i = startFrom; i < reminders.size() && i < startFrom + 5; i++){
            Reminder& r = reminders[i];
            TgBot::InlineKeyboardButton::Ptr inlineButton(new TgBot::InlineKeyboardButton);
            inlineButton->text = string_format("%02i.%02i.%04i: ", r.m_time.tm_mday, r.m_time.tm_mon + 1, r.m_time.tm_year + 1900) + r.m_text;
            inlineButton->callbackData = "remReminder|" + std::to_string(i) + "|" + std::to_string(reminders.size());
            buttonsRow.push_back(inlineButton);
            inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
            buttonsRow.clear();
        }
        if (startFrom > 0) {
            TgBot::InlineKeyboardButton::Ptr inlineButton(new TgBot::InlineKeyboardButton);
            inlineButton->text = "⬅";
            size_t backStartFrom = startFrom - 5;
            if (startFrom < 5) backStartFrom = 0;
            inlineButton->callbackData = "remReminder|" + std::to_string(backStartFrom);
            buttonsRow.push_back(inlineButton);
        }
        if (startFrom + 5 < reminders.size()){
            TgBot::InlineKeyboardButton::Ptr inlineButton(new TgBot::InlineKeyboardButton);
            inlineButton->text = "➡";
            inlineButton->callbackData = "remReminder|" + std::to_string(startFrom + 5);
            buttonsRow.push_back(inlineButton);
        }
        inlineKeyboard->inlineKeyboard.push_back(buttonsRow);
        buttonsRow.clear();

        try {
            g_bot->getApi().editMessageReplyMarkup(query->message->chat->id, query->message->messageId, "", inlineKeyboard);
        } catch (std::exception& e){
            printf("Caught exception while editing rem reminder message (switch page): %s\n", e.what());
        }
        return;
    }
    else if (args.size() == 3){ //removing reminder
        size_t remIndex = 0;
        size_t sizeOfReminders = 0;
        try {
            remIndex = stoul(args[1]);
            sizeOfReminders = stoul(args[2]);
        } catch (...) { goto FAIL; }
        if (sizeOfReminders != reminders.size() || remIndex >= reminders.size()) goto FAIL;
        g_remindChecker.RemoveReminder(remIndex);

        try {
            g_bot->getApi().editMessageText("Напоминалка удалена.", query->message->chat->id, query->message->messageId, "", "HTML");
        } catch (std::exception& e){
            printf("Caught exception while editing rem reminder message: %s\n", e.what());
        }
        return;
    }
    else {
FAIL:
        try {
            g_bot->getApi().editMessageText("Что-то сломалось, попробуйте ещё раз.", query->message->chat->id, query->message->messageId, "", "HTML");
        } catch (std::exception& e) {
            printf("Caught exception while editing rem reminder message (fail): %s\n", e.what());
        }
        return;
    }
}