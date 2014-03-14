// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"

FTextLayout::FBreakCandidate FTextLayout::CreateBreakCandidate( int32& OutRunIndex, FLineModel& Line, int32 PreviousBreak, int32 CurrentBreak )
{
	bool SuccessfullyMeasuredSlice = false;
	int16 MaxAboveBaseline = 0;
	int16 MaxBelowBaseline = 0;
	FVector2D BreakSize( ForceInitToZero );
	FVector2D BreakSizeWithoutTrailingWhitespace( ForceInitToZero );
	int32 WhitespaceStopIndex = CurrentBreak;
	uint8 Kerning = 0;

	if ( Line.Runs.IsValidIndex( OutRunIndex ) )
	{
		FRunModel& Run = Line.Runs[ OutRunIndex ];
		const FTextRange Range = Run.GetTextRange();
		int32 BeginIndex = FMath::Max( PreviousBreak, Range.BeginIndex );

		if ( BeginIndex > 0 )
		{
			Kerning = Run.GetKerning( BeginIndex, Scale );
		}
	}

	// We need to consider the Runs when detecting and measuring the text lengths of Lines because
	// the font style used makes a difference.
	for (; OutRunIndex < Line.Runs.Num(); OutRunIndex++)
	{
		FRunModel& Run = Line.Runs[ OutRunIndex ];
		const FTextRange Range = Run.GetTextRange();

		FVector2D SliceSize;
		FVector2D SliceSizeWithoutTrailingWhitespace;
		int32 StopIndex = PreviousBreak;

		WhitespaceStopIndex = StopIndex = FMath::Min( Range.EndIndex, CurrentBreak );
		int32 BeginIndex = FMath::Max( PreviousBreak, Range.BeginIndex );

		//@todo Use ICU's whitespace functionality [12/5/2013 justin.sargent]
		while( WhitespaceStopIndex > BeginIndex && TChar<TCHAR>::IsWhitespace( (*Line.Text)[ WhitespaceStopIndex - 1 ] ) )
		{
			--WhitespaceStopIndex;
		}

		if ( WhitespaceStopIndex != StopIndex && BeginIndex != WhitespaceStopIndex )
		{
			SliceSize = SliceSizeWithoutTrailingWhitespace = Run.Measure( BeginIndex, WhitespaceStopIndex, Scale );
			SliceSize.X += Run.Measure( WhitespaceStopIndex, StopIndex, Scale ).X;
		}
		else
		{
			SliceSize = Run.Measure( BeginIndex, StopIndex, Scale );
			SliceSizeWithoutTrailingWhitespace = SliceSize;
		}

		BreakSize.X += SliceSize.X; // We accumulate the slice widths
		BreakSizeWithoutTrailingWhitespace.X += SliceSizeWithoutTrailingWhitespace.X; // We accumulate the slice widths

		// Get the baseline and flip it's sign; Baselines are generally negative
		const int16 Baseline = -(Run.GetBaseLine( Scale ));

		// For the height of the slice we need to take into account the largest value below and above the baseline and add those together
		MaxAboveBaseline = FMath::Max( MaxAboveBaseline, (int16)( Run.GetMaxHeight( Scale ) - Baseline ) );
		MaxBelowBaseline = FMath::Max( MaxBelowBaseline, Baseline );

		if ( StopIndex == CurrentBreak )
		{
			SuccessfullyMeasuredSlice = true;

			if ( OutRunIndex < Line.Runs.Num() && StopIndex == Line.Runs[ OutRunIndex ].GetTextRange().EndIndex )
			{
				++OutRunIndex;
			}
			break;
		}
	}
	check( SuccessfullyMeasuredSlice == true );

	BreakSize.Y = BreakSizeWithoutTrailingWhitespace.Y = MaxAboveBaseline + MaxBelowBaseline;

	FBreakCandidate BreakCandidate;
	BreakCandidate.Size = BreakSize;
	BreakCandidate.SizeWithoutTrailingWhitespace = BreakSizeWithoutTrailingWhitespace;
	BreakCandidate.Range = FTextRange( PreviousBreak, CurrentBreak );
	BreakCandidate.RangeWithoutTrailingWhitespace = FTextRange( PreviousBreak, WhitespaceStopIndex );
	BreakCandidate.MaxAboveBaseline = MaxAboveBaseline;
	BreakCandidate.MaxBelowBaseline = MaxBelowBaseline;
	BreakCandidate.Kerning = Kerning;

#if TEXT_LAYOUT_DEBUG
	BreakCandidate.DebugSlice = FString( BreakCandidate.Range.EndIndex - BreakCandidate.Range.BeginIndex, (**Line.Text) + BreakCandidate.Range.BeginIndex );
#endif

	return BreakCandidate;
}

