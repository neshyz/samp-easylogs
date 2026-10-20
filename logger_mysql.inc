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


// obtener la conexion MySQL de un logger, retorna MYSQL_INVALID_HANDLE si el logger es invalido
forward MySQL:GetLoggerDatabase(Logger:logger);

// activar el guardado en MySQL de un logger desde el LogLevel dbAppend (LOG_OFF lo desactiva), crea la tabla si no existe y reinicia sus columnas registradas
// baseSchema agrega las columnas log_level y msg a cada fila
forward SetLoggerDatabaseAppend(Logger:logger, LogLevel:dbAppend, MySQL:handle = MYSQL_INVALID_HANDLE, const table[] = "", bool:baseSchema = true);

// obtener el LogLevel minimo que se inserta en la base, retorna LOG_OFF si el logger es invalido
forward LogLevel:GetLoggerDatabaseAppend(Logger:logger);

// obtener el nombre de la tabla de un logger, retorna false si el logger es invalido
forward bool:GetLoggerDatabaseTable(Logger:logger, output[], const size = sizeof(output));

// cambiar la tabla donde se insertan los logs, no crea la tabla ni sus columnas
forward SetLoggerDatabaseTable(Logger:logger, const table[]);

// obtener si el logger completa las columnas log_level y msg en cada fila
forward bool:GetLoggerDatabaseBaseSchema(Logger:logger);

// activar o desactivar las columnas log_level y msg, no crea las columnas
forward SetLoggerDatabaseBaseSchema(Logger:logger, bool:value);

// registrar una columna en el logger y crearla en la tabla si no existe, solo los fields con columna registrada se guardan en la base
// retorna false si el logger es invalido o no tiene tabla o conexion configurada
forward bool:CreateLoggerDatabaseColumn(Logger:logger, const column[], const typeData[]);

forward bool:IsLoggerFieldCreated(Logger:logger, const field[]);

static enum E_LOGGER_MYSQL_DATA {
    MySQL:E_LOGGER_DB,
    LogLevel:E_LOGGER_DB_APPEND,
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
    #pragma unused msg
    
