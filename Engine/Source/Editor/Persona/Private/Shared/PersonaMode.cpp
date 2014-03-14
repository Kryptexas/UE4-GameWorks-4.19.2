// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PersonaPrivatePCH.h"
#include "PersonaMode.h"
#include "Persona.h"
#include "SSkeletonAnimNotifies.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "PersonaModes"

/////////////////////////////////////////////////////
// FPersonaTabs

// Tab constants
const FName FPersonaTabs::MorphTargetsID("MorphTargetsTab");
const FName FPersonaTabs::SkeletonTreeViewID("SkeletonTreeView");		//@TODO: Name
// Skeleton Pose manager
const FName FPersonaTabs::RetargetSourceManagerID("RetargetSourceManager");
// Anim Blueprint params
// Explorer
// Blueprint Defaults
const FName FPersonaTabs::AnimBlueprintDefaultsEditorID("AnimBlueprintDefaultsEditor");
// Anim Document
const FName FPersonaTabs::ScrubberID("ScrubberTab");

// Toolbar
const FName FPersonaTabs::PreviewViewportID("Viewport");		//@TODO: Name
const FName FPersonaTabs::AssetBrowserID("SequenceBrowser");	//@TODO: Name
const FName FPersonaTabs::MirrorSetupID("MirrorSetupTab");
const FName FPersonaTabs::AnimBlueprintDebugHistoryID("AnimBlueprintDebugHistoryTab");
const FName FPersonaTabs::AnimAssetPropertiesID("AnimAssetPropertiesTab");
const FName FPersonaTabs::MeshAssetPropertiesID("MeshAssetPropertiesTab");
const FName FPersonaTabs::PreviewManagerID("AnimPreviewSetup");		//@TODO: Name
const FName FPersonaTabs::SkeletonAnimNotifiesID("SkeletonAnimNotifies");

/////////////////////////////////////////////////////
// FPersonaMode

// Mode constants
const FName FPersonaModes::SkeletonDisplayMode( "SkeletonName" );
const FName FPersonaModes::MeshEditMode( "MeshName" );
const FName FPersonaModes::PhysicsEditMode( "PhysicsName" );
const FName FPersonaModes::AnimationEditMode( "AnimationName" );
const FName FPersonaModes::AnimBlueprintEditMode( "GraphName" );

/////////////////////////////////////////////////////
// FPersonaAppMode

FPersonaAppMode::FPersonaAppMode(TSharedPtr<class FPersona> InPersona, FName InModeName)
	: FApplicationMode(InModeName)
{
	MyPersona = InPersona;

	PersonaTabFactories.RegisterFactory(MakeShareable(new FSkeletonTreeSummoner(InPersona)));
	PersonaTabFactories.RegisterFactory(MakeShareable(new FAnimationAssetBrowserSummoner(InPersona)));
	PersonaTabFactories.RegisterFactory(MakeShareable(new FPreviewViewportSummoner(InPersona)));
	PersonaTabFactories.RegisterFactory(MakeShareable(new FMorphTargetTabSummoner(InPersona)));
	PersonaTabFactories.RegisterFactory(MakeShareable(new FSkeletonAnimNotifiesSummoner(InPersona)));
	PersonaTabFactories.RegisterFactory(MakeShareable(new FRetargetSourceManagerTabSummoner(InPersona)));
}

void FPersonaAppMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	TSharedPtr<FBlueprintEditor> BP = MyPersona.Pin();
	
	BP->RegisterToolbarTab(InTabManager.ToSharedRef());

	// Mode-specific setup
	BP->PushTabFactories(PersonaTabFactories);
}

/////////////////////////////////////////////////////
// FPersonaModeSharedData

FPersonaModeSharedData::FPersonaModeSharedData()
	: OrthoZoom(1.0f)
	, bCameraLock(true)
	, bCameraFollow(false)
	, bShowReferencePose(false)
	, bShowBones(false)
	, bShowBoneNames(false)
	, bShowSockets(false)
	, bShowBound(false)
	, ViewportType(0)
	, PlaybackSpeedMode(0)
	, LocalAxesMode(0)
{}

/////////////////////////////////////////////////////
// FSkeletonTreeSummoner

#include "SSkeletonTree.h"

FSkeletonTreeSummoner::FSkeletonTreeSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(FPersonaTabs::SkeletonTreeViewID, InHostingApp)
{
	TabLabel = LOCTEXT("SkeletonTreeTabTitle", "Skeleton Tree");

	EnableTabPadding();
	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("SkeletonTreeView", "Skeleton Tree");
	ViewMenuTooltip = LOCTEXT("SkeletonTreeView_ToolTip", "Shows the skeleton tree");
}

TSharedRef<SWidget> FSkeletonTreeSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SSkeletonTree)
		.Persona(StaticCastSharedPtr<FPersona>(HostingApp.Pin()));
}

/////////////////////////////////////////////////////
// FMorphTargetTabSummoner

#include "SMorphTargetViewer.h"

