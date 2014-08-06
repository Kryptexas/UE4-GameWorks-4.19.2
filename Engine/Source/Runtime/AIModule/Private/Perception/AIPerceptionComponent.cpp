// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Perception/AIPerceptionComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Canvas.h"

//----------------------------------------------------------------------//
// FActorPerceptionInfo
//----------------------------------------------------------------------//
void FActorPerceptionInfo::Merge(const FActorPerceptionInfo& Other)
{
	for (int32 Index = 0; Index < ECorePerceptionTypes::MAX; ++Index)
	{
		if (LastSensedStimuli[Index].GetAge() > Other.LastSensedStimuli[Index].GetAge())
		{
			LastSensedStimuli[Index] = Other.LastSensedStimuli[Index];
		}
	}
}

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
const int32 UAIPerceptionComponent::InitialStimuliToProcessArraySize = 10;

UAIPerceptionComponent::UAIPerceptionComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, DominantSense(ECorePerceptionTypes::MAX)
	, PerceptionListenerId(AIPerception::InvalidListenerId)
{
	FMemory::Memzero(MaxActiveAge, sizeof(MaxActiveAge));
}

void UAIPerceptionComponent::RequestStimuliListenerUpdate()
{
	UAIPerceptionSystem* AIPerceptionSys = UAIPerceptionSystem::GetCurrent(GetWorld());
	if (AIPerceptionSys != NULL)
	{
		AIPerceptionSys->UpdateListener(this);
	}
}

void UAIPerceptionComponent::PostInitProperties() 
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		AIOwner = Cast<AAIController>(GetOuter());
	}
}

void UAIPerceptionComponent::BeginDestroy()
{
	UAIPerceptionSystem* AIPerceptionSys = UAIPerceptionSystem::GetCurrent(GetWorld());
	if (AIPerceptionSys != NULL)
	{
		AIPerceptionSys->UnregisterListener(this);
	}

	Super::BeginDestroy();
}

void UAIPerceptionComponent::SetPeripheralVisionAngle(const float NewPeripheralVisionAngle)
{
	PeripheralVisionAngle = NewPeripheralVisionAngle;	
	RequestStimuliListenerUpdate();	
}

void UAIPerceptionComponent::SetHearingRange(const float NewHearingRange)
{
	HearingRange = NewHearingRange;
	RequestStimuliListenerUpdate();	
}

void UAIPerceptionComponent::SetLOSHearingRange(const float NewLOSHearingRange)
{
	LOSHearingRange = NewLOSHearingRange;
	RequestStimuliListenerUpdate();	
}

void UAIPerceptionComponent::SetSightRadius(const float NewSightRadius)
{
	SightRadius = NewSightRadius;
	RequestStimuliListenerUpdate();	
}

void UAIPerceptionComponent::UpdatePerceptionFilter(FAISenseId Channel, bool bNewValue)
{
	const bool bCurrentValue = PerceptionFilter.ShouldRespondToChannel(Channel);
	if (bNewValue != bCurrentValue)
	{
		bNewValue ? PerceptionFilter.AcceptChannel(Channel) : PerceptionFilter.FilterOutChannel(Channel);
		RequestStimuliListenerUpdate();	
	}
}

void UAIPerceptionComponent::GetHostileActors(TArray<const AActor*>& OutActors)
{
	OutActors.Reserve(PerceptualData.Num());
	for (TActorPerceptionContainer::TConstIterator DataIt = GetPerceptualDataConstIterator(); DataIt; ++DataIt)
	{
		if (DataIt->Value.bIsHostile && DataIt->Value.Target.IsValid())
		{
			OutActors.Add(DataIt->Value.Target.Get());
		}
	}
}

const FActorPerceptionInfo* UAIPerceptionComponent::GetFreshestTrace(const FAISenseId Sense) const
{
	// @note will stop on first age 0 stimulus
	float BestAge = FAIStimulus::NeverHappenedAge;
	const FActorPerceptionInfo* Result = NULL;

	for (TActorPerceptionContainer::TConstIterator DataIt = GetPerceptualDataConstIterator(); DataIt; ++DataIt)
	{
		const FActorPerceptionInfo* Info = &DataIt->Value;
		const float Age = Info->LastSensedStimuli[Sense].GetAge();
		if (Age < BestAge && Info->Target.IsValid())
		{
			BestAge = Age;
			Result = Info;
			if (BestAge == 0.f)
			{
				// won't find any younger then this
				break;
			}
		}
	}

	return Result;
}

void UAIPerceptionComponent::SetDominantSense(ECorePerceptionTypes::Type InDominantSense)
{
	if (DominantSense != InDominantSense)
	{
		DominantSense = InDominantSense;
		// update all perceptual info with this info
		for (TActorPerceptionContainer::TIterator DataIt = GetPerceptualDataIterator(); DataIt; ++DataIt)
		{
			DataIt->Value.DominantSense = DominantSense;
		}
	}
}

void UAIPerceptionComponent::SetShouldSee(bool bNewValue)
{
	UpdatePerceptionFilter(ECorePerceptionTypes::Sight, bNewValue);
}