void FTextLayout::CreateLineViewBlocks( const FLineModel& Line, const int32 StopIndex, const int32 VisualStopIndex, int32& OutRunIndex, int32& OutHighlightIndex, int32& OutPreviousBlockEnd, TArray< TSharedRef< ILayoutBlock > >& OutSoftLine )
{
	const bool VisualStopIndexIsValid = VisualStopIndex != INDEX_NONE;
	int16 MaxAboveBaseline = 0;
	int16 MaxBelowBaseline = 0;

	for (; OutRunIndex < Line.Runs.Num(); )
	{
		const FRunModel& Run = Line.Runs[ OutRunIndex ];
		const FTextRange RunRange = Run.GetTextRange();

		TSharedPtr< IRunHighlighter > BlockHighlighter = NULL;

		int32 BlockStopIndex = RunRange.EndIndex;
		if ( OutHighlightIndex != INDEX_NONE )
		{
			const FTextHighlight& Highlight = Line.Highlights[ OutHighlightIndex ];
			if ( OutPreviousBlockEnd >= Highlight.Range.BeginIndex )
			{
				if ( Highlight.Range.EndIndex <= RunRange.EndIndex )
				{
					BlockStopIndex = Highlight.Range.EndIndex;
					BlockHighlighter = Highlight.Highlighter;
				}
			}
			else
			{
				if ( Highlight.Range.BeginIndex <= RunRange.EndIndex )
				{
					BlockStopIndex = Highlight.Range.BeginIndex;
					BlockHighlighter = NULL;
				}
			}
		}

		if ( VisualStopIndex != INDEX_NONE )
		{
			BlockStopIndex = FMath::Min( VisualStopIndex, BlockStopIndex );
		}

		int32 BlockBeginIndex = FMath::Max( OutPreviousBlockEnd, RunRange.BeginIndex );
		const bool IsLastBlock = BlockStopIndex == VisualStopIndex;

		if ( RunRange.BeginIndex < BlockStopIndex && RunRange.EndIndex > BlockBeginIndex )
		{
			check( BlockBeginIndex <= BlockStopIndex );

			FBlockDefinition BlockDefine;
			BlockDefine.Range = BlockDefine.VisualRange = FTextRange( BlockBeginIndex, BlockStopIndex );
			BlockDefine.Highlighter = BlockHighlighter;

			OutSoftLine.Add( Run.CreateBlock( BlockDefine, Scale ) );
			OutPreviousBlockEnd = BlockStopIndex;
		}
		else
		{
			FBlockDefinition BlockDefine;
			BlockDefine.Range = RunRange;
			BlockDefine.VisualRange = FTextRange( RunRange.BeginIndex, ( IsLastBlock && VisualStopIndexIsValid ) ? VisualStopIndex : RunRange.EndIndex );
			BlockDefine.Highlighter = BlockHighlighter;

			OutSoftLine.Add( Run.CreateBlock( BlockDefine, Scale ) );
			OutPreviousBlockEnd = RunRange.EndIndex;
		}

		// Get the baseline and flip it's sign; Baselines are generally negative
		const int16 Baseline = -(Run.GetBaseLine( Scale ));

		// For the height of the slice we need to take into account the largest value below and above the baseline and add those together
		MaxAboveBaseline = FMath::Max( MaxAboveBaseline, (int16)( Run.GetMaxHeight( Scale ) - Baseline ) );
		MaxBelowBaseline = FMath::Max( MaxBelowBaseline, Baseline );

		if ( BlockStopIndex == RunRange.EndIndex )
		{
			++OutRunIndex;
		}

		if ( OutHighlightIndex != INDEX_NONE && BlockStopIndex == Line.Highlights[ OutHighlightIndex ].Range.EndIndex )
		{
			++OutHighlightIndex;

			if ( OutHighlightIndex >= Line.Highlights.Num() )
			{
				OutHighlightIndex = INDEX_NONE;
			}
		}

		if ( IsLastBlock )
		{
			break;
		}
	}

	//Skip any highlighted ranges which we have skipped due to ignoring trailing whitespace
	while ( OutHighlightIndex != INDEX_NONE && Line.Highlights[ OutHighlightIndex ].Range.EndIndex <= StopIndex )
	{
		OutHighlightIndex++;

		if ( OutHighlightIndex >= Line.Highlights.Num() )
		{
			OutHighlightIndex = INDEX_NONE;
		}
	}

	FVector2D LineSize( ForceInitToZero );
	FVector2D CurrentOffset( Margin.Left, Margin.Top + DrawSize.Y );

	if ( OutSoftLine.Num() > 0 )
	{
		float CurrentHorizontalPos = 0.0f;
		for (int Index = 0; Index < OutSoftLine.Num(); Index++)
		{
			const TSharedRef< ILayoutBlock > Block = OutSoftLine[ Index ];
			const int16 BlockBaseline = Block->GetRun()->GetBaseLine( Scale );
			const int16 VerticalOffset = MaxAboveBaseline - Block->GetSize().Y - BlockBaseline;

			Block->SetLocationOffset( FVector2D( CurrentOffset.X + CurrentHorizontalPos, CurrentOffset.Y + VerticalOffset ) );

			CurrentHorizontalPos += Block->GetSize().X;
		}

		LineSize.X = CurrentHorizontalPos;
		LineSize.Y = MaxAboveBaseline + MaxBelowBaseline;
	}

	FTextLayout::FLineView LineView;
	LineView.Size = LineSize;
	LineView.Offset = CurrentOffset;
	LineView.Blocks.Append( OutSoftLine );

	LineViews.Add( LineView );

	DrawSize.X = FMath::Max( DrawSize.X, LineSize.X ); //DrawSize.X is the size of the longest line + the Margin
	const float LineHeight = LineSize.Y * LineHeightPercentage; 
	DrawSize.Y += LineHeight; //DrawSize.Y is the total height of all lines
}

