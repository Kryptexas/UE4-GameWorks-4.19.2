// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LevelSequenceEditorPCH.h"
#include "SCinematicLevelViewport.h"
#include "ISequencer.h"
#include "SequencerCommands.h"
#include "MovieScene.h"
#include "MovieSceneCinematicShotTrack.h"
#include "MovieSceneCinematicShotSection.h"
#include "MovieSceneSubTrack.h"
#include "MovieSceneSubSection.h"
#include "MovieSceneCommonHelpers.h"
#include "SCinematicTransportRange.h"
#include "FilmOverlays.h"
#include "LevelViewportLayout.h"
#include "LevelViewportTabContent.h"
#include "LevelSequenceEditorStyle.h"
#include "SWidgetSwitcher.h"
#include "CineCameraComponent.h"
#include "UnitConversion.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SCinematicLevelViewport"

template<typename T>
struct SNonThrottledSpinBox : SSpinBox<T>
{
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		FReply Reply = SSpinBox<T>::OnMouseButtonDown(MyGeometry, MouseEvent);
		if (Reply.IsEventHandled())
		{
			Reply.PreventThrottling();
		}
		return Reply;
	}
};

struct FTypeInterfaceProxy : INumericTypeInterface<float>
{
	TSharedPtr<INumericTypeInterface<float>> Impl;

	/** Convert the type to/from a string */
	virtual FString ToString(const float& Value) const override
	{
		if (Impl.IsValid())
		{
			return Impl->ToString(Value);
		}
		return FString();
	}

	virtual TOptional<float> FromString(const FString& InString, const float& InExistingValue) override
	{
		if (Impl.IsValid())
		{
			return Impl->FromString(InString, InExistingValue);
		}
		return TOptional<float>();
	}

	/** Check whether the typed character is valid */
	virtual bool IsCharacterValid(TCHAR InChar) const
	{
		if (Impl.IsValid())
		{
			return Impl->IsCharacterValid(InChar);
		}
		return false;
	}
};

FCinematicViewportClient::FCinematicViewportClient()
	: FLevelEditorViewportClient(nullptr)
{
	bDrawAxes = false;
	bIsRealtime = true;
	SetGameView(true);
	SetAllowCinematicPreview(true);
	bDisableInput = false;
}

bool FCinematicViewportClient::InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool bGamepad)
{
	// Allow immersive mode to be processed when input handling is disabled
	if (bDisableInput)		
	{
		if (Key == EKeys::F11)
		{
			return false;
		}
	}

	return FLevelEditorViewportClient::InputKey(Viewport, ControllerId, Key, Event, AmountDepressed, bGamepad);
}


class SPreArrangedBox : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnArrange, const FGeometry&);

	SLATE_BEGIN_ARGS(SPreArrangedBox){}
		SLATE_EVENT(FOnArrange, OnArrange)
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		OnArrange = InArgs._OnArrange;
		ChildSlot
		[
			InArgs._Content.Widget
		];
	}

	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override
	{
		OnArrange.ExecuteIfBound(AllottedGeometry);
		SCompoundWidget::OnArrangeChildren(AllottedGeometry, ArrangedChildren);
	}

private:

	FOnArrange OnArrange;
};

