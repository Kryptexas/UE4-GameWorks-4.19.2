// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"

#include "SAnimationEditorViewport.h"
#include "SAnimationScrubPanel.h"
#include "SAnimMontageScrubPanel.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "SAnimViewportToolBar.h"
#include "AnimViewportMenuCommands.h"
#include "AnimViewportShowCommands.h"
#include "AnimViewportLODCommands.h"
#include "AnimViewportPlaybackCommands.h"
#include "AnimGraphDefinitions.h"
#include "AnimationEditorViewportClient.h"
#include "ScopedTransaction.h"
#include "Editor/UnrealEd/Public/LODUtilities.h"

#define LOCTEXT_NAMESPACE "PersonaViewportToolbar"

namespace EAnimationPlaybackSpeeds
{
	// Speed scales for animation playback, must match EAnimationPlaybackSpeeds::Type
	float Values[EAnimationPlaybackSpeeds::NumPlaybackSpeeds] = {0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f};
};

//////////////////////////////////////////////////////////////////////////
// SAnimationEditorViewport

void SAnimationEditorViewport::Construct(const FArguments& InArgs, TSharedPtr<class FPersona> InPersona, TSharedPtr<class SAnimationEditorViewportTabBody> InTabBody)
{
	PersonaPtr = InPersona;
	TabBodyPtr = InTabBody;

	SEditorViewport::Construct(
		SEditorViewport::FArguments()
			.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
		);
}

TSharedRef<FEditorViewportClient> SAnimationEditorViewport::MakeEditorViewportClient()
{
	// Create an animation viewport client
	LevelViewportClient = MakeShareable(new FAnimationViewportClient(PersonaPtr.Pin()->GetPreviewScene(), PersonaPtr));

	LevelViewportClient->ViewportType = LVT_Perspective;
	LevelViewportClient->bSetListenerPosition = false;
	LevelViewportClient->SetViewLocation(EditorViewportDefs::DefaultPerspectiveViewLocation);
	LevelViewportClient->SetViewRotation(EditorViewportDefs::DefaultPerspectiveViewRotation);

	return LevelViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SAnimationEditorViewport::MakeViewportToolbar()
{
	return SNew(SAnimViewportToolBar, TabBodyPtr.Pin(), SharedThis(this))
		.Cursor(EMouseCursor::Default);
}

//////////////////////////////////////////////////////////////////////////
// 
/** //@TODO MODES: Simple text entry popup used it to get 3 vector position*/

class STranslationInputWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( STranslationInputWindow )
		: _X_Value(0.0f)
		, _Y_Value(0.0f)
		, _Z_Value(0.0f)
	{}


	SLATE_ARGUMENT( float, X_Value )
	SLATE_ARGUMENT( float, Y_Value )
	SLATE_ARGUMENT( float, Z_Value )
	SLATE_EVENT( FOnTextCommitted, OnXModified )
	SLATE_EVENT( FOnTextCommitted, OnYModified )
	SLATE_EVENT( FOnTextCommitted, OnZModified )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs )
	{
		FString XString = FString::Printf(TEXT("%0.2f"), InArgs._X_Value);
		FString YString = FString::Printf(TEXT("%0.2f"), InArgs._Y_Value);
		FString ZString = FString::Printf(TEXT("%0.2f"), InArgs._Z_Value);

		this->ChildSlot
		[
			SNew(SBorder)
			. BorderImage(FEditorStyle::GetBrush(TEXT("Menu.Background")))
			. Padding(10)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("AnimationEditorViewport", "XValueLabel", "X:"))
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SEditableTextBox)
						.MinDesiredWidth(10.0f)
						.Text( FText::FromString(XString) )
						.OnTextCommitted( InArgs._OnXModified )
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("AnimationEditorViewport", "YValueLabel", "Y:"))
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SEditableTextBox)
						.MinDesiredWidth(10.0f)
						.Text( FText::FromString(YString) )
						.OnTextCommitted( InArgs._OnYModified )
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("AnimationEditorViewport", "ZValueLabel", "Z: "))
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SEditableTextBox)
						.MinDesiredWidth(10.0f)
						.Text( FText::FromString(ZString) )
						.OnTextCommitted( InArgs._OnZModified )
					]
				]
			]
		];
	}
};

//////////////////////////////////////////////////////////////////////////
// SAnimationEditorViewportTabBody

SAnimationEditorViewportTabBody::SAnimationEditorViewportTabBody()
	: AnimationPlaybackSpeedMode(EAnimationPlaybackSpeeds::Normal)
{
}

SAnimationEditorViewportTabBody::~SAnimationEditorViewportTabBody()
{
	CleanupPersonaReferences();

	// Close viewport
	if (LevelViewportClient.IsValid())
	{
		LevelViewportClient->Viewport = NULL;
	}

	// Release our reference to the viewport client
	LevelViewportClient.Reset();
}

void SAnimationEditorViewportTabBody::CleanupPersonaReferences()
{
	if (PersonaPtr.IsValid())
	{
		PersonaPtr.Pin()->UnregisterOnAnimChanged(this);
		PersonaPtr.Reset();
	}
}

bool SAnimationEditorViewportTabBody::CanUseGizmos() const
{
	class UDebugSkelMeshComponent* Component = PersonaPtr.Pin()->PreviewComponent;

	if (Component != NULL)
	{
		if (Component->bForceRefpose)
		{
			return true;
		}
		else if (Component->IsPreviewOn())
		{
			return true;
		}
	}

	return false;
}

