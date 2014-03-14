// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"

#include "SAnimCurvePanel.h"
#include "ScopedTransaction.h"
#include "SCurveEditor.h"
#include "Editor/KismetWidgets/Public/SScrubWidget.h"
#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "AnimCurvePanel"

//////////////////////////////////////////////////////////////////////////
//  FAnimCurveInterface interface 

/** Interface you implement if you want the CurveEditor to be able to edit curves on you */
class FAnimCurveBaseInterface : public FCurveOwnerInterface
{
public:
	TWeakObjectPtr<UAnimSequenceBase>	BaseSequence;
	FAnimCurveBase*	CurveData;

public:
	FAnimCurveBaseInterface(UAnimSequenceBase * BaseSeq, FAnimCurveBase*	InData)
		: BaseSequence(BaseSeq)
		, CurveData(InData)
	{
		// they should be valid
		check (BaseSequence.IsValid());
		check (CurveData);
	}

	/** Returns set of curves to edit. Must not release the curves while being edited. */
	virtual TArray<FRichCurveEditInfoConst> GetCurves() const
	{
		TArray<FRichCurveEditInfoConst> Curves;
		FFloatCurve * FloatCurveData = (FFloatCurve*)(CurveData);
		Curves.Add(&FloatCurveData->FloatCurve);

		return Curves;
	}

	/** Returns set of curves to query. Must not release the curves while being edited. */
	virtual TArray<FRichCurveEditInfo> GetCurves()
	{
		TArray<FRichCurveEditInfo> Curves;
		FFloatCurve * FloatCurveData = (FFloatCurve*)(CurveData);
		Curves.Add(&FloatCurveData->FloatCurve);

		return Curves;
	}

	virtual UObject* GetOwner()
	{
		if (BaseSequence.IsValid())
		{
			return BaseSequence.Get();
		}

		return NULL;
	}

	/** Called to modify the owner of the curve */
	virtual void ModifyOwner()
	{
		if (BaseSequence.IsValid())
		{
			BaseSequence.Get()->Modify(true);
		}
	}

	/** Called to make curve owner transactional */
	virtual void MakeTransactional()
	{
		if (BaseSequence.IsValid())
		{
			BaseSequence.Get()->SetFlags(BaseSequence.Get()->GetFlags() | RF_Transactional);
		}
	}
};

//////////////////////////////////////////////////////////////////////////
//  SAnimCurveEd : anim curve editor

class SAnimCurveEd : public SCurveEditor
{
public:
	SLATE_BEGIN_ARGS( SAnimCurveEd )
		: _ViewMinInput(0.0f)
		, _ViewMaxInput(10.0f)
		, _TimelineLength(5.0f)
		, _DrawCurve(true)
		, _HideUI(true)
		, _OnGetScrubValue()
	{}
	
		SLATE_ATTRIBUTE( float, ViewMinInput )
		SLATE_ATTRIBUTE( float, ViewMaxInput )
		SLATE_ATTRIBUTE( TOptional<float>, DataMinInput )
		SLATE_ATTRIBUTE( TOptional<float>, DataMaxInput )
		SLATE_ATTRIBUTE( float, TimelineLength )
		SLATE_ATTRIBUTE( int32, NumberOfKeys)
		SLATE_ATTRIBUTE( FVector2D, DesiredSize )
		SLATE_ARGUMENT( bool, DrawCurve )
		SLATE_ARGUMENT( bool, HideUI )
		SLATE_EVENT( FOnGetScrubValue, OnGetScrubValue )
		SLATE_EVENT( FOnSetInputViewRange, OnSetInputViewRange )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	// SWidget interface
	virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const OVERRIDE;
	// SWidget interface

	// SCurveEditor interface
	virtual void SetDefaultOutput(const float MinZoomRange) OVERRIDE;
	virtual float GetTimeStep(FTrackScaleInfo &ScaleInfo) const OVERRIDE;
	// SCurveEditor interface
	
private:
	// scrub value grabber
	FOnGetScrubValue	OnGetScrubValue;
	// @todo redo this code, all mess and dangerous
	TAttribute<int32>	NumberOfKeys;
};

