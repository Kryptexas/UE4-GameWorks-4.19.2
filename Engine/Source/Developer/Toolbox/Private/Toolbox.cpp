// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Toolbox.h"
#include "ModuleManager.h"
#include "GammaUI.h"
#include "ModuleUIInterface.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "MainFrame.h"

IMPLEMENT_MODULE( FToolboxModule, Toolbox );


static bool CanShowModulesTab()
{
	return FModuleManager::Get().IsSolutionFilePresent();
}

class SDebugPanel : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SDebugPanel){}
	SLATE_END_ARGS()

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct( const FArguments& )
	{
		ChildSlot
		[
			SNew( SVerticalBox )
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 4.0f )
			.HAlign(HAlign_Left)
			[
				SNew( SButton )
				.Text( NSLOCTEXT("DeveloperToolbox", "ReloadTextures", "Reload Textures") )
				.OnClicked( this, &SDebugPanel::OnReloadTexturesClicked )
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 4.0f )
			.HAlign(HAlign_Left)
			[
				SNew( SButton )
				.Text( NSLOCTEXT("DeveloperToolbox", "FlushFontCache", "Flush Font Cache") )
				.OnClicked( this, &SDebugPanel::OnFlushFontCacheClicked )
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 4.0f )
			.HAlign(HAlign_Left)
			[
				SNew( SButton )
				.Text( NSLOCTEXT("DeveloperToolbox", "TestNotifications", "Test Notifications") )
				.OnClicked( this, &SDebugPanel::OnTestNotificationsClicked )
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 4.0f )
			.HAlign(HAlign_Left)
			[
				SNew( SButton )
				.Text( NSLOCTEXT("DeveloperToolbox", "DisplayTextureAtlases", "Display Texture Atlases") )
				.OnClicked( this, &SDebugPanel::OnDisplayTextureAtlases )
			]
		];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	
	FReply OnReloadTexturesClicked()
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
		return FReply::Handled();
	}

	FReply OnDisplayTextureAtlases()
	{
		FSlateApplication::Get().GetRenderer()->DisplayTextureAtlases();
		return FReply::Handled();
	}

	FReply OnFlushFontCacheClicked()
	{
		FSlateApplication::Get().GetRenderer()->FlushFontCache();
		return FReply::Handled();
	}

	FReply OnTestNotificationsClicked()
	{
		static int32 Counter = 0;
		FFormatNamedArguments Args;
		Args.Add( TEXT("CounterNumber"), Counter++ );
		FNotificationInfo Info( FText::Format( NSLOCTEXT("DeveloperToolbox", "TestNotificationCounter", "Test Notification {CounterNumber}"), Args ) );
		Info.bFireAndForget = true;
		Info.ExpireDuration = 3.0f;
		Info.FadeOutDuration = 3.0f;

		FSlateNotificationManager::Get().AddNotification(Info);
		return FReply::Handled();
	}

};

TSharedRef<SDockTab> CreateDebugToolsTab( const FSpawnTabArgs& Args )
{
	FGammaUI& GammaUI = FModuleManager::Get().LoadModuleChecked<FGammaUI>( FName("GammaUI") );

	return
		SNew(SDockTab)
		.TabRole( ETabRole::NomadTab )
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew( SDebugPanel )
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				GammaUI.GetGammaUIPanel().ToSharedRef()
			]
		];
}

TSharedRef<SDockTab> CreateModulesTab( const FSpawnTabArgs& Args )
{
	IModuleUIInterface& ModuleUI = FModuleManager::Get().LoadModuleChecked<IModuleUIInterface>( FName("ModuleUI") );

	return
		SNew(SDockTab)
		.TabRole( ETabRole::NomadTab )
		[
			SNew(SBorder)
			.Padding(2)
			.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
			[
				ModuleUI.GetModuleUIWidget()
			]
		];
}



void FToolboxModule::StartupModule()
{
	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner( "DebugTools", FOnSpawnTab::CreateStatic( &CreateDebugToolsTab ) )
		.SetDisplayName( NSLOCTEXT("Toolbox", "DebugTools", "Debug Tools") )
		.SetGroup( MenuStructure.GetDeveloperToolsCategory() )
		.SetIcon( FSlateIcon(FEditorStyle::GetStyleSetName(), "DebugTools.TabIcon") );
	if ( CanShowModulesTab() )
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner( "ModulesTab", FOnSpawnTab::CreateStatic( &CreateModulesTab ) )
			.SetDisplayName( NSLOCTEXT("Toolbox", "Modules", "Modules") )
			.SetGroup( MenuStructure.GetDeveloperToolsCategory() )
			.SetIcon( FSlateIcon(FEditorStyle::GetStyleSetName(), "Modules.TabIcon") );
	}
}

void FToolboxModule::ShutdownModule()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("DebugTools");
		if (CanShowModulesTab())
		{
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("ModulesTab");
		}
	}
}