FString SAnimationEditorViewportTabBody::GetDisplayString() const
{
	class UDebugSkelMeshComponent* Component = PersonaPtr.Pin()->PreviewComponent;
	FString TargetSkeletonName = TargetSkeleton ?TargetSkeleton->GetName() : FName(NAME_None).ToString();

	if ( Component!=NULL )
	{
		if ( Component->bForceRefpose )
		{
			return LOCTEXT("ReferencePose", "Reference pose").ToString();
		}
		else if (Component->IsPreviewOn())
		{
			return FText::Format( LOCTEXT("Previewing", "Previewing {0}"), FText::FromString(Component->GetPreviewText()) ).ToString();
		}
		else if (Component->AnimBlueprintGeneratedClass != NULL)
		{
			const bool bWarnAboutBoneManip = !PersonaPtr.Pin()->IsModeCurrent(FPersonaModes::AnimBlueprintEditMode);
			if (bWarnAboutBoneManip)
			{
				return FText::Format(LOCTEXT("PreviewingAnimBP_WarnDisabled", "Previewing {0}. \nBone manipulation is disabled in this mode. "), FText::FromString(Component->AnimBlueprintGeneratedClass->GetName())).ToString();
			}
			else
			{
				return FText::Format(LOCTEXT("PreviewingAnimBP", "Previewing {0}"), FText::FromString(Component->AnimBlueprintGeneratedClass->GetName())).ToString();
			}
		}
		else if (Component->SkeletalMesh == NULL)
		{
			return FText::Format( LOCTEXT("NoMeshFound", "No skeletal mesh found for skeleton '{0}'"), FText::FromString(TargetSkeletonName) ).ToString();
		}
		else 
		{
			return LOCTEXT("NothingToPlay", "Nothing to play").ToString();
		}
	}
	else
	{
		return FText::Format( LOCTEXT("NoMeshFound", "No skeletal mesh found for skeleton '{0}'"), FText::FromString(TargetSkeletonName) ).ToString();
	}
}

void SAnimationEditorViewportTabBody::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void SAnimationEditorViewportTabBody::RefreshViewport()
{
	LevelViewportClient->Invalidate();
}

bool SAnimationEditorViewportTabBody::IsVisible() const
{
	return ViewportWidget.IsValid();
}


void SAnimationEditorViewportTabBody::Construct(const FArguments& InArgs)
{
	UICommandList = MakeShareable(new FUICommandList);

	PersonaPtr = InArgs._Persona;
	IsEditable = InArgs._IsEditable;
	TargetSkeleton = InArgs._Skeleton;
	bPreviewLockModeOn = 0;
	LODSelection = LOD_Auto;

	// register delegate for anim change notification
	PersonaPtr.Pin()->RegisterOnAnimChanged(FPersona::FOnAnimChanged::CreateSP(this, &SAnimationEditorViewportTabBody::AnimChanged));

	const FSlateFontInfo SmallLayoutFont( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10 );

	FAnimViewportMenuCommands::Register();
	FAnimViewportShowCommands::Register();
	FAnimViewportLODCommands::Register();
	FAnimViewportPlaybackCommands::Register();

	// Build toolbar widgets
	UVChannelCombo = SNew(STextComboBox)
		.OptionsSource(&UVChannels)
		.OnSelectionChanged(this, &SAnimationEditorViewportTabBody::ComboBoxSelectionChanged);

	ViewportWidget = SNew(SAnimationEditorViewport, InArgs._Persona, SharedThis(this));

	this->ChildSlot
	[
		SNew(SVerticalBox)

		// Build our toolbar level toolbar
		+SVerticalBox::Slot()
		.FillHeight(1)
		[
			SNew(SOverlay)

			// The viewport
			+SOverlay::Slot()
			[
				ViewportWidget.ToSharedRef()
			]

			// The 'dirty/in-error' indicator text in the bottom-right corner
			+SOverlay::Slot()
			.Padding(8)
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Right)
			[
				SNew(SButton)
				.ButtonStyle(FEditorStyle::Get(), "NoBorder")
				.Visibility(this, &SAnimationEditorViewportTabBody::GetViewportCornerTextVisibility)
				.TextStyle(FEditorStyle::Get(), "Persona.Viewport.BlueprintDirtyText")
				.Text(this, &SAnimationEditorViewportTabBody::GetViewportCornerText)
				.ToolTipText(LOCTEXT("BlueprintStatusTooltip", "Shows the status of the animation blueprint.\nClick to recompile a dirty blueprint"))
				.OnClicked(this, &SAnimationEditorViewportTabBody::ClickedOnViewportCornerText)
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(ScrubPanelContainer, SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SAnimationScrubPanel)
				.Persona(PersonaPtr)
				.ViewInputMin(this, &SAnimationEditorViewportTabBody::GetViewMinInput)
				.ViewInputMax(this, &SAnimationEditorViewportTabBody::GetViewMaxInput)
				.bAllowZoom(true)
			]
		]
	];

	UpdateScrubPanel(Cast<UAnimationAsset>(PersonaPtr.Pin()->GetPreviewAnimationAsset()));

	LevelViewportClient = ViewportWidget->GetViewportClient();
	LevelViewportClient->VisibilityDelegate.BindSP(this, &SAnimationEditorViewportTabBody::IsVisible);

	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());

	// Load the view mode from config
	AnimViewportClient->SetViewMode((EViewModeIndex)AnimViewportClient->ConfigOption->ViewModeIndex);
	UpdateShowFlagForMeshEdges();

	UpdateViewportClientPlaybackScale();

	BindCommands();
}

