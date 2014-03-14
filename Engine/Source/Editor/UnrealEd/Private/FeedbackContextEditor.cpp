// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "FeedbackContextEditor.h"
#include "Dialogs/SBuildProgress.h"
#include "Editor/MainFrame/Public/MainFrame.h"

/** Called to cancel the slow task activity */
DECLARE_DELEGATE( FOnCancelClickedDelegate );


/**
 * Simple "slow task" widget
 */
class SSlowTaskWidget : public SBorder
{
public:
	SLATE_BEGIN_ARGS( SSlowTaskWidget )	{ }
		/** Called to when an asset is clicked */
		SLATE_EVENT( FOnCancelClickedDelegate, OnCancelClickedDelegate )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs )
	{
		OnCancelClickedDelegate = InArgs._OnCancelClickedDelegate;

		TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 14.0f, 4.0f, 14.0f, 10.0f )
			[
				SNew( STextBlock )
					.Text( this, &SSlowTaskWidget::OnGetProgressText )
					.ShadowOffset( FVector2D( 1.0f, 1.0f ) )
			];

		if ( OnCancelClickedDelegate.IsBound() )
		{
			VerticalBox->AddSlot()
				.AutoHeight()
				.Padding(10.0f, 7.0f)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.Padding(5,0,0,0)
					.FillWidth(0.8f)
					[
						SNew(SProgressBar)
						.Percent( this, &SSlowTaskWidget::GetProgressFraction )
					]
					+SHorizontalBox::Slot()
					.Padding(5,0,0,0)
					.FillWidth(0.2f)
					[
						SNew(SButton)
						.Text( NSLOCTEXT("FeedbackContextProgress", "Cancel", "Cancel") )
						.HAlign(EHorizontalAlignment::HAlign_Center)
						.OnClicked(this, &SSlowTaskWidget::OnCancel)
					]
				];
		}
		else
		{
			VerticalBox->AddSlot()
				.AutoHeight()
				.Padding(10.0f, 7.0f)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.Padding(5,0,0,0)
					.FillWidth(1.0f)
					[
						SNew(SProgressBar)
						.Percent( this, &SSlowTaskWidget::GetProgressFraction )
					]
				];
		}

		SBorder::Construct( SBorder::FArguments()
			.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
			.VAlign(VAlign_Center)
			[
				VerticalBox
			]
		);
	}

	SSlowTaskWidget()
		: ProgressNumerator(0)
		, ProgressDenominator(1)
	{
	}

	void SetProgressPercent( int32 InProgressNumerator, int32 InProgressDenominator )
	{
		// interlocked exchange for threaded version
		FPlatformAtomics::InterlockedExchange( &ProgressNumerator, InProgressNumerator );
		FPlatformAtomics::InterlockedExchange( &ProgressDenominator, InProgressDenominator );
		UpdateProgressText();
	}

	void SetProgressText( const FText& InProgressText )
	{
		ProgressText = InProgressText;
		UpdateProgressText();
	}
	
	FReply OnCancel()
	{
		OnCancelClickedDelegate.ExecuteIfBound();
		return FReply::Handled();
	}
private:

	TOptional<float> GetProgressFraction() const
	{
		// Only show a percentage if there is something interesting to report
		if( ProgressNumerator > 0 && ProgressDenominator > 0 )
		{
			return (float)ProgressNumerator/ProgressDenominator;
		}
		else
		{
			// Return non-value to indicate marquee mode
			// for the progress bar.
			return TOptional<float>();
		}
	}

	FText OnGetProgressText() const
	{
		return ProgressStatusText;
	}

	void UpdateProgressText()
	{
		if( ProgressNumerator > 0 && ProgressDenominator > 0 )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("StatusText"), ProgressText );
			Args.Add( TEXT("ProgressCompletePercentage"), FText::AsPercent( (float)ProgressNumerator/ProgressDenominator) );
			ProgressStatusText = FText::Format( NSLOCTEXT("FeedbackContextProgress", "ProgressStatusFormat", "{StatusText} ({ProgressCompletePercentage})"), Args );
		}
		else
		{
			ProgressStatusText = ProgressText;
		}
	}

private:
	FText ProgressText;
	FText ProgressStatusText;
	int32 ProgressNumerator;
	int32 ProgressDenominator;
	FOnCancelClickedDelegate OnCancelClickedDelegate;
};

