easylogs es un logger para **sa-mp** que simplifica el logging utilizando handlers y la posibilidad de exportar tus logs en distintos archivos.

# uso:

Primero, importa la librería después de `a_samp`

```pawn
#include <a_samp>
#include <logger>
```

Crear un logger:
```pawn
new Logger:logger = GetLogger("server");
```

`GetLogger` tiene parámetros opcionales que permiten personalizar el uso de archivos externos `scriptfiles/logs` y un LogLevel que por defecto será `LOG_WARN`.

Crear un logger que no exporte archivos y LogLevel personalizado:
```pawn
new Logger:logger = GetLogger("mysql", false, LOG_ERROR); // printeará errores a consola
```

Una vez instanciado el logger, puedes usarlo con `logger_int, logger_float, logger_string`:

```pawn
new Logger:g_PlayerLogger = GetLogger("players", false, LOG_ERROR); // printeará errores a consola

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
    // enviar este log con todos sus campos de información
}

```

Pero el log no se mostrará, porque definimos nuestro logger con un nivel de ERROR. Para esto, la librería permite cambiar el LogLevel de un logger handler en tiempo de ejecución:

```pawn
SetLoggerLogLevel(g_PlayerLogger, LOG_TRACE); // log all
```


# Defines

Puedes configurar la lib antes de incluirla:

```pawn
#define MAX_LOGGER (10) // default: 100
#define MAX_LOGGER_FIELD_LENGTH (32) // default: 64
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