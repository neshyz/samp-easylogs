#if defined _logger_module_config
    #endinput
#endif

#define _logger_module_config

#if !defined _json_included
    #tryinclude <json>
    #if !defined _json_included
        #warning "logger_config.inc no pudo incluirse, json.inc no esta instalado y no se pudo encontrar"
        #endinput
    #endif
#endif

#if !defined _logger_included
    #error "logger_config.inc es un modulo y debe ser incluido despues de logger.inc"
#endif






#if !defined LOGGER_DEFAULT_CONFIG_FILE_PATH
    #define LOGGER_DEFAULT_CONFIG_FILE_PATH "config.json"
#endif

#if !defined LOGGER_DEFAULT_CONSOLE_APPEND
    #define LOGGER_DEFAULT_CONSOLE_APPEND (LOG_DEBUG)
#endif

#if !defined LOGGER_DEFAULT_FILE_APPEND
    #define LOGGER_DEFAULT_FILE_APPEND (LOG_WARN)
#endif


#if !defined LOGGER_DEFAULT_LEVEL
    #define LOGGER_DEFAULT_LEVEL (LOG_ALL)
#endif

#if !defined LOGGER_MYSQL_CONFIG_DELAY
    #define LOGGER_MYSQL_CONFIG_DELAY (1000)
#endif


// cargar la configuración de loggers proveniente de un archivo .json
// filepath es la ruta al archivo junto con su extension
// ej: "logger_config.json", "scriptfiles/logger.json"
// se recomienda alojarlo en la carpeta scriptfiles/ por consistencia
forward LoadLoggerConfig(const filepath[], &MySQL:handle);


// se llamará a este callback cuando el logger cargue su configuración.
// Node:loggerCOnfig contendrá la configuración encontrada (sera Node:0 si no habíá ninguna configuración cargada)
// callback útil para cargar tu propia config a un logger
forward OnLoggerConfigLoaded(Logger:logger, Node:loggerConfig);


static
    Node:s_LoggerConfig
;


public LoadLoggerConfig(const filepath[], &MySQL:handle)
{
    if(s_LoggerConfig != Node:0) {
        JSON_Cleanup(s_LoggerConfig);
        s_LoggerConfig = Node:0;
    }


    JSON_ParseFile(filepath, s_LoggerConfig);
    JSON_ToggleGC(s_LoggerConfig, false);

    new loggerName[MAX_LOGGER_NAME_LENGTH];
    new temp_stringlevel[32];

    for(new Logger:logger; logger < Logger:GetMaxLoggers(); logger++) {

        if(!IsValidLogger(logger)) {
            continue;
        }

        
        GetLoggerName(logger, loggerName);

        // cargaremos la configuración del logger almacenada en filepath en variables
        new
            LogLevel:consoleAppend = LOG_OFF, LogLevel:fileAppend = LOG_OFF,
            LogLevel:level = LOG_OFF
        ;

        new Node:loggerConfig;
        if(!JSON_GetObject(s_LoggerConfig, loggerName, loggerConfig)) {
            

            // se encontró el objeto que contiene la configuración de este logger

            JSON_ToggleGC(loggerConfig, false);

            if(!JSON_GetString(loggerConfig, "consoleAppend", temp_stringlevel)) {
                consoleAppend = GetLogLevelId(temp_stringlevel);
                SetLoggerConsoleAppend(logger, consoleAppend);
            }


            if(!JSON_GetString(loggerConfig, "fileAppend", temp_stringlevel)) {
                fileAppend = GetLogLevelId(temp_stringlevel);
                SetLoggerFileAppend(logger, fileAppend);
            }
            
            if(!JSON_GetString(loggerConfig, "level", temp_stringlevel)) {
                level = GetLogLevelId(temp_stringlevel);
                SetLoggerLevel(logger, level);
            }

        }
        else {
            
            // no se encontró el objeto que contiene la configuración de este logger, demosle una config por defecto

            consoleAppend = LOGGER_DEFAULT_CONSOLE_APPEND;
            fileAppend = LOGGER_DEFAULT_FILE_APPEND;
            level = LOGGER_DEFAULT_LEVEL;

        }
        
    
        
        #if LOGGER_MYSQL_CONFIG_DELAY > 0
        // cargar configuracion mysql
        SetTimerEx("t_LoadLoggerMysqlConfig", LOGGER_MYSQL_CONFIG_DELAY, false, "iisr", _:logger, _:loggerConfig, filepath, _:handle);
        #else
        t_LoadLoggerMysqlConfig(logger, loggerConfig, handle);
        #endif
        
    }

    return 1;
}


