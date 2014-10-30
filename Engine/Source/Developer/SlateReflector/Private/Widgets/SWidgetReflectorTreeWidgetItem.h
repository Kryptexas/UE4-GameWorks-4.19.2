// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Widget that visualizes the contents of a FReflectorNode.
 */
class SLATEREFLECTOR_API SReflectorTreeWidgetItem
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
	 * @param InArgs Declaration from which to construct this widget.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		this->WidgetInfo = InArgs._WidgetInfoToVisualize;
		this->OnAccessSourceCode = InArgs._SourceCodeAccessor;

		SMultiColumnTableRow< TSharedPtr<FReflectorNode> >::Construct( SMultiColumnTableRow< TSharedPtr<FReflectorNode> >::FArguments().Padding(1), InOwnerTableView );
	}

public:

	// SMultiColumnTableRow overrides

	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override;

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
	FSlateColor GetTint() const
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

private:

	/** The info about the widget that we are visualizing. */
	TAttribute< TSharedPtr<FReflectorNode> > WidgetInfo;

	FAccessSourceCode OnAccessSourceCode;
};
