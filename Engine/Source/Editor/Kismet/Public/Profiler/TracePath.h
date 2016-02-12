// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FTracePath

class FTracePath
{
public:

	FTracePath()
		: CachedCrc(0U)
	{
	}

	FTracePath(const FTracePath& TracePathIn)
		: CachedCrc(TracePathIn.CachedCrc)
	{
	}

	/** Cast to uint32 operator */
	operator const uint32() const { return CachedCrc; }

	/** Add exit pin */
	void AddExitPin(const int32 ExitPinScriptOffset)
	{
		CachedCrc = FCrc::MemCrc32(&ExitPinScriptOffset, 1, CachedCrc);
	}

	/** Reset */
	void Reset()
	{
		CachedCrc = 0;
	}

private:

	/** Cached crc */
	uint32 CachedCrc;

};
