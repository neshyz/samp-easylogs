
#define MAX_LOGGER_DB_TABLE_LEN (32)

forward MySQL:GetLoggerDatabase(Logger:logger);
forward bool:GetLoggerDatabaseAppend(Logger:logger);
forward GetLoggerDatabaseTable(Logger:logger, output[], const size = sizeof(output));
forward SetLoggerDatabaseTable(Logger:logger, const table[]);
forward CreateLoggerDatabaseColumn(Logger:logger, const column[], const typeData[]);

static enum E_LOGGER_MYSQL_DATA {
    MySQL:E_LOGGER_DB,
    bool:E_LOGGER_DB_APPEND,
    E_LOGGER_DB_TABLE[MAX_LOGGER_DB_TABLE_LEN char]
}
static s_LoggerMysql[Logger:MAX_LOGGERS][E_LOGGER_MYSQL_DATA];


static s_QueryBuffer[2048], s_InsertedRows = 0;


_OnLoggerLog_Mysql(Logger:logger, LogLevel:level, const msg[])
{
    if(GetLoggerDatabaseAppend(logger) && s_InsertedRows > 0) {

        new tableName[MAX_LOGGER_DB_TABLE_LEN];
        GetLoggerDatabaseTable(logger, tableName);

        new levelName[32];
        GetLogLevelName(level, levelName);

        mysql_format(GetLoggerDatabase(logger),
            s_QueryBuffer, sizeof(s_QueryBuffer),
            "INSERT INTO %s VALUES (NULL, %s, %s, %s);", tableName, levelName, msg, s_QueryBuffer
        );

        mysql_tquery(GetLoggerDatabase(logger), s_QueryBuffer);

        s_InsertedRows = 0;
    }
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
	MYSQL_CreateTable(GetLoggerDatabase(logger), table);
}

stock CreateLoggerDatabaseColumn(Logger:logger, const column[], const typeData[])
{
	new tableName[32];
	GetLoggerDatabaseTable(logger, tableName);

	MYSQL_CreateColumn(GetLoggerDatabase(logger), tableName, column, typeData);
}


stock LoggerField:loggerdb_float(Float:value)
{
    new Logger:logger = GetActiveLogger();

    if(GetLoggerDatabaseAppend(logger)) {
        mysql_format(GetLoggerDatabase(logger),
            s_QueryBuffer, sizeof(s_QueryBuffer),
            "%s, %f", s_QueryBuffer, value  
        );

        s_InsertedRows++;
    }
}

stock LoggerField:loggerdb_int(value)
{
    new Logger:logger = GetActiveLogger();

    if(GetLoggerDatabaseAppend(logger)) {
        mysql_format(GetLoggerDatabase(logger),
            s_QueryBuffer, sizeof(s_QueryBuffer),
            "%s, %d", s_QueryBuffer, value  
        );

        s_InsertedRows++;
    }
}

stock LoggerField:loggerdb_string(const value[])
{
    new Logger:logger = GetActiveLogger();

    if(GetLoggerDatabaseAppend(logger)) {
        mysql_format(GetLoggerDatabase(logger),
            s_QueryBuffer, sizeof(s_QueryBuffer),
            "%s, '%s'", s_QueryBuffer, value  
        );

        s_InsertedRows++;
    }
}

















// extracto de strlib.inc


#if !defined ispacked
    #define ispacked(%1) \
        ((%1)[0] > 255)
#endif

static stock bool:isempty(const string[]) {
	if (ispacked(string))
		return string{0} == '\0';
	else
		return string[0] == '\0';
}

static stock strexplode(output[][], const input[], const delimiter[] = !",", limit = cellmax, bool:trim = true, bool:ignorecase = false, size1 = sizeof(output), size2 = sizeof(output[])) {
	if (!size1 || !size2) {
		printf("(strexplode) ERROR: size1 = %d, size2 = %d. Can't be 0.", size1, size2);
		
		return 0;
	}
	
	if (isempty(delimiter)) {
		print(!"(strexplode) ERROR: delimiter is empty.");
		
		return 0;
	}
	
	if (trim) {
		new i = -1;
		
		if (ispacked(input)) {
			while (input{++i}) {
				if (input{i} > ' ') {
					i = -1;
					
					break;
				}
			}
		} else {
			while (input[++i]) {
				if (input[i] > ' ') {
					i = -1;
					
					break;
				}
			}
		}
		
		if (i != -1)
			return 0;
	} else if (isempty(input)) {
		return 0;
	}
	
	if (limit == 0) {
		return 0;
	} else if (limit == cellmax) {
		limit = 0;
	}
	
	new
		     pos = 0,
		     next,
		bool:packed = ispacked(input),
		     dlen = strlen(delimiter),
		     count = 0,
		     end
	;
	
	while (pos != -1) {
		++count;
		
		if (limit > 0 && count >= limit) {
			next = -1;
		} else {
			next = strfind(input, delimiter, ignorecase, pos);
		}
		
		end = (next == -1) ? cellmax : next;
		
		if (trim) {
			if (end == cellmax)
				end = strlen(input);
			
			if (packed) {
				while (0 < input{pos} <= ' ') pos++;
				while (end > 0 && input{end - 1} <= ' ') end--;
			} else {
				while (0 < input[pos] <= ' ') pos++;
				while (end > 0 && input[end - 1] <= ' ') end--;
			}
		}
		
		strmid(output[count - 1], input, pos, end, size2);
		
		if (count >= size1 || next == -1 || (limit < 0 && count >= -limit))
			break;
		
		pos = next + dlen;
	}
	
	return count;
}









// utils

static stock MYSQL_CreateTable(MySQL:handle, const table_name[], const info[] = "ID INTEGER AUTO_INCREMENT, PRIMARY KEY(ID)")
{
    new query[300];

    mysql_format(handle, query, sizeof(query),
        "CREATE TABLE IF NOT EXISTS `%s` (\
            %s);", table_name, info
	);
        
    mysql_query(handle, query, true);

    if(mysql_errno(handle) != 0) {

        new errorbuffer[1024];
        mysql_error(errorbuffer);

        printf("no se pudo crear la tabla '%s' con info '%s', error '%s'", table_name, info, errorbuffer);

    }
}




static stock MYSQL_CheckColumnExists(MySQL:handle, const table[], const column_name[])
{
    new query[300];

    mysql_format(handle, query, sizeof(query),
        "SELECT COUNT(*) AS column_exists \
        FROM information_schema.COLUMNS \
        WHERE TABLE_NAME = '%e' \
        AND TABLE_SCHEMA = '%e' \
        AND COLUMN_NAME = '%e';",
        table,
        handle == handle_db ? SELE_DB : SELE_DB"_logs",
        column_name
    );

    mysql_query(handle, query, true);

    new int;
    cache_get_value_name_int(0, "column_exists", int);

    return int;
}

static stock MYSQL_CreateColumn(MySQL:handle, const table[], const column_name[], const data[])
{
    if(MYSQL_CheckColumnExists(handle, table, column_name)) {

        return;
        
    }

    new query[300];
    format(query, sizeof(query),

        "ALTER TABLE `%s` ADD COLUMN `%s` %s;\n",
        table, column_name, data

    );

    mysql_query(handle, query, true);

    if(mysql_errno(handle) != 0) {

        new errorBuffer[1024];
        mysql_error(errorBuffer);

		printf("MYSQL_CreateColumn:  error  %s", errorBuffer);

    }
    cache_delete(cache_save());
}