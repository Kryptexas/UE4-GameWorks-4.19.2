// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"
#include "LayoutUtils.h"
#include "SInvalidationPanel.h"
#include "WidgetCaching.h"

#if !UE_BUILD_SHIPPING

/** True if we should allow widgets to be cached in the UI at all. */
TAutoConsoleVariable<int32> InvalidationDebugging(
	TEXT("Slate.InvalidationDebugging"),
	false,
	TEXT("Whether to show invalidation debugging visualization"));

bool SInvalidationPanel::IsInvalidationDebuggingEnabled()
{
	return InvalidationDebugging.GetValueOnGameThread() == 1;
}

void SInvalidationPanel::EnableInvalidationDebugging(bool bEnable)
{
	InvalidationDebugging.AsVariable()->Set(bEnable);
}

/** True if we should allow widgets to be cached in the UI at all. */
TAutoConsoleVariable<int32> EnableWidgetCaching(
	TEXT("Slate.EnableWidgetCaching"),
	true,
	TEXT("Whether to attempt to cache any widgets through invalidation panels."));

#endif

void SInvalidationPanel::Construct( const FArguments& InArgs )
{
	ChildSlot
	[
		InArgs._Content.Widget
	];

	bNeedsCaching = true;
	bIsInvalidating = false;
	bCanCache = true;
	RootCacheNode = nullptr;
	LastUsedCachedNodeIndex = 0;
}

SInvalidationPanel::~SInvalidationPanel()
{
	for ( int32 i = 0; i < NodePool.Num(); i++ )
	{
		delete NodePool[i];
	}
}

bool SInvalidationPanel::GetCanCache() const
{
#if !UE_BUILD_SHIPPING
	return bCanCache && EnableWidgetCaching.GetValueOnGameThread() == 1;
#else
	return bCanCache;
#endif
}

void SInvalidationPanel::SetCanCache(bool InCanCache)
{
	bCanCache = InCanCache;

	InvalidateCache();
}

void SInvalidationPanel::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if ( GetCanCache() )
	{
		if ( bNeedsCaching == false )
		{
			// If the container we're in has changed in any way we need to invalidate for sure.
			if ( AllottedGeometry.GetAccumulatedLayoutTransform() != LastAllottedGeometry.GetAccumulatedLayoutTransform() ||
				AllottedGeometry.GetAccumulatedRenderTransform() != LastAllottedGeometry.GetAccumulatedRenderTransform() )
			{
				InvalidateCache();
			}
		}

		LastAllottedGeometry = AllottedGeometry;

		if ( bNeedsCaching )
		{
			SlatePrepass(LastAllottedGeometry.Scale);
			CachePrepass(SharedThis(this));
		}
	}
}

FChildren* SInvalidationPanel::GetChildren()
{
	if ( GetCanCache() == false || bNeedsCaching )
	{
		return SCompoundWidget::GetChildren();
	}
	else
	{
		return &EmptyChildSlot;
	}
}

void SInvalidationPanel::InvalidateWidget(SWidget* InvalidateWidget)
{
	bNeedsCaching = true;

#if !UE_BUILD_SHIPPING
	if ( InvalidateWidget != nullptr && IsInvalidationDebuggingEnabled() )
	{
		InvalidatorWidgets.Add(InvalidateWidget->AsShared());
	}
#endif
}

FCachedWidgetNode* SInvalidationPanel::CreateCacheNode() const
{
	// If the node pool is empty, allocate a few
	if ( LastUsedCachedNodeIndex >= NodePool.Num() )
	{
		for ( int32 i = 0; i < 10; i++ )
		{
			NodePool.Add(new FCachedWidgetNode());
		}
	}

	// Return one of the preallocated nodes and increment the next node index.
	FCachedWidgetNode* NewNode = NodePool[LastUsedCachedNodeIndex];
	++LastUsedCachedNodeIndex;

	return NewNode;
}