class SCinematicPreviewViewport : public SEditorViewport
{
public:

	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient()
	{
		return MakeShareable( new FCinematicViewportClient );
	}
};

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SCinematicLevelViewport::Construct(const FArguments& InArgs)
{
	ParentLayout = InArgs._ParentLayout;
	LayoutName = InArgs._LayoutName;
	RevertToLayoutName = InArgs._RevertToLayoutName;

	TypeInterfaceProxy = MakeShareable( new FTypeInterfaceProxy );

	FLevelSequenceEditorToolkit::OnOpened().AddSP(this, &SCinematicLevelViewport::OnEditorOpened);

	FLinearColor Gray(.3f, .3f, .3f, 1.f);

	TSharedRef<SFilmOverlayOptions> FilmOverlayOptions = SNew(SFilmOverlayOptions);

	TSharedRef<SWidget> MaximizeButton = SNullWidget::NullWidget;
	if (InArgs._bShowMaximize)
	{
		MaximizeButton =
			SNew(SButton)
			.Cursor(EMouseCursor::Default)
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
			.OnClicked(this, &SCinematicLevelViewport::OnToggleMaximize)
			.ToolTipText(LOCTEXT("Maximize_ToolTip", "Maximizes or restores this viewport"))
			[
				SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image(this, &SCinematicLevelViewport::GetMaximizeImage)
					.ColorAndOpacity(Gray)
				]
			];
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("BlackBrush"))
		.ForegroundColor(Gray)
		.Padding(5.f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SComboButton)
					.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
					.ForegroundColor(Gray)
					.OnGetMenuContent(this, &SCinematicLevelViewport::MakeCameraMenu)
					.ButtonContent()
					[
						SNew(SBox)
						.WidthOverride(36)
						.HeightOverride(24)
						.ToolTipText(LOCTEXT("CameraSelectionTooltip", "Displays a list of available cameras to view through."))
						[
							SNew(SImage)
							.Image(FLevelSequenceEditorStyle::Get()->GetBrush("LevelSequenceEditor.CinematicViewportCamera"))
							.ColorAndOpacity(Gray)
						]
					]
				]
				
				+ SHorizontalBox::Slot()
				[
					SNew(SSpacer)
				]
				
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					FilmOverlayOptions
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					MaximizeButton
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Cursor(EMouseCursor::Default)
					.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
					.OnClicked(this, &SCinematicLevelViewport::OnRevertLayout)
					.ToolTipText(LOCTEXT("Close_ToolTip", "Close this cinematic viewport, reverting it back to a standard viewport"))
					[
						SNew(SBox)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SNew(SImage)
							.Image(FLevelSequenceEditorStyle::Get()->GetBrush("LevelSequenceEditor.CinematicViewportClose"))
							.ColorAndOpacity(Gray)
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.Padding(20.f)
			[
				SNew(SPreArrangedBox)
				.OnArrange(this, &SCinematicLevelViewport::CacheDesiredViewportSize)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					[
						SNew(SSpacer)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(SBox)
						.HeightOverride(this, &SCinematicLevelViewport::GetDesiredViewportHeight)
						.WidthOverride(this, &SCinematicLevelViewport::GetDesiredViewportWidth)
						[
							SNew(SOverlay)

							+ SOverlay::Slot()
							[
								SAssignNew(ViewportWidget, SCinematicPreviewViewport)
							]

							+ SOverlay::Slot()
							[
								FilmOverlayOptions->GetFilmOverlayWidget()
							]
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SAssignNew(ViewportControls, SBox)
						.Visibility(this, &SCinematicLevelViewport::GetControlsVisibility)
						.WidthOverride(this, &SCinematicLevelViewport::GetDesiredViewportWidth)
						.Padding(FMargin(0.f, 10.f))
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.HAlign(HAlign_Left)
							[
								SNew(STextBlock)
								.ColorAndOpacity(Gray)
								.Text_Lambda([=]{ return UIData.ShotName; })
							]

							+ SHorizontalBox::Slot()
							.HAlign(HAlign_Center)
							.AutoWidth()
							[
								SNew(STextBlock)
								.ColorAndOpacity(Gray)
								.Text_Lambda([=]{ return UIData.Filmback; })
							]

							+ SHorizontalBox::Slot()
							.HAlign(HAlign_Right)
							[
								SNew(STextBlock)
								.Font(FEditorStyle::GetFontStyle("Sequencer.FixedFont"))
								.ColorAndOpacity(Gray)
								.Text_Lambda([=]{ return UIData.Frame; })
							]
						]
					]

					+ SVerticalBox::Slot()
					[
						SNew(SSpacer)
					]
				]
			]


			+ SVerticalBox::Slot()
			.AutoHeight()
			[
			
				SNew(SWidgetSwitcher)
				.WidgetIndex(this, &SCinematicLevelViewport::GetVisibleWidgetIndex)

				+ SWidgetSwitcher::Slot()
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5.f)
					[
						SAssignNew(TransportRange, SCinematicTransportRange)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5.f, 0.f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock)
							.Font(FEditorStyle::GetFontStyle("Sequencer.FixedFont"))
							.ColorAndOpacity(Gray)
							.Text_Lambda([=]{ return UIData.MasterStartText; })
						]

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						.Padding(FMargin(10.f, 0.f))
						[
							SNew(SNonThrottledSpinBox<float>)
							.TypeInterface(TypeInterfaceProxy)
							.Style(FEditorStyle::Get(), "Sequencer.HyperlinkSpinBox")
							.Font(FEditorStyle::GetFontStyle("Sequencer.FixedFont"))
							.OnValueCommitted(this, &SCinematicLevelViewport::OnTimeCommitted)
							.OnValueChanged(this, &SCinematicLevelViewport::SetTime)
							.OnEndSliderMovement(this, &SCinematicLevelViewport::SetTime)
							.MinValue(this, &SCinematicLevelViewport::GetMinTime)
							.MaxValue(this, &SCinematicLevelViewport::GetMaxTime)
							.Value(this, &SCinematicLevelViewport::GetTime)
						]

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SButton)
							.OnClicked(this, &SCinematicLevelViewport::SetPlaybackStart)
							.ToolTipText(LOCTEXT("SetPlayStart_Tooltip", "Set playback start to the current position"))
							.ButtonStyle(FEditorStyle::Get(), "NoBorder")
							.ContentPadding(2.0f)
							[
								SNew(SImage)
								.Image(FLevelSequenceEditorStyle::Get()->GetBrush("LevelSequenceEditor.CinematicViewportSetPlayStart"))
							]
						]

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.AutoWidth()
						[
							SAssignNew(TransportControls, SBox)
						]

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						[
							SNew(SButton)
							.OnClicked(this, &SCinematicLevelViewport::SetPlaybackEnd)
							.ToolTipText(LOCTEXT("SetPlayEnd_Tooltip", "Set playback end to the current position"))
							.ButtonStyle(FEditorStyle::Get(), "NoBorder")
							.ContentPadding(2.0f)
							[
								SNew(SImage)
								.Image(FLevelSequenceEditorStyle::Get()->GetBrush("LevelSequenceEditor.CinematicViewportSetPlayEnd"))
							]
						]

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock)
							.Font(FEditorStyle::GetFontStyle("Sequencer.FixedFont"))
							.ColorAndOpacity(Gray)
							.Text_Lambda([=]{ return UIData.MasterEndText; })
						]
					]
				]

				+ SWidgetSwitcher::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.Padding(5.f, 0.f)
				[
					SNew(STextBlock)
					.ColorAndOpacity(Gray)
					.Text(LOCTEXT("NoSequencerMessage", "No active Level Sequencer detected. Please edit a Level Sequence to enable full controls."))
				]
			]
		]
	];

	FLevelSequenceEditorToolkit::IterateOpenToolkits([&](FLevelSequenceEditorToolkit& Toolkit){
		Setup(Toolkit);
		return false;
	});

	CommandList = MakeShareable( new FUICommandList );
	// Ensure the commands are registered
	FLevelSequenceEditorCommands::Register();
	BindCommands();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply SCinematicLevelViewport::OnRevertLayout()
{
	TSharedPtr<FLevelViewportLayout> LayoutPinned = ParentLayout.Pin();
	if (LayoutPinned.IsValid())
	{
		TSharedPtr<FLevelViewportTabContent> ViewportTabPinned = LayoutPinned->GetParentTabContent().Pin();
		if (ViewportTabPinned.IsValid())
		{
			// Viewport clients are going away.  Any current one is invalid.
			GCurrentLevelEditingViewportClient = nullptr;
			ViewportTabPinned->SetViewportConfiguration(RevertToLayoutName);
			FSlateApplication::Get().DismissAllMenus();
		}
	}
	return FReply::Handled();
}