static void TickSlate()
{
	// Tick Slate application
	FSlateApplication::Get().Tick();

	// Sync the game thread and the render thread. This is needed if many StatusUpdate are called
	FSlateApplication::Get().GetRenderer()->Sync();
}


const float FFeedbackContextEditor::UIUpdateGatingTime = 1.0f / 60.0f;


FFeedbackContextEditor::FFeedbackContextEditor() : 
	DialogRequestCount(0),
	SlowTaskCount(0),
	HasTaskBeenCancelled(false)
{
	DialogRequestStack.Reserve(15);
}

void FFeedbackContextEditor::Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category )
{
	if( !GLog->IsRedirectingTo( this ) )
	{
		GLog->Serialize( V, Verbosity, Category );
	}
}

/**
 * Tells the editor that a slow task is beginning
 * 
 * @param Task					The description of the task beginning.
 * @param ShowProgressDialog	Whether to show the progress dialog.
 */
void FFeedbackContextEditor::BeginSlowTask( const FText& Task, bool bShowProgressDialog, bool bShowCancelButton )
{
	// Attempt to parent the slow task window to the slate main frame
	TSharedPtr<SWindow> ParentWindow;
	if( FModuleManager::Get().IsModuleLoaded( "MainFrame" ) )
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>( "MainFrame" );
		ParentWindow = MainFrame.GetParentWindow();
	}

	if( GIsEditor && ParentWindow.IsValid())
	{
		GIsSlowTask = ++SlowTaskCount>0;
		GSlowTaskOccurred = GIsSlowTask;

		// Show a wait cursor for long running tasks
		if (SlowTaskCount == 1)
		{
			// NOTE: Any slow tasks that start after the first slow task is running won't display
			//   status text unless 'StatusUpdate' is called
			StatusMessage.StatusText = Task;
		}

		// Don't show the progress dialog if the Build Progress dialog is already visible
		bool bProgressWindowShown = BuildProgressWidget.IsValid();

		// Don't show the progress dialog if a Slate menu is currently open
		const bool bHaveOpenMenu = FSlateApplication::Get().AnyMenusVisible();

		if( bShowProgressDialog && bHaveOpenMenu )
		{
			UE_LOG(LogSlate, Warning, TEXT("Prevented a slow task dialog from being summoned while a context menu was open") );
		}

		// reset the cancellation flag
		HasTaskBeenCancelled = false;

		bShowProgressDialog &= !bHaveOpenMenu;

		DialogRequestStack.Push( ( bProgressWindowShown ? 0 : bShowProgressDialog) );

		FOnCancelClickedDelegate OnCancelClicked;
		if ( bShowCancelButton )
		{
			// The cancel button is only displayed if a delegate is bound to it.
			OnCancelClicked = FOnCancelClickedDelegate::CreateRaw(this, &FFeedbackContextEditor::OnUserCancel);
		}

		if(!bProgressWindowShown && bShowProgressDialog && ++DialogRequestCount == 1 )
		{
			if( !bProgressWindowShown && FSlateApplication::Get().CanDisplayWindows() )
			{
				TSharedRef<SWindow> SlowTaskWindowRef = SNew(SWindow)
					.ClientSize(FVector2D(500,100))
					.IsPopupWindow(true)
					.ActivateWhenFirstShown(true);

				SlowTaskWidget = SNew(SSlowTaskWidget)
					.OnCancelClickedDelegate( OnCancelClicked );
				SlowTaskWindowRef->SetContent( SlowTaskWidget.ToSharedRef() );

				SlowTaskWidget->SetProgressText( StatusMessage.StatusText );

				SlowTaskWindow = SlowTaskWindowRef;

				const bool bSlowTask = true;
				FSlateApplication::Get().AddModalWindow( SlowTaskWindowRef, ParentWindow, bSlowTask );

				SlowTaskWindowRef->ShowWindow();

				TickSlate();
			}
		}
	}
}

/** Whether or not the user has canceled out of this dialog */
bool FFeedbackContextEditor::ReceivedUserCancel( void )
{
	const bool res = HasTaskBeenCancelled;
	HasTaskBeenCancelled = false;
	return res;
}

void FFeedbackContextEditor::OnUserCancel()
{
	HasTaskBeenCancelled = true;
}

/**
 * Tells the editor that the slow task is done.
 */
