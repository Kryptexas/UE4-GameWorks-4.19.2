// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FPaperStyle : public FEditorStyle
{
public:
	static void Initialize();

	static void Shutdown();

private:
	static TSharedRef<class FSlateStyleSet> Create();

private:
	static TSharedPtr<class FSlateStyleSet> PaperStyleInstance;

private:
	FPaperStyle() {}
};
