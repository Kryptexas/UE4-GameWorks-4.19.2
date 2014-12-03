// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Visibility.h"

class SWidget;
struct FGeometry;
class FHittestGrid;
class FSlateRect;
class ICustomHitTestPath;

/**
 * SWidget::OnPaint and SWidget::Paint use FPaintArgs as their
 * sole parameter in order to ease the burden of passing
 * through multiple fields.
 */
class SLATECORE_API FPaintArgs
{
public:
	FPaintArgs( const TSharedRef<SWidget>& Parent, FHittestGrid& InHittestGrid, FVector2D InWindowOffset );
	FPaintArgs WithNewParent( const SWidget* Parent ) const;
	FPaintArgs RecordHittestGeometry(const SWidget* Widget, const FGeometry& WidgetGeometry, const FSlateRect& InClippingRect) const;
	FPaintArgs InsertCustomHitTestPath( TSharedRef<ICustomHitTestPath> CustomHitTestPath, int32 HitTestIndex ) const;
	int32 GetLastHitTestIndex() const { return LastHittestIndex; }
private:
	const TSharedRef<SWidget>& ParentPtr;
	FHittestGrid& Grid;
	int32 LastHittestIndex;
	EVisibility LastRecordedVisibility;
	FVector2D WindowOffset;
};