FMorphTargetTabSummoner::FMorphTargetTabSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(FPersonaTabs::MorphTargetsID, InHostingApp)
{
	TabLabel = LOCTEXT("MorphTargetTabTitle", "Morph Target Previewer");

	EnableTabPadding();
	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("MorphTargetTabView", "Morph Target Previewer");
	ViewMenuTooltip = LOCTEXT("MorphTargetTabView_ToolTip", "Shows the morph target viewer");
}

TSharedRef<SWidget> FMorphTargetTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SMorphTargetViewer)
		.Persona(StaticCastSharedPtr<FPersona>(HostingApp.Pin()));
}

/////////////////////////////////////////////////////
// FAnimationAssetBrowserSummoner

#include "SAnimationSequenceBrowser.h"

FAnimationAssetBrowserSummoner::FAnimationAssetBrowserSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(FPersonaTabs::AssetBrowserID, InHostingApp)
{
	TabLabel = LOCTEXT("AssetBrowserTabTitle", "Asset Browser");
	TabIcon = FEditorStyle::GetBrush("LevelEditor.Tabs.ContentBrowser");

	EnableTabPadding();
	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("AssetBrowser", "Asset Browser");
	ViewMenuTooltip = LOCTEXT("AssetBrowser_ToolTip", "Shows the animation asset browser");
}

TSharedRef<SWidget> FAnimationAssetBrowserSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SAnimationSequenceBrowser)
		.Persona(StaticCastSharedPtr<FPersona>(HostingApp.Pin()));
}

/////////////////////////////////////////////////////
// FPreviewViewportSummoner

#include "SAnimationEditorViewport.h"

FPreviewViewportSummoner::FPreviewViewportSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(FPersonaTabs::PreviewViewportID, InHostingApp)
{
	TabLabel = LOCTEXT("ViewportTabTitle", "Viewport");

	bIsSingleton = true;

	EnableTabPadding();

	ViewMenuDescription = LOCTEXT("ViewportView", "Viewport");
	ViewMenuTooltip = LOCTEXT("ViewportView_ToolTip", "Shows the viewport");
}

TSharedRef<SWidget> FPreviewViewportSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FPersona> PersonaPtr = StaticCastSharedPtr<FPersona>(HostingApp.Pin());

	TSharedRef<SAnimationEditorViewportTabBody> NewViewport = SNew(SAnimationEditorViewportTabBody)
		.Persona(PersonaPtr)
		.Skeleton(PersonaPtr->GetSkeleton());

	//@TODO:MODES:check(!PersonaPtr->Viewport.IsValid());
	// mode switch data sharing
	// we saves data from previous viewport
	// and restore after switched
	bool bRestoreData = PersonaPtr->Viewport.IsValid();
	if (bRestoreData)
	{
		NewViewport.Get().SaveData(PersonaPtr->Viewport.Pin().Get());
	}

	PersonaPtr->SetViewport(NewViewport);

	if (bRestoreData)
	{
		NewViewport.Get().RestoreData();
	}

	return NewViewport;
}

/////////////////////////////////////////////////////
// FRetargetSourceManagerTabSummoner

#include "SRetargetSourceManager.h"

FRetargetSourceManagerTabSummoner::FRetargetSourceManagerTabSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(FPersonaTabs::RetargetSourceManagerID, InHostingApp)
{
	TabLabel = LOCTEXT("RetargetSourceManagerTabTitle", "Retarget Source Manager");

	EnableTabPadding();
	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("RetargetSourceManagerTabView", "Retarget Source Manager");
	ViewMenuTooltip = LOCTEXT("RetargetSourceManagerTabView_ToolTip", "Manages different source pose for animations (retargeting)");
}

TSharedRef<SWidget> FRetargetSourceManagerTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SRetargetSourceManager)
		.Persona(StaticCastSharedPtr<FPersona>(HostingApp.Pin()));
}

/////////////////////////////////////////////////////
// FAnimBlueprintDefaultsEditorSummoner

#include "SKismetInspector.h"

FAnimBlueprintDefaultsEditorSummoner::FAnimBlueprintDefaultsEditorSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(FPersonaTabs::AnimBlueprintDefaultsEditorID, InHostingApp)
{
	TabLabel = LOCTEXT("AnimBlueprintDefaultsTabTitle", "Animation Blueprint Editor");

	bIsSingleton = true;

	CurrentMode = EAnimBlueprintEditorMode::PreviewMode;

	ViewMenuDescription = LOCTEXT("AnimBlueprintDefaultsView", "Defaults");
	ViewMenuTooltip = LOCTEXT("AnimBlueprintDefaultsView_ToolTip", "Shows the animation blueprint defaults/preview editor view");
}

