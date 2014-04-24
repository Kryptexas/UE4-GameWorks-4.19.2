// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SDesignSurface.h"

#define LOCTEXT_NAMESPACE "Slate"

struct FZoomLevelEntry
{
public:
	FZoomLevelEntry(float InZoomAmount, const FText& InDisplayText, EGraphRenderingLOD::Type InLOD)
		: DisplayText(FText::Format(NSLOCTEXT("GraphEditor", "Zoom", "Zoom {0}"), InDisplayText))
		, ZoomAmount(InZoomAmount)
		, LOD(InLOD)
	{
	}

public:
	FText DisplayText;
	float ZoomAmount;
	EGraphRenderingLOD::Type LOD;
};

struct FFixedZoomLevelsContainer : public FZoomLevelsContainer
{
	FFixedZoomLevelsContainer()
	{
		// Initialize zoom levels if not done already
		if ( ZoomLevels.Num() == 0 )
		{
			ZoomLevels.Reserve(20);
			ZoomLevels.Add(FZoomLevelEntry(0.100f, NSLOCTEXT("GraphEditor", "ZoomLevel", "-12"), EGraphRenderingLOD::LowestDetail));
			ZoomLevels.Add(FZoomLevelEntry(0.125f, NSLOCTEXT("GraphEditor", "ZoomLevel", "-11"), EGraphRenderingLOD::LowestDetail));
			ZoomLevels.Add(FZoomLevelEntry(0.150f, NSLOCTEXT("GraphEditor", "ZoomLevel", "-10"), EGraphRenderingLOD::LowestDetail));
			ZoomLevels.Add(FZoomLevelEntry(0.175f, NSLOCTEXT("GraphEditor", "ZoomLevel", "-9"), EGraphRenderingLOD::LowestDetail));
			ZoomLevels.Add(FZoomLevelEntry(0.200f, NSLOCTEXT("GraphEditor", "ZoomLevel", "-8"), EGraphRenderingLOD::LowestDetail));
			ZoomLevels.Add(FZoomLevelEntry(0.225f, NSLOCTEXT("GraphEditor", "ZoomLevel", "-7"), EGraphRenderingLOD::LowDetail));
			ZoomLevels.Add(FZoomLevelEntry(0.250f, NSLOCTEXT("GraphEditor", "ZoomLevel", "-6"), EGraphRenderingLOD::LowDetail));
			ZoomLevels.Add(FZoomLevelEntry(0.375f, NSLOCTEXT("GraphEditor", "ZoomLevel", "-5"), EGraphRenderingLOD::MediumDetail));
			ZoomLevels.Add(FZoomLevelEntry(0.500f, NSLOCTEXT("GraphEditor", "ZoomLevel", "-4"), EGraphRenderingLOD::MediumDetail));
			ZoomLevels.Add(FZoomLevelEntry(0.675f, NSLOCTEXT("GraphEditor", "ZoomLevel", "-3"), EGraphRenderingLOD::MediumDetail));
			ZoomLevels.Add(FZoomLevelEntry(0.750f, NSLOCTEXT("GraphEditor", "ZoomLevel", "-2"), EGraphRenderingLOD::DefaultDetail));
			ZoomLevels.Add(FZoomLevelEntry(0.875f, NSLOCTEXT("GraphEditor", "ZoomLevel", "-1"), EGraphRenderingLOD::DefaultDetail));
			ZoomLevels.Add(FZoomLevelEntry(1.000f, NSLOCTEXT("GraphEditor", "ZoomLevel", "1:1"), EGraphRenderingLOD::DefaultDetail));
			ZoomLevels.Add(FZoomLevelEntry(1.250f, NSLOCTEXT("GraphEditor", "ZoomLevel", "+1"), EGraphRenderingLOD::DefaultDetail));
			ZoomLevels.Add(FZoomLevelEntry(1.375f, NSLOCTEXT("GraphEditor", "ZoomLevel", "+2"), EGraphRenderingLOD::DefaultDetail));
			ZoomLevels.Add(FZoomLevelEntry(1.500f, NSLOCTEXT("GraphEditor", "ZoomLevel", "+3"), EGraphRenderingLOD::FullyZoomedIn));
			ZoomLevels.Add(FZoomLevelEntry(1.675f, NSLOCTEXT("GraphEditor", "ZoomLevel", "+4"), EGraphRenderingLOD::FullyZoomedIn));
			ZoomLevels.Add(FZoomLevelEntry(1.750f, NSLOCTEXT("GraphEditor", "ZoomLevel", "+5"), EGraphRenderingLOD::FullyZoomedIn));
			ZoomLevels.Add(FZoomLevelEntry(1.875f, NSLOCTEXT("GraphEditor", "ZoomLevel", "+6"), EGraphRenderingLOD::FullyZoomedIn));
			ZoomLevels.Add(FZoomLevelEntry(2.000f, NSLOCTEXT("GraphEditor", "ZoomLevel", "+7"), EGraphRenderingLOD::FullyZoomedIn));
		}
	}

