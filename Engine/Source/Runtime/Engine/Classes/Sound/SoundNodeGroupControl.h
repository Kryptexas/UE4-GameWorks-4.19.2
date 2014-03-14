// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SoundNodeGroupControl.generated.h"

/** 
 * Plays different sounds depending on the number and proximity of the sounds to the listener
 */
UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, meta=( DisplayName="Group Control" ))
class USoundNodeGroupControl : public USoundNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, editfixedsize, Category=GroupControl)
	TArray<int32> GroupSizes;

public:
	// Begin USoundNode interface.
	virtual bool NotifyWaveInstanceFinished( FWaveInstance* WaveInstance );
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual int32 GetMaxChildNodes() const OVERRIDE 
	{ 
		return MAX_ALLOWED_CHILD_NODES; 
	}
	virtual void InsertChildNode( int32 Index ) OVERRIDE;
	virtual void RemoveChildNode( int32 Index ) OVERRIDE;
#if WITH_EDITOR
	/** Ensure amount of Groups matches new amount of children */
	virtual void SetChildNodes(TArray<USoundNode*>& InChildNodes) OVERRIDE;
#endif //WITH_EDITOR
	virtual void CreateStartingConnectors() OVERRIDE;
	virtual FString GetUniqueString() const OVERRIDE;
	// End USoundNode interface.

private:
	// Ensure the child count and group sizes are the same counts
	void FixGroupSizesArray();

	static TMap< USoundNodeGroupControl*, TArray< TMap< FActiveSound*, int32 > > > GroupControlSlotUsageMap;
};