void SAnimationEditorViewportTabBody::BindCommands()
{
	FUICommandList& CommandList = *UICommandList;

	//Bind menu commands
	const FAnimViewportMenuCommands& MenuActions = FAnimViewportMenuCommands::Get();

	CommandList.MapAction( 
		MenuActions.Lock,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::SetPreviewMode, 1),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsPreviewModeOn, 1));

	CommandList.MapAction( 
		MenuActions.Auto,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::SetPreviewMode, 0),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsPreviewModeOn, 0));

	CommandList.MapAction(
		MenuActions.CameraFollow,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::ToggleCameraFollow),
		FCanExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::CanChangeCameraMode),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsCameraFollowEnabled));

	CommandList.MapAction(
		MenuActions.UseInGameBound,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::UseInGameBound),
		FCanExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::CanUseInGameBound),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsUsingInGameBound));

	TSharedRef<FAnimationViewportClient> EditorViewportClientRef = GetAnimationViewportClient();

	CommandList.MapAction(
		MenuActions.SetShowNormals,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FAnimationViewportClient::ToggleShowNormals ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FAnimationViewportClient::IsSetShowNormalsChecked ) );

	CommandList.MapAction(
		MenuActions.SetShowTangents,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FAnimationViewportClient::ToggleShowTangents ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FAnimationViewportClient::IsSetShowTangentsChecked ) );

	CommandList.MapAction(
		MenuActions.SetShowBinormals,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FAnimationViewportClient::ToggleShowBinormals ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FAnimationViewportClient::IsSetShowBinormalsChecked ) );

	CommandList.MapAction(
		MenuActions.SetDrawUVs,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FAnimationViewportClient::ToggleDrawUVOverlay ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FAnimationViewportClient::IsSetDrawUVOverlayChecked ) );

	//Bind Show commands
	const FAnimViewportShowCommands& ViewportShowMenuCommands = FAnimViewportShowCommands::Get();
	
	CommandList.MapAction(
		ViewportShowMenuCommands.ShowReferencePose,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::ShowReferencePose),
		FCanExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::CanShowReferencePose),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowReferencePoseEnabled));

	CommandList.MapAction(
		ViewportShowMenuCommands.ShowBound,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::ShowBound),
		FCanExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::CanShowBound),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowBoundEnabled));

	CommandList.MapAction(
		ViewportShowMenuCommands.ShowPreviewMesh,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::ToggleShowPreviewMesh),
		FCanExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::CanShowPreviewMesh),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowPreviewMeshEnabled));

	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowBones,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowBones),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingBones));

	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowBoneNames,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowBoneNames),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingBoneNames));

	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowRawAnimation,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowRawAnimation),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingRawAnimation));

	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowNonRetargetedAnimation,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowNonRetargetedAnimation),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingNonRetargetedPose));

	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowAdditiveBaseBones,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowAdditiveBase),
		FCanExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::IsPreviewingAnimation),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingAdditiveBase));

	//Display info
	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowDisplayInfo,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowDisplayInfo),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingMeshInfo));

	//Bone weight
	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowBoneWeight,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowBoneWeight),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingBoneWeight));

	// Show sockets
	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowSockets,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowSockets),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingSockets));

	// Set bone local axes mode
	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowLocalAxesNone,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnSetLocalAxesMode, (int32)ELocalAxesMode::None),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsLocalAxesModeSet, (int32)ELocalAxesMode::None));
	
	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowLocalAxesSelected,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnSetLocalAxesMode, (int32)ELocalAxesMode::Selected),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsLocalAxesModeSet, (int32)ELocalAxesMode::Selected));
	
	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowLocalAxesAll,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnSetLocalAxesMode, (int32)ELocalAxesMode::All),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsLocalAxesModeSet, (int32)ELocalAxesMode::All));

#if WITH_APEX_CLOTHING
	//Clothing show options
	CommandList.MapAction( 
		ViewportShowMenuCommands.DisableClothSimulation,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnDisableClothSimulation),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsDisablingClothSimulation));

	//Apply wind
	CommandList.MapAction( 
		ViewportShowMenuCommands.ApplyClothWind,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnApplyClothWind),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsApplyingClothWind));
	
	//Cloth simulation normal
	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowClothSimulationNormals,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowClothSimulationNormals),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingClothSimulationNormals));

	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowClothGraphicalTangents,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowClothGraphicalTangents),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingClothGraphicalTangents));

	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowClothCollisionVolumes,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowClothCollisionVolumes),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingClothCollisionVolumes));

	CommandList.MapAction( 
		ViewportShowMenuCommands.EnableCollisionWithAttachedClothChildren,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnEnableCollisionWithAttachedClothChildren),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsEnablingCollisionWithAttachedClothChildren));

	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowOnlyClothSections,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowOnlyClothSections),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingOnlyClothSections));
#endif// #if WITH_APEX_CLOTHING		

	//Bind LOD preview menu commands
	const FAnimViewportLODCommands& ViewportLODMenuCommands = FAnimViewportLODCommands::Get();

	//LOD Auto
	CommandList.MapAction( 
		ViewportLODMenuCommands.LODAuto,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnSetLODModel, LOD_Auto),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsLODModelSelected, LOD_Auto));

	//LOD 0
	CommandList.MapAction( 
		ViewportLODMenuCommands.LOD0,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnSetLODModel, LOD_0),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsLODModelSelected, LOD_0));

	//LOD 1
	CommandList.MapAction( 
		ViewportLODMenuCommands.LOD1,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnSetLODModel, LOD_1),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsLODModelSelected, LOD_1));

	//LOD 2
	CommandList.MapAction( 
		ViewportLODMenuCommands.LOD2,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnSetLODModel, LOD_2),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsLODModelSelected, LOD_2));

	//LOD 3
	CommandList.MapAction( 
		ViewportLODMenuCommands.LOD3,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnSetLODModel, LOD_3),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsLODModelSelected, LOD_3));

	CommandList.MapAction(
		ViewportLODMenuCommands.ShowLevelOfDetailSettings,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowLevelOfDetailSettings));

	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowGrid,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowGrid),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingGrid));

	// Highlight origin should be disabled if the grid isn't showing
	CommandList.MapAction( 
		ViewportShowMenuCommands.HighlightOrigin,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnHighlightOrigin),
		FCanExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingGrid),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsHighlightingOrigin));

	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowFloor,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowFloor),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingFloor));

	CommandList.MapAction( 
		ViewportShowMenuCommands.ShowSky,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnShowSky),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsShowingSky));

	//Bind LOD preview menu commands
	const FAnimViewportPlaybackCommands& ViewportPlaybackCommands = FAnimViewportPlaybackCommands::Get();

	//Create a menu item for each playback speed in EAnimationPlaybackSpeeds
	for(int32 i = 0; i < int(EAnimationPlaybackSpeeds::NumPlaybackSpeeds); ++i)
	{
		CommandList.MapAction( 
			ViewportPlaybackCommands.PlaybackSpeedCommands[i],
			FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnSetPlaybackSpeed, i),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsPlaybackSpeedSelected, i));
	}

	CommandList.MapAction(
		ViewportShowMenuCommands.MuteAudio,
		FExecuteAction::CreateSP(this, &SAnimationEditorViewportTabBody::OnMuteAudio),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SAnimationEditorViewportTabBody::IsAudioMuted));
}

