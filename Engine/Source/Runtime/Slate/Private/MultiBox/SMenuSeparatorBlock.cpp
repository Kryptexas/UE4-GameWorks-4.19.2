// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "MultiBox.h"
#include "SMenuSeparatorBlock.h"


/**
 * Constructor
 */
FMenuSeparatorBlock::FMenuSeparatorBlock(const FName& InExtensionHook)
	: FMultiBlock( NULL, NULL, InExtensionHook )
{
}


/**
 * Allocates a widget for this type of MultiBlock.  Override this in derived classes.
 *
 * @return  MultiBlock widget object
 */
TSharedRef< class IMultiBlockBaseWidget > FMenuSeparatorBlock::ConstructWidget() const
{
	return SNew( SMenuSeparatorBlock );
}



/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SMenuSeparatorBlock::Construct( const FArguments& InArgs )
{
}



/**
 * Builds this MultiBlock widget up from the MultiBlock associated with it
 */
void SMenuSeparatorBlock::BuildMultiBlockWidget(const ISlateStyle* StyleSet, const FName& StyleName)
{
	ChildSlot.Widget =
		SNew( SVerticalBox )
			+SVerticalBox::Slot()
				.AutoHeight()

				// Add some empty space before the line, and a tiny bit after it
				.Padding( 0.0f, 4.0f, 0.0f, 2.0f )
				[
					SNew( SBorder )

						// We'll use the border's padding to actually create the horizontal line
						.Padding( StyleSet->GetMargin( StyleName, ".Separator.Padding" ) )

						// Separator graphic
						.BorderImage( StyleSet->GetBrush( StyleName, ".Separator" ) )
				];
}


