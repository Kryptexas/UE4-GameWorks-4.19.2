// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SWidgetReflector.cpp: Implements the SWidgetReflector class.
=============================================================================*/

#include "SlateReflectorPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SWidgetReflector"


/* SWidgetReflector interface
 *****************************************************************************/

void SWidgetReflector::Construct( const FArguments& InArgs )
{
	bShowFocus = false;
	bIsPicking = false;

	this->ChildSlot
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				
				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
					
						+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
									.Text(LOCTEXT("AppScale", "Application Scale: ").ToString())
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SSpinBox<float>)
									.Value(this, &SWidgetReflector::HandleAppScaleSliderValue)
									.MinValue(0.1f)
									.MaxValue(3.0f)
									.Delta(0.01f)
									.OnValueChanged(this, &SWidgetReflector::HandleAppScaleSliderChanged)
							]
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(5.0f)
							[
								// Check box that controls LIVE MODE
								SNew(SCheckBox)
									.IsChecked(this, &SWidgetReflector::HandleFocusCheckBoxIsChecked)
									.OnCheckStateChanged(this, &SWidgetReflector::HandleFocusCheckBoxCheckedStateChanged)
									[
										SNew(STextBlock)
											.Text(LOCTEXT("ShowFocus", "Show Focus").ToString())
									]
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(5.0f)
							[
								// Check box that controls PICKING A WIDGET TO INSPECT
								SNew(SButton)
									.OnClicked(this, &SWidgetReflector::HandlePickButtonClicked)
									.ButtonColorAndOpacity(this, &SWidgetReflector::HandlePickButtonColorAndOpacity)
									[
										SNew(STextBlock)
											.Text(this, &SWidgetReflector::HandlePickButtonText)
									]
							]
					]

				+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						// The tree view that shows all the info that we capture.
						SAssignNew(ReflectorTree, SReflectorTree)
							.ItemHeight(24.0f)
							.TreeItemsSource(&ReflectorTreeRoot)
							.OnGenerateRow(this, &SWidgetReflector::HandleReflectorTreeGenerateRow)
							.OnGetChildren(this, &SWidgetReflector::HandleReflectorTreeGetChildren)
							.OnSelectionChanged(this, &SWidgetReflector::HandleReflectorTreeSelectionChanged)
							.HeaderRow
							(
								SNew(SHeaderRow)

								+ SHeaderRow::Column("WidgetName")
									.DefaultLabel(LOCTEXT("WidgetName", "Widget Name").ToString())
									.FillWidth(0.50f)

								+ SHeaderRow::Column("ForegroundColor")
									.DefaultLabel(LOCTEXT("ForegroundColor", "Foreground Color").ToString())
									.FixedWidth(20.0f)

								+ SHeaderRow::Column("Visibility")
									.DefaultLabel(LOCTEXT("Visibility", "Visibility" ).ToString())
									.FixedWidth(125.0f)

								+ SHeaderRow::Column("WidgetInfo")
									.DefaultLabel(LOCTEXT("WidgetInfo", "Widget Info" ).ToString())
									.FillWidth(30.0f)
							)
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						// Frame rate counter
						SNew(STextBlock)
							.Text(this, &SWidgetReflector::HandleFrameRateText)
					]
			]
	];
}


/* SCompoundWidget overrides
 *****************************************************************************/

void SWidgetReflector::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	ReflectorTree->RequestTreeRefresh();
}


/* IWidgetReflector overrides
 *****************************************************************************/

bool SWidgetReflector::ReflectorNeedsToDrawIn( TSharedRef<SWindow> ThisWindow ) const
{
	return ((SelectedNodes.Num() > 0) && (ReflectorTreeRoot.Num() > 0) && (ReflectorTreeRoot[0]->Widget.Pin() == ThisWindow));
}


void SWidgetReflector::SetWidgetsToVisualize( const FWidgetPath& InWidgetsToVisualize )
{
	ReflectorTreeRoot.Empty();

	if (InWidgetsToVisualize.IsValid())
	{
		ReflectorTreeRoot.Add(FReflectorNode::NewTreeFrom(InWidgetsToVisualize.Widgets(0)));
		PickedPath.Empty();

		FReflectorNode::FindWidgetPath( ReflectorTreeRoot, InWidgetsToVisualize, PickedPath );
		VisualizeAsTree(PickedPath);
	}
}


int32 SWidgetReflector::Visualize( const FWidgetPath& InWidgetsToVisualize, FSlateWindowElementList& OutDrawElements, int32 LayerId )
{
	const bool bAttemptingToVisualizeReflector = InWidgetsToVisualize.ContainsWidget(SharedThis(this));

	if (!InWidgetsToVisualize.IsValid())
	{
		TSharedPtr<SWidget> WindowWidget = ReflectorTreeRoot[0]->Widget.Pin();
		TSharedPtr<SWindow> Window = StaticCastSharedPtr<SWindow>(WindowWidget);

		return VisualizeSelectedNodesAsRectangles(SelectedNodes, Window.ToSharedRef(), OutDrawElements, LayerId);
	}

	if (!bAttemptingToVisualizeReflector)
	{
		SetWidgetsToVisualize(InWidgetsToVisualize);

		return VisualizePickAsRectangles(InWidgetsToVisualize, OutDrawElements, LayerId);
	}		

	return LayerId;
}


/* SWidgetReflector implementation
 *****************************************************************************/

TSharedRef<SToolTip> SWidgetReflector::GenerateToolTipForReflectorNode( TSharedPtr<FReflectorNode> InReflectorNode )
{
	return SNew(SToolTip)
	[
		SNew(SReflectorToolTipWidget)
			.WidgetInfoToVisualize(InReflectorNode)
	];
}


