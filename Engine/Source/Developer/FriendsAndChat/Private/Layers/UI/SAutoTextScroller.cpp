// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SAutoTextScroller.h"

namespace EScrollerState
{
	enum Type
	{
		Start = 0,
		Scrolling,
		End
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
				.ColorAndOpacity(FontStyle.DefaultDullFontColor)
				.Text(InArgs._Text)
			]
		]);

		TimeElapsed = 0.f;
		ScrollOffset = 0;
		ScrollerState = EScrollerState::Start;
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		const float ContentSize = ScrollBox->GetDesiredSize().X;
		const float Speed = 20.0f;
		const float StartDelay = 2.0f;
		const float EndDelay = 2.0f;

		TimeElapsed += InDeltaTime;

		switch (ScrollerState)
		{
			case EScrollerState::Start:
			{
				auto PinnedActiveTimerHandle = ActiveTimerHandle.Pin();
				if (PinnedActiveTimerHandle.IsValid())
				{
					UnRegisterActiveTimer(PinnedActiveTimerHandle.ToSharedRef());
				}
				ActiveTimerHandle = RegisterActiveTimer(StartDelay, FWidgetActiveTimerDelegate::CreateSP(this, &SAutoTextScrollerImpl::UpdateScrollerState));
			}break;
			case EScrollerState::Scrolling:
			{
				ScrollOffset += Speed * InDeltaTime;
				if (ExternalScrollbar->DistanceFromBottom()==0.0f)
				{
					ScrollOffset = ContentSize;
					TimeElapsed = 0.0f;
					ScrollerState = EScrollerState::End;
				}
				
			}break;
			case EScrollerState::End:
			{
				auto PinnedActiveTimerHandle = ActiveTimerHandle.Pin();
				if (PinnedActiveTimerHandle.IsValid())
				{
					UnRegisterActiveTimer(PinnedActiveTimerHandle.ToSharedRef());
				}
				ActiveTimerHandle = RegisterActiveTimer(EndDelay, FWidgetActiveTimerDelegate::CreateSP(this, &SAutoTextScrollerImpl::UpdateScrollerState));
			}break;
		};
		ScrollBox->SetScrollOffset(ScrollOffset);
	}

	EActiveTimerReturnType UpdateScrollerState(double InCurrentTime, float InDeltaTime)
	{
		if (ScrollerState == EScrollerState::End)
		{
			TimeElapsed = 0.0f;
			ScrollOffset = 0.0f;
			ScrollerState = EScrollerState::Start;
			ScrollBox->SetScrollOffset(ScrollOffset);
		}
		else if (ScrollerState == EScrollerState::Start)
		{
			TimeElapsed = 0.0f;
			ScrollOffset = 0.0f;
			ScrollerState = EScrollerState::Scrolling;
			ScrollBox->SetScrollOffset(ScrollOffset);
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
};

TSharedRef<SAutoTextScroller> SAutoTextScroller::New()
{
	return MakeShareable(new SAutoTextScrollerImpl());
}
