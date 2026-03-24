#if defined _logger_module_discord
    #endinput
#endif

#if !defined _logger_included
    #error "logger_discord.inc es un modulo y debe ser incluido despues de logger.inc"
#endif

#define _logger_module_discord


#if !defined MAX_LOGGER_DISCORD_CHANNEL_SIZE
    #define MAX_LOGGER_DISCORD_CHANNEL_SIZE (32)
#endif


static enum E_LOGGER_DISCORD_DATA {
    bool:E_LOGGER_USE_DISCORD,
    E_LOGGER_CHANNEL_NAME[MAX_LOGGER_DISCORD_CHANNEL_SIZE char],
}
static s_LoggerDiscord[Logger:MAX_LOGGERS][E_LOGGER_DISCORD_DATA];


_OnLoggerLog_Discord(const Logger:logger, const LogLevel:level, const msg[])
{
    if(!s_LoggerDiscord[logger][E_LOGGER_USE_DISCORD]) {
        return;
    }

    new
        discordChannelName[MAX_LOGGER_DISCORD_CHANNEL_SIZE],
        logLevelName[32];

    strunpack(s_LoggerDiscord[logger][E_LOGGER_CHANNEL_NAME], discordChannelName);
    GetLogLevelName(level, logLevelName);

    CallRemoteFunction("SendDiscordMessage", "sss", discordChannelName, logLevelName, msg);
}





SetLoggerDiscordAppend(const Logger:logger, const bool:discordAppend, const channelName[])
{
    s_LoggerDiscord[logger][E_LOGGER_USE_DISCORD] = discordAppend;

    if(discordAppend) {
        strpack(s_LoggerDiscord[logger][E_LOGGER_CHANNEL_NAME], channelName);
    }
}