// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 
#include "Slate.h"


const EVisibility EVisibility::Visible(EVisibility::VIS_Visible);

const EVisibility EVisibility::Collapsed(EVisibility::VIS_Collapsed);

const EVisibility EVisibility::Hidden(EVisibility::VIS_Hidden);

const EVisibility EVisibility::HitTestInvisible(EVisibility::VIS_HitTestInvisible);

const EVisibility EVisibility::SelfHitTestInvisible(EVisibility::VIS_SelfHitTestInvisible);

const EVisibility EVisibility::All(EVisibility::VIS_All);

FString EVisibility::ToString() const
{
	static const FString VisibleString("Visible");
	static const FString HiddenString("Hidden");
	static const FString CollapsedString("Collapsed");
	static const FString HitTestInvisibleString("HitTestInvisible");
	static const FString SelfHitTestInvisibleString("SelfHitTestInvisible");
	if ( (Value & VISPRIVATE_ChildrenHitTestVisible) == 0 )
	{
		return HitTestInvisibleString;
	}
	else if ( (Value & VISPRIVATE_SelfHitTestVisible) == 0 )
	{
		return SelfHitTestInvisibleString;
	}
	else if ((Value & VISPRIVATE_Visible) != 0)
	{
		return VisibleString;
	}
	else if ((Value & VISPRIVATE_Hidden) != 0 )
	{
		return HiddenString;
	}
	else if ((Value & VISPRIVATE_Collapsed) != 0 )
	{
		return CollapsedString;
	}
	else
	{
		return FString();
	}

}

void FArrangedChildren::AddWidget( EVisibility VisibilityOverride, const FArrangedWidget& InWidgetGeometry )
{
	if ( this->Accepts(VisibilityOverride) )
	{
		Array.Add( InWidgetGeometry );
	}
}

const float FOptionalSize::Unspecified = -1.0f;

void FArrangedChildren::AddWidget( const FArrangedWidget& InWidgetGeometry )
{
	AddWidget( InWidgetGeometry.Widget->GetVisibility(), InWidgetGeometry );
}

FArrangedWidget FGeometry::MakeChild( const TSharedRef<SWidget>& InWidget, const FVector2D& ChildOffset, const FVector2D& ChildSize, float InScale ) const
{
	return FArrangedWidget( InWidget, this->MakeChild(ChildOffset, ChildSize, InScale) );
}

FPaintGeometry FGeometry::ToPaintGeometry( FVector2D InOffset, FVector2D InSize, float InScale ) const
{
	const float CombinedScale = this->Scale*InScale;
	return FPaintGeometry( this->AbsolutePosition + InOffset*CombinedScale, InSize * CombinedScale, CombinedScale );
}

FPaintGeometry FGeometry::ToInflatedPaintGeometry( const FVector2D& InflateAmount ) const
{
	return FPaintGeometry( this->AbsolutePosition - InflateAmount*this->Scale, (this->Size+InflateAmount*2)*this->Scale, this->Scale );
}

FPaintGeometry FGeometry::CenteredPaintGeometryOnLeft( const FVector2D& SizeBeingAligned, float InScale ) const
{
	const float CombinedScale = this->Scale*InScale;
	
	return FPaintGeometry(
		this->AbsolutePosition + FVector2D(-SizeBeingAligned.X, this->Size.Y/2 - SizeBeingAligned.Y/2) * InScale,
		SizeBeingAligned * CombinedScale,
		CombinedScale
	);
}

FPaintGeometry FGeometry::CenteredPaintGeometryOnRight( const FVector2D& SizeBeingAligned, float InScale ) const
{
	const float CombinedScale = this->Scale*InScale;
	
	return FPaintGeometry(
		this->AbsolutePosition + FVector2D(this->Size.X, this->Size.Y/2 - SizeBeingAligned.Y/2) * InScale,
		SizeBeingAligned * CombinedScale,
		CombinedScale
	);
}

FPaintGeometry FGeometry::CenteredPaintGeometryBelow( const FVector2D& SizeBeingAligned, float InScale ) const
{
	const float CombinedScale = this->Scale*InScale;
	
	return FPaintGeometry(
		this->AbsolutePosition + FVector2D(this->Size.X/2 - SizeBeingAligned.X/2, this->Size.Y) * InScale,
		SizeBeingAligned * CombinedScale,
		CombinedScale
	);
}


/** @return a String representation of the Widget and corresponding Geometry*/
FString FArrangedWidget::ToString() const
{
	return FString::Printf( TEXT("%s @ %s"), *Widget->ToString(), *Geometry.ToString() );
}


FSlateRect FSlateRect::InsetBy( const FMargin& InsetAmount ) const
{
	return FSlateRect( Left + InsetAmount.Left, Top + InsetAmount.Top, Right - InsetAmount.Right, Bottom - InsetAmount.Bottom );
}