//////////////////////////////////////////////////////////////////////////
//  SAnimCurveEd : anim curve editor

float SAnimCurveEd::GetTimeStep(FTrackScaleInfo &ScaleInfo)const
{
	if(NumberOfKeys.Get())
	{
		int32 Divider = SScrubWidget::GetDivider( ViewMinInput.Get(), ViewMaxInput.Get(), ScaleInfo.WidgetSize, TimelineLength.Get(), NumberOfKeys.Get());

		float TimePerKey;

		if ( NumberOfKeys.Get() != 0.f )
		{
			TimePerKey = TimelineLength.Get()/(float)NumberOfKeys.Get();
		}
		else
		{
			TimePerKey = 1.f;
		}

		return TimePerKey * Divider;
	}

	return 0.0f;
}

int32 SAnimCurveEd::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 NewLayerId = SCurveEditor::OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;

	float Value = 0.f;

	if ( OnGetScrubValue.IsBound() )
	{
		Value = OnGetScrubValue.Execute();
	}

	FPaintGeometry MyGeometry = AllottedGeometry.ToPaintGeometry();

	// scale info
	FTrackScaleInfo ScaleInfo(ViewMinInput.Get(), ViewMaxInput.Get(), 0.f, 0.f, AllottedGeometry.Size);
	float XPos = ScaleInfo.InputToLocalX(Value);

	TArray<FVector2D> LinePoints;
	LinePoints.Add(FVector2D(XPos-1, 0.f));
	LinePoints.Add(FVector2D(XPos+1, AllottedGeometry.Size.Y));


	FSlateDrawElement::MakeLines( 
		OutDrawElements,
		NewLayerId,
		MyGeometry,
		LinePoints,
		MyClippingRect,
		ESlateDrawEffect::None,
		FLinearColor(1, 0, 0)
		);

	// now draw scrub with new layer ID + 1;
	return NewLayerId;
}

void SAnimCurveEd::Construct(const FArguments& InArgs)
{
	OnGetScrubValue = InArgs._OnGetScrubValue;
	NumberOfKeys = InArgs._NumberOfKeys;

	SCurveEditor::Construct( SCurveEditor::FArguments()
		.ViewMinInput(InArgs._ViewMinInput)
		.ViewMaxInput(InArgs._ViewMaxInput)
		.DataMinInput(InArgs._DataMinInput)
		.DataMaxInput(InArgs._DataMaxInput)
		.ViewMinOutput(0.f)
		.ViewMaxOutput(1.f)
		.ZoomToFit(false)
		.TimelineLength(InArgs._TimelineLength)
		.DrawCurve(InArgs._DrawCurve)
		.HideUI(InArgs._HideUI)
		.AllowZoomOutput(false)
		.DesiredSize(InArgs._DesiredSize)
		.OnSetInputViewRange(InArgs._OnSetInputViewRange));
}

void SAnimCurveEd::SetDefaultOutput(const float MinZoomRange)
{
	ViewMinOutput = (ViewMinOutput);
	ViewMaxOutput = (ViewMaxOutput + (MinZoomRange));
}
//////////////////////////////////////////////////////////////////////////
//  SCurveEd Track : each track for curve editing 

/** Widget for editing a single track of animation curve - this includes CurveEditor */
class SCurveEdTrack : public SCompoundWidget
{
private:
	/** Pointer to notify panel for drawing*/
	TSharedPtr<class SCurveEditor>			CurveEditor;

	/** Name of curve it's editing - CurveName should be unique within this tracks**/
	FAnimCurveBaseInterface	*				CurveInterface;

	/** Curve Panel Ptr **/
	TWeakPtr<SAnimCurvePanel>				PanelPtr;

