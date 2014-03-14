// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SRichTextBlock.h"
#include "TextLayoutEngine.h"
#include "SlateTextLayout.h"
#include "SlateSimpleRunHighlighter.h"
#include "IRichTextMarkupParser.h"
#include "Text/RichTextMarkupProcessing.h"

void SRichTextBlock::Construct( const FArguments& InArgs )
{
	WrapTextAt = InArgs._WrapTextAt;
	AutoWrapText = InArgs._AutoWrapText;
	Margin = InArgs._Margin;
	LineHeightPercentage = InArgs._LineHeightPercentage;
	Justification = InArgs._Justification;
	TagStyleSet = InArgs._DecoratorStyleSet;
	TextStyle = *InArgs._TextStyle;

	CachedAutoWrapTextWidth = 0.0f;

	Parser = InArgs._Parser;
	if ( !Parser.IsValid() )
	{
		Parser = FRichTextMarkupProcessing::Create();
	}

	TextHighlighter = FSlateSimpleRunHighlighter::Create();

	Decorators.Append( InArgs._Decorators );
	Decorators.Append( InArgs.InlineDecorators );

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
}

int32 SRichTextBlock::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	SCOPE_CYCLE_COUNTER( STAT_SlateOnPaint_SRichTextBlock );

	// Cache the last known size in case we need it for performing auto-wrap.
	CachedAutoWrapTextWidth = AllottedGeometry.Size.X;

	const ETextJustify::Type TextJustification = TextLayout->GetJustification();

	if ( TextJustification == ETextJustify::Right )
	{
		const FVector2D TextLayoutSize = TextLayout->GetSize();
		const FVector2D Offset( ( AllottedGeometry.Size.X - TextLayoutSize.X ), 0 );
		LayerId = TextLayout->OnPaint( TextStyle, AllottedGeometry.MakeChild( Offset, TextLayoutSize ), MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, ShouldBeEnabled( bParentEnabled ) );
	}
	else if ( TextJustification == ETextJustify::Center )
	{
		const FVector2D TextLayoutSize = TextLayout->GetSize();
		const FVector2D Offset( ( AllottedGeometry.Size.X - TextLayoutSize.X ) / 2, 0 );
		LayerId = TextLayout->OnPaint( TextStyle, AllottedGeometry.MakeChild( Offset, TextLayoutSize ), MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, ShouldBeEnabled( bParentEnabled ) );
	}
	else
	{
		LayerId = TextLayout->OnPaint( TextStyle, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, ShouldBeEnabled( bParentEnabled ) );
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
	const FMargin Margin = TextLayout->GetMargin();
	const FVector2D TextLayoutSize = TextLayout->GetSize();

	//If a wrapping width has been provided that should be reported as the desired width.
	const float Width = TextLayout->GetWrappingWidth() > 0 ? TextLayout->GetWrappingWidth() : TextLayoutSize.X;

	return FVector2D( FMath::Max( Margin.GetTotalSpaceAlong<Orient_Horizontal>(), Width ), FMath::Max( Margin.GetTotalSpaceAlong<Orient_Vertical>(), TextLayoutSize.Y ) );
}

FChildren* SRichTextBlock::GetChildren()
{
	return TextLayout->GetChildren();
}

void SRichTextBlock::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	TextLayout->ArrangeChildren( AllottedGeometry, ArrangedChildren );
}

TSharedPtr< ITextDecorator > SRichTextBlock::TryGetDecorator( const TArray< TSharedRef< ITextDecorator > >& Decorators, const FString& Text, const FTextRunParseResults& TextRun ) const
{
	if ( Decorators.Num() > 0 )
	{
		for( auto DecoratorIter = Decorators.CreateConstIterator(); DecoratorIter; ++DecoratorIter )
		{
			if ( (*DecoratorIter)->Supports( TextRun, Text ) )
			{
				return *DecoratorIter;
			}
		}
	}

	return NULL;
}