void FTextLayout::JustifyLayout()
{
	if ( Justification == ETextJustify::Left )
	{
		return;
	}

	const float LayoutWidthNoMargin = DrawSize.X - Margin.GetTotalSpaceAlong<Orient_Horizontal>() * Scale;

	for (int LineViewIndex = 0; LineViewIndex < LineViews.Num(); LineViewIndex++)
	{
		FLineView& LineView = LineViews[ LineViewIndex ];
		const float ExtraSpace = LayoutWidthNoMargin - LineView.Size.X;

		FVector2D BlockOffset( ForceInitToZero );
		if ( Justification == ETextJustify::Center )
		{
			BlockOffset = FVector2D( ExtraSpace / 2, 0 );
		}
		else if ( Justification == ETextJustify::Right )
		{
			BlockOffset = FVector2D( ExtraSpace, 0 );
		}

		for (int BlockIndex = 0; BlockIndex < LineView.Blocks.Num(); BlockIndex++)
		{
			const TSharedRef< ILayoutBlock >& Block = LineView.Blocks[ BlockIndex ];
			Block->SetLocationOffset( Block->GetLocationOffset() + BlockOffset );
		}
	}
}

void FTextLayout::FlowLayout()
{
	check( WrappingWidth >= 0 );
	const bool IsWrapping = WrappingWidth > 0;
	const float WrappingDrawWidth = FMath::Max( 0.01f, ( WrappingWidth - ( Margin.Left + Margin.Right ) ) * Scale );

	if ( IsWrapping )
	{
		ReadyForWrapping();
	}

	TArray< TSharedRef< ILayoutBlock > > SoftLine;

	for (int LineModelIndex = 0; LineModelIndex < LineModels.Num(); LineModelIndex++)
	{
		const FLineModel& LineModel = LineModels[ LineModelIndex ];

		float CurrentWidth = 0.0f;
		int32 CurrentRunIndex = 0;
		int32 PreviousBlockEnd = 0;

		int32 CurrentHighlightIndex = 0;
		if ( CurrentHighlightIndex >= LineModel.Highlights.Num() )
		{
			CurrentHighlightIndex = INDEX_NONE;
		}

		// if the Line doesn't have any BreakCandidates
		if ( LineModel.BreakCandidates.Num() == 0 )
		{
			//Then iterate over all of it's runs
			CreateLineViewBlocks( LineModel, INDEX_NONE, INDEX_NONE, /*OUT*/CurrentRunIndex, /*OUT*/CurrentHighlightIndex, /*OUT*/PreviousBlockEnd, SoftLine );
			check( CurrentRunIndex == LineModel.Runs.Num() );
			CurrentWidth = 0;
			SoftLine.Empty();
		}
		else
		{
			for (int BreakIndex = 0; BreakIndex < LineModel.BreakCandidates.Num(); BreakIndex++)
			{
				const FBreakCandidate& Break = LineModel.BreakCandidates[ BreakIndex ];

				const bool IsLastBreak = BreakIndex + 1 == LineModel.BreakCandidates.Num();
				const bool IsFirstBreakOnSoftLine = CurrentWidth == 0;
				const uint8 Kerning = ( IsFirstBreakOnSoftLine ) ? Break.Kerning : 0;
				const bool BreakDoesFit = !IsWrapping || CurrentWidth + Break.Size.X + Kerning <= WrappingDrawWidth;

				if ( !BreakDoesFit || IsLastBreak )
				{
					const bool BreakWithoutTrailingWhitespaceDoesFit = !IsWrapping || CurrentWidth + Break.SizeWithoutTrailingWhitespace.X + Kerning <= WrappingDrawWidth;
					const bool IsFirstBreak = BreakIndex == 0;

					const FBreakCandidate& FinalBreakOnSoftLine = ( !IsFirstBreak && !IsFirstBreakOnSoftLine && !BreakWithoutTrailingWhitespaceDoesFit ) ? LineModel.BreakCandidates[ --BreakIndex ] : Break;

					CreateLineViewBlocks( LineModel, FinalBreakOnSoftLine.Range.EndIndex, FinalBreakOnSoftLine.RangeWithoutTrailingWhitespace.EndIndex, /*OUT*/CurrentRunIndex, /*OUT*/CurrentHighlightIndex, /*OUT*/PreviousBlockEnd, SoftLine );

					if ( CurrentRunIndex < LineModel.Runs.Num() && FinalBreakOnSoftLine.Range.EndIndex == LineModel.Runs[ CurrentRunIndex ].GetTextRange().EndIndex )
					{
						++CurrentRunIndex;
					}

					PreviousBlockEnd = FinalBreakOnSoftLine.Range.EndIndex;

					CurrentWidth = 0;
					SoftLine.Empty();
				} 
				else
				{
					CurrentWidth += Break.Size.X;
				}
			}
		}
	}

	//Add on the margins to the DrawSize
	DrawSize.X += Margin.GetTotalSpaceAlong<Orient_Horizontal>() * Scale;
	DrawSize.Y += Margin.GetTotalSpaceAlong<Orient_Vertical>() * Scale;
}

