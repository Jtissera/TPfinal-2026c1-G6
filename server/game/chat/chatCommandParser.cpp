#include "chatCommandParser.h"

std::string ChatCommandParser::trim(const std::string &s)
{
    auto notSpace = [](unsigned char c)
    {
        return !std::isspace(c);
    };

    auto start = std::find_if(s.begin(), s.end(), notSpace);
    auto end = std::find_if(s.rbegin(), s.rend(), notSpace).base();
    return (start < end) ? std::string(start, end) : std::string{};
}

std::optional<ParsedChatInput> ChatCommandParser::parse(const std::string &raw)
{
    std::string t = trim(raw);
    if (t.empty())
        return std::nullopt;

    if (t[0] == '/')
        return parseCommand(t.substr(1));

    if (t[0] == '@')
        return parsePrivate(t.substr(1));

    ParsedChatInput result;
    result.type = ChatInputType::GENERAL;
    result.keyword = t;
    return result;
}

ParsedChatInput ChatCommandParser::parseCommand(const std::string &body)
{
    ParsedChatInput result;
    result.type = ChatInputType::COMMAND;

    std::istringstream ss(body);
    ss >> result.keyword;
    std::transform(result.keyword.begin(), result.keyword.end(),
                   result.keyword.begin(), ::tolower);

    ss >> std::ws;
    std::getline(ss, result.argument);
    return result;
}

ParsedChatInput ChatCommandParser::parsePrivate(const std::string &body)
{
    ParsedChatInput result;
    result.type = ChatInputType::PRIVATE;

    std::istringstream ss(body);
    ss >> result.targetNick;
    ss >> std::ws;
    std::getline(ss, result.keyword);
    return result;
}