// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SSequencer.h"
#include "Editor/SequencerWidgets/Public/SequencerWidgetsModule.h"
#include "Sequencer.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "ISequencerSection.h"
#include "ScopedTransaction.h"
#include "MovieSceneTrack.h"
#include "MovieSceneTrackEditor.h"
#include "SSequencerSectionOverlay.h"
#include "SSequencerTrackArea.h"
#include "SSequencerTrackOutliner.h"
#include "SequencerNodeTree.h"
#include "TimeSliderController.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "DragAndDrop/ActorDragDropGraphEdOp.h"
#include "DragAndDrop/ClassDragDropOp.h"
#include "AssetSelection.h"
#include "MovieSceneCameraCutSection.h"
#include "CommonMovieSceneTools.h"
#include "SSearchBox.h"
#include "SNumericDropDown.h"
#include "EditorWidgetsModule.h"
#include "SSequencerTreeView.h"
#include "MovieSceneSequence.h"
#include "SSequencerSplitterOverlay.h"
#include "IKeyArea.h"
#include "VirtualTrackArea.h"
#include "SequencerHotspots.h"
#include "SSequencerShotFilterOverlay.h"
#include "GenericCommands.h"
#include "SequencerContextMenus.h"
#include "SSequencerTreeViewBox.h"
#include "NumericTypeInterface.h"
#include "NumericUnitTypeInterface.inl"
#include "SNumericEntryBox.h"
#include "SSequencerGotoBox.h"


#define LOCTEXT_NAMESPACE "Sequencer"

DECLARE_DELEGATE_RetVal(bool, FOnGetShowFrames)
DECLARE_DELEGATE_RetVal(uint8, FOnGetZeroPad)

/* Numeric type interface for showing numbers as frames or times */
struct FFramesOrTimeInterface : public TNumericUnitTypeInterface<float>
{
	FFramesOrTimeInterface(FOnGetShowFrames InShowFrameNumbers,TSharedPtr<FSequencerTimeSliderController> InController, FOnGetZeroPad InOnGetZeroPad)
		: TNumericUnitTypeInterface(EUnit::Seconds)
		, ShowFrameNumbers(MoveTemp(InShowFrameNumbers))
		, TimeSliderController(MoveTemp(InController))
		, OnGetZeroPad(MoveTemp(InOnGetZeroPad))
	{}

private:
	FOnGetShowFrames ShowFrameNumbers;
	TSharedPtr<FSequencerTimeSliderController> TimeSliderController;
	FOnGetZeroPad OnGetZeroPad;

	virtual FString ToString(const float& Value) const override
	{
		if (ShowFrameNumbers.Execute())
		{
			int32 Frame = TimeSliderController->TimeToFrame(Value);
			if (OnGetZeroPad.IsBound())
			{
				return FString::Printf(*FString::Printf(TEXT("%%0%dd"), OnGetZeroPad.Execute()), Frame);
			}
			return FString::Printf(TEXT("%d"), Frame);
		}

		return FString::Printf(TEXT("%.2fs"), Value);
	}

	virtual TOptional<float> FromString(const FString& InString, const float& InExistingValue) override
	{
		if (ShowFrameNumbers.Execute())
		{
			int32 NewEndFrame = FCString::Atoi(*InString);
			return float(TimeSliderController->FrameToTime(NewEndFrame));
		}

		return TNumericUnitTypeInterface::FromString(InString, InExistingValue);
	}
};


/* SSequencer interface
 *****************************************************************************/
