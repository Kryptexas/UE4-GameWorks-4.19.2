// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Editor/UnrealEd/Public/DragAndDrop/ActorDragDropGraphEdOp.h"
#include "SLayerStats.h"

#define LOCTEXT_NAMESPACE "LayersView"

namespace LayersView
{
	/** IDs for list columns */
	static const FName ColumnID_LayerLabel( "Layer" );
	static const FName ColumnID_Visibility( "Visibility" );
}

/**
 * The widget that represents a row in the LayersView's list view control.  Generates widgets for each column on demand.
 */
class SLayersViewRow : public SMultiColumnTableRow< TSharedPtr< FLayerViewModel > >
{

public:

	SLATE_BEGIN_ARGS( SLayersViewRow ){}
		SLATE_ATTRIBUTE( FText, HighlightText )
	SLATE_END_ARGS()

	/**
	 * Construct this widget.  Called by the SNew() Slate macro.
	 *
	 * @param	InArgs				Declaration used by the SNew() macro to construct this widget
	 * @param	InViewModel			The layer the row widget is supposed to represent
	 * @param	InOwnerTableView	The owner of the row widget
	 */
	void Construct( const FArguments& InArgs,  TSharedRef< FLayerViewModel > InViewModel, TSharedRef< STableViewBase > InOwnerTableView )
	{
		ViewModel = InViewModel;

		HighlightText = InArgs._HighlightText;

		SMultiColumnTableRow< TSharedPtr< FLayerViewModel > >::Construct( FSuperRowType::FArguments(), InOwnerTableView );
	}

	~SLayersViewRow()
	{
		ViewModel->OnRenamedRequest().RemoveSP(InlineTextBlock.Get(), &SInlineEditableTextBlock::EnterEditingMode);
	}

protected:

	/**
	 * Constructs the widget that represents the specified ColumnID for this Row
	 * 
	 * @param ColumnName    A unique ID for a column in this TableView; see SHeaderRow::FColumn for more info.
	 *
	 * @return a widget to represent the contents of a cell in this row of a TableView. 
	 */
	virtual TSharedRef< SWidget > GenerateWidgetForColumn( const FName & ColumnID ) OVERRIDE
	{
		TSharedPtr< SWidget > TableRowContent;

		if( ColumnID == LayersView::ColumnID_LayerLabel )
		{
			TableRowContent =
				SNew( SHorizontalBox )
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding( 0.0f, 1.0f, 3.0f, 1.0f )
				[
					SNew( SImage )
					.Image( FEditorStyle::GetBrush( TEXT( "Layer.Icon16x" ) ) )
					.ColorAndOpacity( FSlateColor::UseForeground() )
				]

				+SHorizontalBox::Slot()
				.FillWidth( 1.0f )
				[
					SAssignNew(InlineTextBlock, SInlineEditableTextBlock)
					.Font( FEditorStyle::GetFontStyle("LayersView.LayerNameFont") )
					.Text( ViewModel.Get(), &FLayerViewModel::GetNameAsText )
					.ColorAndOpacity( this, &SLayersViewRow::GetColorAndOpacity )
					.HighlightText( HighlightText )
					.ToolTipText( LOCTEXT("DoubleClickToolTip", "Double Click to Select All Actors") )
						.OnTextCommitted(this, &SLayersViewRow::OnRenameLayerTextCommitted)
						.IsSelected(this, &SLayersViewRow::IsSelectedExclusively)
				]
			;

			ViewModel->OnRenamedRequest().AddSP(InlineTextBlock.Get(), &SInlineEditableTextBlock::EnterEditingMode);
		}
		else if( ColumnID == LayersView::ColumnID_Visibility )
		{
			TableRowContent = 
				SAssignNew( VisibilityButton, SButton )
				.ContentPadding( 0 )
				.ButtonStyle( FEditorStyle::Get(), "ToggleButton" )
				.OnClicked( this, &SLayersViewRow::OnToggleVisibility )
				.ToolTipText( LOCTEXT("VisibilityButtonToolTip", "Toggle Layer Visibility") )
				.ForegroundColor( FSlateColor::UseForeground() )
				.HAlign( HAlign_Center )
				.VAlign( VAlign_Center )
				.Content()
				[
					SNew( SImage )
					.Image( this, &SLayersViewRow::GetVisibilityBrushForLayer )
					.ColorAndOpacity( this, &SLayersViewRow::GetForegroundColorForButton )
				]
			;
		}
		else
		{
			checkf(false, TEXT("Unknown ColumnID provided to SLayersView") );
		}

		return TableRowContent.ToSharedRef();
	}

	/** Callback when the SInlineEditableTextBlock changes, to update the name of the layer this row represents. */
	void OnRenameLayerTextCommitted(const FText& InText, ETextCommit::Type eInCommitType)
	{
		ViewModel->RenameTo(InText.ToString());
	}

