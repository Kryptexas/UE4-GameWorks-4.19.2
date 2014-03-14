// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================
	SDeviceProfileEditor.cpp: Implements the SDeviceProfileEditor class.
=============================================================================*/


#include "DeviceProfileEditorPCH.h"

#include "SDeviceProfileCreateProfilePanel.h"

#include "PropertyEditorModule.h"
#include "PropertyPath.h"


#define LOCTEXT_NAMESPACE "DeviceProfileEditor"

// Tab names for those available in the Device Profile Editor.
static const FName DeviceProfileEditorTabName("DeviceProfiles");


void SDeviceProfileEditor::Construct( const FArguments& InArgs )
{
	DeviceProfileManager = GEngine->GetDeviceProfileManager();

	// Setup the tab layout for the editor.
	TSharedRef<FWorkspaceItem> RootMenuGroup = FWorkspaceItem::NewGroup( LOCTEXT("RootMenuGroupName", "Root") );
	TSharedRef<FWorkspaceItem> DeviceManagerMenuGroup = RootMenuGroup->AddGroup(LOCTEXT("DeviceProfileEditorMenuGroupName", "Device Profile Editor Tabs"));
	{
		TSharedRef<SDockTab> DeviceProfilePropertyEditorTab =
			SNew(SDockTab)
			. TabRole( ETabRole::MajorTab )
			. Label( LOCTEXT("TabTitle", "Device Profile Editor") )
			. ToolTipText( LOCTEXT( "TabTitle_ToolTip", "The Device Profile Editor" ) );

		TabManager = FGlobalTabmanager::Get()->NewTabManager( DeviceProfilePropertyEditorTab );

		TabManager->RegisterTabSpawner( DeviceProfileEditorTabName, FOnSpawnTab::CreateRaw( this, &SDeviceProfileEditor::HandleTabManagerSpawnTab, DeviceProfileEditorTabName) )
			.SetDisplayName( LOCTEXT("DeviceProfileEditorPropertyTableTabTitle", "Device Profiles") )
			.SetIcon( FSlateIcon(FEditorStyle::GetStyleSetName(), "DeviceDetails.Tabs.ProfileEditor") )
			.SetGroup( DeviceManagerMenuGroup );
	}


	// Create the tab layout widget
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("DeviceProfileEditorLayout_v2.0")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(DeviceProfileEditorTabName, ETabState::OpenedTab)
				->SetHideTabWell(false)
				->SetForegroundTab(DeviceProfileEditorTabName)
			)
		);


	// Create & initialize main menu
	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());
	MenuBarBuilder.AddPullDownMenu(
		LOCTEXT("WindowMenuLabel", "Window"),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateSP(TabManager.ToSharedRef(), &FTabManager::PopulateTabSpawnerMenu, RootMenuGroup)
	);


	ChildSlot
	[		
		SNew( SVerticalBox )
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MenuBarBuilder.MakeWidget()
		]

		+ SVerticalBox::Slot()
		[
			// Create tab well where our property grid etc. will live
			SNew( SVerticalBox )
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				TabManager->RestoreFrom( Layout, TSharedPtr<SWindow>() ).ToSharedRef()
			]
		]
	];

}


SDeviceProfileEditor::~SDeviceProfileEditor()
{
	if( DeviceProfileManager.IsValid() )
	{
		DeviceProfileManager->SaveProfiles();

		// Unbind our delegates when destroyed
		DeviceProfileManager->OnManagerUpdated().RemoveRaw( this, &SDeviceProfileEditor::RebuildPropertyTable );
	}
}


