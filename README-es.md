easylogs is a **sa-mp** logger that simplifies, abstracts and expands logging into PAWN using handlers `Logger` handlers.

# usage:

First, include the library after `a_samp`.

```pawn
#include <a_samp>
#include <logger>
```

Create a logger:
```pawn
new Logger:logger = GetLogger("server");
```

`GetLogger` has optional parameters that allow you to customize whether you want to export your logs into `scriptfiles/logs` (and/or print them into server_log.txt) and a LogLevel parameter that will be `LOG_WARN` by default, you can change it later.

Create a logger that does not export logs and uses a custom LogLevel:
```pawn
new Logger:logger = GetLogger("mysql", false, LOG_ERROR); // prints only to the console
```

Logger instantiated, you can use it with `sendlog` and add your data fields with `logger_int, logger_float, logger_string`:

```pawn
new Logger:g_PlayerLogger = GetLogger("players", false, LOG_ERROR);

...

public OnPlayerConnect(playerid)
{
    new name[MAX_PLAYER_NAME], ip[32], version[32], ping = GetPlayerPing(playerid), Float:packetloss = NetStats_GetPacketLoss(playerid);
    GetPlayerName(playerid, name);
    GetPlayerIp(playerid, ip);
    GetPlayerVersion(playerid, version);

    sendlog(g_PlayerLogger, LOG_WARN, "player connection",
        logger_string("username", name),
        logger_string("ip", ip),
        logger_string("version", version)
        logger_int("ping", ping),
        logger_float("packetloss", packetloss)
    );
}

```

But this log wont show, because the logger was defined with a ERROR LogLevel.
This library allows you to change a loggers LogLevel on the runtime:

```pawn
SetLoggerLogLevel(g_PlayerLogger, LOG_ALL);
```


# Defines

You can config easylogs before including it, so these defines would be overwritten:

```pawn
#define MAX_LOGGER (10) // default: 100
#define MAX_LOGGER_FIELD_LENGTH (256) // default: 128
#include <logger>
```

For more information about easylogs configuration defines, see docs/config.md.


# LoggerField

This logger is heavily inspired by Southclaw's, and it borrows the whole idea of LoggerFields.
You can easily instantiate and use your own LoggerFields as long as they return a string.
They should follow a `key=value` format, or even you could insert multiple key-value pairs in one LoggerField if you wanted to.


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

This is a special module that does all the work needed to export logs to a MySQL database, one column per LoggerField.
You are going to need this config in order to use it:

```pawn


#include <a_mysql>

#define LOGGER_USE_MYSQL // `logger.inc` needs this definition to know `logger_mysql.inc` will be included
#define LOGGER_USE_CALLBACKS // activate logger callbacks
#define LOGGER_USE_INIT_CALLBACKS // activate OnLoggerInit
#define LOGGER_USE_LOG_CALLBACKS // activate OnLoggerLog
#include <logger>
#include <logger_mysql>

```



And now you can:

```pawn

new Logger:g_log_Connection = INVALID_LOGGER_ID;

public OnGameModeInit()
{
    handle_db = ezmysql_connect("127.0.0.1", ....);

    g_log_Connection = GetLogger("connections", .level = LOG_WARN);
    
    // toggle database append to true
    SetLoggerDatabaseAppend(
        .logger = g_log_Connection,
        .dbAppend = true,
        .handle = handle_db, // handler mysql
        .table = "log_connections", // your output table name
        .baseSchema = true // optionally include 'log_level' and 'msg' columns (default true)
    );

    // create your columns
    CreateLoggerDatabaseColumn(
        .logger = g_log_Connection,
        .column = "playerid", //  column name
        .typeData = "INTEGER DEFAULT 0" // data type along with its default value and/or any other extra info
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

    // then you just send your logs normally...
    sendlog(g_log_Connection, LOG_TRACE, "connection incoming",
        /*
            you just need to pay attention to your fields:  column definitions like above and their reference in `sendlog()` MUST be exactly the same
            you could also try to store, for example, a VARCHAR (string) inside an INTEGER, that would give an error, so again, just pay attention to
            what you do with your LoggerFields if you work with mysql
        */
        logger_int("playerid", playerid),
        logger_string("username", name),
        logger_string("version", version),
        logger_string("ip", ip)
    );
}
```


query result:

```sql
INSERT INTO log_connections (log_level, msg, username, ip, version) VALUES ('warn', 'player connection', 'Neshy', '127.0.0.1', '0.3.7');
```