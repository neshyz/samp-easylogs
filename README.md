easylogs es un logger para **sa-mp** que simplifica el logging utilizando handlers y la posibilidad de exportar tus logs en distintos archivos.

# uso:

Primero, importa la librería después de `a_samp`

```pawn
#include <a_samp>
#include <logger>
```


Inicializa en OnGameModeInit:

```pawn
public OnGameModeInit()
{
    ezlogs_init();
}
```


Crear un logger:
```pawn
new Logger:logger = GetLogger("server");
```

`GetLogger` tiene parámetros opcionales que permiten personalizar el uso de archivos externos `scriptfiles/logs` y un LogLevel que por defecto será `LOG_WARN`.

Crear un logger que no exporte archivos y LogLevel personalizado:
```pawn
new Logger:logger = GetLogger("mysql", .fileAppend = LOG_OFF, .consoleAppend = LOG_ERROR, .level = LOG_ALL); // printeará errores a consola
```

Una vez instanciado el logger, puedes usarlo con `logger_int, logger_float, logger_string`:

```pawn
new Logger:g_PlayerLogger = GetLogger("players", .fileAppend = LOG_OFF, .consoleAppend = LOG_ERROR, .level = LOG_INFO); // printeará errores a consola

...

public OnPlayerConnect(playerid)
{
    new name[MAX_PLAYER_NAME], ip[32], version[32], ping = GetPlayerPing(playerid), Float:packetloss = NetStats_GetPacketLoss(playerid);
    GetPlayerName(playerid, name);
    GetPlayerIp(playerid, ip);
    GetPlayerVersion(playerid, version);

    sendlog(g_PlayerLogger, LOG_INFO, "player connection",
        logger_string("username", name),
        logger_string("ip", ip),
        logger_string("version", version)
        logger_int("ping", ping),
        logger_float("packetloss", packetloss)
    );
    // enviar este log con todos sus campos de información
}

```

Pero el log no se mostrará, porque definimos nuestro logger con un nivel global de INFO, pero ConsoleAppender con un nivel de ERROR.

Para esto, la librería permite cambiar el LogLevel global de un handler en tiempo de ejecución, así como sus appenders:

```pawn
SetLoggerLogLevel(g_PlayerLogger, LOG_ALL);
SetLoggerConsoleAppend(g_PlayerLogger, LOG_ERROR);
SetLoggerFileAppend(g_PlayerLogger, LOG_DEBUG);
```

## Pero espera... Hay tres "LogLevel" en lugar de uno, por qué?

**fileAppend:**  ajusta el LogLevel del file appender,   LOG_OFF significa que este appender esta apagado.
**consoleAppend:** ajusta el LogLevel del console appender, LOG_OFF significa que este appender esta apagado.
**level:** ajusta el LogLevel global del logger

## Caso hipotético

**level** equivale a `LOG_WARN`, **consoleAppend** equivale a `LOG_ERROR` y **fileAppend** a `LOG_ALL`.
El comportamiento que conseguimos es que todos los mensajes warning se envíen a un archivo y los errores o fatales se muestren en consola.

# Defines

Puedes configurar la lib antes de incluirla:

```pawn
#define MAX_LOGGER (10) // default: 100
#define MAX_LOGGER_FIELD_LENGTH (256) // default: 128
#include <logger>
```


# LoggerField

Este logger está altamente inspirado en el logger de Southclaws y toma este concepto prestado.
Puedes facilmente agregar tus propios logger fields:


```pawn
// logger field de vehículos
stock LoggerField:logger_vehicle(vehId)
{
    new field[MAX_LOGGER_FIELD_LENGTH];
    new Float:vx, Float:vy, Float:vz;
    GetVehiclePos(vehId, vx, vy, vz);

    format(field, sizeof(field),
        "vehicleid=%d, modelid=%d, pos=(%.2f,%.2f,%.2f)",
         vehId, GetVehicleModel(vehId), vx, vy, vz
    );

    return LoggerField:field;
}



```



# logger_mysql

Es un modulo opcional que hace todo el trabajo de exportar logs a tu base de datos MySQL.
Para usarlo, deberas realizar la siguiente configuracion:

```pawn


#include <a_mysql>
// a_mysql debe estar incluido antes que logger y logger_mysql

#define LOGGER_USE_MYSQL // logger.inc necesita este define para incluir algunas porciones de codigo 
#define LOGGER_USE_CALLBACKS // al usar este define (con o sin mysql), se activan los callbacks OnLoggerLog y OnLoggerInit
#define LOGGER_USE_INIT_CALLBACKS // complementario con LOGGER_USE_CALLBACKS, pero nos aseguramos que OnLoggerInit este activo
#define LOGGER_USE_LOG_CALLBACKS // nos aseguramos que OnLoggerLog este activo
#include <logger>
#include <logger_mysql>

```



Ahora ya puedes:

