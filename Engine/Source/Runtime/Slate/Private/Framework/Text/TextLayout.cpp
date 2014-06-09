// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"
#include "LineBreakIterator.h"
#include "WordBreakIterator.h"

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

		while( WhitespaceStopIndex > BeginIndex && FText::IsWhitespace( (*Line.Text)[ WhitespaceStopIndex - 1 ] ) )
		{
			--WhitespaceStopIndex;
		}

		if ( BeginIndex == StopIndex )
		{
			// This slice is empty, no need to adjust anything
			SliceSize = SliceSizeWithoutTrailingWhitespace = FVector2D::ZeroVector;
		}
		else if ( BeginIndex == WhitespaceStopIndex )
		{
			// This slice contains only whitespace, no need to adjust SliceSizeWithoutTrailingWhitespace
			SliceSize = Run.Measure( BeginIndex, StopIndex, Scale );
			SliceSizeWithoutTrailingWhitespace = FVector2D::ZeroVector;
		}
		else if ( WhitespaceStopIndex != StopIndex )
		{
			// This slice contains trailing whitespace, measure the text size, then add on the whitespace size
			SliceSize = SliceSizeWithoutTrailingWhitespace = Run.Measure( BeginIndex, WhitespaceStopIndex, Scale );
			SliceSize.X += Run.Measure( WhitespaceStopIndex, StopIndex, Scale ).X;
		}
		else
		{
			// This slice contains no whitespace, both sizes are the same and can use the same measurement
			SliceSize = SliceSizeWithoutTrailingWhitespace = Run.Measure( BeginIndex, StopIndex, Scale );
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
	BreakCandidate.ActualSize = BreakSize;
	BreakCandidate.TrimmedSize = BreakSizeWithoutTrailingWhitespace;
	BreakCandidate.ActualRange = FTextRange( PreviousBreak, CurrentBreak );
	BreakCandidate.TrimmedRange = FTextRange( PreviousBreak, WhitespaceStopIndex );
	BreakCandidate.MaxAboveBaseline = MaxAboveBaseline;
	BreakCandidate.MaxBelowBaseline = MaxBelowBaseline;
	BreakCandidate.Kerning = Kerning;

#if TEXT_LAYOUT_DEBUG
	BreakCandidate.DebugSlice = FString( BreakCandidate.Range.EndIndex - BreakCandidate.Range.BeginIndex, (**Line.Text) + BreakCandidate.Range.BeginIndex );
#endif

	return BreakCandidate;
}

