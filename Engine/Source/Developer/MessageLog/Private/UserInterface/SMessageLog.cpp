// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MessageLogPrivatePCH.h"
#include "SMessageLog.h"
#include "MessageLogViewModel.h"
#include "MessageLogListingViewModel.h"
#include "SMessageLogListing.h"

#define LOCTEXT_NAMESPACE "Developer.MessageLog"

SMessageLog::SMessageLog()
	: bUpdatingSelection( false )
{

}

SMessageLog::~SMessageLog()
{
	MessageLogViewModel->OnSelectionChanged().RemoveAll( this );
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SMessageLog::Construct( const FArguments& InArgs, const TSharedPtr<FMessageLogViewModel>& InMessageLogViewModel )
{
	MessageLogViewModel = InMessageLogViewModel;
	check(MessageLogViewModel.IsValid());

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot().AutoHeight() .Padding(2)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(CurrentListingSelector, SComboBox< TSharedPtr<FMessageLogListingViewModel> >)
					.ContentPadding(3)
					.OptionsSource(&MessageLogViewModel->GetLogListingViewModels())
					.OnSelectionChanged(this, &SMessageLog::ChangeCurrentListingSelection)
					.OnGenerateWidget(this, &SMessageLog::MakeCurrentListingSelectionWidget)
					[
						SNew(STextBlock)
						.Text(this, &SMessageLog::GetCurrentListingLabel)
					]
			]
		]
		+SVerticalBox::Slot()
		.FillHeight(1)
		[
			SAssignNew(CurrentListingDisplay, SBorder)
		]
	];

	// Default to the first available (or the last used) listing
	const TArray< TSharedPtr< FMessageLogListingViewModel > >& ViewModels = MessageLogViewModel->GetLogListingViewModels();
	if ( ViewModels.Num() > 0 )
	{
		TSharedPtr< FMessageLogListingViewModel > DefaultViewModel = ViewModels[0];
		if ( FPaths::FileExists(GEditorUserSettingsIni) )
		{
			FString LogName;
			if ( GConfig->GetString( TEXT("MessageLog"), TEXT("LastLogListing"), LogName, GEditorUserSettingsIni ) )
			{
				TSharedPtr< FMessageLogListingViewModel > LoadedViewModel = MessageLogViewModel->FindLogListingViewModel( FName( *LogName ) );
				if ( LoadedViewModel.IsValid() )
				{
					DefaultViewModel = LoadedViewModel;
				}
			}
		}
		ChangeCurrentListingSelection( DefaultViewModel, ESelectInfo::Direct );
	}

	MessageLogViewModel->OnSelectionChanged().AddSP( this, &SMessageLog::OnSelectionUpdated );

}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SMessageLog::OnSelectionUpdated()
{
	TSharedPtr<FMessageLogListingViewModel> LogListingViewModel = MessageLogViewModel->GetCurrentListingViewModel();

	if ( LogListingViewModel.IsValid() )
	{
		if( !bUpdatingSelection )
		{
			bUpdatingSelection = true;
			CurrentListingSelector->SetSelectedItem( LogListingViewModel ) ;
			bUpdatingSelection = false;
		}

		CurrentListingDisplay->SetContent(SAssignNew(LogListing, SMessageLogListing, LogListingViewModel.ToSharedRef()));
	}
}

FReply SMessageLog::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	if(LogListing.IsValid())
	{
		return LogListing->GetCommandList()->ProcessCommandBindings( InKeyboardEvent ) ? FReply::Handled() : FReply::Unhandled();
	}
	
	return FReply::Unhandled();
}

FString SMessageLog::GetCurrentListingLabel() const
{
	return MessageLogViewModel->GetCurrentListingLabel();
}

void SMessageLog::ChangeCurrentListingSelection( TSharedPtr<FMessageLogListingViewModel> Selection, ESelectInfo::Type /*SelectInfo*/ )
{
	if( bUpdatingSelection )
	{
		return;
	}
	bUpdatingSelection = true;
	MessageLogViewModel->ChangeCurrentListingViewModel( Selection->GetName() );
	bUpdatingSelection = false;
}

TSharedRef<SWidget> SMessageLog::MakeCurrentListingSelectionWidget( TSharedPtr<FMessageLogListingViewModel> Selection )
{
	return SNew(STextBlock) .Text( Selection->GetLabel() );
}


#undef LOCTEXT_NAMESPACE
