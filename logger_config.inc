#if defined _logger_module_config
    #endinput
#endif

#define _logger_module_config

#if !defined _logger_included
    #error "logger_config.inc es un modulo y debe ser incluido despues de logger.inc"
#endif

#if !defined _json_included
    #tryinclude <json>
    #if !defined _json_included
        #warning "logger_config.inc no pudo incluirse, json.inc no esta instalado y no se pudo encontrar"
        #endinput
    #endif
#endif



/*
    logger_config.inc

    Carga la configuracion de los loggers desde un archivo .json, con un objeto por logger
    cuya clave es el nombre del logger:

    {
        "players": {
            "level": "all",
            "consoleAppend": "error",
            "fileAppend": "info",

            // solo si logger_mysql.inc esta incluido
            "dbAppend": "info",
            "tableName": "log_players",
            "baseSchema": true,
            "columns": [
                { "name": "playerid", "typeData": "INTEGER DEFAULT 0" }
            ]
        }
    }

    Las claves que falten se completan con los LOGGER_DEFAULT_* y se guardan en el archivo,
    de modo que la primera ejecucion genera un archivo con todos los loggers existentes.

    Si se usa logger_mysql.inc, debe incluirse ANTES que este modulo.
*/



// ruta del archivo de configuracion, relativa a la carpeta scriptfiles/
#if !defined LOGGER_DEFAULT_CONFIG_FILE_PATH
    #define LOGGER_DEFAULT_CONFIG_FILE_PATH "logger.json"
#endif

// valores usados cuando una clave no existe en la configuracion de un logger
#if !defined LOGGER_DEFAULT_CONSOLE_APPEND
    #define LOGGER_DEFAULT_CONSOLE_APPEND (LOG_DEBUG)
#endif

#if !defined LOGGER_DEFAULT_FILE_APPEND
    #define LOGGER_DEFAULT_FILE_APPEND (LOG_WARN)
#endif

#if !defined LOGGER_DEFAULT_LEVEL
    #define LOGGER_DEFAULT_LEVEL (LOG_ALL)
#endif

#if !defined LOGGER_DEFAULT_DATABASE_APPEND
    #define LOGGER_DEFAULT_DATABASE_APPEND (LOG_INFO)
#endif

// igual al valor por defecto de SetLoggerDatabaseAppend
#if !defined LOGGER_DEFAULT_BASE_SCHEMA
    #define LOGGER_DEFAULT_BASE_SCHEMA (true)
#endif

// largo maximo del tipo de dato de una columna leida del json, ej: "VARCHAR(64) DEFAULT NULL"
#if !defined MAX_LOGGER_CONFIG_TYPEDATA_LEN
    #define MAX_LOGGER_CONFIG_TYPEDATA_LEN (128)
#endif



// cargar la configuracion de los loggers desde un archivo .json y aplicarla a todos los loggers existentes.
// los loggers creados despues con GetLogger tambien reciben su configuracion automaticamente.
// filepath es relativo a scriptfiles/, ej: "logger.json", "config/logger.json"
// si el archivo no existe se crea con los valores por defecto.
// si el archivo existe pero tiene un json invalido se usan los valores por defecto y NO se sobrescribe.
// retorna false si el archivo no se pudo leer o guardar
forward bool:LoadLoggerConfig(const filepath[] = LOGGER_DEFAULT_CONFIG_FILE_PATH);


#if defined _logger_module_mysql
// indicar la conexion MySQL que se usara para aplicar dbAppend, tableName, baseSchema y columns.
// llamarla despues de conectar a la base de datos, antes o despues de LoadLoggerConfig.
// retorna false si el handle no es una conexion valida de easy-mysql
forward bool:LoadLoggerDatabaseConfig(MySQL:handle);
#endif


// se llama cada vez que se aplica la configuracion a un logger: al cargar el archivo, al crear
// un logger despues de cargarlo y al llamar LoadLoggerDatabaseConfig.
// loggerConfig es el objeto json de ese logger y solo es valido durante el callback;
// las claves que agregues o modifiques en el se guardan en el archivo (util para tu propia config)
forward OnLoggerConfigLoaded(Logger:logger, Node:loggerConfig);



static
    Node:s_LoggerConfig,
    bool:s_LoggerConfigLoaded = false,
    // false cuando el archivo existe pero no se pudo leer, para no pisarlo con los valores por defecto
    bool:s_LoggerConfigWritable = false,
    s_LoggerConfigPath[128]
;

#if defined _logger_module_mysql
static MySQL:s_LoggerConfigHandle = MYSQL_INVALID_HANDLE;
#endif



