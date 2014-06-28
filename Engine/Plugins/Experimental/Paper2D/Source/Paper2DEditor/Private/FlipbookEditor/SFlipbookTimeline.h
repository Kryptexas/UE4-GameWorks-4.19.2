// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FSpriteAnimationFrame;

class SFlipbookTimeline : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlipbookTimeline)
		: _FlipbookBeingEdited(nullptr)
		, _PlayTime(0)
	{}

		SLATE_ATTRIBUTE(class UPaperFlipbook*, FlipbookBeingEdited)
		SLATE_ATTRIBUTE(float, PlayTime)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TAttribute<class UPaperFlipbook*> FlipbookBeingEdited;
	TAttribute<float> PlayTime;
};
