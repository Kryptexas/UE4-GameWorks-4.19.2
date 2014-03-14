// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "SCurveEditor.h"
#include "ScopedTransaction.h"
#include "SColorGradientEditor.h"

#define LOCTEXT_NAMESPACE "SCurveEditor"

const static FVector2D	CONST_KeySize		= FVector2D(10,10);
const static FVector2D	CONST_CurveSize		= FVector2D(12,12);

const static float		CONST_FitMargin		= 0.05f;
const static float		CONST_MinViewRange	= 0.01f;
const static float		CONST_DefaultZoomRange	= 1.0f;
const static float      CONST_KeyTangentOffset = 20.0f;


//////////////////////////////////////////////////////////////////////////
// SCurveEditor

void SCurveEditor::Construct(const FArguments& InArgs)
{
	CurveFactory = NULL;
	Commands = TSharedPtr< FUICommandList >(new FUICommandList);
	CurveOwner = NULL;

	// view input
	ViewMinInput = InArgs._ViewMinInput;
	ViewMaxInput = InArgs._ViewMaxInput;
	// data input - only used when it's set
	DataMinInput = InArgs._DataMinInput;
	DataMaxInput = InArgs._DataMaxInput;

	ViewMinOutput = InArgs._ViewMinOutput;
	ViewMaxOutput = InArgs._ViewMaxOutput;

	bZoomToFit = InArgs._ZoomToFit;
	DesiredSize = InArgs._DesiredSize;

	// if editor size is set, use it, otherwise, use default value
	if (DesiredSize.Get().IsZero())
	{
		DesiredSize.Set(FVector2D(128, 64));
	}

	TimelineLength = InArgs._TimelineLength;
	SetInputViewRangeHandler = InArgs._OnSetInputViewRange;
	bDrawCurve = InArgs._DrawCurve;
	bHideUI = InArgs._HideUI;
	bAllowZoomOutput = InArgs._AllowZoomOutput;
	bAlwaysDisplayColorCurves = InArgs._AlwaysDisplayColorCurves;

	OnCreateAsset = InArgs._OnCreateAsset;

	bMovingKeys = false;
	bPanning = false;
	bMovingTangent = false;

	//Simple r/g/b for now
	CurveColors.Add(FLinearColor(1.0f,0.0f,0.0f));
	CurveColors.Add(FLinearColor(0.0f,1.0f,0.0f));
	CurveColors.Add(FLinearColor(0.0f,0.0f,1.0f));

	TransactionIndex = -1;

	Commands->MapAction( FGenericCommands::Get().Undo,
		FExecuteAction::CreateSP( this, &SCurveEditor::UndoAction ));

	Commands->MapAction( FGenericCommands::Get().Redo,
		FExecuteAction::CreateSP( this, &SCurveEditor::RedoAction ));

	SAssignNew(WarningMessageText, SErrorText );

	auto CurveSelector	=	
		SNew(SBorder)
		.BorderImage( FEditorStyle::GetBrush("NoBorder") )
		.Visibility(this, &SCurveEditor::GetControlVisibility)
		[
			CreateCurveSelectionWidget()
		];

	CurveSelectionWidget = CurveSelector;

	this->ChildSlot
	[
		SNew( SVerticalBox )
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SHorizontalBox)
			.Visibility( this, &SCurveEditor::GetCurveAreaVisibility )
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				CurveSelector
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBorder)
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Left)
				.BorderImage( FEditorStyle::GetBrush("NoBorder") )
				.DesiredSizeScale(FVector2D(256.0f,32.0f))
				.Visibility(this, &SCurveEditor::GetControlVisibility)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ToolTipText(LOCTEXT("ZoomToFitHorizontal", "Zoom To Fit Horizontal"))
						.OnClicked(this, &SCurveEditor::ZoomToFitHorizontal)
						.ContentPadding(1)
						[
							SNew(SImage) 
							.Image( FEditorStyle::GetBrush("CurveEd.FitHorizontal") ) 
							.ColorAndOpacity( FSlateColor::UseForeground() ) 
						]
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ToolTipText(LOCTEXT("ZoomToFitVertical", "Zoom To Fit Vertical"))
						.OnClicked(this, &SCurveEditor::ZoomToFitVertical)
						.ContentPadding(1)
						[
							SNew(SImage) 
							.Image( FEditorStyle::GetBrush("CurveEd.FitVertical") ) 
							.ColorAndOpacity( FSlateColor::UseForeground() ) 
						]
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBorder)
						.Visibility(this, &SCurveEditor::GetEditVisibility)
						.VAlign(VAlign_Center)
						[
							SNew(SEditableText)
							.MinDesiredWidth(64.0f)
							.IsReadOnly(true)
							.IsEnabled(true)
							.Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
							.ColorAndOpacity( FLinearColor::White )
							.Text(this, &SCurveEditor::GetKeyInfoText)
						]
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBorder)
						.Visibility(this, &SCurveEditor::GetEditVisibility)
						.VAlign(VAlign_Center)
						[
							SNew(SEditableText)
							.MinDesiredWidth(64.0f)
							.IsEnabled(this, &SCurveEditor::GetInputEditEnabled)
							.Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
							.ColorAndOpacity( FLinearColor::White )
							.SelectAllTextWhenFocused(true)
							.Text(this, &SCurveEditor::GetInputInfoText)
							.OnTextCommitted(this, &SCurveEditor::NewTimeEntered)
						]
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBorder)
						.Visibility(this, &SCurveEditor::GetEditVisibility)
						.VAlign(VAlign_Center)
						[
							SNew(SEditableText)
							.MinDesiredWidth(64.0f)
							.Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
							.ColorAndOpacity( FLinearColor::White )
							.SelectAllTextWhenFocused(true)
							.Text(this, &SCurveEditor::GetOutputInfoText)
							.OnTextCommitted(this, &SCurveEditor::NewValueEntered)
						]
					]
				]
			]
		]
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Bottom)
		.FillHeight(.75f)
		[
			SNew( SBorder )
			.Visibility( this, &SCurveEditor::GetColorGradientVisibility )
			.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
			.BorderBackgroundColor( FLinearColor( .8f, .8f, .8f, .60f ) )
			.Padding(1.0f)
			[
				SAssignNew( GradientViewer, SColorGradientEditor )
				.ViewMinInput( ViewMinInput )
				.ViewMaxInput( ViewMaxInput )
				.IsEditingEnabled( this, &SCurveEditor::IsEditingEnabled )
			]
		]
	];

	if (GEditor != NULL)
	{
		GEditor->RegisterForUndo(this);
	}
}

SCurveEditor::~SCurveEditor()
{
	if (GEditor != NULL)
	{
		GEditor->UnregisterForUndo(this);
	}
}

TSharedRef<SWidget> SCurveEditor::CreateCurveSelectionWidget() const
{
	auto Box =  SNew(SVerticalBox);
	//Curve selection buttons only useful if you have more then 1 curve
	if(Curves.Num() > 1)
	{
		for(auto It = Curves.CreateConstIterator();It;++It)
		{
			FRichCurve* Curve = It->CurveToEdit;

			Box->AddSlot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.ForegroundColor(GetCurveColor(Curve))
					.IsChecked(this, &SCurveEditor::GetCheckState, Curve)
					.OnCheckStateChanged(this, &SCurveEditor::OnUserSelectedCurve, Curve)
					.ToolTipText(this, &SCurveEditor::GetCurveCheckBoxToolTip, Curve)
				]		
			];
		}
	}

	return Box;
}

void SCurveEditor::PushWarningMenu( FVector2D Position, const FText& Message )
{
	WarningMessageText->SetError(Message);

	FSlateApplication::Get().PushMenu(
		SharedThis( this ),
		WarningMessageText->AsWidget(),
		Position,
		FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu));
}


