// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

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
#include "K2Node_PlayMovieScene.h"
#include "MovieSceneShotSection.h"
#include "CommonMovieSceneTools.h"
#include "SSearchBox.h"
#include "SNumericDropDown.h"
#include "EditorWidgetsModule.h"
#include "SequencerTrackLaneFactory.h"
#include "SSequencerSplitterOverlay.h"
#include "IKeyArea.h"

#define LOCTEXT_NAMESPACE "Sequencer"


class SSequencerScrollBox : public SScrollBox
{
public:
	void Construct( const FArguments& InArgs, TSharedRef<FSequencer> InSequencer);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/** The sequencer which owns this widget. */
	TWeakPtr<FSequencer> Sequencer;
};

void SSequencerScrollBox::Construct(const FArguments& InArgs, TSharedRef<FSequencer> InSequencer)
{
	Sequencer = InSequencer;
	SScrollBox::Construct(InArgs);
}

FReply SSequencerScrollBox::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Clear the selection if no sequencer display nodes were clicked on
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (Sequencer.Pin()->GetSelection().GetSelectedOutlinerNodes().Num() != 0)
		{
			Sequencer.Pin()->GetSelection().EmptySelectedOutlinerNodes();

			if (Sequencer.Pin()->IsLevelEditorSequencer())
			{
				const bool bNotifySelectionChanged = false;
				const bool bDeselectBSP = true;
				const bool bWarnAboutTooManyActors = false;

				const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "ClickingOnActors", "Clicking on Actors"));
				GEditor->SelectNone(bNotifySelectionChanged, bDeselectBSP, bWarnAboutTooManyActors);
				return FReply::Handled();
			}
		}
	}
	return FReply::Unhandled();
}

/**
 * The shot filter overlay displays the overlay needed to filter out widgets based on
 * which shots are actively in use.
 */
class SShotFilterOverlay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SShotFilterOverlay) {}
		SLATE_ATTRIBUTE( TRange<float>, ViewRange )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FSequencer> InSequencer)
	{
		ViewRange = InArgs._ViewRange;
		Sequencer = InSequencer;
	}

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override
	{
		if (Sequencer.Pin()->IsShotFilteringOn())
		{
			TArray< TWeakObjectPtr<UMovieSceneSection> > FilterShots = Sequencer.Pin()->GetFilteringShotSections();
			// if there are filtering shots, continuously update the cached filter zones
			// we do this in tick, and cache it in order to make animation work properly
			CachedFilteredRanges.Empty();
			for (int32 i = 0; i < FilterShots.Num(); ++i)
			{
				UMovieSceneSection* Filter = FilterShots[i].Get();
				CachedFilteredRanges.Add(Filter->GetRange());
			}
		}
	}
	
	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override
	{
		float Alpha = Sequencer.Pin()->GetOverlayFadeCurve();
		
		if (Alpha > 0.f)
		{
			FTimeToPixel TimeToPixelConverter = FTimeToPixel(AllottedGeometry, ViewRange.Get());
		
			TRange<float> TimeBounds = TRange<float>(TimeToPixelConverter.PixelToTime(0),
				TimeToPixelConverter.PixelToTime(AllottedGeometry.Size.X));

			TArray< TRange<float> > OverlayRanges = ComputeOverlayRanges(TimeBounds, CachedFilteredRanges);

			for (int32 i = 0; i < OverlayRanges.Num(); ++i)
			{
				float LowerBound = TimeToPixelConverter.TimeToPixel(OverlayRanges[i].GetLowerBoundValue());
				float UpperBound = TimeToPixelConverter.TimeToPixel(OverlayRanges[i].GetUpperBoundValue());
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId,
					AllottedGeometry.ToPaintGeometry(FVector2D(LowerBound, 0), FVector2D(UpperBound - LowerBound, AllottedGeometry.Size.Y)),
					FEditorStyle::GetBrush("Sequencer.ShotFilter"),
					MyClippingRect,
					ESlateDrawEffect::None,
					FLinearColor(1.f, 1.f, 1.f, Alpha)
				);
			}
		}

		return LayerId;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(100, 100);
	}