PRAGMA_DISABLE_OPTIMIZATION
void SSequencer::Construct(const FArguments& InArgs, TSharedRef<FSequencer> InSequencer)
{
	SequencerPtr = InSequencer;
	bIsActiveTimerRegistered = false;
	bUserIsSelecting = false;

	USelection::SelectionChangedEvent.AddSP(this, &SSequencer::OnActorSelectionChanged);

	Settings = InSequencer->GetSettings();
	Settings->GetOnShowCurveEditorChanged().AddSP(this, &SSequencer::OnCurveEditorVisibilityChanged);

	// Create a node tree which contains a tree of movie scene data to display in the sequence
	SequencerNodeTree = MakeShareable( new FSequencerNodeTree( InSequencer.Get() ) );

	FSequencerWidgetsModule& SequencerWidgets = FModuleManager::Get().LoadModuleChecked<FSequencerWidgetsModule>( "SequencerWidgets" );

	OnBeginPlaybackRangeDrag = InArgs._OnBeginPlaybackRangeDrag;
	OnEndPlaybackRangeDrag = InArgs._OnEndPlaybackRangeDrag;

	FTimeSliderArgs TimeSliderArgs;
	{
		TimeSliderArgs.ViewRange = InArgs._ViewRange;
		TimeSliderArgs.ClampRange = InArgs._ClampRange;
		TimeSliderArgs.PlaybackRange = InArgs._PlaybackRange;
		TimeSliderArgs.OnPlaybackRangeChanged = InArgs._OnPlaybackRangeChanged;
		TimeSliderArgs.OnBeginPlaybackRangeDrag = OnBeginPlaybackRangeDrag;
		TimeSliderArgs.OnEndPlaybackRangeDrag = OnEndPlaybackRangeDrag;
		TimeSliderArgs.OnViewRangeChanged = InArgs._OnViewRangeChanged;
		TimeSliderArgs.OnClampRangeChanged = InArgs._OnClampRangeChanged;
		TimeSliderArgs.ScrubPosition = InArgs._ScrubPosition;
		TimeSliderArgs.OnBeginScrubberMovement = InArgs._OnBeginScrubbing;
		TimeSliderArgs.OnEndScrubberMovement = InArgs._OnEndScrubbing;
		TimeSliderArgs.OnScrubPositionChanged = InArgs._OnScrubPositionChanged;
		TimeSliderArgs.Settings = Settings;
	}

	TSharedRef<FSequencerTimeSliderController> TimeSliderController( new FSequencerTimeSliderController( TimeSliderArgs ) );
	
	{
		auto ShowFrameNumbersDelegate = FOnGetShowFrames::CreateSP(this, &SSequencer::ShowFrameNumbers);
		USequencerSettings* SequencerSettings = Settings;
		auto GetZeroPad = [=]() -> uint8 {
			if (SequencerSettings)
			{
				return SequencerSettings->GetZeroPadFrames();
			}
			return 0;
		};

		NumericTypeInterface = MakeShareable( new FFramesOrTimeInterface(ShowFrameNumbersDelegate, TimeSliderController, FOnGetZeroPad()) );
		ZeroPadNumericTypeInterface = MakeShareable( new FFramesOrTimeInterface(ShowFrameNumbersDelegate, TimeSliderController, FOnGetZeroPad::CreateLambda(GetZeroPad)) );
	}

	bool bMirrorLabels = false;
	// Create the top and bottom sliders
	TSharedRef<ITimeSlider> TopTimeSlider = SequencerWidgets.CreateTimeSlider( TimeSliderController, bMirrorLabels );
	bMirrorLabels = true;
	TSharedRef<ITimeSlider> BottomTimeSlider = SequencerWidgets.CreateTimeSlider( TimeSliderController, TAttribute<EVisibility>(this, &SSequencer::GetBottomTimeSliderVisibility), bMirrorLabels );

	// Create bottom time range slider
	TSharedRef<ITimeSlider> BottomTimeRange = SequencerWidgets.CreateTimeRange( TimeSliderController, TAttribute<EVisibility>(this, &SSequencer::GetTimeRangeVisibility), TAttribute<bool>(this, &SSequencer::ShowFrameNumbers), TAttribute<float>(this, &SSequencer::OnGetTimeSnapInterval));

	OnGetAddMenuContent = InArgs._OnGetAddMenuContent;
	AddMenuExtender = InArgs._AddMenuExtender;

	ColumnFillCoefficients[0] = 0.3f;
	ColumnFillCoefficients[1] = 0.7f;

	TAttribute<float> FillCoefficient_0, FillCoefficient_1;
	{
		FillCoefficient_0.Bind(TAttribute<float>::FGetter::CreateSP(this, &SSequencer::GetColumnFillCoefficient, 0));
		FillCoefficient_1.Bind(TAttribute<float>::FGetter::CreateSP(this, &SSequencer::GetColumnFillCoefficient, 1));
	}

	TSharedRef<SScrollBar> ScrollBar = SNew(SScrollBar)
		.Thickness(FVector2D(5.0f, 5.0f));

	SAssignNew(TrackOutliner, SSequencerTrackOutliner);
	SAssignNew(TrackArea, SSequencerTrackArea, TimeSliderController, InSequencer);
	SAssignNew(TreeView, SSequencerTreeView, SequencerNodeTree.ToSharedRef(), TrackArea.ToSharedRef())
		.ExternalScrollbar(ScrollBar);

	SAssignNew(CurveEditor, SSequencerCurveEditor, InSequencer, TimeSliderController)
		.Visibility(this, &SSequencer::GetCurveEditorVisibility)
		.OnViewRangeChanged(InArgs._OnViewRangeChanged)
		.ViewRange(InArgs._ViewRange);

	CurveEditor->SetAllowAutoFrame(Settings->GetShowCurveEditor());
	TrackArea->SetTreeView(TreeView);

	const int32 Column0 = 0, Column1 = 1;
	const int32 Row0 = 0, Row1 = 1, Row2 = 2, Row3 = 3;

	const float CommonPadding = 3.f;
	const FMargin ResizeBarPadding(4.f, 0, 0, 0);

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			
			+ SSplitter::Slot()
			.Value(0.1f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				.Visibility(this, &SSequencer::HandleLabelBrowserVisibility)
				[
					// track label browser
					SAssignNew(LabelBrowser, SSequencerLabelBrowser, InSequencer)
					.OnSelectionChanged(this, &SSequencer::HandleLabelBrowserSelectionChanged)
				]
			]

			+ SSplitter::Slot()
			.Value(0.9f)
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				[
					// track area grid panel
					SNew( SGridPanel )
					.FillRow( 2, 1.f )
					.FillColumn( 0, FillCoefficient_0 )
					.FillColumn( 1, FillCoefficient_1 )

					// Toolbar
					+ SGridPanel::Slot( Column0, Row0, SGridPanel::Layer(10) )
					.ColumnSpan(2)
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.Padding(FMargin(CommonPadding, 0.f))
						[
							SNew(SHorizontalBox)

							+SHorizontalBox::Slot()
							.AutoWidth()
							[
								MakeToolBar()
							]

							+SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SSequencerCurveEditorToolBar, InSequencer, CurveEditor->GetCommands())
								.Visibility(this, &SSequencer::GetCurveEditorToolBarVisibility)
							]

							+ SHorizontalBox::Slot()
							.HAlign(HAlign_Right)
							.VAlign(VAlign_Center)
							[
								SAssignNew(BreadcrumbTrail, SBreadcrumbTrail<FSequencerBreadcrumb>)
								.Visibility(this, &SSequencer::GetBreadcrumbTrailVisibility)
								.OnCrumbClicked(this, &SSequencer::OnCrumbClicked)
								.ButtonStyle(FEditorStyle::Get(), "FlatButton")
								.DelimiterImage(FEditorStyle::GetBrush("Sequencer.BreadcrumbIcon"))
								.TextStyle(FEditorStyle::Get(), "Sequencer.BreadcrumbText")
							]
						]
					]

					+ SGridPanel::Slot( Column0, Row1 )
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							SNew(SSpacer)
						]
					]

					// outliner search box
					+ SGridPanel::Slot( Column0, Row1, SGridPanel::Layer(10) )
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.Padding(FMargin(CommonPadding*2, CommonPadding))
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(FMargin(0.f, 0.f, CommonPadding, 0.f))
							[
								MakeAddButton()
							]

							+ SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SAssignNew(SearchBox, SSearchBox)
								.HintText(LOCTEXT("FilterNodesHint", "Filter"))
								.OnTextChanged( this, &SSequencer::OnOutlinerSearchChanged )
							]
						]
					]

					// main sequencer area
					+ SGridPanel::Slot( Column0, Row2, SGridPanel::Layer(10) )
					.ColumnSpan(2)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						[
							SNew( SOverlay )

							+ SOverlay::Slot()
							[
								SNew(SScrollBorder, TreeView.ToSharedRef())
								[
									SNew(SHorizontalBox)

									// outliner tree
									+ SHorizontalBox::Slot()
									.FillWidth( FillCoefficient_0 )
									[
										SNew(SSequencerTreeViewBox, InSequencer, SharedThis(this))
										[
											TreeView.ToSharedRef()
										]
									]

									// track area
									+ SHorizontalBox::Slot()
									.FillWidth( FillCoefficient_1 )
									[
										SNew(SBox)
										.Padding(ResizeBarPadding)
										.Visibility(this, &SSequencer::GetTrackAreaVisibility )
										[
											TrackArea.ToSharedRef()
										]
									]
								]
							]

							+ SOverlay::Slot()
							.HAlign( HAlign_Right )
							[
								ScrollBar
							]
						]

						+ SHorizontalBox::Slot()
						.FillWidth( TAttribute<float>( this, &SSequencer::GetOutlinerSpacerFill ) )
						[
							SNew(SSpacer)
						]
					]

					// playback buttons
					+ SGridPanel::Slot( Column0, Row3, SGridPanel::Layer(10) )
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						//.BorderBackgroundColor(FLinearColor(.50f, .50f, .50f, 1.0f))
						.HAlign(HAlign_Center)
						[
							MakeTransportControls()
						]
					]

					// Second column

					+ SGridPanel::Slot( Column1, Row1 )
					.Padding(ResizeBarPadding)
					.RowSpan(3)
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							SNew(SSpacer)
						]
					]

					+ SGridPanel::Slot( Column1, Row1, SGridPanel::Layer(10) )
					.Padding(ResizeBarPadding)
					[
						SNew( SBorder )
						.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
						.BorderBackgroundColor( FLinearColor(.50f, .50f, .50f, 1.0f ) )
						.Padding(0)
						[
							TopTimeSlider
						]
					]

					// Overlay that draws the tick lines
					+ SGridPanel::Slot( Column1, Row2, SGridPanel::Layer(10) )
					.Padding(ResizeBarPadding)
					[
						SNew( SSequencerSectionOverlay, TimeSliderController )
						.Visibility( EVisibility::HitTestInvisible )
						.DisplayScrubPosition( false )
						.DisplayTickLines( true )
					]

					// Goto box
					+ SGridPanel::Slot( Column1, Row2, SGridPanel::Layer(11) )
						.Padding(ResizeBarPadding)
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Top)
						[
							SAssignNew(GotoBox, SSequencerGotoBox, SequencerPtr.Pin().ToSharedRef(), *Settings, NumericTypeInterface.ToSharedRef())
						]

					// Curve editor
					+ SGridPanel::Slot( Column1, Row2, SGridPanel::Layer(20) )
					.Padding(ResizeBarPadding)
					[
						CurveEditor.ToSharedRef()
					]

					// Overlay that draws the scrub position
					+ SGridPanel::Slot( Column1, Row2, SGridPanel::Layer(30) )
					.Padding(ResizeBarPadding)
					[
						SNew( SSequencerSectionOverlay, TimeSliderController )
						.Visibility( EVisibility::HitTestInvisible )
						.DisplayScrubPosition( true )
						.DisplayTickLines( false )
						.PaintPlaybackRangeArgs(this, &SSequencer::GetSectionPlaybackRangeArgs)
					]

					// play range sliders
					+ SGridPanel::Slot( Column1, Row3, SGridPanel::Layer(10) )
					.Padding(ResizeBarPadding)
					[
						SNew( SBorder )
						.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
						.BorderBackgroundColor( FLinearColor(.50f, .50f, .50f, 1.0f ) )
						.Padding(0)
						[
							SNew( SOverlay )

							+ SOverlay::Slot()
							[
								BottomTimeSlider
							]

							+ SOverlay::Slot()
							[
								BottomTimeRange
							]
						]
					]
				]

				+ SOverlay::Slot()
				[
					// track area virtual splitter overlay
					SNew(SSequencerSplitterOverlay)
					.Style(FEditorStyle::Get(), "Sequencer.AnimationOutliner.Splitter")
					.Visibility(EVisibility::SelfHitTestInvisible)

					+ SSplitter::Slot()
					.Value(FillCoefficient_0)
					.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SSequencer::OnColumnFillCoefficientChanged, 0))
					[
						SNew(SSpacer)
					]

					+ SSplitter::Slot()
					.Value(FillCoefficient_1)
					.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SSequencer::OnColumnFillCoefficientChanged, 1))
					[
						SNew(SSpacer)
					]
				]
			]
		]
	];

	InSequencer->GetSelection().GetOnKeySelectionChanged().AddSP(this, &SSequencer::HandleKeySelectionChanged);
	InSequencer->GetSelection().GetOnSectionSelectionChanged().AddSP(this, &SSequencer::HandleSectionSelectionChanged);

	ResetBreadcrumbs();
}
PRAGMA_ENABLE_OPTIMIZATION

