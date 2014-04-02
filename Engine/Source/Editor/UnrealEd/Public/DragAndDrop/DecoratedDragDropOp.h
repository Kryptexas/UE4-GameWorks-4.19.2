// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#define LOCTEXT_NAMESPACE "DecoratedDragDrop"

class FDecoratedDragDropOp : public FDragDropOperation, public TSharedFromThis<FDecoratedDragDropOp>
{
public:
	DRAG_DROP_OPERATOR_TYPE(FDecoratedDragDropOp, FDragDropOperation)

	/** String to show as hover text */
	FString								CurrentHoverText;

	/** Icon to be displayed */
	const FSlateBrush*					CurrentIconBrush;

	FDecoratedDragDropOp()
		: CurrentIconBrush(nullptr)
	{ }

	/** Overridden to provide public access */
	virtual void Construct() OVERRIDE
	{
		FDragDropOperation::Construct();
	}

	/** Set the decorator back to the icon and text defined by the default */
	virtual void ResetToDefaultToolTip()
	{
		CurrentHoverText = DefaultHoverText;
		CurrentIconBrush = DefaultHoverIcon;
	}

	/** The widget decorator to use */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE
	{
		// Create hover widget
		return SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Graph.ConnectorFeedback.Border"))
			.Content()
			[			
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding( 0.0f, 0.0f, 3.0f, 0.0f )
				[
					SNew( SImage )
					.Image( this, &FDecoratedDragDropOp::GetIcon )
				]
				+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign( VAlign_Center )
					[
						SNew(STextBlock) 
						.Text( this, &FDecoratedDragDropOp::GetHoverText )
					]
			];
	}

	FString GetHoverText() const
	{
		return CurrentHoverText;
	}

	const FSlateBrush* GetIcon( ) const
	{
		return CurrentIconBrush;
	}

	/** Set the text and icon for this tooltip */
	void SetToolTip(const FString& Text, const FSlateBrush* Icon)
	{
		CurrentHoverText = Text;
		CurrentIconBrush = Icon;
	}

	/** Setup some default values for the decorator */
	void SetupDefaults()
	{
		DefaultHoverText = CurrentHoverText;
		DefaultHoverIcon = CurrentIconBrush;
	}

protected:

	/** Default string to show as hover text */
	FString								DefaultHoverText;

	/** Default icon to be displayed */
	const FSlateBrush*					DefaultHoverIcon;
};


#undef LOCTEXT_NAMESPACE
