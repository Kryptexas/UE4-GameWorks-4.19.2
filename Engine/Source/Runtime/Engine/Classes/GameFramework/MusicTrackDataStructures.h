// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "MusicTrackDataStructures.generated.h"

USTRUCT()
struct FMusicTrackStruct
{
	GENERATED_USTRUCT_BODY()

	/** The soundCue to play **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MusicTrackStruct)
	class USoundBase* Sound;

	/** Controls whether or not the track is auto-played when it is attached to the scene. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MusicTrackStruct)
	uint32 bAutoPlay:1;

	/** Controls whether the sound is not stopped on a map change */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MusicTrackStruct)
	uint32 bPersistentAcrossLevels:1;

	/** Time taken for sound to fade in when action is activated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MusicTrackStruct)
	float FadeInTime;

	/** Volume the sound to should fade in to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MusicTrackStruct)
	float FadeInVolumeLevel;

	/** Time take for sound to fade out when Stop input is fired. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MusicTrackStruct)
	float FadeOutTime;

	/** Volume the sound to should fade out to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MusicTrackStruct)
	float FadeOutVolumeLevel;

	/** Alternate Mp3 file to play on devices (iPhone, not PC) that support it.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Mobile)
	FString MP3Filename;



		FMusicTrackStruct()
		: Sound(NULL)
		, bAutoPlay(false)
		, bPersistentAcrossLevels(false)
		, FadeInTime(5.0f)
		, FadeInVolumeLevel(1.0f)
		, FadeOutTime(5.0f)
		, FadeOutVolumeLevel(0.0f)
		{
		}
	
};

UCLASS()
class UMusicTrackDataStructures : public UObject
{
	GENERATED_UCLASS_BODY()

};

