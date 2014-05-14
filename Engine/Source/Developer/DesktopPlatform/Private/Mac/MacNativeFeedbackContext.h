// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MacNativeFeedbackContext.h: Unreal Mac user interface interaction.
=============================================================================*/

#pragma once

/**
 * Feedback context implementation for Mac.
 */
class FMacNativeFeedbackContext : public FFeedbackContext
{
public:
	// Constructor.
	FMacNativeFeedbackContext();
	virtual ~FMacNativeFeedbackContext();

	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) OVERRIDE;

	virtual bool YesNof(const FText& Text) OVERRIDE;

	virtual bool ReceivedUserCancel() OVERRIDE;
	virtual void BeginSlowTask( const FText& Task, bool bShowProgressDialog, bool bInShowCancelButton=false ) OVERRIDE;
	virtual void EndSlowTask() OVERRIDE;

	virtual bool StatusUpdate( int32 Numerator, int32 Denominator, const FText& NewStatus ) OVERRIDE;
	virtual bool StatusForceUpdate( int32 Numerator, int32 Denominator, const FText& StatusText ) OVERRIDE;
	virtual void UpdateProgress(int32 Numerator, int32 Denominator) OVERRIDE;

	FContextSupplier* GetContext() const;
	void SetContext( FContextSupplier* InSupplier );

private:
	FContextSupplier* Context;
	int32 SlowTaskCount;
	bool bShowingConsoleForSlowTask;
};