TSharedPtr< SWidget > SDeviceProfileEditor::CreateMainDeviceProfilePanel()
{
	TSharedPtr< SWidget > PanelWidget = SNew( SVerticalBox )
	+ SVerticalBox::Slot()
	[
		SNew( SSplitter )
		.Orientation( Orient_Vertical )
		+ SSplitter::Slot()
		.SizeRule(SSplitter::ESizeRule::SizeToContent)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding( FMargin( 2.0f, 0.0f ) )
			.AutoHeight()
			[
				SNew( SBorder )
				.BorderImage( FEditorStyle::GetBrush( "Docking.Tab.ContentAreaBrush" ) )
				[
					SNew( SDeviceProfileCreateProfilePanel, DeviceProfileManager )
				]
			]
		]
		
		+ SSplitter::Slot()
		.Value( 0.3f )
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding( FMargin( 2.0f, 0.0f ) )
			[
				SNew( SBorder )
				.BorderImage( FEditorStyle::GetBrush( "Docking.Tab.ContentAreaBrush" ) )
				[
					SAssignNew( DeviceProfileSelectionPanel, SDeviceProfileSelectionPanel, DeviceProfileManager )
					.OnDeviceProfilePinned( this, &SDeviceProfileEditor::HandleDeviceProfilePinned )
					.OnDeviceProfileUnpinned( this, &SDeviceProfileEditor::HandleDeviceProfileUnpinned )
					.OnDeviceProfileSelectionChanged( this, &SDeviceProfileEditor::HandleDeviceProfileSelectionChanged )
				]
			]
		]

		+ SSplitter::Slot()
		.Value( 0.7f )
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding( FMargin( 2.0f, 0.0f ) )
			.FillHeight(1.0f)
			[
				SNew( SBorder )
				.BorderImage( FEditorStyle::GetBrush( "Docking.Tab.ContentAreaBrush" ) )
				[
					// Create the details panel
					SAssignNew( DetailsPanel, SDeviceProfileDetailsPanel )		
				]
			]
		]
	];

	return PanelWidget;
}


TSharedRef<SDockTab> SDeviceProfileEditor::HandleTabManagerSpawnTab( const FSpawnTabArgs& Args, FName TabIdentifier )
{
	TSharedPtr<SWidget> TabWidget = SNullWidget::NullWidget;

	if( TabIdentifier == DeviceProfileEditorTabName )
	{
		TabWidget = SNew( SVerticalBox )
		+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(0.2f)
			[
				CreateMainDeviceProfilePanel().ToSharedRef()
			]
			+SHorizontalBox::Slot()
			.FillWidth(0.8f)
			[		
				SNew( SBorder )
				.BorderImage( FEditorStyle::GetBrush( "Docking.Tab.ContentAreaBrush" ) )
				[
					SNew( SVerticalBox )
					+SVerticalBox::Slot()
					.Padding( FMargin( 2.0f ) )
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding( 0.0f, 0.0f, 4.0f, 0.0f )
						[
							SNew( SImage )
							.Image( FEditorStyle::GetBrush( "LevelEditor.Tabs.Details" ) )
						]
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						[
							SNew(STextBlock)
							.TextStyle( FEditorStyle::Get(), "Docking.TabFont" )
							.Text( LOCTEXT("DeviceProfilePropertyEditorLabel", "Device Profile Property Editor...") )
						]
					]
					+ SVerticalBox::Slot()
					.Padding( FMargin( 4.0f ) )
					[
						SNew( SBorder )
						.BorderImage( FEditorStyle::GetBrush( "ToolBar.Background" ) )
						[
							SNew( SOverlay )
							+ SOverlay::Slot()
							[
								// Show the property editor
								SNew( SHorizontalBox )
								+SHorizontalBox::Slot()
								.FillWidth(0.375f)
								[
									SNew( SBorder )
									.Padding( 2 )
									.Content()
									[
										SetupPropertyEditor()
									]
								]			
							]

							+ SOverlay::Slot()
							[
								// Conditionally draw a notification that indicates profiles should be pinned to be visible.
								SNew( SVerticalBox )
								.Visibility( this, &SDeviceProfileEditor::GetEmptyDeviceProfileGridNotificationVisibility )
								+SVerticalBox::Slot()
								[
									SNew( SBorder )
									.BorderImage( FEditorStyle::GetBrush( "ToolBar.Background" ) )
									[
										SNew( SHorizontalBox )
										+SHorizontalBox::Slot()
										.HAlign( HAlign_Center )
										[
											SNew( SHorizontalBox )
											+SHorizontalBox::Slot()
											.AutoWidth()
											.HAlign( HAlign_Center )
											.VAlign( VAlign_Center )
											[
												SNew( SImage )
												.Image( FEditorStyle::GetBrush( "PropertyEditor.AddColumnOverlay" ) )
											]

											+SHorizontalBox::Slot()
											.AutoWidth()
											.HAlign( HAlign_Center )
											.VAlign( VAlign_Center )
											[
												SNew( SImage )
												.Image( FEditorStyle::GetBrush( "PropertyEditor.RemoveColumn" ) )
											]
								
											+ SHorizontalBox::Slot()
											.AutoWidth()
											.HAlign( HAlign_Center )
											.VAlign( VAlign_Center )
											.Padding( FMargin( 0, 0, 3, 0 ) )
											[
												SNew( STextBlock )
												.Font( FEditorStyle::GetFontStyle( "PropertyEditor.AddColumnMessage.Font" ) )
												.Text( LOCTEXT("GenericPropertiesTitle", "Pin Profiles to Add Columns") )
												.ColorAndOpacity( FEditorStyle::GetColor( "PropertyEditor.AddColumnMessage.Color" ) )
											]
										]
									]
								]
							]
						]
					]
				]
			]
		];
	}

	// Return the tab with the relevant widget embedded
	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			TabWidget.ToSharedRef()
		];
}


