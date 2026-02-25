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

#if !defined MAX_LOGGER_DB_QUERY_LEN
    #define MAX_LOGGER_DB_QUERY_LEN (4096)
#endif

#if !defined MAX_LOGGER_DB_BUFFER_LEN
    #define MAX_LOGGER_DB_BUFFER_LEN (1024)
#endif


forward MySQL:GetLoggerDatabase(Logger:logger);

forward SetLoggerDatabaseAppend(Logger:logger, bool:dbAppend, MySQL:handle = MYSQL_INVALID_HANDLE, const table[] = "", bool:baseSchema = true);
forward bool:GetLoggerDatabaseAppend(Logger:logger);
forward GetLoggerDatabaseTable(Logger:logger, output[], const size = sizeof(output));
forward SetLoggerDatabaseTable(Logger:logger, const table[]);
forward bool:GetLoggerDatabaseBaseSchema(Logger:logger);
forward SetLoggerDatabaseBaseSchema(Logger:logger, bool:value);
forward CreateLoggerDatabaseColumn(Logger:logger, const column[], const typeData[]);

static enum E_LOGGER_MYSQL_DATA {
    MySQL:E_LOGGER_DB,
    bool:E_LOGGER_DB_APPEND,
    bool:E_LOGGER_DB_BASE_SCHEMA,
    E_LOGGER_DB_TABLE[MAX_LOGGER_DB_TABLE_LEN char],
    E_LOGGER_DB_COLUMN_COUNT
}
static s_LoggerMysql[Logger:MAX_LOGGERS][E_LOGGER_MYSQL_DATA];
static s_LoggerColumns[Logger:MAX_LOGGERS][MAX_LOGGER_DB_COLUMNS][MAX_LOGGER_DB_COLUMN_LEN char];

static s_QueryBuffer[MAX_LOGGER_DB_QUERY_LEN],
        s_QueryBuffer_Columns[MAX_LOGGER_DB_BUFFER_LEN],
        s_QueryBuffer_Values[MAX_LOGGER_DB_BUFFER_LEN],
        s_InsertedRows = 0;


_OnLoggerLog_Mysql(Logger:logger, LogLevel:level, const msg[])
{
    if(GetLoggerDatabaseAppend(logger) && s_InsertedRows > 0) {

        new tableName[MAX_LOGGER_DB_TABLE_LEN];
        GetLoggerDatabaseTable(logger, tableName);

        new levelName[32];
        GetLogLevelName(level, levelName);




        __strreplace("\"", "'", s_QueryBuffer_Values);
        __strreplace("\"", "`", s_QueryBuffer_Columns);

        mysql_format(GetLoggerDatabase(logger),
            s_QueryBuffer, sizeof(s_QueryBuffer),
            "INSERT INTO %s (%s) VALUES (%s);", tableName, s_QueryBuffer_Columns, s_QueryBuffer_Values
        );

        mysql_tquery(GetLoggerDatabase(logger), s_QueryBuffer);

        printf("final query %s", s_QueryBuffer);
        
        format(s_QueryBuffer, sizeof(s_QueryBuffer), "");
        format(s_QueryBuffer_Columns, sizeof(s_QueryBuffer_Columns), "");
        format(s_QueryBuffer_Values, sizeof(s_QueryBuffer_Values), "");

        s_InsertedRows = 0;
    }
}

static enum e_FieldContent {
    e_field_name[MAX_LOGGER_FIELD_LENGTH],
    e_field_value[MAX_LOGGER_FIELD_LENGTH]
}
static s_ExtractedField[e_FieldContent];

