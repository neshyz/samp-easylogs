#if defined _logger_included
    #endinput
#endif

#define _logger_included

#define INVALID_LOGGER_ID  (Logger:-1)


#if !defined MAX_LOGGERS
    #define MAX_LOGGERS 100
#endif


#if !defined MAX_LOGGER_NAME_LENGTH
    #define MAX_LOGGER_NAME_LENGTH (16)
#endif

#if !defined MAX_LOGGER_FIELD_LENGTH
    #define MAX_LOGGER_FIELD_LENGTH (128)
#endif

#if !defined LOGGER_USE_INIT_CALLBACKS
    #define LOGGER_USE_INIT_CALLBACKS
#endif

#if !defined LOGGER_USE_LOG_CALLBACKS
    #define LOGGER_USE_LOG_CALLBACKS
#endif


static bool:s_LoggerInit = false;


enum LogLevel {
    LOG_ALL,
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
    LOG_OFF
}



static enum E_LOGGER_DATA {
    E_LOGGER_NAME[MAX_LOGGER_NAME_LENGTH char],
    File:E_LOGGER_FILE,
    bool:E_LOGGER_VALID,
    LogLevel:E_LOGGER_APPEND_FILE,
    LogLevel:E_LOGGER_APPEND_CONSOLE,
    LogLevel:E_LOGGER_LEVEL
}

static s_LoggerData[Logger:MAX_LOGGERS][E_LOGGER_DATA];


static s_Buffer[2048];


static Logger:s_ActiveLogger = INVALID_LOGGER_ID;


// cambiar el LogLevel de un logger
forward SetLoggerLevel(Logger:logger, LogLevel:level);

// obtener el LogLevel de un logger
forward LogLevel:GetLoggerLevel(Logger:logger);

// cambiar el LogLevel de todos los loggers a la vez
forward SetAllLoggerLevel(LogLevel:level);

// ejecutar opcionalmente en OnGameModeExit
forward CloseLoggers();

// obtener el nombre de un logger
forward GetLoggerName(Logger:logger, output[], const size = sizeof(output));

// obtener el maximo de loggers en tiempo de ejecucion
forward GetMaxLoggers();



// obtener un LogLevel ID a partir de su nombre, ejemplo: `warning` retornara LogLevel:LOG_WARN (4)
forward LogLevel:GetLogLevelId(const logLevelName[]);


// configuracion de logger en tiempo de ejecucion
forward SetLoggerConsoleAppend(Logger:logger, LogLevel:consoleAppend);
forward LogLevel:GetLoggerConsoleAppend(Logger:logger);

forward SetLoggerFileAppend(Logger:logger, LogLevel:fileAppend);
forward LogLevel:GetLoggerFileAppend(Logger:logger);

// retorna true si el logger esta inicializado, false si el logger es invalido y no existe
forward bool:IsValidLogger(Logger:logger);




// logger_* retorna una cadena de etiqueta LoggerField con formato "field = valor"
forward LoggerField:logger_string(const name[], const value[]);
forward LoggerField:logger_int(const name[], value);
forward LoggerField:logger_float(const name[], Float:value);



// GetLogger permite generar un nuevo logger con configuración independiente
forward Logger:GetLogger(const handlerName[], LogLevel:fileAppend = LOG_OFF, LogLevel:consoleAppend = LOG_OFF, LogLevel:level = LOG_WARN);






// sendlog es la funcion utilizada para enviar logs a un handler, con un nivel, mensaje y fields
forward sendlog(Logger:logger, LogLevel:level, const message[], LoggerField:...);


#if defined LOGGER_USE_CALLBACKS
    forward OnLoggerInit(Logger:logger);
    #if defined LOGGER_USE_LOG_CALLBACKS
        forward OnLoggerLog(Logger:logger, LogLevel:level, const msg[]);
    #endif
#endif











ezlogs_init()
{
    for(new Logger:logger; logger < Logger:MAX_LOGGERS; logger++) {
        s_LoggerData[logger][E_LOGGER_APPEND_CONSOLE] = LOG_OFF;
        s_LoggerData[logger][E_LOGGER_APPEND_FILE] = LOG_OFF;
        #if defined LOGGER_USE_MYSQL
            SetLoggerDatabaseAppend(logger, LOG_OFF);
        #endif

        #if defined LOGGER_USE_DISCORD
            SetLoggerDiscordAppend(logger, LOG_OFF, "");
        #endif
        
        s_LoggerData[logger][E_LOGGER_LEVEL] = LOG_OFF;
        s_LoggerData[logger][E_LOGGER_VALID] = false;
    }
    s_LoggerInit = true;
}