void FTextLayout::CreateView()
{
	ClearView();
	BeginLayout();

	FlowLayout();
	JustifyLayout();

	EndLayout();
	HasChanged = false;
}

void FTextLayout::EndLayout()
{
	for (int LineModelIndex = 0; LineModelIndex < LineModels.Num(); LineModelIndex++)
	{
		FLineModel& LineModel = LineModels[ LineModelIndex ];
		for (int RunIndex = 0; RunIndex < LineModel.Runs.Num(); RunIndex++)
		{
			LineModel.Runs[ RunIndex ].EndLayout();
		}
	}
}

void FTextLayout::BeginLayout()
{
	for (int LineModelIndex = 0; LineModelIndex < LineModels.Num(); LineModelIndex++)
	{
		FLineModel& LineModel = LineModels[ LineModelIndex ];
		for (int RunIndex = 0; RunIndex < LineModel.Runs.Num(); RunIndex++)
		{
			LineModel.Runs[ RunIndex ].BeginLayout();
		}
	}
}

void FTextLayout::ClearView()
{
	DrawSize = FVector2D::ZeroVector;
	LineViews.Empty();
}

void FTextLayout::ReadyForWrapping()
{
	if ( HasWrapInformation )
	{
		return;
	}

	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	for (int LineModelIndex = 0; LineModelIndex < LineModels.Num(); LineModelIndex++)
	{
		FLineModel& LineModel = LineModels[ LineModelIndex ];

		for (int RunIndex = 0; RunIndex < LineModel.Runs.Num(); RunIndex++)
		{
			LineModel.Runs[ RunIndex ].ClearCache();
		}

		FLineBreakIterator LineBreakIterator( **LineModel.Text );

		int32 PreviousBreak = 0;
		int32 CurrentBreak = 0;
		int32 CurrentRunIndex = 0;

		while( ( CurrentBreak = LineBreakIterator.MoveToNext() ) != INDEX_NONE )
		{
			LineModel.BreakCandidates.Add( CreateBreakCandidate(/*OUT*/CurrentRunIndex, LineModel, PreviousBreak, CurrentBreak) );
			PreviousBreak = CurrentBreak;
		}
	}

	HasWrapInformation = true;
}

