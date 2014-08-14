// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"

#if WITH_FANCY_TEXT

#include "TextLayoutEngine.h"
#include "SlateTextLayout.h"
#include "SlateTextHighlightRunRenderer.h"
#include "IRichTextMarkupParser.h"
#include "RichTextMarkupProcessing.h"


void SRichTextBlock::Construct( const FArguments& InArgs )
{
	TextStyle = *InArgs._TextStyle;
	WrapTextAt = InArgs._WrapTextAt;
	AutoWrapText = InArgs._AutoWrapText;
	Margin = InArgs._Margin;
	LineHeightPercentage = InArgs._LineHeightPercentage;
	Justification = InArgs._Justification;

	CachedAutoWrapTextWidth = 0.0f;

	{
		TSharedPtr<IRichTextMarkupParser> Parser = InArgs._Parser;
		if ( !Parser.IsValid() )
		{
			Parser = FDefaultRichTextMarkupParser::Create();
		}

		Marshaller = FRichTextLayoutMarshaller::Create(Parser, nullptr, InArgs._Decorators, InArgs._DecoratorStyleSet, TextStyle);
		for ( const TSharedRef< ITextDecorator >& Decorator : InArgs.InlineDecorators )
		{
			Marshaller->AppendInlineDecorator( Decorator );
		}
	}

	TextHighlighter = FSlateTextHighlightRunRenderer::Create();

	TextLayout = FSlateTextLayout::Create();

	SetText( InArgs._Text );

	TextLayout->SetWrappingWidth( WrapTextAt.Get() );
	TextLayout->SetMargin( Margin.Get() );
	TextLayout->SetLineHeightPercentage( LineHeightPercentage.Get() );
	TextLayout->SetJustification( Justification.Get() );
	TextLayout->UpdateIfNeeded();
}

void SRichTextBlock::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	TextLayout->SetScale( AllottedGeometry.Scale );

	bool bRequiresTextUpdate = false;
	const FText& TextToSet = GetText();
	if (BoundText.IsBound() && !BoundTextLastTick.IdenticalTo(TextToSet))
	{
		// The pointer used by the bound text has changed, however the text may still be the same - check that now
		if (!BoundTextLastTick.ToString().Equals(TextToSet.ToString(), ESearchCase::CaseSensitive))
		{
			// The source text has changed, so update the internal editable text
			bRequiresTextUpdate = true;
		}

		// Update this even if the text is lexically identical, as it will update the pointer compared by IdenticalTo for the next Tick
		BoundTextLastTick = TextToSet;
	}

	if (bRequiresTextUpdate || Marshaller->IsDirty())
	{
		SetText(TextToSet);
	}
}

int32 SRichTextBlock::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	SCOPE_CYCLE_COUNTER( STAT_SlateOnPaint_SRichTextBlock );

	// Cache the last known size in case we need it for performing auto-wrap.
	CachedAutoWrapTextWidth = AllottedGeometry.Size.X;

	const ETextJustify::Type TextJustification = TextLayout->GetJustification();

	if ( TextJustification == ETextJustify::Right )
	{
		const FVector2D TextLayoutSize = TextLayout->GetSize();
		const FVector2D Offset( ( AllottedGeometry.Size.X - TextLayoutSize.X ), 0 );
		LayerId = TextLayout->OnPaint( Args.WithNewParent(this), TextStyle, AllottedGeometry.MakeChild( Offset, TextLayoutSize ), MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, ShouldBeEnabled( bParentEnabled ) );
	}
	else if ( TextJustification == ETextJustify::Center )
	{
		const FVector2D TextLayoutSize = TextLayout->GetSize();
		const FVector2D Offset( ( AllottedGeometry.Size.X - TextLayoutSize.X ) / 2, 0 );
		LayerId = TextLayout->OnPaint( Args.WithNewParent(this), TextStyle, AllottedGeometry.MakeChild( Offset, TextLayoutSize ), MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, ShouldBeEnabled( bParentEnabled ) );
	}
	else
	{
		LayerId = TextLayout->OnPaint( Args.WithNewParent(this), TextStyle, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, ShouldBeEnabled( bParentEnabled ) );
	}

	return LayerId;
}

void SRichTextBlock::CacheDesiredSize()
{
	float WrappingWidth = WrapTextAt.Get();

	// Text wrapping can either be used defined (WrapTextAt), automatic (AutoWrapText and CachedAutoWrapTextWidth), 
	// or a mixture of both.  Take whichever has the smallest value (>1)
	if ( AutoWrapText.Get() && CachedAutoWrapTextWidth >= 1.0f )
	{
		WrappingWidth = (WrappingWidth >= 1.0f) ? FMath::Min(WrappingWidth, CachedAutoWrapTextWidth) : CachedAutoWrapTextWidth;
	}

	TextLayout->SetWrappingWidth( WrappingWidth );
	TextLayout->SetMargin( Margin.Get() );
	TextLayout->SetLineHeightPercentage( LineHeightPercentage.Get() );
	TextLayout->SetJustification( Justification.Get() );
	TextLayout->UpdateIfNeeded();

	SWidget::CacheDesiredSize();
}