```pawn

new Logger:g_log_Connection = INVALID_LOGGER_ID;

public OnGameModeInit()
{
    handle_db = ezmysql_connect("127.0.0.1", ....); // tenes que usar easy-mysql, que viene incluida con easylogs pero recomiendo bajarla aparte

    g_log_Connection = GetLogger("connections", .fileAppend = LOG_ALL, .consoleAppend = LOG_ERROR, .level = LOG_ALL);
    
    // activar el database appending para un logger
    SetLoggerDatabaseAppend(
        .logger = g_log_Connection,
        .dbAppend = LOG_INFO, // puedes usar la misma funcion para desactivar (LOG_OFF), dejando el resto de parametros en por defecto 
        .handle = handle_db, // handler mysql
        .table = "log_connections", // el nombre de la tabla en la que estaremos exportando
        .baseSchema = true // incluir opcionalmente 'log_level' y 'msg' en la estructura de la tabla
    );

    // crear columnas para la base de datos de un logger
    CreateLoggerDatabaseColumn(
        .logger = g_log_Connection,
        .column = "playerid", //  nombre de la columna
        .typeData = "INTEGER DEFAULT 0" // tipo de dato junto a su default o informacion extra
    );

    CreateLoggerDatabaseColumn(g_log_Connection, "username", "VARCHAR(64) DEFAULT NULL");
    CreateLoggerDatabaseColumn(g_log_Connection, "ip", "VARCHAR(64) DEFAULT NULL");
    CreateLoggerDatabaseColumn(g_log_Connection, "version", "VARCHAR(64) DEFAULT NULL");
}



...

public OnPlayerConnect(playerid)
{
    new name[MAX_PLAYER_NAME], version[16], ip[16];
    GetPlayerName(playerid, name);
    GetPlayerVersion(playerid, version);
    GetPlayerIp(playerid, ip);

    // envia tus logs normalmente, no debes cambiar nada
    sendlog(g_log_Connection, LOG_TRACE, "connection incoming",
        // el nombre de tus fields debe ser exactamente igual al de las columnas para que las inserciones no fallen
        logger_int("playerid", playerid),
        logger_string("username", name),
        logger_string("version", version),
        logger_string("ip", ip),
    );
}
```


query result:

```sql
INSERT INTO log_connections (log_level, msg, username, ip, version) VALUES ('warn', 'player connection', 'Neshy', '127.0.0.1', '0.3.7');
```


# logger_config

Es un modulo opcional que carga la configuracion de los loggers desde un archivo `.json`, para poder cambiar niveles, appenders y la exportacion a MySQL sin recompilar.
Necesita [pawn-json](https://github.com/Southclaws/pawn-json) (`json.inc`).

```pawn
#include <a_mysql>
#include <logger>
#include <logger_mysql> // opcional, siempre antes de logger_config
#include <logger_config>
```

## Uso

```pawn
public OnGameModeInit()
{
    ezlogs_init();

    g_PlayerLogger = GetLogger("players");

    // lee scriptfiles/logger.json (o la ruta que le pases, relativa a scriptfiles/)
    // y lo aplica a todos los loggers existentes
    LoadLoggerConfig();

    // solo con logger_mysql: aplica dbAppend, tableName, baseSchema y columns con esta conexion
    handle_db = ezmysql_connect("127.0.0.1", ....);
    LoadLoggerDatabaseConfig(handle_db);
}
```

- El orden entre `GetLogger`, `LoadLoggerConfig` y `LoadLoggerDatabaseConfig` no importa: los loggers creados despues de cargar el archivo reciben su configuracion al crearse.
- Si el archivo no existe se crea con todos los loggers y sus valores por defecto (`LOGGER_DEFAULT_*`, ver [docs/config.md](docs/config.md)).
- Las claves que falten se completan y se guardan en el archivo.
- Si el archivo existe pero no es un json valido se usan los valores por defecto y el archivo **no** se sobrescribe.
- Un nivel invalido (ej: `"warning"`) se avisa por consola y se usa el valor por defecto. Los niveles no distinguen mayusculas: `all`, `trace`, `debug`, `info`, `warn`, `error`, `fatal`, `off`.

## Formato del archivo

Un objeto por logger, cuya clave es el nombre del logger:

```json
{
    "players": {
        "level": "all",
        "consoleAppend": "error",
        "fileAppend": "info",
        "dbAppend": "info",
        "tableName": "log_players",
        "baseSchema": true,
        "columns": [
            { "name": "playerid", "typeData": "INTEGER DEFAULT 0" },
            { "name": "username", "typeData": "VARCHAR(64) DEFAULT NULL" }
        ]
    }
}
```

| clave | tipo | descripcion |
| --- | --- | --- |
| `level` | string | LogLevel global del logger |
| `consoleAppend` | string | LogLevel del console appender |
| `fileAppend` | string | LogLevel del file appender |
| `dbAppend` | string | LogLevel minimo que se inserta en MySQL, `off` lo desactiva (solo logger_mysql) |
| `tableName` | string | tabla donde se insertan los logs, por defecto el nombre del logger (solo logger_mysql) |
| `baseSchema` | bool | agrega las columnas `log_level` y `msg` (solo logger_mysql) |
| `columns` | array | columnas propias, cada una con `name` y `typeData` (solo logger_mysql) |

## OnLoggerConfigLoaded

Se llama cada vez que se aplica la configuracion a un logger: al cargar el archivo, al crear un logger despues de cargarlo y al llamar `LoadLoggerDatabaseConfig`.
`loggerConfig` solo es valido durante el callback; lo que agregues o modifiques en el se guarda en el archivo, util para guardar tu propia configuracion junto a la del logger:

```pawn
public OnLoggerConfigLoaded(Logger:logger, Node:loggerConfig)
{
    new bool:alertAdmins;
    if(JSON_GetBool(loggerConfig, "alertAdmins", alertAdmins)) {
        JSON_SetBool(loggerConfig, "alertAdmins", false); // no existe: crear la clave con su valor por defecto
    }
    return 1;
}
```
