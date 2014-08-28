// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "STutorialButton.h"
#include "EditorTutorialSettings.h"
#include "TutorialStateSettings.h"

#define LOCTEXT_NAMESPACE "STutorialButton"

namespace TutorialButtonConstants
{
	const float MaxPulseOffset = 32.0f;
	const float PulseAnimationLength = 2.0f;
}

void STutorialButton::Construct(const FArguments& InArgs)
{
	Context = InArgs._Context;
	ContextWindow = InArgs._ContextWindow;

	bTutorialAvailable = false;
	bTutorialCompleted = false;
	bTutorialDismissed = false;
	bDeferTutorialOpen = true;

	PulseAnimation.AddCurve(0.0f, TutorialButtonConstants::PulseAnimationLength, ECurveEaseFunction::Linear);
	PulseAnimation.Play();

	ChildSlot
	[
		SNew(SButton)
		.Tag(*FString::Printf(TEXT("%s.TutorialLaunchButton"), *Context.ToString()))
		.ButtonStyle(FEditorStyle::Get(), "TutorialLaunch.Button")
		.ToolTipText(LOCTEXT("TutorialLaunchToolTip", "Launch Tutorial (right-click for more options)"))
		.Visibility(this, &STutorialButton::GetVisibility)
		.OnClicked(this, &STutorialButton::HandleButtonClicked)
		.ContentPadding(0.0f)
		[
			SNew(SBox)
			.WidthOverride(16)
			.HeightOverride(16)
		]
	];
}

void STutorialButton::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (bDeferTutorialOpen)
	{
		UEditorTutorial* AttractTutorial = nullptr;
		UEditorTutorial* LaunchTutorial = nullptr;
		GetDefault<UEditorTutorialSettings>()->FindTutorialsForContext(Context, AttractTutorial, LaunchTutorial);

		bTutorialAvailable = (LaunchTutorial != nullptr);
		bTutorialCompleted = (LaunchTutorial != nullptr) && GetDefault<UTutorialStateSettings>()->HaveCompletedTutorial(LaunchTutorial);
		bTutorialDismissed = (AttractTutorial != nullptr) && GetDefault<UTutorialStateSettings>()->IsTutorialDismissed(AttractTutorial);
		if (bTutorialAvailable && AttractTutorial != nullptr && !bTutorialDismissed && !bTutorialCompleted)
		{
			// kick off the attract tutorial if the user hasn't dismissed it and hasn't completed it
			FIntroTutorials& IntroTutorials = FModuleManager::GetModuleChecked<FIntroTutorials>(TEXT("IntroTutorials"));
			const bool bRestart = true;
			IntroTutorials.LaunchTutorial(AttractTutorial, bRestart, ContextWindow);
		}
	}
	bDeferTutorialOpen = false;
}

static void GetAnimationValues(float InAnimationProgress, float& OutAlphaFactor0, float& OutPulseFactor0, float& OutAlphaFactor1, float& OutPulseFactor1)
{
	InAnimationProgress = FMath::Fmod(InAnimationProgress * 2.0f, 1.0f);

	OutAlphaFactor0 = FMath::Square(1.0f - InAnimationProgress);
	OutPulseFactor0 = 1.0f - FMath::Square(1.0f - InAnimationProgress);

	float OffsetProgress = FMath::Fmod(InAnimationProgress + 0.25f, 1.0f);
	OutAlphaFactor1 = FMath::Square(1.0f - OffsetProgress);
	OutPulseFactor1 = 1.0f - FMath::Square(1.0f - OffsetProgress);
}

