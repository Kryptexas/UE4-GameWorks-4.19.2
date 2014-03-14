// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FeedbackContextEditor.h: Feedback context tailored to UnrealEd

=============================================================================*/

#pragma once



/**
 * A FFeedbackContext implementation for use in UnrealEd.
 */
class FFeedbackContextEditor : public FFeedbackContext
{
	/** minimum time between progress updates displayed - more frequent updates are ignored */
	static const float UIUpdateGatingTime;

	/** the number of times a caller requested the progress dialog be shown */
	int32				DialogRequestCount;
	/** keeps track of the order in which calls were made requesting the progress dialog */
	TArray<bool>	DialogRequestStack;

	/** Slate slow task widget */
	TWeakPtr<class SWindow> SlowTaskWindow;
	TSharedPtr<class SSlowTaskWidget> SlowTaskWidget;

	/**
	 * StatusMessageStackItem
	 */
	struct StatusMessageStackItem
	{
		/** Status message text */
		FText StatusText;

		/** Progress numerator */
		int32 ProgressNumerator;

		/** Progress denominator */
		int32 ProgressDenominator;

		/** Cached numerator so we can update less frequently */
		int32 SavedNumerator;
		
		/** Cached denominator so we can update less frequently */
		int32 SavedDenominator;

		/** The list time we updated the progress, so we can make sure to update at least once a second */
		double LastUpdateTime;

		StatusMessageStackItem()
		: ProgressNumerator(0), ProgressDenominator(0), SavedNumerator(0), SavedDenominator(0), LastUpdateTime(0.0)
		{
		}
	};

	/** Current status message and progress */
	StatusMessageStackItem StatusMessage;

	/** Stack of status messages and progress values */
	TArray< StatusMessageStackItem > StatusMessageStack;

	/** Special Windows/Widget popup for building */
	TWeakPtr<class SWindow> BuildProgressWindow;
	TSharedPtr<class SBuildProgressWidget> BuildProgressWidget;

	bool HasTaskBeenCancelled;


	/**
	 *  Updates text and value for various progress meters.
	 *
	 *	@param StatusText				New status text
	 *	@param ProgressNumerator		Numerator for the progress meter (its current value).
	 *	@param ProgressDenominitator	Denominiator for the progress meter (its range).
	 */
	void StatusUpdateProgress( const FText& StatusText, int32 ProgressNumerator, int32 ProgressDenominator, bool bUpdateBuildDialog =true );


	/** 
	 *  Update dialog window text and progress value
	 *	@param StatusText				New status text
	 *	@param ProgressNumerator		Numerator for the progress meter (its current value).
	 *	@param ProgressDenominitator	Denominator for the progress meter (its range).
	 *
	 *	@return true by default and false if status update can't be done. 
	 */
	bool ApplyStatusUpdate( const FText& StatusText, int32 ProgressNumerator, int32 ProgressDenominator );

public:
	int32 SlowTaskCount;

	UNREALED_API FFeedbackContextEditor();

	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) OVERRIDE;

	void BeginSlowTask( const FText& Task, bool bShouldShowProgressDialog, bool bShowCancelButton=false );
	void EndSlowTask();
	void SetContext( FContextSupplier* InSupplier );

	/** Whether or not the user has canceled out of this dialog */
	bool ReceivedUserCancel();

	void OnUserCancel();

	virtual bool YesNof( const FText& Question ) OVERRIDE
	{
		return EAppReturnType::Yes == FMessageDialog::Open( EAppMsgType::YesNo, Question );
	}

	/** 
	 * Slow task update status
	 * @param Numerator		New progress numerator
	 * @param Denominator	New progress denominator
	 * @param StatusText	New status message
	 * 
	 * @return true by default and false if status update is not possible
	 */
	virtual bool StatusUpdate( int32 Numerator, int32 Denominator, const FText& StatusText ) OVERRIDE;

	/** Force updating feedback status
	 *
	 * @param Numerator		New progress numerator
	 * @param Denominator	New progress denominator
	 * @param StatusText	New status message
	 * @return true by default and false if status update is not possible
	 *
	 */
	virtual bool StatusForceUpdate( int32 Numerator, int32 Denominator, const FText& StatusText ) OVERRIDE;

	/**
	 * Updates the progress amount without changing the status message text
	 *
	 * @param Numerator		New progress numerator
	 * @param Denominator	New progress denominator
	 */
	virtual void UpdateProgress( int32 Numerator, int32 Denominator );

	/** Pushes the current status message/progress onto the stack so it can be restored later */
	virtual void PushStatus();

	/** Restores the previously pushed status message/progress */
	virtual void PopStatus();

	/** 
	 * Show the Build Progress Window 
	 * @return Handle to the Build Progress Widget created
	 */
	TWeakPtr<class SBuildProgressWidget> ShowBuildProgressWindow();
	
	/** Close the Build Progress Window */
	void CloseBuildProgressWindow();
};
