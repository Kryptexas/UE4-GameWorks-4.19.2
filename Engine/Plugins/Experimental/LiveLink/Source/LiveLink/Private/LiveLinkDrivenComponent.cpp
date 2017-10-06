// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkDrivenComponent.h"

#include "ILiveLinkClient.h"
#include "LiveLinkTypes.h"

#include "GameFramework/Actor.h"
#include "Features/IModularFeatures.h"

FLiveLinkClientReference::FLiveLinkClientReference()
	: LiveLinkClient(nullptr)
{
	IModularFeatures& ModularFeatures = IModularFeatures::Get();

	ModularFeatures.OnModularFeatureRegistered().AddRaw(this, &FLiveLinkClientReference::OnLiveLinkClientRegistered);
	ModularFeatures.OnModularFeatureUnregistered().AddRaw(this, &FLiveLinkClientReference::OnLiveLinkClientUnregistered);

	InitClient();
}

FLiveLinkClientReference::~FLiveLinkClientReference()
{
	IModularFeatures& ModularFeatures = IModularFeatures::Get();

	ModularFeatures.OnModularFeatureRegistered().RemoveAll(this);
	ModularFeatures.OnModularFeatureUnregistered().RemoveAll(this);
}

void FLiveLinkClientReference::InitClient()
{
	IModularFeatures& ModularFeatures = IModularFeatures::Get();

	if (ModularFeatures.IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		LiveLinkClient = &IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
	}
}

void FLiveLinkClientReference::OnLiveLinkClientRegistered(const FName& Type, class IModularFeature* ModularFeature)
{
	if (Type == ILiveLinkClient::ModularFeatureName && !LiveLinkClient)
	{
		LiveLinkClient = static_cast<ILiveLinkClient*>(ModularFeature);
	}
}

void FLiveLinkClientReference::OnLiveLinkClientUnregistered(const FName& Type, class IModularFeature* ModularFeature)
{
	if (Type == ILiveLinkClient::ModularFeatureName && ModularFeature == LiveLinkClient)
	{
		LiveLinkClient = nullptr;
		InitClient();
	}
}

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