private:
	/**
	 * Given a range of time bounds, find what ranges still should be
	 * filtered based on shot filters
	 */
	TArray< TRange<float> > ComputeOverlayRanges(TRange<float> TimeBounds, TArray< TRange<float> > RangesToSubtract) const
	{
		TArray< TRange<float> > FilteredRanges;
		FilteredRanges.Add(TimeBounds);
		// @todo Sequencer Optimize - This is O(n^2)
		// However, n is likely to stay very low, and the average case is likely O(n)
		// Also, the swapping of TArrays in this loop could use some heavy optimization as well
		for (int32 i = 0; i < RangesToSubtract.Num(); ++i)
		{
			TRange<float>& CurrentRange = RangesToSubtract[i];

			// ignore ranges that don't overlap with the time bounds
			if (CurrentRange.Overlaps(TimeBounds))
			{
				TArray< TRange<float> > NewFilteredRanges;
				for (int32 j = 0; j < FilteredRanges.Num(); ++j)
				{
					TArray< TRange<float> > SubtractedRanges = TRange<float>::Difference(FilteredRanges[j], CurrentRange);
					NewFilteredRanges.Append(SubtractedRanges);
				}
				FilteredRanges = NewFilteredRanges;
			}
		}
		return FilteredRanges;
	}

private:
	/** The current minimum view range */
	TAttribute< TRange<float> > ViewRange;
	/** The main sequencer interface */
	TWeakPtr<FSequencer> Sequencer;
	/** Cached set of ranges that are being filtering currently */
	TArray< TRange<float> > CachedFilteredRanges;
};