	float GetZoomAmount(int32 InZoomLevel) const OVERRIDE
	{
		checkSlow(ZoomLevels.IsValidIndex(InZoomLevel));
		return ZoomLevels[InZoomLevel].ZoomAmount;
	}

	int32 GetNearestZoomLevel(float InZoomAmount) const OVERRIDE
	{
		for ( int32 ZoomLevelIndex=0; ZoomLevelIndex < GetNumZoomLevels(); ++ZoomLevelIndex )
		{
			if ( InZoomAmount <= GetZoomAmount(ZoomLevelIndex) )
			{
				return ZoomLevelIndex;
			}
		}

		return GetDefaultZoomLevel();
	}

	FText GetZoomText(int32 InZoomLevel) const OVERRIDE
	{
		checkSlow(ZoomLevels.IsValidIndex(InZoomLevel));
		return ZoomLevels[InZoomLevel].DisplayText;
	}

	int32 GetNumZoomLevels() const OVERRIDE
	{
		return ZoomLevels.Num();
	}

	int32 GetDefaultZoomLevel() const OVERRIDE
	{
		return 12;
	}

	EGraphRenderingLOD::Type GetLOD(int32 InZoomLevel) const OVERRIDE
	{
		checkSlow(ZoomLevels.IsValidIndex(InZoomLevel));
		return ZoomLevels[InZoomLevel].LOD;
	}

	static TArray<FZoomLevelEntry> ZoomLevels;
};

TArray<FZoomLevelEntry> FFixedZoomLevelsContainer::ZoomLevels;

/////////////////////////////////////////////////////
// SDesignSurface

void SDesignSurface::Construct(const FArguments& InArgs)
{
	if ( !ZoomLevels.IsValid() )
	{
		ZoomLevels = new FFixedZoomLevelsContainer();
	}
	ZoomLevel = ZoomLevels->GetDefaultZoomLevel();
	PreviousZoomLevel = ZoomLevels->GetDefaultZoomLevel();
	PostChangedZoom();
	SnapGridSize = 16.0f;
	bAllowContinousZoomInterpolation = false;

	ViewOffset = FVector2D::ZeroVector;

	ZoomLevelFade = FCurveSequence(0.0f, 1.0f);
	ZoomLevelFade.Play();

	ZoomLevelGraphFade = FCurveSequence(0.0f, 0.5f);
	ZoomLevelGraphFade.Play();

	ChildSlot
	[
		InArgs._Content.Widget
	];
}

int32 SDesignSurface::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FSlateBrush* BackgroundImage = FEditorStyle::GetBrush(TEXT("Graph.Panel.SolidBackground"));
	PaintBackgroundAsLines(BackgroundImage, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId);

	SCompoundWidget::OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	return LayerId;
}

FReply SDesignSurface::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// We want to zoom into this point; i.e. keep it the same fraction offset into the panel
	const FVector2D WidgetSpaceCursorPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const int32 ZoomLevelDelta = FMath::Floor(MouseEvent.GetWheelDelta());
	ChangeZoomLevel(ZoomLevelDelta, WidgetSpaceCursorPos, MouseEvent.IsControlDown());

	return FReply::Handled();
}

inline float FancyMod(float Value, float Size)
{
	return ( ( Value >= 0 ) ? 0.0f : Size ) + FMath::Fmod(Value, Size);
}

