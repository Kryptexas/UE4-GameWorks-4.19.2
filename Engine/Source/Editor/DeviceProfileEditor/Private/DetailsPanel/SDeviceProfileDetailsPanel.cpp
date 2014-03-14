// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================
	SDeviceProfileDetailsPanel.cpp: Implements the SDeviceProfileDetailsPanel class.
=============================================================================*/


#include "DeviceProfileEditorPCH.h"


#define LOCTEXT_NAMESPACE "DeviceProfileDetailsPanel"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SDeviceProfileDetailsPanel::Construct( const FArguments& InArgs )
{
	// Generate our details panel.
	DetailsViewBox = SNew( SVerticalBox );
	RefreshUI();

	ChildSlot
	[
		SNew( SBorder )
		.BorderImage( FEditorStyle::GetBrush( "Docking.Tab.ContentAreaBrush" ) )
		[
			SNew( SVerticalBox )
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( FMargin( 2.0f ) )
			.VAlign( VAlign_Bottom )
			[
				SNew( SHorizontalBox )
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding( 0.0f, 0.0f, 4.0f, 0.0f )
				[
					SNew( SImage )
					.Image( FEditorStyle::GetBrush( "LevelEditor.Tabs.Details" ) )
				]
				+ SHorizontalBox::Slot()
					.VAlign( VAlign_Center )
					[
						SNew( STextBlock )
						.Text( LOCTEXT("CVarsLabel", "Console Variables").ToString() )
						.TextStyle( FEditorStyle::Get(), "Docking.TabFont" )
					]
			]

			+ SVerticalBox::Slot()
			[
				SNew( SHorizontalBox )
				+ SHorizontalBox::Slot()
				[
					DetailsViewBox.ToSharedRef()
				]
			]
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SDeviceProfileDetailsPanel::UpdateUIForProfile( const TWeakObjectPtr< UDeviceProfile > InProfile )
{
	ViewingProfile = InProfile;
	RefreshUI();
}


void SDeviceProfileDetailsPanel::RefreshUI()
{
	DetailsViewBox->ClearChildren();

	// initialize settings view
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bObjectsUseNameArea = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = false;
	}

	SettingsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);

	if( ViewingProfile.IsValid() )
	{
		FString DeviceProfileTypeIcon;
		TArray<ITargetPlatform*> TargetPlatforms = GetTargetPlatformManager()->GetTargetPlatforms();
		for (int32 TargetPlatformIndex = 0; TargetPlatformIndex < TargetPlatforms.Num(); ++TargetPlatformIndex)
		{
			DeviceProfileTypeIcon = FString::Printf(TEXT("Launcher.Platform_%s"), *TargetPlatforms[TargetPlatformIndex]->PlatformName());
			break;
		}

		SettingsView->SetObject(&*ViewingProfile);
		// If a profile is provided, show the details for this profile.
		DetailsViewBox->AddSlot()
		[
			SNew( SBorder )
			.BorderImage( FEditorStyle::GetBrush( "ToolBar.Background" ) )
			[
				SNew( SVerticalBox )
				+ SVerticalBox::Slot()
				.HAlign( HAlign_Left )
				.AutoHeight()
				[
					SNew( SHorizontalBox )
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(4.0f, 0.0f, 2.0f, 0.0f)
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush(*DeviceProfileTypeIcon))
					]

					+SHorizontalBox::Slot()
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.VAlign(VAlign_Center)
						[
							SNew( STextBlock )
							.Text( FString::Printf( TEXT( "%s selected" ), *ViewingProfile->GetName() ) )
						]
					]
				]

				/**
				 * CVars part of the details panel
				 */
				+ SVerticalBox::Slot()
				.Padding(4.0f)
				.FillHeight(1.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew( SBorder )
						.BorderImage( FEditorStyle::GetBrush( "Docking.Tab.ContentAreaBrush" ) )
						[
							SNew( SVerticalBox )
							+ SVerticalBox::Slot()
							.FillHeight(1.0f)
							.Padding( FMargin( 4.0f ) )
							[
								SettingsView.ToSharedRef()
							]
						]
					]
				]
			]
		];
	}
	else
	{
		// No profile was selected, so the panel should reflect this
		DetailsViewBox->AddSlot()
			[
				SNew( SBorder )
				.BorderImage( FEditorStyle::GetBrush( "ToolBar.Background" ) )
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.VAlign( VAlign_Top )
					.HAlign( HAlign_Center )
					.Padding( 4.0f )
					[
						SNew( STextBlock )
						.Text( LOCTEXT("SelectAProfile", "Select a device profile above...").ToString() )					
					]

				]
			];
	}
}


#undef LOCTEXT_NAMESPACE