void FTextLayout::CreateLineViewBlocks( int32 LineModelIndex, const int32 StopIndex, int32& OutRunIndex, int32& OutHighlightIndex, int32& OutPreviousBlockEnd, TArray< TSharedRef< ILayoutBlock > >& OutSoftLine )
{
	const FLineModel& LineModel = LineModels[ LineModelIndex ];

	int16 MaxAboveBaseline = 0;
	int16 MaxBelowBaseline = 0;

	for (; OutRunIndex < LineModel.Runs.Num(); )
	{
		const FRunModel& Run = LineModel.Runs[ OutRunIndex ];
		const FTextRange RunRange = Run.GetTextRange();

		TSharedPtr< IRunHighlighter > BlockHighlighter = NULL;

		int32 BlockStopIndex = RunRange.EndIndex;
		if ( OutHighlightIndex != INDEX_NONE )
		{
			//Grab the currently active highlighter
			const FTextHighlight& Highlight = LineModel.Highlights[OutHighlightIndex];

			//Check to see if the last block was highlighted with the same highlighter
			if ( OutPreviousBlockEnd >= Highlight.Range.BeginIndex )
			{
				//If the highlighter ends before our run...
				if ( Highlight.Range.EndIndex <= RunRange.EndIndex )
				{
					//Adjust the stopping point of the block to be the end of the highlight,
					//since highlights need their own block segments
					BlockStopIndex = Highlight.Range.EndIndex;
					BlockHighlighter = Highlight.Highlighter;
				}
				else
				{
					//This whole run is encompassed by the highlighter
					BlockHighlighter = Highlight.Highlighter;
				}
			}
			else
			{
				//Does the highlight begin before our run ends?
				if ( Highlight.Range.BeginIndex <= RunRange.EndIndex )
				{
					// then adjust the current block stopping point to just before the highlight begins,
					//since highlights need their own block segments
					BlockStopIndex = Highlight.Range.BeginIndex;
					BlockHighlighter = NULL;
				}
			}
		}

		if (StopIndex != INDEX_NONE)
		{
			BlockStopIndex = FMath::Min(StopIndex, BlockStopIndex);
		}

		int32 BlockBeginIndex = FMath::Max( OutPreviousBlockEnd, RunRange.BeginIndex );
		const bool IsLastBlock = BlockStopIndex == StopIndex;

		// if this new block will be entirely encapsulated by the run...
		if ( RunRange.BeginIndex < BlockStopIndex && RunRange.EndIndex > BlockBeginIndex )
		{
			check( BlockBeginIndex <= BlockStopIndex );

			FBlockDefinition BlockDefine;
			BlockDefine.ActualRange = FTextRange(BlockBeginIndex, BlockStopIndex);
			BlockDefine.Highlighter = BlockHighlighter;

			OutSoftLine.Add( Run.CreateBlock( BlockDefine, Scale ) );
			OutPreviousBlockEnd = BlockStopIndex;
		}
		else
		{
			FBlockDefinition BlockDefine;
			BlockDefine.ActualRange = RunRange;
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

		if ( OutHighlightIndex != INDEX_NONE && BlockStopIndex == LineModel.Highlights[ OutHighlightIndex ].Range.EndIndex )
		{
			++OutHighlightIndex;

			if ( OutHighlightIndex >= LineModel.Highlights.Num() )
			{
				OutHighlightIndex = INDEX_NONE;
			}
		}

		if ( IsLastBlock )
		{
			break;
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
			const TSharedRef< IRun > Run = Block->GetRun();

			const int16 BlockBaseline = Run->GetBaseLine(Scale);
			const int16 VerticalOffset = MaxAboveBaseline - Block->GetSize().Y - BlockBaseline;
			const int8 BlockKerning = Run->GetKerning(Block->GetTextRange().BeginIndex, Scale);

			Block->SetLocationOffset(FVector2D(CurrentOffset.X + CurrentHorizontalPos + BlockKerning, CurrentOffset.Y + VerticalOffset));

			CurrentHorizontalPos += Block->GetSize().X;
		}

		const float UnscaleLineHeight = MaxAboveBaseline + MaxBelowBaseline;

		LineSize.X = CurrentHorizontalPos;
		LineSize.Y = UnscaleLineHeight * LineHeightPercentage;

		FTextLayout::FLineView LineView;
		LineView.Offset = CurrentOffset;
		LineView.Size = LineSize;
		LineView.TextSize = FVector2D(CurrentHorizontalPos, UnscaleLineHeight);
		LineView.Range = FTextRange(OutSoftLine[0]->GetTextRange().BeginIndex, OutSoftLine.Last()->GetTextRange().EndIndex);
		LineView.ModelIndex = LineModelIndex;
		LineView.Blocks.Append( OutSoftLine );

		LineViews.Add( LineView );
	}

	DrawSize.X = FMath::Max( DrawSize.X, LineSize.X ); //DrawSize.X is the size of the longest line + the Margin
	DrawSize.Y += LineSize.Y; //DrawSize.Y is the total height of all lines
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
	const float WrappingDrawWidth = FMath::Max( 0.01f, ( WrappingWidth - Margin.GetTotalSpaceAlong<Orient_Horizontal>() ) * Scale );

	CreateWrappingCache();

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
			CreateLineViewBlocks( LineModelIndex, INDEX_NONE, /*OUT*/CurrentRunIndex, /*OUT*/CurrentHighlightIndex, /*OUT*/PreviousBlockEnd, SoftLine );
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
				const bool BreakDoesFit = !IsWrapping || CurrentWidth + Break.ActualSize.X + Kerning <= WrappingDrawWidth;

				if ( !BreakDoesFit || IsLastBreak )
				{
					const bool BreakWithoutTrailingWhitespaceDoesFit = !IsWrapping || CurrentWidth + Break.TrimmedSize.X + Kerning <= WrappingDrawWidth;
					const bool IsFirstBreak = BreakIndex == 0;

					const FBreakCandidate& FinalBreakOnSoftLine = ( !IsFirstBreak && !IsFirstBreakOnSoftLine && !BreakWithoutTrailingWhitespaceDoesFit ) ? LineModel.BreakCandidates[ --BreakIndex ] : Break;

					CreateLineViewBlocks( LineModelIndex, FinalBreakOnSoftLine.ActualRange.EndIndex, /*OUT*/CurrentRunIndex, /*OUT*/CurrentHighlightIndex, /*OUT*/PreviousBlockEnd, SoftLine );

					if ( CurrentRunIndex < LineModel.Runs.Num() && FinalBreakOnSoftLine.ActualRange.EndIndex == LineModel.Runs[ CurrentRunIndex ].GetTextRange().EndIndex )
					{
						++CurrentRunIndex;
					}

					PreviousBlockEnd = FinalBreakOnSoftLine.ActualRange.EndIndex;

					CurrentWidth = 0;
					SoftLine.Empty();
				} 
				else
				{
					CurrentWidth += Break.ActualSize.X;
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

void FTextLayout::CreateWrappingCache()
{
	for (int LineModelIndex = 0; LineModelIndex < LineModels.Num(); LineModelIndex++)
	{
		FLineModel& LineModel = LineModels[ LineModelIndex ];

		if ( !LineModel.HasWrappingInformation )
		{
			LineModel.BreakCandidates.Empty();
			LineModel.HasWrappingInformation = true;

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
	}
}

void FTextLayout::ClearWrappingCache()
{
	for (int LineModelIndex = 0; LineModelIndex < LineModels.Num(); LineModelIndex++)
	{
		FLineModel& LineModel = LineModels[ LineModelIndex ];
		LineModel.HasWrappingInformation = false;
	}
}

FTextLayout::FTextLayout() : LineModels()
	, LineViews()
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
		Update();
	}
}

void FTextLayout::Update()
{
	CreateView();
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

FTextLocation FTextLayout::GetTextLocationAt( const FLineView& LineView, const FVector2D& Relative )
{
	for (int32 BlockIndex = 0; BlockIndex < LineView.Blocks.Num(); BlockIndex++)
	{
		const TSharedRef< ILayoutBlock >& Block = LineView.Blocks[ BlockIndex ];
		const int32 TextIndex = Block->GetRun()->GetTextIndexAt(Block, FVector2D(Relative.X, Block->GetLocationOffset().Y), Scale);

		if ( TextIndex == INDEX_NONE )
		{
			continue;
		}

		return FTextLocation( LineView.ModelIndex, TextIndex );
	}

	const int32 LineTextLength = LineModels[ LineView.ModelIndex ].Text->Len();
	if ( LineTextLength == 0 )
	{
		return FTextLocation( LineView.ModelIndex, 0 );
	}
	else if (Relative.X < LineView.Offset.X)
	{
		return FTextLocation(LineView.ModelIndex, LineView.Range.BeginIndex);
	}

	return FTextLocation( LineView.ModelIndex, LineView.Range.EndIndex );
}

int32 FTextLayout::GetLineViewIndexForTextLocation(const TArray< FTextLayout::FLineView >& LineViews, const FTextLocation& Location)
{
	const int32 LineModelIndex = Location.GetLineIndex();
	const int32 Offset = Location.GetOffset();

	const FLineModel& LineModel = LineModels[LineModelIndex];
	for (int Index = 0; Index < LineViews.Num(); Index++)
	{
		const FTextLayout::FLineView& LineView = LineViews[Index];

		if (LineView.ModelIndex == LineModelIndex && (Offset == 0 || LineView.Range.InclusiveContains(Offset) || LineModel.Text->IsEmpty()))
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

FTextLocation FTextLayout::GetTextLocationAt( const FVector2D& Relative )
{
	// Early out if we have no LineViews
	if (LineViews.Num() == 0)
	{
		return FTextLocation(0,0);
	}

	// Iterate until we find a LineView that is below our expected Y location
	int32 ViewIndex;
	for (ViewIndex = 0; ViewIndex < LineViews.Num(); ViewIndex++)
	{
		const FLineView& LineView = LineViews[ ViewIndex ];

		if (LineView.Offset.Y > Relative.Y)
		{
			// Set the ViewIndex back to the previous line, but not lower than the first line
			ViewIndex = FMath::Max( 0, ViewIndex - 1 );
			break;
		}
	}

	//if none of the lines are below our expected Y location then...
	if (ViewIndex >= LineViews.Num())
	{
		// just use the very last line
		ViewIndex = LineViews.Num() - 1;
		const FLineView& LineView = LineViews[ ViewIndex ];
		return GetTextLocationAt( LineView, Relative );
	}
	else 
	{
		// if the current LineView does not encapsulate our expected Y location then jump to the next LineView
		// if we aren't already at the last LineView
		const FLineView& LineView = LineViews[ ViewIndex ];
		if ((LineView.Offset.Y + LineView.Size.Y) < Relative.Y && ViewIndex < LineViews.Num() - 1)
		{
			++ViewIndex;
		}
	}

	const FLineView& LineView = LineViews[ ViewIndex ];
	return GetTextLocationAt(LineView, FVector2D(Relative.X, LineView.Offset.Y));
}

FVector2D FTextLayout::GetLocationAt( const FTextLocation& Location )
{
	const int32 LineModelIndex = Location.GetLineIndex();
	const int32 Offset = Location.GetOffset();

	// Find the LineView which encapsulates the location's offset
	int LineViewIndex;
	for (LineViewIndex = 0; LineViewIndex < LineViews.Num(); LineViewIndex++)
	{
		const FTextLayout::FLineView& LineView = LineViews[ LineViewIndex ];

		if ( LineView.ModelIndex == LineModelIndex && ( Offset == 0 || LineView.Range.InclusiveContains(Offset) ) )
		{
			break;
		}
	}

	// if we failed to find a LineView for the location, early out
	if ( !LineViews.IsValidIndex( LineViewIndex ) )
	{
		return FVector2D( ForceInitToZero );
	}

	const FTextLayout::FLineView& LineView = LineViews[ LineViewIndex ];

	//Iterate over the LineView's blocks...
	for (int32 BlockIndex = 0; BlockIndex < LineView.Blocks.Num(); BlockIndex++)
	{
		const TSharedRef< ILayoutBlock >& Block = LineView.Blocks[ BlockIndex ];
		const FTextRange& BlockRange = Block->GetTextRange();

		//if the block's range contains the specified locations offset...
		if (BlockRange.InclusiveContains(Offset))
		{
			// Ask the block for the exact screen location
			const FVector2D ScreenLocation = Block->GetRun()->GetLocationAt( Block, Offset - BlockRange.BeginIndex, Scale );

			// if the block was unable to provide a location continue iterating
			if ( ScreenLocation.IsZero() )
			{
				continue;
			}

			return ScreenLocation;
		}
	}

	// Failed to find the screen location
	return FVector2D( ForceInitToZero );
}

bool FTextLayout::InsertAt(const FTextLocation& Location, TCHAR Character)
{
	const int32 InsertLocation = Location.GetOffset();
	const int32 LineIndex = Location.GetLineIndex();

	if (!LineModels.IsValidIndex(LineIndex))
	{
		return false;
	}

	FLineModel& LineModel = LineModels[LineIndex];

	LineModel.Text->InsertAt(InsertLocation, Character);
	LineModel.HasWrappingInformation = false;

	bool RunIsAfterInsertLocation = false;
	for (int RunIndex = 0; RunIndex < LineModel.Runs.Num(); RunIndex++)
	{
		FRunModel& RunModel = LineModel.Runs[ RunIndex ];
		const FTextRange& RunRange = RunModel.GetTextRange();

		const bool bIsLastRun = RunIndex == LineModel.Runs.Num() - 1;
		if (RunRange.Contains(InsertLocation) || (bIsLastRun && !RunIsAfterInsertLocation))
		{
			check(RunIsAfterInsertLocation == false);
			RunIsAfterInsertLocation = true;

			RunModel.SetTextRange( FTextRange( RunRange.BeginIndex, RunRange.EndIndex + 1 ) );
		}
		else if (RunIsAfterInsertLocation)
		{
			FTextRange NewRange = RunRange;
			NewRange.Offset(1);
			RunModel.SetTextRange(NewRange);
		}
	}

	HasChanged = true;
	return true;
}

bool FTextLayout::InsertAt( const FTextLocation& Location, const FString& Text )
{
	const int32 InsertLocation = Location.GetOffset();
	const int32 LineIndex = Location.GetLineIndex();

	if (!LineModels.IsValidIndex(LineIndex))
	{
		return false;
	}

	FLineModel& LineModel = LineModels[LineIndex];

	LineModel.Text->InsertAt(InsertLocation, Text);
	LineModel.HasWrappingInformation = false;

	bool RunIsAfterInsertLocation = false;
	for (int RunIndex = 0; RunIndex < LineModel.Runs.Num(); RunIndex++)
	{
		FRunModel& RunModel = LineModel.Runs[ RunIndex ];
		const FTextRange& RunRange = RunModel.GetTextRange();

		const bool bIsLastRun = RunIndex == LineModel.Runs.Num() - 1;
		if (RunRange.Contains(InsertLocation) || (bIsLastRun && !RunIsAfterInsertLocation))
		{
			check(RunIsAfterInsertLocation == false);
			RunIsAfterInsertLocation = true;

			RunModel.SetTextRange( FTextRange( RunRange.BeginIndex, RunRange.EndIndex + Text.Len() ) );
		}
		else if (RunIsAfterInsertLocation)
		{
			FTextRange NewRange = RunRange;
			NewRange.Offset(Text.Len());
			RunModel.SetTextRange(NewRange);
		}
	}

	HasChanged = true;
	return true;
}

bool FTextLayout::JoinLineWithNextLine(int32 LineIndex)
{
	if (!LineModels.IsValidIndex(LineIndex) || !LineModels.IsValidIndex(LineIndex + 1))
	{
		return false;
	}

	//Grab both lines we are supposed to join
	FLineModel& LineModel = LineModels[LineIndex];
	FLineModel& NextLineModel = LineModels[LineIndex + 1];

	//If the next line is empty we'll just remove it
	if (NextLineModel.Text->Len() == 0)
	{
		return RemoveLine(LineIndex + 1);
	}

	const int32 LineLength = LineModel.Text->Len();

	//Append the next line to the current line
	LineModel.Text->InsertAt(LineLength, NextLineModel.Text.Get());

	//Clear the current lines wrapping information
	LineModel.HasWrappingInformation = false;

	//Iterate through all of the next lines runs and bring them over to the current line
	for (int RunIndex = 0; RunIndex < NextLineModel.Runs.Num(); RunIndex++)
	{
		const TSharedRef<IRun> Run = NextLineModel.Runs[RunIndex].GetRun();
		FTextRange NewRange = Run->GetTextRange();

		if (!NewRange.IsEmpty())
		{
			NewRange.Offset(LineLength);

			Run->Move(LineModel.Text, NewRange);
			LineModel.Runs.Add(FRunModel(Run));
		}
	}

	//Remove the next line from the list of line models
	LineModels.RemoveAt(LineIndex + 1);

	HasChanged = true;
	return true;
}

bool FTextLayout::SplitLineAt(const FTextLocation& Location)
{
	int32 BreakLocation = Location.GetOffset();
	int32 LineIndex = Location.GetLineIndex();

	if (!LineModels.IsValidIndex(LineIndex))
	{
		return false;
	}

	FLineModel& LineModel = LineModels[LineIndex];

	FLineModel LeftLineModel(MakeShareable(new FString(BreakLocation, **LineModel.Text)));
	FLineModel RightLineModel(MakeShareable(new FString(LineModel.Text->Len() - BreakLocation, **LineModel.Text + BreakLocation)));

	check(LeftLineModel.Text->Len() == BreakLocation);

	bool RunIsToTheLeftOfTheBreakLocation = true;
	for (int RunIndex = 0; RunIndex < LineModel.Runs.Num(); RunIndex++)
	{
		const TSharedRef<IRun> Run = LineModel.Runs[RunIndex].GetRun();
		const FTextRange& RunRange = Run->GetTextRange();
		
		if ((LineModel.Runs.Num() == 1 && RunRange.InclusiveContains(BreakLocation)) || RunRange.Contains(BreakLocation))
		{
			check(RunIsToTheLeftOfTheBreakLocation == true);
			RunIsToTheLeftOfTheBreakLocation = false;

			const TSharedRef<IRun> LeftRun = Run;
			const TSharedRef<IRun> RightRun = Run->Clone();
			
			LeftRun->Move(LeftLineModel.Text, FTextRange(RunRange.BeginIndex, LeftLineModel.Text->Len()));
			RightRun->Move(RightLineModel.Text, FTextRange(0, RunRange.EndIndex - LeftLineModel.Text->Len()));

			LeftLineModel.Runs.Add(FRunModel(LeftRun));
			RightLineModel.Runs.Add(FRunModel(RightRun));
		}
		else if (RunIsToTheLeftOfTheBreakLocation)
		{
			Run->Move(LeftLineModel.Text, RunRange);
			LeftLineModel.Runs.Add(FRunModel(Run));
		}
		else
		{
			// This run is after the break, so adjust the range to match that of RHS of the split (RightLineModel)
			// We can do this by subtracting the LeftLineModel text size, since that's the LHS of the split
			FTextRange NewRange = RunRange;
			NewRange.Offset(-LeftLineModel.Text->Len());

			Run->Move(RightLineModel.Text, NewRange);
			RightLineModel.Runs.Add(FRunModel(Run));
		}
	}

	LineModels.RemoveAt(LineIndex);
	LineModels.Insert(LeftLineModel, LineIndex);
	LineModels.Insert(RightLineModel, LineIndex + 1);

	HasChanged = true;
	return true;
}

bool FTextLayout::RemoveAt( const FTextLocation& Location, int32 Count )
{
	int32 RemoveLocation = Location.GetOffset();
	int32 LineIndex = Location.GetLineIndex();

	if (!LineModels.IsValidIndex(LineIndex))
	{
		return false;
	}

	FLineModel& LineModel = LineModels[LineIndex];

	//Make sure we aren't trying to remove more characters then we have
	Count = RemoveLocation + Count > LineModel.Text->Len() ? Count - ((RemoveLocation + Count) - LineModel.Text->Len()) : Count;

	if (Count == 0)
	{
		return false;
	}

	LineModel.Text->RemoveAt(RemoveLocation, Count);
	LineModel.HasWrappingInformation = false;

	bool RunIsAfterRemoveLocation = false;
	for (int RunIndex = 0; RunIndex < LineModel.Runs.Num(); RunIndex++)
	{
		FRunModel& RunModel = LineModel.Runs[RunIndex];
		const FTextRange& RunRange = RunModel.GetTextRange();

		if (RunRange.Contains(RemoveLocation))
		{
			check(RunIsAfterRemoveLocation == false);
			RunIsAfterRemoveLocation = true;

			FTextRange NewRange(RunRange.BeginIndex, RunRange.EndIndex - Count);
			if (NewRange.IsEmpty() && LineModel.Runs.Num() > 1)
			{
				LineModel.Runs.RemoveAt(RunIndex--);
			}
			else
			{
				RunModel.SetTextRange(NewRange);
			}
		}
		else if (RunIsAfterRemoveLocation)
		{
			FTextRange NewRange = RunRange;
			NewRange.Offset(-Count);
			RunModel.SetTextRange(NewRange);
		}
	}

	HasChanged = true;
	return true;
}

bool FTextLayout::RemoveLine(int32 LineIndex)
{
	if (!LineModels.IsValidIndex(LineIndex))
	{
		return false;
	}

	LineModels.RemoveAt(LineIndex);

	return true;;
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

bool FTextLayout::IsEmpty() const
{
	return (LineModels.Num() == 0 || (LineModels.Num() == 1 && LineModels[0].Text->Len() == 0));
}

void FTextLayout::GetAsText(FString& DisplayText) const
{
	for (int LineModelIndex = 0; LineModelIndex < LineModels.Num(); LineModelIndex++)
	{
		const FLineModel& LineModel = LineModels[LineModelIndex];

		for (int RunIndex = 0; RunIndex < LineModel.Runs.Num(); RunIndex++)
		{
			const FRunModel& Run = LineModel.Runs[RunIndex];
			Run.AppendText(DisplayText);
		}

		DisplayText.AppendChar('\n');
	}
}

void GetRangeAsTextFromLine(FString& DisplayText, const FTextLayout::FLineModel& LineModel, const FTextRange& Range)
{
	for (int RunIndex = 0; RunIndex < LineModel.Runs.Num(); RunIndex++)
	{
		const FTextLayout::FRunModel& Run = LineModel.Runs[RunIndex];
		const FTextRange& RunRange = Run.GetTextRange();

		const FTextRange IntersectRange = RunRange.Intersect(Range);

		if (!IntersectRange.IsEmpty())
		{
			Run.AppendText(DisplayText, IntersectRange);
		}
		else if (RunRange.BeginIndex > Range.EndIndex)
		{
			//We're past the selection range so we can stop
			break;
		}
	}
}

void FTextLayout::GetSelectionAsText(FString& DisplayText, const FTextSelection& Selection) const
{
	int32 SelectionBeginningLineIndex = Selection.GetBeginning().GetLineIndex();
	int32 SelectionBeginningLineOffset = Selection.GetBeginning().GetOffset();

	int32 SelectionEndLineIndex = Selection.GetEnd().GetLineIndex();
	int32 SelectionEndLineOffset = Selection.GetEnd().GetOffset();

	if (LineModels.IsValidIndex(SelectionBeginningLineIndex) && LineModels.IsValidIndex(SelectionEndLineIndex))
	{
		if (SelectionBeginningLineIndex == SelectionEndLineIndex)
		{
			const FTextRange SelectionRange(SelectionBeginningLineOffset, SelectionEndLineOffset);
			const FLineModel& LineModel = LineModels[SelectionBeginningLineIndex];

			GetRangeAsTextFromLine(DisplayText, LineModel, SelectionRange);
		}
		else
		{
			for (int LineIndex = SelectionBeginningLineIndex; LineIndex <= SelectionEndLineIndex; LineIndex++)
			{
				if (LineIndex == SelectionBeginningLineIndex)
				{
					const FLineModel& LineModel = LineModels[SelectionBeginningLineIndex];
					const FTextRange SelectionRange(SelectionBeginningLineOffset, LineModel.Text->Len());

					GetRangeAsTextFromLine(DisplayText, LineModel, SelectionRange);
				}
				else if (LineIndex == SelectionEndLineIndex)
				{
					const FLineModel& LineModel = LineModels[SelectionEndLineIndex];
					const FTextRange SelectionRange(0, SelectionEndLineOffset);

					GetRangeAsTextFromLine(DisplayText, LineModel, SelectionRange);
				}
				else
				{
					const FLineModel& LineModel = LineModels[LineIndex];
					const FTextRange SelectionRange(0, LineModel.Text->Len());

					GetRangeAsTextFromLine(DisplayText, LineModel, SelectionRange);
				}

				if (LineIndex != SelectionEndLineIndex)
				{
					DisplayText.AppendChar('\n');
				}
			}
		}
	}
}

FTextSelection FTextLayout::GetWordAt(const FTextLocation& Location) const
{
	const int32 LineIndex = Location.GetLineIndex();
	const int32 Offset = Location.GetOffset();

	if (!LineModels.IsValidIndex(LineIndex))
	{
		return FTextSelection();
	}

	const FLineModel& LineModel = LineModels[LineIndex];

	FWordBreakIterator WordBreakIterator(**LineModel.Text);

	int32 PreviousBreak = WordBreakIterator.MoveToCandidateAfter(Offset);
	int32 CurrentBreak = 0;

	while ((CurrentBreak = WordBreakIterator.MoveToPrevious()) != INDEX_NONE)
	{
		bool HasLetter = false;
		for (int32 Index = CurrentBreak; Index < PreviousBreak; Index++)
		{
			if (!FText::IsWhitespace((*LineModel.Text)[Index]))
			{
				HasLetter = true;
				break;
			}
		}

		if (HasLetter)
		{
			break;
		}

		PreviousBreak = CurrentBreak;
	}

	if (PreviousBreak == CurrentBreak || CurrentBreak == INDEX_NONE)
	{
		return FTextSelection();
	}

	return FTextSelection(FTextLocation(LineIndex, CurrentBreak), FTextLocation(LineIndex, PreviousBreak));
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

	// Changing the scale will affect the wrapping information for *all lines*
	// Clear out the entire cache so it gets regenerated on the text call to FlowLayout
	ClearWrappingCache();
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

FTextLayout::FLineModel::FLineModel( const TSharedRef< FString >& InText ) 
	: Text( InText )
	, Runs()
	, BreakCandidates()
	, Highlights()
	, HasWrappingInformation( false )
{

}

void FTextLayout::FRunModel::ClearCache()
{
	MeasuredRanges.Empty();
	MeasuredRangeSizes.Empty();
}

void FTextLayout::FRunModel::AppendText(FString& Text) const
{
	Run->AppendText(Text);
}

void FTextLayout::FRunModel::AppendText(FString& Text, const FTextRange& Range) const
{
	Run->AppendText(Text, Range);
}

TSharedRef< ILayoutBlock > FTextLayout::FRunModel::CreateBlock( const FBlockDefinition& BlockDefine, float Scale ) const
{
	const FTextRange& SizeRange = BlockDefine.ActualRange;

	if ( MeasuredRanges.Num() == 0 )
	{
		FTextRange RunRange = Run->GetTextRange();
		return Run->CreateBlock( BlockDefine.ActualRange.BeginIndex, BlockDefine.ActualRange.EndIndex, Run->Measure( SizeRange.BeginIndex, SizeRange.EndIndex, Scale ), BlockDefine.Highlighter );
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

	return Run->CreateBlock( BlockDefine.ActualRange.BeginIndex, BlockDefine.ActualRange.EndIndex, BlockSize, BlockDefine.Highlighter );
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

uint8 FTextLayout::FRunModel::GetKerning(int32 CurrentIndex, float InScale)
{
	return Run->GetKerning(CurrentIndex, InScale);
}

FVector2D FTextLayout::FRunModel::Measure(int32 BeginIndex, int32 EndIndex, float InScale)
{
	FVector2D Size = Run->Measure(BeginIndex, EndIndex, InScale);

	MeasuredRanges.Add( FTextRange( BeginIndex, EndIndex ) );
	MeasuredRangeSizes.Add( Size );

	return Size;
}

int16 FTextLayout::FRunModel::GetMaxHeight(float InScale) const
{
	return Run->GetMaxHeight(InScale);
}

int16 FTextLayout::FRunModel::GetBaseLine( float InScale ) const
{
	return Run->GetBaseLine(InScale);
}

FTextRange FTextLayout::FRunModel::GetTextRange() const
{
	return Run->GetTextRange();
}

void FTextLayout::FRunModel::SetTextRange( const FTextRange& Value )
{
	Run->SetTextRange( Value );
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