	/** is using expanded editor **/
	bool									bUseExpandEditor;

public:
	SLATE_BEGIN_ARGS( SCurveEdTrack )
		: _AnimCurvePanel()
		, _Sequence()
		, _CurveName()
		, _WidgetWidth()
		, _ViewInputMin()
		, _ViewInputMax()
		, _OnSetInputViewRange()
		, _OnGetScrubValue()
	{}
	SLATE_ARGUMENT( TSharedPtr<SAnimCurvePanel>, AnimCurvePanel)
	// editing related variables
	SLATE_ARGUMENT( class UAnimSequenceBase*, Sequence )
	SLATE_ARGUMENT( FName, CurveName ) 
	// widget viewing related variables
	SLATE_ARGUMENT( float, WidgetWidth ) // @todo do I need this?
	SLATE_ATTRIBUTE( float, ViewInputMin )
	SLATE_ATTRIBUTE( float, ViewInputMax )
	SLATE_EVENT( FOnSetInputViewRange, OnSetInputViewRange )
	SLATE_EVENT( FOnGetScrubValue, OnGetScrubValue )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SCurveEdTrack();

	// input handling for curve name
	void NewCurveNameEntered( const FText& NewText, ETextCommit::Type CommitInfo );

	// Delete current track
	void DeleteTrack();

	// Sets the current mode for this curve
	void ToggleCurveMode(ESlateCheckBoxState::Type NewState, EAnimCurveFlags ModeToSet);

	// Returns whether this curve is of the specificed mode type
	ESlateCheckBoxState::Type IsCurveOfMode(EAnimCurveFlags ModeToTest) const;

	/**
	 * Build and display curve track context menu.
	 *
	 */
	FReply OnContextMenu();

	// expand editor mode 
	ESlateCheckBoxState::Type IsEditorExpanded() const;
	void ToggleExpandEditor(ESlateCheckBoxState::Type NewType);
	const FSlateBrush* GetExpandContent() const;
	FVector2D GetDesiredSize() const;
};

//////////////////////////////////////////////////////////////////////////
// SCurveEdTrack

void SCurveEdTrack::Construct(const FArguments& InArgs)
{
	TSharedRef<SAnimCurvePanel> PanelRef = InArgs._AnimCurvePanel.ToSharedRef();
	PanelPtr = InArgs._AnimCurvePanel;
	bUseExpandEditor = false;
	// now create CurveInterface, 
	// find which curve this belongs to
	UAnimSequenceBase * Sequence = InArgs._Sequence;
	check (Sequence);

	// get the curve data
	FAnimCurveBase * Curve = Sequence->RawCurveData.GetCurveData(InArgs._CurveName);
	check (Curve);

	CurveInterface = new FAnimCurveBaseInterface(Sequence, Curve);
	int32 NumberOfKeys = Sequence->GetNumberOfFrames();
	//////////////////////////////
	this->ChildSlot
	[
		SNew( SBorder )
		.Padding( FMargin(2.0f, 2.0f) )
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1)
			[
				// Notification editor panel
				SAssignNew(CurveEditor, SAnimCurveEd)
				.ViewMinInput(InArgs._ViewInputMin)
				.ViewMaxInput(InArgs._ViewInputMax)
				.DataMinInput(0.f)
				.DataMaxInput(Sequence->SequenceLength)
				// @fixme fix this to delegate
				.TimelineLength(Sequence->SequenceLength)
				.NumberOfKeys(NumberOfKeys)
				.DesiredSize(this, &SCurveEdTrack::GetDesiredSize)
				.OnSetInputViewRange(InArgs._OnSetInputViewRange)
				.OnGetScrubValue(InArgs._OnGetScrubValue)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew( SBox )
				.WidthOverride(InArgs._WidgetWidth)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Center)
					.FillWidth(1)
					[
						// Name of track
						SNew(SEditableText)
						.MinDesiredWidth(64.0f)
						.IsEnabled(true)
						.Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
						.SelectAllTextWhenFocused(true)
						.Text(FText::FromName(Curve->CurveName))
						.OnTextCommitted(this, &SCurveEdTrack::NewCurveNameEntered)
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						// Name of track
						SNew( SCheckBox )
						.IsChecked(this, &SCurveEdTrack::IsEditorExpanded)
						.OnCheckStateChanged(this, &SCurveEdTrack::ToggleExpandEditor)
						.ToolTipText( LOCTEXT("Expand window", "Expand window") )
						.IsEnabled( true )
						[
							SNew( SImage )
							.Image(this, &SCurveEdTrack::GetExpandContent)
						]
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						// Name of track
						SNew(SButton)
						.ToolTipText( LOCTEXT("DisplayTrackOptionsMenuTooltip", "Display track options menu") )
						.OnClicked( this, &SCurveEdTrack::OnContextMenu )
						.Content()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("ComboButton.Arrow"))
							.ColorAndOpacity( FSlateColor::UseForeground() )
						]
					]
				]
			]
		]
	];

	//Inform track widget about the curve and whether it is editable or not.
	CurveEditor->SetCurveOwner(CurveInterface, true );
}