void FFeedbackContextEditor::EndSlowTask()
{
	TSharedPtr<SWindow> ParentWindow;
	if( FModuleManager::Get().IsModuleLoaded( "MainFrame" ) )
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>( "MainFrame" );
		ParentWindow = MainFrame.GetParentWindow();
	}

	if( GIsEditor && ParentWindow.IsValid())
	{
		check(SlowTaskCount>0);
		GIsSlowTask = --SlowTaskCount>0;

		checkSlow(DialogRequestStack.Num()>0);
		const bool bWasDialogRequest = DialogRequestStack.Pop();

		// Restore the cursor now that the long running task is done
		if (SlowTaskCount == 0)
		{
			// Reset cached message
			StatusMessage.StatusText = FText::GetEmpty();
			StatusMessage.ProgressNumerator = StatusMessage.SavedNumerator = 0;
			StatusMessage.ProgressDenominator = StatusMessage.SavedDenominator = 0;
			StatusMessage.LastUpdateTime  = FPlatformTime::Seconds();
		}

		if( bWasDialogRequest )
		{
			checkSlow(DialogRequestCount>0);
			if ( --DialogRequestCount == 0 )
			{
				if( SlowTaskWindow.IsValid() )
				{
					SlowTaskWindow.Pin()->RequestDestroyWindow();
					SlowTaskWindow.Reset();
					SlowTaskWidget.Reset();

				}
			}
		}
	}
}

void FFeedbackContextEditor::SetContext( FContextSupplier* InSupplier )
{
}

bool FFeedbackContextEditor::StatusUpdate( int32 Numerator, int32 Denominator, const FText& StatusText )
{
	// don't update too frequently
	if( GIsSlowTask )
	{
		// limit to update to once every 'y' seconds
		double Now = FPlatformTime::Seconds();
		double Diff = Now - StatusMessage.LastUpdateTime;
		if ( Diff >= UIUpdateGatingTime )
		{
			StatusMessage.LastUpdateTime = Now;
		}
		else
		{
			return true;
		}
	}

	return ApplyStatusUpdate( StatusText, Numerator, Denominator );
}

bool FFeedbackContextEditor::StatusForceUpdate( int32 Numerator, int32 Denominator, const FText& StatusText )
{
	return ApplyStatusUpdate( StatusText, Numerator, Denominator );
}

bool FFeedbackContextEditor::ApplyStatusUpdate( const FText& StatusText, int32 ProgressNumerator, int32 ProgressDenominator )
{
	// Cache the new status message and progress
	StatusMessage.StatusText = StatusText;

	if( ProgressNumerator >= 0 ) // Ignore if set to -1
	{
		StatusMessage.ProgressNumerator = StatusMessage.SavedNumerator = ProgressNumerator;
	}
	if( ProgressDenominator >= 0 ) // Ignore if set to -1
	{
		StatusMessage.ProgressDenominator = StatusMessage.SavedDenominator = ProgressDenominator;
	}

	if( GIsSlowTask )
	{
		bool bCanCancelTask = (BuildProgressWidget.IsValid());

		// Don't bother refreshing progress for sub-tasks (tasks greater than 2 levels deep.)  The reason we
		// allow two levels is because some subsystems call PushStatus() before reporting any status for
		// first-tier tasks, and we wouldn't be able to display any progress for those tasks otherwise.
		if( StatusMessageStack.Num() <= 1 || bCanCancelTask)
		{
			StatusUpdateProgress(
				StatusMessage.StatusText,
				StatusMessage.ProgressNumerator,
				StatusMessage.ProgressDenominator,
				StatusMessageStack.Num() == 0 || DialogRequestCount == 0
				);

			if ( DialogRequestCount > 0 )
			{
				if( SlowTaskWindow.IsValid() )
				{
					SlowTaskWidget->SetProgressText( StatusMessage.StatusText );
					SlowTaskWidget->SetProgressPercent( StatusMessage.ProgressNumerator, StatusMessage.ProgressDenominator );

					if (FSlateApplication::Get().CanDisplayWindows())
					{
						TickSlate();
					}
				}
			}
		}
	}

	// Also update the splash screen text (in case we're in the middle of starting up)
	if ( !StatusMessage.StatusText.IsEmpty() )
	{
		FPlatformSplash::SetSplashText( SplashTextType::StartupProgress, *StatusMessage.StatusText.ToString() );
	}
	return true;
}