void SSequencer::Construct( const FArguments& InArgs, TSharedRef< class FSequencer > InSequencer )
{
	Sequencer = InSequencer;
	bIsActiveTimerRegistered = false;
	bUserIsSelecting = false;

	USelection::SelectionChangedEvent.AddSP(this, &SSequencer::OnActorSelectionChanged);
	Settings = InSequencer->GetSettings();

	Settings->GetOnShowCurveEditorChanged().AddSP(this, &SSequencer::OnCurveEditorVisibilityChanged);

	// Create a node tree which contains a tree of movie scene data to display in the sequence
	SequencerNodeTree = MakeShareable( new FSequencerNodeTree( InSequencer.Get() ) );

	FSequencerWidgetsModule& SequencerWidgets = FModuleManager::Get().LoadModuleChecked<FSequencerWidgetsModule>( "SequencerWidgets" );

	FTimeSliderArgs TimeSliderArgs;
	TimeSliderArgs.ViewRange = InArgs._ViewRange;
	TimeSliderArgs.OnViewRangeChanged = InArgs._OnViewRangeChanged;
	TimeSliderArgs.ScrubPosition = InArgs._ScrubPosition;
	TimeSliderArgs.OnBeginScrubberMovement = InArgs._OnBeginScrubbing;
	TimeSliderArgs.OnEndScrubberMovement = InArgs._OnEndScrubbing;
	TimeSliderArgs.OnScrubPositionChanged = InArgs._OnScrubPositionChanged;
	TimeSliderArgs.Settings = Settings;

	TSharedRef<FSequencerTimeSliderController> TimeSliderController( new FSequencerTimeSliderController( TimeSliderArgs ) );
	
	bool bMirrorLabels = false;
	// Create the top and bottom sliders
	TSharedRef<ITimeSlider> TopTimeSlider = SequencerWidgets.CreateTimeSlider( TimeSliderController, bMirrorLabels );

	bMirrorLabels = true;
	TSharedRef<ITimeSlider> BottomTimeSlider = SequencerWidgets.CreateTimeSlider( TimeSliderController, bMirrorLabels);

	OnGetAddMenuContent = InArgs._OnGetAddMenuContent;

	ColumnFillCoefficients[0] = .25f;
	ColumnFillCoefficients[1] = .75f;

	TAttribute<float> FillCoefficient_0, FillCoefficient_1;
	FillCoefficient_0.Bind(TAttribute<float>::FGetter::CreateSP(this, &SSequencer::GetColumnFillCoefficient, 0));
	FillCoefficient_1.Bind(TAttribute<float>::FGetter::CreateSP(this, &SSequencer::GetColumnFillCoefficient, 1));

	TSharedRef<SScrollBar> ScrollBar =
		SNew(SScrollBar)
		.Thickness(FVector2D(5.0f, 5.0f));

	auto PinnedSequencer = Sequencer.Pin().ToSharedRef();

	SAssignNew( TrackOutliner, SSequencerTrackOutliner );

	SAssignNew( TrackArea, SSequencerTrackArea, TimeSliderController )
		.Visibility( this, &SSequencer::GetTrackAreaVisibility );

	SAssignNew( CurveEditor, SSequencerCurveEditor, PinnedSequencer, TimeSliderController )
		.Visibility( this, &SSequencer::GetCurveEditorVisibility )
		.OnViewRangeChanged( InArgs._OnViewRangeChanged )
		.ViewRange( InArgs._ViewRange );
	CurveEditor->SetAllowAutoFrame(Settings->GetShowCurveEditor());

	const int32 				Column0	= 0,	Column1	= 1;
	const int32 Row0	= 0,
				Row1	= 1,
				Row2	= 2;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(2)
		.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
		[
			SNew( SVerticalBox )
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew( SHorizontalBox )
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					MakeToolBar()
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew( SSequencerCurveEditorToolBar, CurveEditor->GetCommands() )
					.Visibility( this, &SSequencer::GetCurveEditorToolBarVisibility )
				]
			]

			+ SVerticalBox::Slot()
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				[
					SNew( SGridPanel )
					.FillRow( 1, 1.f )
					.FillColumn( 0, FillCoefficient_0 )
					.FillColumn( 1, FillCoefficient_1 )

					+ SGridPanel::Slot( Column0, Row0 )
					.VAlign( VAlign_Center )
					[
						// Search box for searching through the outliner
						SNew( SSearchBox )
						.OnTextChanged( this, &SSequencer::OnOutlinerSearchChanged )
					]

					+ SGridPanel::Slot( Column0, Row1 )
					.ColumnSpan(2)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						[
							SNew( SOverlay )

							+ SOverlay::Slot()
							[
								SNew( SSequencerScrollBox, Sequencer.Pin().ToSharedRef() )
								.ExternalScrollbar(ScrollBar)

								+ SScrollBox::Slot()
								[
									SNew( SHorizontalBox )

									+ SHorizontalBox::Slot()
									.FillWidth( FillCoefficient_0 )
									[
										SNew(SBox)
										// Padding to allow space for the scroll bar
										.Padding(FMargin(0,0,10.f,0))
										[
											TrackOutliner.ToSharedRef()
										]
									]

									+ SHorizontalBox::Slot()
									.FillWidth( FillCoefficient_1 )
									[
										TrackArea.ToSharedRef()
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

					+ SGridPanel::Slot( Column0, Row2 )
					.HAlign( HAlign_Center )
					[
						MakeTransportControls()
					]

					// Second column
					+ SGridPanel::Slot( Column1, Row0 )
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
					+ SGridPanel::Slot( Column1, Row1, SGridPanel::Layer(0) )
					[
						SNew( SSequencerSectionOverlay, TimeSliderController )
						.Visibility( EVisibility::HitTestInvisible )
						.DisplayScrubPosition( false )
						.DisplayTickLines( true )
					]

					// Curve editor
					+ SGridPanel::Slot( Column1, Row1 )
					[
						CurveEditor.ToSharedRef()
					]

					// Overlay that draws the scrub position
					+ SGridPanel::Slot( Column1, Row1, SGridPanel::Layer(30) )
					[
						SNew( SSequencerSectionOverlay, TimeSliderController )
						.Visibility( EVisibility::HitTestInvisible )
						.DisplayScrubPosition( true )
						.DisplayTickLines( false )
					]

					+ SGridPanel::Slot( Column1, Row2 )
					[
						SNew( SBorder )
						.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
						.BorderBackgroundColor( FLinearColor(.50f, .50f, .50f, 1.0f ) )
						.Padding(0)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								BottomTimeSlider
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SAssignNew( BreadcrumbTrail, SBreadcrumbTrail<FSequencerBreadcrumb> )
								.Visibility( this, &SSequencer::GetBreadcrumbTrailVisibility )
								.OnCrumbClicked( this, &SSequencer::OnCrumbClicked )
							]
						]
					]
				]

				+ SOverlay::Slot()
				[
					SNew( SSequencerSplitterOverlay )
					.Style(FEditorStyle::Get(), "Sequencer.AnimationOutliner.Splitter")
					.Visibility(EVisibility::SelfHitTestInvisible)

					+ SSplitter::Slot()
					.Value( FillCoefficient_0 )
					.OnSlotResized( SSplitter::FOnSlotResized::CreateSP(this, &SSequencer::OnColumnFillCoefficientChanged, 0) )
					[
						SNew(SSpacer)
					]

					+ SSplitter::Slot()
					.Value( FillCoefficient_1 )
					.OnSlotResized( SSplitter::FOnSlotResized::CreateSP(this, &SSequencer::OnColumnFillCoefficientChanged, 1) )
					[
						SNew(SSpacer)
					]
				]
			]
		]
	];

	ResetBreadcrumbs();
}