void SRichTextBlock::SetText( const FText& InText )
{
	Text = InText;

	const FString& InString = InText.ToString();
	TArray<FTextLineParseResults> LineParseResultsArray;
	FString ProcessedString;
	Parser->Process(LineParseResultsArray, InString, ProcessedString);

	TextLayout->ClearLines();

	// Iterate through parsed line results and create processed lines with runs.
	for (int LineIndex = 0; LineIndex < LineParseResultsArray.Num(); LineIndex++)
	{
		const FTextLineParseResults& LineParseResults = LineParseResultsArray[ LineIndex ];

		TSharedRef<FString> ModelString = MakeShareable(new FString());
		TArray< TSharedRef< IRun > > Runs;

		for (int RunIndex = 0; RunIndex < LineParseResults.Runs.Num(); RunIndex++)
		{
			const FTextRunParseResults& RunParseResult = LineParseResults.Runs[ RunIndex ];
			TSharedPtr< ISlateRun > Run;

			TSharedPtr< ITextDecorator > Decorator = TryGetDecorator( Decorators, ProcessedString, RunParseResult );

			if ( Decorator.IsValid() )
			{
				// Create run and update model string.
				Run = Decorator->Create( RunParseResult, ProcessedString, ModelString, TagStyleSet );
			}
			else
			{
				const FTextBlockStyle* TextBlockStyle;
				FTextRange ModelRange;
				ModelRange.BeginIndex = ModelString->Len();
				if(!(RunParseResult.Name.IsEmpty()) && TagStyleSet->HasWidgetStyle< FTextBlockStyle >( FName(*RunParseResult.Name) ))
				{
					*ModelString += ProcessedString.Mid(RunParseResult.ContentRange.BeginIndex, RunParseResult.ContentRange.EndIndex - RunParseResult.ContentRange.BeginIndex);
					TextBlockStyle = &(TagStyleSet->GetWidgetStyle< FTextBlockStyle >( FName(*RunParseResult.Name) ));
				}
				else
				{
					*ModelString += ProcessedString.Mid(RunParseResult.OriginalRange.BeginIndex, RunParseResult.OriginalRange.EndIndex - RunParseResult.OriginalRange.BeginIndex);
					TextBlockStyle = &(TextStyle);
				}
				ModelRange.EndIndex = ModelString->Len();

				// Create run.
				Run = FSlateTextRun::Create( ModelString, *TextBlockStyle, ModelRange );
			}

			Runs.Add( Run.ToSharedRef() );
		}

		TextLayout->AddLine( ModelString, Runs );
	}
}

void SRichTextBlock::SetHighlightText( const FText& InHighlightText )
{
	if ( !TextHighlighter.IsValid() )
	{
		return;
	}

	Highlights.Empty();
	HighlightText = InHighlightText;

	HighlightText = InHighlightText;
	const FString HighlightTextString = HighlightText.ToString();
	const int32 HighlightTextLength = HighlightTextString.Len();

	const TArray< FTextLayout::FLineModel >& LineModels = TextLayout->GetLineModels();

	for (int LineIndex = 0; LineIndex < LineModels.Num(); LineIndex++)
	{
		const FTextLayout::FLineModel& LineModel = LineModels[ LineIndex ];

		int32 FindBegin = 0;
		int32 CurrentHighlightBegin;
		const int32 TextLength = LineModel.Text->Len();
		while( FindBegin < TextLength && (CurrentHighlightBegin = LineModel.Text->Find(HighlightTextString, ESearchCase::IgnoreCase, ESearchDir::FromStart, FindBegin)) != INDEX_NONE )
		{
			FindBegin = CurrentHighlightBegin + HighlightTextLength;

			if ( Highlights.Num() > 0 && Highlights.Last().LineIndex == LineIndex && Highlights.Last().Range.EndIndex == CurrentHighlightBegin )
			{
				Highlights[ Highlights.Num() - 1 ] = FTextHighlight( LineIndex, FTextRange( Highlights.Last().Range.BeginIndex, FindBegin ), TextHighlighter.ToSharedRef() );
			}
			else
			{
				Highlights.Add( FTextHighlight( LineIndex, FTextRange( CurrentHighlightBegin, FindBegin ), TextHighlighter.ToSharedRef() ) );
			}
		}
	}

	TextLayout->SetHighlights( Highlights );
}
