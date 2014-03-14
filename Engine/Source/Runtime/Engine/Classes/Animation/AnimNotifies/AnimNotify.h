// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "AnimNotify.generated.h"

UCLASS(abstract, Blueprintable, const, hidecategories=Object, collapsecategories, MinimalAPI)
class UAnimNotify : public UObject
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintImplementableEvent)
	virtual bool Received_Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequence* AnimSeq) const;

#if WITH_EDITORONLY_DATA
	/** Color of Notify in editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AnimNotify)
	FColor NotifyColor;

#endif // WITH_EDITORONLY_DATA

	virtual void Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequence* AnimSeq);

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
	virtual void AnimNotifyEventChanged(class USkeletalMeshComponent* MeshComp, class UAnimSequence* AnimSeq, FAnimNotifyEvent* OwnerEvent) {}

	/**
	 * We don't instance UAnimNotify objects along with the animations they belong to, but
	 * we still need a way to see which world this UAnimNotify is currently operating on.
	 * So this retrieves a contextual world pointer, from the triggering animation/mesh.  
	 * 
	 * @return NULL if this isn't in the middle of a Received_Notify(), otherwise it's the world belonging to the Mesh passed to Received_Notify()
	 */
	virtual class UWorld* GetWorld() const OVERRIDE;

private:
	/* The mesh we're currently triggering a UAnimNotify for (so we can retrieve per instance information) */
	class USkeletalMeshComponent* MeshContext;
};