int32 SCinematicLevelViewport::GetVisibleWidgetIndex() const
{
	return CurrentToolkit.IsValid() ? 0 : 1;
}

EVisibility SCinematicLevelViewport::GetControlsVisibility() const
{
	return CurrentToolkit.IsValid() ? EVisibility::Visible : EVisibility::Hidden;
}

FReply SCinematicLevelViewport::OnToggleMaximize()
{
	TSharedPtr<FLevelViewportLayout> ParentLayoutPinned = ParentLayout.Pin();
	if (ParentLayoutPinned.IsValid() && ParentLayoutPinned->IsMaximizeSupported())
	{
		const bool bAllowAnimation = true;
		const bool bMaximize = !ParentLayoutPinned->IsViewportMaximized(LayoutName);
		
		ParentLayoutPinned->RequestMaximizeViewport( LayoutName, bMaximize, false, bAllowAnimation );
	}
	return FReply::Handled();
}

bool SCinematicLevelViewport::IsMaximized() const
{
	TSharedPtr<FLevelViewportLayout> ParentLayoutPinned = ParentLayout.Pin();
	if (ParentLayoutPinned.IsValid())
	{
		return ParentLayoutPinned->IsViewportMaximized(LayoutName);
	}
	return false;
}

void SCinematicLevelViewport::OnToggleImmersive()
{
	TSharedPtr<FLevelViewportLayout> ParentLayoutPinned = ParentLayout.Pin();
	if (ParentLayoutPinned.IsValid() && ParentLayoutPinned->IsMaximizeSupported())
	{
		const bool bAllowAnimation = true;
		const bool bMaximize = !ParentLayoutPinned->IsViewportMaximized(LayoutName);
		
		const bool bImmersive = !IsImmersive();
		ParentLayoutPinned->RequestMaximizeViewport( LayoutName, bMaximize, bImmersive, bAllowAnimation );
	}
}