void SSequencer::BindCommands(TSharedRef<FUICommandList> SequencerCommandBindings)
{
	auto CanPaste = [this]{
		if (!HasFocusedDescendants() && !HasKeyboardFocus())
		{
			return false;
		}

		return SequencerPtr.Pin()->GetClipboardStack().Num() != 0;
	};
	
	SequencerCommandBindings->MapAction(
		FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &SSequencer::Paste),
		FCanExecuteAction::CreateLambda(CanPaste)
	);

	SequencerCommandBindings->MapAction(
		FSequencerCommands::Get().PasteFromHistory,
		FExecuteAction::CreateSP(this, &SSequencer::PasteFromHistory),
		FCanExecuteAction::CreateLambda(CanPaste)
	);
}


void SSequencer::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FEditPropertyChain* PropertyThatChanged)
{
	// @todo sequencer: is this still needed?
}


/* SSequencer implementation
 *****************************************************************************/

TSharedRef<INumericTypeInterface<float>> SSequencer::GetNumericTypeInterface()
{
	return NumericTypeInterface.ToSharedRef();
}

TSharedRef<INumericTypeInterface<float>> SSequencer::GetZeroPadNumericTypeInterface()
{
	return ZeroPadNumericTypeInterface.ToSharedRef();
}

/* SSequencer callbacks
 *****************************************************************************/

void SSequencer::HandleKeySelectionChanged()
{
}


void SSequencer::HandleLabelBrowserSelectionChanged(FString NewLabel, ESelectInfo::Type SelectInfo)
{
	if (SelectInfo == ESelectInfo::Direct)
	{
		return;
	}

	if (NewLabel.IsEmpty())
	{
		SearchBox->SetText(FText::GetEmpty());
	}
	else
	{
		SearchBox->SetText(FText::FromString(FString(TEXT("label:")) + NewLabel));
	}
}


EVisibility SSequencer::HandleLabelBrowserVisibility() const
{
	if (Settings->GetLabelBrowserVisible())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


void SSequencer::HandleSectionSelectionChanged()
{
}


TSharedRef<SWidget> SSequencer::MakeAddButton()
{
	return SNew(SComboButton)
	.OnGetMenuContent(this, &SSequencer::MakeAddMenu)
	.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
	.ContentPadding(FMargin(2.0f, 1.0f))
	.HasDownArrow(false)
	.ButtonContent()
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(STextBlock)
			.TextStyle(FEditorStyle::Get(), "NormalText.Important")
			.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.10"))
			.Text(FText::FromString(FString(TEXT("\xf067"))) /*fa-plus*/)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4, 0, 0, 0)
		[
			SNew(STextBlock)
			.TextStyle(FEditorStyle::Get(), "NormalText.Important")
			.Text(LOCTEXT("AddButton", "Add"))
		]

		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(4, 0, 0, 0)
		[
			SNew(STextBlock)
			.TextStyle(FEditorStyle::Get(), "NormalText.Important")
			.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.10"))
			.Text(FText::FromString(FString(TEXT("\xf0d7"))) /*fa-caret-down*/)
		]
	];
}

TSharedRef<SWidget> SSequencer::MakeToolBar()
{
	FToolBarBuilder ToolBarBuilder( SequencerPtr.Pin()->GetCommandBindings(), FMultiBoxCustomization::None, TSharedPtr<FExtender>(), Orient_Horizontal, true);
	
	ToolBarBuilder.BeginSection("Base Commands");
	{
		// General 
		if( SequencerPtr.Pin()->IsLevelEditorSequencer() )
		{
			ToolBarBuilder.AddToolBarButton(
				FUIAction(FExecuteAction::CreateSP(this, &SSequencer::OnSaveMovieSceneClicked)),
				NAME_None,
				LOCTEXT("SaveDirtyPackages", "Save"),
				LOCTEXT("SaveDirtyPackagesTooltip", "Saves the current level sequence"),
				FSlateIcon(FEditorStyle::GetStyleSetName(), "Sequencer.Save")
			);

			ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().RenderMovie );
			ToolBarBuilder.AddSeparator();
		}

		ToolBarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateSP(this, &SSequencer::MakeGeneralMenu),
			LOCTEXT("GeneralOptions", "General Options"),
			LOCTEXT("GeneralOptionsToolTip", "General Options"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "Sequencer.General")
		);

		ToolBarBuilder.AddSeparator();

		if( SequencerPtr.Pin()->IsLevelEditorSequencer() )
		{
			TAttribute<FSlateIcon> KeyAllIcon;
			KeyAllIcon.Bind(TAttribute<FSlateIcon>::FGetter::CreateLambda([&]{
				static FSlateIcon KeyAllEnabledIcon(FEditorStyle::GetStyleSetName(), "Sequencer.KeyAllEnabled");
				static FSlateIcon KeyAllDisabledIcon(FEditorStyle::GetStyleSetName(), "Sequencer.KeyAllDisabled");

				return SequencerPtr.Pin()->GetKeyAllEnabled() ? KeyAllEnabledIcon : KeyAllDisabledIcon;
			}));

			ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().ToggleKeyAllEnabled, NAME_None, TAttribute<FText>(), TAttribute<FText>(), KeyAllIcon );
		}

		TAttribute<FSlateIcon> AutoKeyModeIcon;
		AutoKeyModeIcon.Bind(TAttribute<FSlateIcon>::FGetter::CreateLambda( [&] {
			switch ( SequencerPtr.Pin()->GetAutoKeyMode() )
			{
			case EAutoKeyMode::KeyAll:
				return FSequencerCommands::Get().SetAutoKeyModeAll->GetIcon();
			case EAutoKeyMode::KeyAnimated:
				return FSequencerCommands::Get().SetAutoKeyModeAnimated->GetIcon();
			default: // EAutoKeyMode::KeyNone
				return FSequencerCommands::Get().SetAutoKeyModeNone->GetIcon();
			}
		} ) );

		TAttribute<FText> AutoKeyModeToolTip;
		AutoKeyModeToolTip.Bind( TAttribute<FText>::FGetter::CreateLambda( [&] {
			switch ( SequencerPtr.Pin()->GetAutoKeyMode() )
			{
			case EAutoKeyMode::KeyAll:
				return FSequencerCommands::Get().SetAutoKeyModeAll->GetDescription();
			case EAutoKeyMode::KeyAnimated:
				return FSequencerCommands::Get().SetAutoKeyModeAnimated->GetDescription();
			default: // EAutoKeyMode::KeyNone
				return FSequencerCommands::Get().SetAutoKeyModeNone->GetDescription();
			}
		} ) );

		ToolBarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateSP(this, &SSequencer::MakeAutoKeyMenu),
			LOCTEXT("AutoKeyMode", "Auto-Key Mode"),
			AutoKeyModeToolTip,
			AutoKeyModeIcon);
	}
	ToolBarBuilder.EndSection();


	ToolBarBuilder.BeginSection("Snapping");
	{
		ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().ToggleIsSnapEnabled, NAME_None, TAttribute<FText>( FText::GetEmpty() ) );

		ToolBarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateSP( this, &SSequencer::MakeSnapMenu ),
			LOCTEXT( "SnapOptions", "Options" ),
			LOCTEXT( "SnapOptionsToolTip", "Snapping Options" ),
			TAttribute<FSlateIcon>(),
			true );

		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddWidget(
			SNew( SImage )
				.Image(FEditorStyle::GetBrush("Sequencer.Time")) );

		ToolBarBuilder.AddWidget(
			SNew( SBox )
				.VAlign( VAlign_Center )
				[
					SNew( SNumericDropDown<float> )
						.DropDownValues( SequencerSnapValues::GetTimeSnapValues() )
						.bShowNamedValue(true)
						.ToolTipText( LOCTEXT( "TimeSnappingIntervalToolTip", "Time snapping interval" ) )
						.Value( this, &SSequencer::OnGetTimeSnapInterval )
						.OnValueChanged( this, &SSequencer::OnTimeSnapIntervalChanged )
				]);
	}
	ToolBarBuilder.EndSection();

	ToolBarBuilder.BeginSection("Curve Editor");
	{
		ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().ToggleShowCurveEditor );
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}