TSharedRef<SWidget> SSequencer::MakeToolBar()
{
	FToolBarBuilder ToolBarBuilder( Sequencer.Pin()->GetCommandBindings(), FMultiBoxCustomization::None, TSharedPtr<FExtender>(), Orient_Horizontal, true);
	
	ToolBarBuilder.BeginSection("Base Commands");
	{
		ToolBarBuilder.SetLabelVisibility(EVisibility::Visible);

		if (OnGetAddMenuContent.IsBound())
		{
			ToolBarBuilder.AddWidget(
				SNew(SComboButton)
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
						.Text(LOCTEXT("AddButton", "Track"))
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
				]);
		}

		if( Sequencer.Pin()->IsLevelEditorSequencer() )
		{
			ToolBarBuilder.AddWidget
			(
				SNew( SButton )
				.ButtonStyle(FEditorStyle::Get(), "FlatButton")
				.ToolTipText( LOCTEXT( "SaveDirtyPackagesTooltip", "Saves the current Movie Scene" ) )
				.ContentPadding(FMargin(6, 2))
				.ForegroundColor( FSlateColor::UseForeground() )
				.OnClicked( this, &SSequencer::OnSaveMovieSceneClicked )
				[
					SNew( SHorizontalBox )
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(STextBlock)
						.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
						.Text(FText::FromString(FString(TEXT("\xf0c7"))) /*fa-floppy-o*/)
						.ShadowOffset( FVector2D( 1,1) )
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4, 1, 0, 0)
					[
						SNew( STextBlock )
						.Text( LOCTEXT( "SaveMovieScene", "Save" ) )
						.ShadowOffset( FVector2D( 1,1) )
					]
				]
			);

			ToolBarBuilder.AddSeparator();
		}

		ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().ToggleAutoKeyEnabled );

		if ( Sequencer.Pin()->IsLevelEditorSequencer() )
		{
			ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().ToggleCleanView );
		}

		ToolBarBuilder.SetLabelVisibility(EVisibility::Collapsed);
		ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().ToggleAutoScroll, NAME_None, TAttribute<FText>( FText::GetEmpty() ) );
		ToolBarBuilder.SetLabelVisibility(EVisibility::Visible);
	}
	ToolBarBuilder.EndSection();

	ToolBarBuilder.BeginSection("Snapping");
	{
		TArray<SNumericDropDown<float>::FNamedValue> SnapValues;
		SnapValues.Add( SNumericDropDown<float>::FNamedValue( 0.001f, LOCTEXT( "Snap_OneThousandth", "0.001" ), LOCTEXT( "SnapDescription_OneThousandth", "Set snap to 1/1000th" ) ) );
		SnapValues.Add( SNumericDropDown<float>::FNamedValue( 0.01f, LOCTEXT( "Snap_OneHundredth", "0.01" ), LOCTEXT( "SnapDescription_OneHundredth", "Set snap to 1/100th" ) ) );
		SnapValues.Add( SNumericDropDown<float>::FNamedValue( 0.1f, LOCTEXT( "Snap_OneTenth", "0.1" ), LOCTEXT( "SnapDescription_OneTenth", "Set snap to 1/10th" ) ) );
		SnapValues.Add( SNumericDropDown<float>::FNamedValue( 1.0f, LOCTEXT( "Snap_One", "1" ), LOCTEXT( "SnapDescription_One", "Set snap to 1" ) ) );
		SnapValues.Add( SNumericDropDown<float>::FNamedValue( 10.0f, LOCTEXT( "Snap_Ten", "10" ), LOCTEXT( "SnapDescription_Ten", "Set snap to 10" ) ) );
		SnapValues.Add( SNumericDropDown<float>::FNamedValue( 100.0f, LOCTEXT( "Snap_OneHundred", "100" ), LOCTEXT( "SnapDescription_OneHundred", "Set snap to 100" ) ) );

		ToolBarBuilder.SetLabelVisibility( EVisibility::Collapsed );
		ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().ToggleIsSnapEnabled, NAME_None, TAttribute<FText>( FText::GetEmpty() ) );
		ToolBarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateSP( this, &SSequencer::MakeSnapMenu ),
			LOCTEXT( "SnapOptions", "Options" ),
			LOCTEXT( "SnapOptionsToolTip", "Snapping Options" ),
			TAttribute<FSlateIcon>(),
			true );
		ToolBarBuilder.AddWidget(
			SNew( SBox )
			.VAlign( VAlign_Center )
			[
				SNew( SNumericDropDown<float> )
				.DropDownValues( SnapValues )
				.LabelText( LOCTEXT( "TimeSnapLabel", "Time" ) )
				.ToolTipText( LOCTEXT( "TimeSnappingIntervalToolTip", "Time snapping interval" ) )
				.Value( this, &SSequencer::OnGetTimeSnapInterval )
				.OnValueChanged( this, &SSequencer::OnTimeSnapIntervalChanged )
			] );
		ToolBarBuilder.AddWidget(
			SNew( SBox )
			.VAlign( VAlign_Center )
			[
				SNew( SNumericDropDown<float> )
				.DropDownValues( SnapValues )
				.LabelText( LOCTEXT( "ValueSnapLabel", "Value" ) )
				.ToolTipText( LOCTEXT( "ValueSnappingIntervalToolTip", "Curve value snapping interval" ) )
				.Value( this, &SSequencer::OnGetValueSnapInterval )
				.OnValueChanged( this, &SSequencer::OnValueSnapIntervalChanged )
			] );
	}
	ToolBarBuilder.EndSection();

	ToolBarBuilder.BeginSection("Curve Editor");
	{
		ToolBarBuilder.SetLabelVisibility( EVisibility::Visible );
		ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().ToggleShowCurveEditor );
		ToolBarBuilder.SetLabelVisibility( EVisibility::Collapsed );
	}

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SWidget> SSequencer::MakeAddMenu()
{
	return OnGetAddMenuContent.Execute(Sequencer.Pin().ToSharedRef());
}

