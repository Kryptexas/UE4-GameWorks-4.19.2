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

/**
 * Contains an animation sequence of sprite frames
 */
UCLASS(MinimalAPI, BlueprintType, meta = (DisplayThumbnail = "true"))
class UPaperFlipbook : public UObject
{
	GENERATED_UCLASS_BODY()

	// The nominal frame rate to play this flipbook animation back at
	UPROPERTY(Category=Sprite, EditAnywhere)
	float FramesPerSecond;

	// The set of key frames for this flipbook animation (each one has a duration and a sprite to display)
	UPROPERTY(Category=Sprite, EditAnywhere)
	TArray<FPaperFlipbookKeyFrame> KeyFrames;

	PAPER2D_API int32 GetNumFrames() const;
	PAPER2D_API float GetDuration() const;
	PAPER2D_API UPaperSprite* GetSpriteAtTime(float Time) const;
	PAPER2D_API UPaperSprite* GetSpriteAtFrame(int32 FrameIndex) const;

	// Returns the render bounds of this sprite
	FBoxSphereBounds GetRenderBounds() const;

	// UObject interface
#if WITH_EDITORONLY_DATA
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
#endif
	// End of UObject interface
};