Logger:GetLogger(const handlerName[], LogLevel:fileAppend = LOG_OFF, LogLevel:consoleAppend = LOG_OFF, LogLevel:level = LOG_WARN)
{
    if(!s_LoggerInit) {
        printf("usa \"ezmysql_init()\"  para inicializar easylogs!");
        return INVALID_LOGGER_ID;
    }

    new Logger:logger = INVALID_LOGGER_ID;

    for(new Logger:i_logger; i_logger < Logger:MAX_LOGGERS; i_logger++) {
        if(!s_LoggerData[i_logger][E_LOGGER_VALID]) {
            logger = i_logger;
            break;
        }
    }


    if(logger == INVALID_LOGGER_ID) {
        printf("ERROR: no se pudo crear el logger \"%s\"", handlerName);
        return INVALID_LOGGER_ID;
    }


    strpack(s_LoggerData[logger][E_LOGGER_NAME], handlerName);
    s_LoggerData[logger][E_LOGGER_VALID] = true;
    s_LoggerData[logger][E_LOGGER_APPEND_CONSOLE] = consoleAppend;
    s_LoggerData[logger][E_LOGGER_LEVEL] = level;

    


    s_LoggerData[logger][E_LOGGER_APPEND_FILE] = fileAppend;

    if(s_LoggerData[logger][E_LOGGER_APPEND_FILE] != LOG_OFF) {
        OpenLoggerFile(logger);
    }


    #if defined LOGGER_USE_CALLBACKS
        CallLocalFunction("OnLoggerInit", "i", _:logger);

        #if defined LOGGER_USE_MYSQL
            _OnLoggerInit_Mysql(logger);
        #endif
    #endif

    return logger;
}


static stock OpenLoggerFile(Logger:logger)
{
    new handlerName[MAX_LOGGER_NAME_LENGTH];
    GetLoggerName(logger, handlerName);


    new file_name[64];
    format(file_name, sizeof(file_name),
        "logs/log_%s.log", handlerName
    );

    s_LoggerData[logger][E_LOGGER_FILE] = fopen(file_name);
    fseek(s_LoggerData[logger][E_LOGGER_FILE],  .whence = seek_end);
}

static stock CloseLoggerFile(Logger:logger)
{
    fclose(s_LoggerData[logger][E_LOGGER_FILE]);
    s_LoggerData[logger][E_LOGGER_FILE] = File:-1;
}





sendlog(Logger:logger, LogLevel:level, const message[], LoggerField:...)
{
    if(!s_LoggerInit) {
        printf("usa \"ezmysql_init()\"  para inicializar easylogs!");
        return;
    }

    if(logger == INVALID_LOGGER_ID || level < s_LoggerData[logger][E_LOGGER_LEVEL]) {
        return;
    }

    s_ActiveLogger = logger;

    new totalArgs = numargs();
    
    new loggerName[MAX_LOGGER_NAME_LENGTH];
    strunpack(loggerName, s_LoggerData[logger][E_LOGGER_NAME]);

    new logLevelName[16];
    GetLogLevelName(level, logLevelName);

    new dateFormat[32];
    GetDateString(dateFormat);

    format(s_Buffer, sizeof(s_Buffer),
        "[%s]  %s (%s):\t%s", dateFormat, logLevelName,  loggerName, _:logger_string("message", message)   
    );

    if(totalArgs > 3) {
        for(new arg = 3; arg < totalArgs; ++arg) {
            new field[MAX_LOGGER_FIELD_LENGTH];

            for(new ch; ch < MAX_LOGGER_FIELD_LENGTH; ch++) {
                field[ch] = getarg(arg, ch);
                if(field[ch] == EOS) {
                    #if defined LOGGER_USE_MYSQL
                    _OnLoggerAppendField_Mysql(logger, level, message, field);
                    #endif

                    #if defined LOGGER_USE_DISCORD
                    _OnLoggerAppendField_Discord(logger, level, message, field);
                    #endif

                    strcat(s_Buffer, ", ");
                    strcat(s_Buffer, field);
                    break;
                }
            }
        }
    }

    strcat(s_Buffer, "\n");

    #if defined LOGGER_USE_CALLBACKS && defined LOGGER_USE_LOG_CALLBACKS
        if(!CallLocalFunction("OnLoggerLog", "iis", _:logger, _:level, s_Buffer)) {
            s_ActiveLogger = INVALID_LOGGER_ID;
            return;
        }

        #if defined LOGGER_USE_MYSQL
            _OnLoggerLog_Mysql(logger, level, s_Buffer);
        #endif

        #if defined LOGGER_USE_DISCORD
            _OnLoggerLog_Discord(logger, level, s_Buffer);
        #endif
    #endif
    if(level >= s_LoggerData[logger][E_LOGGER_APPEND_FILE]) {
        fwrite(s_LoggerData[logger][E_LOGGER_FILE], s_Buffer);
    }
    
    if(level >= s_LoggerData[logger][E_LOGGER_APPEND_CONSOLE]) {
        print(s_Buffer);
    }

    s_ActiveLogger = INVALID_LOGGER_ID;
}