void UAIPerceptionComponent::SetShouldHear(bool bNewValue)
{
	UpdatePerceptionFilter(ECorePerceptionTypes::Hearing, bNewValue);
}

void UAIPerceptionComponent::SetShouldSenseDamage(bool bNewValue)
{
	UpdatePerceptionFilter(ECorePerceptionTypes::Damage, bNewValue);
}

FGenericTeamId UAIPerceptionComponent::GetTeamIdentifier() const
{
	return FGenericTeamId::GetTeamIdentifier(GetOuter());
}

FVector UAIPerceptionComponent::GetActorLocation(const AActor* Actor) const 
{ 
	// not that Actor == NULL is valid
	const FActorPerceptionInfo* ActorInfo = GetActorInfo(Actor);
	return ActorInfo ? ActorInfo->GetLastStimulusLocation() : FAISystem::InvalidLocation;
}

void UAIPerceptionComponent::GetLocationAndDirection(FVector& Location, FVector& Direction) const
{
	AController* OwnerController = Cast<AController>(GetOuter());
	if (OwnerController != NULL)
	{
		const APawn* OwnerPawn = OwnerController->GetPawn();
		if (OwnerPawn != NULL)
		{
			Location = OwnerPawn->GetActorLocation() + FVector(0,0,OwnerPawn->BaseEyeHeight);
			Direction = OwnerPawn->GetActorRotation().Vector();
			return;
		}
	}
	
	const AActor* OwnerActor = Cast<AActor>(GetOuter());
	if (OwnerActor != NULL)
	{
		Location = OwnerActor->GetActorLocation();
		Direction = OwnerActor->GetActorRotation().Vector();
	}
}

const AActor* UAIPerceptionComponent::GetCollisionActor() const
{
	AController* OwnerController = Cast<AController>(GetOuter());
	if (OwnerController != NULL)
	{
		return OwnerController->GetPawn();
	}

	return Cast<AActor>(GetOuter());
}

void UAIPerceptionComponent::RegisterStimulus(AActor* Source, const FAIStimulus& Stimulus)
{
	StimuliToProcess.Add(FStimulusToProcess(Source, Stimulus));
}

void UAIPerceptionComponent::ProcessStimuli()
{
	if(StimuliToProcess.Num() == 0)
	{
		UE_VLOG(GetOwner(), LogAIPerception, Warning, TEXT("UAIPerceptionComponent::ProcessStimuli called without any Stimuli to process"));
		return;
	}
	
	FStimulusToProcess* SourcedStimulus = StimuliToProcess.GetTypedData();
	TArray<AActor*> UpdatedActors;
	UpdatedActors.Reserve(StimuliToProcess.Num());

	for (int32 StimulusIndex = 0; StimulusIndex < StimuliToProcess.Num(); ++StimulusIndex, ++SourcedStimulus)
	{
		FActorPerceptionInfo* PerceptualInfo = PerceptualData.Find(SourcedStimulus->Source);

		if (PerceptualInfo == NULL)
		{
			if (SourcedStimulus->Stimulus.WasSuccessfullySensed() == false)
			{
				// this means it's a failed perception of an actor our owner is not aware of
				// at all so there's no point in creating perceptual data for a failed stimulus
				continue;
			}
			else
			{
				// create an entry
				PerceptualInfo = &PerceptualData.Add(SourcedStimulus->Source, FActorPerceptionInfo(SourcedStimulus->Source));
				// tell it what's our dominant sense
				PerceptualInfo->DominantSense = DominantSense;
				// @todo add a propert, generic test here
				PerceptualInfo->bIsHostile = (FGenericTeamId::GetTeamIdentifier(GetOuter()) != FGenericTeamId::GetTeamIdentifier(SourcedStimulus->Source));
			}
		}

		check(SourcedStimulus->Stimulus.Type < ECorePerceptionTypes::MAX);

		FAIStimulus& StimulusStore = PerceptualInfo->LastSensedStimuli[SourcedStimulus->Stimulus.Type];

		if (SourcedStimulus->Stimulus.WasSuccessfullySensed() || StimulusStore.WasSuccessfullySensed())
		{
			UpdatedActors.AddUnique(SourcedStimulus->Source);
		}

		if (SourcedStimulus->Stimulus.WasSuccessfullySensed())
		{
			// if there are two stimuli at the same moment, prefer the one with higher strength
			if (SourcedStimulus->Stimulus.GetAge() < StimulusStore.GetAge() || StimulusStore.Strength < SourcedStimulus->Stimulus.Strength)
			{
				StimulusStore = SourcedStimulus->Stimulus;
			}
		}
		else
		{
			// @note there some more valid info in SourcedStimulus->Stimulus regarding test that failed
			// may be useful in future
			StimulusStore.MarkNoLongerSensed();
		}
	}

	StimuliToProcess.Reset();

	if (AIOwner != NULL)
	{
		AIOwner->ActorsPerceptionUpdated(UpdatedActors);
	}
}

