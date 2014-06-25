// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#define TEXT_LAYOUT_DEBUG 0

namespace ETextJustify
{
	enum Type
	{
		Left,
		Center,
		Right
	};
}

/** Location within the text model. */
struct FTextLocation
{
public:
	FTextLocation( const int32 InLineIndex = 0, const int32 InOffset = 0 )
		: LineIndex( InLineIndex )
		, Offset( InOffset )
	{

	}

	FTextLocation( const FTextLocation& InLocation, const int32 InOffset )
		: LineIndex( InLocation.GetLineIndex() )
		, Offset(FMath::Max(InLocation.GetOffset() + InOffset, 0))
	{

	}

	bool operator==( const FTextLocation& Other ) const
	{
		return
			LineIndex == Other.LineIndex &&
			Offset == Other.Offset;
	}

	bool operator!=( const FTextLocation& Other ) const
	{
		return
			LineIndex != Other.LineIndex ||
			Offset != Other.Offset;
	}

	bool operator<( const FTextLocation& Other ) const
	{
		return this->LineIndex < Other.LineIndex || (this->LineIndex == Other.LineIndex && this->Offset < Other.Offset);
	}

	int32 GetLineIndex() const { return LineIndex; }
	int32 GetOffset() const { return Offset; }
	bool IsValid() const { return LineIndex != INDEX_NONE && Offset != INDEX_NONE; }

private:
	int32 LineIndex;
	int32 Offset;
};

class FTextSelection
{

public:

	FTextLocation LocationA;

	FTextLocation LocationB;

public:

	FTextSelection()
		: LocationA(INDEX_NONE)
		, LocationB(INDEX_NONE)
	{
	}

	FTextSelection(const FTextLocation& InLocationA, const FTextLocation& InLocationB)
		: LocationA(InLocationA)
		, LocationB(InLocationB)
	{
	}

	const FTextLocation& GetBeginning() const
	{
		if (LocationA.GetLineIndex() == LocationB.GetLineIndex())
		{
			if (LocationA.GetOffset() < LocationB.GetOffset())
			{
				return LocationA;
			}

			return LocationB;
		}
		else if (LocationA.GetLineIndex() < LocationB.GetLineIndex())
		{
			return LocationA;
		}

		return LocationB;
	}

	const FTextLocation& GetEnd() const
	{
		if (LocationA.GetLineIndex() == LocationB.GetLineIndex())
		{
			if (LocationA.GetOffset() > LocationB.GetOffset())
			{
				return LocationA;
			}

			return LocationB;
		}
		else if (LocationA.GetLineIndex() > LocationB.GetLineIndex())
		{
			return LocationA;
		}

		return LocationB;
	}
};

class SLATE_API FTextLayout
{
public:

	struct FBlockDefinition
	{
		/** Range inclusive of trailing whitespace, as used to visually display and interact with the text */
		FTextRange ActualRange;

		TSharedPtr< IRunHighlighter > Highlighter;
	};

	struct FBreakCandidate
	{
		/** Range inclusive of trailing whitespace, as used to visually display and interact with the text */
		FTextRange ActualRange;
		/** Range exclusive of trailing whitespace, as used to perform wrapping on a word boundary */
		FTextRange TrimmedRange;
		/** Measured size inclusive of trailing whitespace, as used to visually display and interact with the text */
		FVector2D ActualSize;
		/** Measured size exclusive of trailing whitespace, as used to perform wrapping on a word boundary */
		FVector2D TrimmedSize;

		int16 MaxAboveBaseline;
		int16 MaxBelowBaseline;

		uint8 Kerning;

#if TEXT_LAYOUT_DEBUG
		FString DebugSlice;
#endif
	};

	class FRunModel
	{
	public:

		FRunModel( const TSharedRef< IRun >& InRun);

	public:

		TSharedRef< IRun > GetRun() const;;

		void BeginLayout();
		void EndLayout();

		FTextRange GetTextRange() const;
		void SetTextRange( const FTextRange& Value );
		
		int16 GetBaseLine( float Scale ) const;
		int16 GetMaxHeight( float Scale ) const;

		FVector2D Measure( int32 BeginIndex, int32 EndIndex, float Scale );

		uint8 GetKerning( int32 CurrentIndex, float Scale );

		static int BinarySearchForBeginIndex( const TArray< FTextRange >& Ranges, int32 BeginIndex );

		static int BinarySearchForEndIndex( const TArray< FTextRange >& Ranges, int32 RangeBeginIndex, int32 EndIndex );

		TSharedRef< ILayoutBlock > CreateBlock( const FBlockDefinition& BlockDefine, float Scale ) const;

		void ClearCache();

		void AppendText(FString& Text) const;
		void AppendText(FString& Text, const FTextRange& Range) const;

	private:

		TSharedRef< IRun > Run;
		TArray< FTextRange > MeasuredRanges;
		TArray< FVector2D > MeasuredRangeSizes;
	};

	struct FLineModel
	{
	public:

		FLineModel( const TSharedRef< FString >& InText );