TSharedRef<SWidget> SSequencer::MakeAddMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr, AddMenuExtender);
	{

		// let toolkits populate the menu
		MenuBuilder.BeginSection("MainMenu");
		OnGetAddMenuContent.ExecuteIfBound(MenuBuilder, SequencerPtr.Pin().ToSharedRef());
		MenuBuilder.EndSection();

		// let track editors populate the menu
		TSharedPtr<FSequencer> Sequencer = SequencerPtr.Pin();

		// Always create the section so that we afford extension
		MenuBuilder.BeginSection("AddTracks");
		if (Sequencer.IsValid())
		{
			Sequencer->BuildAddTrackMenu(MenuBuilder);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}


TSharedRef<SWidget> SSequencer::MakeGeneralMenu()
{
	FMenuBuilder MenuBuilder( true, SequencerPtr.Pin()->GetCommandBindings() );

	MenuBuilder.BeginSection( "ViewOptions", LOCTEXT( "ViewMenuHeader", "View" ) );
	{
		if (SequencerPtr.Pin()->IsLevelEditorSequencer())
		{
			MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleLabelBrowser );
		}

		if (SequencerPtr.Pin()->IsLevelEditorSequencer())
		{
			MenuBuilder.AddMenuEntry( FSequencerCommands::Get().FindInContentBrowser );
		}

		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleExpandCollapseNodes );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleExpandCollapseNodesAndDescendants );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ExpandAllNodesAndDescendants );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().CollapseAllNodesAndDescendants );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection( "Ranges", LOCTEXT( "RangesHeader", "Playback Range" ) );
	{
		TSharedPtr<FSequencer> Sequencer = SequencerPtr.Pin();
		
		// Menu entry for the start position
		auto OnStartChanged = [=](float NewValue){
			float Upper = Sequencer->GetPlaybackRange().GetUpperBoundValue();
			Sequencer->SetPlaybackRange(TRange<float>(FMath::Min(NewValue, Upper), Upper));
		};
		MenuBuilder.AddWidget(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SSpacer)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SSpinBox<float>)
				.TypeInterface(NumericTypeInterface)
				.Style(&FEditorStyle::GetWidgetStyle<FSpinBoxStyle>("Sequencer.HyperlinkSpinBox"))
				.OnValueCommitted_Lambda([=](float Value, ETextCommit::Type){ OnStartChanged(Value); })
				.OnValueChanged_Lambda(OnStartChanged)
				.OnBeginSliderMovement(OnBeginPlaybackRangeDrag)
				.OnEndSliderMovement_Lambda([=](float Value){ OnStartChanged(Value); OnEndPlaybackRangeDrag.ExecuteIfBound(); })
				.MinValue_Lambda([=]() -> float {
					return Sequencer->GetClampRange().GetLowerBoundValue(); 
				})
				.MaxValue_Lambda([=]() -> float {
					return Sequencer->GetPlaybackRange().GetUpperBoundValue(); 
				})
				.Value_Lambda([=]() -> float {
					return Sequencer->GetPlaybackRange().GetLowerBoundValue();
				})
			],
			LOCTEXT("PlaybackStartLabel", "Start"));

		// Menu entry for the end position
		auto OnEndChanged = [=](float NewValue){
			float Lower = Sequencer->GetPlaybackRange().GetLowerBoundValue();
			Sequencer->SetPlaybackRange(TRange<float>(Lower, FMath::Max(NewValue, Lower)));
		};
		MenuBuilder.AddWidget(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SSpacer)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SSpinBox<float>)
				.TypeInterface(NumericTypeInterface)
				.Style(&FEditorStyle::GetWidgetStyle<FSpinBoxStyle>("Sequencer.HyperlinkSpinBox"))
				.OnValueCommitted_Lambda([=](float Value, ETextCommit::Type){ OnEndChanged(Value); })
				.OnValueChanged_Lambda(OnEndChanged)
				.OnBeginSliderMovement(OnBeginPlaybackRangeDrag)
				.OnEndSliderMovement_Lambda([=](float Value){ OnEndChanged(Value); OnEndPlaybackRangeDrag.ExecuteIfBound(); })
				.MinValue_Lambda([=]() -> float {
					return Sequencer->GetPlaybackRange().GetLowerBoundValue(); 
				})
				.MaxValue_Lambda([=]() -> float {
					return Sequencer->GetClampRange().GetUpperBoundValue(); 
				})
				.Value_Lambda([=]() -> float {
					return Sequencer->GetPlaybackRange().GetUpperBoundValue();
				})
			],
			LOCTEXT("PlaybackStartEnd", "End"));

		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleKeepCursorInPlaybackRange );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleKeepPlaybackRangeInSectionBounds );

		// Menu entry for zero padding
		auto OnZeroPadChanged = [=](uint8 NewValue){
			Settings->SetZeroPadFrames(NewValue);
		};
		MenuBuilder.AddWidget(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SSpacer)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SSpinBox<uint8>)
				.Style(&FEditorStyle::GetWidgetStyle<FSpinBoxStyle>("Sequencer.HyperlinkSpinBox"))
				.OnValueCommitted_Lambda([=](uint8 Value, ETextCommit::Type){ OnZeroPadChanged(Value); })
				.OnValueChanged_Lambda(OnZeroPadChanged)
				.MinValue(0)
				.MaxValue(8)
				.Value_Lambda([=]() -> uint8 {
					return Settings->GetZeroPadFrames();
				})
			],
			LOCTEXT("ZeroPaddingText", "Zero Pad Frame Numbers"));

		MenuBuilder.AddMenuSeparator();
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleShowGotoBox );
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SSequencer::MakeSnapMenu()
{
	FMenuBuilder MenuBuilder( false, SequencerPtr.Pin()->GetCommandBindings() );

	MenuBuilder.BeginSection("FramesRanges", LOCTEXT("SnappingMenuFrameRangesHeader", "Frame Ranges") );
	{
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleAutoScroll );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleShowFrameNumbers );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleShowRangeSlider );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection( "KeySnapping", LOCTEXT( "SnappingMenuKeyHeader", "Key Snapping" ) );
	{
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleSnapKeyTimesToInterval );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleSnapKeyTimesToKeys );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection( "SectionSnapping", LOCTEXT( "SnappingMenuSectionHeader", "Section Snapping" ) );
	{
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleSnapSectionTimesToInterval );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleSnapSectionTimesToSections );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection( "PlayTimeSnapping", LOCTEXT( "SnappingMenuPlayTimeHeader", "Play Time Snapping" ) );
	{
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleSnapPlayTimeToInterval );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleSnapPlayTimeToDraggedKey );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleFixedTimeStepPlayback );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection( "CurveSnapping", LOCTEXT( "SnappingMenuCurveHeader", "Curve Snapping" ) );
	{
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleSnapCurveValueToInterval );
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}