/** return a widget */
const FSlateBrush* SCurveEdTrack::GetExpandContent() const
{
	if (bUseExpandEditor)
	{
		return FEditorStyle::GetBrush("Kismet.VariableList.HideForInstance");
	}
	else
	{
		return FEditorStyle::GetBrush("Kismet.VariableList.ExposeForInstance");
	}

}
void SCurveEdTrack::NewCurveNameEntered( const FText& NewText, ETextCommit::Type CommitInfo )
{
	if ( CommitInfo == ETextCommit::OnEnter || CommitInfo == ETextCommit::OnUserMovedFocus )
	{
		CurveInterface->CurveData->CurveName = FName( *NewText.ToString() );
	}
}

SCurveEdTrack::~SCurveEdTrack()
{
	// @fixme - check -is this okay way of doing it?
	delete CurveInterface;
}

void SCurveEdTrack::DeleteTrack()
{
	if ( PanelPtr.IsValid() )
	{
		PanelPtr.Pin()->DeleteTrack(CurveInterface->CurveData->CurveName.ToString());
	}
}

void SCurveEdTrack::ToggleCurveMode(ESlateCheckBoxState::Type NewState,EAnimCurveFlags ModeToSet)
{
	const int32 AllModes = (ACF_DrivesMorphTarget|ACF_DrivesMaterial);
	check((ModeToSet&AllModes) != 0); //unexpected value for ModeToSet

	FFloatCurve* CurveData = (FFloatCurve*)(CurveInterface->CurveData);

	FText UndoLabel;
	bool bIsSwitchingFlagOn = !CurveData->GetCurveTypeFlag(ModeToSet);
	check(bIsSwitchingFlagOn == (NewState==ESlateCheckBoxState::Checked));
	
	if(bIsSwitchingFlagOn)
	{
		if(ModeToSet == ACF_DrivesMorphTarget)
		{
			UndoLabel = LOCTEXT("AnimCurve_TurnOnMorphMode", "Enable driving of morph targets");
		}
		else if(ModeToSet == ACF_DrivesMaterial)
		{
			UndoLabel = LOCTEXT("AnimCurve_TurnOnMaterialMode", "Enable driving of materials");
		}
	}
	else
	{
		if(ModeToSet == ACF_DrivesMorphTarget)
		{
			UndoLabel = LOCTEXT("AnimCurve_TurnOffMorphMode", "Disable driving of morph targets");
		}
		else if(ModeToSet == ACF_DrivesMaterial)
		{
			UndoLabel = LOCTEXT("AnimCurve_TurnOffMaterialMode", "Disable driving of materials");
		}
	}

	const FScopedTransaction Transaction( UndoLabel );
	CurveInterface->MakeTransactional();
	CurveInterface->ModifyOwner();

	CurveData->SetCurveTypeFlag(ModeToSet, bIsSwitchingFlagOn);
}