void SCurveEditor::PushInterpolationMenu(FVector2D Position)
{
	FText Heading = (SelectedKeys.Num() > 0) ? FText::Format( LOCTEXT("KeyInterpolationMode", "Key Interpolation({0})"), GetInterpolationModeText()) : LOCTEXT("CurveInterpolationMode", "Curve Interpolation");

	FMenuBuilder MenuBuilder(true, NULL);
	MenuBuilder.BeginSection("CurveEditorInterpolation", Heading);
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("InterpolateLinearMode", "Linear"),
			LOCTEXT("LinearInterpolationToolTip", "Use Linear interpolation"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SCurveEditor::OnSelectInterpolationMode, RCIM_Linear, RCTM_Auto) ,
			FCanExecuteAction()));

		MenuBuilder.AddMenuEntry(
			LOCTEXT("InterpolateClampMode", "Clamp"),
			LOCTEXT("ClampInterpolationToolTip", "Clamp to value"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SCurveEditor::OnSelectInterpolationMode, RCIM_Constant, RCTM_Auto) ,
			FCanExecuteAction()));

		MenuBuilder.AddMenuEntry(
			LOCTEXT("InterpolateCubicAutoMode", "Cubic-auto"),
			LOCTEXT("CubicAutoInterpolationToolTip", "Cubic interpolation with automatic tangents"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SCurveEditor::OnSelectInterpolationMode, RCIM_Cubic, RCTM_Auto) ,
			FCanExecuteAction()));

		MenuBuilder.AddMenuEntry(
			LOCTEXT("InterpolateCubicLockedMode", "Cubic-locked"),
			LOCTEXT("CubicLockedInterpolationToolTip", "Cubic interpolation, with user controlled tangent"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SCurveEditor::OnSelectInterpolationMode, RCIM_Cubic, RCTM_User) ,
			FCanExecuteAction()));

		MenuBuilder.AddMenuEntry(
			LOCTEXT("InterpolateCubicBreakMode", "Cubic-break"),
			LOCTEXT("CubicBreakInterpolationToolTip", "Cubic interpolation with seperate user controlled tangents for arrive and leave"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SCurveEditor::OnSelectInterpolationMode, RCIM_Cubic, RCTM_Break) ,
			FCanExecuteAction()));
	}
	MenuBuilder.EndSection(); //CurveEditorInterpolation

	FSlateApplication::Get().PushMenu(
		SharedThis( this ),
		MenuBuilder.MakeWidget(),
		Position,
		FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu));
}


FVector2D SCurveEditor::ComputeDesiredSize() const
{
	return DesiredSize.Get();
}