bool SAnimationEditorViewportTabBody::IsPreviewModeOn(int32 PreviewMode) const
{
	if (bPreviewLockModeOn && PreviewMode)
	{
		return true;
	}
	else if (!bPreviewLockModeOn && !PreviewMode)
	{
		return true;
	}

	return false;

}
void SAnimationEditorViewportTabBody::SetPreviewMode(int32 PreviewMode)
{
	if (PreviewMode)
	{
		bPreviewLockModeOn = true; 
	}
	else
	{
		bPreviewLockModeOn = false; 
	}
}

int32 SAnimationEditorViewportTabBody::GetLODModelCount() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if( PreviewComponent && PreviewComponent->SkeletalMesh )
	{
		return PreviewComponent->SkeletalMesh->GetImportedResource()->LODModels.Num();
	}
	return 0;
}

void SAnimationEditorViewportTabBody::OnShowBones()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if( PreviewComponent != NULL )
	{
		PreviewComponent->bDisplayBones = !PreviewComponent->bDisplayBones;
		PreviewComponent->MarkRenderStateDirty();
		RefreshViewport();
	}
}

void SAnimationEditorViewportTabBody::OnShowBoneNames()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if( PreviewComponent != NULL )
	{
		PreviewComponent->bShowBoneNames = !PreviewComponent->bShowBoneNames;
		PreviewComponent->MarkRenderStateDirty();
		RefreshViewport();
	}
}

void SAnimationEditorViewportTabBody::OnShowRawAnimation()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if( PreviewComponent != NULL )
	{
		PreviewComponent->bDisplayRawAnimation = !PreviewComponent->bDisplayRawAnimation;
		PreviewComponent->MarkRenderStateDirty();
	}
}

void SAnimationEditorViewportTabBody::OnShowNonRetargetedAnimation()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if( PreviewComponent != NULL )
	{
		PreviewComponent->bDisplayNonRetargetedPose = !PreviewComponent->bDisplayNonRetargetedPose;
		PreviewComponent->MarkRenderStateDirty();
	}
}

void SAnimationEditorViewportTabBody::OnShowAdditiveBase()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if( PreviewComponent != NULL )
	{
		PreviewComponent->bDisplayAdditiveBasePose = !PreviewComponent->bDisplayAdditiveBasePose;
		PreviewComponent->MarkRenderStateDirty();
	}
}

bool SAnimationEditorViewportTabBody::IsPreviewingAnimation() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return (PreviewComponent && PreviewComponent->PreviewInstance && (PreviewComponent->PreviewInstance == PreviewComponent->AnimScriptInstance));
}

bool SAnimationEditorViewportTabBody::IsShowingBones() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return PreviewComponent != NULL && PreviewComponent->bDisplayBones;
}

bool SAnimationEditorViewportTabBody::IsShowingBoneNames() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return PreviewComponent != NULL && PreviewComponent->bShowBoneNames;
}

bool SAnimationEditorViewportTabBody::IsShowingRawAnimation() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return PreviewComponent != NULL && PreviewComponent->bDisplayRawAnimation;
}

bool SAnimationEditorViewportTabBody::IsShowingNonRetargetedPose() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return PreviewComponent != NULL && PreviewComponent->bDisplayNonRetargetedPose;
}

bool SAnimationEditorViewportTabBody::IsShowingAdditiveBase() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return PreviewComponent != NULL && PreviewComponent->bDisplayAdditiveBasePose;
}

void SAnimationEditorViewportTabBody::OnShowDisplayInfo()
{
	GetAnimationViewportClient()->OnToggleShowMeshStats();
}

bool SAnimationEditorViewportTabBody::IsShowingMeshInfo() const
{
	return GetAnimationViewportClient()->IsShowingMeshStats();
}

void SAnimationEditorViewportTabBody::OnShowBoneWeight()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if( PreviewComponent )
	{
		PreviewComponent->SetShowBoneWeight( !PreviewComponent->bDrawBoneInfluences );
		UpdateShowFlagForMeshEdges();
		PreviewComponent->MarkRenderStateDirty();
		RefreshViewport();
	}
}

bool SAnimationEditorViewportTabBody::IsShowingBoneWeight() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return PreviewComponent != NULL && PreviewComponent->bDrawBoneInfluences;
}

void SAnimationEditorViewportTabBody::OnSetLocalAxesMode(int32 LocalAxesMode)
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	AnimViewportClient->SetLocalAxesMode((ELocalAxesMode::Type)LocalAxesMode);
}

bool SAnimationEditorViewportTabBody::IsLocalAxesModeSet(int32 LocalAxesMode) const
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	return AnimViewportClient->IsLocalAxesModeSet((ELocalAxesMode::Type)LocalAxesMode);
}

void SAnimationEditorViewportTabBody::OnShowSockets()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if( PreviewComponent )
	{
		PreviewComponent->bDrawSockets = !PreviewComponent->bDrawSockets;
		PreviewComponent->MarkRenderStateDirty();
		RefreshViewport();
	}
}

bool SAnimationEditorViewportTabBody::IsShowingSockets() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return PreviewComponent != NULL && PreviewComponent->bDrawSockets;
}


void SAnimationEditorViewportTabBody::OnShowGrid()
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	AnimViewportClient->OnToggleShowGrid();
}

bool SAnimationEditorViewportTabBody::IsShowingGrid() const
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	return AnimViewportClient->IsShowingGrid();
}

void SAnimationEditorViewportTabBody::OnHighlightOrigin()
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	AnimViewportClient->OnToggleHighlightOrigin();
}

