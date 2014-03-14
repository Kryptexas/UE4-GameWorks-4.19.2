// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SlateTextLayout.h"

TSharedRef< FSlateTextLayout > FSlateTextLayout::Create()
{
	TSharedRef< FSlateTextLayout > Layout = MakeShareable( new FSlateTextLayout() );
	Layout->AggregateChildren();

	return Layout;
}

FSlateTextLayout::FSlateTextLayout()
	: Children()
{

}

FChildren* FSlateTextLayout::GetChildren()
{
	return &Children;
}

void FSlateTextLayout::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	for (int LineIndex = 0; LineIndex < LineViews.Num(); LineIndex++)
	{
		const FTextLayout::FLineView& LineView = LineViews[ LineIndex ];

		for (int BlockIndex = 0; BlockIndex < LineView.Blocks.Num(); BlockIndex++)
		{
			const TSharedRef< ILayoutBlock > Block = LineView.Blocks[ BlockIndex ];
			const TSharedRef< ISlateRun > Run = StaticCastSharedRef< ISlateRun >( Block->GetRun() );
			Run->ArrangeChildren( Block, AllottedGeometry, ArrangedChildren );
		}
	}
}

int32 FSlateTextLayout::OnPaint( const FTextBlockStyle& DefaultTextStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	const FSlateRect ClippingRect = AllottedGeometry.GetClippingRect().IntersectionWith(MyClippingRect);
	const ESlateDrawEffect::Type DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	const int32 DebugLayer = LayerId;
	const int32 TextLayer = LayerId + 1;

	static bool ShowDebug = false;
	FLinearColor BlockHue( 0, 1.0f, 1.0f, 0.5 );
	int32 MaxLayerId = LayerId;

	for (int LineIndex = 0; LineIndex < LineViews.Num(); LineIndex++)
	{
		const FTextLayout::FLineView& LineView = LineViews[ LineIndex ];

		for (int BlockIndex = 0; BlockIndex < LineView.Blocks.Num(); BlockIndex++)
		{
			const TSharedRef< ILayoutBlock > Block = LineView.Blocks[ BlockIndex ];

			if ( ShowDebug )
			{
				BlockHue.R += 50.0f;

				FSlateDrawElement::MakeBox(
					OutDrawElements, 
					DebugLayer,
					FPaintGeometry( AllottedGeometry.AbsolutePosition + Block->GetLocationOffset(), Block->GetSize(), AllottedGeometry.Scale ),
					&DefaultTextStyle.HighlightShape,
					ClippingRect,
					DrawEffects,
					InWidgetStyle.GetColorAndOpacityTint() * BlockHue.HSVToLinearRGB()
					);
			}

			int32 BlockLayerId = TextLayer;
			const TSharedRef< ISlateRun > Run = StaticCastSharedRef< ISlateRun >( Block->GetRun() );

			int32 BlocksLayerId = MaxLayerId;
			const TSharedPtr< ISlateRunHighlighter > Highlighter = StaticCastSharedPtr< ISlateRunHighlighter >( Block->GetHighlighter() );
			if ( Highlighter.IsValid() )
			{
				BlocksLayerId = Highlighter->OnPaint( LineView, Run, Block, DefaultTextStyle, AllottedGeometry, ClippingRect, OutDrawElements, BlockLayerId, InWidgetStyle, bParentEnabled );
			}
			else
			{
				BlocksLayerId = Run->OnPaint( LineView, Block, DefaultTextStyle, AllottedGeometry, ClippingRect, OutDrawElements, BlockLayerId, InWidgetStyle, bParentEnabled );
			}

			MaxLayerId = FMath::Max( MaxLayerId, BlocksLayerId );
		}
	}

	return MaxLayerId;
}

void FSlateTextLayout::EndLayout()
{
	FTextLayout::EndLayout();
	AggregateChildren();
}

void FSlateTextLayout::AggregateChildren()
{
	Children.Empty();
	const TArray< FLineModel >& LineModels = GetLineModels();
	for (int LineModelIndex = 0; LineModelIndex < LineModels.Num(); LineModelIndex++)
	{
		const FLineModel& LineModel = LineModels[ LineModelIndex ];
		for (int RunIndex = 0; RunIndex < LineModel.Runs.Num(); RunIndex++)
		{
			const FRunModel& LineRun = LineModel.Runs[ RunIndex ];
			const TSharedRef< ISlateRun > SlateRun = StaticCastSharedRef< ISlateRun >( LineRun.GetRun() );

			FChildren* RunChildren = SlateRun->GetChildren();
			for (int ChildIndex = 0; ChildIndex < RunChildren->Num(); ChildIndex++)
			{
				const TSharedRef< SWidget >& Child = RunChildren->GetChildAt( ChildIndex );
				Children.Add( Child );
			}
		}
	}
}
