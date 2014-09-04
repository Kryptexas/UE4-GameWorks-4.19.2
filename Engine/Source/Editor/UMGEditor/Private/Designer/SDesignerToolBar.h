// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SViewportToolBar.h"

/**
 * 
 */
class SDesignerToolBar : public SViewportToolBar
{
public:
	SLATE_BEGIN_ARGS( SDesignerToolBar ){}
		SLATE_ARGUMENT( TSharedPtr<FUICommandList>, CommandList )
		SLATE_ARGUMENT( TSharedPtr<FExtender>, Extenders )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	/**
	 * Static: Creates a widget for the main tool bar
	 *
	 * @return	New widget
	 */
	TSharedRef< SWidget > MakeToolBar( const TSharedPtr< FExtender > InExtenders );	

private:

	/** Command list */
	TSharedPtr<FUICommandList> CommandList;
};