bool SCinematicLevelViewport::IsImmersive() const
{
	TSharedPtr<FLevelViewportLayout> ParentLayoutPinned = ParentLayout.Pin();
	if (ParentLayoutPinned.IsValid())
	{
		return ParentLayoutPinned->IsViewportImmersive(LayoutName);
	}
	return false;
}

const FSlateBrush* SCinematicLevelViewport::GetMaximizeImage() const
{
	return IsMaximized() ? FEditorStyle::GetBrush("LevelViewportToolBar.Maximize.Checked") : FEditorStyle::GetBrush("LevelViewportToolBar.Maximize.Normal");
}

TOptional<float> SCinematicLevelViewport::GetMinTime() const
{
	ISequencer* Sequencer = GetSequencer();
	if (Sequencer)
	{
		return Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetEditorData().WorkingRange.GetLowerBoundValue();
	}
	return TOptional<float>();
}

TOptional<float> SCinematicLevelViewport::GetMaxTime() const
{
	ISequencer* Sequencer = GetSequencer();
	if (Sequencer)
	{
		return Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetEditorData().WorkingRange.GetUpperBoundValue();
	}
	return TOptional<float>();
}

void SCinematicLevelViewport::OnTimeCommitted(float Value, ETextCommit::Type)
{
	SetTime(Value);
}

void SCinematicLevelViewport::SetTime(float Value)
{
	ISequencer* Sequencer = GetSequencer();
	if (Sequencer)
	{
		Sequencer->SetGlobalTime(Value);
	}
}

float SCinematicLevelViewport::GetTime() const
{
	ISequencer* Sequencer = GetSequencer();
	if (Sequencer)
	{
		return Sequencer->GetGlobalTime();
	}
	return 0.f;
}

FReply SCinematicLevelViewport::SetPlaybackStart()
{
	ISequencer* Sequencer = GetSequencer();
	if (Sequencer)
	{
		FScopedTransaction Transaction(LOCTEXT("SetPlaybackStart", "Set Playback Start"));

		TRange<float> CurrentRange = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetPlaybackRange();

		float NewPos = FMath::Min(Sequencer->GetGlobalTime(), CurrentRange.GetUpperBoundValue());
		Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->SetPlaybackRange(NewPos, CurrentRange.GetUpperBoundValue());
	}
	return FReply::Handled();
}
FReply SCinematicLevelViewport::SetPlaybackEnd()
{
	ISequencer* Sequencer = GetSequencer();
	if (Sequencer)
	{
		FScopedTransaction Transaction(LOCTEXT("SetPlaybackEnd", "Set Playback End"));

		TRange<float> CurrentRange = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetPlaybackRange();

		float NewPos = FMath::Max(Sequencer->GetGlobalTime(), CurrentRange.GetLowerBoundValue());
		Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->SetPlaybackRange(CurrentRange.GetLowerBoundValue(), NewPos);
	}
	return FReply::Handled();
}
void SCinematicLevelViewport::CacheDesiredViewportSize(const FGeometry& AllottedGeometry)
{
	FVector2D ViewportControlsSize = ViewportControls->GetDesiredSize();

	FVector2D AllowableSpace = AllottedGeometry.GetLocalSize();
	AllowableSpace.Y -= ViewportControlsSize.Y;

	TSharedPtr<FCinematicViewportClient> ViewportClient = GetViewportClient();
	if (ViewportClient->IsAspectRatioConstrained())
	{
		const float MinSize = FMath::Min(AllowableSpace.X / ViewportClient->AspectRatio, AllowableSpace.Y);
		DesiredViewportSize = FVector2D(int32(ViewportClient->AspectRatio * MinSize), int32(MinSize));
	}
	else
	{
		DesiredViewportSize = AllowableSpace;
	}
}

