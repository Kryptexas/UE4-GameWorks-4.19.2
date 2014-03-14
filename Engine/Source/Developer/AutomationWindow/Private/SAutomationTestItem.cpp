// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SAutomationTestItem.cpp: Implements the SAutomationTestItem class.
=============================================================================*/

#include "AutomationWindowPrivatePCH.h"


#define LOCTEXT_NAMESPACE "AutomationTestItem"


/* SAutomationTestItem interface
 *****************************************************************************/

void SAutomationTestItem::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	TestStatus = InArgs._TestStatus;
	ColumnWidth = InArgs._ColumnWidth;
	HighlightText = InArgs._HighlightText;
	OnCheckedStateChangedDelegate = InArgs._OnCheckedStateChanged;

	SMultiColumnTableRow< TSharedPtr< FString > >::Construct( SMultiColumnTableRow< TSharedPtr< FString > >::FArguments(), InOwnerTableView );
}


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedRef<SWidget> SAutomationTestItem::GenerateWidgetForColumn( const FName& ColumnName )
{
	FSlateFontInfo ItemEditorFont = FEditorStyle::GetFontStyle(TEXT("NormalFont"));

	if( ColumnName == AutomationTestWindowConstants::Title)
	{
		return SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.Padding(4.0f, 0.0f)
			[
				//enabled/disabled check box
				SNew( SCheckBox )
				.IsChecked(this, &SAutomationTestItem::IsTestEnabled)
				.OnCheckStateChanged(this, &SAutomationTestItem::HandleTestingCheckbox_Click)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				//this is where the tree is marked as expandable or not.
				SNew( SExpanderArrow, SharedThis( this ) )
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			[
				//name of the test
				SNew( STextBlock )
				.HighlightText( HighlightText )
				.Text( TestStatus->GetDisplayNameWithDecoration() )
			];
	}
	else if( ColumnName == AutomationTestWindowConstants::SmokeTest )
	{
		//icon to show if the test is considered fast or is the parent of a fast test
		return SNew( SImage) 
			.Image( this, &SAutomationTestItem::GetSmokeTestImage );
	}
	else if( ColumnName == AutomationTestWindowConstants::RequiredDeviceCount )
	{
		// Should we display an icon to indicate that this test "Requires" more than one participant?
		if( TestStatus->GetNumParticipantsRequired() > 1 )
		{
			TSharedPtr< SHorizontalBox > HBox = SNew( SHorizontalBox );
			if( TestStatus->GetTotalNumChildren() == 0 )
			{
				// Display a network PC and the number which are required for this test.
				HBox->AddSlot()
					[
						SNew( SImage )
						.Image( FEditorStyle::GetBrush("Automation.Participant") )
					];
				HBox->AddSlot()
					[
						SNew( STextBlock )
						.Text( FString::Printf( TEXT( "x%d" ), TestStatus->GetNumParticipantsRequired() ) )
					];

				HBox->SetToolTipText( FString::Printf( TEXT( "This test requires %d participants to be run." ), TestStatus->GetNumParticipantsRequired() ) );

			}
			else
			{
				HBox->AddSlot()
					.HAlign(HAlign_Center)
					[
						SNew( SImage )
						.Image( FEditorStyle::GetBrush("Automation.ParticipantsWarning") )
						.ToolTipText( LOCTEXT("ParticipantsWarningToolTip", "Some tests require multiple participants") )
					];
			}
			return HBox.ToSharedRef();
		}
	}
	else if( ColumnName == AutomationTestWindowConstants::Status )
	{
		TSharedRef<SHorizontalBox> HBox = SNew (SHorizontalBox);
		int32 NumClusters = FModuleManager::GetModuleChecked<IAutomationControllerModule>("AutomationController").GetAutomationController()->GetNumDeviceClusters();

		//for each cluster, display a status icon
		for (int32 ClusterIndex = 0; ClusterIndex < NumClusters; ++ClusterIndex)
		{
			//if this is a leaf test
			if (TestStatus->GetTotalNumChildren() == 0)
			{
				//for leaf tests
				HBox->AddSlot()
				.MaxWidth(ColumnWidth)
				.FillWidth(1.0)
				[			
					SNew(SBorder)
					.BorderImage( FEditorStyle::GetBrush("ErrorReporting.Box") )
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding( FMargin(3,0) )
					.BorderBackgroundColor( FSlateColor( FLinearColor( 1.0f, 0.0f, 1.0f, 0.0f ) ) )
					.ToolTipText( this, &SAutomationTestItem::GetTestToolTip, ClusterIndex )
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							//image when complete or un-run
							SNew( SImage )
							.Image( this, &SAutomationTestItem::ItemStatus_StatusImage, ClusterIndex )
							.Visibility( this, &SAutomationTestItem::ItemStatus_GetStatusVisibility, ClusterIndex, false )
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(16.0f)
							.HeightOverride(16.0f)
							[
								// Spinning Image while in process
								SNew(SSpinningImage)
								.Image( this, &SAutomationTestItem::ItemStatus_StatusImage, ClusterIndex )
								.Visibility(this, &SAutomationTestItem::ItemStatus_GetStatusVisibility, ClusterIndex, true)
							]
						]
					]
				];
			}
			else
			{
				//for internal tree nodes
				HBox->AddSlot()
				.MaxWidth(ColumnWidth)
				.FillWidth(1.0)
				[
					SNew(SBorder)
					.BorderImage( FEditorStyle::GetBrush("ErrorReporting.Box") )
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding( FMargin(3,0) )
					.BorderBackgroundColor( FSlateColor( FLinearColor( 1.0f, 0.0f, 1.0f, 0.0f ) ) )
					[
						//progress bar for percent of enabled children completed
						SNew(SProgressBar)
						.Percent( this, &SAutomationTestItem::ItemStatus_ProgressFraction, ClusterIndex )
						.FillColorAndOpacity(this, &SAutomationTestItem::ItemStatus_ProgressColor, ClusterIndex )
					]
				];
			}
		}
		return HBox;
	}
	else if( ColumnName == AutomationTestWindowConstants::Timing )
	{
		return SNew( STextBlock )
		.Text( this, &SAutomationTestItem::ItemStatus_DurationText);
	}


	return SNullWidget::NullWidget;
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SAutomationTestItem Implementation
 *****************************************************************************/
