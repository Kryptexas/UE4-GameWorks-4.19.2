// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SReflectorTreeWidgetItem.h: Declares the SReflectorTreeWidgetItem class.
=============================================================================*/

#pragma once


/**
 * Widget that visualizes the contents of a FReflectorNode
 */
class SReflectorTreeWidgetItem
	: public SMultiColumnTableRow<TSharedPtr<FReflectorNode>>
{
public:

	SLATE_BEGIN_ARGS(SReflectorTreeWidgetItem)
		: _WidgetInfoToVisualize()
		, _SourceCodeAccessor()
	{ }

		SLATE_ARGUMENT(TSharedPtr<FReflectorNode>, WidgetInfoToVisualize)
		SLATE_ARGUMENT(FAccessSourceCode, SourceCodeAccessor)

	SLATE_END_ARGS()

public:

	/**
	 * Construct child widgets that comprise this widget.
	 *
	 * @param InArgs  Declaration from which to construct this widget
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		this->WidgetInfo = InArgs._WidgetInfoToVisualize;
		this->OnAccessSourceCode = InArgs._SourceCodeAccessor;

		SMultiColumnTableRow< TSharedPtr<FReflectorNode> >::Construct( SMultiColumnTableRow< TSharedPtr<FReflectorNode> >::FArguments().Padding(1), InOwnerTableView );
	}

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) OVERRIDE
	{
		if (ColumnName == TEXT("WidgetName"))
		{
			return SNew(SHorizontalBox)
			
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SExpanderArrow, SharedThis(this))
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(this, &SReflectorTreeWidgetItem::GetWidgetType)
						.ColorAndOpacity(this, &SReflectorTreeWidgetItem::GetTint)
				];
		}
		else if (ColumnName == TEXT("WidgetInfo"))
		{
			return SNew(SBox)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(FMargin(2.0f, 0.0f))
				[
					SNew(SHyperlink)
						.Text(this, &SReflectorTreeWidgetItem::GetReadableLocation)
						.OnNavigate(this, &SReflectorTreeWidgetItem::HandleHyperlinkNavigate)
				];
		}
		else if (ColumnName == "Visibility")
		{
			return SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FMargin(2.0f, 0.0f))
				[
					SNew(STextBlock)
						.Text(this, &SReflectorTreeWidgetItem::GetVisibilityAsString)
				];
		}
		else if (ColumnName == "ForegroundColor")
		{
			TSharedPtr<SWidget> ThisWidget = WidgetInfo.Get()->Widget.Pin();
			FSlateColor Foreground = (ThisWidget.IsValid())
				? ThisWidget->GetForegroundColor()
				: FSlateColor::UseForeground();

			return SNew(SBorder)
				// Show unset color as an empty space.
				.Visibility(Foreground.IsColorSpecified() ? EVisibility::Visible : EVisibility::Hidden)
				// Show a checkerboard background so we can see alpha values well
				.BorderImage( FCoreStyle::Get().GetBrush("Checkerboard") )
				.VAlign(VAlign_Center)
				.Padding(FMargin(2.0f, 0.0f))
				[
					// Show a color block
					SNew(SColorBlock)
						.Color(Foreground.GetSpecifiedColor())
						.Size(FVector2D(16.0f, 16.0f))
				];
		}
		else
		{
			return SNullWidget::NullWidget;
		}
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

protected:

	/** @return String representation of the widget we are visualizing */
	FString GetWidgetType() const
	{
		return WidgetInfo.Get()->Widget.IsValid()
			? WidgetInfo.Get()->Widget.Pin()->GetTypeAsString()
			: TEXT("Null Widget");
	}
	
	FString GetReadableLocation() const
	{
		return WidgetInfo.Get()->Widget.IsValid()
			? WidgetInfo.Get()->Widget.Pin()->GetReadableLocation()
			: FString();
	}

	FString GetWidgetFile() const
	{
		return WidgetInfo.Get()->Widget.IsValid()
			? WidgetInfo.Get()->Widget.Pin()->GetCreatedInFile()
			: FString();
	}

	int32 GetWidgetLineNumber() const
	{
		return WidgetInfo.Get()->Widget.IsValid()
			? WidgetInfo.Get()->Widget.Pin()->GetCreatedInLineNumber()
			: 0;
	}

	FString GetVisibilityAsString() const
	{
		TSharedPtr<SWidget> TheWidget = WidgetInfo.Get()->Widget.Pin();
		return TheWidget.IsValid()
			? TheWidget->GetVisibility().ToString()
			: FString();
	}

	/** @return The tint of the reflector node */
	FSlateColor GetTint () const
	{
		return WidgetInfo.Get()->Tint;
	}

	void HandleHyperlinkNavigate()
	{
		if(OnAccessSourceCode.IsBound())
		{
			OnAccessSourceCode.Execute(GetWidgetFile(), GetWidgetLineNumber(), 0);
		}
	}

	/** The info about the widget that we are visualizing */
	TAttribute< TSharedPtr<FReflectorNode> > WidgetInfo;

	FAccessSourceCode OnAccessSourceCode;
};
