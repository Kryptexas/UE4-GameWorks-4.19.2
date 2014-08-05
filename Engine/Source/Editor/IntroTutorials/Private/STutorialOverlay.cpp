// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "STutorialOverlay.h"
#include "STutorialContent.h"
#include "EditorTutorial.h"
#include "IntroTutorials.h"

void STutorialOverlay::Construct(const FArguments& InArgs, FTutorialStage* const InStage)
{
	ParentWindow = InArgs._ParentWindow;
	bIsStandalone = InArgs._IsStandalone;
	OnClosed = InArgs._OnClosed;

	TSharedPtr<SOverlay> Overlay;

	ChildSlot
	[
		SAssignNew(Overlay, SOverlay)
		+SOverlay::Slot()
		[
			SAssignNew(OverlayCanvas, SCanvas)
		]
	];

	if(InStage != nullptr)
	{
		// add non-widget content, if any
		if(InArgs._AllowNonWidgetContent && InStage->Content.Type != ETutorialContent::None)
		{
			Overlay->AddSlot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(STutorialContent, InStage->Content)
					.OnClosed(InArgs._OnClosed)
					.IsStandalone(InArgs._IsStandalone)
					.WrapTextAt(600.0f)
				]
			];
		}

		if(InStage->WidgetContent.Num() > 0)
		{
			FIntroTutorials& IntroTutorials = FModuleManager::Get().GetModuleChecked<FIntroTutorials>("IntroTutorials");

			// now add canvas slots for widget-bound content
			for(const FTutorialWidgetContent& WidgetContent : InStage->WidgetContent)
			{
				if(WidgetContent.Content.Type != ETutorialContent::None)
				{
					TSharedPtr<STutorialContent> ContentWidget = 
						SNew(STutorialContent, WidgetContent.Content)
						.HAlign(WidgetContent.HorizontalAlignment)
						.VAlign(WidgetContent.VerticalAlignment)
						.Offset(WidgetContent.Offset)
						.IsStandalone(bIsStandalone)
						.OnClosed(OnClosed)
						.WrapTextAt(WidgetContent.ContentWidth)
						.Anchor(WidgetContent.WidgetAnchor);

					OverlayCanvas->AddSlot()
					.Position(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(ContentWidget.Get(), &STutorialContent::GetPosition)))
					.Size(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(ContentWidget.Get(), &STutorialContent::GetSize)))
					[
						ContentWidget.ToSharedRef()
					];

					OnPaintNamedWidget.AddSP(ContentWidget.Get(), &STutorialContent::HandlePaintNamedWidget);
					OnResetNamedWidget.AddSP(ContentWidget.Get(), &STutorialContent::HandleResetNamedWidget);
					OnCacheWindowSize.AddSP(ContentWidget.Get(), &STutorialContent::HandleCacheWindowSize);
				}
			}
		}
	}
}

int32 STutorialOverlay::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	if(ParentWindow.IsValid())
	{
		TSharedPtr<SWindow> PinnedWindow = ParentWindow.Pin();
		OnResetNamedWidget.Broadcast();
		OnCacheWindowSize.Broadcast(PinnedWindow->GetWindowGeometryInWindow().Size);
		LayerId = TraverseWidgets(PinnedWindow.ToSharedRef(), PinnedWindow->GetWindowGeometryInWindow(), MyClippingRect, OutDrawElements, LayerId);
	}
	
	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

int32 STutorialOverlay::TraverseWidgets(TSharedRef<SWidget> InWidget, const FGeometry& InGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	const FName Tag = InWidget->GetTag();
	if(Tag != NAME_None)
	{
		// we are a named widget - ask it to draw
		OnPaintNamedWidget.Broadcast(InWidget, InGeometry);

		// if we are picking, we need to draw an outline here
		FName WidgetNameToHighlight = NAME_None;
		bool bIsPicking = false;
		FIntroTutorials& IntroTutorials = FModuleManager::Get().GetModuleChecked<FIntroTutorials>("IntroTutorials");
		if(IntroTutorials.OnIsPicking().IsBound())
		{
			bIsPicking = IntroTutorials.OnIsPicking().Execute(WidgetNameToHighlight);
		}
	
		if(WidgetNameToHighlight != NAME_None || bIsPicking)
		{
			const bool bHighlight = WidgetNameToHighlight != NAME_None && WidgetNameToHighlight == Tag;
			if(bIsPicking || (!bIsPicking && bHighlight))
			{
				const FLinearColor Color = bIsPicking && bHighlight ? FLinearColor::Green : FLinearColor::White;
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, InGeometry.ToPaintGeometry(), FCoreStyle::Get().GetBrush(TEXT("Debug.Border")), MyClippingRect, ESlateDrawEffect::None, Color);
			}
		}	
	}

	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	InWidget->ArrangeChildren(InGeometry, ArrangedChildren);
	for(int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ChildIndex++)
	{
		const FArrangedWidget& ArrangedWidget = ArrangedChildren(ChildIndex);
		LayerId = TraverseWidgets(ArrangedWidget.Widget, ArrangedWidget.Geometry, MyClippingRect, OutDrawElements, LayerId);
	}

	return LayerId;
}