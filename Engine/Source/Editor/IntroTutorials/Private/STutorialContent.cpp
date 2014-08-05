// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "STutorialContent.h"

namespace TutorialConstants
{
	const float BorderPulseAnimationLength = 1.0f;
	const float BorderIntroAnimationLength = 0.5f;
	const float MinBorderOpacity = 0.1f;
	const float ShadowScale = 8.0f;
	const float MaxBorderOffset = 8.0f;
}

const float ContentOffset = 10.0f;

void STutorialContent::Construct(const FArguments& InArgs, const FTutorialContent& InContent)
{
	bIsVisible = Anchor.Type == ETutorialAnchorIdentifier::None;

	VerticalAlignment = InArgs._VAlign;
	HorizontalAlignment = InArgs._HAlign;
	WidgetOffset = InArgs._Offset;
	bIsStandalone = InArgs._IsStandalone;
	OnClosed = InArgs._OnClosed;
	Anchor = InArgs._Anchor;

	BorderIntroAnimation.AddCurve(0.0f, TutorialConstants::BorderIntroAnimationLength, ECurveEaseFunction::CubicOut);
	BorderPulseAnimation.AddCurve(0.0f, TutorialConstants::BorderPulseAnimationLength, ECurveEaseFunction::Linear);
	BorderIntroAnimation.Play();
	BorderPulseAnimation.Play();

	ChildSlot
	[
		SAssignNew(ContentWidget, SBorder)

		// Add more padding if the content is to be displayed centrally (i.e. not on a widget)
		.Padding(Anchor.Type != ETutorialAnchorIdentifier::None ? 18.0f : 24.0f)
		.BorderImage(FEditorStyle::GetBrush("Tutorials.Border"))
		.Visibility(this, &STutorialContent::GetVisibility)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.MaxWidth(600.0f)
			.VAlign(VAlign_Center)
			[
				GenerateContentWidget(InContent, InArgs._WrapTextAt, Anchor.Type != ETutorialAnchorIdentifier::None, DocumentationPage)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Top)
			.Padding(5.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.OnClicked(this, &STutorialContent::OnCloseButtonClicked)
				.Visibility(this, &STutorialContent::GetCloseButtonVisibility)
				.ButtonStyle(&FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("Tutorials.Browser.Button"))
				.ContentPadding(0.0f)
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("Symbols.X"))
					.ColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f))
				]
			]
		]
	];
}

static void GetAnimationValues(bool bIsIntro, float InAnimationProgress, float& OutAlphaFactor, float& OutPulseFactor, FLinearColor& OutShadowTint, FLinearColor& OutBorderTint)
{
	if(bIsIntro)
	{
		OutAlphaFactor = InAnimationProgress;
		OutPulseFactor = (1.0f - OutAlphaFactor) * 50.0f;
		OutShadowTint = FLinearColor(1.0f, 1.0f, 0.0f, OutAlphaFactor);
		OutBorderTint = FLinearColor(1.0f, 1.0f, 0.0f, OutAlphaFactor * OutAlphaFactor);
	}
	else
	{
		OutAlphaFactor = 1.0f - (0.5f + (FMath::Cos(2.0f * PI * InAnimationProgress) * 0.5f));
		OutPulseFactor = 0.5f + (FMath::Cos(2.0f * PI * InAnimationProgress) * 0.5f);
		OutShadowTint = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
		OutBorderTint = FLinearColor(1.0f, 1.0f, 0.0f, TutorialConstants::MinBorderOpacity + ((1.0f - TutorialConstants::MinBorderOpacity) * OutAlphaFactor));
	}
}

