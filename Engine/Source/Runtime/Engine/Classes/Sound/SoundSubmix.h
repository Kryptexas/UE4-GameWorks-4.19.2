// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "SoundEffectSubmix.h"
#include "IAmbisonicsMixer.h"
#include "SoundSubmix.generated.h"

class UEdGraph;
class USoundEffectSubmixPreset;
class USoundSubmix;

/* Submix channel format. 
 Allows submixes to have sources mix to a particular channel configuration for potential effect chain requirements.
 Master submix will always render at the device channel count. All child submixes will be down-mixed (or up-mixed) to 
 the device channel count. This feature exists to allow specific submix effects to do their work on multi-channel mixes
 of audio. 
*/
UENUM(BlueprintType)
enum class ESubmixChannelFormat : uint8
{
	// Sets the submix channels to the output device channel count
	Device UMETA(DisplayName = "Device"),

	// Sets the submix mix to stereo (FL, FR)
	Stereo UMETA(DisplayName = "Stereo"),

	// Sets the submix to mix to quad (FL, FR, SL, SR)
	Quad UMETA(DisplayName = "Quad"),

	// Sets the submix to mix 5.1 (FL, FR, FC, LF, SL, SR)
	FiveDotOne UMETA(DisplayName = "5.1"),

	// Sets the submix to mix audio to 7.1 (FL, FR, FC, LF, BL, BR, SL, SR)
	SevenDotOne UMETA(DisplayName = "7.1"),

	// Sets the submix to render audio as an ambisonics bed.
	Ambisonics UMETA(DisplayName = "Ambisonics"),

	Count UMETA(Hidden)
};

// Class used to send audio to submixes from USoundBase
USTRUCT(BlueprintType)
struct ENGINE_API FSoundSubmixSendInfo
{
	GENERATED_USTRUCT_BODY()

	// The amount of audio to send
	UPROPERTY(EditAnywhere, Category = SubmixSend)
	float SendLevel;
	
	// The submix to send the audio to
	UPROPERTY(EditAnywhere, Category = SubmixSend)
	USoundSubmix* SoundSubmix;
};

#if WITH_EDITOR

/** Interface for sound submix graph interaction with the AudioEditor module. */
class ISoundSubmixAudioEditor
{
public:
	virtual ~ISoundSubmixAudioEditor() {}

	/** Refreshes the sound class graph links. */
	virtual void RefreshGraphLinks(UEdGraph* SoundClassGraph) = 0;
};
#endif

UCLASS(config=Engine, hidecategories=Object, editinlinenew, BlueprintType)
class ENGINE_API USoundSubmix : public UObject
{
	GENERATED_UCLASS_BODY()

	// Child submixes to this sound mix
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SoundSubmix)
	TArray<USoundSubmix*> ChildSubmixes;

	UPROPERTY()
	USoundSubmix* ParentSubmix;

#if WITH_EDITORONLY_DATA
	/** EdGraph based representation of the SoundSubmix */
	class UEdGraph* SoundSubmixGraph;
#endif

	// Experimental! Specifies the channel format for the submix. Sources will be mixed at the specified format. Useful for specific effects that need to operate on a specific format.
	UPROPERTY()
	ESubmixChannelFormat ChannelFormat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SoundSubmix)
	TArray<USoundEffectSubmixPreset*> SubmixEffectChain;

	// TODO: Hide this unless Channel Format is ambisonics. Also, worry about thread safety.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SoundSubmix)
	UAmbisonicsSubmixSettingsBase* AmbisonicsPluginSettings;

protected:

	//~ Begin UObject Interface.
	virtual FString GetDesc() override;
	virtual void BeginDestroy() override;
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface.

public:

	// Sound Submix Editor functionality
#if WITH_EDITOR

	/**
	* @return true if the child sound class exists in the tree
	*/
	bool RecurseCheckChild(USoundSubmix* ChildSoundSubmix);

	/**
	* Set the parent submix of this SoundSubmix, removing it as a child from its previous owner
	*
	* @param	InParentSubmix	The New Parent Submix of this
	*/
	void SetParentSubmix(USoundSubmix* InParentSubmix);

	/**
	* Add Referenced objects
	*
	* @param	InThis SoundSubmix we are adding references from.
	* @param	Collector Reference Collector
	*/
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

	/**
	* Refresh all EdGraph representations of SoundSubmixes
	*
	* @param	bIgnoreThis	Whether to ignore this SoundSubmix if it's already up to date
	*/
	void RefreshAllGraphs(bool bIgnoreThis);

	/** Sets the sound submix graph editor implementation.* */
	static void SetSoundSubmixAudioEditor(TSharedPtr<ISoundSubmixAudioEditor> InSoundSubmixAudioEditor);

	/** Gets the sound submix graph editor implementation. */
	static TSharedPtr<ISoundSubmixAudioEditor> GetSoundSubmixAudioEditor();

private:

	/** Ptr to interface to sound class editor operations. */
	static TSharedPtr<ISoundSubmixAudioEditor> SoundSubmixAudioEditor;

#endif

};