EVisibility SCurveEditor::GetCurveAreaVisibility() const
{
	return AreCurvesVisible() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SCurveEditor::GetControlVisibility() const
{
	return (IsHovered() || (false == bHideUI)) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SCurveEditor::GetEditVisibility() const
{
	return (SelectedKeys.Num() > 0) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SCurveEditor::GetColorGradientVisibility() const
{
	return bIsGradientEditorVisible && IsLinearColorCurve() ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SCurveEditor::GetInputEditEnabled() const
{
	return (SelectedKeys.Num() == 1);
}

int32 SCurveEditor::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// Rendering info
	bool bEnabled = ShouldBeEnabled( bParentEnabled );
	ESlateDrawEffect::Type DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	const FSlateBrush* TimelineAreaBrush = FEditorStyle::GetBrush("CurveEd.TimelineArea");
	const FSlateBrush* WhiteBrush = FEditorStyle::GetBrush("WhiteTexture");

	FGeometry CurveAreaGeometry = AllottedGeometry;

	// Positioning info
	FTrackScaleInfo ScaleInfo(ViewMinInput.Get(),  ViewMaxInput.Get(), ViewMinOutput, ViewMaxOutput, CurveAreaGeometry.Size);

	// Draw background to indicate valid timeline area
	float ZeroInputX = ScaleInfo.InputToLocalX(0.f);
	float ZeroOutputY = ScaleInfo.OutputToLocalY(0.f);


	// time=0 line
	{
		FSlateDrawElement::MakeBox
		(
			OutDrawElements,
			LayerId,
			CurveAreaGeometry.ToPaintGeometry( FVector2D(ZeroInputX, 0), FVector2D(1, CurveAreaGeometry.Size.Y) ),
			WhiteBrush,
			MyClippingRect,
			DrawEffects,
			WhiteBrush->GetTint( InWidgetStyle ) * InWidgetStyle.GetColorAndOpacityTint()
		);
	}
	

	if( AreCurvesVisible() )
	{
		// value=0 line
		FSlateDrawElement::MakeBox
		(
			OutDrawElements,
			LayerId,
			CurveAreaGeometry.ToPaintGeometry( FVector2D(0, ZeroOutputY), FVector2D(CurveAreaGeometry.Size.X, 1) ),
			WhiteBrush,
			MyClippingRect,
			DrawEffects,
			WhiteBrush->GetTint( InWidgetStyle ) * InWidgetStyle.GetColorAndOpacityTint()
		);
	}

	// timeline background
	float TimelineMaxX = ScaleInfo.InputToLocalX(TimelineLength.Get());

	FSlateDrawElement::MakeBox
	(
		OutDrawElements,
		LayerId,
		CurveAreaGeometry.ToPaintGeometry( FVector2D(ZeroInputX, 0), FVector2D(TimelineMaxX-ZeroInputX, CurveAreaGeometry.Size.Y) ),
		TimelineAreaBrush,
		MyClippingRect,
		DrawEffects,
		TimelineAreaBrush->GetTint( InWidgetStyle ) * InWidgetStyle.GetColorAndOpacityTint()
	);


	LayerId++; // Ensure we draw line on top of background

	if( AreCurvesVisible() )
	{
		{
			const int32 SelectedCurveLayer = LayerId;
			const int32 UnSelectedCurveLayer = LayerId+1;

			//Paint the curves, any selected curves will be on top
			for(auto It = Curves.CreateConstIterator();It;++It)
			{
				FRichCurve* Curve = It->CurveToEdit;
				int32 LayerToUse = (SelectedCurves.Find(Curve) != INDEX_NONE) ? SelectedCurveLayer : UnSelectedCurveLayer;
				{
					PaintCurve(Curve, CurveAreaGeometry, ScaleInfo, OutDrawElements, LayerToUse, MyClippingRect, DrawEffects, InWidgetStyle);
				}
			}
			LayerId = LayerId+2;
		}

		//Paint the keys on top of the curve
		for(auto It = Curves.CreateConstIterator();It;++It)
		{
			FRichCurve* Curve = It->CurveToEdit;
			LayerId = PaintKeys(Curve, ScaleInfo, OutDrawElements, LayerId, CurveAreaGeometry, MyClippingRect, DrawEffects, InWidgetStyle);
		}
	}

	// Paint children
	SCompoundWidget::OnPaint(CurveAreaGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	PaintGridLines(CurveAreaGeometry, ScaleInfo, OutDrawElements, LayerId++, MyClippingRect, DrawEffects);

	return LayerId;
}

void SCurveEditor::PaintCurve(  FRichCurve* Curve, const FGeometry &AllottedGeometry, FTrackScaleInfo &ScaleInfo, FSlateWindowElementList &OutDrawElements, 
	int32 LayerId, const FSlateRect& MyClippingRect, ESlateDrawEffect::Type DrawEffects, const FWidgetStyle &InWidgetStyle )const
{
	if(Curve != NULL)
	{
		if(bDrawCurve)
		{
			FLinearColor Color = InWidgetStyle.GetColorAndOpacityTint() * GetCurveColor(Curve);

			//If there are multiple curves, alpha fade out non-selected curves
			if(Curves.Num() > 1 && SelectedCurves.Num() > 0 && INDEX_NONE == SelectedCurves.Find(Curve))
			{
				Color *= FLinearColor(1.0f,1.0f,1.0f,0.35f); 
			}

			TArray<FVector2D> LinePoints;
			int32 CurveDrawInterval = 1;

			if(Curve->GetNumKeys() < 2) 
			{
				//Not enough point, just draw flat line
				float Value = Curve->Eval(0.0f);
				float Y = ScaleInfo.OutputToLocalY(Value);
				LinePoints.Add(FVector2D(0.0f, Y));
				LinePoints.Add(FVector2D(AllottedGeometry.Size.X, Y));

				FSlateDrawElement::MakeLines(OutDrawElements,LayerId,AllottedGeometry.ToPaintGeometry(),LinePoints,MyClippingRect,DrawEffects,Color);
				LinePoints.Empty();
			}
			else
			{
				//Add arrive and exit lines
				{
					const FRichCurveKey& FirstKey = Curve->GetFirstKey();
					const FRichCurveKey& LastKey = Curve->GetLastKey();

					float ArriveX = ScaleInfo.InputToLocalX(FirstKey.Time);
					float ArriveY = ScaleInfo.OutputToLocalY(FirstKey.Value);
					float LeaveY  = ScaleInfo.OutputToLocalY(LastKey.Value);
					float LeaveX  = ScaleInfo.InputToLocalX(LastKey.Time);

					//Arrival line
					LinePoints.Add(FVector2D(0.0f, ArriveY));
					LinePoints.Add(FVector2D(ArriveX, ArriveY));
					FSlateDrawElement::MakeLines(OutDrawElements,LayerId,AllottedGeometry.ToPaintGeometry(),LinePoints,MyClippingRect,DrawEffects,Color);
					LinePoints.Empty();

					//Leave line
					LinePoints.Add(FVector2D(AllottedGeometry.Size.X, LeaveY));
					LinePoints.Add(FVector2D(LeaveX, LeaveY));
					FSlateDrawElement::MakeLines(OutDrawElements,LayerId,AllottedGeometry.ToPaintGeometry(),LinePoints,MyClippingRect,DrawEffects,Color);
					LinePoints.Empty();
				}


				//Add enclosed segments
				TArray<FRichCurveKey> Keys = Curve->GetCopyOfKeys();
				for(int32 i = 0;i<Keys.Num()-1;++i)
				{
					CreateLinesForSegment(Curve, Keys[i], Keys[i+1],LinePoints, ScaleInfo);
					FSlateDrawElement::MakeLines(OutDrawElements,LayerId,AllottedGeometry.ToPaintGeometry(),LinePoints,MyClippingRect,DrawEffects,Color);
					LinePoints.Empty();
				}
			}
		}
	}
}


void SCurveEditor::CreateLinesForSegment( FRichCurve* Curve, const FRichCurveKey& Key1, const FRichCurveKey& Key2, TArray<FVector2D>& Points, FTrackScaleInfo &ScaleInfo ) const
{
	switch(Key1.InterpMode)
	{
	case RCIM_Constant:
		{
			//@todo: should really only need 3 points here but something about the line rendering isn't quite behaving as I'd expect, so need extras
			Points.Add(FVector2D(Key1.Time, Key1.Value));
			Points.Add(FVector2D(Key2.Time, Key1.Value));
			Points.Add(FVector2D(Key2.Time, Key1.Value));
			Points.Add(FVector2D(Key2.Time, Key2.Value));
			Points.Add(FVector2D(Key2.Time, Key1.Value));
		}break;
	case RCIM_Linear:
		{
			Points.Add(FVector2D(Key1.Time, Key1.Value));
			Points.Add(FVector2D(Key2.Time, Key2.Value));
		}break;
	case RCIM_Cubic:
		{
			const float StepSize = 1.0f;
			//clamp to screen to avoid massive slowdown when zoomed in
			float StartX	= FMath::Max(ScaleInfo.InputToLocalX(Key1.Time), 0.0f) ;
			float EndX		= FMath::Min(ScaleInfo.InputToLocalX(Key2.Time),ScaleInfo.WidgetSize.X);
			for(;StartX<EndX; StartX += StepSize)
			{
				float CurveIn = ScaleInfo.LocalXToInput(FMath::Min(StartX,EndX));
				float CurveOut = Curve->Eval(CurveIn);
				Points.Add(FVector2D(CurveIn,CurveOut));
			}
			Points.Add(FVector2D(Key2.Time,Key2.Value));

		}break;
	}

	//Transform to screen
	for(auto It = Points.CreateIterator();It;++It)
	{
		FVector2D Vec2D = *It;
		Vec2D.X = ScaleInfo.InputToLocalX(Vec2D.X);
		Vec2D.Y = ScaleInfo.OutputToLocalY(Vec2D.Y);
		*It = Vec2D;
	}
}

int32 SCurveEditor::PaintKeys( FRichCurve* Curve, FTrackScaleInfo &ScaleInfo, FSlateWindowElementList &OutDrawElements, int32 LayerId, const FGeometry &AllottedGeometry, const FSlateRect& MyClippingRect, ESlateDrawEffect::Type DrawEffects, const FWidgetStyle &InWidgetStyle ) const
{
	const int32 UnSelectedLayerID = LayerId;
	const int32 SelectedLayerID = LayerId+1; //selected keys should be on top

	FLinearColor KeyColor = IsEditingEnabled() ? InWidgetStyle.GetColorAndOpacityTint() : FLinearColor(0.1f,0.1f,0.1f,1.f);
	// Iterate over each key
	ERichCurveInterpMode LastInterpMode = RCIM_Linear;
	for (auto It(Curve->GetKeyHandleIterator()); It; ++It)
	{
		FKeyHandle KeyHandle = It.Key();

		// Work out where it is
		const float KeyX = ScaleInfo.InputToLocalX( Curve->GetKeyTime(KeyHandle) ) - CONST_KeySize.X/2;
		const float KeyY = ScaleInfo.OutputToLocalY( Curve->GetKeyValue(KeyHandle) ) - CONST_KeySize.Y/2;

		// Get brush
		bool IsSelected = IsKeySelected(FSelectedCurveKey(Curve,KeyHandle));
		const FSlateBrush* KeyBrush = IsSelected ? FEditorStyle::GetBrush("CurveEd.CurveKeySelected") : FEditorStyle::GetBrush("CurveEd.CurveKey");
		int32 LayerToUse	=  IsSelected ? SelectedLayerID: UnSelectedLayerID;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerToUse,
			AllottedGeometry.ToPaintGeometry( FVector2D(KeyX, KeyY), CONST_KeySize ),
			KeyBrush,
			MyClippingRect,
			DrawEffects,
			KeyBrush->GetTint( InWidgetStyle ) * InWidgetStyle.GetColorAndOpacityTint() * KeyColor
			);

		//Handle drawing the tangent controls for curve
		if(IsSelected && (Curve->GetKeyInterpMode(KeyHandle) == RCIM_Cubic || LastInterpMode == RCIM_Cubic))
		{
			PaintTangent(ScaleInfo, Curve, KeyHandle, KeyX, KeyY, OutDrawElements, LayerId, AllottedGeometry, MyClippingRect, DrawEffects, LayerToUse, InWidgetStyle);
		}

		LastInterpMode = Curve->GetKeyInterpMode(KeyHandle);
	}
	return LayerId+2;
}


void SCurveEditor::PaintTangent( FTrackScaleInfo &ScaleInfo, FRichCurve* Curve, FKeyHandle KeyHandle, const float KeyX, const float KeyY, FSlateWindowElementList &OutDrawElements, int32 LayerId, const FGeometry &AllottedGeometry, const FSlateRect& MyClippingRect, ESlateDrawEffect::Type DrawEffects, int32 LayerToUse, const FWidgetStyle &InWidgetStyle ) const
{
	FVector2D ArriveTangentPos, LeaveTangentPos;
	GetTangentPoints(ScaleInfo, FSelectedCurveKey(Curve,KeyHandle), ArriveTangentPos, LeaveTangentPos);

	FVector2D OffsetKeySize(CONST_KeySize.X,CONST_KeySize.Y);
	OffsetKeySize *= 0.5f;

	ArriveTangentPos -= OffsetKeySize;
	LeaveTangentPos  -= OffsetKeySize;

	const FSlateBrush* TangentBrush = FEditorStyle::GetBrush("CurveEd.Tangent");

	FVector2D KeyOffset(CONST_KeySize.X*0.5f, CONST_KeySize.Y*0.5f);

	//Add lines from tangent control point to 'key'
	TArray<FVector2D> LinePoints;
	LinePoints.Add(FVector2D(KeyX, KeyY)+KeyOffset );
	LinePoints.Add(ArriveTangentPos+KeyOffset);
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		LinePoints,
		MyClippingRect,
		DrawEffects,
		FLinearColor(0.0f,0.66f,0.7f)
		);

	LinePoints.Empty();
	LinePoints.Add(FVector2D(KeyX, KeyY)+KeyOffset);
	LinePoints.Add(LeaveTangentPos+KeyOffset);
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		LinePoints,
		MyClippingRect,
		DrawEffects,
		FLinearColor(0.0f,0.66f,0.7f)
		);

	//Arrive tangent control
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerToUse,
		AllottedGeometry.ToPaintGeometry(ArriveTangentPos, CONST_KeySize ),
		TangentBrush,
		MyClippingRect,
		DrawEffects,
		TangentBrush->GetTint( InWidgetStyle ) * InWidgetStyle.GetColorAndOpacityTint()
		);
	//Leave tangent control
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerToUse,
		AllottedGeometry.ToPaintGeometry(LeaveTangentPos, CONST_KeySize ),
		TangentBrush,
		MyClippingRect,
		DrawEffects,
		TangentBrush->GetTint( InWidgetStyle ) * InWidgetStyle.GetColorAndOpacityTint()
		);
}


float SCurveEditor::CalcGridLineStepDistancePow2(double RawValue)
{
	return float(double(FMath::RoundUpToPowerOfTwo(uint32(RawValue*1024.0))>>1)/1024.0);
}

float SCurveEditor::GetTimeStep(FTrackScaleInfo &ScaleInfo) const
{
	const float MaxGridPixelSpacing = 150.0f;

	const float GridPixelSpacing = FMath::Min(ScaleInfo.WidgetSize.GetMin()/1.5f, MaxGridPixelSpacing);

	double MaxTimeStep = ScaleInfo.LocalXToInput(ViewMinInput.Get() + GridPixelSpacing) - ScaleInfo.LocalXToInput(ViewMinInput.Get());

	return CalcGridLineStepDistancePow2(MaxTimeStep);
}