/**
 * Updates the progress amount without changing the status message text
 *
 * @param Numerator		New progress numerator
 * @param Denominator	New progress denominator
 */
void FFeedbackContextEditor::UpdateProgress( int32 Numerator, int32 Denominator )
{
	// Cache the new progress
	if( Numerator >= 0 ) // Ignore if set to -1
	{
		StatusMessage.ProgressNumerator = Numerator;
	}
	if( Denominator >= 0 ) // Ignore if set to -1
	{
		StatusMessage.ProgressDenominator = Denominator;
	}

	if( GIsSlowTask )
	{
		bool bCanCancelTask = (BuildProgressWidget.IsValid());

		// Don't bother refreshing progress for sub-tasks (tasks greater than 2 levels deep.)  The reason we
		// allow two levels is because some subsystems call PushStatus() before reporting any status for
		// first-tier tasks, and we wouldn't be able to display any progress for those tasks otherwise.
		if( StatusMessageStack.Num() <= 1 || bCanCancelTask)
		{
			// calculate our previous percentage and our new one
			float SavedRatio = (float)StatusMessage.SavedNumerator / (float)StatusMessage.SavedDenominator;
			float NewRatio = (float)StatusMessage.ProgressNumerator / (float)StatusMessage.ProgressDenominator;
			double Now = FPlatformTime::Seconds();

			// update the progress bar if we've moved enough since last time, or we are going to start or end of bar,
			// or if a second has passed
			//
			// or if the build progress is running...
			bool bUpdate =
				(FMath::Abs<float>(SavedRatio - NewRatio) > 0.1f || Numerator == 0 || Numerator >= (Denominator - 1) ||
				Now >= StatusMessage.LastUpdateTime + UIUpdateGatingTime);
				
			if (bUpdate || bCanCancelTask)
			{
				StatusMessage.SavedNumerator = StatusMessage.ProgressNumerator;
				StatusMessage.SavedDenominator = StatusMessage.ProgressDenominator;
				StatusMessage.LastUpdateTime = Now;

				StatusUpdateProgress(
					FText::GetEmpty(),
					StatusMessage.ProgressNumerator,
					StatusMessage.ProgressDenominator,
					StatusMessageStack.Num() == 0 || DialogRequestCount == 0
					);
			
				if ( DialogRequestCount > 0 )
				{
					if( SlowTaskWindow.IsValid()  )
					{
						SlowTaskWidget->SetProgressPercent( StatusMessage.ProgressNumerator, StatusMessage.ProgressDenominator );

						if (FSlateApplication::Get().CanDisplayWindows())
						{
							TickSlate();
						}
					}
				}
			}
		}
	}
}

/**
 *  Updates text and value for various progress meters.
 *
 *	@param StatusText				New status text
 *	@param ProgressNumerator		Numerator for the progress meter (its current value).
 *	@param ProgressDenominitator	Denominiator for the progress meter (its range).
 */
void FFeedbackContextEditor::StatusUpdateProgress( const FText& StatusText, int32 ProgressNumerator, int32 ProgressDenominator, bool bUpdateBuildDialog/*=true*/ )
{
	// Clean up deferred cleanup objects from rendering thread every once in a while.
	static double LastTimePendingCleanupObjectsWhereDeleted;
	if( FPlatformTime::Seconds() - LastTimePendingCleanupObjectsWhereDeleted > 1 )
	{
		// Get list of objects that are pending cleanup.
		FPendingCleanupObjects* PendingCleanupObjects = GetPendingCleanupObjects();
		// Flush rendering commands in the queue.
		FlushRenderingCommands();
		// It is now save to delete the pending clean objects.
		delete PendingCleanupObjects;
		// Keep track of time this operation was performed so we don't do it too often.
		LastTimePendingCleanupObjectsWhereDeleted = FPlatformTime::Seconds();
	}

	// Update build progress dialog if it is visible.
	const bool bBuildProgressDialogVisible = BuildProgressWidget.IsValid();

	if( bBuildProgressDialogVisible && bUpdateBuildDialog )
	{
		if( !StatusText.IsEmpty() )
		{
			BuildProgressWidget->SetBuildStatusText( StatusText );
		}

		BuildProgressWidget->SetBuildProgressPercent( ProgressNumerator, ProgressDenominator );

		if (FSlateApplication::Get().CanDisplayWindows())
		{
			TickSlate();
		}
	}
}

