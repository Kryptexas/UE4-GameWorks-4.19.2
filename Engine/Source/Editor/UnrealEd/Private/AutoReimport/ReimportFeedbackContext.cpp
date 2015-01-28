// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "ReimportFeedbackContext.h"
#include "NotificationManager.h"
#include "SNotificationList.h"
#include "INotificationWidget.h"


class SWidgetStack : public SVerticalBox
{
	SLATE_BEGIN_ARGS(SWidgetStack){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, int32 InMaxNumVisible)
	{
		MaxNumVisible = InMaxNumVisible;
		SlideCurve = FCurveSequence(0.f, .5f, ECurveEaseFunction::QuadOut);

		StartSlideOffset = LerpSlideOffset = 0;
		StartSizeOffset = LerpSizeOffset = 0;
	}

	FVector2D ComputeTotalSize() const
	{
		FVector2D Size;
		for (int32 Index = 0; Index < FMath::Min(NumSlots(), MaxNumVisible); ++Index)
		{
			const FVector2D& ChildSize = Children[Index].GetWidget()->GetDesiredSize();
			if (ChildSize.X > Size.X)
			{
				Size.X = ChildSize.X;
			}
			Size.Y += ChildSize.Y + Children[Index].SlotPadding.Get().GetTotalSpaceAlong<Orient_Vertical>();
		}
		return Size;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		FVector2D Size = ComputeTotalSize();
		Size.Y -= LerpSizeOffset;
		return Size;
	}

	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
	{
		if (Children.Num() == 0)
		{
			return;
		}

		float PositionSoFar = AllottedGeometry.GetLocalSize().Y + LerpSlideOffset;

		for (int32 Index = 0; Index < NumSlots(); ++Index)
		{
			const SBoxPanel::FSlot& CurChild = Children[Index];
			const EVisibility ChildVisibility = CurChild.GetWidget()->GetVisibility();

			if (ChildVisibility != EVisibility::Collapsed)
			{
				const FVector2D ChildDesiredSize = CurChild.GetWidget()->GetDesiredSize();

				const FMargin SlotPadding(CurChild.SlotPadding.Get());
				const FVector2D SlotSize(AllottedGeometry.Size.X, ChildDesiredSize.Y + SlotPadding.GetTotalSpaceAlong<Orient_Vertical>());

				const AlignmentArrangeResult XAlignmentResult = AlignChild<Orient_Horizontal>( SlotSize.X, CurChild, SlotPadding );
				const AlignmentArrangeResult YAlignmentResult = AlignChild<Orient_Vertical>( SlotSize.Y, CurChild, SlotPadding );

				ArrangedChildren.AddWidget( ChildVisibility, AllottedGeometry.MakeChild(
					CurChild.GetWidget(),
					FVector2D( XAlignmentResult.Offset, PositionSoFar - SlotSize.Y + YAlignmentResult.Offset ),
					FVector2D( XAlignmentResult.Size, YAlignmentResult.Size )
					));

				PositionSoFar -= SlotSize.Y;
			}
		}
	}

	void Remove(const TSharedRef<SWidget>& InWidget)
	{

	}

	void Add(const TSharedRef<SWidget>& InWidget)
	{
		InsertSlot(0)
		.AutoHeight()
		[
			SNew(SWidgetStackItem)
			[
				InWidget
			]
		];

		auto Widget = Children[0].GetWidget();
		Widget->SlatePrepass();
		
		StartSlideOffset = LerpSlideOffset + Widget->GetDesiredSize().Y;
		LerpSlideOffset = StartSlideOffset;

		StartSizeOffset = ComputeTotalSize().Y - GetDesiredSize().Y;
		LerpSizeOffset = StartSizeOffset;

		SlideCurve.Play(AsShared());

		if (Children.Num() > MaxNumVisible)
		{
			auto Widget = StaticCastSharedRef<SWidgetStackItem>(Children[MaxNumVisible].GetWidget());
			if (Widget->OpacityCurve.IsPlaying())
			{
				Widget->OpacityCurve.Reverse();
			}
			else
			{
				Widget->OpacityCurve.PlayReverse(Widget);
			}
		}
	}
	
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		const float Alpha = 1.f - SlideCurve.GetLerp();

		LerpSlideOffset = StartSlideOffset * Alpha;
		LerpSizeOffset = StartSizeOffset * Alpha;

