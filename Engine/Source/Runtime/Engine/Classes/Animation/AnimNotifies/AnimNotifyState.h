// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "AnimNotifyState.generated.h"

class USkeletalMeshComponent;
class UAnimSequence;
class UAnimSequenceBase;
struct FAnimNotifyEvent;

UCLASS(abstract, editinlinenew, Blueprintable, const, hidecategories=Object, collapsecategories, meta=(ShowHiddenSelfPins))
class ENGINE_API UAnimNotifyState : public UObject
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintImplementableEvent)
	virtual bool Received_NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation) const;
	
	UFUNCTION(BlueprintImplementableEvent)
	virtual bool Received_NotifyTick(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float FrameDeltaTime) const;

	UFUNCTION(BlueprintImplementableEvent)
	virtual bool Received_NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation) const;

#if WITH_EDITORONLY_DATA
	/** Color of Notify in editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AnimNotify)
	FColor NotifyColor;

#endif // WITH_EDITORONLY_DATA

	virtual void NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation);
	virtual void NotifyTick(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float FrameDeltaTime);
	virtual void NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation);

	// @todo document 
	virtual FString GetEditorComment() 
	{ 
		return TEXT(""); 
	}

	// @todo document 
	virtual FLinearColor GetEditorColor() 
	{ 
#if WITH_EDITORONLY_DATA
		return FLinearColor(NotifyColor); 
#else
		return FLinearColor::Black;
#endif // WITH_EDITORONLY_DATA
	}

	/**
	 *	Called by the AnimSet viewer when the 'parent' FAnimNotifyEvent is edited.
	 *
	 *	@param	AnimSeq			The animation sequence this notify is associated with.
	 *	@param	OwnerEvent		The event that 'owns' this AnimNotify.
	 */
	virtual void AnimNotifyEventChanged(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, FAnimNotifyEvent * OwnerEvent) {}
};