float SDesignSurface::GetZoomAmount() const
{
	if ( bAllowContinousZoomInterpolation )
	{
		return FMath::Lerp(ZoomLevels->GetZoomAmount(PreviousZoomLevel), ZoomLevels->GetZoomAmount(ZoomLevel), ZoomLevelGraphFade.GetLerp());
	}
	else
	{
		return ZoomLevels->GetZoomAmount(ZoomLevel);
	}
}

void SDesignSurface::ChangeZoomLevel(int32 ZoomLevelDelta, const FVector2D& WidgetSpaceZoomOrigin, bool bOverrideZoomLimiting)
{
	// We want to zoom into this point; i.e. keep it the same fraction offset into the panel
	const FVector2D PointToMaintainGraphSpace = PanelCoordToGraphCoord(WidgetSpaceZoomOrigin);

	const int32 DefaultZoomLevel = ZoomLevels->GetDefaultZoomLevel();
	const int32 NumZoomLevels = ZoomLevels->GetNumZoomLevels();

	const bool bAllowFullZoomRange =
		// To zoom in past 1:1 the user must press control
		( ZoomLevel == DefaultZoomLevel && ZoomLevelDelta > 0 && bOverrideZoomLimiting ) ||
		// If they are already zoomed in past 1:1, user may zoom freely
		( ZoomLevel > DefaultZoomLevel );

	const float OldZoomLevel = ZoomLevel;

	if ( bAllowFullZoomRange )
	{
		ZoomLevel = FMath::Clamp(ZoomLevel + ZoomLevelDelta, 0, NumZoomLevels - 1);
	}
	else
	{
		// Without control, we do not allow zooming in past 1:1.
		ZoomLevel = FMath::Clamp(ZoomLevel + ZoomLevelDelta, 0, DefaultZoomLevel);
	}

	if ( OldZoomLevel != ZoomLevel )
	{
		PostChangedZoom();
	}

	// Note: This happens even when maxed out at a stop; so the user sees the animation and knows that they're at max zoom in/out
	ZoomLevelFade.Play();

	// Re-center the screen so that it feels like zooming around the cursor.
	{
		FSlateRect GraphBounds = ComputeSensibleGraphBounds();

		// Make sure we are not zooming into/out into emptiness; otherwise the user will get lost..
		const FVector2D ClampedPointToMaintainGraphSpace(
			FMath::Clamp(PointToMaintainGraphSpace.X, GraphBounds.Left, GraphBounds.Right),
			FMath::Clamp(PointToMaintainGraphSpace.Y, GraphBounds.Top, GraphBounds.Bottom)
			);

		this->ViewOffset = ClampedPointToMaintainGraphSpace - WidgetSpaceZoomOrigin / GetZoomAmount();
	}
}

FSlateRect SDesignSurface::ComputeSensibleGraphBounds() const
{
	float Left = 0.0f;
	float Top = 0.0f;
	float Right = 0.0f;
	float Bottom = 0.0f;

	// Find the bounds of the node positions
	//for ( int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex )
	//{
	//	const TSharedRef<SNode>& SomeChild = Children[ChildIndex];

	//	FVector2D ChildPos = SomeChild->GetPosition();

	//	Left = FMath::Min(Left, ChildPos.X);
	//	Right = FMath::Max(Right, ChildPos.X);
	//	Top = FMath::Min(Top, ChildPos.Y);
	//	Bottom = FMath::Max(Bottom, ChildPos.Y);
	//}

	// Pad it out in every direction, to roughly account for nodes being of non-zero extent
	const float Padding = 100.0f;

	return FSlateRect(Left - Padding, Top - Padding, Right + Padding, Bottom + Padding);
}

void SDesignSurface::PostChangedZoom()
{
}

FVector2D SDesignSurface::GetViewOffset() const
{
	return ViewOffset;
}

float SDesignSurface::GetSnapGridSize() const
{
	return SnapGridSize;
}

FVector2D SDesignSurface::GraphCoordToPanelCoord(const FVector2D& GraphSpaceCoordinate) const
{
	return ( GraphSpaceCoordinate - GetViewOffset() ) * GetZoomAmount();
}

FVector2D SDesignSurface::PanelCoordToGraphCoord(const FVector2D& PanelSpaceCoordinate) const
{
	return PanelSpaceCoordinate / GetZoomAmount() + GetViewOffset();
}

