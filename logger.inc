
#define INVALID_LOGGER_ID  (Logger:-1)


#if !defined MAX_LOGGERS
    #define MAX_LOGGERS 100
#endif


#if !defined MAX_LOGGER_NAME_LENGTH
    #define MAX_LOGGER_NAME_LENGTH (16)
#endif

#if !defined MAX_LOGGER_FIELD_LENGTH
    #define MAX_LOGGER_FIELD_LENGTH (64)
#endif

enum LogLevel {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
}



static enum E_LOGGER_DATA {
    E_LOGGER_NAME[MAX_LOGGER_NAME_LENGTH char],
    File:E_LOGGER_FILE,
    bool:E_LOGGER_VALID,
    bool:E_LOGGER_PROPERFILE,
    LogLevel:E_LOGGER_LEVEL
}

static s_LoggerData[Logger:MAX_LOGGERS][E_LOGGER_DATA];


static s_Buffer[2048];



forward LoggerField:logger_string(const name[], const value[]);
forward LoggerField:logger_int(const name[], value);
forward LoggerField:logger_float(const name[], Float:value);
forward Logger:GetLogger(const handlerName[], bool:properFile = true, LogLevel:level = LOG_WARN);

Logger:GetLogger(const handlerName[], bool:properFile = true, LogLevel:level = LOG_WARN)
{
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
    s_LoggerData[logger][E_LOGGER_PROPERFILE] = properFile;
    s_LoggerData[logger][E_LOGGER_LEVEL] = level;

    new dateFormat[32];
    GetDateString(dateFormat);


    new file_name[64];
    format(file_name, sizeof(file_name),
        "logs/log_%s_%s.log", handlerName, dateFormat
    );
    s_LoggerData[logger][E_LOGGER_FILE] = fopen(file_name);


    return logger;
}







sendlog(Logger:logger, LogLevel:level, const message[], LoggerField:...)
{
    if(logger == INVALID_LOGGER_ID || level < s_LoggerData[logger][E_LOGGER_LEVEL]) {
        return;
    }

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

    if(totalArgs == 3) {
        strcat(s_Buffer, "\n");
        return;
    }

    for(new arg = 3; arg < totalArgs; ++arg) {
        new field[MAX_LOGGER_FIELD_LENGTH];

        for(new ch; ch < MAX_LOGGER_FIELD_LENGTH; ch++) {
            field[ch] = getarg(arg, ch);
            if(field[ch] == EOS) {
                strcat(s_Buffer, " ");
                strcat(s_Buffer, field);
                break;
            }
        }
    }

    strcat(s_Buffer, "\n");

    if(s_LoggerData[logger][E_LOGGER_PROPERFILE]) {
        fwrite(s_LoggerData[logger][E_LOGGER_FILE], s_Buffer);
    }
    else {
        print(s_Buffer);
    }
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


static GetDateString(output[], const size = sizeof(output))
{
    new day, month, year, hour, minute, second;
    gettime(hour, minute, second);
    getdate(year, month, day);

    format(output, size,
        "%02d_%02d_%02d_%02d;%02d;%02d", day, month, year, hour, minute, second
    );
}



static GetLogLevelName(LogLevel:level, output[], const size = sizeof(output))
{
    switch(level) {
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



stock SetLoggerLevel(LogLevel:level)
{
    s_LoggerData[logger][E_LOGGER_LEVEL] = level;
}

stock LogLevel:GetLoggerLevel(Logger:logger)
{
    return s_LoggerData[logger][E_LOGGER_LEVEL];
}