// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"


void SDPIScaler::Construct( const FArguments& InArgs )
{
	this->ChildSlot.Widget = InArgs._Content.Widget;
	this->DPIScale = InArgs._DPIScale;
}

void SDPIScaler::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	const EVisibility MyVisibility = this->GetVisibility();
	if ( ArrangedChildren.Accepts( MyVisibility ) )
	{
		const float MyDPIScale = DPIScale.Get();

		ArrangedChildren.AddWidget( AllottedGeometry.MakeChild(
			this->ChildSlot.Widget,
			FVector2D::ZeroVector,
			AllottedGeometry.Size / MyDPIScale,
			MyDPIScale
		));

	}
}
	
FVector2D SDPIScaler::ComputeDesiredSize() const
{
	return DPIScale.Get() * ChildSlot.Widget->GetDesiredSize();
}

FChildren* SDPIScaler::GetChildren()
{
	return &ChildSlot;
}