ESlateCheckBoxState::Type SCurveEdTrack::IsCurveOfMode(EAnimCurveFlags ModeToTest) const
{
	FFloatCurve* CurveData = (FFloatCurve*)(CurveInterface->CurveData);
	return CurveData->GetCurveTypeFlag(ModeToTest) ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

FReply SCurveEdTrack::OnContextMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	MenuBuilder.BeginSection("AnimCurvePanelCurveTypes", LOCTEXT("CurveTypesHeading", "Curve Types"));
	{
		MenuBuilder.AddWidget(
			SNew(SCheckBox)
			.IsChecked( this, &SCurveEdTrack::IsCurveOfMode, ACF_DrivesMorphTarget )
			.OnCheckStateChanged( this, &SCurveEdTrack::ToggleCurveMode, ACF_DrivesMorphTarget )
			.ToolTipText(LOCTEXT("MorphCurveModeTooltip", "This curve drives a morph target").ToString())
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MorphCurveMode", "Morph Curve").ToString())
			],
			FText()
		);

		MenuBuilder.AddWidget(
			SNew(SCheckBox)
			.IsChecked( this, &SCurveEdTrack::IsCurveOfMode, ACF_DrivesMaterial )
			.OnCheckStateChanged( this, &SCurveEdTrack::ToggleCurveMode, ACF_DrivesMaterial )
			.ToolTipText(LOCTEXT("MaterialCurveModeTooltip", "This curve drives a material").ToString())
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MaterialCurveMode", "Material Curve").ToString())
			],
			FText()
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("AnimCurvePanelTrackOptions", LOCTEXT("TrackOptionsHeading", "Track Options") );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("RemoveTrack", "Remove Track"),
			LOCTEXT("RemoveTrackTooltip", "Remove this track"),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateSP( this, &SCurveEdTrack::DeleteTrack ),
			FCanExecuteAction()
			)
		);

		FSlateApplication::Get().PushMenu(	SharedThis(this),
			MenuBuilder.MakeWidget(),
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup) );
	}
	MenuBuilder.EndSection();

	return FReply::Handled();
}

ESlateCheckBoxState::Type SCurveEdTrack::IsEditorExpanded() const
{
	return (bUseExpandEditor)? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SCurveEdTrack::ToggleExpandEditor(ESlateCheckBoxState::Type NewType)
{
	bUseExpandEditor = (NewType == ESlateCheckBoxState::Checked);
}

FVector2D SCurveEdTrack::GetDesiredSize() const
{
	if (bUseExpandEditor)
	{
		return FVector2D(128.f, 128.f);
	}
	else
	{
		return FVector2D(128.f, 32.f);
	}
}
//////////////////////////////////////////////////////////////////////////
// SAnimCurvePanel

void SAnimCurvePanel::Construct(const FArguments& InArgs)
{
	SAnimTrackPanel::Construct( SAnimTrackPanel::FArguments()
		.WidgetWidth(InArgs._WidgetWidth)
		.ViewInputMin(InArgs._ViewInputMin)
		.ViewInputMax(InArgs._ViewInputMax)
		.InputMin(InArgs._InputMin)
		.InputMax(InArgs._InputMax)
		.OnSetInputViewRange(InArgs._OnSetInputViewRange));

	Sequence = InArgs._Sequence;
	WidgetWidth = InArgs._WidgetWidth;
	OnGetScrubValue = InArgs._OnGetScrubValue;

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.FillHeight(1)
		[
			SNew( SExpandableArea )
			.AreaTitle( LOCTEXT("Curves", "Curves") )
			.BodyContent()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						// Name of track
						SNew(SButton)
						.Text( LOCTEXT("AddFloatTrack", "Add Float Track") )
						.ToolTipText( LOCTEXT("AddTrackTooltip", "Add float track above here") )
						.OnClicked( this, &SAnimCurvePanel::CreateNewTrack )
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew( SComboButton )
						.ContentPadding(FMargin(2.0f))
						.OnGetMenuContent(this, &SAnimCurvePanel::GenerateCurveList)
					]

					+SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.Padding( 2.0f, 0.0f )
					[
						// Name of track
						SNew(SButton)
						.ToolTipText( LOCTEXT("DisplayTrackOptionsMenuForAllTracksTooltip", "Display track options menu for all tracks") )
						.OnClicked( this, &SAnimCurvePanel::OnContextMenu )
						.Visibility( TAttribute<EVisibility>( this, &SAnimCurvePanel::IsSetAllTracksButtonVisible ) )
						.Content()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("ComboButton.Arrow"))
							.ColorAndOpacity( FSlateColor::UseForeground() )
						]
					]

				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew( PanelSlot, SSplitter )
					.Orientation(Orient_Vertical)
				]
			]
		]
	];

	UpdatePanel();
}

