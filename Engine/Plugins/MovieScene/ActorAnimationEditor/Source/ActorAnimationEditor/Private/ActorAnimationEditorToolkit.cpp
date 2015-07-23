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


#define LOCTEXT_NAMESPACE "ActorAnimationEditor"


/* Local constants
 *****************************************************************************/

const FName FActorAnimationEditorToolkit::SequencerMainTabId(TEXT("Sequencer_SequencerMain"));

namespace SequencerDefs
{
	static const FName SequencerAppIdentifier(TEXT("SequencerApp"));
}


/* FActorAnimationEditorToolkit structors
 *****************************************************************************/

FActorAnimationEditorToolkit::FActorAnimationEditorToolkit(const TSharedRef<ISlateStyle>& InStyle)
	: Style(InStyle)
{
	// register sequencer menu extenders
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	int32 NewIndex = SequencerModule.GetMenuExtensibilityManager()->GetExtenderDelegates().Add(
		FAssetEditorExtender::CreateRaw(this, &FActorAnimationEditorToolkit::HandleMenuExtensibilityGetExtender));
	SequencerExtenderHandle = SequencerModule.GetMenuExtensibilityManager()->GetExtenderDelegates()[NewIndex].GetHandle();
}


FActorAnimationEditorToolkit::~FActorAnimationEditorToolkit()
{
	Sequencer->Close();

	// unregister delegates
	if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
	{
		auto& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		LevelEditorModule.OnMapChanged().RemoveAll(this);
	}

	// unregister sequencer menu extenders
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	SequencerModule.GetMenuExtensibilityManager()->GetExtenderDelegates().RemoveAll([this](const FAssetEditorExtender& Extender)
	{
		return SequencerExtenderHandle == Extender.GetHandle();
	});
}


/* FActorAnimationEditorToolkit interface
 *****************************************************************************/

void FActorAnimationEditorToolkit::Initialize( const EToolkitMode::Type Mode, const FSequencerViewParams& InViewParams, const TSharedPtr<IToolkitHost>& InitToolkitHost, UActorAnimation* ActorAnimation, bool bEditWithinLevelEditor )
{
	// create tab layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_ActorAnimationEditor")
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

	// initialize sequencer
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

		LevelEditorModule.AttachSequencer(Sequencer->GetSequencerWidget(), SharedThis(this));

		// We need to find out when the user loads a new map, because we might need to re-create puppet actors
		// when previewing a MovieScene
		LevelEditorModule.OnMapChanged().AddRaw(this, &FActorAnimationEditorToolkit::HandleMapChanged);
	}
}


/* IToolkit interface
 *****************************************************************************/

FText FActorAnimationEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Actor Animation Editor");
}


FName FActorAnimationEditorToolkit::GetToolkitFName() const
{
	static FName SequencerName("ActorAnimationEditor");
	return SequencerName;
}


FLinearColor FActorAnimationEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.7, 0.0f, 0.0f, 0.5f);
}


FString FActorAnimationEditorToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Sequencer ").ToString();
}


void FActorAnimationEditorToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	if (IsWorldCentricAssetEditor())
	{
		return;
	}

	WorkspaceMenuCategory = TabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_SequencerAssetEditor", "Sequencer"));

	TabManager->RegisterTabSpawner(SequencerMainTabId, FOnSpawnTab::CreateSP(this, &FActorAnimationEditorToolkit::HandleTabManagerSpawnTab))
		.SetDisplayName(LOCTEXT("SequencerMainTab", "Sequencer"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(Style->GetStyleSetName(), "ActorAnimationEditor.Tabs.Sequencer"));
}


void FActorAnimationEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	if (!IsWorldCentricAssetEditor())
	{
		TabManager->UnregisterTabSpawner(SequencerMainTabId);
	}

	// @todo remove when world-centric mode is added
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.AttachSequencer(SNullWidget::NullWidget, nullptr);
}


/* FActorAnimationEditorToolkit callbacks
 *****************************************************************************/

void FActorAnimationEditorToolkit::HandleAddComponentActionExecute( UActorComponent* Component )
{
	Sequencer->GetHandleToObject( Component );
}


void FActorAnimationEditorToolkit::HandleMapChanged(class UWorld* NewWorld, EMapChangeType MapChangeType)
{
	Sequencer->NotifyMapChanged(NewWorld, MapChangeType);
}


TSharedRef<FExtender> FActorAnimationEditorToolkit::HandleMenuExtensibilityGetExtender( const TSharedRef<FUICommandList> CommandList, const TArray<UObject*> ContextSensitiveObjects )
{
	TSharedRef<FExtender> AddTrackMenuExtender(new FExtender());
	AddTrackMenuExtender->AddMenuExtension(
		SequencerMenuExtensionPoints::AddTrackMenu_PropertiesSection,
		EExtensionHook::Before,
		CommandList,
		FMenuExtensionDelegate::CreateRaw(this, &FActorAnimationEditorToolkit::HandleTrackMenuExtensionAddTrack, ContextSensitiveObjects));

	return AddTrackMenuExtender;
}


TSharedRef<SDockTab> FActorAnimationEditorToolkit::HandleTabManagerSpawnTab(const FSpawnTabArgs& Args)
{
	TSharedPtr<SWidget> TabWidget = SNullWidget::NullWidget;

	if (Args.GetTabId() == SequencerMainTabId)
	{
		TabWidget = Sequencer->GetSequencerWidget();
	}

	return SNew(SDockTab)
		.Label(LOCTEXT("SequencerMainTitle", "Sequencer"))
		.TabColorScale(GetTabColorScale())
		.TabRole(ETabRole::PanelTab)
		[
			TabWidget.ToSharedRef()
		];
}


void FActorAnimationEditorToolkit::HandleTrackMenuExtensionAddTrack( FMenuBuilder& AddTrackMenuBuilder, TArray<UObject*> ContextObjects )
{
	if (ContextObjects.Num() != 1)
	{
		return;
	}

	AActor* Actor = Cast<AActor>(ContextObjects[0]);
	
	if (Actor == nullptr)
	{
		return;
	}

	AddTrackMenuBuilder.BeginSection("Components", LOCTEXT("ComponentsSection", "Components"));
	{
		for (UActorComponent* Component : Actor->GetComponents())
		{
			FUIAction AddComponentAction(FExecuteAction::CreateSP(this, &FActorAnimationEditorToolkit::HandleAddComponentActionExecute, Component));
			FText AddComponentLabel = FText::FromString(Component->GetName());
			FText AddComponentToolTip = FText::Format(LOCTEXT("ComponentToolTipFormat", "Add {0} component"), FText::FromString(Component->GetName()));
			AddTrackMenuBuilder.AddMenuEntry(AddComponentLabel, AddComponentToolTip, FSlateIcon(), AddComponentAction);
		}
	}
	AddTrackMenuBuilder.EndSection();
}


#undef LOCTEXT_NAMESPACE
