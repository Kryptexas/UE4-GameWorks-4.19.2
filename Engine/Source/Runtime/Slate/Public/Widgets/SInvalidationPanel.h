// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Input/HittestGrid.h"

class SLATE_API SInvalidationPanel : public SCompoundWidget, public ILayoutCache
{
public:
	SLATE_BEGIN_ARGS( SInvalidationPanel )
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}
		SLATE_DEFAULT_SLOT( FArguments, Content )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );
	~SInvalidationPanel();

	bool GetCanCache() const;
	void SetCanCache(bool InCanCache);

	void InvalidateCache();

	// ILayoutCache overrides
	virtual void InvalidateWidget(SWidget* InvalidateWidget) override;
	virtual FCachedWidgetNode* CreateCacheNode() const override;
	// End ILayoutCache

public:

	// SWidget overrides
	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;
	virtual FChildren* GetChildren() override;
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime );
	virtual bool ComputeVolatility() const override;
	// End SWidget

	void SetContent(const TSharedRef< SWidget >& InContent);

#if !UE_BUILD_SHIPPING
	static bool IsInvalidationDebuggingEnabled();
	static void EnableInvalidationDebugging(bool bEnable);
#endif

private:
	FGeometry LastAllottedGeometry;

	FSimpleSlot EmptyChildSlot;
	FVector2D CachedDesiredSize;

#if !UE_BUILD_SHIPPING
	mutable TSet<TWeakPtr<SWidget>> InvalidatorWidgets;
#endif

	mutable FCachedWidgetNode* RootCacheNode;
	mutable TSharedPtr< FSlateWindowElementList > CachedWindowElements;
	mutable TArray< FCachedWidgetNode* > NodePool;
	mutable int32 LastUsedCachedNodeIndex;

	mutable int32 CachedMaxChildLayer;
	mutable bool bNeedsCaching;
	mutable bool bIsInvalidating;
	bool bCanCache;
};