_OnLoggerAppendField_Mysql(Logger:logger, LogLevel:level, const message[], const field[])
{
    __strreplace(" ", "", field, strlen(field) + 1);

    if(GetLoggerDatabaseAppend(logger)) {
        

        // primer iteracion del buffer
        // insertar columnas log_level, msg si el usuario asi lo quiso
        if(s_InsertedRows == 0 && GetLoggerDatabaseBaseSchema(logger)) {
            new logLevelName[32];
            GetLogLevelName(level, logLevelName);

            format(s_QueryBuffer_Columns, sizeof(s_QueryBuffer_Columns), "log_level, msg, ");
            format(s_QueryBuffer_Values, sizeof(s_QueryBuffer_Values), "'%s', '%s', ", logLevelName, message);
            s_LoggerMysql[logger][E_LOGGER_DB_COLUMN_COUNT] += 2;
            s_InsertedRows += 2;
        }
        // iteraciones posteriores, agregar una comma para separar columnas/fields
        else if(s_InsertedRows > 0) {
            strcat(s_QueryBuffer_Values, ", ");
            strcat(s_QueryBuffer_Columns, ", ");
        }


        // usar sscanf para convertir "key = "value"" en 2 arrays independientes
        new sscanfFormat[64];
        format(sscanfFormat, sizeof(sscanfFormat), "p<=>e<s[%d]s[%d]>", MAX_LOGGER_FIELD_LENGTH, MAX_LOGGER_FIELD_LENGTH);
        sscanf(field, sscanfFormat, s_ExtractedField);



        strcat(s_QueryBuffer_Values, s_ExtractedField[e_field_value]);
        strcat(s_QueryBuffer_Columns, s_ExtractedField[e_field_name]);

        s_InsertedRows++;
    }
}


_OnLoggerInit_Mysql(Logger:logger)
{
	#pragma unused logger
	return 1;
}



stock SetLoggerDatabaseAppend(Logger:logger, bool:dbAppend, MySQL:handle = MYSQL_INVALID_HANDLE, const table[] = "", bool:baseSchema = true)
{
    s_LoggerMysql[logger][E_LOGGER_DB_APPEND] = dbAppend;
    s_LoggerMysql[logger][E_LOGGER_DB] = handle;
    s_LoggerMysql[logger][E_LOGGER_DB_COLUMN_COUNT] = 0;


    if(dbAppend) {
        s_LoggerMysql[logger][E_LOGGER_DB_BASE_SCHEMA] = baseSchema;
	    SetLoggerDatabaseTable(logger, table);
        mysql_addtable(GetLoggerDatabase(logger), table);

        

        if(baseSchema) {
            CreateLoggerDatabaseColumn(logger, "log_level", "VARCHAR(16) DEFAULT NULL");
            CreateLoggerDatabaseColumn(logger, "msg", "VARCHAR(512) DEFAULT NULL");
        }

        
    }
    else {
        SetLoggerDatabaseTable(logger, "");
    }

}

stock bool:GetLoggerDatabaseBaseSchema(Logger:logger)
{
    return s_LoggerMysql[logger][E_LOGGER_DB_BASE_SCHEMA];
}



stock SetLoggerDatabaseBaseSchema(Logger:logger, bool:value)
{
    s_LoggerMysql[logger][E_LOGGER_DB_BASE_SCHEMA] = value;
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
}

stock CreateLoggerDatabaseColumn(Logger:logger, const column[], const typeData[])
{
    new idx = s_LoggerMysql[logger][E_LOGGER_DB_COLUMN_COUNT]++;
    strpack(s_LoggerColumns[logger][idx], column);

	new tableName[32];
	GetLoggerDatabaseTable(logger, tableName);

	mysql_addcolumn(GetLoggerDatabase(logger), tableName, column, typeData);
}




















static stock __strclean(output[], const size = sizeof(output))
{
    for(new idx; idx < size; idx++) {
        output[idx] = 0;
    }
}
static stock __strArrayToString(const arr[][], output[], const size = sizeof(output), const arr_size = sizeof(arr))
{
    for(new ind; ind < arr_size; ind++) {

        if(ind == arr_size - 1 || arr_size == 1) {
        
            strcat(output, arr[ind], size);

        }
        else {
            
            strcat(output, arr[ind], size);
            strcat(output, ", ", size);

        }
    }
}


static stock __strreplace(const str1[], const str2[], const output[], const size = sizeof(output))
{
    new bool:anyOcurrencesFound;
    do {
        new idx = strfind(output, str1);
        if(idx == -1) {
            anyOcurrencesFound = false;
            break;
        }

        strdel(output, idx, idx + strlen(str1));
        strins(output, str2, idx, size);
        anyOcurrencesFound = true;
    } while(anyOcurrencesFound);
}