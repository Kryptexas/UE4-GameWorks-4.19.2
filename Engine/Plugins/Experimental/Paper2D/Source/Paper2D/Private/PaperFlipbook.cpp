// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperFlipbook.h"
#include "PaperCustomVersion.h"

//////////////////////////////////////////////////////////////////////////
// UPaperFlipbook

UPaperFlipbook::UPaperFlipbook(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	FramesPerSecond = 15;
}

int32 UPaperFlipbook::GetNumFrames() const
{
	int32 SumFrames = 0;
	for (int32 KeyFrameIndex = 0; KeyFrameIndex < KeyFrames.Num(); ++KeyFrameIndex)
	{
		SumFrames += KeyFrames[KeyFrameIndex].FrameRun;
	}

	return SumFrames;
}

float UPaperFlipbook::GetDuration() const
{
	if ( FramesPerSecond != 0 )
	{
		return GetNumFrames() / FramesPerSecond;
	}

	return 0;
}

UPaperSprite* UPaperFlipbook::GetSpriteAtTime(float Time) const
{
	if ( FramesPerSecond != 0 )
	{
		float SumTime = 0.0f;

		for (int32 KeyFrameIndex = 0; KeyFrameIndex < KeyFrames.Num(); ++KeyFrameIndex)
		{
			SumTime += KeyFrames[KeyFrameIndex].FrameRun / FramesPerSecond;

			if (Time <= SumTime)
			{
				return KeyFrames[KeyFrameIndex].Sprite;
			}
		}
	}

	return NULL;
}

UPaperSprite* UPaperFlipbook::GetSpriteAtFrame(int32 FrameIndex) const
{
	return KeyFrames.IsValidIndex(FrameIndex) ? KeyFrames[FrameIndex].Sprite : nullptr;
}

#if WITH_EDITORONLY_DATA
void UPaperFlipbook::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FPaperCustomVersion::GUID);
}

void UPaperFlipbook::PostLoad()
{
	Super::PostLoad();

	const int32 PaperVer = GetLinkerCustomVersion(FPaperCustomVersion::GUID);
	if (PaperVer < FPaperCustomVersion::AddTransactionalToClasses)
	{
		SetFlags(RF_Transactional);
	}
}
#endif