stock LoggerField:logger_int(const name[], value)
{
    new field[MAX_LOGGER_FIELD_LENGTH];
    format(field, sizeof(field),
        "%s = %d", name, value
    );
    return LoggerField:field;
}

stock LoggerField:logger_float(const name[], Float:value)
{
    new field[MAX_LOGGER_FIELD_LENGTH];
    format(field, sizeof(field),
        "%s = %f", name, value
    );
    return LoggerField:field;
}

stock LoggerField:logger_string(const name[], const value[])
{
    new field[MAX_LOGGER_FIELD_LENGTH];
    format(field, sizeof(field),
        "%s = \"%s\"", name, value
    );
    return LoggerField:field;
}


static stock GetDateString(output[], const size = sizeof(output))
{
    new day, month, year, hour, minute, second;
    gettime(hour, minute, second);
    getdate(year, month, day);

    format(output, size,
        "%02d/%02d/%02d-%02d:%02d:%02d", day, month, year, hour, minute, second
    );
}



stock GetLogLevelName(LogLevel:level, output[], const size = sizeof(output))
{
    switch(level) {
        case LOG_ALL: {
            format(output, size, "all");
        }
        case LOG_OFF: {
            format(output, size, "off");
        }
        case LOG_TRACE: {
            format(output, size, "trace");
        }
        case LOG_DEBUG: {
            format(output, size, "debug");
        }
        case LOG_INFO: {
            format(output, size, "info");
        }
        case LOG_WARN: {
            format(output, size, "warn");
        }
        case LOG_ERROR: {
            format(output, size, "error");
        }
        case LOG_FATAL: {
            format(output, size, "fatal");
        }
    }
}






stock SetLoggerLevel(Logger:logger, LogLevel:level)
{
    s_LoggerData[logger][E_LOGGER_LEVEL] = level;
}



stock LogLevel:GetLoggerLevel(Logger:logger)
{
    return s_LoggerData[logger][E_LOGGER_LEVEL];
}



stock SetLoggerConsoleAppend(Logger:logger, LogLevel:consoleAppend)
{
    s_LoggerData[logger][E_LOGGER_APPEND_CONSOLE] = consoleAppend;
}


stock LogLevel:GetLoggerConsoleAppend(Logger:logger)
{
    return s_LoggerData[logger][E_LOGGER_APPEND_CONSOLE];
}



stock SetLoggerFileAppend(Logger:logger, LogLevel:fileAppend)
{
    if(s_LoggerData[logger][E_LOGGER_APPEND_FILE] != LOG_OFF && fileAppend == LOG_OFF) {
        CloseLoggerFile(logger);
    }
    else if(s_LoggerData[logger][E_LOGGER_APPEND_FILE] == LOG_OFF && fileAppend != LOG_OFF) {
        OpenLoggerFile(logger);
    }

    s_LoggerData[logger][E_LOGGER_APPEND_FILE] = fileAppend;
}


stock LogLevel:GetLoggerFileAppend(Logger:logger)
{
    return s_LoggerData[logger][E_LOGGER_APPEND_FILE];
}



stock SetAllLoggerLevel(LogLevel:level)
{
    for(new Logger:logger; logger < Logger:MAX_LOGGERS; logger++) {
        if(s_LoggerData[logger][E_LOGGER_VALID]) {
            s_LoggerData[logger][E_LOGGER_LEVEL] = level;
        }
    }
}



stock CloseLoggers() 
{
    new count;
    for(new Logger:logger; logger < Logger:MAX_LOGGERS; logger++) {
        if(s_LoggerData[logger][E_LOGGER_VALID]) {
            if(s_LoggerData[logger][E_LOGGER_APPEND_FILE] != LOG_OFF) {
                CloseLoggerFile(logger);
            }

            s_LoggerData[logger][E_LOGGER_VALID] = false;
            count++;
        }
    }

    printf("logger:  se cerraron %d loggers.", count);
}


stock GetLoggerName(Logger:logger, output[], const size = sizeof(output))
{
    strunpack(output, s_LoggerData[logger][E_LOGGER_NAME], size);
}


stock GetMaxLoggers()
{
    return MAX_LOGGERS;
}


stock LogLevel:GetLogLevelId(const logLevelName[])
{
    new tempName[16];
    for(new LogLevel:level; level < LogLevel; level++) {
        GetLogLevelName(level, tempName);

        if(strcmp(logLevelName, tempName, false) == 0 && !easylogs_isempty(tempName)) {
            return level;
        }
    }

    return LOG_OFF;
}


stock bool:IsValidLogger(Logger:logger)
{
    return s_LoggerData[logger][E_LOGGER_VALID];
}

stock Logger:GetActiveLogger()
{
    return s_ActiveLogger;
}











stock easylogs_isempty(const string[]) {
	if (ispacked(string))
		return string{0} == '\0';
	else
		return string[0] == '\0';
}