/** Pushes the current status message/progress onto the stack so it can be restored later */
void FFeedbackContextEditor::PushStatus()
{
	StatusMessage.LastUpdateTime = 0.0f; // Gurantees the next call to update this window will not fail due to the 1 second requirement between updates. When we are popped, our time will also be 0.0f.

	// Push the current message onto the stack.  This doesn't change the current message though.  You should
	// call StatusUpdate after calling PushStatus to update the message.
	StatusMessageStack.Add( StatusMessage );
}



/** Restores the previously pushed status message/progress */
void FFeedbackContextEditor::PopStatus()
{
	if( StatusMessageStack.Num() > 0 )
	{
		// Pop from stack
		StatusMessageStackItem PoppedStatusMessage = StatusMessageStack.Pop();

		// Update the message text
		StatusMessage.StatusText = PoppedStatusMessage.StatusText;

		// Only overwrite progress if the item on the stack actually had some progress set
		if( PoppedStatusMessage.ProgressDenominator > 0 )
		{
			StatusMessage.ProgressNumerator = PoppedStatusMessage.ProgressNumerator;
			StatusMessage.ProgressDenominator = PoppedStatusMessage.ProgressDenominator;
			StatusMessage.SavedNumerator = PoppedStatusMessage.SavedNumerator;
			StatusMessage.SavedDenominator = PoppedStatusMessage.SavedDenominator;
			StatusMessage.LastUpdateTime = PoppedStatusMessage.LastUpdateTime;
		}

		// Update the GUI!
		if( GIsSlowTask )
		{
			StatusMessage.SavedNumerator = StatusMessage.ProgressNumerator;
			StatusMessage.SavedDenominator = StatusMessage.ProgressDenominator;

			// Don't bother refreshing progress for sub-tasks (tasks greater than 2 levels deep.)  The reason we
			// allow two levels is because some subsystems call PushStatus() before reporting any status for
			// first-tier tasks, and we wouldn't be able to display any progress for those tasks otherwise.
			if( StatusMessageStack.Num() <= 1 )
			{
				StatusUpdateProgress(
					StatusMessage.StatusText,
					StatusMessage.ProgressNumerator,
					StatusMessage.ProgressDenominator);

				if ( DialogRequestCount > 0 )
				{
					if( SlowTaskWindow.IsValid()  )
					{
						SlowTaskWidget->SetProgressText( StatusMessage.StatusText );
						SlowTaskWidget->SetProgressPercent( StatusMessage.ProgressNumerator, StatusMessage.ProgressDenominator );

						if (FSlateApplication::Get().CanDisplayWindows())
						{
							TickSlate();
						}
					}
				}
			}
		}
	}
}

/** 
 * Show the Build Progress Window 
 * @return Handle to the Build Progress Widget created
 */
TWeakPtr<class SBuildProgressWidget> FFeedbackContextEditor::ShowBuildProgressWindow()
{
	TSharedRef<SWindow> BuildProgressWindowRef = SNew(SWindow)
		.ClientSize(FVector2D(500,200))
		.IsPopupWindow(true);

	BuildProgressWidget = SNew(SBuildProgressWidget);
		
	BuildProgressWindowRef->SetContent( BuildProgressWidget.ToSharedRef() );

	BuildProgressWindow = BuildProgressWindowRef;
				
	// Attempt to parent the slow task window to the slate main frame
	TSharedPtr<SWindow> ParentWindow;

	if( FModuleManager::Get().IsModuleLoaded( "MainFrame" ) )
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>( "MainFrame" );
		ParentWindow = MainFrame.GetParentWindow();
	}

	FSlateApplication::Get().AddModalWindow( BuildProgressWindowRef, ParentWindow, true );
	BuildProgressWindowRef->ShowWindow();	

	BuildProgressWidget->MarkBuildStartTime();
	
	if (FSlateApplication::Get().CanDisplayWindows())
	{
		TickSlate();
	}

	return BuildProgressWidget;
}

/** Close the Build Progress Window */
void FFeedbackContextEditor::CloseBuildProgressWindow()
{
	if( BuildProgressWindow.IsValid() && BuildProgressWidget.IsValid())
	{
		BuildProgressWindow.Pin()->RequestDestroyWindow();
		BuildProgressWindow.Reset();
		BuildProgressWidget.Reset();	
	}
}