TSharedRef<SWidget> SSequencer::MakeSnapMenu()
{
	FMenuBuilder MenuBuilder( false, Sequencer.Pin()->GetCommandBindings() );

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
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection( "CurveSnapping", LOCTEXT( "SnappingMenuCurveHeader", "Curve Snapping" ) );
	{
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleSnapCurveValueToInterval );
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SSequencer::MakeTransportControls()
{
	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::Get().LoadModuleChecked<FEditorWidgetsModule>( "EditorWidgets" );
	TSharedRef<FSequencer> SequencerPinned = Sequencer.Pin().ToSharedRef();

	FTransportControlArgs TransportControlArgs;
	TransportControlArgs.OnBackwardEnd.BindSP( SequencerPinned, &FSequencer::OnStepToBeginning );
	TransportControlArgs.OnBackwardStep.BindSP( SequencerPinned, &FSequencer::OnStepBackward );
	TransportControlArgs.OnForwardPlay.BindSP( SequencerPinned, &FSequencer::OnPlay, true );
	TransportControlArgs.OnForwardStep.BindSP( SequencerPinned, &FSequencer::OnStepForward );
	TransportControlArgs.OnForwardEnd.BindSP( SequencerPinned, &FSequencer::OnStepToEnd );
	TransportControlArgs.OnToggleLooping.BindSP( SequencerPinned, &FSequencer::OnToggleLooping );
	TransportControlArgs.OnGetLooping.BindSP( SequencerPinned, &FSequencer::IsLooping );
	TransportControlArgs.OnGetPlaybackMode.BindSP( SequencerPinned, &FSequencer::GetPlaybackMode );

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
	if (Sequencer.IsValid())
	{
		auto PlaybackStatus = Sequencer.Pin()->GetPlaybackStatus();
		if (PlaybackStatus == EMovieScenePlayerStatus::Playing || PlaybackStatus == EMovieScenePlayerStatus::Recording)
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
	for (TSharedRef<const FSequencerDisplayNode> SelectedDisplayNode : Sequencer.Pin()->GetSelection().GetSelectedOutlinerNodes().Array())
	{
		SelectedPathNames.Add(SelectedDisplayNode->GetPathName());
	}

	// Update the node tree
	SequencerNodeTree->Update();

	FSequencerTrackLaneFactory Factory(TrackOutliner.ToSharedRef(), TrackArea.ToSharedRef(), Sequencer.Pin().ToSharedRef() );
	Factory.Repopulate( *SequencerNodeTree );

	CurveEditor->SetSequencerNodeTree(SequencerNodeTree);

	// Restore the selection state.
	RestoreSelectionState(SequencerNodeTree->GetRootNodes(), SelectedPathNames, Sequencer.Pin()->GetSelection());	// Update to actor selection.
	OnActorSelectionChanged(NULL);

	SequencerNodeTree->UpdateCachedVisibilityBasedOnShotFiltersChanged();
}

void SSequencer::UpdateBreadcrumbs(const TArray< TWeakObjectPtr<class UMovieSceneSection> >& FilteringShots)
{
	TSharedRef<FMovieSceneInstance> FocusedMovieSceneInstance = Sequencer.Pin()->GetFocusedMovieSceneInstance();

	if (BreadcrumbTrail->PeekCrumb().BreadcrumbType == FSequencerBreadcrumb::ShotType)
	{
		BreadcrumbTrail->PopCrumb();
	}

	if( BreadcrumbTrail->PeekCrumb().BreadcrumbType == FSequencerBreadcrumb::MovieSceneType && BreadcrumbTrail->PeekCrumb().MovieSceneInstance.Pin() != FocusedMovieSceneInstance )
	{
		// The current breadcrumb is not a moviescene so we need to make a new breadcrumb in order return to the parent moviescene later
		BreadcrumbTrail->PushCrumb( FText::FromString(FocusedMovieSceneInstance->GetMovieScene()->GetName()), FSequencerBreadcrumb( FocusedMovieSceneInstance ) );
	}

	if (Sequencer.Pin()->IsShotFilteringOn())
	{
		TAttribute<FText> CrumbString;
		if (FilteringShots.Num() == 1)
		{
			CrumbString = TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SSequencer::GetShotSectionTitle, FilteringShots[0].Get()));
		}
		else 
		{
			CrumbString = LOCTEXT("MultipleShots", "Multiple Shots");
		}

		BreadcrumbTrail->PushCrumb(CrumbString, FSequencerBreadcrumb());
	}

	SequencerNodeTree->UpdateCachedVisibilityBasedOnShotFiltersChanged();
}

