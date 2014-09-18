// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "STutorialButton.h"
#include "EditorTutorialSettings.h"
#include "TutorialStateSettings.h"
#include "TutorialMetaData.h"

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
	AlertStartTime = 0.0f;

	PulseAnimation.AddCurve(0.0f, TutorialButtonConstants::PulseAnimationLength, ECurveEaseFunction::Linear);
	PulseAnimation.Play();

	ChildSlot
	[
		SNew(SButton)
		.AddMetaData<FTagMetaData>(*FString::Printf(TEXT("%s.TutorialLaunchButton"), *Context.ToString()))
		.ButtonStyle(FEditorStyle::Get(), "TutorialLaunch.Button")
		.ToolTipText(this, &STutorialButton::GetButtonToolTip)
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
		FString BrowserFilter;
		GetDefault<UEditorTutorialSettings>()->FindTutorialInfoForContext(Context, AttractTutorial, LaunchTutorial, BrowserFilter);

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

		if(ShouldShowAlert())
		{
			AlertStartTime = FPlatformTime::Seconds();
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

	if (ShouldShowAlert())
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
	FString BrowserFilter;
	GetDefault<UEditorTutorialSettings>()->FindTutorialInfoForContext(Context, AttractTutorial, LaunchTutorial, BrowserFilter);

	bTutorialAvailable = (LaunchTutorial != nullptr);
	bTutorialCompleted = (LaunchTutorial != nullptr) && GetDefault<UTutorialStateSettings>()->HaveCompletedTutorial(LaunchTutorial);
	bTutorialDismissed = (AttractTutorial != nullptr) && GetDefault<UTutorialStateSettings>()->IsTutorialDismissed(AttractTutorial);

	if(ContextWindow.IsValid())
	{
		if( FEngineAnalytics::IsAvailable() )
		{
			TArray<FAnalyticsEventAttribute> EventAttributes;
			EventAttributes.Add(FAnalyticsEventAttribute(TEXT("Context"), Context.ToString()));
			EventAttributes.Add(FAnalyticsEventAttribute(TEXT("TimeSinceAlertStarted"), (AlertStartTime != 0.0f && ShouldShowAlert()) ? (FPlatformTime::Seconds() - AlertStartTime) : -1.0f));
			EventAttributes.Add(FAnalyticsEventAttribute(TEXT("LaunchedBrowser"), ShouldLaunchBrowser());

			FEngineAnalytics::GetProvider().RecordEvent( TEXT("Rocket.Tutorials.ClickedContextButton"), EventAttributes );
		}

		FIntroTutorials& IntroTutorials = FModuleManager::GetModuleChecked<FIntroTutorials>(TEXT("IntroTutorials"));
		if(ShouldLaunchBrowser())
		{
			IntroTutorials.SummonTutorialBrowser(ContextWindow.Pin().ToSharedRef(), BrowserFilter);
		}
		else if (LaunchTutorial != nullptr)
		{
			const bool bRestart = true;
			IntroTutorials.LaunchTutorial(LaunchTutorial, bRestart, ContextWindow);
		}
	}

	return FReply::Handled();
}

FReply STutorialButton::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if(!ShouldLaunchBrowser())
	{
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			const bool bInShouldCloseWindowAfterMenuSelection = true;
			FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr);

			if(ShouldShowAlert())
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("DismissReminder", "Dismiss Alert"),
					LOCTEXT("DismissReminderTooltip", "Don't show me this alert again"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &STutorialButton::DismissAlert))
					);
			}

			MenuBuilder.AddMenuEntry(
				LOCTEXT("LaunchBrowser", "Show Available Tutorials"),
				LOCTEXT("LaunchBrowserTooltip", "Display the tutorials browser"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &STutorialButton::LaunchBrowser))
				);


			FSlateApplication::Get().PushMenu(SharedThis(this), MenuBuilder.MakeWidget(), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
		}
	}
	return FReply::Handled();
}

void STutorialButton::DismissAlert()
{
	UEditorTutorial* AttractTutorial = nullptr;
	UEditorTutorial* LaunchTutorial = nullptr;
	FString BrowserFilter;
	GetDefault<UEditorTutorialSettings>()->FindTutorialInfoForContext(Context, AttractTutorial, LaunchTutorial, BrowserFilter);
	if (AttractTutorial != nullptr)
	{
		if( FEngineAnalytics::IsAvailable() )
		{
			TArray<FAnalyticsEventAttribute> EventAttributes;
			EventAttributes.Add(FAnalyticsEventAttribute(TEXT("Context"), Context.ToString()));
			EventAttributes.Add(FAnalyticsEventAttribute(TEXT("TimeSinceAlertStarted"), (AlertStartTime != 0.0f && ShouldShowAlert()) ? (FPlatformTime::Seconds() - AlertStartTime) : -1.0f));

			FEngineAnalytics::GetProvider().RecordEvent( TEXT("Rocket.Tutorials.DismissedTutorialAlert"), EventAttributes );
		}

		const bool bDismissAcrossSessions = true;
		GetMutableDefault<UTutorialStateSettings>()->DismissTutorial(AttractTutorial, bDismissAcrossSessions);
		GetMutableDefault<UTutorialStateSettings>()->SaveProgress();
		bTutorialDismissed = true;

		FIntroTutorials& IntroTutorials = FModuleManager::GetModuleChecked<FIntroTutorials>(TEXT("IntroTutorials"));
		IntroTutorials.CloseAllTutorialContent();
	}
}

void STutorialButton::LaunchBrowser()
{
	if(ContextWindow.IsValid())
	{
		UEditorTutorial* AttractTutorial = nullptr;
		UEditorTutorial* LaunchTutorial = nullptr;
		FString BrowserFilter;
		GetDefault<UEditorTutorialSettings>()->FindTutorialInfoForContext(Context, AttractTutorial, LaunchTutorial, BrowserFilter);

		FIntroTutorials& IntroTutorials = FModuleManager::GetModuleChecked<FIntroTutorials>(TEXT("IntroTutorials"));
		IntroTutorials.SummonTutorialBrowser(ContextWindow.Pin().ToSharedRef(), BrowserFilter);
	}
}

bool STutorialButton::ShouldLaunchBrowser() const
{
	return (!bTutorialAvailable || (bTutorialAvailable && bTutorialCompleted));
}

bool STutorialButton::ShouldShowAlert() const
{
	return (bTutorialAvailable && !(bTutorialCompleted || bTutorialDismissed));
}

FText STutorialButton::GetButtonToolTip() const
{
	if(ShouldLaunchBrowser())
	{
		return LOCTEXT("TutorialLaunchBrowserToolTip", "Show Available Tutorials");
	}

	return LOCTEXT("TutorialLaunchToolTip", "Launch Tutorial (right-click for more options)");
}

#undef LOCTEXT_NAMESPACE