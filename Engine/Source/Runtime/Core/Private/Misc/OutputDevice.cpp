// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OutputDevice.cpp: Implementation of OutputDevice
=============================================================================*/

#include "CorePrivate.h"
#include "ExceptionHandling.h"
#include "VarargsHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogOutputDevice, Log, All);

const TCHAR* FOutputDevice::VerbosityToString(ELogVerbosity::Type Verbosity)
{
	switch (Verbosity & ELogVerbosity::VerbosityMask)
	{
	case ELogVerbosity::NoLogging:
		return TEXT("NoLogging");
	case ELogVerbosity::Fatal:
		return TEXT("Fatal");
	case ELogVerbosity::Error:
		return TEXT("Error");
	case ELogVerbosity::Warning:
		return TEXT("Warning");
	case ELogVerbosity::Display:
		return TEXT("Display");
	case ELogVerbosity::Log:
		return TEXT("Log");
	case ELogVerbosity::Verbose:
		return TEXT("Verbose");
	case ELogVerbosity::VeryVerbose:
		return TEXT("VeryVerbose");
	}
	return TEXT("UknownVerbosity");
}

FString FOutputDevice::FormatLogLine(ELogVerbosity::Type Verbosity, const class FName& Category, const TCHAR* Message, ELogTimes::Type LogTime)
{
	FString Format;
	switch (LogTime)
	{
		case ELogTimes::SinceGStartTime:
			Format = FString::Printf(TEXT("[%07.2f][%3d]"), FPlatformTime::Seconds() - GStartTime, GFrameCounter % 1000);
			break;

		case ELogTimes::UTC:
			Format = FString::Printf(TEXT("[%s][%3d]"), *FDateTime::UtcNow().ToString(TEXT("%Y.%m.%d-%H.%M.%S:%s")), GFrameCounter % 1000);
			break;

		default:
			break;
	}

	if (Category == NAME_None)
	{
		if (Verbosity != ELogVerbosity::Log)
		{
#if !HACK_HEADER_GENERATOR
			Format += FString(VerbosityToString(Verbosity)) + TEXT(": ");
#endif
		}
	}
	else
	{
		if (Verbosity != ELogVerbosity::Log)
		{
			Format += Category.ToString() + TEXT(":") + VerbosityToString(Verbosity) + TEXT(": ");
		}
		else
		{
			Format += Category.ToString() + TEXT(": ");
		}
	}
	if (Message)
	{
		Format += Message;
	}
	return Format;
}

void FOutputDevice::Log( ELogVerbosity::Type Verbosity, const TCHAR* Str )
{
	Serialize( Str, Verbosity, NAME_None );
}
void FOutputDevice::Log( ELogVerbosity::Type Verbosity, const FString& S )
{
	Serialize( *S, Verbosity, NAME_None );
}
void FOutputDevice::Log( const class FName& Category, ELogVerbosity::Type Verbosity, const TCHAR* Str )
{
	Serialize( Str, Verbosity, Category );
}
void FOutputDevice::Log( const class FName& Category, ELogVerbosity::Type Verbosity, const FString& S )
{
	Serialize( *S, Verbosity, Category );
}
void FOutputDevice::Log( const TCHAR* Str )
{
	FScopedCategoryAndVerbosityOverride::FOverride* TLS = FScopedCategoryAndVerbosityOverride::GetTLSCurrent();
	Serialize( Str, TLS->Verbosity, TLS->Category );
}
void FOutputDevice::Log( const FString& S )
{
	FScopedCategoryAndVerbosityOverride::FOverride* TLS = FScopedCategoryAndVerbosityOverride::GetTLSCurrent();
	Serialize( *S, TLS->Verbosity, TLS->Category );
}
void FOutputDevice::Log( const FText& T )
{
	FScopedCategoryAndVerbosityOverride::FOverride* TLS = FScopedCategoryAndVerbosityOverride::GetTLSCurrent();
	Serialize( *T.ToString(), TLS->Verbosity, TLS->Category );
}
/*-----------------------------------------------------------------------------
	Formatted printing and messages.
-----------------------------------------------------------------------------*/


