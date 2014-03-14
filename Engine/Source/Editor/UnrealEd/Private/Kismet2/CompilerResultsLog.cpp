// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "CompilerResultsLog.h"
#include "MessageLogModule.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "SourceCodeNavigation.h"

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "Editor.Stats"

const FName FCompilerResultsLog::Name(TEXT("CompilerResultsLog"));

//////////////////////////////////////////////////////////////////////////
// FCompilerResultsLog

/** Update the source backtrack map to note that NewObject was most closely generated/caused by the SourceObject */
void FBacktrackMap::NotifyIntermediateObjectCreation(UObject* NewObject, UObject* SourceObject)
{
	// Chase the source to make sure it's really a top-level ('source code') node
	while (UObject** SourceOfSource = SourceBacktrackMap.Find(SourceObject))
	{
		SourceObject = *SourceOfSource;
	}

	// Record the backtrack link
	SourceBacktrackMap.Add(NewObject, SourceObject);
}

/** Returns the true source object for the passed in object */
UObject* FBacktrackMap::FindSourceObject(UObject* PossiblyDuplicatedObject)
{
	UObject** RemappedIfExisting = SourceBacktrackMap.Find(PossiblyDuplicatedObject);
	if (RemappedIfExisting != NULL)
	{
		return *RemappedIfExisting;
	}
	else
	{
		// Not in the map, must be an unduplicated object
		return PossiblyDuplicatedObject;
	}
}

//////////////////////////////////////////////////////////////////////////
// FCompilerResultsLog

FCompilerResultsLog::FCompilerResultsLog()
	: NumErrors(0)
	, NumWarnings(0)
	, bSilentMode(false)
	, bLogInfoOnly(false)
	, bAnnotateMentionedNodes(true)
{
}

FCompilerResultsLog::~FCompilerResultsLog()
{
}


void FCompilerResultsLog::Register()
{
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessageLogModule.RegisterLogListing(Name, LOCTEXT("CompilerLog", "Compiler Log"));

	FModuleManager::Get().OnModuleCompilerFinished().AddStatic( &FCompilerResultsLog::GetGlobalModuleCompilerDump );
}

void FCompilerResultsLog::Unregister()
{
	FModuleManager::Get().OnModuleCompilerFinished().RemoveStatic( &FCompilerResultsLog::GetGlobalModuleCompilerDump );

	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessageLogModule.UnregisterLogListing(Name);
}

/** Update the source backtrack map to note that NewObject was most closely generated/caused by the SourceObject */
void FCompilerResultsLog::NotifyIntermediateObjectCreation(UObject* NewObject, UObject* SourceObject)
{
	SourceBacktrackMap.NotifyIntermediateObjectCreation(NewObject, SourceObject);
}

/** Returns the true source object for the passed in object */
UObject* FCompilerResultsLog::FindSourceObject(UObject* PossiblyDuplicatedObject)
{
	return SourceBacktrackMap.FindSourceObject(PossiblyDuplicatedObject);
}

