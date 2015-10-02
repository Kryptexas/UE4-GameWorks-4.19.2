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
#include "Tools/SequencerEditTool_Selection.h"
#include "Tools/SequencerEditTool_Movement.h"
#include "AssetSelection.h"
#include "MovieSceneShotSection.h"
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

#include "IDetailsView.h"
#include "PropertyEditorModule.h"


#define LOCTEXT_NAMESPACE "Sequencer"


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

/**
* Wrapper box to encompass clicks that don't fall on a tree view row.
*/
class SSequencerTreeViewBox : public SBox
{
public:
	void Construct( const FArguments& InArgs, TSharedRef<FSequencer> InSequencer ) 
	{
		Sequencer = InSequencer;
		SBox::Construct(InArgs);
	}
	
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override 
	{
		if (Sequencer.Pin().IsValid())
		{
			FSequencerSelection& Selection = Sequencer.Pin()->GetSelection();
			if (Selection.GetSelectedOutlinerNodes().Num())
			{
				Selection.EmptySelectedOutlinerNodes();
				return FReply::Handled();
			}
		}
		return FReply::Unhandled();
	}
	
private:
	/** Weak pointer to the sequencer */
	TWeakPtr<FSequencer> Sequencer;
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
	TimeSliderArgs.ClampRange = InArgs._ClampRange;
	TimeSliderArgs.OnViewRangeChanged = InArgs._OnViewRangeChanged;
	TimeSliderArgs.OnClampRangeChanged = InArgs._OnClampRangeChanged;
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
	TSharedRef<ITimeSlider> BottomTimeSlider = SequencerWidgets.CreateTimeSlider( TimeSliderController, TAttribute<EVisibility>(this, &SSequencer::GetBottomTimeSliderVisibility), bMirrorLabels );

	// Create bottom time range slider
	TSharedRef<ITimeSlider> BottomTimeRange = SequencerWidgets.CreateTimeRange( TimeSliderController, TAttribute<EVisibility>(this, &SSequencer::GetTimeRangeVisibility), TAttribute<bool>(this, &SSequencer::ShowFrameNumbers), TAttribute<float>(this, &SSequencer::OnGetTimeSnapInterval));

	OnGetAddMenuContent = InArgs._OnGetAddMenuContent;
	AddMenuExtender = InArgs._AddMenuExtender;

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

	SAssignNew( TrackArea, SSequencerTrackArea, TimeSliderController, SharedThis(this) )
		.Visibility( this, &SSequencer::GetTrackAreaVisibility )
		.LockInOutToStartEndRange(this, &SSequencer::GetLockInOutToStartEndRange );
	
	SAssignNew( TreeView, SSequencerTreeView, SequencerNodeTree.ToSharedRef(), TrackArea.ToSharedRef() )
	.ExternalScrollbar( ScrollBar );

	SAssignNew( CurveEditor, SSequencerCurveEditor, PinnedSequencer, TimeSliderController )
		.Visibility( this, &SSequencer::GetCurveEditorVisibility )
		.OnViewRangeChanged( InArgs._OnViewRangeChanged )
		.ViewRange( InArgs._ViewRange );
	CurveEditor->SetAllowAutoFrame(Settings->GetShowCurveEditor());

	TrackArea->SetTreeView(TreeView);

	EditTool.Reset(new FSequencerEditTool_Movement(PinnedSequencer, SharedThis(this)));