int32 SInvalidationPanel::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	//TODO If the LayerId changes, or other properties, we should probably invalidate, but
	// we can't invalidate here, because we haven't done a pre-pass yet, need to do it next frame.

	if ( GetCanCache() )
	{
		const bool bWasCachingNeeded = bNeedsCaching;

		if ( bNeedsCaching )
		{
			// Always set the caching flag to false first, during the paint / tick pass we may change something
			// to volatile and need to re-cache.
			bNeedsCaching = false;

			bIsInvalidating = true;

			if ( !CachedWindowElements.IsValid() || CachedWindowElements->GetWindow() != OutDrawElements.GetWindow() )
			{
				CachedWindowElements = MakeShareable(new FSlateWindowElementList(OutDrawElements.GetWindow()));
			}
			else
			{
				CachedWindowElements->Reset();
			}

			// Reset the cached node pool index so that we effectively reset the pool.
			LastUsedCachedNodeIndex = 0;

			RootCacheNode = CreateCacheNode();
			RootCacheNode->Initialize(Args, SharedThis(const_cast<SInvalidationPanel*>( this )), AllottedGeometry, MyClippingRect);

			//TODO: When SWidget::Paint is called don't drag self if volatile, and we're doing a cache pass.
			CachedMaxChildLayer = SCompoundWidget::OnPaint(
				Args.EnableCaching(SharedThis(const_cast<SInvalidationPanel*>( this )), RootCacheNode, true, false),
				AllottedGeometry,
				MyClippingRect,
				*CachedWindowElements.Get(),
				LayerId,
				InWidgetStyle,
				bParentEnabled);

			bIsInvalidating = false;
		}

		RootCacheNode->RecordHittestGeometry(Args.GetGrid(), Args.GetLastHitTestIndex());

		OutDrawElements.AppendDrawElements(CachedWindowElements->GetDrawElements());

		int32 OutMaxChildLayer = CachedMaxChildLayer;

		// Paint the volatile elements
		if ( CachedWindowElements.IsValid() )
		{
			OutMaxChildLayer = FMath::Max(CachedMaxChildLayer, CachedWindowElements->PaintVolatile(OutDrawElements));
		}

#if !UE_BUILD_SHIPPING

		if ( IsInvalidationDebuggingEnabled() )
		{
			// Draw a green or red border depending on if we were invalidated this frame.
			{
				check(Args.IsCaching() == false);
				//const bool bShowOutlineAsCached = Args.IsCaching() || bWasCachingNeeded == false;
				const FLinearColor DebugTint = bWasCachingNeeded ? FLinearColor::Red : FLinearColor::Green;

				FGeometry ScaledOutline = AllottedGeometry.MakeChild(FVector2D(0, 0), AllottedGeometry.GetLocalSize() * AllottedGeometry.Scale, Inverse(AllottedGeometry.Scale));

				FSlateDrawElement::MakeBox(
					OutDrawElements,
					++OutMaxChildLayer,
					ScaledOutline.ToPaintGeometry(),
					FCoreStyle::Get().GetBrush(TEXT("Debug.Border")),
					MyClippingRect,
					ESlateDrawEffect::None,
					DebugTint
					);
			}

			// Draw a yellow outline around any volatile elements.
			const TArray< TSharedRef<FSlateWindowElementList::FVolatilePaint> >& VolatileElements = CachedWindowElements->GetVolatileElements();
			for ( const TSharedRef<FSlateWindowElementList::FVolatilePaint>& VolatileElement : VolatileElements )
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					++OutMaxChildLayer,
					VolatileElement->GetGeometry().ToPaintGeometry(),
					FCoreStyle::Get().GetBrush(TEXT("FocusRectangle")),
					MyClippingRect,
					ESlateDrawEffect::None,
					FLinearColor::Yellow
				);
			}

			// Draw a white flash for any widget that invalidated us this frame.
			for ( TWeakPtr<SWidget> Invalidator : InvalidatorWidgets )
			{
				TSharedPtr<SWidget> SafeInvalidator = Invalidator.Pin();
				if ( SafeInvalidator.IsValid() )
				{
					FWidgetPath WidgetPath;
					if ( FSlateApplication::Get().GeneratePathToWidgetUnchecked(SafeInvalidator.ToSharedRef(), WidgetPath, EVisibility::All) )
					{
						FArrangedWidget ArrangedWidget = WidgetPath.FindArrangedWidget(SafeInvalidator.ToSharedRef()).Get(FArrangedWidget::NullWidget);
						ArrangedWidget.Geometry.AppendTransform( FSlateLayoutTransform(Inverse(Args.GetWindowToDesktopTransform())) );

						FSlateDrawElement::MakeBox(
							OutDrawElements,
							++OutMaxChildLayer,
							ArrangedWidget.Geometry.ToPaintGeometry(),
							FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),
							MyClippingRect,
							ESlateDrawEffect::None,
							FLinearColor::White.CopyWithNewOpacity(0.6f)
						);
					}
				}
			}

			InvalidatorWidgets.Reset();
		}

#endif

		return OutMaxChildLayer;
	}
	else
	{
		return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}
}

void SInvalidationPanel::SetContent(const TSharedRef< SWidget >& InContent)
{
	InvalidateCache();

	ChildSlot
	[
		InContent
	];
}

void SInvalidationPanel::InvalidateCache()
{
	bNeedsCaching = true;
}

bool SInvalidationPanel::ComputeVolatility() const
{
	return true;
}