FOptionalSize SCinematicLevelViewport::GetDesiredViewportWidth() const
{
	return DesiredViewportSize.X;
}

FOptionalSize SCinematicLevelViewport::GetDesiredViewportHeight() const
{
	return DesiredViewportSize.Y;
}

TSharedRef<SWidget> SCinematicLevelViewport::MakeCameraMenu()
{
	ISequencer* Sequencer = GetSequencer();
	UMovieSceneSequence* Sequence = Sequencer ? Sequencer->GetFocusedMovieSceneSequence() : nullptr;

	FMenuBuilder MenuBuilder(true, nullptr, nullptr);
	{
		// let toolkits populate the menu
		MenuBuilder.AddMenuEntry(
			LOCTEXT("FreeCamera", "Free Camera"),
			LOCTEXT("FreeCamera_ToolTip", "Allows the arbitrary movement of the camera"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SCinematicLevelViewport::SetCameraMode, ECinematicCameraMode::FreeCamera, TOptional<FGuid>()),
				FCanExecuteAction::CreateLambda([=]{ return true; }),
				FIsActionChecked::CreateLambda([=]{ return CameraMode == ECinematicCameraMode::FreeCamera; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		if (Sequence)
		{
			MenuBuilder.BeginSection("Sequence");
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("AutoCamera", "Auto"),
					LOCTEXT("AutoCamera_ToolTip", "Looks through the camera defined by either the shot track, camera cut track, or any other available camera (in that order)"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSP(this, &SCinematicLevelViewport::SetCameraMode, ECinematicCameraMode::Auto, TOptional<FGuid>()),
						FCanExecuteAction::CreateLambda([=]{ return true; }),
						FIsActionChecked::CreateLambda([=]{ return CameraMode == ECinematicCameraMode::Auto; })
					),
					NAME_None,
					EUserInterfaceActionType::ToggleButton
				);
			}
			MenuBuilder.EndSection();

			MenuBuilder.BeginSection("Cameras");
			{
				for (auto& Pair : Cameras)
				{
					FGuid CameraGuid = Pair.Key;

					MenuBuilder.AddMenuEntry(
						FText::FromString(Pair.Value.DisplayName),
						FText(),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateSP(this, &SCinematicLevelViewport::SetCameraMode, ECinematicCameraMode::SpecificCamera, TOptional<FGuid>(CameraGuid)),
							FCanExecuteAction::CreateLambda([=]{ return true; }),
							FIsActionChecked::CreateLambda([=]{
								return CameraMode == ECinematicCameraMode::SpecificCamera &&
									LockedCameraGuid.IsSet() &&
									LockedCameraGuid.GetValue() == CameraGuid;
							})
						),
						NAME_None,
						EUserInterfaceActionType::ToggleButton
					);
				}
			}
			MenuBuilder.EndSection();
		}
	}

	return MenuBuilder.MakeWidget();
}

void SCinematicLevelViewport::SetCameraMode(ECinematicCameraMode InCameraMode, TOptional<FGuid> InSpecificCameraGuid)
{
	CameraMode = InCameraMode;

	TSharedPtr<FCinematicViewportClient> ViewportClient = GetViewportClient();

	// Reset everything
	LockedCameraGuid = TOptional<FGuid>();
	ViewportClient->SetActorLock(nullptr);

	switch (CameraMode)
	{
	case ECinematicCameraMode::FreeCamera:
		ViewportClient->bDisableInput = false;
		break;

	case ECinematicCameraMode::Auto:
		ViewportClient->bDisableInput = true;
		break;

	case ECinematicCameraMode::SpecificCamera:
		LockedCameraGuid = InSpecificCameraGuid;
		ViewportClient->bDisableInput = true;
		break;
	}

	ISequencer* Sequencer = GetSequencer();
	if (Sequencer)
	{
		Sequencer->SetGlobalTime(Sequencer->GetGlobalTime());
	}
}