		// Delete any widgets that are now offscreen
		if (Children.Num() != 0)
		{
			float PositionSoFar = AllottedGeometry.GetLocalSize().Y + LerpSlideOffset;

			int32 Index = 0;
			for (; PositionSoFar > 0 && Index < NumSlots(); ++Index)
			{
				const SBoxPanel::FSlot& CurChild = Children[Index];
				const EVisibility ChildVisibility = CurChild.GetWidget()->GetVisibility();

				if (ChildVisibility != EVisibility::Collapsed)
				{
					const FVector2D ChildDesiredSize = CurChild.GetWidget()->GetDesiredSize();
					PositionSoFar -= ChildDesiredSize.Y + CurChild.SlotPadding.Get().GetTotalSpaceAlong<Orient_Vertical>();
				}
			}

			while (Children.Num() > MaxNumVisible + 1)
			{
				Children.RemoveAt(Children.Num() - 1);
			}
		}
	}

	class SWidgetStackItem : public SCompoundWidget
	{
		SLATE_BEGIN_ARGS(SWidgetStackItem){}
			SLATE_DEFAULT_SLOT(FArguments, Content)
			SLATE_EVENT(FSimpleDelegate, OnFadeOut)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			OpacityCurve = FCurveSequence(0.f, .5f, ECurveEaseFunction::QuadOut);
			OpacityCurve.Play(AsShared());

			ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("NoBorder"))
				.ColorAndOpacity(this, &SWidgetStackItem::GetColorAndOpacity)
				.Padding(0)
				[
					InArgs._Content.Widget
				]
			];
		}
		
		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
		{
			if (OnFadeOut.IsBound() && OpacityCurve.IsAtStart() && OpacityCurve.IsInReverse())
			{
				OnFadeOut.Execute();
				OnFadeOut = FSimpleDelegate();
			}
		}

		FLinearColor GetColorAndOpacity() const
		{
			return FLinearColor(1.f, 1.f, 1.f, OpacityCurve.GetLerp());
		}

		FCurveSequence OpacityCurve;
		FSimpleDelegate OnFadeOut;
	};

	FCurveSequence SlideCurve;

	float StartSlideOffset, LerpSlideOffset;
	float StartSizeOffset, LerpSizeOffset;

	int32 MaxNumVisible;
};


void SReimportFeedback::Add(const TSharedRef<SWidget>& Widget)
{
	WidgetStack->Add(Widget);
}

void SReimportFeedback::SetMainText(FText InText)
{
	MainText = InText;
}

FText SReimportFeedback::GetMainText() const
{
	return MainText;
}

void SReimportFeedback::Construct(const FArguments& InArgs, FText InMainText)
{
	MainText = InMainText;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(10))
		.BorderImage(FCoreStyle::Get().GetBrush("NotificationList.ItemBackground"))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.Padding(FMargin(0))
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(this, &SReimportFeedback::GetMainText)
				.Font(FCoreStyle::Get().GetFontStyle(TEXT("NotificationList.FontBold")))
			]

			+ SVerticalBox::Slot()
			.Padding(FMargin(0, 5, 0, 0))
			.AutoHeight()
			[
				SAssignNew(WidgetStack, SWidgetStack, 1)
			]
		]
	];
}

FReimportFeedbackContext::FReimportFeedbackContext()
	: MessageLog("AssetReimport")
{}

void FReimportFeedbackContext::Initialize(TSharedRef<SReimportFeedback> Widget)
{
	ShowNotificationDelay = FTimeLimit(.5f);
	NotificationContent = Widget;
	MessageLog.NewPage(Widget->GetMainText());
}

void FReimportFeedbackContext::Tick()
{
	if (!Notification.IsValid() && ShowNotificationDelay.Exceeded())
	{
		FNotificationInfo Info(SharedThis(this));
		Info.ExpireDuration = 1.f;
		Info.bFireAndForget = false;

		Notification = FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void FReimportFeedbackContext::Destroy()
{
	MessageLog.Notify();

	if (Notification.IsValid())
	{
		NotificationContent->SetVisibility(EVisibility::HitTestInvisible);
		//Notification->SetCompletionState(SNotificationItem::CS_Success);
		Notification->ExpireAndFadeout();
	}
}

void FReimportFeedbackContext::AddMessage(EMessageSeverity::Type Severity, const FText& Message)
{
	MessageLog.Message(Severity, Message);
	if (Severity >= EMessageSeverity::Error)
	{
		AddWidget(SNew(STextBlock).Text(Message));
	}
}

void FReimportFeedbackContext::AddWidget(const TSharedRef<SWidget>& Widget)
{
	NotificationContent->Add(Widget);
}

void FReimportFeedbackContext::ProgressReported(const float TotalProgressInterp, FText DisplayMessage)
{
	if (!DisplayMessage.IsEmpty())
	{
		NotificationContent->Add(SNew(STextBlock).Text(DisplayMessage));
	}
}