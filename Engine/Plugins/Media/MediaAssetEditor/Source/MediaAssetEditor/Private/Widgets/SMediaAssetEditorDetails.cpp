// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetEditorPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SMediaAssetEditorDetails"


/* SMediaAssetEditorDetails interface
 *****************************************************************************/

void SMediaAssetEditorDetails::Construct( const FArguments& InArgs, UMediaAsset* InMediaAsset, const TSharedRef<ISlateStyle>& InStyle )
{
	MediaAsset = InMediaAsset;

	// initialize details view
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bObjectsUseNameArea = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = true;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
	}

	TSharedPtr<IDetailsView> DetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(MediaAsset);

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f, 2.0f)
			[
				DetailsView.ToSharedRef()
			]
	];
}


#undef LOCTEXT_NAMESPACE