/** Number of top function calls to hide when dumping the callstack as text. */
#define CALLSTACK_IGNOREDEPTH 2

VARARG_BODY( void, FOutputDevice::CategorizedLogf, const TCHAR*, VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(const class FName& Category) )
{
	GROWABLE_LOGF(Serialize(Buffer, Verbosity, Category))
}
VARARG_BODY( void, FOutputDevice::Logf, const TCHAR*, VARARG_EXTRA(ELogVerbosity::Type Verbosity) )
{
	// call serialize with the final buffer
	GROWABLE_LOGF(Serialize(Buffer, Verbosity, NAME_None))
}
VARARG_BODY( void, FOutputDevice::Logf, const TCHAR*, VARARG_NONE )
{
	FScopedCategoryAndVerbosityOverride::FOverride* TLS = FScopedCategoryAndVerbosityOverride::GetTLSCurrent();
	GROWABLE_LOGF(Serialize(Buffer, TLS->Verbosity, TLS->Category))
}


void FailDebug(const TCHAR* Error, const ANSICHAR* File, int32 Line, const TCHAR* Description)
{
	FPlatformMisc::LowLevelOutputDebugStringf( TEXT("%s(%i): %s\n%s\n"), ANSI_TO_TCHAR(File), Line, Error, Description );
}

#if DO_CHECK || DO_GUARD_SLOW
//
// Failed assertion handler.
//warning: May be called at library startup time.
//
void VARARGS FDebug::AssertFailed( const ANSICHAR* Expr, const ANSICHAR* File, int32 Line, const TCHAR* Format/*=TEXT("")*/, ... )
{
	TCHAR DescriptionString[4096];
	GET_VARARGS( DescriptionString, ARRAY_COUNT(DescriptionString), ARRAY_COUNT(DescriptionString)-1, Format, Format );
	
	if(FPlatformMisc::IsDebuggerPresent())
	{
		TCHAR ErrorString[4096];
		FCString::Sprintf(ErrorString,TEXT("Assertion failed: %s"),ANSI_TO_TCHAR(Expr));

		FailDebug(ErrorString,File,Line,DescriptionString);

		FPlatformMisc::DebugBreak();
	}
	else
	{
		FPlatformMisc::PromptForRemoteDebugging(false);
	}

	// Ignore this assert if we're already forcibly shutting down because of a critical error.
	// Note that appFailAssertFuncDebug() is still called.
	if ( !GIsCriticalError )
	{
		const SIZE_T StackTraceSize = 65535;
		ANSICHAR* StackTrace = (ANSICHAR*) FMemory::SystemMalloc( StackTraceSize );
		if( StackTrace != NULL )
		{
			StackTrace[0] = 0;
			// Walk the stack and dump it to the allocated memory.
			FPlatformStackWalk::StackWalkAndDump( StackTrace, StackTraceSize, CALLSTACK_IGNOREDEPTH );
			UE_LOG(LogOutputDevice, Fatal, TEXT("Assertion failed: %s [File:%s] [Line: %i]\n%s\nStack:\n%s"), ANSI_TO_TCHAR(Expr), ANSI_TO_TCHAR(File), Line, DescriptionString, ANSI_TO_TCHAR(StackTrace) );
			FMemory::SystemFree( StackTrace );
		}
	}
}

/**
 * Called when an 'ensure' assertion fails; gathers stack data and generates and error report.
 *
 * @param	Expr	Code expression ANSI string (#code)
 * @param	File	File name ANSI string (__FILE__)
 * @param	Line	Line number (__LINE__)
 * @param	Msg		Informative error message text
 */
