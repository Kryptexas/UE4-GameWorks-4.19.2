// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EventLogger.h: Declares various Slate event logger implementations.
=============================================================================*/

#pragma once


/**
 * Implements an event logger that forwards events to two other event loggers.
 */

class FMultiTargetLogger
	: public IEventLogger
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InLoggerA The first logger to forward events to.
	 * @param InLoggerB The second logger to forward events to.
	 */
	FMultiTargetLogger( const TSharedRef<IEventLogger>& InLoggerA, const TSharedRef<IEventLogger>& InLoggerB )
		: LoggerA(InLoggerA)
		, LoggerB(InLoggerB)
	{ }

public:

	// Begin IEventLogger interface

	virtual FString GetLog( ) const OVERRIDE
	{
		return LoggerA->GetLog() + TEXT("\n") + LoggerB->GetLog();
	}

	virtual void Log(EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget) OVERRIDE
	{
		LoggerA->Log(Event, AdditionalContent, Widget);
		LoggerB->Log(Event, AdditionalContent, Widget);
	}

	virtual void SaveToFile( ) OVERRIDE
	{
		LoggerA->SaveToFile();
		LoggerB->SaveToFile();
	}

	// EndIEventLogger interface

private:

	TSharedRef<IEventLogger> LoggerA;
	TSharedRef<IEventLogger> LoggerB;
};


/**
 * Implements an event logger that logs to a file.
 */
class SLATECORE_API FFileEventLogger
	: public IEventLogger
{
public:

	// Begin IEventLogger interface

	virtual FString GetLog( ) const OVERRIDE;

	virtual void Log( EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget ) OVERRIDE;
	
	virtual void SaveToFile( ) OVERRIDE;

	// EndIEventLogger interface

private:

	// Holds the collection of logged events.
	TArray<FString> LoggedEvents;
};


class SLATECORE_API FConsoleEventLogger
	: public IEventLogger
{
public:

	// Begin IEventLogger interface

	virtual FString GetLog( ) const OVERRIDE;

	virtual void Log( EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget ) OVERRIDE;
	
	virtual void SaveToFile( ) OVERRIDE { }
	
	// End IEventLogger interface

private:
};


/**
 * Implements an event logger that logs events related to assertions and ensures.
 */
class SLATECORE_API FStabilityEventLogger
	: public IEventLogger
{
public:

	// Begin IEventLogger interface
	
	virtual FString GetLog( ) const OVERRIDE;

	virtual void Log( EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget ) OVERRIDE;
	
	virtual void SaveToFile( ) OVERRIDE { }
	
	// End IEventLogger interface

private:

	// Holds the collection of logged events.
	TArray<FString> LoggedEvents;
};
