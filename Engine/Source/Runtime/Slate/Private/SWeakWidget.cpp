// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"


void SWeakWidget::Construct(const FArguments& InArgs)
{
	WeakChild.Widget = InArgs._PossiblyNullContent;
}

FVector2D SWeakWidget::ComputeDesiredSize() const
{	
	TSharedPtr<SWidget> ReferencedWidget = WeakChild.Widget.Pin();

	if (ReferencedWidget.IsValid() && ReferencedWidget->GetVisibility() != EVisibility::Collapsed)
	{
		return ReferencedWidget->GetDesiredSize();
	}

	return FVector2D::ZeroVector;
}

void SWeakWidget::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	// We just want to show the child that we are presenting. Always stretch it to occupy all of the space.
	TSharedPtr<SWidget> MyContent = WeakChild.Widget.Pin();

	if( MyContent.IsValid() && ArrangedChildren.Accepts(MyContent->GetVisibility()))
	{
		ArrangedChildren.AddWidget( AllottedGeometry.MakeChild(
			MyContent.ToSharedRef(),
			FVector2D(0,0),
			AllottedGeometry.Size
			) );
	}
}
		
FChildren* SWeakWidget::GetChildren()
{
	return &WeakChild;
}

int32 SWeakWidget::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// Just draw the children.
	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	this->ArrangeChildren(AllottedGeometry, ArrangedChildren);

	// There may be zero elements in this array if our child collapsed/hidden
	if( ArrangedChildren.Num() > 0 )
	{
		check( ArrangedChildren.Num() == 1 );
		FArrangedWidget& TheChild = ArrangedChildren(0);

		return TheChild.Widget->OnPaint( TheChild.Geometry, 
			MyClippingRect, 
			OutDrawElements, 
			LayerId + 1,
			InWidgetStyle, 
			ShouldBeEnabled( bParentEnabled ) );
	}
	return LayerId;
}

void SWeakWidget::SetContent(TWeakPtr<SWidget> InWidget)
{
	WeakChild.Widget = InWidget;
}

bool SWeakWidget::ChildWidgetIsValid() const
{
	return WeakChild.Widget.IsValid();
}

TWeakPtr<SWidget> SWeakWidget::GetChildWidget() const
{
	return WeakChild.Widget;
}
