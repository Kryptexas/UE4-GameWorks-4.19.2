// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A sources view designed for path picking
 */
class SPathPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SPathPicker ){}

		/** A struct containing details about how the path picker should behave */
		SLATE_ARGUMENT(FPathPickerConfig, PathPickerConfig)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );
};