void FDebug::EnsureFailed( const ANSICHAR* Expr, const ANSICHAR* File, int32 Line, const TCHAR* Msg )
{
	// Shipping and Test builds always crash for these asserts
	const bool bShouldCrash = UE_BUILD_SHIPPING != 0 || UE_BUILD_TEST != 0;
	if( bShouldCrash )
	{
		// Just trigger a regular assertion which will crash via GError->Logf()
		FDebug::AssertFailed( Expr, File, Line, Msg );
	}
	else
	{
		// Print initial debug message for this error
		TCHAR ErrorString[4096];
		FCString::Sprintf(ErrorString,TEXT("Ensure condition failed: %s"),ANSI_TO_TCHAR(Expr));
		FailDebug( ErrorString, File, Line, Msg );

		// Is there a debugger attached?  If so we'll just break, otherwise we'll submit an error report.
		if( FPlatformMisc::IsDebuggerPresent() )
		{
			// ensure() assertion failed.  Break into the attached debugger.  Presumedly, this assertion
			// is handled and you can safely "set next statement" and resume execution if you want.
			// NOTE: In Release builds FPlatformMisc::DebugBreak currently initiates a crash
			FPlatformMisc::DebugBreak();
		}
		else
		{
			FPlatformMisc::PromptForRemoteDebugging(true);

			// If we determine that we have not sent a report for this ensure yet, send the report below.
			bool bShouldSendNewReport = false;

			// No debugger attached, so generate a call stack and submit a crash report
			// Walk the stack and dump it to the allocated memory.
			const SIZE_T StackTraceSize = 65535;
			ANSICHAR* StackTrace = (ANSICHAR*) FMemory::SystemMalloc( StackTraceSize );
			if( StackTrace != NULL )
			{
				StackTrace[0] = 0;
				FPlatformStackWalk::StackWalkAndDump( StackTrace, StackTraceSize, CALLSTACK_IGNOREDEPTH );

				// Create a final string that we'll output to the log (and error history buffer)
				TCHAR ErrorMsg[16384];
				FCString::Sprintf( ErrorMsg, TEXT("Ensure condition failed: %s [File:%s] [Line: %i]") LINE_TERMINATOR TEXT("%s") LINE_TERMINATOR TEXT("Stack: "), ANSI_TO_TCHAR(Expr), ANSI_TO_TCHAR(File), Line, Msg );

				// Also append the stack trace
				FCString::Strncat( ErrorMsg, ANSI_TO_TCHAR(StackTrace), ARRAY_COUNT(ErrorMsg) - 1 );
				FMemory::SystemFree( StackTrace );

				// Dump the error and flush the log.
				UE_LOG(LogOutputDevice, Log, TEXT("=== Handled error: ===") LINE_TERMINATOR TEXT("%s"), ErrorMsg);
				GLog->Flush();

				// Submit the error report to the server! (and display a balloon in the system tray)
				{
					// How many unique previous errors we should keep track of
					const uint32 MaxPreviousErrorsToTrack = 4;
					static uint32 StaticPreviousErrorCount = 0;
					if( StaticPreviousErrorCount < MaxPreviousErrorsToTrack )
					{
						// Check to see if we've already reported this error.  No point in blasting the server with
						// the same error over and over again in a single application session.
						bool bHasErrorAlreadyBeenReported = false;

						// Static: Array of previous unique error message CRCs
						static uint32 StaticPreviousErrorCRCs[ MaxPreviousErrorsToTrack ];

						// Compute CRC of error string.  Note that along with the call stack, this includes the message
						// string passed to the macro, so only truly redundant errors will go unreported.  Though it also
						// means you shouldn't pass loop counters to ensureMsgf(), otherwise failures may spam the server!
						const uint32 ErrorStrCRC = FCrc::StrCrc_DEPRECATED( ErrorMsg );

						for( uint32 CurErrorIndex = 0; CurErrorIndex < StaticPreviousErrorCount; ++CurErrorIndex )
						{
							if( StaticPreviousErrorCRCs[ CurErrorIndex ] == ErrorStrCRC )
							{
								// Found it!  This is a redundant error message.
								bHasErrorAlreadyBeenReported = true;
								break;
							}
						}

						// Add the element to the list and bump the count
						StaticPreviousErrorCRCs[ StaticPreviousErrorCount++ ] = ErrorStrCRC;

						if( !bHasErrorAlreadyBeenReported )
						{
							FCoreDelegates::OnHandleSystemEnsure.Broadcast();

							FPlatformMisc::SubmitErrorReport( ErrorMsg, EErrorReportMode::Balloon );

							bShouldSendNewReport = true;
						}
					}
				}
			}
			else
			{
				// If we fail to generate the string to identify the crash we don't know if we should skip sending the report,
				// so we will just send the report anyway.
				bShouldSendNewReport = true;
			}

			if ( bShouldSendNewReport )
			{
#if PLATFORM_DESKTOP
				NewReportEnsure( ErrorString );
#endif
			}
		}
	}
}

