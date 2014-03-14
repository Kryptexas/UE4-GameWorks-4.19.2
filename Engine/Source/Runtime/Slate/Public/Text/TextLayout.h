// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "LineBreakIterator.h"

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

class SLATE_API FTextLayout
{
public:

	struct FBlockDefinition
	{
		FTextRange Range;
		FTextRange VisualRange;
		TSharedPtr< IRunHighlighter > Highlighter;
	};

	struct FBreakCandidate
	{
		FTextRange Range; 
		FTextRange RangeWithoutTrailingWhitespace; 
		FVector2D Size;
		FVector2D SizeWithoutTrailingWhitespace;

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
		int16 GetBaseLine( float Scale ) const;
		int16 GetMaxHeight( float Scale ) const;

		FVector2D Measure( int32 BeginIndex, int32 EndIndex, float Scale );

		uint8 GetKerning( int32 CurrentIndex, float Scale );

		static int BinarySearchForBeginIndex( const TArray< FTextRange >& Ranges, int32 BeginIndex );

		static int BinarySearchForEndIndex( const TArray< FTextRange >& Ranges, int32 RangeBeginIndex, int32 EndIndex );

		TSharedRef< ILayoutBlock > CreateBlock( const FBlockDefinition& BlockDefine, float Scale ) const;

		void ClearCache();

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
	};

	struct FLineView
	{
		TArray< TSharedRef< ILayoutBlock > > Blocks;
		FVector2D Size;
		FVector2D Offset;
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

protected:

	FTextLayout();

	/**
	* Prepares the cache with the information needed to perform line wrapping if the information does not already exist.
	*/
	void ReadyForWrapping();

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

	void CreateLineViewBlocks( const FLineModel& Line, const int32 StopIndex, const int32 VisualStopIndex, int32& OutRunIndex, int32& OutHighlightIndex, int32& OutPreviousBlockEnd, TArray< TSharedRef< ILayoutBlock > >& OutSoftLine );

	FBreakCandidate CreateBreakCandidate( int32& OutRunIndex, FLineModel& Line, int32 PreviousBreak, int32 CurrentBreak );

protected:

	/** The models for the lines of text. A LineModel represents a single string with no manual breaks. */
	TArray< FLineModel > LineModels;

	/** The views for the lines of text. A LineView represents a single visual line of text. Multiple LineViews can map to the same LineModel, if for example wrapping occurs. */
	TArray< FLineView > LineViews;

	/** Whether or not we currently have the information cached to perform line wrapping. */
	bool HasWrapInformation;

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