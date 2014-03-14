// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SGestureTreeItem.h: Declares the SGestureTreeItem class.
=============================================================================*/

#pragma once


/**
 * An item for the gesture tree view
 */
struct FGestureTreeItem
{
	// Note these are mutually exclusive
	TWeakPtr<FBindingContext> BindingContext;
	TSharedPtr<FUICommandInfo> CommandInfo;

	TSharedPtr<FBindingContext> GetBindingContext() { return BindingContext.Pin(); }

	bool IsContext() const { return BindingContext.IsValid(); }
	bool IsCommand() const { return CommandInfo.IsValid(); }
};


typedef STreeView< TSharedPtr<FGestureTreeItem> > SGestureTree;


/**
 * A widget which visualizes a command info                    
 */
class SGestureTreeItem
	: public SMultiColumnTableRow< TSharedPtr< FGestureTreeItem > >
 {
public:

	SLATE_BEGIN_ARGS( SGestureTreeItem ){}
	SLATE_END_ARGS()


 public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InOwnerTable - The table that owns this tree item.
	 * @param InItem - The actual item to be displayed.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FGestureTreeItem> InItem )
	{
		TreeItem = InItem;

		SMultiColumnTableRow< TSharedPtr<FGestureTreeItem> >::Construct( FSuperRowType::FArguments(), InOwnerTable );
	}

	/**
	 * Called to generate a widget for each column
	 *
	 * @param ColumnName	The name of the column that needs a widget
	 */
	TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName )
	{
		if( ColumnName == "Name")
		{
			return SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding( 2.0f )
					.VAlign(VAlign_Center)
					[
						SNew( SExpanderArrow, SharedThis( this ) )
					]

				+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew( SVerticalBox )

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew( STextBlock )
								.Text( TreeItem->CommandInfo->GetLabel() )
								.ToolTipText( TreeItem->CommandInfo->GetDescription() )
							]

						+ SVerticalBox::Slot()
							.Padding(0.0f,0.0f,0.0f,5.0f)
							.AutoHeight()
							[
								SNew( STextBlock )
									.Font( FEditorStyle::GetFontStyle("InputBindingEditor.SmallFont") )
									.ColorAndOpacity( FLinearColor::Gray )
									.Text( TreeItem->CommandInfo->GetDescription() )
							]
					];
		}
		else if( ColumnName == "Binding")
		{
			return SNew( SBox )
				.Padding(2.0f)
				.VAlign( VAlign_Center )
				[
					SNew( SGestureEditBox, TreeItem )
				];
		}
		else
		{
			return SNew(STextBlock) .Text(NSLOCTEXT("GestureTreeItem", "UnknownColumn", "Unknown Column"));
		}
	}

	/** SWidget Interface*/
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE
	{
		if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && FMultiBoxSettings::IsInToolbarEditMode() )
		{
			return FReply::Handled().DetectDrag( SharedThis(this), MouseEvent.GetEffectingButton() );
		}

		return FReply::Unhandled();
	}

	virtual FReply OnDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE
	{
		FToolBarBuilder TempBuilder( MakeShareable(new FUICommandList), FMultiBoxCustomization::None );
		TempBuilder.AddToolBarButton( TreeItem->CommandInfo.ToSharedRef() );

		TSharedRef<SWidget> CursorDecorator = TempBuilder.MakeWidget();

		return FReply::Handled().BeginDragDrop( FUICommandDragDropOp::New( TreeItem->CommandInfo.ToSharedRef(), NAME_None, TempBuilder.MakeWidget(), FVector2D::ZeroVector ) );
	}
private:

	// Holds the tree item being visualized.
	TSharedPtr<FGestureTreeItem> TreeItem;
};
