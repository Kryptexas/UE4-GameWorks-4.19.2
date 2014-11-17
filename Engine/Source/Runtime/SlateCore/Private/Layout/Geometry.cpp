// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Geometry.cpp: Implements the FGeometry class.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* FArrangedChildren interface
 *****************************************************************************/

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
