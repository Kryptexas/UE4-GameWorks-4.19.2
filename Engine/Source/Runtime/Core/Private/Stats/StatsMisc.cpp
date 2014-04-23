// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"

FScopeLogTime::FScopeLogTime( const TCHAR* InName, FTotalTimeAndCount* InGlobal /*= nullptr */ ) 
: StartTime( FPlatformTime::Seconds() )
, Name( InName )
, Cumulative( InGlobal )
{}

FScopeLogTime::~FScopeLogTime()
{
	const double ScopedTime = FPlatformTime::Seconds() - StartTime;
	if( Cumulative )
	{
		Cumulative->Key += ScopedTime;
		Cumulative->Value++;

		const double Average = Cumulative->Key / (double)Cumulative->Value;
		UE_LOG( LogStats, Log, TEXT( "%32s - %6.3f ms - Total %6.2f s / %5u / %6.3f ms" ), *Name, ScopedTime*1000.0f, Cumulative->Key, Cumulative->Value, Average*1000.0f );
	}
	else
	{
		UE_LOG( LogStats, Log, TEXT( "%32s - %6.3f ms" ), *Name, ScopedTime*1000.0f );
	}
}
