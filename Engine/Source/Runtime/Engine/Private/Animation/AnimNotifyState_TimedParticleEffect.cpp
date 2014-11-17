// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UAnimNotifyState_TimedParticleEffect::UAnimNotifyState_TimedParticleEffect(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	PSTemplate = nullptr;
	LocationOffset.Set(0.0f, 0.0f, 0.0f);
	RotationOffset = FRotator(0.0f, 0.0f, 0.0f);
}

void UAnimNotifyState_TimedParticleEffect::NotifyBegin(USkeletalMeshComponent * MeshComp, class UAnimSequence * AnimSeq)
{
	// Only spawn if we've got valid params
	if(ValidateParameters(MeshComp))
	{
		UParticleSystemComponent* NewComponent = UGameplayStatics::SpawnEmitterAttached(PSTemplate, MeshComp, SocketName, LocationOffset, RotationOffset);
	}

	Received_NotifyBegin(MeshComp, AnimSeq);
}

void UAnimNotifyState_TimedParticleEffect::NotifyTick(USkeletalMeshComponent * MeshComp, class UAnimSequence * AnimSeq, float FrameDeltaTime)
{
	Received_NotifyTick(MeshComp, AnimSeq, FrameDeltaTime);
}

void UAnimNotifyState_TimedParticleEffect::NotifyEnd(USkeletalMeshComponent * MeshComp, class UAnimSequence * AnimSeq)
{
	TArray<USceneComponent*> Children;
	MeshComp->GetChildrenComponents(false, Children);

	for(USceneComponent* Component : Children)
	{
		if(UParticleSystemComponent* ParticleComponent = Cast<UParticleSystemComponent>(Component))
		{
			bool bSocketMatch = ParticleComponent->AttachSocketName == SocketName;
			bool bTemplateMatch = ParticleComponent->Template == PSTemplate;

#if WITH_EDITORONLY_DATA
			// In editor someone might have changed our parameters while we're ticking; so check 
			// previous known parameters too.
			bSocketMatch |= PreviousSocketNames.Contains(ParticleComponent->AttachSocketName);
			bTemplateMatch |= PreviousPSTemplates.Contains(ParticleComponent->Template);
#endif

			if(bSocketMatch && bTemplateMatch)
			{
				// Either destroy the component or deactivate it to have it's active particles finish.
				// The component will auto destroy once all particle are gone.
				if(bDestroyAtEnd)
				{
					ParticleComponent->DestroyComponent();
				}
				else
				{
					ParticleComponent->bAutoDestroy = true;
					ParticleComponent->DeactivateSystem();
				}

#if WITH_EDITORONLY_DATA
				// No longer need to track previous values as we've found our component
				// and removed it.
				PreviousPSTemplates.Empty();
				PreviousSocketNames.Empty();
#endif
				// Removed a component, no need to continue
				break;
			}
		}
	}

	Received_NotifyEnd(MeshComp, AnimSeq);
}

bool UAnimNotifyState_TimedParticleEffect::ValidateParameters(USkeletalMeshComponent* MeshComp)
{
	bool bValid = true;

	if(!PSTemplate)
	{
		bValid = false;
	}
	else if(!MeshComp->DoesSocketExist(SocketName) && MeshComp->GetBoneIndex(SocketName) == INDEX_NONE)
	{
		bValid = false;
	}

	return bValid;
}

#if WITH_EDITORONLY_DATA
void UAnimNotifyState_TimedParticleEffect::PreEditChange(UProperty* PropertyAboutToChange)
{
	if(PropertyAboutToChange->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UAnimNotifyState_TimedParticleEffect, PSTemplate) && PSTemplate != NULL)
	{
		PreviousPSTemplates.Add(PSTemplate);
	}

	if(PropertyAboutToChange->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UAnimNotifyState_TimedParticleEffect, SocketName) && SocketName != FName(TEXT("None")))
	{
		PreviousSocketNames.Add(SocketName);
	}
}
#endif