FTextLayout::FTextLayout() : LineModels()
	, LineViews()
	, HasWrapInformation( false )
	, HasChanged( false )
	, Scale( 1.0f )
	, WrappingWidth( 0 )
	, Margin()
	, LineHeightPercentage( 1.0f )
	, DrawSize( ForceInitToZero )
{

}

void FTextLayout::UpdateIfNeeded()
{
	if ( HasChanged )
	{
		// if something has changed then create a new View
		CreateView();
	}
}

void FTextLayout::SetHighlights( const TArray< FTextHighlight >& Highlights )
{
	ClearHighlights();

	for (int Index = 0; Index < Highlights.Num(); Index++)
	{
		AddHighlight( Highlights[ Index ] );
	}
}

void FTextLayout::AddHighlight( const FTextHighlight& Highlight )
{
	checkf( LineModels.IsValidIndex( Highlight.LineIndex ), TEXT("Highlights must be for a valid Line Index") );

	//Highlights needs to be in order and not overlap
	bool InsertSuccessful = false;
	FLineModel& LineModel = LineModels[ Highlight.LineIndex ];
	for (int Index = 0; Index < LineModel.Highlights.Num(); Index++)
	{
		if ( LineModel.Highlights[ Index ].Range.BeginIndex > Highlight.Range.BeginIndex )
		{
			checkf( Index == 0 || LineModel.Highlights[ Index - 1 ].Range.EndIndex <= Highlight.Range.BeginIndex, TEXT("Highlights cannot overlap") );
			LineModel.Highlights.Insert( Highlight, Index - 1 );
			InsertSuccessful = true;
		}
		else if ( LineModel.Highlights[ Index ].Range.EndIndex > Highlight.Range.EndIndex )
		{
			checkf( LineModel.Highlights[ Index ].Range.BeginIndex >= Highlight.Range.EndIndex, TEXT("Highlights cannot overlap") );
			LineModel.Highlights.Insert( Highlight, Index - 1 );
			InsertSuccessful = true;
		}
	}

	if ( !InsertSuccessful )
	{
		LineModel.Highlights.Add( Highlight );
	}

	HasChanged = true;
}

void FTextLayout::ClearHighlights()
{
	for (int32 Index = 0; Index < LineModels.Num(); Index++)
	{
		LineModels[ Index ].Highlights.Empty();
	}

	HasChanged = true;
}

void FTextLayout::AddLine( const TSharedRef< FString >& Text, const TArray< TSharedRef< IRun > >& Runs )
{
	FLineModel LineModel( Text );

	for (int Index = 0; Index < Runs.Num(); Index++)
	{
		LineModel.Runs.Add( FRunModel( Runs[ Index ] ) );
	}

	LineModels.Add( LineModel );
	HasChanged = true;
}

