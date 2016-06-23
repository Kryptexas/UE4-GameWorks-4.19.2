// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FTracePath

#define TRACEPATH_DEBUG 0

class FTracePath
{
public:

	FTracePath()
		: CachedCrc(0U)
	{
	}

	FTracePath(const FTracePath& TracePathIn)
		: CachedCrc(TracePathIn.CachedCrc)
		, Tunnel(TracePathIn.Tunnel)
#if TRACEPATH_DEBUG
		, TraceStack(TracePathIn.TraceStack)
#endif
	{
	}

	FTracePath(TSharedPtr<const class FScriptExecutionTunnelEntry> TunnelNode)
		: CachedCrc(0U)
		, Tunnel(TunnelNode)
	{
	}

	FTracePath(const FTracePath& TracePathIn, TSharedPtr<const class FScriptExecutionTunnelEntry> TunnelNode)
		: CachedCrc(TracePathIn.CachedCrc)
		, Tunnel(TunnelNode)
#if TRACEPATH_DEBUG
		, TraceStack(TracePathIn.TraceStack)
#endif
	{
	}

#if TRACEPATH_DEBUG
	/** Copy operator */
	FTracePath& operator = (const FTracePath& TracePathIn)
	{
		TraceStack = TracePathIn.TraceStack;
		CachedCrc = TracePathIn.CachedCrc;
		Tunnel = TracePathIn.Tunnel;
		return *this;
	}
#endif

	/** Cast to uint32 operator */
	operator const uint32() const { return CachedCrc; }

	/** Add exit pin */
	void AddExitPin(const int32 ExitPinScriptOffset)
	{
#if TRACEPATH_DEBUG
		TraceStack.Add(ExitPinScriptOffset);
#endif
		CachedCrc = FCrc::MemCrc32(&ExitPinScriptOffset, 1, CachedCrc);
	}

	/** Reset Path and Tunnel */
	void Reset()
	{
#if TRACEPATH_DEBUG
		TraceStack.SetNum(0);
#endif
		CachedCrc = 0;
		Tunnel.Reset();
	}

	/** Reset Path Only */
	void ResetPath()
	{
#if TRACEPATH_DEBUG
		TraceStack.SetNum(0);
#endif
		CachedCrc = 0;
	}

	/** Returns the current tunnel */
	TSharedPtr<const class FScriptExecutionTunnelEntry> GetTunnel() const { return Tunnel; }

#if TRACEPATH_DEBUG
	const FString GetPathString() const
	{
		FString PathString(TEXT("<"));
		for (auto Iter : TraceStack)
		{
			PathString = FString::Printf( TEXT( "%s:%i" ), *PathString, Iter);
		}
		PathString += TEXT(">");
		return PathString;
	}
#endif

private:

	/** Cached crc */
	uint32 CachedCrc;
	/** The current tunnel entry point */
	TSharedPtr<const class FScriptExecutionTunnelEntry> Tunnel;

#if TRACEPATH_DEBUG
	/** Contributing pin list - for debugging only */
	TArray<int32> TraceStack;
#endif
};