FReply SAnimCurvePanel::CreateNewTrack()
{
	TSharedRef<STextEntryPopup> TextEntry =
		SNew(STextEntryPopup)
		.Label( LOCTEXT("TrackName", "Track Name") )
		.OnTextCommitted( this, &SAnimCurvePanel::AddTrack );

	// Show dialog to enter new track name
	FSlateApplication::Get().PushMenu(
		SharedThis( this ),
		TextEntry,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup)
		);

	TextEntry->FocusDefaultWidget();

	return FReply::Handled();	
}

void SAnimCurvePanel::AddTrack(const FText & CurveNameToAdd, ETextCommit::Type CommitInfo)
{
	if ( CommitInfo == ETextCommit::OnEnter )
	{
		const FScopedTransaction Transaction( LOCTEXT("AnimCurve_AddTrack", "Add New Curve") );
		// before insert, make sure everything behind is fixed
		if ( Sequence->RawCurveData.AddCurveData(FName( *CurveNameToAdd.ToString() )) )
		{
			Sequence->Modify(true);
			UpdatePanel();
		}

		FSlateApplication::Get().DismissAllMenus();
	}
}

FReply SAnimCurvePanel::DeleteTrack(const FString & CurveNameToDelete)
{
	const FScopedTransaction Transaction( LOCTEXT("AnimCurve_AddTrack", "Add New Curve") );
	if ( Sequence->RawCurveData.DeleteCurveData(*CurveNameToDelete) )
	{
		Sequence->Modify(true);
		UpdatePanel(true);
		return FReply::Handled();
	}

	return FReply::Unhandled();	
}

FReply SAnimCurvePanel::OnContextMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	MenuBuilder.BeginSection("AnimCurvePanelCurveTypes", LOCTEXT("AllCurveTypesHeading", "All Curve Types"));
	{
		MenuBuilder.AddWidget(
			SNew(SCheckBox)
			.IsChecked( this, &SAnimCurvePanel::AreAllCurvesOfMode, ACF_DrivesMorphTarget )
			.OnCheckStateChanged( this, &SAnimCurvePanel::ToggleAllCurveModes, ACF_DrivesMorphTarget )
			.ToolTipText(LOCTEXT("MorphCurveModeTooltip", "This curve drives a morph target").ToString())
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MorphCurveMode", "Morph Curve").ToString())
			],
			FText()
		);

		MenuBuilder.AddWidget(
			SNew(SCheckBox)
			.IsChecked( this, &SAnimCurvePanel::AreAllCurvesOfMode, ACF_DrivesMaterial )
			.OnCheckStateChanged( this, &SAnimCurvePanel::ToggleAllCurveModes, ACF_DrivesMaterial )
			.ToolTipText(LOCTEXT("MaterialCurveModeTooltip", "This curve drives a material").ToString())
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MaterialCurveMode", "Material Curve").ToString())
			],
			FText()
		);

		FSlateApplication::Get().PushMenu(	SharedThis(this),
			MenuBuilder.MakeWidget(),
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup) );
	}
	MenuBuilder.EndSection();

	return FReply::Handled();
}

EVisibility SAnimCurvePanel::IsSetAllTracksButtonVisible() const
{
	return (Tracks.Num() > 1) ? EVisibility::Visible : EVisibility::Hidden;
}

