// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineKismetLibraryClasses.h"
#include "SoundDefinitions.h"

UBTTask_MakeNoise::UBTTask_MakeNoise(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
	, Loudnes(1.0f)
{
	NodeName = "MakeNoise";
}

EBTNodeResult::Type UBTTask_MakeNoise::ExecuteTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	const AAIController* MyController = OwnerComp ? Cast<AAIController>(OwnerComp->GetOwner()) : NULL;
	APawn* MyPawn = MyController ? MyController->GetPawn() : NULL;

	if (MyPawn)
	{
		MyPawn->MakeNoise(Loudnes, MyPawn);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}

