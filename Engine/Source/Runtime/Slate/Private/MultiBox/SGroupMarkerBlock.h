// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Group Start MultiBlock
 */
class FGroupStartBlock
	: public FMultiBlock
{
public:
	FGroupStartBlock();

	virtual bool IsGroupStartBlock()	const	{ return true; };
private:

	/**
	 * Allocates a widget for this type of MultiBlock.  Override this in derived classes.
	 *
	 * @return  MultiBlock widget object
	 */
	virtual TSharedRef< class IMultiBlockBaseWidget > ConstructWidget() const;
};

/**
 * Group End MultiBlock
 */
class FGroupEndBlock
	: public FMultiBlock
{

public:
	FGroupEndBlock();

	virtual bool IsGroupEndBlock()	const	{ return true; };
private:

	/**
	 * Allocates a widget for this type of MultiBlock.  Override this in derived classes.
	 *
	 * @return  MultiBlock widget object
	 */
	virtual TSharedRef< class IMultiBlockBaseWidget > ConstructWidget() const;
};

/**
 * Group Marker MultiBlock widget
 */
class SLATE_API SGroupMarkerBlock 
	: public SMultiBlockBaseWidget
{

public:

	SLATE_BEGIN_ARGS( SGroupMarkerBlock ){}
	SLATE_END_ARGS()

	/**
	 * Builds this MultiBlock widget up from the MultiBlock associated with it
	 */
	virtual void BuildMultiBlockWidget(const ISlateStyle* StyleSet, const FName& StyleName) OVERRIDE;

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs ) {};

private:

};