void FTextLayout::ClearLines()
{
	LineModels.Empty();
	HasChanged = true;
}

void FTextLayout::SetMargin( const FMargin& InMargin )
{
	if ( Margin == InMargin )
	{
		return;
	}

	const FMargin PreviousMargin = Margin;
	Margin = InMargin;

	if ( WrappingWidth > 0 )
	{
		// Since we are wrapping our text we'll need to rebuild the view as the actual wrapping width includes the margin.
		HasChanged = true;
	}
	else
	{
		FVector2D OffsetAdjustment( Margin.Left - PreviousMargin.Left, Margin.Top - PreviousMargin.Top );
		OffsetAdjustment *= Scale;
		for (int LineViewIndex = 0; LineViewIndex < LineViews.Num(); LineViewIndex++)
		{
			FLineView& LineView = LineViews[ LineViewIndex ];

			for (int BlockIndex = 0; BlockIndex < LineView.Blocks.Num(); BlockIndex++)
			{
				const TSharedRef< ILayoutBlock >& Block = LineView.Blocks[ BlockIndex ];
				Block->SetLocationOffset( Block->GetLocationOffset() + OffsetAdjustment );
			}
		}

		DrawSize.X += ( Margin.GetTotalSpaceAlong<Orient_Horizontal>() - PreviousMargin.GetTotalSpaceAlong<Orient_Horizontal>() ) * Scale;
		DrawSize.Y += ( Margin.GetTotalSpaceAlong<Orient_Vertical>() - PreviousMargin.GetTotalSpaceAlong<Orient_Vertical>() ) * Scale;
	}
}

FMargin FTextLayout::GetMargin() const
{
	return Margin;
}

void FTextLayout::SetScale( float Value )
{
	if ( Scale == Value )
	{
		return;
	}

	Scale = Value;
	HasChanged = true;

	if ( HasWrapInformation )
	{
		for (int LineModelIndex = 0; LineModelIndex < LineModels.Num(); LineModelIndex++)
		{
			FLineModel& LineModel = LineModels[ LineModelIndex ];
			int32 CurrentRunIndex = 0;
			int32 PreviousBreak = 0;
			TArray< FBreakCandidate > UpdatedBreakCandidates;

			for (int BreakIndex = 0; BreakIndex < LineModel.BreakCandidates.Num(); BreakIndex++)
			{
				const FBreakCandidate& Break = LineModel.BreakCandidates[ BreakIndex ];
				UpdatedBreakCandidates.Add( CreateBreakCandidate( /*OUT*/CurrentRunIndex, LineModel, PreviousBreak, Break.Range.EndIndex ) );
				PreviousBreak = Break.Range.EndIndex;
			}

			check( LineModel.BreakCandidates.Num() == UpdatedBreakCandidates.Num() );
			LineModel.BreakCandidates = UpdatedBreakCandidates;

			for (int RunIndex = 0; RunIndex < LineModel.Runs.Num(); RunIndex++)
			{
				FRunModel& Run = LineModel.Runs[ RunIndex ];
				Run.ClearCache();
			}
		}
	}
}

float FTextLayout::GetScale() const
{
	return Scale;
}

void FTextLayout::SetJustification( ETextJustify::Type Value )
{
	if ( Justification == Value )
	{
		return;
	}

	Justification = Value;
	HasChanged = true;
}

ETextJustify::Type FTextLayout::GetJustification() const
{
	return Justification;
}

void FTextLayout::SetLineHeightPercentage( float Value )
{
	if ( LineHeightPercentage != Value )
	{
		LineHeightPercentage = Value; 
		HasChanged = true;
	}
}

float FTextLayout::GetLineHeightPercentage() const
{
	return LineHeightPercentage;
}

void FTextLayout::SetWrappingWidth( float Value )
{
	if ( WrappingWidth != Value )
	{
		WrappingWidth = Value; 
		HasChanged = true;
	}
}

float FTextLayout::GetWrappingWidth() const
{
	return WrappingWidth;
}

FVector2D FTextLayout::GetDrawSize() const
{
	return DrawSize;
}

