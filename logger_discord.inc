#if defined _logger_module_discord
    #endinput
#endif

#if !defined _logger_included
    #error "logger_discord.inc es un modulo y debe ser incluido despues de logger.inc"
#endif

#define _logger_module_discord


#if !defined MAX_LOGGER_DISCORD_CHANNEL_SIZE
    #define MAX_LOGGER_DISCORD_CHANNEL_SIZE (64)
#endif

#if !defined MAX_LOGGER_DISCORD_MSG_SIZE
    #define MAX_LOGGER_DISCORD_MSG_SIZE (1024)
#endif

#if !defined MAX_LOGGER_DISCORD_TITLE_SIZE
    #define MAX_LOGGER_DISCORD_TITLE_SIZE (128)
#endif


static enum E_LOGGER_DISCORD_DATA {
    LogLevel:E_LOGGER_DISCORD_APPEND,
    E_LOGGER_CHANNEL_NAME[MAX_LOGGER_DISCORD_CHANNEL_SIZE char],
}
static s_LoggerDiscord[Logger:MAX_LOGGERS][E_LOGGER_DISCORD_DATA];



static s_TitleBuffer[MAX_LOGGER_DISCORD_TITLE_SIZE];
static s_MessageBuffer[MAX_LOGGER_DISCORD_MSG_SIZE];
static s_FieldCount;


_OnLoggerLog_Discord(const Logger:logger, const LogLevel:level, const msg[])
{
    #pragma unused msg, level
    if(level < s_LoggerDiscord[logger][E_LOGGER_DISCORD_APPEND]) {
        return;
    }

    if(s_FieldCount < 1) {
        format(s_TitleBuffer, sizeof(s_TitleBuffer), msg);
        format(s_MessageBuffer, sizeof(s_MessageBuffer), " ");
    }

    new
        discordChannelName[MAX_LOGGER_DISCORD_CHANNEL_SIZE];

    strunpack(discordChannelName, s_LoggerDiscord[logger][E_LOGGER_CHANNEL_NAME]);

    CallRemoteFunction("SendDiscordMessage", "sss", s_TitleBuffer, s_MessageBuffer, discordChannelName);

    format(s_TitleBuffer, sizeof(s_TitleBuffer), "");
    format(s_MessageBuffer, sizeof(s_MessageBuffer), "");
    s_FieldCount = 0;
}


_OnLoggerAppendField_Discord(const Logger:logger, const LogLevel:level, const msg[], const field[])
{
    #pragma unused level
    if(level < s_LoggerDiscord[logger][E_LOGGER_DISCORD_APPEND]) {
        return;
    }
    
    if(easylogs_isempty(s_TitleBuffer)) {
        format(s_TitleBuffer, sizeof(s_TitleBuffer), msg);
    }

    if(s_FieldCount > 0) {
        strcat(s_MessageBuffer, "\n");
    }

    strcat(s_MessageBuffer, field);
    s_FieldCount++;
}




SetLoggerDiscordAppend(const Logger:logger, const LogLevel:discordAppend, const channelName[])
{
    s_LoggerDiscord[logger][E_LOGGER_DISCORD_APPEND] = discordAppend;

    if(discordAppend) {
        strpack(s_LoggerDiscord[logger][E_LOGGER_CHANNEL_NAME], channelName);
    }
}