		TSharedRef< FString > Text;
		TArray< FRunModel > Runs;
		TArray< FBreakCandidate > BreakCandidates;
		TArray< FTextHighlight > Highlights;
		mutable bool HasWrappingInformation;
	};

	struct FLineView
	{
		TArray< TSharedRef< ILayoutBlock > > Blocks;
		FVector2D Offset;
		FVector2D Size;
		FVector2D TextSize;
		FTextRange Range;
		int32 ModelIndex;
	};


public:

	virtual ~FTextLayout();

	const TArray< FTextLayout::FLineView >& GetLineViews() const;
	const TArray< FTextLayout::FLineModel >& GetLineModels() const;

	FVector2D GetSize() const;
	FVector2D GetDrawSize() const;

	float GetWrappingWidth() const;
	void SetWrappingWidth( float Value );

	float GetLineHeightPercentage() const;
	void SetLineHeightPercentage( float Value );

	ETextJustify::Type GetJustification() const;
	void SetJustification( ETextJustify::Type Value );

	float GetScale() const;
	void SetScale( float Value );

	FMargin GetMargin() const;
	void SetMargin( const FMargin& InMargin );

	void ClearLines();

	void AddLine( const TSharedRef< FString >& Text, const TArray< TSharedRef< IRun > >& Runs );

	/**
	* Clears all highlights
	*/
	void ClearHighlights();

	/**
	* Adds a single highlight to the existing set of highlights.
	*/
	void AddHighlight( const FTextHighlight& Highlight );

	/**
	* Replaces the current set of highlights with the provided highlights.
	*/
	void SetHighlights( const TArray< FTextHighlight >& Highlights );

	/**
	* Updates the TextLayout's if any changes have occurred since the last update.
	*/
	virtual void UpdateIfNeeded();

	virtual void Update();

	int32 GetLineViewIndexForTextLocation(const TArray< FTextLayout::FLineView >& LineViews, const FTextLocation& Location);

	/**
	 * 
	 */
	FTextLocation GetTextLocationAt( const FVector2D& Relative );

	FTextLocation GetTextLocationAt( const FLineView& LineView, const FVector2D& Relative );

	FVector2D GetLocationAt( const FTextLocation& Location );

	bool SplitLineAt(const FTextLocation& Location);

	bool JoinLineWithNextLine(int32 LineIndex);

	bool InsertAt(const FTextLocation& Location, TCHAR Character);

	bool InsertAt( const FTextLocation& Location, const FString& Text );

	bool RemoveAt( const FTextLocation& Location, int32 Count = 1 );

	bool RemoveLine(int32 LineIndex);

	bool IsEmpty() const;

	void GetAsText(FString& DisplayText) const;

	void GetAsText(FText& DisplayText) const;

	void GetSelectionAsText(FString& DisplayText, const FTextSelection& Selection) const;

	FTextSelection GetWordAt(const FTextLocation& Location) const;

protected:

	FTextLayout();

	/**
	* Create the wrapping cache for the current text based upon the current scale
	* Each line keeps its own cached state, so needs to be cleared when changing the text within a line
	* When changing the scale, all the lines need to be cleared (see ClearWrappingCache)
	*/
	void CreateWrappingCache();

	/**
	 * Clears the current wrapping cache for all lines
	 */
	void ClearWrappingCache();

	/**
	* Clears the current layouts view information.
	*/
	void ClearView();

	/**
	* Notifies all Runs that we are beginning to generate a new layout.
	*/
	virtual void BeginLayout();

	/**
	* Notifies all Runs that the layout has finished generating.
	*/
	virtual void EndLayout();

	/**
	* Creates a new View for the layout, replacing the current one.
	*/
	void CreateView();


private:

	void FlowLayout();

	void JustifyLayout();

	void CreateLineViewBlocks( int32 LineModelIndex, const int32 StopIndex, int32& OutRunIndex, int32& OutHighlightIndex, int32& OutPreviousBlockEnd, TArray< TSharedRef< ILayoutBlock > >& OutSoftLine );

	FBreakCandidate CreateBreakCandidate( int32& OutRunIndex, FLineModel& Line, int32 PreviousBreak, int32 CurrentBreak );

protected:

	/** The models for the lines of text. A LineModel represents a single string with no manual breaks. */
	TArray< FLineModel > LineModels;

	/** The views for the lines of text. A LineView represents a single visual line of text. Multiple LineViews can map to the same LineModel, if for example wrapping occurs. */
	TArray< FLineView > LineViews;

	/** Whether parameters on the layout have changed which requires the view be updated. */
	bool HasChanged;

	/** The scale to draw the text at */
	float Scale;

	/** The width that the text should be wrap at. If 0 or negative no wrapping occurs. */
	float WrappingWidth;

	/** The size of the margins to put about the text. This is an unscaled value. */
	FMargin Margin;

	/** How the text should be aligned with the margin. */
	ETextJustify::Type Justification;

	/** The percentage to modify a line height by. */
	float LineHeightPercentage;

	/** The final size of the text layout on screen. */
	FVector2D DrawSize;
};