void SDeviceProfileEditor::HandleDeviceProfileSelectionChanged( const TWeakObjectPtr< UDeviceProfile > InDeviceProfile )
{
	DetailsPanel->UpdateUIForProfile( InDeviceProfile );
}


void SDeviceProfileEditor::HandleDeviceProfilePinned( const TWeakObjectPtr< UDeviceProfile > DeviceProfile )
{
	if( !DeviceProfiles.Contains( DeviceProfile.Get() ) )
	{
		DeviceProfiles.Add( DeviceProfile.Get() );
		RebuildPropertyTable();
	}
}


void SDeviceProfileEditor::HandleDeviceProfileUnpinned( const TWeakObjectPtr< UDeviceProfile > DeviceProfile )
{
	if( DeviceProfiles.Contains( DeviceProfile.Get() ) )
	{
		DeviceProfiles.Remove( DeviceProfile.Get() );
		RebuildPropertyTable();
	}
}


EVisibility SDeviceProfileEditor::GetEmptyDeviceProfileGridNotificationVisibility() const
{
	// IF we aren't showing any items, our prompt should be visible
	return PropertyTable->GetRows().Num() > 0 ?	EVisibility::Hidden : EVisibility::Visible;
}


TSharedRef< SWidget > SDeviceProfileEditor::SetupPropertyEditor()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>( "PropertyEditor" );

	PropertyTable = PropertyEditorModule.CreatePropertyTable();
	RebuildPropertyTable();

	return PropertyEditorModule.CreatePropertyTableWidget( PropertyTable.ToSharedRef() );
}

void SDeviceProfileEditor::RebuildPropertyTable()
{
	PropertyTable->SetObjects( DeviceProfiles );
	PropertyTable->SetSelectionMode( ESelectionMode::None );

	PropertyTable->SetIsUserAllowedToChangeRoot( false );

	for (TFieldIterator<UProperty> DeviceProfilePropertyIter( UDeviceProfile::StaticClass() ); DeviceProfilePropertyIter; ++DeviceProfilePropertyIter)
	{
		TWeakObjectPtr< UProperty > DeviceProfileProperty = *DeviceProfilePropertyIter;
		PropertyTable->AddColumn( DeviceProfileProperty );
	}

	PropertyTable->RequestRefresh();
}


#undef LOCTEXT_NAMESPACE