const FSlateBrush* SAutomationTestItem::GetSmokeTestImage() const
{
	const FSlateBrush* ImageToUse = NULL;
	if ( TestStatus->GetTestType() & EAutomationTestType::ATT_SmokeTest )
	{
		if ( TestStatus->IsParent() )
		{
			ImageToUse = FEditorStyle::GetBrush("Automation.SmokeTest");
		}
		else
		{
			ImageToUse = FEditorStyle::GetBrush("Automation.SmokeTestParent");
		}
	}

	return ImageToUse;
}


FString SAutomationTestItem::GetTestToolTip( int32 ClusterIndex ) const
{
	FString ToolTip;
	EAutomationState::Type TestState = TestStatus->GetState( ClusterIndex );
	if ( TestState == EAutomationState::NotRun )
	{
		ToolTip = LOCTEXT("TestToolTipNotRun", "Not Run").ToString();
	}
	else if( TestState == EAutomationState::InProcess )
	{
		ToolTip = LOCTEXT("TestToolTipInProgress", "In progress").ToString();
	}
	else if( TestState == EAutomationState::NotEnoughParticipants )
	{
		ToolTip = LOCTEXT("ToolTipNotEnoughParticipants", "This test could not be completed as there were not enough participants.").ToString();
	}
	else
	{
		ToolTip = TestState == EAutomationState::Success ? LOCTEXT("TestToolTipComplete", "Completed on: ").ToString() : LOCTEXT("TestToolTipFailed", "Failed on: ").ToString();
		ToolTip += TestStatus->GetGameInstanceName( ClusterIndex );
	}
	return ToolTip;
}


ESlateCheckBoxState::Type SAutomationTestItem::IsTestEnabled() const
{
	return TestStatus->IsEnabled();
}


FSlateColor SAutomationTestItem::ItemStatus_BackgroundColor(const int32 ClusterIndex) const
{
	if (TestStatus->GetTotalNumChildren()==0)
	{
		EAutomationState::Type TestState = TestStatus->GetState(ClusterIndex);
		if (TestState == EAutomationState::Fail)
		{
			// Failure is marked by a red background.
			return FSlateColor( FLinearColor( 0.5f, 0.0f, 0.0f ) );
		}
		else if (TestState == EAutomationState::InProcess)
		{
			// In Process, yellow.
			return FSlateColor( FLinearColor( 0.5f, 0.5f, 0.0f ) );
		}
		else if (TestState == EAutomationState::Success)
		{
			// Success is marked by a green background.
			return FSlateColor( FLinearColor( 0.0f, 0.5f, 0.0f ) );
		}

		// Not Scheduled will receive this color which is to say no color since alpha is 0.
		return FSlateColor( FLinearColor( 1.0f, 0.0f, 1.0f, 0.0f ) );
	}
	else
	{
		// Not Scheduled will receive this color which is to say no color since alpha is 0.
		return FSlateColor( FLinearColor( 1.0f, 0.0f, 1.0f, 0.0f ) );
	}
}