	// initialize details view
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bCustomFilterAreaLocation = true;
		DetailsViewArgs.bCustomNameAreaLocation = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.NotifyHook = this;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
	}

	DetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	{
		DetailsView->SetEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SSequencer::HandleDetailsViewEnabled)));
		DetailsView->SetVisibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &SSequencer::HandleDetailsViewVisibility)));
	}

	const int32 Column0 = 0, Column1 = 1;
	const int32 Row0 = 0, Row1 = 1, Row2 = 2;

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
						SNew(SSplitter)
							.Orientation(Orient_Horizontal)
						
						+ SSplitter::Slot()
							.Value(0.80f)
							[
								SNew(SBox)
									.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
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
																SNew(SHorizontalBox)
								
																+ SHorizontalBox::Slot()
																	.FillWidth( FillCoefficient_0 )
																	[
																		SNew(SSequencerTreeViewBox, PinnedSequencer)
																			.Padding(FMargin(0, 0, 10.f, 0)) // Padding to allow space for the scroll bar
																			[
																				TreeView.ToSharedRef()
																			]
																	]

																+ SHorizontalBox::Slot()
																	.FillWidth( FillCoefficient_1 )
																	[
																		TrackArea.ToSharedRef()
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
							]

						+ SSplitter::Slot()
							.Value(0.2f)
							[
								DetailsView.ToSharedRef()
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

	InSequencer->GetSelection().GetOnKeySelectionChanged().AddSP(this, &SSequencer::HandleKeySelectionChanged);
	InSequencer->GetSelection().GetOnSectionSelectionChanged().AddSP(this, &SSequencer::HandleSectionSelectionChanged);

	ResetBreadcrumbs();
}

void SSequencer::BindCommands(TSharedRef<FUICommandList> SequencerCommandBindings)
{
	SequencerCommandBindings->MapAction(
		FSequencerCommands::Get().MoveTool,
		FExecuteAction::CreateLambda( [&]{
			EditTool.Reset( new FSequencerEditTool_Movement(Sequencer.Pin(), SharedThis(this)) );
		} ),
		FCanExecuteAction::CreateLambda( []{ return true; } ),
		FIsActionChecked::CreateSP( this, &SSequencer::IsEditToolEnabled, FName("Movement") )
		);

	SequencerCommandBindings->MapAction(
		FSequencerCommands::Get().MarqueeTool,
		FExecuteAction::CreateLambda( [&]{
			EditTool.Reset( new FSequencerEditTool_Selection(Sequencer.Pin(), SharedThis(this)) );
		} ),
		FCanExecuteAction::CreateLambda( []{ return true; } ),
		FIsActionChecked::CreateSP( this, &SSequencer::IsEditToolEnabled, FName("Selection") )
		);
}


void SSequencer::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged)
{
}


/* SSequencer implementation
 *****************************************************************************/

void SSequencer::UpdateDetailsView()
{
	TArray<TWeakObjectPtr<UObject>> Sections;

	// get selected sections
	for (auto Section : Sequencer.Pin()->GetSelection().GetSelectedSections())
	{
		Sections.Add(Section);
	}

	// get sections from selected keys
	const TArray<FSelectedKey> SelectedKeys = Sequencer.Pin()->GetSelection().GetSelectedKeys().Array();

	for (const auto& Key : SelectedKeys)
	{
		if (Key.Section != nullptr)
		{
			Sections.AddUnique(Key.Section);
		}
	}

	// @todo sequencer: highlight selected keys in details view

	// update details view
	DetailsView->SetObjects(Sections, true);
}


/* SSequencer callbacks
 *****************************************************************************/

bool SSequencer::HandleDetailsViewEnabled() const
{
	return true;
}