void SCinematicLevelViewport::UpdateActiveCameras()
{
	Cameras.Reset();

	ISequencer* Sequencer = GetSequencer();
	if (!Sequencer)
	{
		return;
	}

	TSharedPtr<FMovieSceneSequenceInstance> SequenceInstance = Sequencer->GetFocusedMovieSceneSequenceInstance();
	UMovieScene* MovieScene = SequenceInstance->GetSequence()->GetMovieScene();

	const int32 NumSpawnables = MovieScene->GetSpawnableCount();
	for (int32 SpawnableIndex = 0; SpawnableIndex < NumSpawnables; ++SpawnableIndex)
	{
		FMovieSceneSpawnable& Spawnable = MovieScene->GetSpawnable(SpawnableIndex);
		if (Spawnable.GetClass()->IsChildOf(AActor::StaticClass()))
		{
			AActor* CameraActor = Cast<AActor>(SequenceInstance->FindObject(Spawnable.GetGuid(), *Sequencer));
			if (CameraActor)
			{
				UCameraComponent* CameraComponent = MovieSceneHelpers::CameraComponentFromActor(CameraActor);
				if (CameraComponent)
				{
					Cameras.Add(Spawnable.GetGuid(), FActiveCamera(Spawnable.GetName(), CameraActor));
				}
			}
		}
	}

	const int32 NumPossessables = MovieScene->GetPossessableCount();
	for (int32 PossessableIndex = 0; PossessableIndex < NumPossessables; ++PossessableIndex)
	{
		FMovieScenePossessable& Possessable = MovieScene->GetPossessable(PossessableIndex);
		AActor* CameraActor = Cast<AActor>(SequenceInstance->FindObject(Possessable.GetGuid(), *Sequencer));
		if (CameraActor)
		{
			UCameraComponent* CameraComponent = MovieSceneHelpers::CameraComponentFromActor(CameraActor);
			if (CameraComponent)
			{
				Cameras.Add(Possessable.GetGuid(), FActiveCamera(Possessable.GetName(), CameraActor));
			}
		}
	}
}

TSharedPtr<FCinematicViewportClient> SCinematicLevelViewport::GetViewportClient() const
{
	return StaticCastSharedPtr<FCinematicViewportClient>(ViewportWidget->GetViewportClient());
}

void SCinematicLevelViewport::OnUpdateCameraCut(UObject* CameraObject)
{
	if (CameraMode != ECinematicCameraMode::Auto)
	{
		return;
	}

	AActor* CameraActor = Cast<AActor>(CameraObject);
	TSharedPtr<FCinematicViewportClient> ViewportClient = GetViewportClient();
	
	if (CameraActor)
	{
		ViewportClient->SetViewLocation(CameraActor->GetActorLocation());
		ViewportClient->SetViewRotation(CameraActor->GetActorRotation());
		//ViewportClient->bEditorCameraCut = !ViewportClient->IsLockedToActor(CameraActor);
		ViewportClient->bLockedCameraView = true;
	}
	else
	{
		ViewportClient->ViewFOV = ViewportClient->FOVAngle;
		//ViewportClient->bEditorCameraCut = false;
		ViewportClient->bLockedCameraView = false;
	}

	// Set the actor lock.
	ViewportClient->SetActorLock(CameraActor);
	ViewportClient->RemoveCameraRoll();

	// If viewing through a camera - enforce aspect ratio.
	if (CameraActor)
	{
		UCameraComponent* CameraComponent = MovieSceneHelpers::CameraComponentFromActor(CameraActor);
		if (CameraComponent)
		{
			if (CameraComponent->AspectRatio == 0)
			{
				ViewportClient->AspectRatio = 1.7f;
			}
			else
			{
				ViewportClient->AspectRatio = CameraComponent->AspectRatio;
			}

			//don't stop the camera from zooming when not playing back
			ViewportClient->ViewFOV = CameraComponent->FieldOfView;
		}
	}

	// Update ControllingActorViewInfo, so it is in sync with the updated viewport
	ViewportClient->UpdateViewForLockedActor();
}