TSharedRef<SWidget> SSequencer::MakeAutoKeyMenu()
{
	FMenuBuilder MenuBuilder(false, SequencerPtr.Pin()->GetCommandBindings());

	MenuBuilder.AddMenuEntry(FSequencerCommands::Get().SetAutoKeyModeAll);
	MenuBuilder.AddMenuEntry(FSequencerCommands::Get().SetAutoKeyModeAnimated);
	MenuBuilder.AddMenuEntry(FSequencerCommands::Get().SetAutoKeyModeNone);

	return MenuBuilder.MakeWidget();

}


TSharedRef<SWidget> SSequencer::MakeTransportControls()
{
	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::Get().LoadModuleChecked<FEditorWidgetsModule>( "EditorWidgets" );
	TSharedRef<FSequencer> Sequencer = SequencerPtr.Pin().ToSharedRef();
	FTransportControlArgs TransportControlArgs;
	{
		TransportControlArgs.OnBackwardEnd.BindSP( Sequencer, &FSequencer::OnStepToBeginning );
		TransportControlArgs.OnBackwardStep.BindSP( Sequencer, &FSequencer::OnStepBackward );
		TransportControlArgs.OnForwardPlay.BindSP( Sequencer, &FSequencer::OnPlay, true );
		TransportControlArgs.OnForwardStep.BindSP( Sequencer, &FSequencer::OnStepForward );
		TransportControlArgs.OnForwardEnd.BindSP( Sequencer, &FSequencer::OnStepToEnd );
		TransportControlArgs.OnToggleLooping.BindSP( Sequencer, &FSequencer::OnToggleLooping );
		TransportControlArgs.OnGetLooping.BindSP( Sequencer, &FSequencer::IsLooping );
		TransportControlArgs.OnGetPlaybackMode.BindSP( Sequencer, &FSequencer::GetPlaybackMode );
	}

	return EditorWidgetsModule.CreateTransportControl( TransportControlArgs );
}


SSequencer::~SSequencer()
{
	USelection::SelectionChangedEvent.RemoveAll(this);
	Settings->GetOnShowCurveEditorChanged().RemoveAll(this);
}


void SSequencer::RegisterActiveTimerForPlayback()
{
	if (!bIsActiveTimerRegistered)
	{
		bIsActiveTimerRegistered = true;
		RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SSequencer::EnsureSlateTickDuringPlayback));
	}
}


EActiveTimerReturnType SSequencer::EnsureSlateTickDuringPlayback(double InCurrentTime, float InDeltaTime)
{
	if (SequencerPtr.IsValid())
	{
		auto PlaybackStatus = SequencerPtr.Pin()->GetPlaybackStatus();
		if (PlaybackStatus == EMovieScenePlayerStatus::Playing || PlaybackStatus == EMovieScenePlayerStatus::Recording || PlaybackStatus == EMovieScenePlayerStatus::Scrubbing)
		{
			return EActiveTimerReturnType::Continue;
		}
	}

	bIsActiveTimerRegistered = false;
	return EActiveTimerReturnType::Stop;
}


void RestoreSelectionState(const TArray<TSharedRef<FSequencerDisplayNode>>& DisplayNodes, TSet<FString>& SelectedPathNames, FSequencerSelection& SequencerSelection)
{
	for (TSharedRef<FSequencerDisplayNode> DisplayNode : DisplayNodes)
	{
		if (SelectedPathNames.Contains(DisplayNode->GetPathName()))
		{
			SequencerSelection.AddToSelection(DisplayNode);
		}
		for (TSharedRef<FSequencerDisplayNode> ChildDisplayNode : DisplayNode->GetChildNodes())
		{
			RestoreSelectionState(DisplayNode->GetChildNodes(), SelectedPathNames, SequencerSelection);
		}
	}
}


void SSequencer::UpdateLayoutTree()
{
	// Cache the selected path names so selection can be restored after the update.
	TSet<FString> SelectedPathNames;
	for (TSharedRef<const FSequencerDisplayNode> SelectedDisplayNode : SequencerPtr.Pin()->GetSelection().GetSelectedOutlinerNodes().Array())
	{
		SelectedPathNames.Add(SelectedDisplayNode->GetPathName());
	}

	// Suspend broadcasting selection changes because we don't want unnecessary rebuilds.
	SequencerPtr.Pin()->GetSelection().SuspendBroadcast();

	// Update the node tree
	SequencerNodeTree->Update();

	TreeView->Refresh();

	// Restore the selection state.
	RestoreSelectionState(SequencerNodeTree->GetRootNodes(), SelectedPathNames, SequencerPtr.Pin()->GetSelection());	// Update to actor selection.
	OnActorSelectionChanged(nullptr);

	// This must come after the selection state has been restored so that the curve editor is populated with the correctly selected nodes
	CurveEditor->SetSequencerNodeTree(SequencerNodeTree);

	// Continue broadcasting selection changes
	SequencerPtr.Pin()->GetSelection().ResumeBroadcast();
}


void SSequencer::UpdateBreadcrumbs()
{
	TSharedRef<FMovieSceneSequenceInstance> FocusedMovieSceneInstance = SequencerPtr.Pin()->GetFocusedMovieSceneSequenceInstance();

	if (BreadcrumbTrail->PeekCrumb().BreadcrumbType == FSequencerBreadcrumb::ShotType)
	{
		BreadcrumbTrail->PopCrumb();
	}

	if( BreadcrumbTrail->PeekCrumb().BreadcrumbType == FSequencerBreadcrumb::MovieSceneType && BreadcrumbTrail->PeekCrumb().MovieSceneInstance.Pin() != FocusedMovieSceneInstance )
	{
		FText CrumbName = FocusedMovieSceneInstance->GetSequence()->GetDisplayName();
		// The current breadcrumb is not a moviescene so we need to make a new breadcrumb in order return to the parent moviescene later
		BreadcrumbTrail->PushCrumb( CrumbName, FSequencerBreadcrumb( FocusedMovieSceneInstance ) );
	}
}


void SSequencer::ResetBreadcrumbs()
{
	BreadcrumbTrail->ClearCrumbs();
	BreadcrumbTrail->PushCrumb(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SSequencer::GetRootAnimationName)), FSequencerBreadcrumb(SequencerPtr.Pin()->GetRootMovieSceneSequenceInstance()));
}


void SSequencer::OnOutlinerSearchChanged( const FText& Filter )
{
	const FString FilterString = Filter.ToString();

	SequencerNodeTree->FilterNodes(FilterString);
	TreeView->Refresh();

	if (FilterString.StartsWith(TEXT("label:")))
	{
		LabelBrowser->SetSelectedLabel(FilterString.RightChop(6));
	}
	else
	{
		LabelBrowser->SetSelectedLabel(FString());
	}
}


