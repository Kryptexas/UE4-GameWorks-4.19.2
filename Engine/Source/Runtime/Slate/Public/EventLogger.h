// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once



/** The kind of UI event that Slate has logged. */
namespace EEventLog
{
	enum Type
	{
		MouseMove,
		MouseEnter,
		MouseLeave,
		MouseButtonDown,
		MouseButtonUp,
		MouseButtonDoubleClick,
		MouseWheel,
		DragDetected,
		DragEnter,
		DragLeave,
		DragOver,
		DragDrop,
		DropMessage,
		KeyDown,
		KeyUp,
		KeyChar,
		UICommand,
		BeginTransaction,
		EndTransaction,
		TouchGesture,
		Other,
	};

	inline FString ToString(EEventLog::Type Event)
	{
		switch (Event)
		{
		case MouseMove:					return FString("MouseMove");
		case MouseEnter:				return FString("MouseEnter");
		case MouseLeave:				return FString("MouseLeave");
		case MouseButtonDown:			return FString("MouseButtonDown");
		case MouseButtonUp:				return FString("MouseButtonUp");
		case MouseButtonDoubleClick:	return FString("MouseButtonDoubleClick");
		case MouseWheel:				return FString("MouseWheel");
		case DragDetected:				return FString("DragDetected");
		case DragEnter:					return FString("DragEnter");
		case DragLeave:					return FString("DragLeave");
		case DragOver:					return FString("DragOver");
		case DragDrop:					return FString("DragDrop");
		case DropMessage:				return FString("DropMessage");
		case KeyDown:					return FString("KeyDown");
		case KeyUp:						return FString("KeyUp");
		case KeyChar:					return FString("KeyChar");
		case UICommand:					return FString("UICommand");
		case BeginTransaction:			return FString("BeginTransaction");
		case EndTransaction:			return FString("EndTransaction");
		case TouchGesture:				return FString("TouchGesture");
		case Other:						return FString("Other");
		default:						return FString("Unknown");
		}
	}
}



class IEventLogger
{
public:
	virtual ~IEventLogger(){}
	virtual void Log(EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget) = 0;
	virtual void SaveToFile() = 0;
	virtual FString GetLog() const = 0;
};

class FMultiTargetLogger : public IEventLogger
{
public:
	FMultiTargetLogger( const TSharedRef<IEventLogger>& InLoggerA, const TSharedRef<IEventLogger>& InLoggerB )
	: LoggerA( InLoggerA )
	, LoggerB( InLoggerB )
	{
	}

	virtual void Log(EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget) OVERRIDE
	{
		LoggerA->Log( Event, AdditionalContent, Widget );
		LoggerB->Log( Event, AdditionalContent, Widget );
	}

	virtual void SaveToFile() OVERRIDE
	{
		LoggerA->SaveToFile();
		LoggerB->SaveToFile();
	}

	virtual FString GetLog() const OVERRIDE
	{
		return LoggerA->GetLog() + TEXT("\n") + LoggerB->GetLog();
	}

private:
	TSharedRef<IEventLogger> LoggerA;
	TSharedRef<IEventLogger> LoggerB;
};

class FFileEventLogger : public IEventLogger
{
public:
	virtual void Log(EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget) OVERRIDE;
	virtual void SaveToFile() OVERRIDE;
	virtual FString GetLog() const OVERRIDE;

private:
	TArray<FString> LoggedEvents;
};

class FConsoleEventLogger : public IEventLogger
{
public:
	virtual void Log(EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget) OVERRIDE;
	virtual void SaveToFile() OVERRIDE;
	virtual FString GetLog() const OVERRIDE;

private:
};

/** Logs events to be shown whenever an assert or ensure is hit */
class SLATE_API FStabilityEventLogger : public IEventLogger
{
public:
	virtual void Log(EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget) OVERRIDE;
	virtual void SaveToFile() OVERRIDE;
	virtual FString GetLog() const OVERRIDE;

private:
	TArray<FString> LoggedEvents;
};
