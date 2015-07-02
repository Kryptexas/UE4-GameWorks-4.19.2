// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SLATE_API SInvalidationPanel : public SCompoundWidget, public ILayoutCache
{
public:
	SLATE_BEGIN_ARGS( SInvalidationPanel )
	{
		_Visibility = EVisibility::HitTestInvisible;
	}
		SLATE_DEFAULT_SLOT( FArguments, Content )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	bool GetCanCache() const;
	void SetCanCache(bool InCanCache);

	void Invalidate();

	// ILayoutCache overrides
	virtual void InvalidateWidget(SWidget* InvalidateWidget) override;
	// End ILayoutCache

public:

	// SWidget overrides
	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const override;
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

protected:
	void CacheDesiredSize(float LayoutScaleMultiplier) override;

private:
	FGeometry LastAllottedGeometry;

	FSimpleSlot EmptyChildSlot;
	FVector2D CachedDesiredSize;

#if !UE_BUILD_SHIPPING
	mutable TSet<TWeakPtr<SWidget>> InvalidatorWidgets;
#endif

	mutable TSharedPtr< FSlateWindowElementList > CachedWindowElements;

	mutable int32 CachedMaxChildLayer;
	mutable bool bNeedsCaching;
	mutable bool bIsInvalidating;
	bool bCanCache;
};
