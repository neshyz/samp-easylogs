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