float SSequencer::OnGetTimeSnapInterval() const
{
	return Settings->GetTimeSnapInterval();
}


void SSequencer::OnTimeSnapIntervalChanged( float InInterval )
{
	Settings->SetTimeSnapInterval(InInterval);
}


void SSequencer::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	// @todo sequencer: Add drop validity cue
}


void SSequencer::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	// @todo sequencer: Clear drop validity cue
}


FReply SSequencer::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	bool bIsDragSupported = false;

	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
	if (Operation.IsValid() && (
		Operation->IsOfType<FAssetDragDropOp>() ||
		Operation->IsOfType<FClassDragDropOp>() ||
		Operation->IsOfType<FUnloadedClassDragDropOp>() ||
		Operation->IsOfType<FActorDragDropGraphEdOp>() ) )
	{
		bIsDragSupported = true;
	}

	return bIsDragSupported ? FReply::Handled() : FReply::Unhandled();
}


FReply SSequencer::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	bool bWasDropHandled = false;

	// @todo sequencer: Get rid of hard-code assumptions about dealing with ACTORS at this level?

	// @todo sequencer: We may not want any actor-specific code here actually.  We need systems to be able to
	// register with sequencer to support dropping assets/classes/actors, or OTHER types!

	// @todo sequencer: Handle drag and drop from other FDragDropOperations, including unloaded classes/asset and external drags!

	// @todo sequencer: Consider allowing drops into the level viewport to add to the MovieScene as well.
	//		- Basically, when Sequencer is open it would take over drops into the level and auto-add puppets for these instead of regular actors
	//		- This would let people drag smoothly and precisely into the view to drop assets/classes into the scene

	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();

	if (Operation.IsValid() )
	{
		if ( Operation->IsOfType<FAssetDragDropOp>() )
		{
			const auto& DragDropOp = StaticCastSharedPtr<FAssetDragDropOp>( Operation );

			OnAssetsDropped( *DragDropOp );
			bWasDropHandled = true;
		}
		else if( Operation->IsOfType<FClassDragDropOp>() )
		{
			const auto& DragDropOp = StaticCastSharedPtr<FClassDragDropOp>( Operation );

			OnClassesDropped( *DragDropOp );
			bWasDropHandled = true;
		}
		else if( Operation->IsOfType<FUnloadedClassDragDropOp>() )
		{
			const auto& DragDropOp = StaticCastSharedPtr<FUnloadedClassDragDropOp>( Operation );

			OnUnloadedClassesDropped( *DragDropOp );
			bWasDropHandled = true;
		}
		else if( Operation->IsOfType<FActorDragDropGraphEdOp>() )
		{
			const auto& DragDropOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>( Operation );

			OnActorsDropped( *DragDropOp );
			bWasDropHandled = true;
		}
	}

	return bWasDropHandled ? FReply::Handled() : FReply::Unhandled();
}


FReply SSequencer::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) 
{
	// A toolkit tab is active, so direct all command processing to it
	if( SequencerPtr.Pin()->GetCommandBindings()->ProcessCommandBindings( InKeyEvent ) )
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}


void SSequencer::OnAssetsDropped( const FAssetDragDropOp& DragDropOp )
{
	FSequencer& SequencerRef = *SequencerPtr.Pin();

	bool bObjectAdded = false;
	TArray< UObject* > DroppedObjects;
	bool bAllAssetsWereLoaded = true;

	for( auto CurAssetData = DragDropOp.AssetData.CreateConstIterator(); CurAssetData; ++CurAssetData )
	{
		const FAssetData& AssetData = *CurAssetData;

		UObject* Object = AssetData.GetAsset();

		if ( Object != nullptr )
		{
			DroppedObjects.Add( Object );
		}
		else
		{
			bAllAssetsWereLoaded = false;
		}
	}

	const TSet< TSharedRef<FSequencerDisplayNode> >& SelectedNodes = SequencerPtr.Pin()->GetSelection().GetSelectedOutlinerNodes();
	FGuid TargetObjectGuid;
	// if exactly one object node is selected, we have a target object guid
	TSharedPtr<const FSequencerDisplayNode> DisplayNode;
	if (SelectedNodes.Num() == 1)
	{
		for (TSharedRef<const FSequencerDisplayNode> SelectedNode : SelectedNodes )
		{
			DisplayNode = SelectedNode;
		}
		if (DisplayNode.IsValid() && DisplayNode->GetType() == ESequencerNode::Object)
		{
			TSharedPtr<const FSequencerObjectBindingNode> ObjectBindingNode = StaticCastSharedPtr<const FSequencerObjectBindingNode>(DisplayNode);
			TargetObjectGuid = ObjectBindingNode->GetObjectBinding();
		}
	}

	for( auto CurObjectIter = DroppedObjects.CreateConstIterator(); CurObjectIter; ++CurObjectIter )
	{
		UObject* CurObject = *CurObjectIter;

		if (!SequencerRef.OnHandleAssetDropped(CurObject, TargetObjectGuid))
		{
			SequencerRef.MakeNewSpawnable( *CurObject );
		}
		bObjectAdded = true;
	}

	if( bObjectAdded )
	{
		// Update the sequencers view of the movie scene data when any object is added
		SequencerRef.NotifyMovieSceneDataChanged();
	}
}


void SSequencer::OnClassesDropped( const FClassDragDropOp& DragDropOp )
{
	FSequencer& SequencerRef = *SequencerPtr.Pin();

	for( auto ClassIter = DragDropOp.ClassesToDrop.CreateConstIterator(); ClassIter; ++ClassIter )
	{
		UClass* Class = ( *ClassIter ).Get();
		if( Class != nullptr )
		{
			UObject* Object = Class->GetDefaultObject();

			FGuid NewGuid = SequencerRef.MakeNewSpawnable( *Object );

			if (NewGuid.IsValid())
			{
				// Update the sequencers view of the movie scene data
				SequencerRef.NotifyMovieSceneDataChanged();
			}
		}
	}
}


void SSequencer::OnUnloadedClassesDropped( const FUnloadedClassDragDropOp& DragDropOp )
{
	FSequencer& SequencerRef = *SequencerPtr.Pin();
	for( auto ClassDataIter = DragDropOp.AssetsToDrop->CreateConstIterator(); ClassDataIter; ++ClassDataIter )
	{
		auto& ClassData = *ClassDataIter;

		// Check to see if the asset can be found, otherwise load it.
		UObject* Object = FindObject<UObject>( nullptr, *ClassData.AssetName );
		if( Object == nullptr )
		{
			Object = FindObject<UObject>(nullptr, *FString::Printf(TEXT("%s.%s"), *ClassData.GeneratedPackageName, *ClassData.AssetName));
		}

		if( Object == nullptr )
		{
			// Load the package.
			GWarn->BeginSlowTask( LOCTEXT("OnDrop_FullyLoadPackage", "Fully Loading Package For Drop"), true, false );
			UPackage* Package = LoadPackage(nullptr, *ClassData.GeneratedPackageName, LOAD_NoRedirects );
			if( Package != nullptr )
			{
				Package->FullyLoad();
			}
			GWarn->EndSlowTask();

			Object = FindObject<UObject>(Package, *ClassData.AssetName);
		}

		if( Object != nullptr )
		{
			// Check to see if the dropped asset was a blueprint
			if(Object->IsA(UBlueprint::StaticClass()))
			{
				// Get the default object from the generated class.
				Object = Cast<UBlueprint>(Object)->GeneratedClass->GetDefaultObject();
			}
		}

		if( Object != nullptr )
		{
			FGuid NewGuid = SequencerRef.MakeNewSpawnable( *Object );
			if (NewGuid.IsValid())
			{
				SequencerRef.NotifyMovieSceneDataChanged();
			}
		}
	}
}