FVector2D SRichTextBlock::ComputeDesiredSize() const
{
	//The layouts current margin size. We should not report a size smaller then the margins.
	const FMargin LayoutMargin = TextLayout->GetMargin();
	const FVector2D TextLayoutSize = TextLayout->GetSize();

	//If a wrapping width has been provided that should be reported as the desired width.
	const float Width = TextLayout->GetWrappingWidth() > 0 ? TextLayout->GetWrappingWidth() : TextLayoutSize.X;

	return FVector2D(FMath::Max(LayoutMargin.GetTotalSpaceAlong<Orient_Horizontal>(), Width), FMath::Max(LayoutMargin.GetTotalSpaceAlong<Orient_Vertical>(), TextLayoutSize.Y));
}

FChildren* SRichTextBlock::GetChildren()
{
	return TextLayout->GetChildren();
}

void SRichTextBlock::OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	const ETextJustify::Type TextJustification = TextLayout->GetJustification();

	if ( TextJustification == ETextJustify::Right )
	{
		const FVector2D TextLayoutSize = TextLayout->GetSize();
		const FVector2D Offset( ( AllottedGeometry.Size.X - TextLayoutSize.X ), 0 );
		TextLayout->ArrangeChildren( AllottedGeometry.MakeChild( Offset, TextLayoutSize ), ArrangedChildren );
	}
	else if ( TextJustification == ETextJustify::Center )
	{
		const FVector2D TextLayoutSize = TextLayout->GetSize();
		const FVector2D Offset( ( AllottedGeometry.Size.X - TextLayoutSize.X ) / 2, 0 );
		TextLayout->ArrangeChildren( AllottedGeometry.MakeChild( Offset, TextLayoutSize ), ArrangedChildren );
	}
	else
	{
		TextLayout->ArrangeChildren( AllottedGeometry, ArrangedChildren );
	}
}

void SRichTextBlock::SetText( const TAttribute<FText>& InTextAttr )
{
	BoundText = InTextAttr;

	const FText& InText = GetText();

	// Update the cached BoundText value to prevent it triggering another SetText update again next Tick
	if (BoundText.IsBound())
	{
		BoundTextLastTick = InText;
	}

	TextLayout->ClearLines();

	Marshaller->SetText(InText.ToString(), *TextLayout);
}

void SRichTextBlock::SetHighlightText( const FText& InHighlightText )
{
	if ( !TextHighlighter.IsValid() )
	{
		return;
	}

	HighlightText = InHighlightText;

	const FString& HighlightTextString = HighlightText.ToString();
	const int32 HighlightTextLength = HighlightTextString.Len();

	const TArray< FTextLayout::FLineModel >& LineModels = TextLayout->GetLineModels();

	TArray< FTextRunRenderer > TextHighlights;
	for (int32 LineIndex = 0; LineIndex < LineModels.Num(); LineIndex++)
	{
		const FTextLayout::FLineModel& LineModel = LineModels[ LineIndex ];

		int32 FindBegin = 0;
		int32 CurrentHighlightBegin;
		const int32 TextLength = LineModel.Text->Len();
		while( FindBegin < TextLength && (CurrentHighlightBegin = LineModel.Text->Find(HighlightTextString, ESearchCase::IgnoreCase, ESearchDir::FromStart, FindBegin)) != INDEX_NONE )
		{
			FindBegin = CurrentHighlightBegin + HighlightTextLength;

			if ( TextHighlights.Num() > 0 && TextHighlights.Last().LineIndex == LineIndex && TextHighlights.Last().Range.EndIndex == CurrentHighlightBegin )
			{
				TextHighlights[ TextHighlights.Num() - 1 ] = FTextRunRenderer( LineIndex, FTextRange( TextHighlights.Last().Range.BeginIndex, FindBegin ), TextHighlighter.ToSharedRef() );
			}
			else
			{
				TextHighlights.Add( FTextRunRenderer( LineIndex, FTextRange( CurrentHighlightBegin, FindBegin ), TextHighlighter.ToSharedRef() ) );
			}
		}
	}

	TextLayout->SetRunRenderers( TextHighlights );
}

void SRichTextBlock::Refresh()
{
	TextLayout->DirtyLayout();
}

#endif //WITH_FANCY_TEXT