TSharedRef<SWidget> FAnimBlueprintDefaultsEditorSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FPersona> PersonaPtr = StaticCastSharedPtr<FPersona>(HostingApp.Pin());

	return	SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding(FMargin( 0.f, 0.f, 2.f, 0.f ))
				[
					SNew(SBorder)
					.BorderImage(this, &FAnimBlueprintDefaultsEditorSummoner::GetBorderBrushByMode, EAnimBlueprintEditorMode::PreviewMode )
					.Padding(0)
					[
						SNew(SCheckBox)
						.Style(FEditorStyle::Get(), "RadioButton")
						.IsChecked( this, &FAnimBlueprintDefaultsEditorSummoner::IsChecked, EAnimBlueprintEditorMode::PreviewMode )
						.OnCheckStateChanged( this, &FAnimBlueprintDefaultsEditorSummoner::OnCheckedChanged, EAnimBlueprintEditorMode::PreviewMode )
						.ToolTip(IDocumentation::Get()->CreateToolTip(	LOCTEXT("AnimBlueprintPropertyEditorPreviewMode", "Switch to editing the preview instance properties"),
																		NULL,
																		TEXT("Shared/Editors/Persona"),
																		TEXT("AnimBlueprintPropertyEditorPreviewMode")))
						[
							SNew( STextBlock )
							.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 9 ) )
							.Text( LOCTEXT("AnimBlueprintDefaultsPreviewMode", "Edit Preview").ToString() )
						]
					]
				]
				+SHorizontalBox::Slot()
				.Padding(FMargin( 2.f, 0.f, 0.f, 0.f ))
				[
					SNew(SBorder)
					.BorderImage(this, &FAnimBlueprintDefaultsEditorSummoner::GetBorderBrushByMode, EAnimBlueprintEditorMode::DefaultsMode )
					.Padding(0)
					[
						SNew(SCheckBox)
						.Style(FEditorStyle::Get(), "RadioButton")
						.IsChecked( this, &FAnimBlueprintDefaultsEditorSummoner::IsChecked, EAnimBlueprintEditorMode::DefaultsMode )
						.OnCheckStateChanged( this, &FAnimBlueprintDefaultsEditorSummoner::OnCheckedChanged, EAnimBlueprintEditorMode::DefaultsMode )
						.ToolTip(IDocumentation::Get()->CreateToolTip(	LOCTEXT("AnimBlueprintPropertyEditorDefaultMode", "Switch to editing the blueprint property defaults"),
																		NULL,
																		TEXT("Shared/Editors/Persona"),
																		TEXT("AnimBlueprintPropertyEditorDefaultMode")))
						[
							SNew( STextBlock )
							.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 9 ) )
							.Text( LOCTEXT("AnimBlueprintDefaultsDefaultsMode", "Edit Defaults").ToString() )
						]
					]
				]
			]
			+SVerticalBox::Slot()
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					SNew(SBorder)
					.Padding(0)
					.BorderImage( FEditorStyle::GetBrush("NoBorder") )
					.Visibility( this, &FAnimBlueprintDefaultsEditorSummoner::IsEditorVisible, EAnimBlueprintEditorMode::PreviewMode)
					[
						PersonaPtr->GetPreviewEditor()
					]
				]
				+SOverlay::Slot()
				[
					SNew(SBorder)
					.Padding(0)
					.BorderImage( FEditorStyle::GetBrush("NoBorder") )
					.Visibility( this, &FAnimBlueprintDefaultsEditorSummoner::IsEditorVisible, EAnimBlueprintEditorMode::DefaultsMode)
					[
						PersonaPtr->GetDefaultEditor()
					]
				]
			];
}

FText FAnimBlueprintDefaultsEditorSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("AnimBlueprintDefaultsEditorTooltip", "The editor lets you set the default values for all variables in your Blueprint or lets you change the values of the preview instance, depending on mode");
}

EVisibility FAnimBlueprintDefaultsEditorSummoner::IsEditorVisible(EAnimBlueprintEditorMode::Type Mode) const
{
	return CurrentMode == Mode ? EVisibility::Visible: EVisibility::Hidden;
}

ESlateCheckBoxState::Type FAnimBlueprintDefaultsEditorSummoner::IsChecked(EAnimBlueprintEditorMode::Type Mode) const
{
	return CurrentMode == Mode ? ESlateCheckBoxState::Checked: ESlateCheckBoxState::Unchecked;
}

const FSlateBrush* FAnimBlueprintDefaultsEditorSummoner::GetBorderBrushByMode(EAnimBlueprintEditorMode::Type Mode) const
{
	if(Mode == CurrentMode)
	{
		return FEditorStyle::GetBrush("ModeSelector.ToggleButton.Pressed");
	}
	else
	{
		return FEditorStyle::GetBrush("ModeSelector.ToggleButton.Normal");
	}
}

void FAnimBlueprintDefaultsEditorSummoner::OnCheckedChanged(ESlateCheckBoxState::Type NewType, EAnimBlueprintEditorMode::Type Mode)
{
	if(NewType == ESlateCheckBoxState::Checked)
	{
		CurrentMode = Mode;
	}
}
#undef LOCTEXT_NAMESPACE
