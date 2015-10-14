// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieScenePropertyTrack.h"
#include "MovieScenePropertyTrack.h"

#include "MovieSceneVectorTrack.generated.h"

struct MOVIESCENETRACKS_API FVectorKey
{
	FVectorKey() { }
	FVectorKey( const FVector& InValue, FName InCurveName );
	FVectorKey( const FVector2D& InValue, FName InCurveName );
	FVectorKey( const FVector4& InValue, FName InCurveName );

	FVector4 Value;
	uint32 ChannelsUsed;
	FName CurveName;
};

/**
 * Handles manipulation of component transforms in a movie scene
 */
UCLASS(MinimalAPI)
class UMovieSceneVectorTrack : public UMovieScenePropertyTrack
{
	GENERATED_UCLASS_BODY()
public:
	/** UMovieSceneTrack interface */
	virtual UMovieSceneSection* CreateNewSection() override;
	virtual TSharedPtr<IMovieSceneTrackInstance> CreateInstance() override;

	/**
	 * Adds a key to a section.  Will create the section if it doesn't exist
	 *
	 * @param Time				The time relative to the owning movie scene where the section should be
	 * @param Key				The vector key to add
	 * @param KeyParams         The keying parameters 
	 * @return True if the key was successfully added.
	 */
	virtual bool AddKeyToSection( float Time, const FVectorKey& Key, FKeyParams KeyParams );
	
	/**
	 * Evaluates the track at the playback position
	 *
	 * @param Position	The current playback position
	 * @param LastPosition	The last plackback position
	 * @param OutVector 	The value at the playback position
	 * @return true if anything was evaluated. Note: if false is returned OutVector remains unchanged
	 */
	virtual bool Eval( float Position, float LastPostion, FVector4& InOutVector ) const;

	/**
	 * Get whether the track can be keyed at a particular time.
	 *
	 * @param Time				The time relative to the owning movie scene where the section should be
	 * @param Value				The value of the key
	 * @param KeyParams         The keying parameters
	 * @return Whether the track can be keyed
	 */
	virtual bool CanKeyTrack( float Time, const FVectorKey& Key, FKeyParams KeyParams ) const;

	/** @return Get the number of channels used by the vector */
	int32 GetNumChannelsUsed() const { return NumChannelsUsed; }

private:
	virtual bool AddKeyToSection( float Time, const FVector4& InKey, int32 InChannelsUsed, FName CurveName, FKeyParams KeyParams );
private:
	/** The number of channels used by the vector (2,3, or 4) */
	UPROPERTY()
	int32 NumChannelsUsed;
};
