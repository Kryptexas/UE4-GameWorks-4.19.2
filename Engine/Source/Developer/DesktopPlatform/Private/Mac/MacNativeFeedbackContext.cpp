// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DesktopPlatformPrivatePCH.h"
#include "MacNativeFeedbackContext.h"

FMacNativeFeedbackContext::FMacNativeFeedbackContext()
	: FFeedbackContext()
	, Context( NULL )
	, SlowTaskCount( 0 )
	, bShowingConsoleForSlowTask( false )
{
}

FMacNativeFeedbackContext::~FMacNativeFeedbackContext()
{
}

void FMacNativeFeedbackContext::Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category )
{
	if( !GLog->IsRedirectingTo( this ) )
	{
		GLog->Serialize( V, Verbosity, Category );
	}
}

bool FMacNativeFeedbackContext::YesNof(const FText& Text)
{
	return GWarn->YesNof(Text);
}

bool FMacNativeFeedbackContext::ReceivedUserCancel()
{
	return false;
}

void FMacNativeFeedbackContext::BeginSlowTask( const FText& Task, bool bShowProgressDialog, bool bInShowCancelButton )
{
	GIsSlowTask = ++SlowTaskCount>0;

	if(SlowTaskCount > 0 && bShowProgressDialog && GLogConsole != NULL && !GLogConsole->IsShown())
	{
		GLogConsole->Show(true);
		bShowingConsoleForSlowTask = true;
	}
}

void FMacNativeFeedbackContext::EndSlowTask()
{
	check(SlowTaskCount>0);
	GIsSlowTask = --SlowTaskCount>0;

	if(SlowTaskCount == 0 && bShowingConsoleForSlowTask)
	{
		if(GLogConsole != NULL) GLogConsole->Show(false);
		bShowingConsoleForSlowTask = false;
	}
}

bool FMacNativeFeedbackContext::StatusUpdate( int32 Numerator, int32 Denominator, const FText& NewStatus )
{
	return true;
}

bool FMacNativeFeedbackContext::StatusForceUpdate( int32 Numerator, int32 Denominator, const FText& StatusText )
{
	return true;
}

void FMacNativeFeedbackContext::UpdateProgress(int32 Numerator, int32 Denominator)
{
}

FContextSupplier* FMacNativeFeedbackContext::GetContext() const
{
	return Context;
}

void FMacNativeFeedbackContext::SetContext( FContextSupplier* InSupplier )
{
	Context = InSupplier;
}
