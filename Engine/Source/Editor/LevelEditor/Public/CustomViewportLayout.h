// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


class FLevelViewportLayout;

struct FCustomViewportLayoutDefinition
{
	template<typename T>
	static FCustomViewportLayoutDefinition FromType()
	{
		return FCustomViewportLayoutDefinition([]() -> TSharedRef<FLevelViewportLayout> {
			return MakeShareable(new T);
		});
	}

	FCustomViewportLayoutDefinition(TFunction<TSharedRef<FLevelViewportLayout>()> InFactoryFunction)
		: FactoryFunction(MoveTemp(InFactoryFunction))
	{}

	/** User provided display name of the layout */
	FText DisplayName;

	/** User provided description of the layout */
	FText Description;

	/** Icon used to represent this viewport layout */
	FSlateIcon Icon;

	/** Function used to create a new instance of the viewport layout */
	TFunction<TSharedRef<FLevelViewportLayout>()> FactoryFunction;
};