FVector2D FTextLayout::GetSize() const
{
	return DrawSize * ( 1 / Scale );
}

const TArray< FTextLayout::FLineModel >& FTextLayout::GetLineModels() const
{
	return LineModels;
}

const TArray< FTextLayout::FLineView >& FTextLayout::GetLineViews() const
{
	return LineViews;
}

FTextLayout::~FTextLayout()
{

}

FTextLayout::FLineModel::FLineModel( const TSharedRef< FString >& InText ) : Text( InText )
	, Runs()
	, BreakCandidates()
	, Highlights()
{

}

void FTextLayout::FRunModel::ClearCache()
{
	MeasuredRanges.Empty();
	MeasuredRangeSizes.Empty();
}

TSharedRef< ILayoutBlock > FTextLayout::FRunModel::CreateBlock( const FBlockDefinition& BlockDefine, float Scale ) const
{
	check( BlockDefine.Range.EndIndex >= BlockDefine.VisualRange.EndIndex );
	check( BlockDefine.Range.BeginIndex <= BlockDefine.VisualRange.BeginIndex );

	const FTextRange& SizeRange = BlockDefine.VisualRange;

	if ( MeasuredRanges.Num() == 0 )
	{
		FTextRange RunRange = Run->GetTextRange();
		return Run->CreateBlock( BlockDefine.Range.BeginIndex, BlockDefine.Range.EndIndex, Run->Measure( SizeRange.BeginIndex, SizeRange.EndIndex, Scale ), BlockDefine.Highlighter );
	}

	int32 StartRangeIndex = 0;
	int32 EndRangeIndex = 0;

	if ( MeasuredRanges.Num() > 16 )
	{
		if ( SizeRange.BeginIndex != 0 )
		{
			StartRangeIndex = BinarySearchForBeginIndex( MeasuredRanges, SizeRange.BeginIndex );
			check( StartRangeIndex != INDEX_NONE);
		}

		EndRangeIndex = StartRangeIndex;
		if ( StartRangeIndex != MeasuredRanges.Num() - 1 )
		{
			EndRangeIndex = BinarySearchForEndIndex( MeasuredRanges, StartRangeIndex, SizeRange.EndIndex );
			check( EndRangeIndex != INDEX_NONE);
		}
	}
	else
	{
		const int32 MaxValidIndex = MeasuredRanges.Num() - 1;

		if ( SizeRange.BeginIndex != 0 )
		{
			bool FoundBeginIndexMatch = false;
			for (; StartRangeIndex < MaxValidIndex; StartRangeIndex++)
			{
				FoundBeginIndexMatch = MeasuredRanges[ StartRangeIndex ].BeginIndex >= SizeRange.BeginIndex;
				if ( FoundBeginIndexMatch ) 
				{
					break;
				}
			}
			check( FoundBeginIndexMatch == true || StartRangeIndex == MaxValidIndex );
		}

		EndRangeIndex = StartRangeIndex;
		if ( StartRangeIndex != MaxValidIndex )
		{
			bool FoundEndIndexMatch = false;
			for (; EndRangeIndex < MeasuredRanges.Num(); EndRangeIndex++)
			{
				FoundEndIndexMatch = MeasuredRanges[ EndRangeIndex ].EndIndex >= SizeRange.EndIndex;
				if ( FoundEndIndexMatch ) 
				{
					break;
				}
			}
			check( FoundEndIndexMatch == true || EndRangeIndex == MaxValidIndex );
		}
	}

	FVector2D BlockSize( ForceInitToZero );
	if ( StartRangeIndex == EndRangeIndex )
	{
		if ( MeasuredRanges[ StartRangeIndex ].BeginIndex == SizeRange.BeginIndex && 
			MeasuredRanges[ StartRangeIndex ].EndIndex == SizeRange.EndIndex )
		{
			BlockSize += MeasuredRangeSizes[ StartRangeIndex ];
		}
		else
		{
			BlockSize += Run->Measure( SizeRange.BeginIndex, SizeRange.EndIndex, Scale );
		}
	}
	else
	{
		if ( MeasuredRanges[ StartRangeIndex ].BeginIndex == SizeRange.BeginIndex )
		{
			BlockSize += MeasuredRangeSizes[ StartRangeIndex ];
		}
		else
		{
			BlockSize += Run->Measure( SizeRange.BeginIndex, MeasuredRanges[ StartRangeIndex ].EndIndex, Scale );
		}

		for (int Index = StartRangeIndex + 1; Index < EndRangeIndex; Index++)
		{
			BlockSize.X += MeasuredRangeSizes[ Index ].X;
			BlockSize.Y = FMath::Max( MeasuredRangeSizes[ Index ].Y, BlockSize.Y );
		}

		if ( MeasuredRanges[ EndRangeIndex ].EndIndex == SizeRange.EndIndex )
		{
			BlockSize.X += MeasuredRangeSizes[ EndRangeIndex ].X;
			BlockSize.Y = FMath::Max( MeasuredRangeSizes[ EndRangeIndex ].Y, BlockSize.Y );
		}
		else
		{
			FVector2D Size = Run->Measure( MeasuredRanges[ EndRangeIndex ].BeginIndex, SizeRange.EndIndex, Scale );
			BlockSize.X += Size.X;
			BlockSize.Y = FMath::Max( Size.Y, BlockSize.Y );
		}
	}

	return Run->CreateBlock( BlockDefine.Range.BeginIndex, BlockDefine.Range.EndIndex, BlockSize, BlockDefine.Highlighter );
}