bool SAnimationEditorViewportTabBody::IsHighlightingOrigin() const
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	return AnimViewportClient->IsHighlightingOrigin();
}

void SAnimationEditorViewportTabBody::OnShowFloor()
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	AnimViewportClient->OnToggleShowFloor();
}

bool SAnimationEditorViewportTabBody::IsShowingFloor() const
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	return AnimViewportClient->IsShowingFloor();
}

void SAnimationEditorViewportTabBody::OnShowSky()
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	AnimViewportClient->OnToggleShowSky();
}

bool SAnimationEditorViewportTabBody::IsShowingSky() const
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	return AnimViewportClient->IsShowingSky();
}


/** Function to set the current playback speed*/
void SAnimationEditorViewportTabBody::OnSetPlaybackSpeed(int32 PlaybackSpeedMode)
{
	AnimationPlaybackSpeedMode = (EAnimationPlaybackSpeeds::Type)PlaybackSpeedMode;
	UpdateViewportClientPlaybackScale();
}

bool SAnimationEditorViewportTabBody::IsPlaybackSpeedSelected(int32 PlaybackSpeedMode)
{
	return PlaybackSpeedMode == AnimationPlaybackSpeedMode;
}

void SAnimationEditorViewportTabBody::ShowReferencePose()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if(PreviewComponent)
	{
		PreviewComponent->bForceRefpose = !PreviewComponent->bForceRefpose;
	}
}

bool SAnimationEditorViewportTabBody::CanShowReferencePose() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return PreviewComponent != NULL;
}

bool SAnimationEditorViewportTabBody::IsShowReferencePoseEnabled() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if(PreviewComponent)
	{
		return PreviewComponent->bForceRefpose;
	}
	return false;
}

void SAnimationEditorViewportTabBody::ShowBound()
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());	
	AnimViewportClient->ToggleShowBounds();

	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if(PreviewComponent)
	{
		PreviewComponent->bDisplayBound = AnimViewportClient->EngineShowFlags.Bounds;
		PreviewComponent->RecreateRenderState_Concurrent();
	}
}

bool SAnimationEditorViewportTabBody::CanShowBound() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return PreviewComponent != NULL;
}

bool SAnimationEditorViewportTabBody::IsShowBoundEnabled() const
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());	
	return AnimViewportClient->IsSetShowBoundsChecked();
}

void SAnimationEditorViewportTabBody::ToggleShowPreviewMesh()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if(PreviewComponent != NULL)
	{
		bool bCurrentlyVisible = IsShowPreviewMeshEnabled();
		PreviewComponent->SetVisibility(!bCurrentlyVisible);
	}
}

bool SAnimationEditorViewportTabBody::CanShowPreviewMesh() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return PreviewComponent != NULL;
}

bool SAnimationEditorViewportTabBody::IsShowPreviewMeshEnabled() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return (PreviewComponent != NULL) && PreviewComponent->IsVisible();
}

void SAnimationEditorViewportTabBody::UseInGameBound()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if (PreviewComponent != NULL)
	{
		PreviewComponent->UseInGameBounds(! PreviewComponent->IsUsingInGameBounds());
	}
}

bool SAnimationEditorViewportTabBody::CanUseInGameBound() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return PreviewComponent != NULL;
}

bool SAnimationEditorViewportTabBody::IsUsingInGameBound() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	return PreviewComponent != NULL && PreviewComponent->IsUsingInGameBounds();
}

void SAnimationEditorViewportTabBody::SetPreviewComponent(class UDebugSkelMeshComponent* PreviewComponent)
{
	PreviewComponent->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	PreviewComponent->bCanHighlightSelectedSections = true;

	//Set preview skeleton mesh component
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	AnimViewportClient->SetPreviewMeshComponent(PreviewComponent);

	AnimViewportClient->FocusViewportOnPreviewMesh();

	// Adding the component to the PreviewScene can change AnimScriptInstance so we must re register it
	// with the AnimBlueprint
	UAnimBlueprint* SourceBlueprint = Cast<UAnimBlueprint>(PersonaPtr.Pin()->GetBlueprintObj());
	if ( SourceBlueprint && PreviewComponent->IsAnimBlueprintInstanced() )
	{
		SourceBlueprint->SetObjectBeingDebugged(PreviewComponent->AnimScriptInstance);
	}

	PopulateNumUVChannels();
}

void SAnimationEditorViewportTabBody::AnimChanged(UAnimationAsset* AnimAsset)
{
	UpdateScrubPanel(AnimAsset);
}

void SAnimationEditorViewportTabBody::ComboBoxSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	int32 NewUVSelection = UVChannels.Find(NewSelection);

	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	AnimViewportClient->SetUVChannelToDraw(NewUVSelection);

	RefreshViewport();
}

void SAnimationEditorViewportTabBody::PopulateNumUVChannels()
{
	NumUVChannels.Empty();

	if(UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent)
	{
		if(FSkeletalMeshResource* MeshResource = PreviewComponent->GetSkeletalMeshResource())
		{
			int32 NumLods = MeshResource->LODModels.Num();
			NumUVChannels.AddZeroed(NumLods);
			for(int32 LOD = 0; LOD < NumLods; ++LOD)
			{
				NumUVChannels[LOD] = MeshResource->LODModels[LOD].VertexBufferGPUSkin.GetNumTexCoords();
			}
		}
	}

	PopulateUVChoices();
}

