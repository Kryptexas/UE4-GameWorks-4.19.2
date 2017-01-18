// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SBackgroundBlur.h"
#include "DrawElements.h"
#include "IConsoleManager.h"

// This feature has not been tested on es2
#if PLATFORM_USES_ES2
static int32 bAllowBackgroundBlur = 1;
#else
static int32 bAllowBackgroundBlur = 1;
#endif

static FAutoConsoleVariableRef CVarSlateAllowBackgroundBlurWidgets(TEXT("Slate.AllowBackgroundBlurWidgets"), bAllowBackgroundBlur, TEXT("If 0, no background blur widgets will be rendered"));

static int32 MaxKernelSize = 255;
static FAutoConsoleVariableRef CVarSlateMaxKernelSize(TEXT("Slate.BackgroundBlurMaxKernelSize"), MaxKernelSize, TEXT("The maximum allowed kernel size.  Note: Very large numbers can cause a huge decrease in performance"));

static int32 bDownsampleForBlur = 1;
static FAutoConsoleVariableRef CVarDownsampleForBlur(TEXT("Slate.BackgroundBlurDownsample"), bDownsampleForBlur, TEXT(""), ECVF_Cheat);

static int32 bForceLowQualityBrushFallback = 0;
static FAutoConsoleVariableRef CVarForceLowQualityBackgroundBlurOverride(TEXT("Slate.ForceBackgroundBlurLowQualityOverride"), bForceLowQualityBrushFallback, TEXT("Whether or not to force a slate brush to be used instead of actually blurring the background"), ECVF_Scalability);


void SBackgroundBlur::Construct(const FArguments& InArgs)
{
	bApplyAlphaToBlur = InArgs._bApplyAlphaToBlur;
	LowQualityFallbackBrush = InArgs._LowQualityFallbackBrush;
	BlurStrength = InArgs._BlurStrength;
	BlurRadius = InArgs._BlurRadius;

	ChildSlot
		.HAlign(InArgs._HAlign)
		.VAlign(InArgs._VAlign)
		.Padding(InArgs._Padding)
	[
		InArgs._Content.Widget
	];
}

void SBackgroundBlur::SetContent(const TSharedRef<SWidget>& InContent)
{
	ChildSlot.AttachWidget(InContent);
}

void SBackgroundBlur::SetApplyAlphaToBlur(bool bInApplyAlphaToBlur)
{
	bApplyAlphaToBlur = bInApplyAlphaToBlur;
	Invalidate(EInvalidateWidget::Layout);
}

void SBackgroundBlur::SetBlurRadius(const TAttribute<TOptional<int32>>& InBlurRadius)
{
	BlurRadius = InBlurRadius;
	Invalidate(EInvalidateWidget::Layout);
}

void SBackgroundBlur::SetBlurStrength(const TAttribute<float>& InStrength)
{
	BlurStrength = InStrength;
	Invalidate(EInvalidateWidget::Layout);
}

void SBackgroundBlur::SetLowQualityBackgroundBrush(const FSlateBrush* InBrush)
{
	LowQualityFallbackBrush = InBrush;
	Invalidate(EInvalidateWidget::Layout);
}

void SBackgroundBlur::SetHAlign(EHorizontalAlignment HAlign)
{
	ChildSlot.HAlignment = HAlign;
}

void SBackgroundBlur::SetVAlign(EVerticalAlignment VAlign)
{
	ChildSlot.VAlignment = VAlign;
}

void SBackgroundBlur::SetPadding(const TAttribute<FMargin>& InPadding)
{
	ChildSlot.SlotPadding = InPadding;
}

int32 SBackgroundBlur::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 PostFXLayerId = LayerId;
	if (bAllowBackgroundBlur && AllottedGeometry.GetLocalSize() > FVector2D::ZeroVector)
	{
		if (!bForceLowQualityBrushFallback)
		{
			// Modulate blur strength by the widget alpha
			const float Strength = BlurStrength.Get() * (bApplyAlphaToBlur ? InWidgetStyle.GetColorAndOpacityTint().A : 1.f);
			if (Strength > 0.f)
			{
				FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();

				// extract the layout transform from the draw element
				FSlateLayoutTransform InverseLayoutTransform(Inverse(FSlateLayoutTransform(PaintGeometry.DrawScale, PaintGeometry.DrawPosition)));
				// The clip rect is NOT subject to the rotations specified by MakeRotatedBox.
				FSlateRotatedClipRectType RenderClipRect = FSlateRotatedClipRectType::MakeSnappedRotatedRect(MyClippingRect, InverseLayoutTransform, AllottedGeometry.GetAccumulatedRenderTransform());

				float OffsetX = PaintGeometry.DrawPosition.X - FMath::TruncToFloat(PaintGeometry.DrawPosition.X);
				float OffsetY = PaintGeometry.DrawPosition.Y - FMath::TruncToFloat(PaintGeometry.DrawPosition.Y);

				int32 RenderTargetWidth = FMath::RoundToInt(RenderClipRect.ExtentX.X);
				int32 RenderTargetHeight = FMath::RoundToInt(RenderClipRect.ExtentY.Y);

				int32 KernelSize = 0;
				int32 DownsampleAmount = 0;
				ComputeEffectiveKernelSize(Strength, KernelSize, DownsampleAmount);

				float ComputedStrength = FMath::Max(.5f, Strength);

				if (DownsampleAmount > 0)
				{
					RenderTargetWidth = FMath::DivideAndRoundUp(RenderTargetWidth, DownsampleAmount);
					RenderTargetHeight = FMath::DivideAndRoundUp(RenderTargetHeight, DownsampleAmount);
					ComputedStrength /= DownsampleAmount;
				}

				if (RenderTargetWidth > 0 && RenderTargetHeight > 0)
				{
					FSlateDrawElement::MakePostProcessPass(OutDrawElements, LayerId, PaintGeometry, MyClippingRect, FVector4(KernelSize, ComputedStrength, RenderTargetWidth, RenderTargetHeight), DownsampleAmount);
				}


				++PostFXLayerId;
			}
		}
		else if (bAllowBackgroundBlur && bForceLowQualityBrushFallback && LowQualityFallbackBrush && LowQualityFallbackBrush->DrawAs != ESlateBrushDrawType::NoDrawType)
		{
			const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
			const uint32 DrawEffects = bIsEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

			const FLinearColor FinalColorAndOpacity(InWidgetStyle.GetColorAndOpacityTint() * LowQualityFallbackBrush->GetTint(InWidgetStyle));

			FSlateDrawElement::MakeBox(OutDrawElements, PostFXLayerId, AllottedGeometry.ToPaintGeometry(), LowQualityFallbackBrush, MyClippingRect, DrawEffects, FinalColorAndOpacity);
			++PostFXLayerId;
		}
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, PostFXLayerId, InWidgetStyle, bParentEnabled);
}

void SBackgroundBlur::ComputeEffectiveKernelSize(float Strength, int32& OutKernelSize, int32& OutDownsampleAmount) const
{
	// If the radius isn't set, auto-compute it based on the strength
	OutKernelSize = BlurRadius.Get().Get(FMath::RoundToInt(Strength * 3.f));

	// Downsample if needed
	if (bDownsampleForBlur && OutKernelSize > 9)
	{
		OutDownsampleAmount = OutKernelSize >= 31 ? 4 : 2;
		OutKernelSize /= OutDownsampleAmount;
	}

	// Kernel sizes must be odd
	if (OutKernelSize % 2 == 0)
	{
		++OutKernelSize;
	}

	OutKernelSize = FMath::Clamp(OutKernelSize, 3, MaxKernelSize);
}