FReply SCinematicLevelViewport::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) 
{
	if (CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}

	ISequencer* Sequencer = GetSequencer();
	if (Sequencer && Sequencer->GetCommandBindings()->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SCinematicLevelViewport::Setup(FLevelSequenceEditorToolkit& NewToolkit)
{
	CurrentToolkit = StaticCastSharedRef<FLevelSequenceEditorToolkit>(NewToolkit.AsShared());

	NewToolkit.OnClosed().AddSP(this, &SCinematicLevelViewport::OnEditorClosed);

	ISequencer* Sequencer = GetSequencer();
	if (Sequencer)
	{
		TypeInterfaceProxy->Impl = Sequencer->GetZeroPadNumericTypeInterface();

		OnCameraCutHandle = Sequencer->OnCameraCut().AddSP(this, &SCinematicLevelViewport::OnUpdateCameraCut);

		if (TransportRange.IsValid())
		{
			TransportRange->SetSequencer(Sequencer->AsShared());
		}

		if (TransportControls.IsValid())
		{
			TransportControls->SetContent(Sequencer->MakeTransportControls());
		}

		SetCameraMode(ECinematicCameraMode::Auto);
	}
	else
	{
		SetCameraMode(ECinematicCameraMode::FreeCamera);
	}
}

void SCinematicLevelViewport::CleanUp()
{
	ISequencer* Sequencer = GetSequencer();
	if (Sequencer && OnCameraCutHandle.IsValid())
	{
		Sequencer->OnCameraCut().Remove(OnCameraCutHandle);
	}

	TransportControls->SetContent(SNullWidget::NullWidget);
}

void SCinematicLevelViewport::OnEditorOpened(FLevelSequenceEditorToolkit& Toolkit)
{
	if (!CurrentToolkit.IsValid())
	{
		Setup(Toolkit);
	}
}

void SCinematicLevelViewport::OnEditorClosed()
{
	CleanUp();

	FLevelSequenceEditorToolkit* NewToolkit = nullptr;
	FLevelSequenceEditorToolkit::IterateOpenToolkits([&](FLevelSequenceEditorToolkit& Toolkit){
		NewToolkit = &Toolkit;
		return false;
	});

	if (NewToolkit)
	{
		Setup(*NewToolkit);
	}
	else
	{
		SetCameraMode(ECinematicCameraMode::FreeCamera);
	}
}

void SCinematicLevelViewport::BindCommands()
{
	FUICommandList& CommandListRef = *CommandList;

	const FLevelSequenceEditorCommands& Commands = FLevelSequenceEditorCommands::Get();

	CommandListRef.MapAction( 
		Commands.ToggleImmersive,
		FExecuteAction::CreateSP( this, &SCinematicLevelViewport::OnToggleImmersive ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SCinematicLevelViewport::IsImmersive));
}

ISequencer* SCinematicLevelViewport::GetSequencer() const
{
	TSharedPtr<FLevelSequenceEditorToolkit> Toolkit = CurrentToolkit.Pin();
	if (Toolkit.IsValid())
	{
		return Toolkit->GetSequencer().Get();
	}

	return nullptr;
}

void SCinematicLevelViewport::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	ISequencer* Sequencer = GetSequencer();
	UMovieSceneSequence* Sequence = Sequencer ? Sequencer->GetFocusedMovieSceneSequence() : nullptr;

	if (!Sequence)
	{
		return;
	}

	UpdateActiveCameras();

	TSharedPtr<FCinematicViewportClient> ViewportClient = GetViewportClient();

	// Update the specific locked camera if set. We do this each tick as it might be a spawnable camera
	if (LockedCameraGuid.IsSet())
	{
		FActiveCamera* Camera = Cameras.Find(LockedCameraGuid.GetValue());
		if (Camera && Camera->CameraActor.IsValid())
		{
			ViewportClient->SetActorLock(Camera->CameraActor.Get());

			ViewportClient->bLockedCameraView = true;
			ViewportClient->bDisableInput = true;
		}
		else
		{
			// If the camera's not available, just use free cam until it is
			ViewportClient->SetActorLock(nullptr);
			ViewportClient->bLockedCameraView = false;
			ViewportClient->bDisableInput = false;
		}
	}

	// Find either a cinematic shot track or a sub track
	UMovieSceneSubTrack* SubTrack = Cast<UMovieSceneSubTrack>(Sequence->GetMovieScene()->FindMasterTrack(UMovieSceneCinematicShotTrack::StaticClass()));
	if (!SubTrack)
	{
		SubTrack = Cast<UMovieSceneSubTrack>(Sequence->GetMovieScene()->FindMasterTrack(UMovieSceneSubTrack::StaticClass()));
	}

	UMovieSceneSubSection* SubSection = nullptr;
	if (SubTrack)
	{
		const float CurrentTime = Sequencer->GetGlobalTime();

		for (UMovieSceneSection* Section : SubTrack->GetAllSections())
		{
			if (Section->IsInfinite() || Section->IsTimeWithinSection(CurrentTime))
			{
				SubSection = CastChecked<UMovieSceneSubSection>(Section);
			}
		}
	}

	const float AbsoluteTime = Sequencer->GetGlobalTime();

	FText TimeFormat = LOCTEXT("TimeFormat", "{0}");

	TSharedPtr<INumericTypeInterface<float>> ZeroPadTypeInterface = Sequencer->GetZeroPadNumericTypeInterface();

	if (SubSection)
	{
		float PlaybackRangeStart = 0.f;
		if (SubSection->GetSequence() != nullptr)
		{
			UMovieScene* ShotMovieScene = SubSection->GetSequence()->GetMovieScene();
			PlaybackRangeStart = ShotMovieScene->GetPlaybackRange().GetLowerBoundValue();
		}

		const float ShotOffset = SubSection->StartOffset + PlaybackRangeStart - SubSection->PrerollTime;
		const float AbsoluteShotPosition = ShotOffset + (AbsoluteTime - (SubSection->GetStartTime() - SubSection->PrerollTime)) / SubSection->TimeScale;
		UIData.Frame = FText::Format(
			TimeFormat,
			FText::FromString(ZeroPadTypeInterface->ToString(AbsoluteShotPosition))
		);

		UMovieSceneCinematicShotSection* CinematicShotSection = Cast<UMovieSceneCinematicShotSection>(SubSection);
		if (CinematicShotSection)
		{
			UIData.ShotName = CinematicShotSection->GetShotDisplayName();
		}
		else
		{
			UIData.ShotName = SubSection->GetSequence()->GetDisplayName();
		}
	}
	else
	{
		UIData.Frame = FText::Format(
			TimeFormat,
			FText::FromString(ZeroPadTypeInterface->ToString(AbsoluteTime))
			);

		UIData.ShotName = Sequence->GetDisplayName();
	}

	TRange<float> EntireRange = Sequence->GetMovieScene()->GetEditorData().WorkingRange;

	UIData.MasterStartText = FText::Format(
		TimeFormat,
		FText::FromString(ZeroPadTypeInterface->ToString(EntireRange.GetLowerBoundValue()))
	);

	UIData.MasterEndText = FText::Format(
		TimeFormat,
		FText::FromString(ZeroPadTypeInterface->ToString(EntireRange.GetUpperBoundValue()))
	);

	UCameraComponent* CameraComponent = ViewportClient->GetCameraComponentForView();
	if (CameraComponent)
	{
		if (UCineCameraComponent* CineCam = Cast<UCineCameraComponent>(CameraComponent))
		{
			const float SensorWidth = CineCam->FilmbackSettings.SensorWidth;
			const float SensorHeight = CineCam->FilmbackSettings.SensorHeight;

			// Search presets for one that matches
			const FNamedFilmbackPreset* Preset = UCineCameraComponent::GetFilmbackPresets().FindByPredicate([&](const FNamedFilmbackPreset& InPreset){
				return InPreset.FilmbackSettings.SensorWidth == SensorWidth && InPreset.FilmbackSettings.SensorHeight == SensorHeight;
			});

			if (Preset)
			{
				UIData.Filmback = FText::FromString(Preset->Name);
			}
			else
			{
				FNumberFormattingOptions Opts = FNumberFormattingOptions().SetMaximumFractionalDigits(1);
				UIData.Filmback = FText::Format(
					LOCTEXT("CustomFilmbackFormat", "Custom ({0}mm x {1}mm)"),
					FText::AsNumber(SensorWidth, &Opts),
					FText::AsNumber(SensorHeight, &Opts)
				);
			}
		}
		else
		{
			// Just use the FoV
			UIData.Filmback = FText::FromString(LexicalConversion::ToString(FNumericUnit<float>(CameraComponent->FieldOfView, EUnit::Degrees)));
		}
	}
	else
	{
		UIData.Filmback = FText();
	}
}

#undef LOCTEXT_NAMESPACE