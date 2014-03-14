// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Weak widgets present a piece of content without owning it.
 * e.g. A tooltip is owned by the hovered widget but displayed
 *      by a floating window. That window cannot own the tooltip
 *      and must therefore use an SWeakWidget.
 */
class SLATE_API SWeakWidget : public SWidget
{

public:
	SLATE_BEGIN_ARGS(SWeakWidget)
		{
			_Visibility = EVisibility::SelfHitTestInvisible;
		}
		SLATE_ARGUMENT( TSharedPtr<SWidget>, PossiblyNullContent )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FVector2D ComputeDesiredSize() const OVERRIDE;

	virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;
		
	virtual FChildren* GetChildren() OVERRIDE;

	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;

	void SetContent(TWeakPtr<SWidget> InWidget);

	bool ChildWidgetIsValid() const;

	TWeakPtr<SWidget> GetChildWidget() const;

private:
	TWeakChild<SWidget> WeakChild;

};