int32 STutorialButton::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1000;

	if (!(bTutorialCompleted || bTutorialDismissed))
	{
		float AlphaFactor0 = 0.0f;
		float AlphaFactor1 = 0.0f;
		float PulseFactor0 = 0.0f;
		float PulseFactor1 = 0.0f;
		GetAnimationValues(PulseAnimation.GetLerpLooping(), AlphaFactor0, PulseFactor0, AlphaFactor1, PulseFactor1);

		const FSlateBrush* PulseBrush = FEditorStyle::Get().GetBrush(TEXT("TutorialLaunch.Circle"));
		const FLinearColor PulseColor = FEditorStyle::Get().GetColor(TEXT("TutorialLaunch.Circle.Color"));

		// We should be clipped by the window size, not our containing widget, as we want to draw outside the widget
		const FVector2D WindowSize = OutDrawElements.GetWindow()->GetSizeInScreen();
		FSlateRect WindowClippingRect(0.0f, 0.0f, WindowSize.X, WindowSize.Y);

		{
			FVector2D PulseOffset = FVector2D(PulseFactor0 * TutorialButtonConstants::MaxPulseOffset, PulseFactor0 * TutorialButtonConstants::MaxPulseOffset);

			FVector2D BorderPosition = (AllottedGeometry.AbsolutePosition - ((FVector2D(PulseBrush->Margin.Left, PulseBrush->Margin.Top) * PulseBrush->ImageSize * AllottedGeometry.Scale) + PulseOffset));
			FVector2D BorderSize = ((AllottedGeometry.Size * AllottedGeometry.Scale) + (PulseOffset * 2.0f) + (FVector2D(PulseBrush->Margin.Right * 2.0f, PulseBrush->Margin.Bottom * 2.0f) * PulseBrush->ImageSize * AllottedGeometry.Scale));

			FPaintGeometry BorderGeometry(BorderPosition, BorderSize, AllottedGeometry.Scale);

			// draw highlight border
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, BorderGeometry, PulseBrush, WindowClippingRect, ESlateDrawEffect::None, FLinearColor(PulseColor.R, PulseColor.G, PulseColor.B, AlphaFactor0));
		}

		{
			FVector2D PulseOffset = FVector2D(PulseFactor1 * TutorialButtonConstants::MaxPulseOffset, PulseFactor1 * TutorialButtonConstants::MaxPulseOffset);

			FVector2D BorderPosition = (AllottedGeometry.AbsolutePosition - ((FVector2D(PulseBrush->Margin.Left, PulseBrush->Margin.Top) * PulseBrush->ImageSize * AllottedGeometry.Scale) + PulseOffset));
			FVector2D BorderSize = ((AllottedGeometry.Size * AllottedGeometry.Scale) + (PulseOffset * 2.0f) + (FVector2D(PulseBrush->Margin.Right * 2.0f, PulseBrush->Margin.Bottom * 2.0f) * PulseBrush->ImageSize * AllottedGeometry.Scale));

			FPaintGeometry BorderGeometry(BorderPosition, BorderSize, AllottedGeometry.Scale);

			// draw highlight border
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, BorderGeometry, PulseBrush, WindowClippingRect, ESlateDrawEffect::None, FLinearColor(PulseColor.R, PulseColor.G, PulseColor.B, AlphaFactor1));
		}
	}

	return LayerId;
}

FReply STutorialButton::HandleButtonClicked()
{
	UEditorTutorial* AttractTutorial = nullptr;
	UEditorTutorial* LaunchTutorial = nullptr;
	GetDefault<UEditorTutorialSettings>()->FindTutorialsForContext(Context, AttractTutorial, LaunchTutorial);
	if (LaunchTutorial != nullptr && ContextWindow.IsValid())
	{
		FIntroTutorials& IntroTutorials = FModuleManager::GetModuleChecked<FIntroTutorials>(TEXT("IntroTutorials"));
		const bool bRestart = true;
		IntroTutorials.LaunchTutorial(LaunchTutorial, bRestart, ContextWindow);
	}

	return FReply::Handled();
}

EVisibility STutorialButton::GetVisibility() const
{
	return bTutorialAvailable ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply STutorialButton::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		const bool bInShouldCloseWindowAfterMenuSelection = true;
		FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("DismissReminder", "Dismiss Alert"),
			LOCTEXT("DismissReminderTooltip", "Don't show me this alert again"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &STutorialButton::DismissAlert))
			);

		FSlateApplication::Get().PushMenu(SharedThis(this), MenuBuilder.MakeWidget(), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
	}
	return FReply::Handled();
}

void STutorialButton::DismissAlert()
{
	UEditorTutorial* AttractTutorial = nullptr;
	UEditorTutorial* LaunchTutorial = nullptr;
	GetDefault<UEditorTutorialSettings>()->FindTutorialsForContext(Context, AttractTutorial, LaunchTutorial);
	if (AttractTutorial != nullptr)
	{
		const bool bDismissAcrossSessions = true;
		GetMutableDefault<UTutorialStateSettings>()->DismissTutorial(AttractTutorial, bDismissAcrossSessions);
		GetMutableDefault<UTutorialStateSettings>()->SaveProgress();
		bTutorialDismissed = true;

		FIntroTutorials& IntroTutorials = FModuleManager::GetModuleChecked<FIntroTutorials>(TEXT("IntroTutorials"));
		IntroTutorials.CloseAllTutorialContent();
	}
}

#undef LOCTEXT_NAMESPACE