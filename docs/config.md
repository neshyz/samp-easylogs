# easymysql config options

All options that will get discussed in this section are `#define` constants that you can override to customize your logging experience.
Value in parenthesis is the default.

# logger.inc

#### MAX_LOGGERS (100)
Loggers global buffer array size.
I would recommend to custom this to your memory/logging usage needs, in most cases you dont really need 100 active loggers.

#### MAX_LOGGER_NAME_LENGTH (16)
Maximum name length for a logger.
You would maybe want to extend this, but in most cases, a small name will be ok.

#### MAX_LOGGER_FIELD_LENGTH (128)
Maximum length of a single logger field, including key and value.
You will probably run short with the default as I would recommend increasing it to something like 256 or 512.

#### LOGGER_USE_CALLBACKS (undefined)
Defining this means that OnLoggerLog and OnLoggerInit callbacks can be enabled.
You need to enable this in order to use the mysql module.

#### LOGGER_USE_INIT_CALLBACKS (undefined)
Use this with LOGGER_USE_CALLBACKS to enable OnLoggerInit

#### LOGGER_USE_LOG_CALLBACKS (undefined)
Use this with LOGGER_USE_CALLBACKS to enable OnLoggerLog

# logger_mysql.inc

#### LOGGER_USE_MYSQL (undefined)
Use this to enable `logger_mysql.inc` code that is embedded into `logger.inc`.

*Note:* You need to both use *LOGGER_USE_MYSQL* and *#include <logger_mysql>* for mysql module to properly work.
*Note:* This needs to be defined before `logger.inc` and `logger-mysql.inc`.

#### MAX_LOGGER_DB_TABLE_LEN (32)
Maximum length of table name.

#### MAX_LOGGER_DB_COLUMN_LEN (32)
Maximum length of column name.

#### MAX_LOGGER_DB_COLUMNS (16)
Maximum amount of columns per table.

#### MAX_LOGGER_DB_QUERY_LEN (4096)
Maximum length of a complete query.
This is a global buffer variable, but still 4096 is probably too much and you could decrease it with no risk.

#### MAX_LOGGER_DB_BUFFER_LEN (1024)
Maximum length for buffers.
Currently easylogs uses 2 temporary buffers that get merged into one final buffer that completes an INSERT query (the one defined with `MAX_LOGGER_DB_QUERY_LEN`, hence its bigger size).