void SWidgetReflector::VisualizeAsTree( const TArray< TSharedPtr<FReflectorNode> >& WidgetPathToVisualize )
{
	const FLinearColor TopmostWidgetColor(1.0f, 0.0f, 0.0f);
	const FLinearColor LeafmostWidgetColor(0.0f, 1.0f, 0.0f);

	for (int32 WidgetIndex = 0; WidgetIndex<WidgetPathToVisualize.Num(); ++WidgetIndex)
	{
		TSharedPtr<FReflectorNode> CurWidget = WidgetPathToVisualize[WidgetIndex];

		// Tint the item based on depth in picked path
		const float ColorFactor = static_cast<float>(WidgetIndex)/WidgetPathToVisualize.Num();
		CurWidget->Tint = FMath::Lerp(TopmostWidgetColor, LeafmostWidgetColor, ColorFactor);

		// Make sure the user can see the picked path in the tree.
		ReflectorTree->SetItemExpansion(CurWidget, true);
	}

	ReflectorTree->RequestScrollIntoView(WidgetPathToVisualize.Last());
	ReflectorTree->SetSelection(WidgetPathToVisualize.Last());
}


int32 SWidgetReflector::VisualizePickAsRectangles( const FWidgetPath& InWidgetsToVisualize, FSlateWindowElementList& OutDrawElements, int32 LayerId)
{
	const FLinearColor TopmostWidgetColor(1.0f, 0.0f, 0.0f);
	const FLinearColor LeafmostWidgetColor(0.0f, 1.0f, 0.0f);

	for (int32 WidgetIndex = 0; WidgetIndex < InWidgetsToVisualize.Widgets.Num(); ++WidgetIndex)
	{
		const FArrangedWidget& WidgetGeometry = InWidgetsToVisualize.Widgets(WidgetIndex);
		const float ColorFactor = static_cast<float>(WidgetIndex)/InWidgetsToVisualize.Widgets.Num();
		const FLinearColor Tint(1.0f - ColorFactor, ColorFactor, 0.0f, 1.0f);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			++LayerId,
			FPaintGeometry(
				WidgetGeometry.Geometry.AbsolutePosition - InWidgetsToVisualize.TopLevelWindow->GetPositionInScreen(),
				WidgetGeometry.Geometry.Size * WidgetGeometry.Geometry.Scale,
				WidgetGeometry.Geometry.Scale),
				FCoreStyle::Get().GetBrush(TEXT("Debug.Border")),
				InWidgetsToVisualize.TopLevelWindow->GetClippingRectangleInWindow(),
				ESlateDrawEffect::None,
				FMath::Lerp( TopmostWidgetColor, LeafmostWidgetColor, ColorFactor
			)
		);
	}

	return LayerId;
}


int32 SWidgetReflector::VisualizeSelectedNodesAsRectangles( const TArray<TSharedPtr<FReflectorNode>>& InNodesToDraw, const TSharedRef<SWindow>& VisualizeInWindow, FSlateWindowElementList& OutDrawElements, int32 LayerId )
{
	for (int32 NodeIndex = 0; NodeIndex < InNodesToDraw.Num(); ++NodeIndex)
	{
		const TSharedPtr<FReflectorNode>& NodeToDraw = InNodesToDraw[NodeIndex];
		const FLinearColor Tint(0.0f, 1.0f, 0.0f);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			++LayerId,
			FPaintGeometry(
				NodeToDraw->Geometry.AbsolutePosition - VisualizeInWindow->GetPositionInScreen(),
				NodeToDraw->Geometry.Size * NodeToDraw->Geometry.Scale,
				NodeToDraw->Geometry.Scale
			),
			FCoreStyle::Get().GetBrush(TEXT("Debug.Border")),
			VisualizeInWindow->GetClippingRectangleInWindow(),
			ESlateDrawEffect::None,
			NodeToDraw->Tint
		);
	}

	return LayerId;
}


/* SWidgetReflector callbacks
 *****************************************************************************/

void SWidgetReflector::HandleFocusCheckBoxCheckedStateChanged( ESlateCheckBoxState::Type NewValue )
{
	bShowFocus = NewValue != ESlateCheckBoxState::Unchecked;

	if (bShowFocus)
	{
		bIsPicking = false;
	}
}


FString SWidgetReflector::HandleFrameRateText( ) const
{
	FString MyString;
#if 0 // the new stats system does not support this
	MyString = FString::Printf(TEXT("FPS: %0.2f (%0.2f ms)"), (float)( 1.0f / FPSCounter.GetAverage()), (float)FPSCounter.GetAverage() * 1000.0f);
#endif

	return MyString;
}


FString SWidgetReflector::HandlePickButtonText( ) const
{
	static const FString NotPicking = LOCTEXT("PickWidget", "Pick Widget").ToString();
	static const FString Picking = LOCTEXT("PickingWidget", "Picking (Esc to Stop)").ToString();

	return bIsPicking ? Picking : NotPicking;
}


TSharedRef<ITableRow> SWidgetReflector::HandleReflectorTreeGenerateRow( TSharedPtr<FReflectorNode> InReflectorNode, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(SReflectorTreeWidgetItem, OwnerTable)
		.WidgetInfoToVisualize(InReflectorNode)
		.ToolTip(GenerateToolTipForReflectorNode(InReflectorNode))
		.SourceCodeAccessor(SourceAccessDelegate);
}


#undef LOCTEXT_NAMESPACE
