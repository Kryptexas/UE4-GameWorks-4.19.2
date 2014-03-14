// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoundCue.generated.h"

struct FActiveSound;
struct FSoundParseParameters;

USTRUCT()
struct FSoundNodeEditorData
{
	GENERATED_USTRUCT_BODY()

	int32 NodePosX;

	int32 NodePosY;


	FSoundNodeEditorData()
		: NodePosX(0)
		, NodePosY(0)
	{
	}


	friend FArchive& operator<<(FArchive& Ar,FSoundNodeEditorData& MySoundNodeEditorData)
	{
		return Ar << MySoundNodeEditorData.NodePosX << MySoundNodeEditorData.NodePosY;
	}
};

/**
 * The behavior of audio playback is defined within Sound Cues.
 */
UCLASS(dependson=USoundClass, hidecategories=object, MinimalAPI, BlueprintType)
class USoundCue : public USoundBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Attenuation)
	uint32 bOverrideAttenuation:1;

	UPROPERTY()
	class USoundNode* FirstNode;

	UPROPERTY(EditAnywhere, Category=SoundCue, AssetRegistrySearchable)
	float VolumeMultiplier;

	UPROPERTY(EditAnywhere, Category=SoundCue, AssetRegistrySearchable)
	float PitchMultiplier;

	UPROPERTY(EditAnywhere, Category=Attenuation)
	FAttenuationSettings AttenuationOverrides;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TArray<USoundNode*> AllNodes;

	/** EdGraph based representation of the SoundCue */
	class USoundCueGraph* SoundCueGraph;

	TMap<USoundNode*,FSoundNodeEditorData> EditorData_DEPRECATED;
#endif

private:
	float MaxAudibleDistance;


public:

	// Begin UObject interface.
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	virtual FString GetDesc() OVERRIDE;
#if WITH_EDITOR
	virtual void PostInitProperties() OVERRIDE;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void Serialize( FArchive& Ar ) OVERRIDE;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	virtual void PostLoad() OVERRIDE;
#endif
	// End UObject interface.

	// Begin USoundBase interface.
	virtual bool IsPlayable() const OVERRIDE;
	virtual void Parse( class FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual float GetVolumeMultiplier() OVERRIDE;
	virtual float GetPitchMultiplier() OVERRIDE;
	virtual float GetMaxAudibleDistance() OVERRIDE;
	virtual float GetDuration() OVERRIDE;
	virtual const FAttenuationSettings* GetAttenuationSettingsToApply() const OVERRIDE;
	// End USoundBase interface.

	/** Construct and initialize a node within this Cue */
	template<class T>
	T* ConstructSoundNode(TSubclassOf<USoundNode> SoundNodeClass = T::StaticClass(), bool bSelectNewNode = true)
	{
		T* SoundNode = ConstructObject<T>(SoundNodeClass, this);
#if WITH_EDITORONLY_DATA
		AllNodes.Add(SoundNode);
		SetupSoundNode(SoundNode, bSelectNewNode);
#endif // WITH_EDITORONLY_DATA
		return SoundNode;
	}

	/**
	*	@param		Format		Format to check
	 *
	 *	@return		Sum of the size of waves referenced by this cue for the given platform.
	 */
	virtual int32 GetResourceSizeForFormat(FName Format);

	/**
	 * Recursively finds all Nodes in the Tree
	 */
	ENGINE_API void RecursiveFindAllNodes( USoundNode* Node, TArray<class USoundNode*>& OutNodes );

	/**
	 * Recursively finds sound nodes of type T
	 */
	template<typename T> ENGINE_API void RecursiveFindNode( USoundNode* Node, TArray<T*>& OutNodes );

	/** Find the path through the sound cue to a node identified by its hash */
	ENGINE_API bool FindPathToNode(const UPTRINT NodeHashToFind, TArray<USoundNode*>& OutPath) const;

protected:
	bool RecursiveFindPathToNode(USoundNode* CurrentNode, const UPTRINT CurrentHash, const UPTRINT NodeHashToFind, TArray<USoundNode*>& OutPath) const;

public:

	/**
	 * Instantiate certain functions to work around a linker issue
	 */
	ENGINE_API void RecursiveFindAttenuation( USoundNode* Node, TArray<class USoundNodeAttenuation*> &OutNodes );

#if WITH_EDITORONLY_DATA
	/** Create the basic sound graph */
	ENGINE_API void CreateGraph();

	/** Clears all nodes from the graph (for old editor's buffer soundcue) */
	ENGINE_API void ClearGraph();

	/** Set up EdGraph parts of a SoundNode */
	ENGINE_API void SetupSoundNode(class USoundNode* InSoundNode, bool bSelectNewNode = true);

	/** Use the SoundCue's children to link EdGraph Nodes together */
	ENGINE_API void LinkGraphNodesFromSoundNodes();

	/** Use the EdGraph representation to compile the SoundCue */
	ENGINE_API void CompileSoundNodesFromGraphNodes();

	/** Get the EdGraph of SoundNodes */
	USoundCueGraph* GetGraph()
	{
		return SoundCueGraph;
	}
#endif
};