	/**
	 * Called during drag and drop when the drag leaves a widget.
	 *
	 * @param DragDropEvent   The drag and drop event.
	 */
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) OVERRIDE
	{
		if( !DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>( DragDropEvent.GetOperation() ) )
		{
			return;
		}

		TSharedPtr< FActorDragDropGraphEdOp > DragActorOp = StaticCastSharedPtr< FActorDragDropGraphEdOp >( DragDropEvent.GetOperation() );	
		DragActorOp->SetToolTip( FActorDragDropGraphEdOp::ToolTip_Default );
	}

	/**
	 * Called during drag and drop when the the mouse is being dragged over a widget.
	 *
	 * @param MyGeometry      The geometry of the widget receiving the event.
	 * @param DragDropEvent   The drag and drop event.
	 *
	 * @return A reply that indicated whether this event was handled.
	 */
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE
	{
		if( !DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>( DragDropEvent.GetOperation() ) )
		{
			return FReply::Unhandled();
		}

		TSharedPtr< FActorDragDropGraphEdOp > DragActorOp = StaticCastSharedPtr< FActorDragDropGraphEdOp >( DragDropEvent.GetOperation() );		

		bool bCanAssign = false;
		FString Message;
		if( DragActorOp->Actors.Num() > 1 )
		{
			bCanAssign = ViewModel->CanAssignActors( DragActorOp->Actors, OUT Message );
		}
		else
		{
			bCanAssign = ViewModel->CanAssignActor( DragActorOp->Actors[ 0 ], OUT Message );
		}

		if ( bCanAssign )
		{
			DragActorOp->SetToolTip( FActorDragDropGraphEdOp::ToolTip_CompatibleGeneric, Message );
		}
		else
		{
			DragActorOp->SetToolTip( FActorDragDropGraphEdOp::ToolTip_IncompatibleGeneric, Message );
		}

		return FReply::Handled();
	}

	/**
	 * Called when the user is dropping something onto a widget; terminates drag and drop.
	 *
	 * @param MyGeometry      The geometry of the widget receiving the event.
	 * @param DragDropEvent   The drag and drop event.
	 *
	 * @return A reply that indicated whether this event was handled.
	 */
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE
	{
		if ( !DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>( DragDropEvent.GetOperation() ) )	
		{
			return FReply::Unhandled();
		}

		TSharedPtr< FActorDragDropGraphEdOp > DragActorOp = StaticCastSharedPtr< FActorDragDropGraphEdOp >( DragDropEvent.GetOperation() );		
		ViewModel->AddActors( DragActorOp->Actors );

		return FReply::Handled();
	}


private:
	
	/**
	 *	Gets the appropriate SlateColor for the specified button depending on the button's current state
	 *
	 *	@param	Button	The SButton Widget to get the Foreground Color for
	 *	@return			The ForegroundColor
	 */
	FSlateColor GetForegroundColorForButton() const
	{
		return ( VisibilityButton.IsValid() && ( VisibilityButton->IsHovered() || VisibilityButton->IsPressed() ) ) ? FEditorStyle::GetSlateColor( "InvertedForeground" ) : FSlateColor::UseForeground();
	}

	/**
	 *	Returns the Color and Opacity for displaying the bound Layer's Name.
	 *	The Color and Opacity changes depending on whether a drag/drop operation is occurring
	 *
	 *	@return	The SlateColor to render the Layer's name in
	 */
	FSlateColor GetColorAndOpacity() const
	{
		if ( !FSlateApplication::Get().IsDragDropping() )
		{
			return FSlateColor::UseForeground();
		}

		bool bCanAcceptDrop = false;
		TSharedPtr<FDragDropOperation> DragDropOp = FSlateApplication::Get().GetDragDroppingContent();

		if (DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>(DragDropOp))	
		{
			TSharedPtr<FActorDragDropGraphEdOp> DragDropActorOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>(DragDropOp);

			FString Message;
			bCanAcceptDrop = ViewModel->CanAssignActors( DragDropActorOp->Actors, OUT Message );
		}

		return ( bCanAcceptDrop ) ? FSlateColor::UseForeground() : FLinearColor( 0.30f, 0.30f, 0.30f );
	}

	/**
	 *	Called when the user clicks on the visibility icon for a layer's row widget
	 *
	 *	@return	A reply that indicated whether this event was handled.
	 */
	FReply OnToggleVisibility()
	{
		ViewModel->ToggleVisibility();
		return FReply::Handled();
	}

	/**
	 *	Called to get the Slate Image Brush representing the visibility state of
	 *	the layer this row widget represents
	 *
	 *	@return	The SlateBrush representing the layer's visibility state
	 */
	const FSlateBrush* GetVisibilityBrushForLayer() const
	{
		return ( ViewModel->IsVisible() ) ? FEditorStyle::GetBrush( "Layer.VisibleIcon16x" ) : FEditorStyle::GetBrush( "Layer.NotVisibleIcon16x" );
	}


private:

	/** The layer associated with this row of data */
	TSharedPtr< FLayerViewModel > ViewModel;

	/**	The visibility button for the layer */
	TSharedPtr< SButton > VisibilityButton;

	/** The string to highlight on any text contained in the row widget */
	TAttribute< FText > HighlightText;

	/** Widget for displaying and editing the Layer name */
	TSharedPtr< SInlineEditableTextBlock > InlineTextBlock;
};


#undef LOCTEXT_NAMESPACE