void SAnimCurvePanel::ToggleAllCurveModes(ESlateCheckBoxState::Type NewState, EAnimCurveFlags ModeToSet)
{
	const ESlateCheckBoxState::Type CurrentAllState = AreAllCurvesOfMode(ModeToSet);
	for( auto It=Tracks.CreateIterator(); It; ++It )
	{
		TSharedPtr<SCurveEdTrack> TrackWidget = It->Value.Pin();
		if( TrackWidget.IsValid() )
		{
			const ESlateCheckBoxState::Type CurrentTrackState = TrackWidget->IsCurveOfMode(ModeToSet);
			if( (CurrentAllState == CurrentTrackState) || ((CurrentAllState == ESlateCheckBoxState::Undetermined) && (CurrentTrackState == ESlateCheckBoxState::Unchecked)) )
			{
				TrackWidget->ToggleCurveMode( NewState, ModeToSet );
			}
		}
	}
}

ESlateCheckBoxState::Type SAnimCurvePanel::AreAllCurvesOfMode(EAnimCurveFlags ModeToSet) const
{
	int32 NumChecked = 0;
	for( auto It=Tracks.CreateConstIterator(); It; ++It )
	{
		TSharedPtr<SCurveEdTrack> TrackWidget = It->Value.Pin();
		if( TrackWidget.IsValid() )
		{
			if( TrackWidget->IsCurveOfMode(ModeToSet) )
			{
				NumChecked++;
			}
		}
	}
	if( NumChecked == Tracks.Num() )
	{
		return ESlateCheckBoxState::Checked;
	}
	else if( NumChecked == 0 )
	{
		return ESlateCheckBoxState::Unchecked;
	}
	return ESlateCheckBoxState::Undetermined;
}

void SAnimCurvePanel::UpdatePanel(bool bClearAll/*=false*/)
{
	if(Sequence != NULL)
	{
		// see if we need to clear or not
		FChildren * Children = PanelSlot->GetChildren();

		int32 TotalCurve = Sequence->RawCurveData.FloatCurves.Num();
		int32 CurveEditor = Children->Num();
		int32 CurrentIt = 0;

		// a curve is removed, refresh all window
		if (bClearAll || (CurveEditor > TotalCurve))
		{
			for (int32 Id=Children->Num()-1; Id>=0; --Id)
			{
				PanelSlot->RemoveAt(Id);
			}

			// Clear all tracks as we're re-adding them all anyway.
			Tracks.Empty();

			// refresh from 0 index
			CurrentIt = 0;
		}
		else
		{
			// otherwise start from new curve, 
			// this way, we don't destroy old curves
			CurrentIt = CurveEditor;
		}

		// Updating new tracks
		for (; CurrentIt<TotalCurve; ++CurrentIt)
		{
			FFloatCurve&  Curve = Sequence->RawCurveData.FloatCurves[CurrentIt];

			// if editable, add to the list
			if (Curve.GetCurveTypeFlag(ACF_Editable))
			{
				TSharedPtr<SCurveEdTrack> CurrentTrack;
				PanelSlot->AddSlot()
				.SizeRule(SSplitter::SizeToContent)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Center)
					[
						SAssignNew(CurrentTrack, SCurveEdTrack)
						.Sequence(Sequence)
						.CurveName(Curve.CurveName)
						.AnimCurvePanel(SharedThis(this))
						.WidgetWidth(WidgetWidth)
						.ViewInputMin(ViewInputMin)
						.ViewInputMax(ViewInputMax)
						.OnGetScrubValue(OnGetScrubValue)
						.OnSetInputViewRange(OnSetInputViewRange)
					]
				];
				Tracks.Add( Curve.CurveName.ToString(), CurrentTrack );
			}
		}
	}
}

void SAnimCurvePanel::SetSequence(class UAnimSequenceBase *	InSequence)
{
	if (InSequence != Sequence)
	{
		Sequence = InSequence;
		UpdatePanel(true);
	}
}