/** Create a tokenized message record from a message containing @@ indicating where each UObject* in the ArgPtr list goes and place it in the MessageLog. */
void FCompilerResultsLog::InternalLogMessage(const EMessageSeverity::Type& Severity, const TCHAR* Message, va_list ArgPtr)
{
	UEdGraphNode* OwnerNode = NULL;

	// Create the tokenized message
	TSharedRef<FTokenizedMessage> Line = FTokenizedMessage::Create( Severity );
	Messages.Add(Line);

	const TCHAR* DelimiterStr = TEXT("@@");
	int32 DelimLength = FCString::Strlen(DelimiterStr);

	const TCHAR* Start = Message;
	if (Start && DelimLength)
	{
		while (const TCHAR* At = FCString::Strstr(Start, DelimiterStr))
		{
			// Found a delimiter, create a token from the preceding text
			Line->AddToken( FTextToken::Create( FText::FromString( FString(At - Start, Start) ) ) );
			Start += DelimLength + (At - Start);

			// And read the object and add another token for the object
			UObject* ObjectArgument = va_arg(ArgPtr, UObject*);

			FText ObjText;
			if (ObjectArgument)
			{
				// Remap object references to the source nodes
				ObjectArgument = FindSourceObject(ObjectArgument);

				if (ObjectArgument)
				{
					const UEdGraphNode* Node = Cast<UEdGraphNode>(ObjectArgument);
					const UEdGraphPin* Pin = Cast<UEdGraphPin>(ObjectArgument);

					//Get owner node reference
					OwnerNode = Cast<UEdGraphNode>(ObjectArgument);
					if (Pin)
					{
						OwnerNode = Cast<UEdGraphNode>(Pin->GetOwningNode());
					}

					if (ObjectArgument->GetOutermost() == GetTransientPackage())
					{
						ObjText = LOCTEXT("Transient", "(transient)");					
					}
					else if (Node != NULL)
					{
						ObjText = FText::FromString( Node->GetNodeTitle(ENodeTitleType::ListView) );
					}
					else if (Pin != NULL)
					{
						ObjText = FText::FromString( Pin->PinFriendlyName.IsEmpty() ? Pin->PinName : Pin->PinFriendlyName );
					}
					else
					{
						ObjText = FText::FromString( ObjectArgument->GetName() );
					}
				}
				else
				{
					ObjText = LOCTEXT("None", "(none)");
				}

			}
			else
			{
				ObjText = LOCTEXT("None", "(none)");
			}
			
			TSharedRef<FUObjectToken> Token = FUObjectToken::Create( ObjectArgument, ObjText );
			Token->OnMessageTokenActivated(FOnMessageTokenActivated::CreateRaw(this, &FCompilerResultsLog::OnTokenActivated));
			Line->AddToken( Token );
		}
		Line->AddToken( FTextToken::Create( FText::FromString( Start ) ) );
	}

	va_end(ArgPtr);

	// Register node error/warning.
	if ((OwnerNode != NULL) && bAnnotateMentionedNodes)
	{
		// Determine if this message is the first or more important than the previous one (only showing one error/warning per node for now)
		bool bUpdateMessage = true;
		if (OwnerNode->bHasCompilerMessage)
		{
			// Already has a message, see if we meet or trump the severity
			bUpdateMessage = Severity <= OwnerNode->ErrorType;
		}
		else
		{
			OwnerNode->ErrorMsg.Empty();
		}

		// Update the message
		if (bUpdateMessage)
		{
			OwnerNode->ErrorType = (int32)Severity;
			OwnerNode->bHasCompilerMessage = true;

			FText FullMessage = Line->ToText();

			if (OwnerNode->ErrorMsg.IsEmpty())
			{
				OwnerNode->ErrorMsg = FullMessage.ToString();
			}
			else
			{
				FFormatNamedArguments Args;
				Args.Add( TEXT("PreviousMessage"), FText::FromString( OwnerNode->ErrorMsg ) );
				Args.Add( TEXT("NewMessage"), FullMessage );
				OwnerNode->ErrorMsg = FText::Format( LOCTEXT("AggregateMessagesFormatter", "{PreviousMessage}\n{NewMessage}"), Args ).ToString();
			}
		}
	}

	if( !bSilentMode && (!bLogInfoOnly || (Severity == EMessageSeverity::Info)) )
	{
		if(Severity == EMessageSeverity::CriticalError || Severity == EMessageSeverity::Error)
		{
			UE_LOG(LogBlueprint, Error, TEXT("[compiler] %s"), *Line->ToText().ToString());
		}
		else if(Severity == EMessageSeverity::Warning || Severity == EMessageSeverity::PerformanceWarning)
		{
			UE_LOG(LogBlueprint, Warning, TEXT("[compiler] %s"), *Line->ToText().ToString());
		}
		else
		{
			UE_LOG(LogBlueprint, Log, TEXT("[compiler] %s"), *Line->ToText().ToString());
		}
	}
}