int32 STutorialContent::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	if(bIsVisible && Anchor.Type != ETutorialAnchorIdentifier::None)
	{
		float AlphaFactor;
		float PulseFactor;
		FLinearColor ShadowTint;
		FLinearColor BorderTint;
		GetAnimationValues(BorderIntroAnimation.IsPlaying(),  BorderIntroAnimation.IsPlaying() ? BorderIntroAnimation.GetLerp() : BorderPulseAnimation.GetLerpLooping(), AlphaFactor, PulseFactor, ShadowTint, BorderTint);

		const FSlateBrush* ShadowBrush = FCoreStyle::Get().GetBrush(TEXT("Tutorials.Shadow"));
		const FSlateBrush* BorderBrush = FCoreStyle::Get().GetBrush(TEXT("Tutorials.Border"));
					
		const FGeometry& WidgetGeometry = CachedGeometry;
		const FVector2D WindowSize = OutDrawElements.GetWindow()->GetSizeInScreen();

		// We should be clipped by the window size, not our containing widget, as we want to draw outside the widget
		FSlateRect WindowClippingRect(0.0f, 0.0f, WindowSize.X, WindowSize.Y);

		FPaintGeometry ShadowGeometry((WidgetGeometry.AbsolutePosition - FVector2D(ShadowBrush->Margin.Left, ShadowBrush->Margin.Top) * ShadowBrush->ImageSize * WidgetGeometry.Scale * TutorialConstants::ShadowScale),
										((WidgetGeometry.Size * WidgetGeometry.Scale) + (FVector2D(ShadowBrush->Margin.Right * 2.0f, ShadowBrush->Margin.Bottom * 2.0f) * ShadowBrush->ImageSize * WidgetGeometry.Scale * TutorialConstants::ShadowScale)),
										WidgetGeometry.Scale * TutorialConstants::ShadowScale);
		// draw highlight shadow
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, ShadowGeometry, ShadowBrush, WindowClippingRect, ESlateDrawEffect::None, ShadowTint);

		FVector2D PulseOffset = FVector2D(PulseFactor * TutorialConstants::MaxBorderOffset, PulseFactor * TutorialConstants::MaxBorderOffset);

		FVector2D BorderPosition = (WidgetGeometry.AbsolutePosition - ((FVector2D(BorderBrush->Margin.Left, BorderBrush->Margin.Top) * BorderBrush->ImageSize * WidgetGeometry.Scale) + PulseOffset));
		FVector2D BorderSize = ((WidgetGeometry.Size * WidgetGeometry.Scale) + (PulseOffset * 2.0f) + (FVector2D(BorderBrush->Margin.Right * 2.0f, BorderBrush->Margin.Bottom * 2.0f) * BorderBrush->ImageSize * WidgetGeometry.Scale));

		FPaintGeometry BorderGeometry(BorderPosition, BorderSize, WidgetGeometry.Scale);

		// draw highlight border
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, BorderGeometry, BorderBrush, WindowClippingRect, ESlateDrawEffect::None, BorderTint);
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

