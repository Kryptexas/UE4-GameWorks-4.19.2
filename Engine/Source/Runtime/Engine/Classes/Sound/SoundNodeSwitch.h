// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SoundNodeSwitch.generated.h"

/** 
 * Selects a child node based on the value of a integer parameter
 */
UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, meta=(DisplayName="Switch"))
class USoundNodeSwitch : public USoundNode
{
	GENERATED_UCLASS_BODY()

	/** The name of the integer parameter to use to determine which branch we should take */
	UPROPERTY(EditAnywhere, Category=Switch)
	FName IntParameterName;

public:
	// Begin USoundNode interface.
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual int32 GetMaxChildNodes( void ) const OVERRIDE 
	{ 
		return MAX_ALLOWED_CHILD_NODES; 
	}
	virtual int32 GetMinChildNodes() const OVERRIDE
	{ 
		return 1;
	}
	virtual void CreateStartingConnectors( void ) OVERRIDE;
	
#if WITH_EDITOR
	void RenamePins();

	virtual void InsertChildNode( int32 Index ) OVERRIDE
	{
		Super::InsertChildNode(Index);

		RenamePins();
	}

	virtual void RemoveChildNode( int32 Index ) OVERRIDE
	{
		Super::RemoveChildNode(Index);
		
		RenamePins();
	}

	virtual FString GetInputPinName(int32 PinIndex) const OVERRIDE;
	virtual FString GetTitle() const OVERRIDE;
#endif //WITH_EDITOR
	// End USoundNode interface.
};