#endif // DO_CHECK || DO_GUARD_SLOW

//appLowLevelFatalErrorFunc
void VARARGS FError::LowLevelFatal(const ANSICHAR* File, int32 Line, const TCHAR* Format, ... )
{
	TCHAR DescriptionString[4096];

	GET_VARARGS( DescriptionString, ARRAY_COUNT(DescriptionString), ARRAY_COUNT(DescriptionString)-1, Format, Format );

	FailDebug(TEXT("LowLevelFatalError"),File,Line,DescriptionString);

	if(!FPlatformMisc::IsDebuggerPresent())
	{
		FPlatformMisc::PromptForRemoteDebugging(false);
	}

	FPlatformMisc::DebugBreak();

	GError->Log(DescriptionString);
}

#if DO_CHECK || DO_GUARD_SLOW
bool VARARGS FDebug::EnsureNotFalseFormatted( bool bExpressionResult, const ANSICHAR* Expr, const ANSICHAR* File, int32 Line, const TCHAR* FormattedMsg, ... )
{
	const int32 TempStrSize = 4096;
	TCHAR TempStr[ TempStrSize ];
	GET_VARARGS( TempStr, TempStrSize, TempStrSize - 1, FormattedMsg, FormattedMsg );

	if( bExpressionResult == 0 )
	{
		EnsureFailed( Expr, File, Line, TempStr );
	}
	
	return bExpressionResult;
}
#endif

void FMessageDialog::Debugf( const FText& Message )
{
	if( FApp::IsUnattended() == true )
	{
		GLog->Logf( *Message.ToString() );
	}
	else
	{
		if ( GIsEditor && FCoreDelegates::ModalErrorMessage.IsBound() )
		{
			FCoreDelegates::ModalErrorMessage.Execute( Message, EAppMsgType::Ok );
		}
		else
		{
			FPlatformMisc::MessageBoxExt( EAppMsgType::Ok, *Message.ToString(), *NSLOCTEXT("MessageDialog", "DefaultDebugMessageTitle", "ShowDebugMessagef").ToString() );
		}
	}
}

void FMessageDialog::ShowLastError()
{
	uint32 LastError = FPlatformMisc::GetLastError();

	TCHAR TempStr[MAX_SPRINTF]=TEXT("");
	TCHAR ErrorBuffer[1024];
	FCString::Sprintf( TempStr, TEXT("GetLastError : %d\n\n%s"), LastError, FPlatformMisc::GetSystemErrorMessage(ErrorBuffer, 1024, 0) );
	if( FApp::IsUnattended() == true )
	{
		UE_LOG(LogOutputDevice, Fatal, TempStr);
	}
	else
	{
		FPlatformMisc::MessageBoxExt( EAppMsgType::Ok, TempStr, *NSLOCTEXT("MessageDialog", "DefaultSystemErrorTitle", "System Error").ToString() );
	}
}

EAppReturnType::Type FMessageDialog::Open( EAppMsgType::Type MessageType, const FText& Message )
{
	if( FApp::IsUnattended() == true )
	{
		if (GWarn)
		{
			GWarn->Logf( *Message.ToString() );
		}

		switch(MessageType)
		{
		case EAppMsgType::Ok:
			return EAppReturnType::Ok;
		case EAppMsgType::YesNo:
			return EAppReturnType::No;
		case EAppMsgType::OkCancel:
			return EAppReturnType::Cancel;
		case EAppMsgType::YesNoCancel:
			return EAppReturnType::Cancel;
		case EAppMsgType::CancelRetryContinue:
			return EAppReturnType::Cancel;
		case EAppMsgType::YesNoYesAllNoAll:
			return EAppReturnType::No;
		case EAppMsgType::YesNoYesAllNoAllCancel:
		default:
			return EAppReturnType::Yes;
			break;
		}
	}
	else
	{
		if ( GIsEditor && !IsRunningCommandlet() && FCoreDelegates::ModalErrorMessage.IsBound() )
		{
			return FCoreDelegates::ModalErrorMessage.Execute( Message, MessageType );
		}
		else
		{
			return FPlatformMisc::MessageBoxExt( MessageType, *Message.ToString(), *NSLOCTEXT("MessageDialog", "DefaultMessageTitle", "Message").ToString() );
		}
	}
}