void SCurveEditor::PaintGridLines(const FGeometry &AllottedGeometry, FTrackScaleInfo &ScaleInfo, FSlateWindowElementList &OutDrawElements, 
	int32 LayerId, const FSlateRect& MyClippingRect, ESlateDrawEffect::Type DrawEffects )const
{
	const float MaxGridPixelSpacing = 150.0f;

	const float GridPixelSpacing = FMath::Min(ScaleInfo.WidgetSize.GetMin()/1.5f, MaxGridPixelSpacing);

	const FLinearColor GridColor = FLinearColor(0.0f,0.0f,0.0f, 0.25f) ;
	const FLinearColor GridTextColor = FLinearColor(1.0f,1.0f,1.0f, 0.75f) ;

	//Vertical grid(time)
	{
		float TimeStep = GetTimeStep(ScaleInfo);
		float ScreenStepTime = ScaleInfo.InputToLocalX(TimeStep) -  ScaleInfo.InputToLocalX(0.0f);

		if(ScreenStepTime >= 1.0f)
		{
			float StartTime = ScaleInfo.LocalXToInput(0.0f);
			TArray<FVector2D> LinePoints;
			float ScaleX = (TimeStep)/(AllottedGeometry.Size.X);

			//draw vertical grid lines
			float StartOffset = -FMath::Fractional(StartTime / TimeStep)*ScreenStepTime;
			float Time =  ScaleInfo.LocalXToInput(StartOffset);
			for(float X = StartOffset;X< AllottedGeometry.Size.X;X+= ScreenStepTime, Time += TimeStep)
			{
				if(SMALL_NUMBER < FMath::Abs(X)) //don't show at 0 to avoid overlapping with center axis 
				{
					LinePoints.Add(FVector2D(X, 0.0));
					LinePoints.Add(FVector2D(X, AllottedGeometry.Size.Y));
					FSlateDrawElement::MakeLines(OutDrawElements,LayerId,AllottedGeometry.ToPaintGeometry(),LinePoints,MyClippingRect,DrawEffects,GridColor);

					//Show grid time
					{
						FString TimeStr = FString::Printf(TEXT("%.2f"), Time);
						FSlateDrawElement::MakeText(OutDrawElements,LayerId,AllottedGeometry.MakeChild(FVector2D(X, 0.0), FVector2D(1.0f, ScaleX )).ToPaintGeometry(),TimeStr,
							FEditorStyle::GetFontStyle("CurveEd.InfoFont"), MyClippingRect, DrawEffects, GridTextColor );
					}

					LinePoints.Empty();
				}
			}
		}
	}

	//Horizontal grid(values)
	// This is only useful if the curves are visible
	if( AreCurvesVisible() )
	{
		double MaxValueStep = ScaleInfo.LocalYToOutput(0) - ScaleInfo.LocalYToOutput(GridPixelSpacing) ;
		float ValueStep = CalcGridLineStepDistancePow2(MaxValueStep);
		float ScreenStepValue = ScaleInfo.OutputToLocalY(0.0f) - ScaleInfo.OutputToLocalY(ValueStep);  
		if(ScreenStepValue >= 1.0f)
		{
			float StartValue = ScaleInfo.LocalYToOutput(0.0f);
			TArray<FVector2D> LinePoints;

			float StartOffset = FMath::Fractional(StartValue / ValueStep)*ScreenStepValue;
			float Value = ScaleInfo.LocalYToOutput(StartOffset);
			float ScaleY = (ValueStep)/(AllottedGeometry.Size.Y);

			for(float Y = StartOffset;Y< AllottedGeometry.Size.Y;Y+= ScreenStepValue, Value-=ValueStep)
			{
				if(SMALL_NUMBER < FMath::Abs(Y)) //don't show at 0 to avoid overlapping with center axis 
				{
					LinePoints.Add(FVector2D(0.0f, Y));
					LinePoints.Add(FVector2D(AllottedGeometry.Size.X,Y));
					FSlateDrawElement::MakeLines(OutDrawElements,LayerId,AllottedGeometry.ToPaintGeometry(),LinePoints,MyClippingRect,DrawEffects,GridColor);

					//Show grid value
					{
						FString ValueStr = FString::Printf(TEXT("%.2f"), Value);
						FSlateFontInfo Font = FEditorStyle::GetFontStyle("CurveEd.InfoFont");

						const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
						FVector2D DrawSize = FontMeasureService->Measure(ValueStr, Font);

						// draw at the start
						FSlateDrawElement::MakeText(OutDrawElements,LayerId,AllottedGeometry.MakeChild(FVector2D(0.0f, Y), FVector2D(ScaleY, 1.0f )).ToPaintGeometry(),ValueStr,
												 Font, MyClippingRect, DrawEffects, GridTextColor );

						// draw at the last since sometimes start can be hidden
						FSlateDrawElement::MakeText(OutDrawElements,LayerId,AllottedGeometry.MakeChild(FVector2D(AllottedGeometry.Size.X-DrawSize.X, Y), FVector2D(ScaleY, 1.0f )).ToPaintGeometry(),ValueStr,
												 Font, MyClippingRect, DrawEffects, GridTextColor );
					}
					
					LinePoints.Empty();
				}
			}
		}
	}
}

void SCurveEditor::SetCurveOwner(FCurveOwnerInterface* InCurveOwner, bool bCanEdit)
{
	if(InCurveOwner != CurveOwner)
	{
		EmptySelection();
		SelectedCurves.Empty();
	}

	GradientViewer->SetCurveOwner(InCurveOwner);

	CurveOwner = InCurveOwner;
	bCanEditTrack = bCanEdit;


	bAreCurvesVisible = !IsLinearColorCurve();
	bIsGradientEditorVisible = IsLinearColorCurve();

	Curves.Empty();
	if(CurveOwner != NULL)
	{
		Curves = InCurveOwner->GetCurves();

		//Auto select first curve
		if(Curves.Num() > 0)
		{
			SelectCurve(Curves[0].CurveToEdit, false);
		}
		CurveOwner->MakeTransactional();
	}

	SelectedKeys.Empty();

	if( bZoomToFit )
	{
		ZoomToFitVertical();
		ZoomToFitHorizontal();
	}

	CurveSelectionWidget.Pin()->SetContent(CreateCurveSelectionWidget());
}

void SCurveEditor::SetZoomToFit(bool bNewZoomToFit)
{
	bZoomToFit = bNewZoomToFit;
}

FCurveOwnerInterface* SCurveEditor::GetCurveOwner() const
{
	return CurveOwner;
}

FRichCurve* SCurveEditor::GetCurve(int32 CurveIndex) const
{
	if(CurveIndex < Curves.Num())
	{
		return Curves[CurveIndex].CurveToEdit;
	}
	return NULL;
}

void SCurveEditor::DeleteSelectedKeys()
{
	// While there are still keys
	while(SelectedKeys.Num() > 0)
	{
		const FScopedTransaction Transaction( LOCTEXT("CurveEditor_RemoveKeys", "Delete Key(s)") );
		CurveOwner->ModifyOwner();

		// Pull one out of the selected set
		FSelectedCurveKey Key = SelectedKeys.Pop();
		if(IsValidCurve(Key.Curve))
		{
			// Remove from the curve
			Key.Curve->DeleteKey(Key.KeyHandle);
		}
	}
}