void SSequencer::ResetBreadcrumbs()
{
	BreadcrumbTrail->ClearCrumbs();
	BreadcrumbTrail->PushCrumb(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SSequencer::GetRootMovieSceneName)), FSequencerBreadcrumb(Sequencer.Pin()->GetRootMovieSceneInstance()));
}

void SSequencer::OnOutlinerSearchChanged( const FText& Filter )
{
	SequencerNodeTree->FilterNodes( Filter.ToString() );
}

float SSequencer::OnGetTimeSnapInterval() const
{
	return Settings->GetTimeSnapInterval();
}

void SSequencer::OnTimeSnapIntervalChanged( float InInterval )
{
	Settings->SetTimeSnapInterval(InInterval);
}

float SSequencer::OnGetValueSnapInterval() const
{
	return Settings->GetCurveValueSnapInterval();
}

void SSequencer::OnValueSnapIntervalChanged( float InInterval )
{
	Settings->SetCurveValueSnapInterval( InInterval );
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
	if ( !Operation.IsValid() )
	{

	}
	else if ( Operation->IsOfType<FAssetDragDropOp>() )
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

	return bWasDropHandled ? FReply::Handled() : FReply::Unhandled();
}

FReply SSequencer::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) 
{
	// A toolkit tab is active, so direct all command processing to it
	if( Sequencer.Pin()->GetCommandBindings()->ProcessCommandBindings( InKeyEvent ) )
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SSequencer::OnAssetsDropped( const FAssetDragDropOp& DragDropOp )
{
	FSequencer& SequencerRef = *Sequencer.Pin();

	bool bObjectAdded = false;
	bool bSpawnableAdded = false;
	TArray< UObject* > DroppedObjects;
	bool bAllAssetsWereLoaded = true;
	for( auto CurAssetData = DragDropOp.AssetData.CreateConstIterator(); CurAssetData; ++CurAssetData )
	{
		const FAssetData& AssetData = *CurAssetData;

		UObject* Object = AssetData.GetAsset();
		if ( Object != NULL )
		{
			DroppedObjects.Add( Object );
		}
		else
		{
			bAllAssetsWereLoaded = false;
		}
	}

	const TSet< TSharedRef<FSequencerDisplayNode> >& SelectedNodes = Sequencer.Pin()->GetSelection().GetSelectedOutlinerNodes();
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
			TSharedPtr<const FObjectBindingNode> ObjectBindingNode = StaticCastSharedPtr<const FObjectBindingNode>(DisplayNode);
			TargetObjectGuid = ObjectBindingNode->GetObjectBinding();
		}
	}

	for( auto CurObjectIter = DroppedObjects.CreateConstIterator(); CurObjectIter; ++CurObjectIter )
	{
		UObject* CurObject = *CurObjectIter;

		if (!SequencerRef.OnHandleAssetDropped(CurObject, TargetObjectGuid))
		{
			FGuid NewGuid = SequencerRef.AddSpawnableForAssetOrClass( CurObject, NULL );
			bSpawnableAdded = NewGuid.IsValid();
		}
		bObjectAdded = true;
	}


	if( bSpawnableAdded )
	{
		SequencerRef.SpawnOrDestroyPuppetObjects( SequencerRef.GetFocusedMovieSceneInstance() );
	}

	if( bObjectAdded )
	{
		// Update the sequencers view of the movie scene data when any object is added
		SequencerRef.NotifyMovieSceneDataChanged();
	}
}


