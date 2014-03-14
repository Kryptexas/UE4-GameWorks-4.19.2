// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineKismetLibraryClasses.h"
#include "SoundDefinitions.h"

UBTTask_PlaySound::UBTTask_PlaySound(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "PlaySound";
}

EBTNodeResult::Type UBTTask_PlaySound::ExecuteTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	const AAIController* MyController = OwnerComp ? Cast<AAIController>(OwnerComp->GetOwner()) : NULL;

	UAudioComponent* AC = NULL;
	if (SoundToPlay && MyController)
	{
		if (const APawn* MyPawn = MyController->GetPawn())
		{
			AC = UGameplayStatics::PlaySoundAttached(SoundToPlay, MyPawn->GetRootComponent());
		}
	}
	return AC ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

FString UBTTask_PlaySound::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: '%s'"), *Super::GetStaticDescription(), SoundToPlay ? *SoundToPlay->GetName() : TEXT(""));
}