void SAnimationEditorViewportTabBody::PopulateUVChoices()
{
	// Fill out the UV channels combo.
	UVChannels.Empty();
	
	if( UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent )
	{
		int32 CurrentLOD = FMath::Clamp(PreviewComponent->ForcedLodModel - 1, 0, NumUVChannels.Num() - 1);

		if(NumUVChannels.IsValidIndex(CurrentLOD))
		{
			for(int32 UVChannelID = 0; UVChannelID < NumUVChannels[CurrentLOD]; ++UVChannelID)
			{
				UVChannels.Add( MakeShareable( new FString( FString::Printf(*NSLOCTEXT("AnimationEditorViewport", "UVChannel_ID", "UV Channel %d").ToString(), UVChannelID ) ) ) );
			}

			TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
			int32 CurrentUVChannel = AnimViewportClient->GetUVChannelToDraw();
			if(!UVChannels.IsValidIndex(CurrentUVChannel))
			{
				CurrentUVChannel = 0;
			}

			AnimViewportClient->SetUVChannelToDraw(CurrentUVChannel);

			if(UVChannelCombo.IsValid() && UVChannels.IsValidIndex(CurrentUVChannel))
			{
				UVChannelCombo->SetSelectedItem(UVChannels[CurrentUVChannel]);
			}
		}
	}
}

void SAnimationEditorViewportTabBody::UpdateScrubPanel(UAnimationAsset* AnimAsset)
{
	ScrubPanelContainer->ClearChildren();
	bool bUseDefaultScrubPanel = true;
	if (UAnimMontage* Montage = Cast<UAnimMontage>(AnimAsset))
	{
		ScrubPanelContainer->AddSlot()
		.AutoHeight()
		[
			SNew(SAnimMontageScrubPanel)
			.Persona(PersonaPtr)
			.ViewInputMin(this, &SAnimationEditorViewportTabBody::GetViewMinInput)
			.ViewInputMax(this, &SAnimationEditorViewportTabBody::GetViewMaxInput)
			.bAllowZoom(true)
		];
		bUseDefaultScrubPanel = false;
	}
	if (bUseDefaultScrubPanel)
	{
		ScrubPanelContainer->AddSlot()
		.AutoHeight()
		[
			SNew(SAnimationScrubPanel)
			.Persona(PersonaPtr)
			.ViewInputMin(this, &SAnimationEditorViewportTabBody::GetViewMinInput)
			.ViewInputMax(this, &SAnimationEditorViewportTabBody::GetViewMaxInput)
			.bAllowZoom(true)
		];
	}
}

float SAnimationEditorViewportTabBody::GetViewMinInput() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if (PreviewComponent != NULL)
	{
		UObject* PreviewAsset = PersonaPtr.Pin()->GetPreviewAnimationAsset();
		if (PreviewAsset != NULL)
		{
			return 0.0f;
		}
		else if (PreviewComponent->AnimScriptInstance != NULL)
		{
			return FMath::Max<float>((float)(PreviewComponent->AnimScriptInstance->LifeTimer - 30.0), 0.0f);
		}
	}

	return 0.f; 
}

float SAnimationEditorViewportTabBody::GetViewMaxInput() const
{ 
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if (PreviewComponent != NULL)
	{
		UObject* PreviewAsset = PersonaPtr.Pin()->GetPreviewAnimationAsset();
		if ((PreviewAsset != NULL) && (PreviewComponent->PreviewInstance != NULL))
		{
			return PreviewComponent->PreviewInstance->GetLength();
		}
		else if (PreviewComponent->AnimScriptInstance != NULL)
		{
			return PreviewComponent->AnimScriptInstance->LifeTimer;
		}
	}

	return 0.f;
}

void SAnimationEditorViewportTabBody::UpdateShowFlagForMeshEdges()
{
	bool bDrawBonesInfluence = false;
	if (UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent)
	{
		bDrawBonesInfluence = PreviewComponent->bDrawBoneInfluences;
	}

	//@TODO: SNOWPOCALYPSE: broke UnlitWithMeshEdges
	bool bShowMeshEdgesViewMode = false;
#if 0
	bShowMeshEdgesViewMode = (CurrentViewMode == EAnimationEditorViewportMode::UnlitWithMeshEdges);
#endif

	LevelViewportClient->EngineShowFlags.MeshEdges = (bDrawBonesInfluence || bShowMeshEdgesViewMode) ? 1 : 0;
}

bool SAnimationEditorViewportTabBody::IsLODModelSelected(ELODViewSelection LODSelectionType) const
{
	return (LODSelection == LODSelectionType) ? true : false;
}

void SAnimationEditorViewportTabBody::OnSetLODModel(ELODViewSelection LODSelectionType)
{
	LODSelection = LODSelectionType;

	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if( PreviewComponent )
	{
		PreviewComponent->ForcedLodModel = LODSelection;
		PopulateUVChoices();
	}
}

void SAnimationEditorViewportTabBody::OnShowLevelOfDetailSettings()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;
	if(TargetSkeleton)
	{
		FSkeletalMeshUpdateContext UpdateContext;
		UpdateContext.SkeletalMesh = PersonaPtr.Pin()->GetMesh();
		UpdateContext.AssociatedComponents.Push(PreviewComponent);
		UpdateContext.OnLODChanged.BindSP(this, &SAnimationEditorViewportTabBody::OnLODChanged);

		FLODUtilities::CreateLODSettingsDialog(UpdateContext);
	}
}

FLinearColor SAnimationEditorViewportTabBody::GetViewportBackgroundColor() const
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	return AnimViewportClient->GetBackgroundColor();
}

void SAnimationEditorViewportTabBody::SetViewportBackgroundColor(FLinearColor InColor)
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	AnimViewportClient->SetBackgroundColor( InColor );
}

float SAnimationEditorViewportTabBody::GetBackgroundBrightness() const
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	return AnimViewportClient->GetBrightnessValue();
}

void SAnimationEditorViewportTabBody::SetBackgroundBrightness(float Value)
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	AnimViewportClient->SetBrightnessValue(Value);
	RefreshViewport();
}

TSharedRef<FAnimationViewportClient> SAnimationEditorViewportTabBody::GetAnimationViewportClient() const
{
	return StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
}

void SAnimationEditorViewportTabBody::ToggleCameraFollow()
{
	// Switch to rotation mode
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	AnimViewportClient->SetCameraFollow();
}

bool SAnimationEditorViewportTabBody::IsCameraFollowEnabled() const
{
	// need a single selected bone in the skeletal tree
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	return (AnimViewportClient->IsSetCameraFollowChecked());
}