// la config mysql se carga con delay para dar tiempo a la base de datos de cargar
// ya que hay logs que podrían querer enviarse antes de `mysql_connect` cuando aún la base de datos no fue conectada al sv
forward t_LoadLoggerMysqlConfig(Logger:logger, Node:loggerConfig, const filepath[], MySQL:handle);
public t_LoadLoggerMysqlConfig(Logger:logger, Node:loggerConfig, const filepath[], MySQL:handle)
{
    if(!ezmysql_isvalid(handle)) {
        new delay;
        #if LOGGER_MYSQL_CONFIG_DELAY > 0
            delay = LOGGER_MYSQL_CONFIG_DELAY
        #else
            delay = 1000;
        #endif

        SetTimerEx("t_LoadLoggerMysqlConfig", delay, false, "iisr", _:logger, _:loggerConfig, filepath, _:handle);
        return;
    }



    new
        loggerName[32]
    ;

    GetLoggerName(logger, loggerName);


    new
        LogLevel:dbAppend = LOG_OFF,
        bool:baseSchema,
        tableName[32],
        temp_stringlevel[32]
    ;
    if(!JSON_GetString(loggerConfig, "dbAppend", temp_stringlevel)) {
        dbAppend = GetLogLevelId(temp_stringlevel);
    }
    else {
        dbAppend = LOGGER_DEFAULT_DATABASE_APPEND;
    }

    
    if(dbAppend != LOG_OFF) {


        if(JSON_GetBool(loggerConfig, "baseSchema", baseSchema)) {
            baseSchema = false;
            JSON_SetBool(loggerConfig, "baseSchema", baseSchema);
        }
        
        if(JSON_GetString(loggerConfig, "tableName", tableName)) {
            format(tableName, sizeof(tableName), "%s", loggerName);
            JSON_SetString(loggerConfig, "tableName", tableName);
        }
        else {
            SetLoggerDatabaseAppend(
                .logger = logger,
                .dbAppend = dbAppend,
                .handle = handle,
                .table = tableName,
                .baseSchema = baseSchema
            );



            new Node:arrayColumns, arrayLength;
            if(JSON_GetArray(loggerConfig, "columns", arrayColumns)) {
                arrayColumns = JSON_Array(); // todo: generar un array con columnas por defecto
                JSON_SetObject(loggerConfig, "columns", arrayColumns);
            }


            JSON_ArrayLength(arrayColumns, arrayLength);
            
            new columnIndex = -1, Node:columnOutput;
            while(!JSON_ArrayIterate(arrayColumns, columnIndex, columnOutput)) {
                new columnName[MAX_LOGGER_DB_COLUMN_LEN], typeData[64];
                JSON_GetString(columnOutput, "name", columnName);
                JSON_GetString(columnOutput, "typeData", typeData);
                CreateLoggerDatabaseColumn(logger, columnName, typeData);
            }
        }




    }


    CallRemoteFunction("OnLoggerConfigLoaded", "ii", _:logger, _:loggerConfig);
    
    JSON_SetObject(s_LoggerConfig, loggerName, loggerConfig);
    JSON_SaveFile(filepath, s_LoggerConfig, true);

    JSON_Cleanup(loggerConfig);
}