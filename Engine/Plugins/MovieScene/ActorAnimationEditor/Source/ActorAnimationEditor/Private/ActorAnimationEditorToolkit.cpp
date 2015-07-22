// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ActorAnimationEditorPrivatePCH.h"
#include "Toolkits/IToolkitHost.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/LevelEditor/Public/ILevelViewport.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "ISequencer.h"
#include "ISequencerModule.h"
#include "SDockTab.h"


#define LOCTEXT_NAMESPACE "Sequencer"

const FName FActorAnimationEditorToolkit::SequencerMainTabId(TEXT("Sequencer_SequencerMain"));


namespace SequencerDefs
{
	static const FName SequencerAppIdentifier( TEXT( "SequencerApp" ) );
}


void FActorAnimationEditorToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	if (IsWorldCentricAssetEditor())
	{
		return;
	}

	WorkspaceMenuCategory = TabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_SequencerAssetEditor", "Sequencer"));

	TabManager->RegisterTabSpawner(SequencerMainTabId, FOnSpawnTab::CreateSP(this, &FActorAnimationEditorToolkit::SpawnTab_SequencerMain))
		.SetDisplayName(LOCTEXT("SequencerMainTab", "Sequencer"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}


void FActorAnimationEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	if (!IsWorldCentricAssetEditor())
	{
		TabManager->UnregisterTabSpawner( SequencerMainTabId );
	}
	
	// @todo remove when world-centric mode is added
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.AttachSequencer( SNullWidget::NullWidget, nullptr );
}


void FActorAnimationEditorToolkit::Initialize( const EToolkitMode::Type Mode, const FSequencerViewParams& InViewParams, const TSharedPtr<IToolkitHost>& InitToolkitHost, UActorAnimation* ActorAnimation, bool bEditWithinLevelEditor )
{
	{
		const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_Sequencer_Layout")
			->AddArea
			(
				FTabManager::NewPrimaryArea()
					->Split
					(
						FTabManager::NewStack()
						->AddTab(SequencerMainTabId, ETabState::OpenedTab)
					)
			);

		const bool bCreateDefaultStandaloneMenu = true;
		const bool bCreateDefaultToolbar = false;
		FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, SequencerDefs::SequencerAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ActorAnimation);
	}
	
	// Init Sequencer
	FSequencerInitParams SequencerInitParams;
	{
		SequencerInitParams.ViewParams = InViewParams;
		SequencerInitParams.Animation = ActorAnimation;
		SequencerInitParams.bEditWithinLevelEditor = bEditWithinLevelEditor;
		SequencerInitParams.ToolkitHost = InitToolkitHost;
	}

	Sequencer = FModuleManager::LoadModuleChecked<ISequencerModule>("Sequencer").CreateSequencer(SequencerInitParams);

	if (bEditWithinLevelEditor)
	{
		// @todo remove when world-centric mode is added
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

		LevelEditorModule.AttachSequencer( Sequencer->GetSequencerWidget(), SharedThis(this));
			
		// We need to find out when the user loads a new map, because we might need to re-create puppet actors
		// when previewing a MovieScene
		LevelEditorModule.OnMapChanged().AddRaw(this, &FActorAnimationEditorToolkit::HandleMapChanged);
	}
}


FActorAnimationEditorToolkit::FActorAnimationEditorToolkit()
{
	// Register sequencer menu extenders.
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>( "Sequencer" );
	int32 NewIndex = SequencerModule.GetMenuExtensibilityManager()->GetExtenderDelegates().Add(
		FAssetEditorExtender::CreateRaw( this, &FActorAnimationEditorToolkit::GetContextSensitiveSequencerExtender ) );
	SequencerExtenderHandle = SequencerModule.GetMenuExtensibilityManager()->GetExtenderDelegates()[NewIndex].GetHandle();
}


FActorAnimationEditorToolkit::~FActorAnimationEditorToolkit()
{
	Sequencer->Close();

	// Unregister delegates
	if( FModuleManager::Get().IsModuleLoaded( TEXT( "LevelEditor" ) ) )
	{
		auto& LevelEditorModule = FModuleManager::LoadModuleChecked< FLevelEditorModule >( TEXT( "LevelEditor" ) );
		LevelEditorModule.OnMapChanged().RemoveAll( this );
	}

	// Un-Register sequencer menu extenders.
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>( "Sequencer" );
	SequencerModule.GetMenuExtensibilityManager()->GetExtenderDelegates().RemoveAll( [this]( const FAssetEditorExtender& Extender )
	{
		return SequencerExtenderHandle == Extender.GetHandle();
	} );
}


TSharedRef<SDockTab> FActorAnimationEditorToolkit::SpawnTab_SequencerMain(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == SequencerMainTabId);

	return SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("Sequencer.Tabs.SequencerMain") )
		.Label( LOCTEXT("SequencerMainTitle", "Sequencer") )
		.TabColorScale( GetTabColorScale() )
		[
			Sequencer->GetSequencerWidget()
		];
}


FName FActorAnimationEditorToolkit::GetToolkitFName() const
{
	static FName SequencerName("Sequencer");
	return SequencerName;
}


FText FActorAnimationEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Sequencer");
}


FLinearColor FActorAnimationEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.7, 0.0f, 0.0f, 0.5f );
}


FString FActorAnimationEditorToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Sequencer ").ToString();
}

TSharedRef<FExtender> FActorAnimationEditorToolkit::GetContextSensitiveSequencerExtender( const TSharedRef<FUICommandList> CommandList, const TArray<UObject*> ContextSensitiveObjects )
{
	TSharedRef<FExtender> AddTrackMenuExtender( new FExtender() );
	AddTrackMenuExtender->AddMenuExtension(
		SequencerMenuExtensionPoints::AddTrackMenu_PropertiesSection,
		EExtensionHook::Before,
		CommandList,
		FMenuExtensionDelegate::CreateRaw( this, &FActorAnimationEditorToolkit::ExtendSequencerAddTrackMenu, ContextSensitiveObjects ) );
	return AddTrackMenuExtender;
}

void FActorAnimationEditorToolkit::ExtendSequencerAddTrackMenu( FMenuBuilder& AddTrackMenuBuilder, TArray<UObject*> ContextObjects )
{
	if ( ContextObjects.Num() == 1 )
	{
		AActor* Actor = Cast<AActor>( ContextObjects[0] );
		if ( Actor != nullptr )
		{
			AddTrackMenuBuilder.BeginSection( "Components", LOCTEXT( "ComponentsSection", "Components" ) );
			{
				for ( UActorComponent* Component : Actor->GetComponents() )
				{
					FUIAction AddComponentAction( FExecuteAction::CreateSP( this, &FActorAnimationEditorToolkit::AddComponentTrack, Component ) );
					FText AddComponentLabel = FText::FromString(Component->GetName());
					FText AddComponentToolTip = FText::Format( LOCTEXT( "ComponentToolTipFormat", "Add {0} component" ), FText::FromString( Component->GetName() ) );
					AddTrackMenuBuilder.AddMenuEntry( AddComponentLabel, AddComponentToolTip, FSlateIcon(), AddComponentAction );
				}
			}
			AddTrackMenuBuilder.EndSection();
		}
	}
}

void FActorAnimationEditorToolkit::AddComponentTrack( UActorComponent* Component )
{
	Sequencer->GetHandleToObject( Component );
}


void FActorAnimationEditorToolkit::HandleMapChanged(class UWorld* NewWorld, EMapChangeType MapChangeType)
{
	Sequencer->NotifyMapChanged(NewWorld, MapChangeType);
}


#undef LOCTEXT_NAMESPACE
