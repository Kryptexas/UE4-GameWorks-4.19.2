// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

/////////////////////////////////////////////////////
// UAnimNotify

UAnimNotify::UAnimNotify(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, MeshContext(NULL)
{

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(255, 200, 200, 255);
#endif // WITH_EDITORONLY_DATA
}


void UAnimNotify::Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequence* AnimSeq)
{
	USkeletalMeshComponent* PrevContext = MeshContext;
	MeshContext = MeshComp;
	Received_Notify(MeshComp, AnimSeq);
	MeshContext = PrevContext;
}

class UWorld* UAnimNotify::GetWorld() const
{
	return (MeshContext ? MeshContext->GetWorld() : NULL);
}