EVisibility SSequencer::HandleDetailsViewVisibility() const
{
	if (Sequencer.Pin()->GetSettings()->GetDetailsViewVisible())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


void SSequencer::HandleKeySelectionChanged()
{
	UpdateDetailsView();
}


void SSequencer::HandleSectionSelectionChanged()
{
	UpdateDetailsView();
}


bool SSequencer::IsEditToolEnabled(FName InIdentifier)
{
	return GetEditTool().GetIdentifier() == InIdentifier;
}

TSharedRef<SWidget> SSequencer::MakeToolBar()
{
	FToolBarBuilder ToolBarBuilder( Sequencer.Pin()->GetCommandBindings(), FMultiBoxCustomization::None, TSharedPtr<FExtender>(), Orient_Horizontal, true);
	
	ToolBarBuilder.BeginSection("Base Commands");
	{
		// Add Track menu
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
			]);

		// General 
		if( Sequencer.Pin()->IsLevelEditorSequencer() )
		{
			ToolBarBuilder.AddToolBarButton(
				FUIAction(FExecuteAction::CreateSP(this, &SSequencer::OnSaveMovieSceneClicked))
				, NAME_None
				, LOCTEXT("SaveDirtyPackages", "Save")
				, LOCTEXT("SaveDirtyPackagesTooltip", "Saves the current level sequence")
				, FSlateIcon(FEditorStyle::GetStyleSetName(), "Sequencer.Save"));

			ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().RenderMovie );

			ToolBarBuilder.AddSeparator();
		}

		ToolBarBuilder.AddComboButton(
			FUIAction()
			, FOnGetContent::CreateSP( this, &SSequencer::MakeGeneralMenu )
			, LOCTEXT( "GeneralOptions", "General Options" )
			, LOCTEXT( "GeneralOptionsToolTip", "General Options" )
			, FSlateIcon(FEditorStyle::GetStyleSetName(), "Sequencer.General") );

		ToolBarBuilder.AddSeparator();

		if( Sequencer.Pin()->IsLevelEditorSequencer() )
		{
			TAttribute<FSlateIcon> KeyAllIcon;
			KeyAllIcon.Bind(TAttribute<FSlateIcon>::FGetter::CreateLambda([&]{
				static FSlateIcon KeyAllEnabledIcon(FEditorStyle::GetStyleSetName(), "Sequencer.KeyAllEnabled");
				static FSlateIcon KeyAllDisabledIcon(FEditorStyle::GetStyleSetName(), "Sequencer.KeyAllDisabled");

				return Sequencer.Pin()->GetKeyAllEnabled() ? KeyAllEnabledIcon : KeyAllDisabledIcon;
			}));
			ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().ToggleKeyAllEnabled, NAME_None, TAttribute<FText>(), TAttribute<FText>(), KeyAllIcon );
		}

		ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().ToggleAutoKeyEnabled );
	}
	ToolBarBuilder.EndSection();

	ToolBarBuilder.BeginSection("Tools");
	{
		ToolBarBuilder.SetLabelVisibility( EVisibility::Collapsed );

		ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().MoveTool );
		ToolBarBuilder.AddToolBarButton( FSequencerCommands::Get().MarqueeTool );
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
			] );

		ToolBarBuilder.AddWidget(
			SNew( SImage )
			.Image(FEditorStyle::GetBrush("Sequencer.Value")) );

		ToolBarBuilder.AddWidget(
			SNew( SBox )
			.VAlign( VAlign_Center )
			[
				SNew( SNumericDropDown<float> )
				.DropDownValues( SequencerSnapValues::GetSnapValues() )
				.ToolTipText( LOCTEXT( "ValueSnappingIntervalToolTip", "Curve value snapping interval" ) )
				.Value( this, &SSequencer::OnGetValueSnapInterval )
				.OnValueChanged( this, &SSequencer::OnValueSnapIntervalChanged )
			] );
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
		OnGetAddMenuContent.ExecuteIfBound(MenuBuilder, Sequencer.Pin().ToSharedRef());
		MenuBuilder.EndSection();

		// let track editors populate the menu
		TSharedPtr<FSequencer> PinnedSequencer = Sequencer.Pin();

		// Always create the section so that we afford extension
		MenuBuilder.BeginSection("AddTracks");
		if (PinnedSequencer.IsValid())
		{
			PinnedSequencer->BuildAddTrackMenu(MenuBuilder);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SSequencer::MakeGeneralMenu()
{
	FMenuBuilder MenuBuilder( true, Sequencer.Pin()->GetCommandBindings() );

	MenuBuilder.BeginSection( "ViewOptions", LOCTEXT( "ViewMenuHeader", "View" ) );
	{
		if (Sequencer.Pin()->IsLevelEditorSequencer())
		{
			MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleDetailsView );
		}

		if (Sequencer.Pin()->IsLevelEditorSequencer())
		{
			MenuBuilder.AddMenuEntry( FSequencerCommands::Get().FindInContentBrowser );
		}

		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ExpandNodesAndDescendants );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().CollapseNodesAndDescendants );
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SSequencer::MakeSnapMenu()
{
	FMenuBuilder MenuBuilder( false, Sequencer.Pin()->GetCommandBindings() );

	MenuBuilder.BeginSection("FramesRanges", LOCTEXT("SnappingMenuFrameRangesHeader", "Frame Ranges") );
	{
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleAutoScroll );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleShowFrameNumbers );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleShowRangeSlider );
		MenuBuilder.AddMenuEntry( FSequencerCommands::Get().ToggleLockInOutToStartEndRange );				
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
	for (TSharedRef<const FSequencerDisplayNode> SelectedDisplayNode : Sequencer.Pin()->GetSelection().GetSelectedOutlinerNodes().Array())
	{
		SelectedPathNames.Add(SelectedDisplayNode->GetPathName());
	}

	// Suspend broadcasting selection changes because we don't want unnecessary rebuilds.
	Sequencer.Pin()->GetSelection().SuspendBroadcast();

	// Update the node tree
	SequencerNodeTree->Update();

	TreeView->Refresh();

	// Restore the selection state.
	RestoreSelectionState(SequencerNodeTree->GetRootNodes(), SelectedPathNames, Sequencer.Pin()->GetSelection());	// Update to actor selection.
	OnActorSelectionChanged(NULL);

	// This must come after the selection state has been restored so that the curve editor is populated with the correctly selected nodes
	CurveEditor->SetSequencerNodeTree(SequencerNodeTree);

	// Continue broadcasting selection changes
	Sequencer.Pin()->GetSelection().ResumeBroadcast();
}