FString SAutomationTestItem::ItemStatus_DurationText() const
{
	FString DurationString;
	float MinDuration;
	float MaxDuration;
	if (TestStatus->GetDurationRange(MinDuration, MaxDuration))
	{
		//if there is a duration range
		if (MinDuration != MaxDuration)
		{
			DurationString = FString::Printf(TEXT("%4.4fs - %4.4fs"), MinDuration, MaxDuration);
		}
		else
		{
			DurationString = FString::Printf(TEXT("%4.4fs"), MinDuration);
		}
	}
	return DurationString;
}


EVisibility SAutomationTestItem::ItemStatus_GetStatusVisibility(const int32 ClusterIndex, const bool bForInProcessThrobber) const
{
	EAutomationState::Type TestState = TestStatus->GetState(ClusterIndex);
	bool bImageVisible = TestState != EAutomationState::InProcess;

	bool bFinalVisibility =  bForInProcessThrobber ? !bImageVisible : bImageVisible;

	return bFinalVisibility ? EVisibility::Visible : EVisibility::Collapsed;
}


FString SAutomationTestItem::ItemStatus_NumParticipantsRequiredText() const
{
	return FString::Printf( TEXT( "x %d" ), TestStatus->GetNumParticipantsRequired() );
}


FSlateColor SAutomationTestItem::ItemStatus_ProgressColor(const int32 ClusterIndex) const
{
	FAutomationCompleteState CompleteState;
	TestStatus->GetCompletionStatus(ClusterIndex, CompleteState);

	if (CompleteState.TotalEnabled > 0)
	{
		if (CompleteState.NumEnabledTestsFailed > 0)
		{
			// Failure is marked by a red background.
			return FSlateColor( FLinearColor( 1.0f, 0.0f, 0.0f ) );
		}
		else if((CompleteState.NumEnabledTestsPassed !=  CompleteState.TotalEnabled) || 
			(CompleteState.NumEnabledTestsWarnings > 0) ||
			(CompleteState.NumEnabledTestsCouldntBeRun > 0 ))
		{
			// In Process, yellow.
			return FSlateColor( FLinearColor( 1.0f, 1.0f, 0.0f ) );
		}
		else
		{
			// Success is marked by a green background.
			return FSlateColor( FLinearColor( 0.0f, 1.0f, 0.0f ) );
		}
	}

	// Not Scheduled will receive this color which is to say no color since alpha is 0.
	return FSlateColor( FLinearColor( 1.0f, 0.0f, 1.0f, 0.0f ) );
}


TOptional<float> SAutomationTestItem::ItemStatus_ProgressFraction(const int32 ClusterIndex) const
{
	FAutomationCompleteState CompleteState;
	TestStatus->GetCompletionStatus(ClusterIndex, CompleteState);

	uint32 TotalComplete = CompleteState.NumEnabledTestsPassed + CompleteState.NumEnabledTestsFailed + CompleteState.NumEnabledTestsCouldntBeRun;
	// Only show a percentage if there is something interesting to report
	if( (TotalComplete> 0) && (CompleteState.TotalEnabled > 0) )
	{
		return (float)TotalComplete/CompleteState.TotalEnabled;
	}
	// Return incomplete state
	return 0.0f;
}


const FSlateBrush* SAutomationTestItem::ItemStatus_StatusImage(const int32 ClusterIndex) const
{
	EAutomationState::Type TestState = TestStatus->GetState(ClusterIndex);

	const FSlateBrush* ImageToUse;
	switch( TestState )
	{
	case EAutomationState::Success:
		{
			FAutomationCompleteState CompleteState;
			TestStatus->GetCompletionStatus(ClusterIndex, CompleteState);
			//If there were ANY warnings in the results
			if (CompleteState.NumEnabledTestsWarnings || CompleteState.NumDisabledTestsWarnings)
			{
				ImageToUse = FEditorStyle::GetBrush("Automation.Warning");
			}
			else
			{
				ImageToUse = FEditorStyle::GetBrush("Automation.Success");
			}
		}
		break;

	case EAutomationState::Fail:
		ImageToUse = FEditorStyle::GetBrush("Automation.Fail");
		break;

	case EAutomationState::NotRun:
		{
			ImageToUse = FEditorStyle::GetBrush("Automation.NotRun");
		}
		break;

	case EAutomationState::NotEnoughParticipants:
		ImageToUse = FEditorStyle::GetBrush("Automation.NotEnoughParticipants");
		break;

	default:
	case EAutomationState::InProcess:
		ImageToUse = FEditorStyle::GetBrush("Automation.InProcess");
		break;
	}

	return ImageToUse;
}


/* SAutomationTestitem event handlers
 *****************************************************************************/

void SAutomationTestItem::HandleTestingCheckbox_Click(ESlateCheckBoxState::Type)
{
	OnCheckedStateChangedDelegate.ExecuteIfBound(TestStatus);
}


#undef LOCTEXT_NAMESPACE