TSharedRef<SWidget> SAnimCurvePanel::GenerateCurveList()
{
	TSharedPtr<SVerticalBox> MainBox, ListBox;
	TSharedRef<SWidget> NewWidget = SAssignNew(MainBox, SVerticalBox);

	if ( Sequence && Sequence->RawCurveData.FloatCurves.Num() > 0)
	{
		MainBox->AddSlot()
			.AutoHeight()
			.MaxHeight(300)
			[
				SNew( SScrollBox )
				+SScrollBox::Slot() 
				[
					SAssignNew(ListBox, SVerticalBox)
				]
			];

		for (auto Iter=Sequence->RawCurveData.FloatCurves.CreateConstIterator(); Iter; ++Iter)
		{
			const FFloatCurve& Curve= *Iter;

			ListBox->AddSlot()
				.AutoHeight()
				.VAlign(VAlign_Center)
				.Padding( 2.0f, 2.0f )
				[
					SNew( SCheckBox )
					.IsChecked(this, &SAnimCurvePanel::IsCurveEditable, Curve.CurveName)
					.OnCheckStateChanged(this, &SAnimCurvePanel::ToggleEditability, Curve.CurveName)
					.ToolTipText( LOCTEXT("Show Curves", "Show or Hide Curves") )
					.IsEnabled( true )
					[
						SNew( STextBlock )
						.Text(Curve.CurveName.ToString())
					]
				];
		}

		MainBox->AddSlot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding( 2.0f, 2.0f )
			[
				SNew( SButton )
				.VAlign( VAlign_Center )
				.HAlign( HAlign_Center )
				.OnClicked( this, &SAnimCurvePanel::RefreshPanel )
				[
					SNew( STextBlock )
					.Text( LOCTEXT("RefreshCurve", "Refresh") )
				]
			];

		MainBox->AddSlot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding( 2.0f, 2.0f )
			[
				SNew( SButton )
				.VAlign( VAlign_Center )
				.HAlign( HAlign_Center )
				.OnClicked( this, &SAnimCurvePanel::ShowAll, true )
				[
					SNew( STextBlock )
					.Text( LOCTEXT("ShowAll", "Show All") )
				]
			];

		MainBox->AddSlot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding( 2.0f, 2.0f )
			[
				SNew( SButton )
				.VAlign( VAlign_Center )
				.HAlign( HAlign_Center )
				.OnClicked( this, &SAnimCurvePanel::ShowAll, false )
				[
					SNew( STextBlock )
					.Text( LOCTEXT("HideAll", "Hide All") )
				]
			];
	}
	else
	{
		MainBox->AddSlot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding( 2.0f, 2.0f )
			[
				SNew( STextBlock )
				.Text(LOCTEXT("Not Available", "No curve exists"))
			];
	}

	return NewWidget;
}

ESlateCheckBoxState::Type SAnimCurvePanel::IsCurveEditable(FName CurveName) const
{
	if ( Sequence )
	{
		const FFloatCurve * Curve = Sequence->RawCurveData.GetCurveData(CurveName);
		if ( Curve )
		{
			return Curve->GetCurveTypeFlag(ACF_Editable)? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
		}
	}

	return ESlateCheckBoxState::Undetermined;
}

void SAnimCurvePanel::ToggleEditability(ESlateCheckBoxState::Type NewType, FName CurveName)
{
	bool bEdit = (NewType == ESlateCheckBoxState::Checked);

	if ( Sequence )
	{
		FFloatCurve * Curve = Sequence->RawCurveData.GetCurveData(CurveName);
		if ( Curve )
		{
			Curve->SetCurveTypeFlag(ACF_Editable, bEdit);
		}
	}
}

FReply		SAnimCurvePanel::RefreshPanel()
{
	UpdatePanel(true);
	return FReply::Handled();
}

FReply		SAnimCurvePanel::ShowAll(bool bShow)
{
	if ( Sequence )
	{
		for (auto Iter = Sequence->RawCurveData.FloatCurves.CreateIterator(); Iter; ++Iter)
		{
			FFloatCurve & Curve = *Iter;
			Curve.SetCurveTypeFlag(ACF_Editable, bShow);
		}

		UpdatePanel(true);
	}

	return FReply::Handled();
}
#undef LOCTEXT_NAMESPACE