int FTextLayout::FRunModel::BinarySearchForEndIndex( const TArray< FTextRange >& Ranges, int32 RangeBeginIndex, int32 EndIndex )
{
	int32 Min = RangeBeginIndex;
	int32 Mid = 0;
	int32 Max = Ranges.Num() - 1;
	while (Max >= Min)
	{
		Mid = Min + ((Max - Min) / 2);
		if ( Ranges[Mid].EndIndex == EndIndex )
		{
			return Mid; 
		}
		else if ( Ranges[Mid].EndIndex < EndIndex )
		{
			Min = Mid + 1;
		}
		else
		{
			Max = Mid - 1;
		}
	}

	return Mid;
}

int FTextLayout::FRunModel::BinarySearchForBeginIndex( const TArray< FTextRange >& Ranges, int32 BeginIndex )
{
	int32 Min = 0;
	int32 Mid = 0;
	int32 Max = Ranges.Num() - 1;
	while (Max >= Min)
	{
		Mid = Min + ((Max - Min) / 2);
		if ( Ranges[Mid].BeginIndex == BeginIndex )
		{
			return Mid; 
		}
		else if ( Ranges[Mid].BeginIndex < BeginIndex )
		{
			Min = Mid + 1;
		}
		else
		{
			Max = Mid - 1;
		}
	}

	return Mid;
}

uint8 FTextLayout::FRunModel::GetKerning( int32 CurrentIndex, float Scale )
{
	return Run->GetKerning( CurrentIndex, Scale );
}

FVector2D FTextLayout::FRunModel::Measure( int32 BeginIndex, int32 EndIndex, float Scale )
{
	FVector2D Size = Run->Measure( BeginIndex, EndIndex, Scale );

	MeasuredRanges.Add( FTextRange( BeginIndex, EndIndex ) );
	MeasuredRangeSizes.Add( Size );

	return Size;
}

int16 FTextLayout::FRunModel::GetMaxHeight( float Scale ) const
{
	return Run->GetMaxHeight( Scale );
}

int16 FTextLayout::FRunModel::GetBaseLine( float Scale ) const
{
	return Run->GetBaseLine( Scale );
}

FTextRange FTextLayout::FRunModel::GetTextRange() const
{
	return Run->GetTextRange();
}

void FTextLayout::FRunModel::EndLayout()
{
	Run->EndLayout();
}

void FTextLayout::FRunModel::BeginLayout()
{
	Run->BeginLayout();
}

TSharedRef< IRun > FTextLayout::FRunModel::GetRun() const
{
	return Run;
}

FTextLayout::FRunModel::FRunModel( const TSharedRef< IRun >& InRun ) : Run( InRun )
{

}