FReply SCurveEditor::OnMouseButtonDown( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	const bool bLeftMouseButton = InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
	const bool bRightMouseButton = InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
	const bool bControlDown = InMouseEvent.IsControlDown();
	const bool bShiftDown = InMouseEvent.IsShiftDown();

	// See if we hit a key
	FSelectedCurveKey HitKey = HitTestKeys(InMyGeometry, InMouseEvent.GetScreenSpacePosition() );

	//Turn off error msg
	//WarningMessageText->SetError(FString());

	if( bLeftMouseButton || bRightMouseButton )
	{
		bMovingTangent = false;
		// Hit a key
		if( IsEditingEnabled() && HitKey.IsValid())
		{
			if(bLeftMouseButton)
			{
				if(!IsKeySelected(HitKey))
				{
					// If control is down, add to current selection
					if(!bControlDown)
					{
						EmptySelection();
					}

					AddToSelection(HitKey);
				}
				else if(bControlDown)
				{
					RemoveFromSelection(HitKey);
				}

				// If th key we clicked on is now selected, start movement mode
				if(IsKeySelected(HitKey))
				{
					BeginDragTransaction();
					bMovingKeys = true;
				}
			}
			else
			{
				// Make sure key is selected in readyness for context menu
				if(!IsKeySelected(HitKey))
				{
					EmptySelection();
					AddToSelection(HitKey);
				}
				
				PushInterpolationMenu(InMouseEvent.GetScreenSpacePosition());
			}
		}
		// Hit background
		else
		{
			if( bLeftMouseButton && IsEditingEnabled() )
			{
				// shift click on background = add new key
				if(bShiftDown)
				{
					if(SelectedCurves.Num() == 0)
					{
						if(Curves.Num() == 1)
						{
							SelectCurve(Curves[0].CurveToEdit, false);
						}
						else
						{
							PushWarningMenu(InMouseEvent.GetScreenSpacePosition(), LOCTEXT("NoCurveSelected", "Please select a curve"));
						}						
					}
					
					for(auto It = SelectedCurves.CreateIterator();It;++It)
					{
						FRichCurve* SelectedCurve = *It;
						if(IsValidCurve(SelectedCurve))
						{
							const FScopedTransaction Transaction( LOCTEXT("CurveEditor_AddKey", "Add Key(s)") );
							CurveOwner->ModifyOwner();

							FTrackScaleInfo ScaleInfo(ViewMinInput.Get(),  ViewMaxInput.Get(), ViewMinOutput, ViewMaxOutput, InMyGeometry.Size);

							FVector2D LocalClickPos = InMyGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

							float NewInput = ScaleInfo.LocalXToInput( LocalClickPos.X );
							float NewOutput = ScaleInfo.LocalYToOutput( LocalClickPos.Y );
							FKeyHandle NewKeyHandle = SelectedCurve->AddKey(NewInput, NewOutput);

							EmptySelection();
							AddToSelection(FSelectedCurveKey(SelectedCurve,NewKeyHandle));
						}
					}
				}
				else
				{
					// clicking on background clears selection
					FSelectedTangent Tangent = HitTestCubicTangents(InMyGeometry, InMouseEvent.GetScreenSpacePosition());
					if(Tangent.IsValid())
					{
						BeginDragTransaction();
						SelectedTangent = Tangent;
						bMovingTangent = true;
					}
					else
					{
						// clicking on background clears key selection
						EmptySelection();

						//select any curve that is nearby
						if(Curves.Num() > 1)
						{
							HitTestCurves(InMyGeometry, InMouseEvent);
						}
					}
		
				}
			}
			else if(bRightMouseButton)
			{
				bPanning = true;
			}
		}

		// Set keyboard focus to this so that selected text box doesn't try to apply to newly selected keys
		if(!HasKeyboardFocus())
		{
			FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EKeyboardFocusCause::SetDirectly);
		}

		// Always capture mouse if we left or right click on the widget
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

void SCurveEditor::ResetDragValues()
{
	TotalDragDist = 0.f;
	bPanning = false;
	bMovingKeys = false;
	bMovingTangent = false;
}

void SCurveEditor::OnMouseCaptureLost()
{
	// if we began a drag transaction we need to finish it to make sure undo doesn't get out of sync
	if ( IsEditingEnabled() && ( bMovingTangent || bMovingKeys ) )
	{
		EndDragTransaction();

		ResetDragValues();
	}
}

FReply SCurveEditor::OnMouseButtonUp( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	const bool bLeftMouseButton = InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
	const bool bRightMouseButton = InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton;

	if( this->HasMouseCapture() && (bLeftMouseButton || bRightMouseButton) )
	{
		if( IsEditingEnabled() )
		{
			EndDragTransaction();
		}

		//Allow context menu to appear if drag distance is less than threshold distance.
		const float DragThresholdDist = 5.0f;
		if( bRightMouseButton && TotalDragDist < DragThresholdDist )
		{
			FVector2D ScreenPos = InMouseEvent.GetScreenSpacePosition();

			if( IsEditingEnabled() && SelectedKeys.Num() == 0 && HitTestCurves(InMyGeometry, InMouseEvent))
			{
				PushInterpolationMenu(InMouseEvent.GetScreenSpacePosition());
			}
			else if(
				!HitTestCubicTangents(InMyGeometry, ScreenPos).IsValid() && 
				!HitTestKeys(InMyGeometry,ScreenPos).IsValid())
			{
				CreateContextMenu();
			}
		}

		ResetDragValues();

		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

void ClampViewRangeToDataIfBound( float & NewViewMin, float & NewViewMax, const TAttribute< TOptional<float> > & DataMin, const TAttribute< TOptional<float> > & DataMax, const float ViewRange)
{
	// if we have data bound
	const TOptional<float> & Min = DataMin.Get();
	const TOptional<float> & Max = DataMax.Get();
	if ( Min.IsSet() && NewViewMin < Min.GetValue())
	{
		// if we have min data set
		NewViewMin = Min.GetValue();
		NewViewMax = ViewRange;
	}
	else if ( Max.IsSet() && NewViewMax > Max.GetValue() )
	{
		// if we have min data set
		NewViewMin = Max.GetValue() - ViewRange;
		NewViewMax = Max.GetValue();
	}
}

FReply SCurveEditor::OnMouseMove( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	FRichCurve* Curve = GetCurve(0);
	if( Curve != NULL)
	{
		// When mouse moves, if we are moving a key, update its 'input' position
		if(bPanning  || bMovingKeys || bMovingTangent)
		{
			FTrackScaleInfo ScaleInfo(ViewMinInput.Get(),  ViewMaxInput.Get(), ViewMinOutput, ViewMaxOutput, InMyGeometry.Size);
			FVector2D ScreenDelta = InMouseEvent.GetCursorDelta();
			TotalDragDist += ScreenDelta.Size(); // accumulate distance moved

			FVector2D InputDelta;
			InputDelta.X = ScreenDelta.X/ScaleInfo.PixelsPerInput;
			InputDelta.Y = -ScreenDelta.Y/ScaleInfo.PixelsPerOutput;

			if( IsEditingEnabled() &&  bMovingKeys)
			{
				MoveSelectedKeysByDelta(InputDelta);
			}
			else if( IsEditingEnabled() &&  bMovingTangent)
			{
				FVector2D MousePositionScreen = InMouseEvent.GetScreenSpacePosition();
				MousePositionScreen -= InMyGeometry.AbsolutePosition;
				FVector2D MousePositionCurve(ScaleInfo.LocalXToInput(MousePositionScreen.X), ScaleInfo.LocalYToOutput(MousePositionScreen.Y));
				OnMoveTangent(MousePositionCurve);
			}
			else if(bPanning)
			{
				// we don't filter Output
				ViewMinOutput = (ViewMinOutput-InputDelta.Y);
				ViewMaxOutput = (ViewMaxOutput-InputDelta.Y);

				// we clamped to Data in/output if set
				float NewMinInput = ViewMinInput.Get() - InputDelta.X;
				float NewMaxInput = ViewMaxInput.Get() - InputDelta.X;

				// clamp to data input if set
				ClampViewRangeToDataIfBound(NewMinInput, NewMaxInput, DataMinInput, DataMaxInput, ScaleInfo.ViewInputRange);
				SetInputViewRangeHandler.Execute(NewMinInput, NewMaxInput);
			}

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SCurveEditor::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	const float ZoomDelta = -0.1f * MouseEvent.GetWheelDelta();

	if (bAllowZoomOutput)
	{
		const float OutputViewSize = ViewMaxOutput - ViewMinOutput;
		const float OutputChange = OutputViewSize * ZoomDelta;

		ViewMinOutput = (ViewMinOutput - (OutputChange * 0.5f));
		ViewMaxOutput = (ViewMaxOutput + (OutputChange * 0.5f));
	}

	{
		const float InputViewSize = ViewMaxInput.Get() - ViewMinInput.Get();
		const float InputChange = InputViewSize * ZoomDelta;

		const float NewMinInput = ViewMinInput.Get() - (InputChange * 0.5f);
		const float NewMaxInput = ViewMaxInput.Get() + (InputChange * 0.5f);


		SetInputViewRangeHandler.Execute(NewMinInput, NewMaxInput);
	}

	return FReply::Handled();
}

FReply SCurveEditor::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	if(IsEditingEnabled() && InKeyboardEvent.GetKey() == EKeys::Delete)
	{
		DeleteSelectedKeys();
		return FReply::Handled();
	}
	else
	{
		if( Commands->ProcessCommandBindings( InKeyboardEvent ) )
		{
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}
}

void SCurveEditor::NewValueEntered( const FText& NewText, ETextCommit::Type CommitInfo )
{
	// Don't digest the number if we just clicked away from the popup
	if ((CommitInfo == ETextCommit::OnEnter) || (CommitInfo == ETextCommit::OnUserMovedFocus))
	{
		float NewValue = FCString::Atof( *NewText.ToString() );

		// Iterate over selected set
		for(int32 i=0; i<SelectedKeys.Num(); i++)
		{
			auto Key = SelectedKeys[i];
			if(IsValidCurve(Key.Curve))
			{
				const FScopedTransaction Transaction( LOCTEXT("CurveEditor_NewValue", "New Value Entered") );
				CurveOwner->ModifyOwner();

				// Fill in each element of this key
				Key.Curve->SetKeyValue(Key.KeyHandle, NewValue);
			}
		}

		FSlateApplication::Get().DismissAllMenus();
	}
}

void SCurveEditor::NewTimeEntered( const FText& NewText, ETextCommit::Type CommitInfo )
{
	// Don't digest the number if we just clicked away from the pop-up
	if ((CommitInfo == ETextCommit::OnEnter) || (CommitInfo == ETextCommit::OnUserMovedFocus))
	{
		float NewTime = FCString::Atof( *NewText.ToString() );

		if(SelectedKeys.Num() >= 1)
		{
			auto Key = SelectedKeys[0];
			if(IsValidCurve(Key.Curve))
			{
				const FScopedTransaction Transaction( LOCTEXT("CurveEditor_NewTime", "New Time Entered") );
				CurveOwner->ModifyOwner();
				Key.Curve->SetKeyTime(Key.KeyHandle, NewTime);
			}
		}
		
		FSlateApplication::Get().DismissAllMenus();
	}
}


SCurveEditor::FSelectedCurveKey SCurveEditor::HitTestKeys(const FGeometry& InMyGeometry, const FVector2D& HitScreenPosition)
{	
	FSelectedCurveKey SelectedKey(NULL,FKeyHandle());

	if( AreCurvesVisible() )
	{
		FTrackScaleInfo ScaleInfo(ViewMinInput.Get(),  ViewMaxInput.Get(), ViewMinOutput, ViewMaxOutput, InMyGeometry.Size);

		const FVector2D HitPosition = InMyGeometry.AbsoluteToLocal( HitScreenPosition );


		for(auto CurveIt = Curves.CreateConstIterator();CurveIt;++CurveIt)
		{
			FRichCurve* Curve = CurveIt->CurveToEdit;
			if(Curve != NULL)
			{
				for (auto It(Curve->GetKeyHandleIterator()); It; ++It)
				{
					float KeyScreenX = ScaleInfo.InputToLocalX(Curve->GetKeyTime(It.Key()));
					float KeyScreenY = ScaleInfo.OutputToLocalY(Curve->GetKeyValue(It.Key()));

					if(	HitPosition.X > (KeyScreenX - (0.5f * CONST_KeySize.X)) && 
						HitPosition.X < (KeyScreenX + (0.5f * CONST_KeySize.X)) &&
						HitPosition.Y > (KeyScreenY - (0.5f * CONST_KeySize.Y)) &&
						HitPosition.Y < (KeyScreenY + (0.5f * CONST_KeySize.Y)) )
					{
						SelectedKey =  FSelectedCurveKey(Curve, It.Key());

						//prioritize selected curves
						if(SelectedCurves.Contains(Curve))
						{
							return SelectedKey;
						}
					}
				}
			}
		}
	}

	return SelectedKey;
}

void SCurveEditor::MoveSelectedKeysByDelta(FVector2D InputDelta)
{
	const FScopedTransaction Transaction( LOCTEXT("CurveEditor_MoveKeys", "Move Keys") );
	CurveOwner->ModifyOwner();

	// track all unique curves encountered so their tangents can be updated later
	TSet<FRichCurve*> UniqueCurves;

	for(int32 i=0; i<SelectedKeys.Num(); i++)
	{
		FSelectedCurveKey OldKey = SelectedKeys[i];

		if(!IsValidCurve(OldKey.Curve))
		{
			continue;
		}

		FKeyHandle OldKeyHandle	= OldKey.KeyHandle;
		FRichCurve* Curve		= OldKey.Curve;

		// Update output - easy
		float OldOutput = Curve->GetKeyValue(OldKeyHandle);
		// set the key value, but don't update the tangents yet
		Curve->SetKeyValue(OldKeyHandle, OldOutput + InputDelta.Y, false);

		// Update input - hard
		float OldInput = Curve->GetKeyTime(OldKeyHandle);
		FKeyHandle KeyHandle = Curve->SetKeyTime(OldKeyHandle, OldInput + InputDelta.X);

		// Update this key
		SelectedKeys[i] = FSelectedCurveKey(Curve, KeyHandle);

		UniqueCurves.Add(Curve);
	}

	// update auto tangents for all curves encountered, once each only
	for(TSet<FRichCurve*>::TIterator SetIt(UniqueCurves);SetIt;++SetIt)
	{
		(*SetIt)->AutoSetTangents();
	}
}

FText SCurveEditor::GetKeyValueText(FSelectedCurveKey Key) const
{
	FText ValueText;
	if(IsValidCurve(Key.Curve))
	{
		float Value = Key.Curve->GetKeyValue(Key.KeyHandle);
		ValueText = FText::AsNumber( Value );
	}

	return ValueText;
}

FText SCurveEditor::GetKeyTimeText(FSelectedCurveKey Key) const
{
	FText TimeText;
	if ( IsValidCurve(Key.Curve) )
	{
		float Time = Key.Curve->GetKeyTime(Key.KeyHandle);
		TimeText = FText::AsNumber( Time );
	}

	return TimeText;
}

void SCurveEditor::EmptySelection()
{
	SelectedKeys.Empty();
}

void SCurveEditor::AddToSelection(FSelectedCurveKey Key)
{
	SelectedKeys.AddUnique(Key);
}

void SCurveEditor::RemoveFromSelection(FSelectedCurveKey Key)
{
	SelectedKeys.Remove(Key);
}

bool SCurveEditor::IsKeySelected(FSelectedCurveKey Key) const
{
	return SelectedKeys.Contains(Key);
}

bool SCurveEditor::AreKeysSelected() const
{
	return SelectedKeys.Num() > 0;
}


TArray<FRichCurve*> SCurveEditor::GetCurvesToFit() const
{
	TArray<FRichCurve*> FitCurves;

	if(SelectedCurves.Num() > 0)
	{
		FitCurves = SelectedCurves;
	}
	else
	{
		for(auto It = Curves.CreateConstIterator();It;++It)
		{
			FitCurves.Add(It->CurveToEdit);
		}
	}

	return FitCurves;
}


FReply SCurveEditor::ZoomToFitHorizontal()
{
	TArray<FRichCurve*> CurvesToFit = GetCurvesToFit();

	if(Curves.Num() > 0)
	{
		float InMin = FLT_MAX, InMax = -FLT_MAX;
		int32 TotalKeys = 0;
		for(auto It = CurvesToFit.CreateConstIterator();It;++It)
		{
			FRichCurve* Curve = *It;
			float MinTime, MaxTime;
			Curve->GetTimeRange(MinTime, MaxTime);
			InMin = FMath::Min(MinTime, InMin);
			InMax = FMath::Max(MaxTime, InMax);
			TotalKeys += Curve->GetNumKeys();
		}
		
		if(TotalKeys > 0)
		{
			// Clamp the minimum size
			float Size = InMax - InMin;
			if( Size < CONST_MinViewRange )
			{
				InMin -= (0.5f*CONST_MinViewRange);
				InMax += (0.5f*CONST_MinViewRange);
				Size = InMax - InMin;
			}

			// add margin
			InMin -= CONST_FitMargin*Size;
			InMax += CONST_FitMargin*Size;
		}
		else
		{
			InMin = -CONST_FitMargin*2.0f;
			InMax = (CONST_DefaultZoomRange + CONST_FitMargin)*2.0;
		}

		SetInputViewRangeHandler.Execute(InMin, InMax);
	}
	return FReply::Handled();
}

/** Set Default output values when range is too small **/
void SCurveEditor::SetDefaultOutput(const float MinZoomRange)
{
	ViewMinOutput = (ViewMinOutput - (0.5f*MinZoomRange));
	ViewMaxOutput = (ViewMaxOutput + (0.5f*MinZoomRange));
}

FReply SCurveEditor::ZoomToFitVertical()
{
	TArray<FRichCurve*> CurvesToFit = GetCurvesToFit();

	if(CurvesToFit.Num() > 0)
	{
		float InMin = FLT_MAX, InMax = -FLT_MAX;
		int32 TotalKeys = 0;
		for(auto It = CurvesToFit.CreateConstIterator();It;++It)
		{
			FRichCurve* Curve = *It;
			float MinVal, MaxVal;
			Curve->GetValueRange(MinVal, MaxVal);
			InMin = FMath::Min(MinVal, InMin);
			InMax = FMath::Max(MaxVal, InMax);
			TotalKeys += Curve->GetNumKeys();
		}
		const float MinZoomRange = (TotalKeys > 0 ) ? CONST_MinViewRange: CONST_DefaultZoomRange;

		ViewMinOutput = (InMin);
		ViewMaxOutput = (InMax);

		// Clamp the minimum size
		float Size = ViewMaxOutput - ViewMinOutput;
		if( Size < MinZoomRange )
		{
			SetDefaultOutput(MinZoomRange);
			Size = ViewMaxOutput - ViewMinOutput;
		}

		// add margin
		ViewMinOutput = (ViewMinOutput - CONST_FitMargin*Size);
		ViewMaxOutput = (ViewMaxOutput + CONST_FitMargin*Size);
	}
	return FReply::Handled();
}

FText SCurveEditor::GetInputInfoText() const
{
	if ( SelectedKeys.Num() == 1 )
	{
		return GetKeyTimeText( SelectedKeys[0] );
	}

	return LOCTEXT("NoInputInfo", "---");
}

FText SCurveEditor::GetOutputInfoText() const
{
	// Return the value string if all selected keys have the same output string, otherwise empty
	FText OutputText;
	if ( SelectedKeys.Num() > 0 )
	{
		OutputText = GetKeyValueText(SelectedKeys[0]);
		for(int32 i=1; i<SelectedKeys.Num(); i++)
		{
			FText KeyOutput = GetKeyValueText( SelectedKeys[i] );
			if ( !KeyOutput.EqualTo( OutputText ) )
			{
				OutputText = FText::GetEmpty();
			}
		}
	}

	return OutputText;
}

FText SCurveEditor::GetKeyInfoText() const
{
	if ( SelectedKeys.Num() == 1 && SelectedKeys[0].Curve->IsKeyHandleValid(SelectedKeys[0].KeyHandle) )
	{
		return FText::Format(LOCTEXT("KeyInfo", "Index {0}"), FText::AsNumber( SelectedKeys[0].Curve->GetIndexSafe(SelectedKeys[0].KeyHandle) ) );
	}
	else
	{
		return LOCTEXT("NoKeyInfo", "---");
	}
}

void SCurveEditor::CreateContextMenu()
{
	const bool CloseAfterSelection = false;
	FMenuBuilder MenuBuilder( CloseAfterSelection, NULL );

	MenuBuilder.BeginSection("CurveEditorActions", LOCTEXT("CurveAction", "Curve Actions") );
	{
		if( OnCreateAsset.IsBound() && IsEditingEnabled() )
		{
			FUIAction Action = FUIAction( FExecuteAction::CreateSP( this, &SCurveEditor::OnCreateExternalCurveClicked ) );
			MenuBuilder.AddMenuEntry
			( 
				LOCTEXT("CreateExternalCurve", "Create External Curve"),
				LOCTEXT("CreateExternalCurve_ToolTip", "Create an external asset using this internal curve"), 
				FSlateIcon(), 
				Action
			);
		}

		if( IsLinearColorCurve() && !bAlwaysDisplayColorCurves )
		{
			FUIAction ShowCurveAction( FExecuteAction::CreateSP( this, &SCurveEditor::OnShowCurveToggled ), FCanExecuteAction(), FIsActionChecked::CreateSP( this, &SCurveEditor::AreCurvesVisible ) );
			MenuBuilder.AddMenuEntry
			(
				LOCTEXT("ShowCurves","Show Curves"),
				LOCTEXT("ShowCurves_ToolTip", "Toggles displaying the curves for linear colors"),
				FSlateIcon(),
				ShowCurveAction,
				NAME_None,
				EUserInterfaceActionType::ToggleButton 
			);
		}

		if( IsLinearColorCurve() )
		{
			FUIAction ShowGradientAction( FExecuteAction::CreateSP( this, &SCurveEditor::OnShowGradientToggled ), FCanExecuteAction(), FIsActionChecked::CreateSP( this, &SCurveEditor::IsGradientEditorVisible ) );
			MenuBuilder.AddMenuEntry
			(
				LOCTEXT("ShowGradient","Show Gradient"),
				LOCTEXT("ShowGradient_ToolTip", "Toggles displaying the gradient for linear colors"),
				FSlateIcon(),
				ShowGradientAction,
				NAME_None,
				EUserInterfaceActionType::ToggleButton 
			);
		}
	}

	MenuBuilder.EndSection();
	FSlateApplication::Get().PushMenu( SharedThis( this ), MenuBuilder.MakeWidget(), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu ) );
}

void SCurveEditor::OnCreateExternalCurveClicked()
{
	OnCreateAsset.ExecuteIfBound();
}


UObject* SCurveEditor::CreateCurveObject( TSubclassOf<UCurveBase> CurveType, UObject* PackagePtr, FName& AssetName )
{
	UObject* NewObj = NULL;
	CurveFactory = Cast<UCurveFactory>(ConstructObject<UFactory>( UCurveFactory::StaticClass() ) );
	if(CurveFactory)
	{
		CurveFactory->CurveClass = CurveType;
		NewObj = CurveFactory->FactoryCreateNew( CurveFactory->GetSupportedClass(), PackagePtr, AssetName, RF_Public|RF_Standalone, NULL, GWarn );
	}
	CurveFactory = NULL;
	return NewObj;
}

bool SCurveEditor::IsEditingEnabled() const
{
	return bCanEditTrack;
}

void SCurveEditor::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject( CurveFactory );
}

bool SCurveEditor::IsValidCurve( FRichCurve* Curve ) const
{
	bool bIsValid = false;
	if(Curve)
	{
		for(auto It = Curves.CreateConstIterator();It;++It)
		{
			if(It->CurveToEdit == Curve)
			{
				bIsValid = true;
				break;
			}
		}
	}
	return bIsValid;
}

FLinearColor SCurveEditor::GetCurveColor( FRichCurve* TestCurve ) const
{
	check(TestCurve);
	for(int32 I = 0;I != Curves.Num() && I < CurveColors.Num();I++)
	{
		FRichCurve* Curve =Curves[I].CurveToEdit;
		if(Curve == TestCurve)
		{
			return CurveColors[I];
		}
	}
	return FLinearColor::Black;
}

void SCurveEditor::OnUserSelectedCurve( ESlateCheckBoxState::Type State, FRichCurve* Curve )
{
	if(IsValidCurve(Curve))
	{
		if(State == ESlateCheckBoxState::Checked)
		{
			SelectCurve(Curve, false);
		}
		else
		{
			SelectedCurves.Remove(Curve);
		}	
	}
}

FText SCurveEditor::GetCurveName( FRichCurve* Curve ) const
{
	for(auto It = Curves.CreateConstIterator();It;++It)
	{
		if(It->CurveToEdit == Curve)
		{
			return FText::FromName( It->CurveName );
		}
	}
	return FText::GetEmpty();
}

bool SCurveEditor::HitTestCurves(  const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	bool bHit = false;

	if( AreCurvesVisible() )
	{
		const bool bControlDown = InMouseEvent.IsControlDown();

		FTrackScaleInfo ScaleInfo(ViewMinInput.Get(),  ViewMaxInput.Get(), ViewMinOutput, ViewMaxOutput, InMyGeometry.Size);

		const FVector2D HitPosition = InMyGeometry.AbsoluteToLocal( InMouseEvent.GetScreenSpacePosition()  );



		for(auto It = Curves.CreateConstIterator();It;++It)
		{
			FRichCurve* Curve = It->CurveToEdit;
			if(Curve != NULL)
			{
				float Time		 = ScaleInfo.LocalXToInput(HitPosition.X);
				float KeyScreenY = ScaleInfo.OutputToLocalY(Curve->Eval(Time));


				if( HitPosition.Y > (KeyScreenY - (0.5f * CONST_CurveSize.Y)) &&
					HitPosition.Y < (KeyScreenY + (0.5f * CONST_CurveSize.Y)))
				{
					SelectCurve(Curve, bControlDown); 
					bHit = true;
				}
			}
		}
	}

	if(!bHit)
	{
		//Clicked on emptyness, so clear curve selection
		SelectedCurves.Empty();
	}
	return bHit;
}

SCurveEditor::FSelectedTangent SCurveEditor::HitTestCubicTangents( const FGeometry& InMyGeometry, const FVector2D& HitScreenPosition )
{
	FSelectedTangent Tangent;

	if( AreCurvesVisible() )
	{
		FTrackScaleInfo ScaleInfo(ViewMinInput.Get(),  ViewMaxInput.Get(), ViewMinOutput, ViewMaxOutput, InMyGeometry.Size);

		const FVector2D HitPosition = InMyGeometry.AbsoluteToLocal( HitScreenPosition);

		for(auto It = SelectedKeys.CreateConstIterator();It;++It)
		{
			FSelectedCurveKey Key = *It;
			if(Key.IsValid())
			{
				float Time		 = ScaleInfo.LocalXToInput(HitPosition.X);
				float KeyScreenY = ScaleInfo.OutputToLocalY(Key.Curve->Eval(Time));

				FVector2D Arrive, Leave;
				GetTangentPoints(ScaleInfo, Key, Arrive, Leave);

				if( HitPosition.Y > (Arrive.Y - (0.5f * CONST_CurveSize.Y)) &&
					HitPosition.Y < (Arrive.Y + (0.5f * CONST_CurveSize.Y)) &&
					HitPosition.X > (Arrive.X - (0.5f * CONST_KeySize.X)) && 
					HitPosition.X < (Arrive.X + (0.5f * CONST_KeySize.X)))
				{
					Tangent.Key = Key;
					Tangent.bIsArrival = true;
					break;
				}
				if( HitPosition.Y > (Leave.Y - (0.5f * CONST_CurveSize.Y)) &&
					HitPosition.Y < (Leave.Y  + (0.5f * CONST_CurveSize.Y)) &&
					HitPosition.X > (Leave.X - (0.5f * CONST_KeySize.X)) && 
					HitPosition.X < (Leave.X + (0.5f * CONST_KeySize.X)))
				{
					Tangent.Key = Key;
					Tangent.bIsArrival = false;
					break;
				}
			}
		}
	}

	return Tangent;
}



void SCurveEditor::SelectCurve( FRichCurve* Curve, bool bMultiple )
{
	if(bMultiple)
	{
		SelectedCurves.AddUnique(Curve);
	}
	else
	{
		SelectedCurves.Empty();
		SelectedCurves.Add(Curve);
	}	
}

ESlateCheckBoxState::Type SCurveEditor::GetCheckState( FRichCurve* Curve ) const
{
	return (SelectedCurves.Find(Curve) == INDEX_NONE) ?  ESlateCheckBoxState::Unchecked : ESlateCheckBoxState::Checked;
}

FText SCurveEditor::GetCurveCheckBoxToolTip( FRichCurve* Curve ) const
{
	FText Result;
	if ( IsValidCurve(Curve) )
	{
		if(SelectedCurves.Find(Curve) != INDEX_NONE)
		{
			Result = FText::Format( LOCTEXT("CurveIndexSelectorToolTipChecked", "Toggle to Disable Adding Keys to Curve '{0}': shift + left mouse to Add Keys"), GetCurveName(Curve) );
		}
		else
		{
			Result = FText::Format( LOCTEXT("CurveIndexSelectorToolTipUnChecked", "Toggle to Enable Adding Keys to Curve '{0}'"), GetCurveName(Curve) );
		}
	}

	return Result;
}

void SCurveEditor::OnSelectInterpolationMode(ERichCurveInterpMode InterpMode, ERichCurveTangentMode TangentMode)
{
	const FScopedTransaction Transaction( LOCTEXT("CurveEditor_SetInterpolationMode", "Select Interpolation Mode") );
	CurveOwner->ModifyOwner();
	if(SelectedKeys.Num() > 0)
	{
		for(auto It = SelectedKeys.CreateIterator();It;++It)
		{
			FSelectedCurveKey& Key = *It;
			check(IsValidCurve(Key.Curve));
			Key.Curve->SetKeyInterpMode(Key.KeyHandle,InterpMode );
			Key.Curve->SetKeyTangentMode(Key.KeyHandle,TangentMode );
		}
	}
	else
	{
		for(auto CurveIt = SelectedCurves.CreateIterator();CurveIt;++CurveIt)
		{
			FRichCurve* Curve = *CurveIt;
			check(IsValidCurve(Curve));
			for (auto It(Curve->GetKeyHandleIterator()); It; ++It)
			{
				Curve->SetKeyInterpMode(It.Key(),InterpMode );
				Curve->SetKeyTangentMode(It.Key(),TangentMode );
			}
		}
	}

}

/* Given a tangent value for a key, calculates the 2D delta vector from that key in curve space */
static inline FVector2D CalcTangentDir(float Tangent)
{
	const float Angle = FMath::Atan(Tangent);
	return FVector2D( FMath::Cos(Angle), -FMath::Sin(Angle) );
}

/*Given a 2d delta vector in curve space, calculates a tangent value */
static inline float CalcTangent(const FVector2D& HandleDelta)
{
	// Ensure X is positive and non-zero.
	// Tangent is gradient of handle.
	return HandleDelta.Y / FMath::Max<double>(HandleDelta.X, KINDA_SMALL_NUMBER);
}

void SCurveEditor::OnMoveTangent(FVector2D MouseCurvePosition)
{
	auto& RichKey = SelectedTangent.Key.Curve->GetKey(SelectedTangent.Key.KeyHandle);

	const FSelectedCurveKey &Key = SelectedTangent.Key;

	FVector2D KeyPosition( Key.Curve->GetKeyTime(Key.KeyHandle),Key.Curve->GetKeyValue(Key.KeyHandle) );

	FVector2D Movement = MouseCurvePosition - KeyPosition;
	if(SelectedTangent.bIsArrival)
	{
		Movement *= -1.0f;
	}
	float Tangent = CalcTangent(Movement);

	if(RichKey.TangentMode  != RCTM_Break)
	{
		RichKey.ArriveTangent = Tangent;
		RichKey.LeaveTangent  = Tangent;
		
		RichKey.TangentMode = RCTM_User;
	}
	else
	{
		if(SelectedTangent.bIsArrival)
		{
			RichKey.ArriveTangent = Tangent;
		}
		else
		{
			RichKey.LeaveTangent = Tangent;
		}
	}	

}
void SCurveEditor::GetTangentPoints(  FTrackScaleInfo &ScaleInfo, const FSelectedCurveKey &Key, FVector2D& Arrive, FVector2D& Leave ) const
{
	FVector2D ArriveTangentDir = CalcTangentDir( Key.Curve->GetKey(Key.KeyHandle).ArriveTangent);
	FVector2D LeaveTangentDir = CalcTangentDir( Key.Curve->GetKey(Key.KeyHandle).LeaveTangent);

	FVector2D KeyPosition( Key.Curve->GetKeyTime(Key.KeyHandle),Key.Curve->GetKeyValue(Key.KeyHandle) );

	ArriveTangentDir.Y *= -1.0f;
	LeaveTangentDir.Y *= -1.0f;
	FVector2D ArrivePosition = -ArriveTangentDir + KeyPosition;

	FVector2D LeavePosition = LeaveTangentDir + KeyPosition;

	Arrive = FVector2D(ScaleInfo.InputToLocalX(ArrivePosition.X), ScaleInfo.OutputToLocalY(ArrivePosition.Y));
	Leave = FVector2D(ScaleInfo.InputToLocalX(LeavePosition.X), ScaleInfo.OutputToLocalY(LeavePosition.Y));

	FVector2D KeyScreenPosition = FVector2D(ScaleInfo.InputToLocalX(KeyPosition.X), ScaleInfo.OutputToLocalY(KeyPosition.Y));

	FVector2D ToArrive = Arrive - KeyScreenPosition;
	ToArrive.Normalize();

	Arrive = KeyScreenPosition + ToArrive*CONST_KeyTangentOffset;

	FVector2D ToLeave = Leave - KeyScreenPosition;
	ToLeave.Normalize();

	Leave = KeyScreenPosition + ToLeave*CONST_KeyTangentOffset;
}

void SCurveEditor::BeginDragTransaction()
{
	TransactionIndex = GEditor->BeginTransaction( LOCTEXT("CurveEditor_Drag", "Mouse Drag") );
	check( TransactionIndex >= 0 );
	CurveOwner->ModifyOwner();
}

void SCurveEditor::EndDragTransaction()
{
	if ( TransactionIndex >= 0 )
	{
		GEditor->EndTransaction( );
		TransactionIndex = -1;
	}
}

bool SCurveEditor::FSelectedTangent::IsValid() const
{
	return Key.IsValid();
}

void SCurveEditor::UndoAction()
{
	GEditor->UndoTransaction();
}

void SCurveEditor::RedoAction()
{
	GEditor->RedoTransaction();
}

void SCurveEditor::PostUndo(bool bSuccess)
{
	//remove any invalid keys
	for(int32 i = 0;i<SelectedKeys.Num();++i)
	{
		auto Key = SelectedKeys[i];
		if(!IsValidCurve(Key.Curve) || !Key.IsValid())
		{
			SelectedKeys.RemoveAt(i);
			i--;
		}
	}
}

bool SCurveEditor::IsLinearColorCurve() const 
{
	return CurveOwner && CurveOwner->GetOwner() && CurveOwner->GetOwner()->IsA<UCurveLinearColor>();
}

FText SCurveEditor::GetInterpolationModeText() const
{
	FString Results;

	const int32 LastValidKeyIndex = SelectedKeys.Num()-1;
	for (int32 i = 0; i<SelectedKeys.Num(); ++i)
	{
		FSelectedCurveKey Key = SelectedKeys[i];
		switch(Key.Curve->GetKeyInterpMode(Key.KeyHandle))
		{
		case RCIM_Linear:
			{
				Results += LOCTEXT("InterpolateLinearMode", "Linear").ToString();
			}break;
		case RCIM_Constant:
			{
				Results += LOCTEXT("InterpolateClampMode", "Clamp").ToString();
			}break;
		case RCIM_Cubic:
			{
				switch(Key.Curve->GetKey(Key.KeyHandle).TangentMode)
				{
				case RCTM_Auto:
					{
						Results += LOCTEXT("InterpolateCubicAutoMode", "Cubic-auto").ToString();
					}break;
				case RCTM_User:
					{
						Results += LOCTEXT("InterpolateCubicLockedMode", "Cubic-locked").ToString();
					}break;
				case RCTM_Break:
					{
						Results += LOCTEXT("InterpolateCubicBreakMode", "Cubic-break").ToString();
					}break;
				}
			}break;
		}

		if(i != LastValidKeyIndex)
		{
			Results += ", ";
		}
	}

	return FText::FromString( Results );
}

#undef LOCTEXT_NAMESPACE
