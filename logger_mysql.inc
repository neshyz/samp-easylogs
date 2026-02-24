#if defined _logger_module_mysql
    #endinput
#endif

#if !defined _logger_included
    #error "logger_mysql.inc es un modulo y debe ser incluido despues de logger.inc"
#endif

#define _logger_module_mysql



#if !defined _easymysql_included

    #tryinclude <easy-mysql>

    #if !defined _easymysql_included

        #tryinclude "../lib/easymysql/easy-mysql.inc"

        #if !defined _easymysql_included
            #error "install easy-mysql ->  neshyz/easy-mysql"
        #endif
    #endif
#endif



#if !defined _INC_SSCANF
    #tryinclude <sscanf2>
    #if !defined _INC_SSCANF
        #error "install sscanf2.inc"
    #endif
#endif


#if !defined MAX_LOGGER_DB_TABLE_LEN
    #define MAX_LOGGER_DB_TABLE_LEN (32)
#endif

#if !defined MAX_LOGGER_DB_COLUMN_LEN
    #define MAX_LOGGER_DB_COLUMN_LEN (32)
#endif


#if !defined MAX_LOGGER_DB_COLUMNS
    #define MAX_LOGGER_DB_COLUMNS (16)
#endif


forward MySQL:GetLoggerDatabase(Logger:logger);

forward SetLoggerDatabaseAppend(Logger:logger, bool:dbAppend, MySQL:handle = MYSQL_INVALID_HANDLE, const table[] = "", const columns[][] = {""}, const columns_size = sizeof(columns));
forward bool:GetLoggerDatabaseAppend(Logger:logger);
forward GetLoggerDatabaseTable(Logger:logger, output[], const size = sizeof(output));
forward SetLoggerDatabaseTable(Logger:logger, const table[]);
forward CreateLoggerDatabaseColumn(Logger:logger, const column[], const typeData[]);

static enum E_LOGGER_MYSQL_DATA {
    MySQL:E_LOGGER_DB,
    bool:E_LOGGER_DB_APPEND,
    E_LOGGER_DB_TABLE[MAX_LOGGER_DB_TABLE_LEN char],
    E_LOGGER_DB_COLUMN_COUNT
}
static s_LoggerMysql[Logger:MAX_LOGGERS][E_LOGGER_MYSQL_DATA];
static s_LoggerColumns[Logger:MAX_LOGGERS][MAX_LOGGER_DB_COLUMNS][MAX_LOGGER_DB_COLUMN_LEN char];

static s_QueryBuffer[4098],
        s_QueryBuffer_Columns[512],
        s_InsertedRows = 0;


_OnLoggerLog_Mysql(Logger:logger, LogLevel:level, const msg[])
{
    if(GetLoggerDatabaseAppend(logger) && s_InsertedRows > 0) {

        new tableName[MAX_LOGGER_DB_TABLE_LEN];
        GetLoggerDatabaseTable(logger, tableName);

        new levelName[32];
        GetLogLevelName(level, levelName);

        mysql_format(GetLoggerDatabase(logger),
            s_QueryBuffer, sizeof(s_QueryBuffer),
            "INSERT INTO %s (%s) VALUES (%s);", tableName, s_QueryBuffer_Columns, s_QueryBuffer
        );

        mysql_tquery(GetLoggerDatabase(logger), s_QueryBuffer);

        printf("final query %s", s_QueryBuffer);

        s_InsertedRows = 0;
    }
}

_OnLoggerAppendField_Mysql(Logger:logger, const field[])
{
    __strArrayToString(s_LoggerColumns[logger], s_QueryBuffer_Columns, .arr_size = s_LoggerMysql[logger][E_LOGGER_DB_COLUMN_COUNT]);


    static enum e_FieldContent {
        e_field_name[MAX_LOGGER_FIELD_LENGTH],
        e_field_value[MAX_LOGGER_FIELD_LENGTH]
    }
    new extractedField[e_FieldContent];

    new sscanfFormat[64];
    format(sscanfFormat, sizeof(sscanfFormat), "p<=>e<s[%d]s[%d]>", MAX_LOGGER_FIELD_LENGTH, MAX_LOGGER_FIELD_LENGTH);
    sscanf(field, sscanfFormat, extractedField);


    if(s_InsertedRows > 0) {
        strcat(s_QueryBuffer, ", ");
    }
    format(s_QueryBuffer, sizeof(s_QueryBuffer),
        "%s%s", s_QueryBuffer, extractedField[e_field_value]
    );

    s_InsertedRows++;
}


