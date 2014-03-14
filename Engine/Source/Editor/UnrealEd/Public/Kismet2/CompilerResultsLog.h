// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "BlueprintUtilities.h"
#include "TokenizedMessage.h"

/** This class maps from final objects to their original source object, across cloning, autoexpansion, etc... */
class UNREALED_API FBacktrackMap
{
protected:
	// Maps from transient object created during compiling to original 'source code' object
	TMap<UObject*, UObject*> SourceBacktrackMap;
public:
	FBacktrackMap(){}
	virtual ~FBacktrackMap(){}

	/** Update the source backtrack map to note that NewObject was most closely generated/caused by the SourceObject */
	void NotifyIntermediateObjectCreation(UObject* NewObject, UObject* SourceObject);

	/** Returns the true source object for the passed in object */
	UObject* FindSourceObject(UObject* PossiblyDuplicatedObject);
};

/** This class represents a log of compiler output lines (errors, warnings, and information notes), each of which can be a rich tokenized message */
class UNREALED_API FCompilerResultsLog
{
public:
	// List of all tokenized messages
	TArray< TSharedRef<class FTokenizedMessage> > Messages;

	// Number of error messages
	int32 NumErrors;

	// Number of warnings
	int32 NumWarnings;

	// Should we be silent?
	bool bSilentMode;

	// Should we log only Info messages, or all messages?
	bool bLogInfoOnly;

	// Should nodes mentioned in messages be annotated for display with that message?
	bool bAnnotateMentionedNodes;
protected:
	// Maps from transient object created during compiling to original 'source code' object
	FBacktrackMap SourceBacktrackMap;
public:
	FCompilerResultsLog();
	virtual ~FCompilerResultsLog();

	/** Register this log with the MessageLog module */
	static void Register();

	/** Unregister this log from the MessageLog module */
	static void Unregister();

	/** Accessor for the LogName, so it can be opened elsewhere */
	static FName GetLogName(){ return Name; }

	// Note: Message is not a fprintf string!  It should be preformatted, but can contain @@ to indicate object references, which are the varargs
	void Error(const TCHAR* Message, ...);
	void Warning(const TCHAR* Message, ...);
	void Note(const TCHAR* Message, ...);
	void ErrorVA(const TCHAR* Message, va_list ArgPtr);
	void WarningVA(const TCHAR* Message, va_list ArgPtr);
	void NoteVA(const TCHAR* Message, va_list ArgPtr);

	/** Update the source backtrack map to note that NewObject was most closely generated/caused by the SourceObject */
	void NotifyIntermediateObjectCreation(UObject* NewObject, UObject* SourceObject);

	/** Returns the true source object for the passed in object */
	UObject* FindSourceObject(UObject* PossiblyDuplicatedObject);

	/** Returns the true source object for the passed in object; does type checking on the result */
	template <typename T>
	T* FindSourceObjectTypeChecked(UObject* PossiblyDuplicatedObject)
	{
		return CastChecked<T>(FindSourceObject(PossiblyDuplicatedObject));
	}
protected:
	/** Create a tokenized message record from a message containing @@ indicating where each UObject* in the ArgPtr list goes and place it in the MessageLog. */
	void InternalLogMessage(const EMessageSeverity::Type& Severity, const TCHAR* Message, va_list ArgPtr);

	/** Callback when a token is activated */
	void OnTokenActivated(const TSharedRef<class IMessageToken>& InTokenRef);

private:
	/** Parses a compiler log dump to generate tokenized output */
	static TArray< TSharedRef<class FTokenizedMessage> > ParseCompilerLogDump(const FString& LogDump);

	/** Goes to an error given a Message Token */
	static void OnGotoError(const class TSharedRef<IMessageToken>& Token);

	/** Callback function for binding the global compiler dump to open the static compiler log */
	static void GetGlobalModuleCompilerDump(const FString& LogDump, bool bCompileSucceeded, bool bShowLog);

	/** The log's name, for easy re-use */
	static const FName Name;
};
#endif	//#if WITH_EDITOR