/** Helper function to generate title widget, if any */
static TSharedRef<SWidget> GetStageTitle(const FExcerpt& InExcerpt, int32 InCurrentExcerptIndex)
{
	// First try for unadorned 'StageTitle'
	FString VariableName(TEXT("StageTitle"));
	const FString* VariableValue = InExcerpt.Variables.Find(VariableName);
	if(VariableValue != NULL)
	{
		return SNew(STextBlock)
			.Text(FText::FromString(*VariableValue))
			.TextStyle(FEditorStyle::Get(), "Tutorials.CurrentExcerpt");
	}

	// Then try 'StageTitle<StageNum>'
	VariableName = FString::Printf(TEXT("StageTitle%d"), InCurrentExcerptIndex + 1);
	VariableValue = InExcerpt.Variables.Find(VariableName);
	if(VariableValue != NULL)
	{
		return SNew(STextBlock)
			.Text(FText::FromString(*VariableValue))
			.TextStyle(FEditorStyle::Get(), "Tutorials.CurrentExcerpt");
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> STutorialContent::GenerateContentWidget(const FTutorialContent& InContent, float WrapTextAt, bool bForWidget, TSharedPtr<IDocumentationPage>& OutDocumentationPage)
{
	// Style for the documentation
	static FDocumentationStyle DocumentationStyle;
	DocumentationStyle
		.ContentStyle(TEXT("Tutorials.Content"))
		.BoldContentStyle(TEXT("Tutorials.BoldContent"))
		.NumberedContentStyle(TEXT("Tutorials.NumberedContent"))
		.Header1Style(TEXT("Tutorials.Header1"))
		.Header2Style(TEXT("Tutorials.Header2"))
		.HyperlinkButtonStyle(TEXT("Tutorials.Hyperlink.Button"))
		.HyperlinkTextStyle(TEXT("Tutorials.Hyperlink.Text"))
		.SeparatorStyle(TEXT("Tutorials.Separator"));

	switch(InContent.Type)
	{
	case ETutorialContent::Text:
		{
			OutDocumentationPage = nullptr;

			return SNew(STextBlock)
				.Visibility(EVisibility::SelfHitTestInvisible)
				.WrapTextAt(WrapTextAt)
				.Text(InContent.Text)
				.TextStyle(FEditorStyle::Get(), bForWidget ? "Tutorials.WidgetContent" : "Tutorials.Content");
		}

	case ETutorialContent::UDNExcerpt:
		if (IDocumentation::Get()->PageExists(InContent.Content))
		{
			OutDocumentationPage = IDocumentation::Get()->GetPage(InContent.Content, TSharedPtr<FParserConfiguration>(), DocumentationStyle);
			FExcerpt Excerpt;
			if(OutDocumentationPage->GetExcerpt(InContent.ExcerptName, Excerpt) && OutDocumentationPage->GetExcerptContent(Excerpt))
			{
				return SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.HAlign(HAlign_Left)
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 5.0f)
					[
						GetStageTitle(Excerpt, 0)
					]
					+SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.AutoHeight()
					[
						Excerpt.Content.ToSharedRef()
					];
			}
		}
		break;
	}

	return SNullWidget::NullWidget;
}


FVector2D STutorialContent::GetPosition() const
{
	float XOffset = 0.0f;
	switch(HorizontalAlignment.Get())
	{
	case HAlign_Left:
		XOffset = -(ContentWidget->GetDesiredSize().X - ContentOffset);
		break;
	default:
	case HAlign_Fill:
	case HAlign_Center:
		XOffset = (CachedGeometry.Size.X * 0.5f) - (ContentWidget->GetDesiredSize().X * 0.5f);
		break;
	case HAlign_Right:
		XOffset = CachedGeometry.Size.X - ContentOffset;
		break;
	}

	XOffset += WidgetOffset.Get().X;

	float YOffset = 0.0f;
	switch(VerticalAlignment.Get())
	{
	case VAlign_Top:
		YOffset = -(ContentWidget->GetDesiredSize().Y - ContentOffset);
		break;
	default:
	case VAlign_Fill:
	case VAlign_Center:
		YOffset = (CachedGeometry.Size.Y * 0.5f) - (ContentWidget->GetDesiredSize().Y * 0.5f);
		break;
	case VAlign_Bottom:
		YOffset = (CachedGeometry.Size.Y - ContentOffset);
		break;
	}

	YOffset += WidgetOffset.Get().Y;

	// now build & clamp to area
	FVector2D BaseOffset = FVector2D(CachedGeometry.AbsolutePosition.X + XOffset, CachedGeometry.AbsolutePosition.Y + YOffset);
	BaseOffset.X = FMath::Clamp(BaseOffset.X, 0.0f, CachedWindowSize.X - ContentWidget->GetDesiredSize().X);
	BaseOffset.Y = FMath::Clamp(BaseOffset.Y, 0.0f, CachedWindowSize.Y - ContentWidget->GetDesiredSize().Y);
	return BaseOffset;
}

FVector2D STutorialContent::GetSize() const
{
	return ContentWidget->GetDesiredSize();
}

FReply STutorialContent::OnCloseButtonClicked()
{
	OnClosed.ExecuteIfBound();

	return FReply::Handled();
}

EVisibility STutorialContent::GetCloseButtonVisibility() const
{
	return bIsStandalone ? EVisibility::Visible : EVisibility::Collapsed;
}

void STutorialContent::HandlePaintNamedWidget(TSharedRef<SWidget> InWidget, const FGeometry& InGeometry)
{
	switch(Anchor.Type)
	{
	case ETutorialAnchorIdentifier::NamedWidget:
		if(Anchor.WrapperIdentifier == InWidget->GetTag())
		{
			bIsVisible = true;
			CachedGeometry = InGeometry;
		}
		break;
	}
}

void STutorialContent::HandleResetNamedWidget()
{
	bIsVisible = false;
}

void STutorialContent::HandleCacheWindowSize(const FVector2D& InWindowSize)
{
	CachedWindowSize = InWindowSize;
}

EVisibility STutorialContent::GetVisibility() const
{
	return bIsVisible || Anchor.Type == ETutorialAnchorIdentifier::None ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
}