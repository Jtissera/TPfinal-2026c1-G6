#pragma once

#include <string>
#include <optional>

enum class ChatInputType
{
    GENERAL, // Sin prefijo  → difundir a la sala
    PRIVATE, // Prefijo '@'  → mensaje privado
    COMMAND, // Prefijo '/'  → comando del juego
};

struct ParsedChatInput
{
    ChatInputType type = ChatInputType::GENERAL;
    std::string keyword;    // GENERAL: texto completo. COMMAND: nombre del cmd. PRIVATE: cuerpo.
    std::string argument;   // COMMAND: resto después del comando (ej. nombre de ítem)
    std::string targetNick; // PRIVATE: nick del destinatario
};

class ChatCommandParser
{
public:
    static std::optional<ParsedChatInput> parse(const std::string &raw);

private:
    static std::string trim(const std::string &s);
    static ParsedChatInput parseCommand(const std::string &body);
    static ParsedChatInput parsePrivate(const std::string &body);
};