_OnLoggerInit_Mysql(Logger:logger)
{
	#pragma unused logger
	return 1;
}



stock SetLoggerDatabaseAppend(Logger:logger, bool:dbAppend, MySQL:handle = MYSQL_INVALID_HANDLE, const table[] = "")
{
    s_LoggerMysql[logger][E_LOGGER_DB_APPEND] = dbAppend;
    s_LoggerMysql[logger][E_LOGGER_DB] = handle;
    
    s_LoggerMysql[logger][E_LOGGER_DB_COLUMN_COUNT] = 0;


	SetLoggerDatabaseTable(logger, table);
}

stock bool:GetLoggerDatabaseAppend(Logger:logger)
{
    return s_LoggerMysql[logger][E_LOGGER_DB_APPEND];
}

stock MySQL:GetLoggerDatabase(Logger:logger)
{
    return s_LoggerMysql[logger][E_LOGGER_DB];
}


stock GetLoggerDatabaseTable(Logger:logger, output[], const size = sizeof(output))
{
    strunpack(output, s_LoggerMysql[logger][E_LOGGER_DB_TABLE], size);
}

stock SetLoggerDatabaseTable(Logger:logger, const table[])
{
    strpack(s_LoggerMysql[logger][E_LOGGER_DB_TABLE], table);
	mysql_addtable(GetLoggerDatabase(logger), table);
}

stock CreateLoggerDatabaseColumn(Logger:logger, const column[], const typeData[])
{
    new idx = s_LoggerMysql[logger][E_LOGGER_DB_COLUMN_COUNT]++;
    strpack(s_LoggerColumns[logger][idx], column);

	new tableName[32];
	GetLoggerDatabaseTable(logger, tableName);

	mysql_addcolumn(GetLoggerDatabase(logger), tableName, column, typeData);
}




















static enum strArrayType {
    str_array_type_comma,
    str_array_type_line_tab,
    str_array_type_just_line
}
static stock __strArrayToString(const arr[][], output[], strArrayType:type = str_array_type_comma, const size = sizeof(output), const arr_size = sizeof(arr))
{
    switch(type) {

        case str_array_type_just_line: {

            for(new ind; ind < arr_size; ind++) {

                if(ind == arr_size - 1 && arr_size > 1) {
                    
                    strcat(output, " ", size);
                    strcat(output, arr[ind], size);

                }
                else if(ind == arr_size - 2 || arr_size == 1) {

                    strcat(output, " ", size);
                    strcat(output, arr[ind], size);
                    strcat(output, "\n", size);

                }
                else {

                    strcat(output, " ", size);
                    strcat(output, arr[ind], size);
                    strcat(output, "\n", size);

                }
            }

        }
        case str_array_type_comma: {

            for(new ind; ind < arr_size; ind++) {

                if(ind == arr_size - 1 && arr_size > 1) {

                    strcat(output, " y ", size);
                    strcat(output, arr[ind], size);

                }
                else if(ind == arr_size - 2 || arr_size == 1) {
                
                    strcat(output, arr[ind], size);

                }
                else {
                    
                    strcat(output, arr[ind], size);
                    strcat(output, ", ", size);

                }
            }
        }

        case str_array_type_line_tab: {

            for(new ind; ind < arr_size; ind++) {

                if(ind == arr_size - 1 && arr_size > 1) {

                    strcat(output, "\t- ", size);
                    strcat(output, arr[ind], size);


                }
                else if(ind == arr_size - 2 || arr_size == 1) {

                    strcat(output, "\t- ", size);
                    strcat(output, arr[ind], size);
                    strcat(output, "\n", size);
                }
                else {

                    strcat(output, "\t- ", size);
                    strcat(output, arr[ind], size);
                    strcat(output, "\n", size);

                }
            }

        }
    }
}