void SDesignSurface::PaintBackgroundAsLines(const FSlateBrush* BackgroundImage, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32& DrawLayerId) const
{
	const bool bAntialias = false;

	const int32 RulePeriod = (int32)FEditorStyle::GetFloat("Graph.Panel.GridRulePeriod");
	check(RulePeriod > 0);

	const FLinearColor RegularColor(FEditorStyle::GetColor("Graph.Panel.GridLineColor"));
	const FLinearColor RuleColor(FEditorStyle::GetColor("Graph.Panel.GridRuleColor"));
	const FLinearColor CenterColor(FEditorStyle::GetColor("Graph.Panel.GridCenterColor"));
	const float GraphSmallestGridSize = 8.0f;
	const float RawZoomFactor = GetZoomAmount();
	const float NominalGridSize = GetSnapGridSize();

	float ZoomFactor = RawZoomFactor;
	float Inflation = 1.0f;
	while ( ZoomFactor*Inflation*NominalGridSize <= GraphSmallestGridSize )
	{
		Inflation *= 2.0f;
	}

	const float GridCellSize = NominalGridSize * ZoomFactor * Inflation;

	const float GraphSpaceGridX0 = FancyMod(ViewOffset.X, Inflation * NominalGridSize * RulePeriod);
	const float GraphSpaceGridY0 = FancyMod(ViewOffset.Y, Inflation * NominalGridSize * RulePeriod);

	float ImageOffsetX = GraphSpaceGridX0 * -ZoomFactor;
	float ImageOffsetY = GraphSpaceGridY0 * -ZoomFactor;

	const FVector2D ZeroSpace = GraphCoordToPanelCoord(FVector2D::ZeroVector);

	// Fill the background
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		DrawLayerId,
		AllottedGeometry.ToPaintGeometry(),
		BackgroundImage,
		MyClippingRect
		);

	TArray<FVector2D> LinePoints;
	new (LinePoints)FVector2D(0.0f, 0.0f);
	new (LinePoints)FVector2D(0.0f, 0.0f);

	// Horizontal bars
	for ( int32 GridIndex = 0; ImageOffsetY < AllottedGeometry.Size.Y; ImageOffsetY += GridCellSize, ++GridIndex )
	{
		if ( ImageOffsetY >= 0.0f )
		{
			const bool bIsRuleLine = ( GridIndex % RulePeriod ) == 0;
			const int32 Layer = bIsRuleLine ? ( DrawLayerId + 1 ) : DrawLayerId;

			const FLinearColor* Color = bIsRuleLine ? &RuleColor : &RegularColor;
			if ( FMath::IsNearlyEqual(ZeroSpace.Y, ImageOffsetY, 1.0f) )
			{
				Color = &CenterColor;
			}

			LinePoints[0] = FVector2D(0.0f, ImageOffsetY);
			LinePoints[1] = FVector2D(AllottedGeometry.Size.X, ImageOffsetY);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				Layer,
				AllottedGeometry.ToPaintGeometry(),
				LinePoints,
				MyClippingRect,
				ESlateDrawEffect::None,
				*Color,
				bAntialias);
		}
	}

	// Vertical bars
	for ( int32 GridIndex = 0; ImageOffsetX < AllottedGeometry.Size.X; ImageOffsetX += GridCellSize, ++GridIndex )
	{
		if ( ImageOffsetX >= 0.0f )
		{
			const bool bIsRuleLine = ( GridIndex % RulePeriod ) == 0;
			const int32 Layer = bIsRuleLine ? ( DrawLayerId + 1 ) : DrawLayerId;

			const FLinearColor* Color = bIsRuleLine ? &RuleColor : &RegularColor;
			if ( FMath::IsNearlyEqual(ZeroSpace.X, ImageOffsetX, 1.0f) )
			{
				Color = &CenterColor;
			}

			LinePoints[0] = FVector2D(ImageOffsetX, 0.0f);
			LinePoints[1] = FVector2D(ImageOffsetX, AllottedGeometry.Size.Y);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				Layer,
				AllottedGeometry.ToPaintGeometry(),
				LinePoints,
				MyClippingRect,
				ESlateDrawEffect::None,
				*Color,
				bAntialias);
		}
	}

	DrawLayerId += 2;
}

#undef LOCTEXT_NAMESPACE
