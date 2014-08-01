// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"


static TAutoConsoleVariable<float> StaticToolTipWrapWidth(
	TEXT( "Slate.ToolTipWrapWidth" ),
	1000.0f,
	TEXT( "Width of Slate tool-tips before we wrap the tool-tip text" ) );


float SToolTip::GetToolTipWrapWidth()
{
	return StaticToolTipWrapWidth.GetValueOnGameThread();
}


/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SToolTip::Construct( const FArguments& InArgs )
{
	TextContent = InArgs._Text;
	bIsInteractive = InArgs._IsInteractive;
	if( InArgs._Content.Widget != SNullWidget::NullWidget )
	{
		// Widget content argument takes precedence
		// overrides the text content.
		WidgetContent = InArgs._Content.Widget;
	}

	TSharedRef<SWidget> ToolTipContent = ( !WidgetContent.IsValid() )
		? StaticCastSharedRef<SWidget>(
			SNew( STextBlock )
			.Text( TextContent )
			.Font( InArgs._Font )
			.ColorAndOpacity( InArgs._ColorAndOpacity )
			.WrapTextAt_Static( &SToolTip::GetToolTipWrapWidth )
		)
		: InArgs._Content.Widget;

	SBorder::Construct(
		SBorder::FArguments()
		.BorderImage(InArgs._BorderImage)
		.Padding(InArgs._TextMargin)
		[
			ToolTipContent
		]	
	);

	
}


/** @return True if the tool tip has no content to display at this particular moment (remember, attribute callbacks can change this at any time!) */
bool SToolTip::IsEmpty() const
{
	return !WidgetContent.IsValid() && TextContent.Get().IsEmpty();
}


bool SToolTip::IsInteractive() const
{
	return bIsInteractive.Get();
}
