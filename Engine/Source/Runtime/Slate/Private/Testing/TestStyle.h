// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/**
 * Test slate style
 */
class SLATE_API FTestStyle 
{
public:

	static TSharedRef< class ISlateStyle > Create();

	/** @return the singleton instance. */
	static const ISlateStyle& Get()
	{
		return *( Instance.Get() );
	}

	static void ResetToDefault();

private:

	static void SetStyle( const TSharedRef< class ISlateStyle >& NewStyle );

private:

	/** Singleton instances of this style. */
	static TSharedPtr< class ISlateStyle > Instance;
};