void UAIPerceptionComponent::AgeStimuli(const float ConstPerceptionAgingRate)
{
	for (TActorPerceptionContainer::TIterator It(PerceptualData); It; ++It)
	{
		FActorPerceptionInfo& ActorPerceptionInfo = It->Value;

		FAIStimulus* Stimulus = ActorPerceptionInfo.LastSensedStimuli;
		for (int32 SenseIndex = 0; SenseIndex < ECorePerceptionTypes::MAX; ++SenseIndex, ++Stimulus)
		{
			Stimulus->AgeStimulus(ConstPerceptionAgingRate);
		}
	}
}

void UAIPerceptionComponent::ForgetActor(class AActor* ActorToForget)
{
	PerceptualData.Remove(ActorToForget);
}

float UAIPerceptionComponent::GetYoungestStimulusAge(const class AActor* Source) const
{
	const FActorPerceptionInfo* Info = GetActorInfo(Source);
	if (Info == NULL)
	{
		return FAIStimulus::NeverHappenedAge;
	}

	float SmallestAge = FAIStimulus::NeverHappenedAge;
	for (int32 SenseIndex = 0; SenseIndex < ECorePerceptionTypes::MAX; ++SenseIndex)
	{
		if (Info->LastSensedStimuli[SenseIndex].WasSuccessfullySensed())
		{
			float SenseAge = Info->LastSensedStimuli[SenseIndex].GetAge();
			if (SenseAge < SmallestAge)
			{
				SmallestAge = SenseAge;
			}
		}
	}

	return SmallestAge;
}

bool UAIPerceptionComponent::HasAnyActiveStimulus(const class AActor* Source) const
{
	const FActorPerceptionInfo* Info = GetActorInfo(Source);
	if (Info == NULL)
	{
		return false;
	}

	for (int32 SenseIndex = 0; SenseIndex < ECorePerceptionTypes::MAX; ++SenseIndex)
	{
		if (Info->LastSensedStimuli[SenseIndex].WasSuccessfullySensed() &&
			Info->LastSensedStimuli[SenseIndex].GetAge() < FAIStimulus::NeverHappenedAge &&
			Info->LastSensedStimuli[SenseIndex].GetAge() <= MaxActiveAge[SenseIndex])
		{
			return true;
		}
	}

	return false;
}

bool UAIPerceptionComponent::HasActiveStimulus(const class AActor* Source, FAISenseId Sense) const
{
	const FActorPerceptionInfo* Info = GetActorInfo(Source);
	return (Info &&
		Info->LastSensedStimuli[Sense].WasSuccessfullySensed() &&
		Info->LastSensedStimuli[Sense].GetAge() < FAIStimulus::NeverHappenedAge &&
		Info->LastSensedStimuli[Sense].GetAge() <= MaxActiveAge[Sense]);
}

//----------------------------------------------------------------------//
// debug
//----------------------------------------------------------------------//
void UAIPerceptionComponent::DrawDebugInfo(UCanvas* Canvas)
{
	static UEnum* SensesEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ECorePerceptionTypes"));
	check(SensesEnum);

	static const FColor SenseColors[ECorePerceptionTypes::MAX] = {
		FColor::Green,//Sight,
		FColor::Blue,//Hearing,
		FColor::Red,//Damage,
		FColor::Cyan,//Touch
		FColor::Yellow,//Team
		FColorList::Grey,//Prediction
	};
	
	UWorld* World = GetWorld();
	if (World)
	{
		UFont* Font = GEngine->GetSmallFont();

		for (TActorPerceptionContainer::TIterator It(PerceptualData); It; ++It)
		{
			if (It->Key == NULL)
			{
				continue;
			}

			const FActorPerceptionInfo& ActorPerceptionInfo = It->Value;
			
			if (ActorPerceptionInfo.Target.IsValid())
			{
				const FVector TargetLocation = ActorPerceptionInfo.Target->GetActorLocation();

				const FAIStimulus* Stimulus = ActorPerceptionInfo.LastSensedStimuli;
				for (int32 SenseIndex = 0; SenseIndex < ECorePerceptionTypes::MAX; ++SenseIndex, ++Stimulus)
				{
					if (Stimulus->Strength >= 0)
					{
						const FVector ScreenLoc = Canvas->Project(Stimulus->StimulusLocation + FVector(0,0,30));
						Canvas->DrawText(Font, FString::Printf(TEXT("%s: %.2f a:%.2f"), *(SensesEnum->GetEnumText(SenseIndex).ToString())
							, Stimulus->Strength, Stimulus->GetAge())
							, ScreenLoc.X, ScreenLoc.Y);

						DrawDebugSphere(World, Stimulus->StimulusLocation, 30.f, 16, SenseColors[SenseIndex]);
						DrawDebugLine(World, Stimulus->ReceiverLocation, Stimulus->StimulusLocation, SenseColors[SenseIndex]);
						DrawDebugLine(World, TargetLocation, Stimulus->ReceiverLocation, FColor::Black);
					}
				}
			}
		}
	}
}