// funciones internas definidas antes de usarse: al retornar un tag (bool:, LogLevel:)
// usarlas antes de su definicion obliga al compilador a reparsear el archivo

// guardar s_LoggerConfig en el archivo cargado, retorna false si no se pudo
static bool:SaveLoggerConfig()
{
    if(!s_LoggerConfigLoaded || !s_LoggerConfigWritable) {
        return false;
    }

    if(JSON_SaveFile(s_LoggerConfigPath, s_LoggerConfig, true)) {
        printf("logger_config: no se pudo guardar \"%s\"", s_LoggerConfigPath);
        return false;
    }

    return true;
}


// leer un LogLevel guardado como string en la clave `key`.
// si la clave no existe se escribe defaultLevel en la configuracion,
// si el nivel no es valido se avisa por consola y se usa defaultLevel sin modificar el archivo
static LogLevel:ReadLoggerConfigLevel(Node:loggerConfig, const loggerName[], const key[], LogLevel:defaultLevel)
{
    new levelName[16];

    if(JSON_GetString(loggerConfig, key, levelName)) {
        GetLogLevelName(defaultLevel, levelName);
        JSON_SetString(loggerConfig, key, levelName);
        return defaultLevel;
    }

    if(!IsValidLogLevelName(levelName)) {
        new defaultName[16];
        GetLogLevelName(defaultLevel, defaultName);
        printf("logger_config: \"%s.%s\" tiene un nivel invalido (\"%s\"), se usara \"%s\"", loggerName, key, levelName, defaultName);
        return defaultLevel;
    }

    return GetLogLevelId(levelName);
}



stock bool:LoadLoggerConfig(const filepath[] = LOGGER_DEFAULT_CONFIG_FILE_PATH)
{
    if(s_LoggerConfigLoaded) {
        JSON_Cleanup(s_LoggerConfig);
        s_LoggerConfigLoaded = false;
    }

    // json.inc trabaja con rutas relativas a la carpeta del servidor, fexist con rutas relativas a scriptfiles/
    format(s_LoggerConfigPath, sizeof(s_LoggerConfigPath), "scriptfiles/%s", filepath);
    s_LoggerConfigWritable = true;

    if(JSON_ParseFile(s_LoggerConfigPath, s_LoggerConfig)) {
        if(fexist(filepath)) {
            printf("logger_config: \"%s\" no es un json valido, se usaran los valores por defecto y el archivo no se modificara", s_LoggerConfigPath);
            s_LoggerConfigWritable = false;
        }

        // el archivo no existe (o es invalido): empezar con una configuracion vacia
        s_LoggerConfig = JSON_Object();
    }

    // s_LoggerConfig vive mientras la configuracion este cargada, no debe ser liberado automaticamente
    JSON_ToggleGC(s_LoggerConfig, false);
    s_LoggerConfigLoaded = true;

    ApplyAllLoggersConfig();

    return (s_LoggerConfigWritable && SaveLoggerConfig());
}


#if defined _logger_module_mysql
stock bool:LoadLoggerDatabaseConfig(MySQL:handle)
{
    if(!ezmysql_isvalid(handle)) {
        print("logger_config: LoadLoggerDatabaseConfig recibio una conexion MySQL invalida");
        return false;
    }

    s_LoggerConfigHandle = handle;

    // si el archivo aun no se cargo, la config de base de datos se aplicara en LoadLoggerConfig
    if(s_LoggerConfigLoaded) {
        ApplyAllLoggersConfig();
        SaveLoggerConfig();
    }

    return true;
}
#endif



// llamado desde GetLogger (logger.inc) cada vez que se crea un logger
_OnLoggerInit_Config(Logger:logger)
{
    if(!s_LoggerConfigLoaded) {
        return;
    }

    ApplyLoggerConfig(logger);
    SaveLoggerConfig();
}



static ApplyAllLoggersConfig()
{
    for(new Logger:logger; logger < Logger:GetMaxLoggers(); logger++) {
        if(IsValidLogger(logger)) {
            ApplyLoggerConfig(logger);
        }
    }
}