void SSequencer::UpdateBreadcrumbs(const TArray< TWeakObjectPtr<class UMovieSceneSection> >& FilteringShots)
{
	TSharedRef<FMovieSceneSequenceInstance> FocusedMovieSceneInstance = Sequencer.Pin()->GetFocusedMovieSceneSequenceInstance();

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
	BreadcrumbTrail->PushCrumb(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SSequencer::GetRootAnimationName)), FSequencerBreadcrumb(Sequencer.Pin()->GetRootMovieSceneSequenceInstance()));
}

void SSequencer::OnOutlinerSearchChanged( const FText& Filter )
{
	SequencerNodeTree->FilterNodes( Filter.ToString() );
	TreeView->Refresh();
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
		SequencerRef.SpawnOrDestroyPuppetObjects( SequencerRef.GetFocusedMovieSceneSequenceInstance() );
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
		SequencerRef.SpawnOrDestroyPuppetObjects( SequencerRef.GetFocusedMovieSceneSequenceInstance() );

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
		SequencerRef.SpawnOrDestroyPuppetObjects( SequencerRef.GetFocusedMovieSceneSequenceInstance() );

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
		Sequencer.Pin()->GetRuntimeObjects( Sequencer.Pin()->GetFocusedMovieSceneSequenceInstance(), ObjectBindingNode->GetObjectBinding(), RuntimeObjects );
		
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

	const TSet<TSharedRef<FSequencerDisplayNode>>& OutlinerSelection = Sequencer.Pin()->GetSelection().GetSelectedOutlinerNodes();
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
		if( Sequencer.Pin()->GetFocusedMovieSceneSequenceInstance() == Item.MovieSceneInstance.Pin() ) 
		{
			// then do zooming
		}
		else
		{
			Sequencer.Pin()->PopToMovieScene( Item.MovieSceneInstance.Pin().ToSharedRef() );
		}
	}
}

FText SSequencer::GetRootAnimationName() const
{
	return Sequencer.Pin()->GetRootMovieSceneSequence()->GetDisplayName();
}

FText SSequencer::GetShotSectionTitle(UMovieSceneSection* ShotSection) const
{
	return Cast<UMovieSceneShotSection>(ShotSection)->GetShotDisplayName();
}

TSharedPtr<SSequencerTreeView> SSequencer::GetTreeView() const
{
	return TreeView;
}

TArray<FSectionHandle> SSequencer::GetSectionHandles(const TSet<TWeakObjectPtr<UMovieSceneSection>>& DesiredSections) const
{
	TArray<FSectionHandle> SectionHandles;

	for (auto& Node : SequencerNodeTree->GetRootNodes())
	{
		Node->Traverse_ParentFirst([&](FSequencerDisplayNode& InNode){
			if (InNode.GetType() == ESequencerNode::Track)
			{
				FTrackNode& TrackNode = static_cast<FTrackNode&>(InNode);

				const auto& AllSections = TrackNode.GetTrack()->GetAllSections();
				for (int32 Index = 0; Index < AllSections.Num(); ++Index)
				{
					if (DesiredSections.Contains(TWeakObjectPtr<UMovieSceneSection>(AllSections[Index])))
					{
						SectionHandles.Emplace(StaticCastSharedRef<FTrackNode>(TrackNode.AsShared()), Index);
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
	Sequencer.Pin()->SaveCurrentMovieScene();
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
			Sequencer.Pin()->GetRuntimeObjects( Sequencer.Pin()->GetFocusedMovieSceneSequenceInstance(), ObjectBindingNode->GetObjectBinding(), RuntimeObjects );
		
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
		float CurrentTime = Sequencer.Pin()->GetCurrentLocalTime(*Sequencer.Pin()->GetFocusedMovieSceneSequence());
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

EVisibility SSequencer::GetBreadcrumbTrailVisibility() const
{
	return Sequencer.Pin()->IsLevelEditorSequencer() ? EVisibility::Visible : EVisibility::Collapsed;
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
	return Sequencer.Pin()->CanShowFrameNumbers() && Settings->GetShowFrameNumbers();
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

bool SSequencer::GetLockInOutToStartEndRange() const
{
	return Settings->GetLockInOutToStartEndRange();
}

void SSequencer::OnCurveEditorVisibilityChanged()
{
	if (CurveEditor.IsValid())
	{
		// Only zoom horizontally if the editor is visible
		CurveEditor->SetAllowAutoFrame(Settings->GetShowCurveEditor());
	}
}

FVirtualTrackArea SSequencer::GetVirtualTrackArea() const
{
	return FVirtualTrackArea(*Sequencer.Pin(), *TreeView.Get(), TrackArea->GetCachedGeometry());
}

#undef LOCTEXT_NAMESPACE
