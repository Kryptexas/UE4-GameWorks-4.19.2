// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SAutoTextScroller.h"

namespace EScrollerState
{
	enum Type
	{
		FadeOn = 0,
		Start,
		Start_Wait,
		Scrolling,
		Stop,
		Stop_Wait,
		FadeOff
	};
}

class SAutoTextScrollerImpl : public SAutoTextScroller
{
public:

	void Construct(const FArguments& InArgs)
	{
		FontStyle = *InArgs._FontStyle;

		SAssignNew(ExternalScrollbar, SScrollBar);
		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SAssignNew(ScrollBox, SScrollBox)
			.ScrollBarVisibility(EVisibility::Collapsed)
			.Orientation(Orient_Horizontal)
			.ExternalScrollbar(ExternalScrollbar)
			+ SScrollBox::Slot()
			[
				SNew(STextBlock)
				.Font(FontStyle.FriendsFontSmallBold)
				.ColorAndOpacity(this, &SAutoTextScrollerImpl::GetFontColor)
				.Text(InArgs._Text)
				//.Text(NSLOCTEXT("ughu", "eref", "This is a very long sentence to test the auto scroller 5 4 3 2 1 ."))
			]
		]);

		TimeElapsed = 0.f;
		ScrollOffset = 0;
		FontAlpha = 1.0f;
		ScrollerState = EScrollerState::Start;
	}

	FSlateColor GetFontColor() const
	{
		FLinearColor RetVal = FontStyle.DefaultDullFontColor;
		RetVal.A = FontAlpha;
		return RetVal;
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		const float ContentSize = ScrollBox->GetDesiredSize().X;
		const float Speed = 20.0f;
		const float StartDelay = 2.0f;
		const float EndDelay = 2.0f;
		const float FadeOnDelay = 0.5f;
		const float FadeOffDelay = 0.5f;

		TimeElapsed += InDeltaTime;

		switch (ScrollerState)
		{
			case EScrollerState::FadeOn:
			{
				FontAlpha = TimeElapsed / FadeOnDelay;
				if (TimeElapsed >= FadeOnDelay)
				{
					FontAlpha = 1.0f;
					TimeElapsed = 0.0f;
					ScrollOffset = 0.0f;
					ScrollerState = EScrollerState::Start;
				}
			}break;
			case EScrollerState::Start:
			{
				// Unregister Tick Delegate
				auto PinnedActiveTimerHandle = ActiveTimerHandle.Pin();
				if (PinnedActiveTimerHandle.IsValid())
				{
					UnRegisterActiveTimer(PinnedActiveTimerHandle.ToSharedRef());
				}

				// Check to see if we need to scroll
				if (ExternalScrollbar->DistanceFromBottom() == 0.0f && ExternalScrollbar->DistanceFromTop() == 0.0f)
				{
					// Don't run auto scrolling if text already fits.
					break;
				}
				else
				{
					ActiveTimerHandle = RegisterActiveTimer(StartDelay, FWidgetActiveTimerDelegate::CreateSP(this, &SAutoTextScrollerImpl::UpdateScrollerState));
					ScrollerState = EScrollerState::Start_Wait;
				}
			}break;
			case EScrollerState::Start_Wait:
				break;
			case EScrollerState::Scrolling:
			{
				ScrollOffset += Speed * InDeltaTime;
				if (ExternalScrollbar->DistanceFromBottom()==0.0f)
				{
					ScrollOffset = ContentSize;
					TimeElapsed = 0.0f;
					ScrollerState = EScrollerState::Stop;
				}
				
			}break;
			case EScrollerState::Stop:
			{
				// Unregister Tick Delegate
				auto PinnedActiveTimerHandle = ActiveTimerHandle.Pin();
				if (PinnedActiveTimerHandle.IsValid())
				{
					UnRegisterActiveTimer(PinnedActiveTimerHandle.ToSharedRef());
				}

				// Check to see if we need to scroll
				if (ExternalScrollbar->DistanceFromBottom() == 0.0f && ExternalScrollbar->DistanceFromTop() == 0.0f)
				{
					// Don't run auto scrolling if text already fits.
					break;
				}
				else
				{
					ActiveTimerHandle = RegisterActiveTimer(EndDelay, FWidgetActiveTimerDelegate::CreateSP(this, &SAutoTextScrollerImpl::UpdateScrollerState));
					ScrollerState = EScrollerState::Stop_Wait;
				}
			}break;
			case EScrollerState::Stop_Wait:
				break;
			case EScrollerState::FadeOff:
			{
				FontAlpha = 1.0f - TimeElapsed / FadeOffDelay;
				if (TimeElapsed >= FadeOffDelay)
				{
					FontAlpha = 0.0f;
					TimeElapsed = 0.0f;
					ScrollOffset = 0.0f;
					ScrollerState = EScrollerState::FadeOn;
				}
			}break;
		};
		ScrollBox->SetScrollOffset(ScrollOffset);
	}

	EActiveTimerReturnType UpdateScrollerState(double InCurrentTime, float InDeltaTime)
	{
		if (ScrollerState == EScrollerState::Stop_Wait)
		{
			TimeElapsed = 0.0f;
			ScrollerState = EScrollerState::FadeOff;
			ScrollBox->SetScrollOffset(ScrollOffset);

			auto PinnedActiveTimerHandle = ActiveTimerHandle.Pin();
			if (PinnedActiveTimerHandle.IsValid())
			{
				UnRegisterActiveTimer(PinnedActiveTimerHandle.ToSharedRef());
			}
			ActiveTimerHandle = RegisterActiveTimer(0, FWidgetActiveTimerDelegate::CreateSP(this, &SAutoTextScrollerImpl::UpdateScrollerState));
		}
		else if (ScrollerState == EScrollerState::Start_Wait)
		{
			TimeElapsed = 0.0f;
			ScrollOffset = 0.0f;
			ScrollerState = EScrollerState::Scrolling;
			ScrollBox->SetScrollOffset(ScrollOffset);

			auto PinnedActiveTimerHandle = ActiveTimerHandle.Pin();
			if (PinnedActiveTimerHandle.IsValid())
			{
				UnRegisterActiveTimer(PinnedActiveTimerHandle.ToSharedRef());
			}
			ActiveTimerHandle = RegisterActiveTimer(0, FWidgetActiveTimerDelegate::CreateSP(this, &SAutoTextScrollerImpl::UpdateScrollerState));
		}
		return EActiveTimerReturnType::Continue;
	}

private:

	/** Holds the style to use when making the widget. */
	FFriendsFontStyle FontStyle;
	TSharedPtr<SScrollBox> ScrollBox;
	TWeakPtr<FActiveTimerHandle> ActiveTimerHandle;
	TSharedPtr<SScrollBar> ExternalScrollbar;
	EScrollerState::Type ScrollerState;
	float TimeElapsed;
	float ScrollOffset;
	float FontAlpha;
};

TSharedRef<SAutoTextScroller> SAutoTextScroller::New()
{
	return MakeShareable(new SAutoTextScrollerImpl());
}
