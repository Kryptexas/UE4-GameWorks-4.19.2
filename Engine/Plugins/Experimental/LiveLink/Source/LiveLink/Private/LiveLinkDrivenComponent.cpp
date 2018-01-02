// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkDrivenComponent.h"

#include "ILiveLinkClient.h"
#include "LiveLinkTypes.h"

#include "GameFramework/Actor.h"

ULiveLinkDrivenComponent::ULiveLinkDrivenComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;

	bAutoActivate = true;
	
}

void ULiveLinkDrivenComponent::OnRegister()
{
	Super::OnRegister();
}

void ULiveLinkDrivenComponent::OnUnregister()
{
	Super::OnUnregister();
}

void ULiveLinkDrivenComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if (ILiveLinkClient* Client = ClientRef.GetClient())
	{
		if (const FLiveLinkSubjectFrame* Frame = Client->GetSubjectData(SubjectName))
		{
			if (bModifyActorTransform)
			{
				int32 ActorTransformIndex = 0;
				if (ActorTransformBone != NAME_None)
				{
					const int32 NewActorTransformBoneIndex = Frame->RefSkeleton.GetBoneNames().IndexOfByKey(ActorTransformBone);
					if (NewActorTransformBoneIndex != INDEX_NONE)
					{
						ActorTransformIndex = NewActorTransformBoneIndex;
					}
				}

				if (Frame->Transforms.IsValidIndex(ActorTransformIndex))
				{
					const FTransform& NewTransform = Frame->Transforms[ActorTransformIndex];
					if (AActor* Actor = GetOwner())
					{
						if (bSetRelativeLocation)
						{
							Actor->SetActorRelativeTransform(NewTransform);
						}
						else
						{
							Actor->SetActorTransform(NewTransform);
						}
					}
				}
			}
		}
	}
}