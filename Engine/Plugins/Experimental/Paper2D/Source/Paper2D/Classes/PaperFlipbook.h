// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperSprite.h"
#include "PaperFlipbook.generated.h"

USTRUCT()
struct FPaperFlipbookKeyFrame
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category=Sprite, EditAnywhere)
	UPaperSprite* Sprite;

	UPROPERTY(Category=Sprite, EditAnywhere)
	int32 FrameRun;

	FPaperFlipbookKeyFrame()
		: Sprite(NULL)
		, FrameRun(1)
	{
	}
};

UCLASS(MinimalAPI, BlueprintType, meta=(DisplayThumbnail = "true"))
class UPaperFlipbook : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=Sprite, EditAnywhere)
	float FramesPerSecond;

	UPROPERTY(Category=Sprite, EditAnywhere)
	TArray<FPaperFlipbookKeyFrame> KeyFrames;

	PAPER2D_API int32 GetNumFrames() const;
	PAPER2D_API float GetDuration() const;
	PAPER2D_API UPaperSprite* GetSpriteAtTime(float Time) const;
};