void SSequencer::OnClassesDropped( const FClassDragDropOp& DragDropOp )
{
	FSequencer& SequencerRef = *Sequencer.Pin();

	bool bSpawnableAdded = false;
	for( auto ClassIter = DragDropOp.ClassesToDrop.CreateConstIterator(); ClassIter; ++ClassIter )
	{
		UClass* Class = ( *ClassIter ).Get();
		if( Class != NULL )
		{
			UObject* Object = Class->GetDefaultObject();

			FGuid NewGuid = SequencerRef.AddSpawnableForAssetOrClass( Object, NULL );

			bSpawnableAdded = NewGuid.IsValid();

		}
	}

	if( bSpawnableAdded )
	{
		SequencerRef.SpawnOrDestroyPuppetObjects( SequencerRef.GetFocusedMovieSceneInstance() );

		// Update the sequencers view of the movie scene data
		SequencerRef.NotifyMovieSceneDataChanged();
	}
}

void SSequencer::OnUnloadedClassesDropped( const FUnloadedClassDragDropOp& DragDropOp )
{
	FSequencer& SequencerRef = *Sequencer.Pin();
	bool bSpawnableAdded = false;
	for( auto ClassDataIter = DragDropOp.AssetsToDrop->CreateConstIterator(); ClassDataIter; ++ClassDataIter )
	{
		auto& ClassData = *ClassDataIter;

		// Check to see if the asset can be found, otherwise load it.
		UObject* Object = FindObject<UObject>( NULL, *ClassData.AssetName );
		if( Object == NULL )
		{
			Object = FindObject<UObject>(NULL, *FString::Printf(TEXT("%s.%s"), *ClassData.GeneratedPackageName, *ClassData.AssetName));
		}

		if( Object == NULL )
		{
			// Load the package.
			GWarn->BeginSlowTask( LOCTEXT("OnDrop_FullyLoadPackage", "Fully Loading Package For Drop"), true, false );
			UPackage* Package = LoadPackage(NULL, *ClassData.GeneratedPackageName, LOAD_NoRedirects );
			if( Package != NULL )
			{
				Package->FullyLoad();
			}
			GWarn->EndSlowTask();

			Object = FindObject<UObject>(Package, *ClassData.AssetName);
		}

		if( Object != NULL )
		{
			// Check to see if the dropped asset was a blueprint
			if(Object->IsA(UBlueprint::StaticClass()))
			{
				// Get the default object from the generated class.
				Object = Cast<UBlueprint>(Object)->GeneratedClass->GetDefaultObject();
			}
		}

		if( Object != NULL )
		{
			FGuid NewGuid = SequencerRef.AddSpawnableForAssetOrClass( Object, NULL );
			bSpawnableAdded = NewGuid.IsValid();
		}
	}

	if( bSpawnableAdded )
	{
		SequencerRef.SpawnOrDestroyPuppetObjects( SequencerRef.GetFocusedMovieSceneInstance() );

		SequencerRef.NotifyMovieSceneDataChanged();
	}
}


void SSequencer::OnActorsDropped( FActorDragDropGraphEdOp& DragDropOp )
{
	Sequencer.Pin()->OnActorsDropped( DragDropOp.Actors );
}

void SSequencer::OnActorSelectionChanged(UObject*)
{
	if (!Sequencer.IsValid())
	{
		return;
	}

	if (!Sequencer.Pin()->IsLevelEditorSequencer())
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
		TSharedRef<FObjectBindingNode> ObjectBindingNode = StaticCastSharedRef<FObjectBindingNode>(Node);
		TArray<UObject*> RuntimeObjects;
		Sequencer.Pin()->GetRuntimeObjects( Sequencer.Pin()->GetFocusedMovieSceneInstance(), ObjectBindingNode->GetObjectBinding(), RuntimeObjects );
		
		bool bDoSelect = false;
		for (int32 RuntimeIndex = 0; RuntimeIndex < RuntimeObjects.Num(); ++RuntimeIndex )
		{
			if (GEditor->GetSelectedActors()->IsSelected(RuntimeObjects[RuntimeIndex]))
			{
				bDoSelect = true;
				break;
			}
		}

		if (Sequencer.Pin()->GetSelection().IsSelected(Node) != bDoSelect)
		{
			if (bDoSelect)
			{
				Sequencer.Pin()->GetSelection().AddToSelection(Node);
			}
			else
			{
				Sequencer.Pin()->GetSelection().RemoveFromSelection(Node);
			}
		}
	}
}

void SSequencer::OnCrumbClicked(const FSequencerBreadcrumb& Item)
{
	if (Item.BreadcrumbType != FSequencerBreadcrumb::ShotType)
	{
		if( Sequencer.Pin()->GetFocusedMovieSceneInstance() == Item.MovieSceneInstance.Pin() ) 
		{
			// then do zooming
		}
		else
		{
			Sequencer.Pin()->PopToMovieScene( Item.MovieSceneInstance.Pin().ToSharedRef() );
		}

		Sequencer.Pin()->FilterToShotSections( TArray< TWeakObjectPtr<UMovieSceneSection> >() );
	}
	else
	{
		// refilter what we already have
		Sequencer.Pin()->FilterToShotSections(Sequencer.Pin()->GetFilteringShotSections());
	}
}

