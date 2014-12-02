#include "SQLiteSupportPrivatePCH.h"
#include "SQLiteDatabaseConnection.h"
#include "sqlite3.h"

bool FSQLiteDatabase::Execute(const TCHAR* CommandString, FSQLiteResultSet*& RecordSet)
{
	
	//Compile the statement/query
	sqlite3_stmt* PreparedStatement;
	int32 PrepareStatus = sqlite3_prepare16_v2(DbHandle, CommandString, -1, &PreparedStatement, NULL);
	if (PrepareStatus == SQLITE_OK)
	{
		//Initialize records from compiled query
        RecordSet = new FSQLiteResultSet(PreparedStatement);
		return true;
	}
	else
	{
		RecordSet = NULL;
		return false;
	}
}


bool FSQLiteDatabase::Execute(const TCHAR* CommandString)
{
	//Compile the statement/query
	sqlite3_stmt* PreparedStatement;
	int32 PrepareStatus = sqlite3_prepare16_v2(DbHandle, CommandString, -1, &PreparedStatement, NULL);
	if (PrepareStatus == SQLITE_OK)
	{
		int32 StepStatus = SQLITE_ERROR;
		//Execute the statement until it is done or we get an error
		do
		{
			StepStatus = sqlite3_step(PreparedStatement);
		} while ((StepStatus != SQLITE_ERROR) && (StepStatus != SQLITE_CONSTRAINT) && (StepStatus != SQLITE_DONE));
		
		//Did we make it all the way through the query without an error?
		if (StepStatus == SQLITE_DONE)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool FSQLiteDatabase::Execute(const TCHAR* CommandString, FDataBaseRecordSet*& RecordSet)
{
	return Execute(CommandString, (FSQLiteResultSet*&)RecordSet);
}

void FSQLiteDatabase::Close()
{
	if (DbHandle)
	{
		sqlite3_close(DbHandle);
	}
}



bool FSQLiteDatabase::Open(const TCHAR* ConnectionString, const TCHAR* RemoteConnectionIP, const TCHAR* RemoteConnectionStringOverride)
{
	if (DbHandle)
	{
		return false;
	}

	int32 Result = sqlite3_open16(ConnectionString, &DbHandle);
	return Result == SQLITE_OK;
}

FString FSQLiteDatabase::GetLastError()
{
	TCHAR* ErrorString = NULL;
	ErrorString = (TCHAR*) sqlite3_errmsg16(DbHandle);
	if (ErrorString)
	{
		return FString(ErrorString);
	}
	else
	{
		return FString();
	}
}
