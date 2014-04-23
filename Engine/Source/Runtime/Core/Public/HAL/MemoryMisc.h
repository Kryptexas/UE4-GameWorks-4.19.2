// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Holds generic memory stats, internally implemented as a map. */
struct FGenericMemoryStats
{
	void Add( const FName StatName, const SIZE_T StatValue )
	{
		Data.Add( StatName, StatValue );
	}

	TMap<FName,SIZE_T> Data;
};