bool SAnimationEditorViewportTabBody::CanChangeCameraMode() const
{
	//Not allowed to change camera type when we are in an ortho camera
	return !LevelViewportClient->IsOrtho();
}

void SAnimationEditorViewportTabBody::SaveData(class SAnimationEditorViewportTabBody* OldViewport)
{
	if ( PersonaPtr.IsValid() && OldViewport )
	{
		FPersonaModeSharedData & SharedData = PersonaPtr.Pin()->ModeSharedData;

		TSharedRef<FAnimationViewportClient> OldAnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(OldViewport->LevelViewportClient.ToSharedRef());
		// set camera set up
		SharedData.ViewLocation = OldAnimViewportClient.Get().GetViewLocation();
		SharedData.ViewRotation = OldAnimViewportClient.Get().GetViewRotation();
		SharedData.LookAtLocation = OldAnimViewportClient.Get().GetLookAtLocation();
		SharedData.OrthoZoom = OldAnimViewportClient.Get().ViewTransform.GetOrthoZoom();
		SharedData.bCameraLock = OldAnimViewportClient.Get().IsCameraLocked();
		SharedData.bCameraFollow = OldAnimViewportClient.Get().IsSetCameraFollowChecked();
		SharedData.bShowBound = OldAnimViewportClient.Get().IsSetShowBoundsChecked();
		SharedData.LocalAxesMode = OldAnimViewportClient.Get().GetLocalAxesMode();
		SharedData.ViewportType = OldAnimViewportClient.Get().ViewportType;
		SharedData.PlaybackSpeedMode = OldViewport->AnimationPlaybackSpeedMode;
	}
}

void SAnimationEditorViewportTabBody::RestoreData()
{
	if ( PersonaPtr.IsValid() )
	{
		FPersonaModeSharedData & SharedData = PersonaPtr.Pin()->ModeSharedData;
		TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());


		AnimViewportClient.Get().SetViewportType((ELevelViewportType)SharedData.ViewportType);
		AnimViewportClient.Get().SetViewLocation( SharedData.ViewLocation );
		AnimViewportClient.Get().SetViewRotation( SharedData.ViewRotation );
		AnimViewportClient.Get().SetShowBounds(SharedData.bShowBound);
		AnimViewportClient.Get().SetLocalAxesMode((ELocalAxesMode::Type)SharedData.LocalAxesMode);
		AnimViewportClient.Get().ViewTransform.SetOrthoZoom(SharedData.OrthoZoom);

		OnSetPlaybackSpeed(SharedData.PlaybackSpeedMode);

		if (SharedData.bCameraLock)
		{
			AnimViewportClient.Get().SetLookAtLocation( SharedData.LookAtLocation );
		}
		else if(SharedData.bCameraFollow)
		{
			AnimViewportClient.Get().SetCameraFollow();
		}
	}
}

void SAnimationEditorViewportTabBody::UpdateViewportClientPlaybackScale()
{
	TSharedRef<FAnimationViewportClient> AnimViewportClient = StaticCastSharedRef<FAnimationViewportClient>(LevelViewportClient.ToSharedRef());
	AnimViewportClient->SetPlaybackScale(EAnimationPlaybackSpeeds::Values[AnimationPlaybackSpeedMode]);
}

void SAnimationEditorViewportTabBody::OnLODChanged()
{
	int32 LodCount = 0;

	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent && PreviewComponent->SkeletalMesh )
	{
		LodCount = PreviewComponent->SkeletalMesh->GetImportedResource()->LODModels.Num();
	}

	// If the currently selected LoD is invalid, revert to LOD_Auto
	if ( LodCount < LODSelection )
	{
		OnSetLODModel( LOD_Auto );
	}
}

void SAnimationEditorViewportTabBody::OnMuteAudio()
{
	GetAnimationViewportClient()->OnToggleMuteAudio();
}

bool SAnimationEditorViewportTabBody::IsAudioMuted()
{
	return GetAnimationViewportClient()->IsAudioMuted();
}

#if WITH_APEX_CLOTHING
bool SAnimationEditorViewportTabBody::IsDisablingClothSimulation() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		return PreviewComponent->bDisableClothSimulation;
	}

	return false;
}

void SAnimationEditorViewportTabBody::OnDisableClothSimulation()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		PreviewComponent->bDisableClothSimulation = !PreviewComponent->bDisableClothSimulation;
		RefreshViewport();
	}
}

bool SAnimationEditorViewportTabBody::IsApplyingClothWind() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		return PreviewComponent->IsWindEnabled();
	}

	return false;
}

void SAnimationEditorViewportTabBody::OnApplyClothWind()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		PreviewComponent->bEnableWind = !PreviewComponent->IsWindEnabled();
		GetAnimationViewportClient()->EnableWindActor(PreviewComponent->IsWindEnabled());
		RefreshViewport();
	}
}

void SAnimationEditorViewportTabBody::SetWindStrength(float SliderPos)
{
	GetAnimationViewportClient()->SetWindStrength( SliderPos );
	RefreshViewport();
}

float SAnimationEditorViewportTabBody::GetWindStrengthSliderValue() const
{
	return GetAnimationViewportClient()->GetWindStrengthSliderValue();
}

FString SAnimationEditorViewportTabBody::GetWindStrengthLabel() const
{
	return GetAnimationViewportClient()->GetWindStrengthLabel();
}

void SAnimationEditorViewportTabBody::OnShowClothSimulationNormals()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		PreviewComponent->bDisplayClothingNormals = !PreviewComponent->bDisplayClothingNormals;
		RefreshViewport();
	}
}

bool SAnimationEditorViewportTabBody::IsShowingClothSimulationNormals() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		return PreviewComponent->bDisplayClothingNormals;
	}

	return false;
}

void SAnimationEditorViewportTabBody::OnShowClothGraphicalTangents()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		PreviewComponent->bDisplayClothingTangents = !PreviewComponent->bDisplayClothingTangents;
		RefreshViewport();
	}
}

