// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FeedbackContext.h: FFeedbackContext
=============================================================================*/

#pragma once

/** A context for displaying modal warning messages. */
class CORE_API FFeedbackContext : public FOutputDevice
{
public:
	
	virtual bool YesNof( const FText& Question ) { return false; }

	virtual void BeginSlowTask( const FText& Task, bool ShowProgressDialog, bool bShowCancelButton=false )=0;
	virtual void EndSlowTask()=0;
	virtual bool ReceivedUserCancel() { return false; };

	/** Slow task update status
	 * @param Numerator		New progress numerator
	 * @param Denominator	New progress denominator
	 * @param StatusText	New status message
	 * @return true by default and false if status update is not possible
	 */
	virtual bool StatusUpdate( int32 Numerator, int32 Denominator, const FText& StatusText ) = 0;

	/** Force updating feedback status
	 * @param Numerator		New progress numerator
	 * @param Denominator	New progress denominator
	 * @param StatusText	New status message
	 * @return true by default and false if status update is not possible
	 */
	virtual bool StatusForceUpdate( int32 Numerator, int32 Denominator, const FText& StatusText ) { return true; } 


	/**
	 * Updates the progress amount without changing the status message text.  Override this in derived classes.
	 *
	 * @param Numerator		New progress numerator
	 * @param Denominator	New progress denominator
	 */
	virtual void UpdateProgress( int32 Numerator, int32 Denominator )
	{
	}

	/** Pushes the current status message/progress onto the stack so it can be restored later.  Override this
	    in derived classes. */
	virtual void PushStatus()
	{
	}

	/** Restores the previously pushed status message/progress.  Override this in derived classes. */
	virtual void PopStatus()
	{
	}

	virtual FContextSupplier* GetContext() const { return NULL; }
	virtual void SetContext( FContextSupplier* InSupplier ) {}

	/** Shows/Closes Special Build Progress dialogs */
	virtual TWeakPtr<class SBuildProgressWidget> ShowBuildProgressWindow() {return TWeakPtr<class SBuildProgressWidget>();}
	virtual void CloseBuildProgressWindow() {}

	TArray<FString> Warnings;
	TArray<FString> Errors;

	bool	TreatWarningsAsErrors;		

	FFeedbackContext() :
		 TreatWarningsAsErrors( 0 )
	{}
};

/**
 * Helper class to display a status update message in the editor.
 */
class FStatusMessageContext
{
public:

	/**
	 * Updates the status message displayed to the user.
	 */
	explicit FStatusMessageContext( const FText& InMessage, bool bAllowNewSlowTask = false )
	{
		bIsBeginSlowTask = false;

		if ( GIsEditor && !IsRunningCommandlet() && IsInGameThread() )
		{
			if ( GIsSlowTask )
			{
				GWarn->PushStatus();
				GWarn->StatusUpdate(-1, -1, InMessage);
			}
			else if ( bAllowNewSlowTask )
			{
				bIsBeginSlowTask = true;
				const bool bShowProgressDialog = true;
				const bool bShowCancelButton = false;
				GWarn->BeginSlowTask( InMessage, bShowProgressDialog, bShowCancelButton );
			}
		}
	}

	/**
	 * Ensures the status context is popped off the stack.
	 */
	~FStatusMessageContext()
	{
		if ( GIsEditor && !IsRunningCommandlet() && IsInGameThread() )
		{
			if ( bIsBeginSlowTask )
			{
				GWarn->EndSlowTask();
			}
			else
			{
				GWarn->PopStatus();
			}
		}
	}

private:
	bool bIsBeginSlowTask;
};


