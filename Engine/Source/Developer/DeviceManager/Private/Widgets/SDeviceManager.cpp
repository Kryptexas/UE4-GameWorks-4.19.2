// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SDeviceManager.cpp: Implements the SDeviceManager class.
=============================================================================*/

#include "DeviceManagerPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SDeviceManager"


/* Local constants
 *****************************************************************************/

static const FName DeviceAppsTabId("DeviceApps");
static const FName DeviceBrowserTabId("DeviceBrowser");
static const FName DeviceDetailsTabId("DeviceDetails");
static const FName DeviceProcessesTabId("DeviceProcesses");
static const FName DeviceToolbarTabId("DeviceToolbar");


/* SDeviceManager structors
 *****************************************************************************/

SDeviceManager::SDeviceManager( )
	: Model(MakeShareable(new FDeviceManagerModel()))
{ }


/* SDeviceManager interface
 *****************************************************************************/

void SDeviceManager::Construct( const FArguments& InArgs, const ITargetDeviceServiceManagerRef& InDeviceServiceManager, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow )
{
	DeviceServiceManager = InDeviceServiceManager;

	// create & initialize tab manager
	TabManager = FGlobalTabmanager::Get()->NewTabManager(ConstructUnderMajorTab);

	TSharedRef<FWorkspaceItem> RootMenuGroup = FWorkspaceItem::NewGroup(LOCTEXT("RootMenuGroup", "Root"));
	TSharedRef<FWorkspaceItem> AppMenuGroup = RootMenuGroup->AddGroup(LOCTEXT("DeviceManagerMenuGroupName", "Device Manager Tabs"));

	TabManager->RegisterTabSpawner(DeviceBrowserTabId, FOnSpawnTab::CreateRaw(this, &SDeviceManager::HandleTabManagerSpawnTab, DeviceBrowserTabId))
		.SetDisplayName(LOCTEXT("DeviceBrowserTabTitle", "Device Browser"))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "DeviceDetails.Tabs.Tools"))
		.SetGroup(AppMenuGroup);
/*
	TabManager->RegisterTabSpawner(DeviceDetailsTabId, FOnSpawnTab::CreateRaw(this, &SDeviceManager::HandleTabManagerSpawnTab, DeviceDetailsTabId))
		.SetDisplayName(LOCTEXT("DeviceDetailsTabTitle", "Device Details"))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "DeviceDetails.Tabs.Tools"))
		.SetGroup(AppMenuGroup);
*/
	TabManager->RegisterTabSpawner(DeviceAppsTabId, FOnSpawnTab::CreateRaw(this, &SDeviceManager::HandleTabManagerSpawnTab, DeviceAppsTabId))
		.SetDisplayName(LOCTEXT("DeviceAppsTabTitle", "Deployed Apps"))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "DeviceDetails.Tabs.Tools"))
		.SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(DeviceProcessesTabId, FOnSpawnTab::CreateRaw(this, &SDeviceManager::HandleTabManagerSpawnTab, DeviceProcessesTabId))
		.SetDisplayName(LOCTEXT("DeviceProcessesTabTitle", "Running Processes"))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "DeviceDetails.Tabs.Tools"))
		.SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(DeviceToolbarTabId, FOnSpawnTab::CreateRaw(this, &SDeviceManager::HandleTabManagerSpawnTab, DeviceToolbarTabId))
		.SetDisplayName(LOCTEXT("DeviceToolbarTabTitle", "Toolbar"))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "DeviceDetails.Tabs.Tools"))
		.SetGroup(AppMenuGroup);

	// create tab layout
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("DeviceManagerLayout_v1.1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
						->AddTab(DeviceToolbarTabId, ETabState::OpenedTab)
						->SetHideTabWell(true)
				)
				->Split
				(
					FTabManager::NewSplitter()
						->SetOrientation(Orient_Horizontal)
						->SetSizeCoefficient(0.5f)
						->Split
						(
							FTabManager::NewStack()
								->AddTab(DeviceBrowserTabId, ETabState::OpenedTab)
								->SetHideTabWell(true)
								->SetSizeCoefficient(0.7f)
						)
/*						->Split
						(
							FTabManager::NewStack()
								->AddTab(DeviceDetailsTabId, ETabState::OpenedTab)
								->SetHideTabWell(true)
								->SetSizeCoefficient(0.3f)
						)*/
				)
				->Split
				(
					FTabManager::NewStack()
						->AddTab(DeviceAppsTabId, ETabState::ClosedTab)
						->AddTab(DeviceProcessesTabId, ETabState::OpenedTab)
						->SetSizeCoefficient(0.5f)						
				)
		);

	// create & initialize main menu
	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());

	MenuBarBuilder.AddPullDownMenu(
		LOCTEXT("WindowMenuLabel", "Window"),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateStatic(&SDeviceManager::FillWindowMenu, RootMenuGroup, AppMenuGroup, TabManager),
		"Window"
	);

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MenuBarBuilder.MakeWidget()
			]
	
		+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				TabManager->RestoreFrom(Layout, ConstructUnderWindow).ToSharedRef()
			]
	];
}


/* SDeviceManager implementation
 *****************************************************************************/

void SDeviceManager::FillWindowMenu( FMenuBuilder& MenuBuilder, TSharedRef<FWorkspaceItem> RootMenuGroup, TSharedRef<FWorkspaceItem> AppMenuGroup, const TSharedPtr<FTabManager> TabManager )
{
	if (!TabManager.IsValid())
	{
		return;
	}

	MenuBuilder.BeginSection("WindowLocalTabSpawners", LOCTEXT("DeviceManagerMenuGroup", "Device Manager"));
	{
		TabManager->PopulateTabSpawnerMenu(MenuBuilder, AppMenuGroup);
	}
	MenuBuilder.EndSection();

#if !WITH_EDITOR
	MenuBuilder.BeginSection("WindowGlobalTabSpawners", LOCTEXT("UfeMenuGroup", "Unreal Frontend"));
	{
		FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, RootMenuGroup);
	}
	MenuBuilder.EndSection();
#endif //!WITH_EDITOR
}


/* SDeviceManager callbacks
 *****************************************************************************/

TSharedRef<SDockTab> SDeviceManager::HandleTabManagerSpawnTab( const FSpawnTabArgs& Args, FName TabIdentifier )
{
	TSharedPtr<SWidget> TabWidget = SNullWidget::NullWidget;
	bool AutoSizeTab = false;

	if (TabIdentifier == DeviceAppsTabId)
	{
		TabWidget = SNew(SDeviceApps, Model);
	}
	else if (TabIdentifier == DeviceBrowserTabId)
	{
		TabWidget = SNew(SDeviceBrowser, Model, DeviceServiceManager.ToSharedRef());
	}
	else if (TabIdentifier == DeviceDetailsTabId)
	{
		TabWidget = SNew(SDeviceDetails, Model);
	}
	else if (TabIdentifier == DeviceProcessesTabId)
	{
		TabWidget = SNew(SDeviceProcesses, Model);
	}
	else if (TabIdentifier == DeviceToolbarTabId)
	{
		TabWidget = SNew(SDeviceToolbar, Model);
		AutoSizeTab = true;
	}

	return SNew(SDockTab)
		.ShouldAutosize(AutoSizeTab)
		.TabRole(ETabRole::PanelTab)
		[
			TabWidget.ToSharedRef()
		];
}


#undef LOCTEXT_NAMESPACE