// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SImage.h"



/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SImage::Construct( const FArguments& InArgs )
{
	Image = InArgs._Image;
	ColorAndOpacity = InArgs._ColorAndOpacity;
	OnMouseButtonDownHandler = InArgs._OnMouseButtonDown;
}


/**
 * The widget should respond by populating the OutDrawElements array with FDrawElements 
 * that represent it and any of its children.
 *
 * @param AllottedGeometry  The FGeometry that describes an area in which the widget should appear.
 * @param MyClippingRect    The clipping rectangle allocated for this widget and its children.
 * @param OutDrawElements   A list of FDrawElements to populate with the output.
 * @param LayerId           The Layer onto which this widget should be rendered.
 * @param InColorAndOpacity Color and Opacity to be applied to all the descendants of the widget being painted
 * @param bParentEnabled	True if the parent of this widget is enabled.
 *
 * @return The maximum layer ID attained by this widget or any of its children.
 */
int32 SImage::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
#if SLATE_HD_STATS
	SCOPE_CYCLE_COUNTER( STAT_SlateOnPaint_SImage );
#endif
	const FSlateBrush* ImageBrush = Image.Get();

	if ((ImageBrush != NULL) && (ImageBrush->DrawAs != ESlateBrushDrawType::NoDrawType))
	{
		const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
		const uint32 DrawEffects = bIsEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

		const FColor FinalColorAndOpacity( InWidgetStyle.GetColorAndOpacityTint() * ColorAndOpacity.Get().GetColor(InWidgetStyle) * ImageBrush->GetTint( InWidgetStyle ) );

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), ImageBrush, MyClippingRect, DrawEffects, FinalColorAndOpacity );
	}
	return LayerId;
}
	
/**
 * The system calls this method to notify the widget that a mouse button was pressed within it. This event is bubbled.
 *
 * @param MyGeometry The Geometry of the widget receiving the event
 * @param MouseEvent Information about the input event
 *
 * @return Whether the event was handled along with possible requests for the system to take action.
 */
FReply SImage::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ( OnMouseButtonDownHandler.IsBound() )
	{
		// If a handler is assigned, call it.
		return OnMouseButtonDownHandler.Execute(MyGeometry, MouseEvent);
	}
	else
	{
		// otherwise the event is unhandled.
		return FReply::Unhandled();
	}	
}

/**
 * An Image's desired size is whatever the image looks best as. This is decided on a case by case basis via the Width and Height properties.
 */
FVector2D SImage::ComputeDesiredSize() const
{
	const FSlateBrush* ImageBrush = Image.Get();
	if (ImageBrush != NULL)
	{
		return ImageBrush->ImageSize;
	}
	return FVector2D::ZeroVector;
}

void SImage::SetColorAndOpacity( const TAttribute<FSlateColor>& InColorAndOpacity )
{
	ColorAndOpacity = InColorAndOpacity;
}

void SImage::SetColorAndOpacity( FLinearColor InColorAndOpacity )
{
	ColorAndOpacity = InColorAndOpacity;
}