void SSequencer::OnActorsDropped( FActorDragDropGraphEdOp& DragDropOp )
{
	SequencerPtr.Pin()->OnActorsDropped( DragDropOp.Actors );
}


void SSequencer::OnActorSelectionChanged(UObject*)
{
	if (!SequencerPtr.IsValid())
	{
		return;
	}

	if (!SequencerPtr.Pin()->IsLevelEditorSequencer())
	{
		return;
	}

	// If the user is selecting within the sequencer, ignore
	if (bUserIsSelecting)
	{
		return;
	}

	TSet<TSharedRef<FSequencerDisplayNode>> RootNodes(SequencerNodeTree->GetRootNodes());

	// Select the nodes that have runtime objects that are selected in the level
	for (auto Node : RootNodes)
	{
		TSharedRef<FSequencerObjectBindingNode> ObjectBindingNode = StaticCastSharedRef<FSequencerObjectBindingNode>(Node);
		TArray<UObject*> RuntimeObjects;
		SequencerPtr.Pin()->GetRuntimeObjects( SequencerPtr.Pin()->GetFocusedMovieSceneSequenceInstance(), ObjectBindingNode->GetObjectBinding(), RuntimeObjects );
		
		bool bDoSelect = false;
		for (int32 RuntimeIndex = 0; RuntimeIndex < RuntimeObjects.Num(); ++RuntimeIndex )
		{
			if (GEditor->GetSelectedActors()->IsSelected(RuntimeObjects[RuntimeIndex]))
			{
				bDoSelect = true;
				break;
			}
		}

		if (SequencerPtr.Pin()->GetSelection().IsSelected(Node) != bDoSelect)
		{
			if (bDoSelect)
			{
				SequencerPtr.Pin()->GetSelection().AddToSelection(Node);
			}
			else
			{
				SequencerPtr.Pin()->GetSelection().RemoveFromSelection(Node);
			}
		}
	}

	const TSet<TSharedRef<FSequencerDisplayNode>>& OutlinerSelection = SequencerPtr.Pin()->GetSelection().GetSelectedOutlinerNodes();

	if (OutlinerSelection.Num() == 1)
	{
		for (auto& Node : OutlinerSelection)
		{
			TreeView->RequestScrollIntoView(Node);
			break;
		}
	}
}


void SSequencer::OnCrumbClicked(const FSequencerBreadcrumb& Item)
{
	if (Item.BreadcrumbType != FSequencerBreadcrumb::ShotType)
	{
		if( SequencerPtr.Pin()->GetFocusedMovieSceneSequenceInstance() == Item.MovieSceneInstance.Pin() ) 
		{
			// then do zooming
		}
		else
		{
			SequencerPtr.Pin()->PopToSequenceInstance( Item.MovieSceneInstance.Pin().ToSharedRef() );
		}
	}
}


FText SSequencer::GetRootAnimationName() const
{
	return SequencerPtr.Pin()->GetRootMovieSceneSequence()->GetDisplayName();
}


TSharedPtr<SSequencerTreeView> SSequencer::GetTreeView() const
{
	return TreeView;
}


TArray<FSectionHandle> SSequencer::GetSectionHandles(const TSet<TWeakObjectPtr<UMovieSceneSection>>& DesiredSections) const
{
	TArray<FSectionHandle> SectionHandles;

	// @todo sequencer: this is potentially slow as it traverses the entire tree - there's scope for optimization here
	for (auto& Node : SequencerNodeTree->GetRootNodes())
	{
		Node->Traverse_ParentFirst([&](FSequencerDisplayNode& InNode) {
			if (InNode.GetType() == ESequencerNode::Track)
			{
				FSequencerTrackNode& TrackNode = static_cast<FSequencerTrackNode&>(InNode);

				const auto& AllSections = TrackNode.GetTrack()->GetAllSections();
				for (int32 Index = 0; Index < AllSections.Num(); ++Index)
				{
					if (DesiredSections.Contains(TWeakObjectPtr<UMovieSceneSection>(AllSections[Index])))
					{
						SectionHandles.Emplace(StaticCastSharedRef<FSequencerTrackNode>(TrackNode.AsShared()), Index);
					}
				}
			}
			return true;
		});
	}

	return SectionHandles;
}


void SSequencer::OnSaveMovieSceneClicked()
{
	SequencerPtr.Pin()->SaveCurrentMovieScene();
}


void SSequencer::StepToNextKey()
{
	StepToKey(true, false);
}


void SSequencer::StepToPreviousKey()
{
	StepToKey(false, false);
}


void SSequencer::StepToNextCameraKey()
{
	StepToKey(true, true);
}


void SSequencer::StepToPreviousCameraKey()
{
	StepToKey(false, true);
}


void SSequencer::StepToKey(bool bStepToNextKey, bool bCameraOnly)
{
	TSet< TSharedRef<FSequencerDisplayNode> > Nodes;

	if (bCameraOnly)
	{
		TSet<TSharedRef<FSequencerDisplayNode>> RootNodes(SequencerNodeTree->GetRootNodes());

		TSet<TWeakObjectPtr<AActor> > LockedActors;
		for (int32 i = 0; i < GEditor->LevelViewportClients.Num(); ++i)
		{		
			FLevelEditorViewportClient* LevelVC = GEditor->LevelViewportClients[i];
			if (LevelVC && LevelVC->IsPerspective() && LevelVC->GetViewMode() != VMI_Unknown)
			{
				TWeakObjectPtr<AActor> ActorLock = LevelVC->GetActiveActorLock();
				if (ActorLock.IsValid())
				{
					LockedActors.Add(ActorLock);
				}
			}
		}

		for (auto RootNode : RootNodes)
		{
			TSharedRef<FSequencerObjectBindingNode> ObjectBindingNode = StaticCastSharedRef<FSequencerObjectBindingNode>(RootNode);
			TArray<UObject*> RuntimeObjects;
			SequencerPtr.Pin()->GetRuntimeObjects( SequencerPtr.Pin()->GetFocusedMovieSceneSequenceInstance(), ObjectBindingNode->GetObjectBinding(), RuntimeObjects );
		
			for (int32 RuntimeIndex = 0; RuntimeIndex < RuntimeObjects.Num(); ++RuntimeIndex )
			{
				AActor* RuntimeActor = Cast<AActor>(RuntimeObjects[RuntimeIndex]);
				if (RuntimeActor != nullptr && LockedActors.Contains(RuntimeActor))
				{
					Nodes.Add(RootNode);
				}
			}
		}
	}
	else
	{
		const TSet< TSharedRef<FSequencerDisplayNode> >& SelectedNodes = SequencerPtr.Pin()->GetSelection().GetSelectedOutlinerNodes();
		Nodes = SelectedNodes;

		if (Nodes.Num() == 0)
		{
			TSet<TSharedRef<FSequencerDisplayNode>> RootNodes(SequencerNodeTree->GetRootNodes());
			for (auto RootNode : RootNodes)
			{
				Nodes.Add(RootNode);

				SequencerHelpers::GetDescendantNodes(RootNode, Nodes);
			}
		}
	}
		
	if (Nodes.Num() > 0)
	{
		float ClosestKeyDistance = MAX_FLT;
		float CurrentTime = SequencerPtr.Pin()->GetCurrentLocalTime(*SequencerPtr.Pin()->GetFocusedMovieSceneSequence());
		float StepToTime = 0;
		bool StepToKeyFound = false;

		auto It = Nodes.CreateConstIterator();
		bool bExpand = !(*It).Get().IsExpanded();

		for (auto Node : Nodes)
		{
			TSet<TSharedPtr<IKeyArea>> KeyAreas;
			SequencerHelpers::GetAllKeyAreas(Node, KeyAreas);

			for (TSharedPtr<IKeyArea> KeyArea : KeyAreas)
			{
				for (FKeyHandle& KeyHandle : KeyArea->GetUnsortedKeyHandles())
				{
					float KeyTime = KeyArea->GetKeyTime(KeyHandle);
					if (bStepToNextKey)
					{
						if (KeyTime > CurrentTime && KeyTime - CurrentTime < ClosestKeyDistance)
						{
							StepToTime = KeyTime;
							ClosestKeyDistance = KeyTime - CurrentTime;
							StepToKeyFound = true;
						}
					}
					else
					{
						if (KeyTime < CurrentTime && CurrentTime - KeyTime < ClosestKeyDistance)
						{
							StepToTime = KeyTime;
							ClosestKeyDistance = CurrentTime - KeyTime;
							StepToKeyFound = true;
						}
					}
				}
			}
		}

		if (StepToKeyFound)
		{
			SequencerPtr.Pin()->SetGlobalTime(StepToTime);
		}
	}
}


