// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FeedbackContextAnsi.h: Unreal Ansi user interface interaction.
=============================================================================*/

#pragma once

/*-----------------------------------------------------------------------------
	FFeedbackContextAnsi.
-----------------------------------------------------------------------------*/

//
// Feedback context.
//
class FFeedbackContextAnsi : public FFeedbackContext
{
public:
	// Variables.
	int32					SlowTaskCount;
	FContextSupplier*	Context;
	FOutputDevice*		AuxOut;

	// Local functions.
	void LocalPrint( const TCHAR* Str )
	{
#if PLATFORM_APPLE 
		printf("%s", TCHAR_TO_ANSI(Str));
#else
		wprintf(TEXT("%s"), Str);
#endif
	}

	// Constructor.
	FFeedbackContextAnsi()
	: FFeedbackContext()
	, SlowTaskCount( 0 )
	, Context( NULL )
	, AuxOut( NULL )
	{}
	void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category )
	{
		if( Verbosity==ELogVerbosity::Error || Verbosity==ELogVerbosity::Warning || Verbosity==ELogVerbosity::Display )
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

			// Only store off the message if running a commandlet.
			if ( IsRunningCommandlet() )
			{
				if(Verbosity == ELogVerbosity::Error)
				{
					Errors.Add(Format);
				}
				else
				{
					Warnings.Add(Format);
				}
			}
			LocalPrint(*Format);
			LocalPrint( TEXT("\n") );
		}
		else if (Verbosity == ELogVerbosity::SetColor)
		{
		}
		if( !GLog->IsRedirectingTo( this ) )
			GLog->Serialize( V, Verbosity, Category );
		if( AuxOut )
			AuxOut->Serialize( V, Verbosity, Category );
		fflush( stdout );
	}

	virtual bool YesNof( const FText& Question ) OVERRIDE
	{
		if( (GIsClient || GIsEditor) )
		{
			LocalPrint( *Question.ToString() );
			LocalPrint( TEXT(" (Y/N): ") );
			if( ( GIsSilent == true ) || ( FApp::IsUnattended() == true ) )
			{
				LocalPrint( TEXT("Y") );
				return true;
			}
			else
			{
				char InputText[256];
				fgets( InputText, sizeof(InputText), stdin );
				return (InputText[0]=='Y' || InputText[0]=='y');
			}
		}
		return true;
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
	virtual bool StatusUpdate(int32 Numerator, int32 Denominator, const FText& StatusText)
	{
		return true;
	}

	void SetContext( FContextSupplier* InSupplier )
	{
		Context = InSupplier;
	}
};