/*-----------------------------------------------------------------------------
	Exceptions.
-----------------------------------------------------------------------------*/

//
// Throw a string exception with a message.
//
VARARG_BODY( void VARARGS, FError::Throwf, const TCHAR*, VARARG_NONE )
{
	static TCHAR TempStr[4096];
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr)-1, Fmt, Fmt );
#if HACK_HEADER_GENERATOR && !PLATFORM_EXCEPTIONS_DISABLED
	throw( TempStr );
#else
	UE_LOG(LogOutputDevice, Error, TEXT("THROW: %s"), TempStr);
#endif
}

// Null output device.
FOutputDeviceNull NullOut;
/** Global output device redirector, can be static as FOutputDeviceRedirector explicitly empties TArray on TearDown */
static FOutputDeviceRedirector LogRedirector;

// Exception thrower.
class FThrowOut : public FOutputDevice
{
public:
	void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) OVERRIDE
	{
#if PLATFORM_EXCEPTIONS_DISABLED
		FPlatformMisc::DebugBreak();
#else
		throw( V );
#endif
	}
} ThrowOut;

CORE_API FOutputDeviceRedirectorBase*	GLog							= &LogRedirector;			/* Regular logging */
CORE_API FOutputDeviceError*			GError							= NULL;						/* Critical errors */
CORE_API FOutputDevice*					GThrow							= &ThrowOut;				/* Exception thrower */

VARARG_BODY( void, FMsg::Logf, const TCHAR*, VARARG_EXTRA(const ANSICHAR* File) VARARG_EXTRA(int32 Line) VARARG_EXTRA(const class FName& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) )
{
#if !NO_LOGGING
	if (Verbosity != ELogVerbosity::Fatal)
	{
		// SetColour is routed to GWarn just like the other verbosities and handled in the 
		// device that does the actual printing.
		switch (Verbosity)
		{
		case ELogVerbosity::Error:
		case ELogVerbosity::Warning:
		case ELogVerbosity::Display:
		case ELogVerbosity::SetColor:
			{
				if (GWarn)
				{
					GROWABLE_LOGF(GWarn->Log(Category, Verbosity, Buffer))
					break;
				}
			}
		default:
			{
				GROWABLE_LOGF(GLog->Log(Category, Verbosity, Buffer))
			}
			break;
		}
	}
	else
	{
		// Flush logs queued by threads when we hit an assert because they will not make it to the log otherwise
		GLog->PanicFlushThreadedLogs();

		TCHAR DescriptionString[8192]; // Increased from 4096 to fix crashes in the renderthread without autoreporter

		GET_VARARGS( DescriptionString, ARRAY_COUNT(DescriptionString), ARRAY_COUNT(DescriptionString)-1, Fmt, Fmt );

		FailDebug(TEXT("Fatal error:"),File,Line,DescriptionString);
		if(!FPlatformMisc::IsDebuggerPresent())
		{
			FPlatformMisc::PromptForRemoteDebugging(false);
		}

		FPlatformMisc::DebugBreak();

		GError->Log(Category, Verbosity, DescriptionString);
	}
#endif
}

/** Sends a formatted message to a remote tool. */
void VARARGS FMsg::SendNotificationStringf( const TCHAR *Fmt, ... )
{
	GROWABLE_LOGF(SendNotificationString(Buffer));
}

void FMsg::SendNotificationString( const TCHAR* Message )
{
	FPlatformMisc::LowLevelOutputDebugString(Message);
}