EVisibility SSequencer::GetBreadcrumbTrailVisibility() const
{
	return SequencerPtr.Pin()->IsLevelEditorSequencer() ? EVisibility::Visible : EVisibility::Collapsed;
}


EVisibility SSequencer::GetCurveEditorToolBarVisibility() const
{
	return Settings->GetShowCurveEditor() ? EVisibility::Visible : EVisibility::Collapsed;
}


EVisibility SSequencer::GetBottomTimeSliderVisibility() const
{
	return Settings->GetShowRangeSlider() ? EVisibility::Hidden : EVisibility::Visible;
}


EVisibility SSequencer::GetTimeRangeVisibility() const
{
	return Settings->GetShowRangeSlider() ? EVisibility::Visible : EVisibility::Hidden;
}


bool SSequencer::ShowFrameNumbers() const
{
	return SequencerPtr.Pin()->CanShowFrameNumbers() && Settings->GetShowFrameNumbers();
}


float SSequencer::GetOutlinerSpacerFill() const
{
	const float Column1Coeff = GetColumnFillCoefficient(1);
	return Settings->GetShowCurveEditor() ? Column1Coeff / (1 - Column1Coeff) : 0.f;
}


void SSequencer::OnColumnFillCoefficientChanged(float FillCoefficient, int32 ColumnIndex)
{
	ColumnFillCoefficients[ColumnIndex] = FillCoefficient;
}


EVisibility SSequencer::GetTrackAreaVisibility() const
{
	return Settings->GetShowCurveEditor() ? EVisibility::Collapsed : EVisibility::Visible;
}


EVisibility SSequencer::GetCurveEditorVisibility() const
{
	return Settings->GetShowCurveEditor() ? EVisibility::Visible : EVisibility::Collapsed;
}


void SSequencer::OnCurveEditorVisibilityChanged()
{
	if (CurveEditor.IsValid())
	{
		// Only zoom horizontally if the editor is visible
		CurveEditor->SetAllowAutoFrame(Settings->GetShowCurveEditor());
	}
}

FPaintPlaybackRangeArgs SSequencer::GetSectionPlaybackRangeArgs() const
{
	if (GetBottomTimeSliderVisibility() == EVisibility::Visible)
	{
		static FPaintPlaybackRangeArgs Args(FEditorStyle::GetBrush("Sequencer.Timeline.PlayRange_L"), FEditorStyle::GetBrush("Sequencer.Timeline.PlayRange_R"), 6.f);
		return Args;
	}
	else
	{
		static FPaintPlaybackRangeArgs Args(FEditorStyle::GetBrush("Sequencer.Timeline.PlayRange_Bottom_L"), FEditorStyle::GetBrush("Sequencer.Timeline.PlayRange_Bottom_R"), 6.f);
		return Args;
	}
}


FVirtualTrackArea SSequencer::GetVirtualTrackArea() const
{
	return FVirtualTrackArea(*SequencerPtr.Pin(), *TreeView.Get(), TrackArea->GetCachedGeometry());
}

FPasteContextMenuArgs SSequencer::GeneratePasteArgs(float PasteAtTime, TSharedPtr<FMovieSceneClipboard> Clipboard)
{
	TSharedPtr<FSequencer> Sequencer = SequencerPtr.Pin();
	if (Settings->GetIsSnapEnabled())
	{
		PasteAtTime = Settings->SnapTimeToInterval(PasteAtTime);
	}

	// Open a paste menu at the current mouse position
	FSlateApplication& Application = FSlateApplication::Get();
	FVector2D LocalMousePosition = TrackArea->GetCachedGeometry().AbsoluteToLocal(Application.GetCursorPos());

	FVirtualTrackArea VirtualTrackArea = GetVirtualTrackArea();

	// Paste into the currently selected sections, or hit test the mouse position as a last resort
	TArray<TSharedRef<FSequencerDisplayNode>> PasteIntoNodes;
	{
		TSet<TWeakObjectPtr<UMovieSceneSection>> Sections = Sequencer->GetSelection().GetSelectedSections();
		for (const FSequencerSelectedKey& Key : Sequencer->GetSelection().GetSelectedKeys())
		{
			Sections.Add(Key.Section);
		}

		for (const FSectionHandle& Handle : GetSectionHandles(Sections))
		{
			PasteIntoNodes.Add(Handle.TrackNode.ToSharedRef());
		}
	}

	if (PasteIntoNodes.Num() == 0)
	{
		TSharedPtr<FSequencerDisplayNode> Node = VirtualTrackArea.HitTestNode(LocalMousePosition.Y);
		if (Node.IsValid())
		{
			PasteIntoNodes.Add(Node.ToSharedRef());
		}
	}

	return FPasteContextMenuArgs::PasteInto(MoveTemp(PasteIntoNodes), PasteAtTime, Clipboard);
}

void SSequencer::Paste()
{
	TSharedPtr<FPasteContextMenu> ContextMenu;

	TSharedPtr<FSequencer> Sequencer = SequencerPtr.Pin();
	if (Sequencer->GetClipboardStack().Num() != 0)
	{
		FPasteContextMenuArgs Args = GeneratePasteArgs(Sequencer->GetGlobalTime(), Sequencer->GetClipboardStack().Last());
		ContextMenu = FPasteContextMenu::CreateMenu(*Sequencer, Args);
	}

	if (!ContextMenu.IsValid() || !ContextMenu->IsValidPaste())
	{
		return;
	}
	else if (ContextMenu->AutoPaste())
	{
		return;
	}

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, SequencerPtr.Pin()->GetCommandBindings());

	ContextMenu->PopulateMenu(MenuBuilder);

	FWidgetPath Path;
	FSlateApplication::Get().FindPathToWidget(AsShared(), Path);
	
	FSlateApplication::Get().PushMenu(
		AsShared(),
		Path,
		MenuBuilder.MakeWidget(),
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
		);
}

void SSequencer::PasteFromHistory()
{
	TSharedPtr<FSequencer> Sequencer = SequencerPtr.Pin();
	if (Sequencer->GetClipboardStack().Num() == 0)
	{
		return;
	}

	FPasteContextMenuArgs Args = GeneratePasteArgs(Sequencer->GetGlobalTime());
	TSharedPtr<FPasteFromHistoryContextMenu> ContextMenu = FPasteFromHistoryContextMenu::CreateMenu(*Sequencer, Args);

	if (ContextMenu.IsValid())
	{
		const bool bShouldCloseWindowAfterMenuSelection = true;
		FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, Sequencer->GetCommandBindings());

		ContextMenu->PopulateMenu(MenuBuilder);

		FWidgetPath Path;
		FSlateApplication::Get().FindPathToWidget(AsShared(), Path);
		
		FSlateApplication::Get().PushMenu(
			AsShared(),
			Path,
			MenuBuilder.MakeWidget(),
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
			);
	}
}

#undef LOCTEXT_NAMESPACE