FText SSequencer::GetRootMovieSceneName() const
{
	return FText::FromString(Sequencer.Pin()->GetRootMovieScene()->GetName());
}

FText SSequencer::GetShotSectionTitle(UMovieSceneSection* ShotSection) const
{
	return Cast<UMovieSceneShotSection>(ShotSection)->GetShotDisplayName();
}

void SSequencer::DeleteSelectedNodes()
{
	TSet< TSharedRef<FSequencerDisplayNode> > SelectedNodesCopy = Sequencer.Pin()->GetSelection().GetSelectedOutlinerNodes();

	if( SelectedNodesCopy.Num() > 0 )
	{
		const FScopedTransaction Transaction( LOCTEXT("UndoDeletingObject", "Delete Object from MovieScene") );

		FSequencer& SequencerRef = *Sequencer.Pin();

		for( const TSharedRef<FSequencerDisplayNode>& SelectedNode : SelectedNodesCopy )
		{
			if( SelectedNode->IsVisible() )
			{
				// Delete everything in the entire node
				TSharedRef<const FSequencerDisplayNode> NodeToBeDeleted = StaticCastSharedRef<const FSequencerDisplayNode>(SelectedNode);
				SequencerRef.OnRequestNodeDeleted( NodeToBeDeleted );
			}
		}
	}
}

void SSequencer::ExpandCollapseNode(TSharedRef<FSequencerDisplayNode> Node, bool bDescendants, bool bExpand)
{
	if (Node->IsExpanded() != bExpand)
	{
		Node->ToggleExpansion();
	}

	if (bDescendants)
	{
		for (auto ChildNode : Node->GetChildNodes())
		{
			ExpandCollapseNode(ChildNode, bDescendants, bExpand);
		}
	}
}

FReply SSequencer::OnSaveMovieSceneClicked()
{
	Sequencer.Pin()->SaveCurrentMovieScene();

	return FReply::Unhandled();
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
			TSharedRef<FObjectBindingNode> ObjectBindingNode = StaticCastSharedRef<FObjectBindingNode>(RootNode);
			TArray<UObject*> RuntimeObjects;
			Sequencer.Pin()->GetRuntimeObjects( Sequencer.Pin()->GetFocusedMovieSceneInstance(), ObjectBindingNode->GetObjectBinding(), RuntimeObjects );
		
			for (int32 RuntimeIndex = 0; RuntimeIndex < RuntimeObjects.Num(); ++RuntimeIndex )
			{
				AActor* RuntimeActor = Cast<AActor>(RuntimeObjects[RuntimeIndex]);
				if (RuntimeActor != NULL && LockedActors.Contains(RuntimeActor))
				{
					Nodes.Add(RootNode);
				}
			}
		}
	}
	else
	{
		const TSet< TSharedRef<FSequencerDisplayNode> >& SelectedNodes = Sequencer.Pin()->GetSelection().GetSelectedOutlinerNodes();
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
		float CurrentTime = Sequencer.Pin()->GetCurrentLocalTime(*Sequencer.Pin()->GetFocusedMovieScene());
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
			Sequencer.Pin()->SetGlobalTime(StepToTime);
		}
	}
}

void SSequencer::ToggleExpandCollapseSelectedNodes(bool bDescendants)
{
	const TSet< TSharedRef<FSequencerDisplayNode> >& SelectedNodes = Sequencer.Pin()->GetSelection().GetSelectedOutlinerNodes();

	TSet< TSharedRef<FSequencerDisplayNode> > Nodes(SelectedNodes);

	if (Nodes.Num() == 0)
	{
		TSet<TSharedRef<FSequencerDisplayNode>> RootNodes(SequencerNodeTree->GetRootNodes());
		Nodes = RootNodes;
	}

	if (Nodes.Num() > 0)
	{
		auto It = Nodes.CreateConstIterator();
		bool bExpand = !(*It).Get().IsExpanded();

		for (auto Node : Nodes)
		{
			ExpandCollapseNode(Node, bDescendants, bExpand);
		}
	}
}

EVisibility SSequencer::GetBreadcrumbTrailVisibility() const
{
	return Sequencer.Pin()->IsLevelEditorSequencer() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SSequencer::GetCurveEditorToolBarVisibility() const
{
	return Settings->GetShowCurveEditor() ? EVisibility::Visible : EVisibility::Collapsed;
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

#undef LOCTEXT_NAMESPACE
