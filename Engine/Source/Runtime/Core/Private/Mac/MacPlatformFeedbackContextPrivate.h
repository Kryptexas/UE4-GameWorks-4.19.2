// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MacPlatformFeedbackContextPrivate.h: Unreal Mac user interface interaction.
=============================================================================*/

#pragma once

/**
 * Feedback context implementation for Mac.
 */
class FFeedbackContextMac : public FFeedbackContext
{
	/** Context information for warning and error messages */
	FContextSupplier*	Context;

public:
	// Variables.
	int32					SlowTaskCount;

	// Constructor.
	FFeedbackContextMac()
	: FFeedbackContext()
	, SlowTaskCount( 0 )
	, Context( NULL )
	{}
	void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category )
	{
		// if we set the color for warnings or errors, then reset at the end of the function
		// note, we have to set the colors directly without using the standard SET_WARN_COLOR macro
		bool bNeedToResetColor = false;
		if( Verbosity==ELogVerbosity::Error || Verbosity==ELogVerbosity::Warning )
		{
			if( TreatWarningsAsErrors && Verbosity==ELogVerbosity::Warning )
			{
				Verbosity = ELogVerbosity::Error;
			}

			FString Prefix;
			if( Context )
			{
				Prefix = Context->GetContext() + TEXT(" : ");
			}
			FString Format = Prefix + FOutputDevice::FormatLogLine(Verbosity, Category, V);

			if(Verbosity == ELogVerbosity::Error)
			{
				Serialize(COLOR_RED, ELogVerbosity::SetColor, Category); bNeedToResetColor = true;
				// Only store off the message if running a commandlet.
				if ( IsRunningCommandlet() )
				{
					Errors.Add(Format);
				}
			}
			else
			{
				Serialize(COLOR_YELLOW, ELogVerbosity::SetColor, Category); bNeedToResetColor = true;
				// Only store off the message if running a commandlet.
				if ( IsRunningCommandlet() )
				{
					Warnings.Add(Format);
				}
			}
		}

		if( GLogConsole && IsRunningCommandlet() )
			GLogConsole->Serialize( V, Verbosity, Category );
		if( !GLog->IsRedirectingTo( this ) )
			GLog->Serialize( V, Verbosity, Category );

		if (bNeedToResetColor)
		{
			Serialize(COLOR_NONE, ELogVerbosity::SetColor, Category);
		}
	}
	VARARG_BODY( bool, YesNof, const TCHAR*, VARARG_NONE )
	{
		TCHAR TempStr[4096];
		GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr)-1, Fmt, Fmt );
		if( ( GIsClient || GIsEditor ) && ( ( GIsSilent != true ) && ( FApp::IsUnattended() != true ) ) )
		{
			CFStringRef Message = FPlatformString::TCHARToCFString( TempStr );
			CFStringRef Caption = FPlatformString::TCHARToCFString( *NSLOCTEXT( "UnrealEd", "Question", "Question" ).ToString() );
			CFStringRef Yes     = FPlatformString::TCHARToCFString( *NSLOCTEXT( "UnrealEd", "Yes",      "Yes"      ).ToString() );
			CFStringRef No      = FPlatformString::TCHARToCFString( *NSLOCTEXT( "UnrealEd", "No",       "No"       ).ToString() );

			int32 Result = NSRunInformationalAlertPanel( (NSString*)Caption, (NSString*)Message, (NSString*)Yes, (NSString*)No, NULL );

			CFRelease( No );
			CFRelease( Yes );
			CFRelease( Caption );
			CFRelease( Message );

			return Result != 0;
		}
		else
		{
			return false;
		}
	}

	void BeginSlowTask( const FText& Task, bool ShowProgressDialog, bool bShowCancelButton=false )
	{
		GIsSlowTask = ++SlowTaskCount>0;
	}
	void EndSlowTask()
	{
		check(SlowTaskCount>0);
		GIsSlowTask = --SlowTaskCount>0;
	}
	virtual bool StatusUpdate( int32 Numerator, int32 Denominator, const FText& StatusText )
	{
		return true;
	}
	FContextSupplier* GetContext() const
	{
		return Context;
	}
	void SetContext( FContextSupplier* InSupplier )
	{
		Context = InSupplier;
	}
};