// leer la configuracion de un logger (creandola si no existe), aplicarla y guardarla en s_LoggerConfig
static ApplyLoggerConfig(Logger:logger)
{
    new loggerName[MAX_LOGGER_NAME_LENGTH];
    if(!GetLoggerName(logger, loggerName)) {
        return;
    }

    // JSON_GetObject devuelve una copia del objeto: los cambios se vuelcan con JSON_SetObject al final
    new Node:loggerConfig;
    if(JSON_GetObject(s_LoggerConfig, loggerName, loggerConfig)) {
        loggerConfig = JSON_Object();
    }
    else if(JSON_NodeType(loggerConfig) != JSON_NODE_OBJECT) {
        printf("logger_config: la configuracion de \"%s\" no es un objeto, se reemplazara por los valores por defecto", loggerName);
        JSON_Cleanup(loggerConfig);
        loggerConfig = JSON_Object();
    }

    // json.inc libera un Node: cuando sale del scope de cualquier funcion que lo recibe,
    // desactivar el GC mientras se pasa a otras funciones y al callback
    JSON_ToggleGC(loggerConfig, false);

    SetLoggerLevel(logger, ReadLoggerConfigLevel(loggerConfig, loggerName, "level", LOGGER_DEFAULT_LEVEL));
    SetLoggerConsoleAppend(logger, ReadLoggerConfigLevel(loggerConfig, loggerName, "consoleAppend", LOGGER_DEFAULT_CONSOLE_APPEND));
    SetLoggerFileAppend(logger, ReadLoggerConfigLevel(loggerConfig, loggerName, "fileAppend", LOGGER_DEFAULT_FILE_APPEND));

    #if defined _logger_module_mysql
        if(ezmysql_isvalid(s_LoggerConfigHandle)) {
            ApplyLoggerDatabaseConfig(logger, loggerConfig, loggerName);
        }
    #endif

    // CallLocalFunction y no CallRemoteFunction: los ids de logger son propios de cada script
    CallLocalFunction("OnLoggerConfigLoaded", "ii", _:logger, _:loggerConfig);

    JSON_SetObject(s_LoggerConfig, loggerName, loggerConfig);

    // reactivar el GC para que se libere al salir de esta funcion
    JSON_ToggleGC(loggerConfig, true);
}


#if defined _logger_module_mysql
// aplicar dbAppend, tableName, baseSchema y columns; solo se llama con una conexion valida
static ApplyLoggerDatabaseConfig(Logger:logger, Node:loggerConfig, const loggerName[])
{
    new LogLevel:dbAppend = ReadLoggerConfigLevel(loggerConfig, loggerName, "dbAppend", LOGGER_DEFAULT_DATABASE_APPEND);

    if(dbAppend == LOG_OFF) {
        SetLoggerDatabaseAppend(logger, LOG_OFF);
        return;
    }

    new bool:baseSchema;
    if(JSON_GetBool(loggerConfig, "baseSchema", baseSchema)) {
        baseSchema = LOGGER_DEFAULT_BASE_SCHEMA;
        JSON_SetBool(loggerConfig, "baseSchema", baseSchema);
    }

    // por defecto la tabla se llama igual que el logger
    new tableName[MAX_LOGGER_DB_TABLE_LEN];
    if(JSON_GetString(loggerConfig, "tableName", tableName) || easylogs_isempty(tableName)) {
        format(tableName, sizeof(tableName), "%s", loggerName);
        JSON_SetString(loggerConfig, "tableName", tableName);
    }

    // crea la tabla si no existe y reinicia las columnas registradas del logger
    SetLoggerDatabaseAppend(
        .logger = logger,
        .dbAppend = dbAppend,
        .handle = s_LoggerConfigHandle,
        .table = tableName,
        .baseSchema = baseSchema
    );

    new Node:columns;
    if(JSON_GetArray(loggerConfig, "columns", columns)) {
        // sin columnas propias: dejar el array vacio en el archivo para que sea facil completarlo
        columns = JSON_Array();
        JSON_SetArray(loggerConfig, "columns", columns);
        return;
    }

    new length;
    JSON_ArrayLength(columns, length);

    for(new idx; idx < length; idx++) {
        // declarado dentro del for para que cada columna se libere al terminar su iteracion
        new Node:column;

        if(!JSON_ArrayObject(columns, idx, column)) {
            new
                columnName[MAX_LOGGER_DB_COLUMN_LEN],
                typeData[MAX_LOGGER_CONFIG_TYPEDATA_LEN]
            ;

            if(JSON_GetString(column, "name", columnName) || JSON_GetString(column, "typeData", typeData)
                || easylogs_isempty(columnName) || easylogs_isempty(typeData)) {
                printf("logger_config: la columna %d de \"%s\" necesita \"name\" y \"typeData\", se ignorara", idx, loggerName);
            }
            else if(!CreateLoggerDatabaseColumn(logger, columnName, typeData)) {
                printf("logger_config: no se pudo crear la columna \"%s\" de \"%s\"", columnName, loggerName);
            }
        }
    }
}
#endif
