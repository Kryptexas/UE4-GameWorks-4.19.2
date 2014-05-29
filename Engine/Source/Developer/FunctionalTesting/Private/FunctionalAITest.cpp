// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "FunctionalTestingPrivatePCH.h"
#include "ObjectEditorUtils.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "BehaviorTree/BehaviorTree.h"
#include "AIController.h"

AFunctionalAITest::AFunctionalAITest( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, CurrentSpawnSetIndex(INDEX_NONE)
{
	/*struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> Texture;

		FConstructorStatics()
			: Texture(TEXT("/Engine/EditorResources/S_FTest"))
		{
		}
	};*/
}

bool AFunctionalAITest::IsOneOfSpawnedPawns(AActor* Actor)
{
	APawn* Pawn = Cast<APawn>(Actor);
	return Pawn != NULL && SpawnedPawns.Contains(Pawn);
}

void AFunctionalAITest::BeginPlay()
{
	// do a post-load step and remove all disabled spawn sets
	for(int32 Index = SpawnSets.Num()-1; Index >= 0; --Index)
	{
		FAITestSpawnSet& SpawnSet = SpawnSets[Index];
		if (SpawnSet.bEnabled == false)
		{
			UE_LOG(LogFunctionalTest, Log, TEXT("Removing disabled spawn set \'%s\'."), *SpawnSets[Index].Name.ToString());
			SpawnSets.RemoveAt(Index, 1, false);
		}
		else if (SpawnSet.FallbackSpawnLocation != NULL)
		{
			// update all spawn info that doesn't have spawn location set
			for (int32 SpawnIndex = 0; SpawnIndex < SpawnSet.SpawnInfoContainer.Num(); ++SpawnIndex)
			{
				FAITestSpawnInfo& SpawnInfo = SpawnSet.SpawnInfoContainer[SpawnIndex];
				if (SpawnInfo.SpawnLocation == NULL)
				{
					SpawnInfo.SpawnLocation = SpawnSet.FallbackSpawnLocation;
				}
			}
		}
	}
	SpawnSets.Shrink();

	Super::BeginPlay();
}

bool AFunctionalAITest::StartTest()
{
	KillOffSpawnedPawns();

	if (++CurrentSpawnSetIndex < SpawnSets.Num())
	{
		UWorld* World = GetWorld();
		check(World);
		const FAITestSpawnSet& SpawnSet = SpawnSets[CurrentSpawnSetIndex];

		bool bSuccessfullySpawnedAll = true;

		// NOTE: even if some pawns fail to spawn we don't stop spawning to find all spawns that will fails.
		// all spawned pawns get filled off in case of failure.
		CurrentSpawnSetName = SpawnSet.Name.ToString();

		for (int32 SpawnIndex = 0; SpawnIndex < SpawnSet.SpawnInfoContainer.Num(); ++SpawnIndex)
		{
			const FAITestSpawnInfo& SpawnInfo = SpawnSet.SpawnInfoContainer[SpawnIndex];
			if (SpawnInfo.IsValid())
			{
				APawn* SpawnedPawn = UAIBlueprintHelperLibrary::SpawnAIFromClass(World, SpawnInfo.PawnClass, SpawnInfo.BehaviorTree
					, SpawnInfo.SpawnLocation->GetActorLocation()
					, SpawnInfo.SpawnLocation->GetActorRotation()
					, /*bNoCollisionFail=*/true);

				if (SpawnedPawn == NULL)
				{
					FailureMessage = FString::Printf(TEXT("Failed to spawn \'%s\' pawn (\'%s\' set) ")
						, *GetNameSafe(SpawnInfo.PawnClass)
						, *SpawnSet.Name.ToString());

					UE_LOG(LogFunctionalTest, Warning, TEXT("%s"), *FailureMessage);

					bSuccessfullySpawnedAll = false;
				}
				else if (SpawnedPawn->GetController() == NULL)
				{
					FailureMessage = FString::Printf(TEXT("Spawned Pawn %s (\'%s\' set) has no controller ")
						, *GetNameSafe(SpawnedPawn)
						, *SpawnSet.Name.ToString());

					UE_LOG(LogFunctionalTest, Warning, TEXT("%s"), *FailureMessage);

					bSuccessfullySpawnedAll = false;
				}
				else
				{
					SpawnedPawns.Add(SpawnedPawn);
					OnAISpawned.Broadcast(Cast<AAIController>(SpawnedPawn->GetController()), SpawnedPawn);
				}
			}
			else
			{
				FailureMessage = FString::Printf(TEXT("Spawn set \'%s\' contains invalid entry at index %d")
					, *SpawnSet.Name.ToString()
					, SpawnIndex);

				UE_LOG(LogFunctionalTest, Warning, TEXT("%s"), *FailureMessage);

				bSuccessfullySpawnedAll = false;
			}
		}

		if (bSuccessfullySpawnedAll == false)
		{
			KillOffSpawnedPawns();
		}
		else
		{
			return Super::StartTest();
		}
	}

	return false;
}

void AFunctionalAITest::FinishTest(TEnumAsByte<EFunctionalTestResult::Type> TestResult, const FString& Message)
{
	Super::FinishTest(TestResult, Message);
}

bool AFunctionalAITest::WantsToRunAgain() const
{
	return CurrentSpawnSetIndex + 1 < SpawnSets.Num();
}


void AFunctionalAITest::CleanUp()
{
	Super::CleanUp();
	CurrentSpawnSetIndex = INDEX_NONE;

	KillOffSpawnedPawns();
}

FString AFunctionalAITest::GetAdditionalTestFinishedMessage(EFunctionalTestResult::Type TestResult) const
{
	FString Result;

	if (SpawnedPawns.Num() > 0)
	{
		if (CurrentSpawnSetName.Len() > 0 && CurrentSpawnSetName != TEXT("None"))
		{
			Result = FString::Printf(TEXT("set %s, pawns: "), *CurrentSpawnSetName);
		}
		else
		{
			Result = TEXT("pawns: ");
		}
		

		for (int32 PawnIndex = 0; PawnIndex < SpawnedPawns.Num(); ++PawnIndex)
		{
			Result += FString::Printf(TEXT("%s, "), *GetNameSafe(SpawnedPawns[PawnIndex]));
		}
	}

	return Result;
}

void AFunctionalAITest::KillOffSpawnedPawns()
{
	for (int32 PawnIndex = 0; PawnIndex < SpawnedPawns.Num(); ++PawnIndex)
	{
		if (SpawnedPawns[PawnIndex])
		{
			SpawnedPawns[PawnIndex]->Destroy();
		}
	}

	SpawnedPawns.Reset();
}