bool SAnimationEditorViewportTabBody::IsShowingClothGraphicalTangents() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		return PreviewComponent->bDisplayClothingTangents;
	}

	return false;
}

void SAnimationEditorViewportTabBody::OnShowClothCollisionVolumes()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		PreviewComponent->bDisplayClothingCollisionVolumes = !PreviewComponent->bDisplayClothingCollisionVolumes;
		RefreshViewport();
	}
}

bool SAnimationEditorViewportTabBody::IsShowingClothCollisionVolumes() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		return PreviewComponent->bDisplayClothingCollisionVolumes;
	}

	return false;
}

void SAnimationEditorViewportTabBody::SetGravityScale(float SliderPos)
{
	GetAnimationViewportClient()->SetGravityScale(SliderPos);


	RefreshViewport();
}

float SAnimationEditorViewportTabBody::GetGravityScaleSliderValue() const
{
	return GetAnimationViewportClient()->GetGravityScaleSliderValue();
}

FString SAnimationEditorViewportTabBody::GetGravityScaleLabel() const
{
	return GetAnimationViewportClient()->GetGravityScaleLabel();
}

void SAnimationEditorViewportTabBody::OnEnableCollisionWithAttachedClothChildren()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		PreviewComponent->bCollideWithAttachedChildren = !PreviewComponent->bCollideWithAttachedChildren;
		RefreshViewport();
	}
}

bool SAnimationEditorViewportTabBody::IsEnablingCollisionWithAttachedClothChildren() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		return PreviewComponent->bCollideWithAttachedChildren;
	}

	return false;
}

void SAnimationEditorViewportTabBody::OnShowOnlyClothSections()
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		PreviewComponent->bDisplayOnlyClothSections = !PreviewComponent->bDisplayOnlyClothSections;

		PreviewComponent->PreEditChange(NULL);

		if(PreviewComponent->bDisplayOnlyClothSections)
		{
			// disable all except clothing sections
			FSkeletalMeshResource* SkelMeshResource = PreviewComponent->GetSkeletalMeshResource();
			check(SkelMeshResource);
			const int32 LODIndex = FMath::Clamp(PreviewComponent->PredictedLODLevel, 0, SkelMeshResource->LODModels.Num()-1);
			FStaticLODModel& LODModel = SkelMeshResource->LODModels[ LODIndex ];

			for(int32 SecIdx=0; SecIdx < LODModel.Sections.Num(); SecIdx++)
			{
				FSkelMeshSection& Section = LODModel.Sections[SecIdx];

				if(!LODModel.Chunks[Section.ChunkIndex].HasApexClothData())
				{
					Section.bDisabled = true;
				}
			}
		}
		else
		{
			// restore to previous states
			FSkeletalMeshResource* SkelMeshResource = PreviewComponent->GetSkeletalMeshResource();
			check(SkelMeshResource);
			const int32 LODIndex = FMath::Clamp(PreviewComponent->PredictedLODLevel, 0, SkelMeshResource->LODModels.Num()-1);
			FStaticLODModel& LODModel = SkelMeshResource->LODModels[ LODIndex ];

			for(int32 SecIdx=0; SecIdx < LODModel.Sections.Num(); SecIdx++)
			{
				LODModel.Sections[SecIdx].bDisabled = false;
			}

			for(int32 SecIdx=0; SecIdx < LODModel.Sections.Num(); SecIdx++)
			{
				FSkelMeshSection& Section = LODModel.Sections[SecIdx];

				if(LODModel.Chunks[Section.ChunkIndex].HasApexClothData())
				{
					LODModel.Sections[Section.CorrespondClothSectionIndex].bDisabled = true;
				}
			}
		}

		PreviewComponent->PostEditChange();

		RefreshViewport();
	}
}

bool SAnimationEditorViewportTabBody::IsShowingOnlyClothSections() const
{
	UDebugSkelMeshComponent* PreviewComponent = PersonaPtr.Pin()->PreviewComponent;

	if( PreviewComponent )
	{
		return PreviewComponent->bDisplayOnlyClothSections;
	}

	return false;
}

EVisibility SAnimationEditorViewportTabBody::GetViewportCornerTextVisibility() const
{
	TSharedPtr<FPersona> Persona = PersonaPtr.Pin();
	if (Persona->IsModeCurrent(FPersonaModes::AnimBlueprintEditMode))
	{
		if (UBlueprint* Blueprint = Persona->GetBlueprintObj())
		{
			const bool bUpToDate = (Blueprint->Status == BS_UpToDate) || (Blueprint->Status == BS_UpToDateWithWarnings);
			return bUpToDate ? EVisibility::Collapsed : EVisibility::Visible;
		}		
	}

	return EVisibility::Collapsed;
}

FText SAnimationEditorViewportTabBody::GetViewportCornerText() const
{
	TSharedPtr<FPersona> Persona = PersonaPtr.Pin();
	if (Persona->IsModeCurrent(FPersonaModes::AnimBlueprintEditMode))
	{
		if (UBlueprint* Blueprint = Persona->GetBlueprintObj())
		{
			switch (Blueprint->Status)
			{
			case BS_UpToDate:
			case BS_UpToDateWithWarnings:
				// Fall thru and return empty string
				break;
			case BS_Dirty:
				return LOCTEXT("AnimBP_Dirty", "Preview out of date\nClick to recompile");
			case BS_Error:
				return LOCTEXT("AnimBP_CompileError", "Compile Error");
			default:
				return LOCTEXT("AnimBP_UnknownStatus", "Unknown Status");
			}
		}		
	}

	return FText::GetEmpty();
}

FReply SAnimationEditorViewportTabBody::ClickedOnViewportCornerText()
{
	TSharedPtr<FPersona> Persona = PersonaPtr.Pin();
	if (UBlueprint* Blueprint = Persona->GetBlueprintObj())
	{
		if (!Blueprint->IsUpToDate())
		{
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
		}
	}

	return FReply::Handled();
}

#endif // #if WITH_APEX_CLOTHING
#undef LOCTEXT_NAMESPACE