    if(level >= GetLoggerDatabaseAppend(logger) && s_InsertedRows > 0) {

        new tableName[MAX_LOGGER_DB_TABLE_LEN];
        GetLoggerDatabaseTable(logger, tableName);

        new levelName[32];
        GetLogLevelName(level, levelName);




        __strreplace("\"", "", s_QueryBuffer_Values);
        __strreplace("\"", "`", s_QueryBuffer_Columns);

        format(
            s_QueryBuffer, sizeof(s_QueryBuffer),
            "INSERT INTO %s (%s) VALUES (%s);", tableName, s_QueryBuffer_Columns, s_QueryBuffer_Values
        );

        mysql_tquery(GetLoggerDatabase(logger), s_QueryBuffer);

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
    // __strreplace(" ", "", field, strlen(field) + 1);

    if(level >= GetLoggerDatabaseAppend(logger)) {
        


        // usar sscanf para convertir "key = "value"" en 2 arrays independientes
        new sscanfFormat[64];
        format(sscanfFormat, sizeof(sscanfFormat), "p<=>e<s[%d]s[%d]>", MAX_LOGGER_FIELD_LENGTH, MAX_LOGGER_FIELD_LENGTH);
        sscanf(field, sscanfFormat, s_ExtractedField);

        __strreplace(" ", "", s_ExtractedField[e_field_name], strlen(s_ExtractedField[e_field_name]) + 1);
        __strreplace("\"", "", s_ExtractedField[e_field_value], strlen(s_ExtractedField[e_field_value]) + 1);
        
        if(!IsLoggerFieldCreated(logger, s_ExtractedField[e_field_name])) {
            return;
        }
        
        new logLevelName[32];
        GetLogLevelName(level, logLevelName);
        
        if(GetLoggerDatabaseBaseSchema(logger) && s_InsertedRows == 0) {




            new fieldFormat[MAX_LOGGER_FIELD_LENGTH];
            format(fieldFormat, sizeof(fieldFormat),
                "%s", logLevelName
            );
            AppendField(logger, "log_level", fieldFormat);


            format(fieldFormat, sizeof(fieldFormat),
                "%s", message
            );
            AppendField(logger, "msg", fieldFormat);
        }
        
        

        

        


        AppendField(logger, s_ExtractedField[e_field_name], s_ExtractedField[e_field_value]);
    }
}


static
    s_TempFields[MAX_LOGGER_DB_BUFFER_LEN],
    s_TempContents[MAX_LOGGER_DB_BUFFER_LEN]
;

static AppendField(Logger:logger, const field[], const content[])
{
    if(s_InsertedRows > 0) {
        strcat(s_QueryBuffer_Values, ", ");
        strcat(s_QueryBuffer_Columns, ", ");
    }
   
    
    mysql_format(GetLoggerDatabase(logger), s_TempFields, sizeof(s_TempFields),
        "%s`%s`", s_QueryBuffer_Columns, field
    );
    
    mysql_format(GetLoggerDatabase(logger), s_TempContents, sizeof(s_TempContents),
        "%s'%e'", s_QueryBuffer_Values, content
    );

    format(s_QueryBuffer_Values, sizeof(s_QueryBuffer_Values), "%s", s_TempContents);
    format(s_QueryBuffer_Columns, sizeof(s_QueryBuffer_Columns), "%s", s_TempFields);

    s_InsertedRows++;

    __strclean(s_TempFields);
    __strclean(s_TempContents);
}

_OnLoggerInit_Mysql(Logger:logger)
{
	#pragma unused logger
	return 1;
}


static bool:IsLoggerFieldCreated(Logger:logger, const field[])
{
    if(!IsValidLogger(logger)) {
        return false;
    }

    new tempstr[MAX_LOGGER_DB_COLUMN_LEN];

    for(new idx; idx < s_LoggerMysql[logger][E_LOGGER_DB_COLUMN_COUNT]; idx++) {

        strunpack(tempstr, s_LoggerColumns[logger][idx]);

        if(strcmp(field, tempstr, true) == 0) {
            return true;
        }
    }

    return false;
}

// deja la configuracion de mysql de un logger apagada, sin validar el logger (usado por ezlogs_init)
_ResetLogger_Mysql(Logger:logger)
{
    s_LoggerMysql[logger][E_LOGGER_DB_APPEND] = LOG_OFF;
    s_LoggerMysql[logger][E_LOGGER_DB] = MYSQL_INVALID_HANDLE;
    s_LoggerMysql[logger][E_LOGGER_DB_COLUMN_COUNT] = 0;
}


stock SetLoggerDatabaseAppend(Logger:logger, LogLevel:dbAppend, MySQL:handle = MYSQL_INVALID_HANDLE, const table[] = "", bool:baseSchema = true)
{
    if(!IsValidLogger(logger)) {
        return;
    }

    s_LoggerMysql[logger][E_LOGGER_DB_APPEND] = dbAppend;
    s_LoggerMysql[logger][E_LOGGER_DB] = handle;
    s_LoggerMysql[logger][E_LOGGER_DB_COLUMN_COUNT] = 0;


    if(dbAppend != LOG_OFF) {
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
    if(!IsValidLogger(logger)) {
        return false;
    }

    return s_LoggerMysql[logger][E_LOGGER_DB_BASE_SCHEMA];
}



stock SetLoggerDatabaseBaseSchema(Logger:logger, bool:value)
{
    if(!IsValidLogger(logger)) {
        return;
    }

    s_LoggerMysql[logger][E_LOGGER_DB_BASE_SCHEMA] = value;
}


stock LogLevel:GetLoggerDatabaseAppend(Logger:logger)
{
    if(!IsValidLogger(logger)) {
        return LOG_OFF;
    }

    return s_LoggerMysql[logger][E_LOGGER_DB_APPEND];
}

stock MySQL:GetLoggerDatabase(Logger:logger)
{
    if(!IsValidLogger(logger)) {
        return MYSQL_INVALID_HANDLE;
    }

    return s_LoggerMysql[logger][E_LOGGER_DB];
}


stock bool:GetLoggerDatabaseTable(Logger:logger, output[], const size = sizeof(output))
{
    if(!IsValidLogger(logger)) {
        return false;
    }

    strunpack(output, s_LoggerMysql[logger][E_LOGGER_DB_TABLE], size);

    return true;
}

stock SetLoggerDatabaseTable(Logger:logger, const table[])
{
    if(!IsValidLogger(logger)) {
        return;
    }

    strpack(s_LoggerMysql[logger][E_LOGGER_DB_TABLE], table);
}

stock bool:CreateLoggerDatabaseColumn(Logger:logger, const column[], const typeData[])
{
    if(!IsValidLogger(logger)) {
        return false;
    }

    if(easylogs_isempty(column)) {
        return false;
    }

    // evitar escribir fuera de s_LoggerColumns cuando se superan MAX_LOGGER_DB_COLUMNS
    // (baseSchema ocupa 2 de esos lugares con log_level y msg)
    new idx = s_LoggerMysql[logger][E_LOGGER_DB_COLUMN_COUNT];
    if(idx >= MAX_LOGGER_DB_COLUMNS) {
        printf("logger_mysql: no se pudo crear la columna \"%s\", se alcanzo el limite MAX_LOGGER_DB_COLUMNS (%d)", column, MAX_LOGGER_DB_COLUMNS);
        return false;
    }

	new tableName[MAX_LOGGER_DB_TABLE_LEN];
	if(!GetLoggerDatabaseTable(logger, tableName)) {
        return false;
    }

    new MySQL:handle = GetLoggerDatabase(logger);
    if(handle == MYSQL_INVALID_HANDLE) {
        return false;
    }

    // registrar la columna solo cuando ya se sabe que se va a crear
    strpack(s_LoggerColumns[logger][idx], column);

	mysql_addcolumn(GetLoggerDatabase(logger), tableName, column, typeData);

    s_LoggerMysql[logger][E_LOGGER_DB_COLUMN_COUNT]++;

    return true;
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