TArray< TSharedRef<FTokenizedMessage> > FCompilerResultsLog::ParseCompilerLogDump(const FString& LogDump)
{
	TArray< TSharedRef<FTokenizedMessage> > Messages;

	TArray< FString > MessageLines;
	LogDump.ParseIntoArray(&MessageLines, TEXT("\n"), false);

	// delete any trailing empty lines
	for (int32 i = MessageLines.Num()-1; i >= 0; --i)
	{
		if (!MessageLines[i].IsEmpty())
		{
			if (i < MessageLines.Num() - 1)
			{
				MessageLines.RemoveAt(i+1, MessageLines.Num() - (i+1));
			}
			break;
		}
	}

	for (int32 i = 0; i < MessageLines.Num(); ++i)
	{
		FString Line = MessageLines[i];
		if (Line.EndsWith(TEXT("\r")))
		{
			Line = Line.LeftChop(1);
		}
		Line = Line.ConvertTabsToSpaces(4).TrimTrailing();

		// handle output line error message if applicable
		// @todo Handle case where there are parenthesis in path names
		FString LeftStr, RightStr;
		FString FullPath, LineNumberString;
		if (Line.Split(TEXT(")"), &LeftStr, &RightStr, ESearchCase::CaseSensitive) &&
			LeftStr.Split(TEXT("("), &FullPath, &LineNumberString, ESearchCase::CaseSensitive) &&
			(FCString::Strtoi(*LineNumberString, NULL, 10) > 0))
		{
			EMessageSeverity::Type Severity = EMessageSeverity::Error;
			FString FullPathTrimmed = FullPath;
			FullPathTrimmed.Trim();
			if (FullPathTrimmed.Len() != FullPath.Len()) // check for leading whitespace
			{
				Severity = EMessageSeverity::Info;
			}

			TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create( Severity );
			if ( Severity == EMessageSeverity::Info )	// add whitespace now
			{
				FString Whitespace = FullPath.Left(FullPath.Len() - FullPathTrimmed.Len());
				Message->AddToken( FTextToken::Create( FText::FromString( Whitespace ) ) );
				FullPath = FullPathTrimmed;
			}

			FString Link = FullPath + TEXT("(") + LineNumberString + TEXT(")");
			Message->AddToken( FTextToken::Create( FText::FromString( Link ) )->OnMessageTokenActivated(FOnMessageTokenActivated::CreateStatic(&FCompilerResultsLog::OnGotoError) ) );
			Message->AddToken( FTextToken::Create( FText::FromString( RightStr ) ) );
			Messages.Add(Message);
		}
		else
		{
			EMessageSeverity::Type Severity = EMessageSeverity::Info;
			if (Line.Contains(TEXT("error LNK"), ESearchCase::CaseSensitive))
			{
				Severity = EMessageSeverity::Error;
			}

			TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create( Severity );
			Message->AddToken( FTextToken::Create( FText::FromString( Line ) ) );
			Messages.Add(Message);
		}
	}

	return Messages;
}

void FCompilerResultsLog::OnGotoError(const TSharedRef<IMessageToken>& Token)
{
	FString FullPath, LineNumberString;
	if (Token->ToText().ToString().Split(TEXT("("), &FullPath, &LineNumberString, ESearchCase::CaseSensitive))
	{
		LineNumberString = LineNumberString.LeftChop(1); // remove right parenthesis
		int32 LineNumber = FCString::Strtoi(*LineNumberString, NULL, 10);

		FSourceCodeNavigation::OpenSourceFile( FullPath, LineNumber );
	}
}

void FCompilerResultsLog::GetGlobalModuleCompilerDump(const FString& LogDump, bool bCompileSucceeded, bool bShowLog)
{
	FMessageLog MessageLog(Name);

	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("TimeStamp"), FText::AsDateTime(FDateTime::Now()));
	MessageLog.NewPage(FText::Format(LOCTEXT("CompilerLogPage", "Compilation - {TimeStamp}"), Arguments));

	if ( bShowLog )
	{
		MessageLog.Open();
	}

	MessageLog.AddMessages(ParseCompilerLogDump(LogDump));
}

void FCompilerResultsLog::OnTokenActivated(const TSharedRef<class IMessageToken>& InTokenRef)
{

}

// Note: Message is not a fprintf string!  It should be preformatted, but can contain @@ to indicate object references, which are the varargs
void FCompilerResultsLog::Error(const TCHAR* Message, ...)
{
	va_list ArgPtr;
	va_start(ArgPtr, Message);
	ErrorVA(Message, ArgPtr);
}

// Note: Message is not a fprintf string!  It should be preformatted, but can contain @@ to indicate object references, which are the varargs
void FCompilerResultsLog::Warning(const TCHAR* Message, ...)
{
	va_list ArgPtr;
	va_start(ArgPtr, Message);
	WarningVA(Message, ArgPtr);
}

// Note: Message is not a fprintf string!  It should be preformatted, but can contain @@ to indicate object references, which are the varargs
void FCompilerResultsLog::Note(const TCHAR* Message, ...)
{
	va_list ArgPtr;
	va_start(ArgPtr, Message);
	NoteVA(Message, ArgPtr);
}

void FCompilerResultsLog::ErrorVA(const TCHAR* Message, va_list ArgPtr)
{
	++NumErrors;
	InternalLogMessage(EMessageSeverity::Error, Message, ArgPtr);
}

void FCompilerResultsLog::WarningVA(const TCHAR* Message, va_list ArgPtr)
{
	++NumWarnings;
	InternalLogMessage(EMessageSeverity::Warning, Message, ArgPtr);
}

void FCompilerResultsLog::NoteVA(const TCHAR* Message, va_list ArgPtr)
{
	InternalLogMessage(EMessageSeverity::Info, Message, ArgPtr);
}

#undef LOCTEXT_NAMESPACE

#endif	//#if !WITH_EDITOR
