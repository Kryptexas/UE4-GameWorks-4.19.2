// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineKismetLibraryClasses.h"
#include "EngineDecalClasses.h"
#include "ParticleDefinitions.h"
#include "SoundDefinitions.h"
#include "PlatformFeatures.h"
#include "LatentActions.h"
#include "IForceFeedbackSystem.h"
#include "Slate.h"

//////////////////////////////////////////////////////////////////////////
// UGameplayStatics

UGameplayStatics::UGameplayStatics(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

APlayerController* UGameplayStatics::GetPlayerController(UObject* WorldContextObject, int32 PlayerIndex ) 
{
	UWorld* World = GEngine->GetWorldFromContextObject( WorldContextObject );
	uint32 Index = 0;
	for( FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		APlayerController* PlayerController = *Iterator;
		if( Index == PlayerIndex )
		{
			return PlayerController;			
		}
		Index++;
	}

	return NULL;
}

ACharacter* UGameplayStatics::GetPlayerCharacter(UObject* WorldContextObject, int32 PlayerIndex)
{
	ACharacter* Character = NULL;
	APlayerController* PC = GetPlayerController(WorldContextObject, PlayerIndex );
	if (PC != NULL)
	{
		Character = Cast<ACharacter>(PC->GetPawn());
	}

	return Character;
}

APawn* UGameplayStatics::GetPlayerPawn(UObject* WorldContextObject, int32 PlayerIndex)
{
	APlayerController* PlayerController = GetPlayerController(WorldContextObject, PlayerIndex);
	return PlayerController ? PlayerController->GetPawnOrSpectator() : NULL;
}

APlayerCameraManager* UGameplayStatics::GetPlayerCameraManager(UObject* WorldContextObject, int32 PlayerIndex)
{
	APlayerController* const PC = GetPlayerController( WorldContextObject, PlayerIndex );
	return PC ? PC->PlayerCameraManager : NULL;
}

AGameMode* UGameplayStatics::GetGameMode(UObject* WorldContextObject)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject);
	return World ? World->GetAuthGameMode() : NULL;
}

AGameState* UGameplayStatics::GetGameState(UObject* WorldContextObject)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject);
	return World ? World->GameState : NULL;
}

class UClass* UGameplayStatics::GetObjectClass(const UObject* Object)
{
	return Object ? Object->GetClass() : NULL;
}

void UGameplayStatics::SetGlobalTimeDilation(UObject* WorldContextObject, float TimeDilation)
{
	UWorld* const World = GEngine->GetWorldFromContextObject( WorldContextObject );
	if (TimeDilation < 0.0001f || TimeDilation > 20.f)
	{
		UE_LOG(LogBlueprintUserMessages, Warning, TEXT("Time Dilation must be between 0.0001 and 20.  Clamping value to that range."));
		TimeDilation = FMath::Clamp(TimeDilation, 0.0001f, 20.0f);
	}
	World->GetWorldSettings()->TimeDilation = TimeDilation;
}

bool UGameplayStatics::SetGamePaused(UObject* WorldContextObject, bool bPaused)
{
	UWorld* const World = GEngine->GetWorldFromContextObject( WorldContextObject );
	if (bPaused)
	{
		APlayerController* const PC = World->GetFirstPlayerController();
		check(PC); // Gathering some information for TTP #303973

		return World->GetAuthGameMode()->SetPause(PC);
	}
	else
	{
		World->GetAuthGameMode()->ClearPause();
		return true;
	}
}

/** @RETURN True if weapon trace from Origin hits component VictimComp.  OutHitResult will contain properties of the hit. */
static bool ComponentIsVisibleFrom(UPrimitiveComponent const* VictimComp, FVector const& Origin, AActor const* IgnoredActor, const TArray<AActor*>& IgnoreActors, FHitResult& OutHitResult)
{
	static FName NAME_ComponentIsVisibleFrom = FName(TEXT("ComponentIsVisibleFrom"));
	FCollisionQueryParams LineParams(NAME_ComponentIsVisibleFrom, true, IgnoredActor);
	LineParams.AddIgnoredActors( IgnoreActors );

	// Do a trace from origin to middle of box
	UWorld* World = VictimComp->GetWorld();
	check(World);

	FVector const TraceEnd = VictimComp->Bounds.Origin;
	FVector TraceStart = Origin;
	if (Origin == TraceEnd)
	{
		// tiny nudge so LineTraceSingle doesn't early out with no hits
		TraceStart.Z += 0.01f;
	}
	bool const bHadBlockingHit = World->LineTraceSingle(OutHitResult, TraceStart, TraceEnd, ECC_Visibility, LineParams);
	//::DrawDebugLine(World, TraceStart, TraceEnd, FLinearColor::Red, true);

	// If there was a blocking hit, it will be the last one
	if (bHadBlockingHit)
	{
		if (OutHitResult.Component == VictimComp)
		{
			// if blocking hit was the victim component, it is visible
			return true;
		}
		else
		{
			// if we hit something else blocking, it's not
			UE_LOG(LogDamage, Log, TEXT("Radial Damage to %s blocked by %s (%s)"), *GetNameSafe(VictimComp), *GetNameSafe(OutHitResult.GetActor()), *GetNameSafe(OutHitResult.Component.Get()) );
			return false;
		}
	}
		
	// didn't hit anything, including the victim component.  assume not visible.
	return false;
}

bool UGameplayStatics::ApplyRadialDamage(UObject* WorldContextObject, float BaseDamage, const FVector& Origin, float DamageRadius, TSubclassOf<UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser, AController* InstigatedByController, bool bDoFullDamage )
{
	float DamageFalloff = bDoFullDamage ? 0.f : 1.f;
	return ApplyRadialDamageWithFalloff(WorldContextObject, BaseDamage, 0.f, Origin, 0.f, DamageRadius, DamageFalloff, DamageTypeClass, IgnoreActors, DamageCauser, InstigatedByController);
}

bool UGameplayStatics::ApplyRadialDamageWithFalloff(UObject* WorldContextObject, float BaseDamage, float MinimumDamage, const FVector& Origin, float DamageInnerRadius, float DamageOuterRadius, float DamageFalloff, TSubclassOf<class UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser, AController* InstigatedByController )
{
	static FName NAME_ApplyRadialDamage = FName(TEXT("ApplyRadialDamage"));
	FCollisionQueryParams SphereParams(NAME_ApplyRadialDamage, false, DamageCauser);

	SphereParams.AddIgnoredActors(IgnoreActors);

	// query scene to see what we hit
	TArray<FOverlapResult> Overlaps;
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	World->OverlapMulti(Overlaps, Origin, FQuat::Identity, FCollisionShape::MakeSphere(DamageOuterRadius), SphereParams, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects));

	// collate into per-actor list of hit components
	TMap<AActor*, TArray<FHitResult> > OverlapComponentMap;
	for (int32 Idx=0; Idx<Overlaps.Num(); ++Idx)
	{
		FOverlapResult const& Overlap = Overlaps[Idx];
		AActor* const OverlapActor = Overlap.GetActor();

		if ( OverlapActor && 
			OverlapActor->bCanBeDamaged && 
			OverlapActor != DamageCauser &&
			Overlap.Component.IsValid() )
		{
			FHitResult Hit;
			if (ComponentIsVisibleFrom(Overlap.Component.Get(), Origin, DamageCauser, IgnoreActors, Hit))
			{
				TArray<FHitResult>& HitList = OverlapComponentMap.FindOrAdd(OverlapActor);
				HitList.Add(Hit);
			}
		}
	}

	// make sure we have a good damage type
	TSubclassOf<UDamageType> const ValidDamageTypeClass = (DamageTypeClass == NULL) ? TSubclassOf<UDamageType>(UDamageType::StaticClass()) : DamageTypeClass;

	bool bAppliedDamage = false;

	// call damage function on each affected actors
	for (TMap<AActor*, TArray<FHitResult> >::TIterator It(OverlapComponentMap); It; ++It)
	{
		AActor* const Victim = It.Key();
		TArray<FHitResult> const& ComponentHits = It.Value();

		FRadialDamageEvent DmgEvent;
		DmgEvent.DamageTypeClass = ValidDamageTypeClass;
		DmgEvent.ComponentHits = ComponentHits;
		DmgEvent.Origin = Origin;
		DmgEvent.Params = FRadialDamageParams(BaseDamage, MinimumDamage, DamageInnerRadius, DamageOuterRadius, DamageFalloff);

		Victim->TakeDamage(BaseDamage, DmgEvent, InstigatedByController, DamageCauser);

		bAppliedDamage = true;
	}

	return bAppliedDamage;
}

void UGameplayStatics::ApplyPointDamage(AActor* DamagedActor, float BaseDamage, FVector const& HitFromDirection, FHitResult const& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (DamagedActor && BaseDamage != 0.f)
	{
		// make sure we have a good damage type
		TSubclassOf<UDamageType> const ValidDamageTypeClass = DamageTypeClass == NULL ? TSubclassOf<UDamageType>(UDamageType::StaticClass()) : DamageTypeClass;
		FPointDamageEvent PointDamageEvent(BaseDamage, HitInfo, HitFromDirection, ValidDamageTypeClass);

		DamagedActor->TakeDamage(BaseDamage, PointDamageEvent, EventInstigator, DamageCauser);
	}
}

void UGameplayStatics::ApplyDamage(AActor* DamagedActor, float BaseDamage, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass)
{
	if ( DamagedActor && (BaseDamage != 0.f) )
	{
		// make sure we have a good damage type
		TSubclassOf<UDamageType> const ValidDamageTypeClass = DamageTypeClass == NULL ? TSubclassOf<UDamageType>(UDamageType::StaticClass()) : DamageTypeClass;
		FDamageEvent DamageEvent(ValidDamageTypeClass);

		DamagedActor->TakeDamage(BaseDamage, DamageEvent, EventInstigator, DamageCauser);
	}
}


AActor* UGameplayStatics::BeginSpawningActorFromBlueprint(UObject* WorldContextObject, UBlueprint const* Blueprint, const FTransform& SpawnTransform, bool bNoCollisionFail)
{
	AActor* NewActor = NULL;
	if(Blueprint != NULL && Blueprint->GeneratedClass != NULL)
	{
		if( Blueprint->GeneratedClass->IsChildOf(AActor::StaticClass()) )
		{
			NewActor = BeginSpawningActorFromClass(WorldContextObject, *Blueprint->GeneratedClass, SpawnTransform, bNoCollisionFail);
		}
		else
		{
			UE_LOG(LogScript, Warning, TEXT("UGameplayStatics::BeginSpawningActorFromBlueprint: %s is not an actor class"), *Blueprint->GeneratedClass->GetName() );
		}
	}
	return NewActor;
}

AActor* UGameplayStatics::BeginSpawningActorFromClass(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, bool bNoCollisionFail)
{
	const FVector SpawnLoc(SpawnTransform.GetTranslation());
	const FRotator SpawnRot(SpawnTransform.GetRotation());

	AActor* NewActor = NULL;

	UClass* Class = *ActorClass;
	if (Class != NULL)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
		NewActor = World->SpawnActorDeferred<AActor>(Class, SpawnLoc, SpawnRot, NULL, NULL, bNoCollisionFail);
	}

	return NewActor;
}

AActor* UGameplayStatics::FinishSpawningActor(AActor* Actor, const FTransform& SpawnTransform)
{
	if (Actor)
	{
		Actor->OnConstruction(SpawnTransform);
		Actor->PostActorConstruction();
	}

	return Actor;
}

void UGameplayStatics::LoadStreamLevel(UObject* WorldContextObject, FName LevelName,bool bMakeVisibleAfterLoad,bool bShouldBlockOnLoad,FLatentActionInfo LatentInfo)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	FLatentActionManager& LatentManager = World->GetLatentActionManager();
	if (LatentManager.FindExistingAction<FStreamLevelAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == NULL)
	{
		FStreamLevelAction* NewAction = new FStreamLevelAction(true, LevelName, bMakeVisibleAfterLoad, bShouldBlockOnLoad, LatentInfo, World);
		LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, NewAction);
	}
}

void UGameplayStatics::UnloadStreamLevel(UObject* WorldContextObject, FName LevelName,FLatentActionInfo LatentInfo)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	FLatentActionManager& LatentManager = World->GetLatentActionManager();
	if (LatentManager.FindExistingAction<FStreamLevelAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == NULL)
	{
		FStreamLevelAction* NewAction = new FStreamLevelAction(false, LevelName, false, false, LatentInfo, World );
		LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, NewAction );
	}
}

ULevelStreamingKismet* UGameplayStatics::GetStreamingLevel(UObject* WorldContextObject, FName InPackageName)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);

	// User can pass in short package name
	const bool bShortName = FPackageName::IsShortPackageName(InPackageName);

	for (auto It = World->StreamingLevels.CreateConstIterator(); It; ++It)
	{
		FName PackageName = (*It)->PackageName;

#ifdef WITH_EDITOR 
		// PIE uses this field to store original package name
		if (World->IsPlayInEditor())
		{
			PackageName = (*It)->PackageNameToLoad;
		}
#endif // WITH_EDITOR

		if (bShortName)
		{
			PackageName = FPackageName::GetShortFName(PackageName);
		}

		if (PackageName == InPackageName)
		{
			return Cast<ULevelStreamingKismet>(*It);
		}
	}
	
	return NULL;
}

void UGameplayStatics::OpenLevel(UObject* WorldContextObject, FName LevelName, bool bAbsolute, FString Options)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	const ETravelType TravelType = (bAbsolute ? TRAVEL_Absolute : TRAVEL_Relative);
	FWorldContext &WorldContext = GEngine->WorldContextFromWorld(World);
	FString Cmd = LevelName.ToString();
	if (Options.Len() > 0)
	{
		Cmd += FString(TEXT("?")) + Options;
	}
	FURL TestURL(&WorldContext.LastURL, *Cmd, TravelType);
	if (TestURL.IsLocalInternal())
	{
		// make sure the file exists if we are opening a local file
		if (!GEngine->MakeSureMapNameIsValid(TestURL.Map))
		{
			UE_LOG(LogLevel, Warning, TEXT("WARNING: The map '%s' does not exist."), *TestURL.Map);
		}
	}

	GEngine->SetClientTravel( World, *Cmd, TravelType );
}

FVector UGameplayStatics::GetActorArrayAverageLocation(const TArray<AActor*>& Actors)
{
	FVector LocationSum(0,0,0); // sum of locations
	int32 ActorCount = 0; // num actors
	// iterate over actors
	for(int32 ActorIdx=0; ActorIdx<Actors.Num(); ActorIdx++)
	{
		AActor* A = Actors[ActorIdx];
		// Check actor is non-null, not deleted, and has a root component
		if(A != NULL && !A->IsPendingKill() && A->GetRootComponent() != NULL)
		{
			LocationSum += A->GetActorLocation();
			ActorCount++;
		}
	}

	// Find average
	FVector Average(0,0,0);
	if(ActorCount > 0)
	{
		Average = LocationSum/((float)ActorCount);
	}
	return Average;
}

void UGameplayStatics::GetActorArrayBounds(const TArray<AActor*>& Actors, bool bOnlyCollidingComponents, FVector& Center, FVector& BoxExtent)
{
	FBox ActorBounds(0);
	// Iterate over actors and accumulate bouding box
	for(int32 ActorIdx=0; ActorIdx<Actors.Num(); ActorIdx++)
	{
		AActor* A = Actors[ActorIdx];
		// Check actor is non-null, not deleted
		if(A != NULL && !A->IsPendingKill())
		{
			ActorBounds += A->GetComponentsBoundingBox(!bOnlyCollidingComponents);
		}
	}

	// if a valid box, get its center and extent
	Center = BoxExtent = FVector::ZeroVector;
	if(ActorBounds.IsValid)
	{
		Center = ActorBounds.GetCenter();
		BoxExtent = ActorBounds.GetExtent();
	}
}

void UGameplayStatics::GetAllActorsOfClass(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors)
{
	OutActors.Empty();

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);

	// We do nothing if not class provided, rather than giving ALL actors!
	if(ActorClass != NULL)
	{
		// If it's a pawn class, use pawn iterator, that is more efficient
		if(ActorClass->IsChildOf(APawn::StaticClass()))
		{
			for(FConstPawnIterator Iterator = World->GetPawnIterator(); Iterator; ++Iterator)
			{
				APawn* Pawn = *Iterator;
				if(Pawn != NULL && !Pawn->IsPendingKill() && Pawn->IsA(ActorClass))
				{
					OutActors.Add(Pawn);
				}
			}
		}
		// Non-pawn, so use actor iterator in the world
		else
		{
			for(FActorIterator It(World); It; ++It)
			{
				AActor* Actor = *It;
				if(Actor != NULL && !Actor->IsPendingKill() && Actor->IsA(ActorClass))
				{
					OutActors.Add(Actor);
				}
			}
		}
	}
}

void UGameplayStatics::GetAllActorsWithInterface(UObject* WorldContextObject, TSubclassOf<UInterface> Interface, TArray<AActor*>& OutActors)
{
	OutActors.Empty();

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	// We do nothing if not class provided, rather than giving ALL actors!
	if(Interface != NULL)
	{
		for(FActorIterator It(World); It; ++It)
		{
			AActor* Actor = *It;
			if(Actor != NULL && !Actor->IsPendingKill() && Actor->GetClass()->ImplementsInterface(Interface))
			{
				OutActors.Add(Actor);
			}
		}
	}
}


void UGameplayStatics::PlayWorldCameraShake(UObject* WorldContextObject, TSubclassOf<class UCameraShake> Shake, FVector Epicenter, float InnerRadius, float OuterRadius, float Falloff, bool bOrientShakeTowardsEpicenter)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	APlayerCameraManager::PlayWorldCameraShake(World, Shake, Epicenter, InnerRadius, OuterRadius, Falloff, bOrientShakeTowardsEpicenter);
}

UParticleSystemComponent* CreateParticleSystem(UParticleSystem* EmitterTemplate, UWorld* World, AActor* Actor, bool bAutoDestroy)
{
	UParticleSystemComponent* PSC = ConstructObject<UParticleSystemComponent>(UParticleSystemComponent::StaticClass(), (Actor ? Actor : (UObject*)World) );
	PSC->bAutoDestroy = bAutoDestroy;
	PSC->SecondsBeforeInactive = 0.0f;
	PSC->bAutoActivate = false;
	PSC->SetTemplate(EmitterTemplate);
	PSC->bOverrideLODMethod = false;

	PSC->RegisterComponentWithWorld(World);

	return PSC;
}

UParticleSystemComponent* UGameplayStatics::SpawnEmitterAtLocation(UObject* WorldContextObject, UParticleSystem* EmitterTemplate, FVector SpawnLocation, FRotator SpawnRotation, bool bAutoDestroy)
{
	UParticleSystemComponent* PSC = NULL;
	if (EmitterTemplate)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);

		PSC = CreateParticleSystem(EmitterTemplate, World, NULL, bAutoDestroy);

		PSC->SetAbsolute(true, true, true);
		PSC->SetWorldLocationAndRotation(SpawnLocation, SpawnRotation);
		PSC->SetRelativeScale3D(FVector(1.f));
		PSC->ActivateSystem(true);
	}
	return PSC;
}

UParticleSystemComponent* UGameplayStatics::SpawnEmitterAttached(UParticleSystem* EmitterTemplate, USceneComponent* AttachToComponent, FName AttachPointName, FVector Location, FRotator Rotation, EAttachLocation::Type LocationType, bool bAutoDestroy)
{
	UParticleSystemComponent* PSC = NULL;
	if (EmitterTemplate)
	{
		if (AttachToComponent == NULL)
		{
			UE_LOG(LogScript, Warning, TEXT("UGameplayStatics::SpawnEmitterAttached: NULL AttachComponent specified!"));
		}
		else
		{
			PSC = CreateParticleSystem(EmitterTemplate, AttachToComponent->GetWorld(), AttachToComponent->GetOwner(), bAutoDestroy);

			PSC->AttachTo(AttachToComponent, AttachPointName);
			if (LocationType == EAttachLocation::KeepWorldPosition)
			{
				PSC->SetWorldLocationAndRotation(Location, Rotation);
			}
			else
			{
				PSC->SetRelativeLocationAndRotation(Location, Rotation);
			}
			PSC->SetRelativeScale3D(FVector(1.f));
			PSC->ActivateSystem(true);
		}
	}
	return PSC;
}

void UGameplayStatics::BreakHitResult(const FHitResult& Hit, FVector& Location, FVector& Normal, FVector& ImpactPoint, FVector& ImpactNormal, UPhysicalMaterial*& PhysMat, AActor*& HitActor, UPrimitiveComponent*& HitComponent, FName& HitBoneName)
{
	Location = Hit.Location;
	Normal = Hit.Normal;
	ImpactPoint = Hit.ImpactPoint;
	ImpactNormal = Hit.ImpactNormal;
	PhysMat = Hit.PhysMaterial.Get();
	HitActor = Hit.GetActor();
	HitComponent = Hit.Component.Get();
	HitBoneName = Hit.BoneName;
}

EPhysicalSurface UGameplayStatics::GetSurfaceType(const struct FHitResult& Hit)
{
	UPhysicalMaterial* const HitPhysMat = Hit.PhysMaterial.Get();
	return UPhysicalMaterial::DetermineSurfaceType( HitPhysMat );
}

bool UGameplayStatics::AreAnyListenersWithinRange(FVector Location, float MaximumRange)
{
	if (GEngine && GEngine->UseSound())
	{
		return GEngine->GetAudioDevice()->LocationIsAudible(Location, MaximumRange);
	}

	return false;
}

void UGameplayStatics::PlaySoundAtLocation(UObject* WorldContextObject, class USoundBase* Sound, FVector Location, float VolumeMultiplier, float PitchMultiplier, float StartTime, class USoundAttenuation* AttenuationSettings)
{
	if ( Sound == NULL )
	{
		return;
	}

	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject);

	const bool bIsInGameWorld = ThisWorld->IsGameWorld();

	if (GEngine && GEngine->UseSound() && ThisWorld->GetNetMode() != NM_DedicatedServer)
	{
		if( Sound->IsAudibleSimple( Location ) )
		{
			FActiveSound NewActiveSound;
			NewActiveSound.World = ThisWorld;
			NewActiveSound.Sound = Sound;

			NewActiveSound.VolumeMultiplier = VolumeMultiplier;
			NewActiveSound.PitchMultiplier = PitchMultiplier;

			NewActiveSound.RequestedStartTime = FMath::Max(0.f, StartTime);

			NewActiveSound.bLocationDefined = true;
			NewActiveSound.Transform.SetTranslation(Location);

			NewActiveSound.bIsUISound = !bIsInGameWorld;
			NewActiveSound.bHandleSubtitles = true;
			NewActiveSound.SubtitlePriority = 10000.f; // Fixme: pass in? Do we want that exposed to blueprints though?

			const FAttenuationSettings* AttenuationSettingsToApply = (AttenuationSettings ? &AttenuationSettings->Attenuation : Sound->GetAttenuationSettingsToApply());

			NewActiveSound.bHasAttenuationSettings = (bIsInGameWorld && AttenuationSettingsToApply);
			if (NewActiveSound.bHasAttenuationSettings)
			{
				NewActiveSound.AttenuationSettings = *AttenuationSettingsToApply;
			}

			// TODO - Audio Threading. This call would be a task call to dispatch to the audio thread
			GEngine->GetAudioDevice()->AddNewActiveSound(NewActiveSound);
		}
		else
		{
			// Don't play a sound for short sounds that start out of range of any listener
			UE_LOG(LogAudio, Log, TEXT( "Sound not played for out of range Sound %s" ), *Sound->GetName() );
		}
	}
}

void UGameplayStatics::PlayDialogueAtLocation(UObject* WorldContextObject, class UDialogueWave* Dialogue, const FDialogueContext& Context, FVector Location, float VolumeMultiplier, float PitchMultiplier, float StartTime, class USoundAttenuation* AttenuationSettings)
{
	if ( Dialogue == NULL )
	{
		return;
	}

	USoundBase* Sound = Dialogue->GetWaveFromContext(Context);
	if ( Sound == NULL )
	{
		return;
	}

	if (GEngine && GEngine->UseSound())
	{
		UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject);
		const bool bIsInGameWorld = ThisWorld->IsGameWorld();

		if( Sound->IsAudibleSimple( Location ) )
		{
			FActiveSound NewActiveSound;
			NewActiveSound.World = ThisWorld;
			NewActiveSound.Sound = Sound;

			NewActiveSound.VolumeMultiplier = VolumeMultiplier;
			NewActiveSound.PitchMultiplier = PitchMultiplier;

			NewActiveSound.RequestedStartTime = FMath::Max(0.f, StartTime);

			NewActiveSound.bLocationDefined = true;
			NewActiveSound.Transform.SetTranslation(Location);

			NewActiveSound.bIsUISound = !bIsInGameWorld;
			NewActiveSound.bHandleSubtitles = true;
			NewActiveSound.SubtitlePriority = 10000.f; // Fixme: pass in? Do we want that exposed to blueprints though?

			const FAttenuationSettings* AttenuationSettingsToApply = (AttenuationSettings ? &AttenuationSettings->Attenuation : Sound->GetAttenuationSettingsToApply());

			NewActiveSound.bHasAttenuationSettings = (bIsInGameWorld && AttenuationSettingsToApply);
			if (NewActiveSound.bHasAttenuationSettings)
			{
				NewActiveSound.AttenuationSettings = *AttenuationSettingsToApply;
			}

			// TODO - Audio Threading. This call would be a task call to dispatch to the audio thread
			GEngine->GetAudioDevice()->AddNewActiveSound(NewActiveSound);
		}
		else
		{
			// Don't play a sound for short sounds that start out of range of any listener
			UE_LOG(LogAudio, Log, TEXT( "Sound not played for out of range Sound %s" ), *Sound->GetName() );
		}
	}
}

class UAudioComponent* UGameplayStatics::PlaySoundAttached(class USoundBase* Sound, class USceneComponent* AttachToComponent, FName AttachPointName, FVector Location, EAttachLocation::Type LocationType, bool bStopWhenAttachedToDestroyed, float VolumeMultiplier, float PitchMultiplier, float StartTime, class USoundAttenuation* AttenuationSettings)
{
	if ( Sound == NULL )
	{
		return NULL;
	}
	if ( AttachToComponent == NULL )
	{
		UE_LOG(LogScript, Warning, TEXT("UGameplayStatics::PlaySoundAttached: NULL AttachComponent specified!"));
		return NULL;
	}

	UAudioComponent* AudioComponent = FAudioDevice::CreateComponent( Sound, AttachToComponent->GetWorld(), AttachToComponent->GetOwner(), false, bStopWhenAttachedToDestroyed );
	if(AudioComponent)
	{
		const bool bIsInGameWorld = AudioComponent->GetWorld()->IsGameWorld();

		AudioComponent->AttachTo(AttachToComponent, AttachPointName);
		if (LocationType == EAttachLocation::KeepWorldPosition)
		{
			AudioComponent->SetWorldLocation(Location);
		}
		else
		{
			AudioComponent->SetRelativeLocation(Location);
		}
		AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
		AudioComponent->SetPitchMultiplier(PitchMultiplier);
		AudioComponent->bAllowSpatialization	= bIsInGameWorld;
		AudioComponent->bIsUISound				= !bIsInGameWorld;
		AudioComponent->bAutoDestroy			= true;
		AudioComponent->SubtitlePriority		= 10000.f; // Fixme: pass in? Do we want that exposed to blueprints though?
		AudioComponent->AttenuationSettings		= AttenuationSettings;
		AudioComponent->Play(StartTime);
	}

	return AudioComponent;
}

class UAudioComponent* UGameplayStatics::PlayDialogueAttached(class UDialogueWave* Dialogue, const FDialogueContext& Context, class USceneComponent* AttachToComponent, FName AttachPointName, FVector Location, EAttachLocation::Type LocationType, bool bStopWhenAttachedToDestroyed, float VolumeMultiplier, float PitchMultiplier, float StartTime, class USoundAttenuation* AttenuationSettings)
{
	if ( Dialogue == NULL )
	{
		return NULL;
	}

	USoundBase* Sound = Dialogue->GetWaveFromContext(Context);
	if ( Sound == NULL )
	{
		return NULL;
	}

	if ( AttachToComponent == NULL )
	{
		UE_LOG(LogScript, Warning, TEXT("UGameplayStatics::PlaySoundAttached: NULL AttachComponent specified!"));
		return NULL;
	}

	UAudioComponent* AudioComponent = FAudioDevice::CreateComponent( Sound, AttachToComponent->GetWorld(), AttachToComponent->GetOwner(), false, bStopWhenAttachedToDestroyed );
	if(AudioComponent)
	{
		const bool bIsGameWorld = AudioComponent->GetWorld()->IsGameWorld();

		AudioComponent->AttachTo(AttachToComponent, AttachPointName);
		if (LocationType == EAttachLocation::KeepWorldPosition)
		{
			AudioComponent->SetWorldLocation(Location);
		}
		else
		{
			AudioComponent->SetRelativeLocation(Location);
		}
		AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
		AudioComponent->SetPitchMultiplier(PitchMultiplier);
		AudioComponent->bAllowSpatialization	= bIsGameWorld;
		AudioComponent->bIsUISound				= !bIsGameWorld;
		AudioComponent->bAutoDestroy			= true;
		AudioComponent->SubtitlePriority		= 10000.f; // Fixme: pass in? Do we want that exposed to blueprints though?
		AudioComponent->AttenuationSettings		= AttenuationSettings;
		AudioComponent->Play(StartTime);
	}

	return AudioComponent;
}

void UGameplayStatics::PlaySound(UObject* WorldContextObject, class USoundCue* InSoundCue, class USceneComponent* AttachComponent, FName AttachName, bool bFollow, float VolumeMultiplier, float PitchMultiplier)
{
	if (bFollow)
	{
		PlaySoundAttached(InSoundCue, AttachComponent, AttachName, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, false, VolumeMultiplier, PitchMultiplier);
	}
	else
	{
		if ( AttachComponent == NULL )
		{
			UE_LOG(LogScript, Warning, TEXT("UGameplayStatics::PlaySound: NULL AttachComponent specified!"));
			return;
		}
		FVector SoundLocation;
		if (AttachName == NAME_None)
		{
			SoundLocation = AttachComponent->GetComponentLocation();
		}
		else
		{
			FTransform const SocketTransform = AttachComponent->GetSocketTransform(AttachName,RTS_World);
			SoundLocation = SocketTransform.GetTranslation();
		}
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
		PlaySoundAtLocation(World, InSoundCue,SoundLocation, VolumeMultiplier, PitchMultiplier);
	}
}

void UGameplayStatics::SetBaseSoundMix(USoundMix* InSoundMix)
{
	if (InSoundMix)
	{
		FAudioDevice* AudioDevice = GEngine->GetAudioDevice();
		if( AudioDevice != NULL )
		{
			AudioDevice->SetBaseSoundMix(InSoundMix);
		}
	}
}

void UGameplayStatics::PushSoundMixModifier(USoundMix* InSoundMixModifier)
{
	if (InSoundMixModifier)
	{
		FAudioDevice* AudioDevice = GEngine->GetAudioDevice();
		if( AudioDevice != NULL )
		{
			AudioDevice->PushSoundMixModifier(InSoundMixModifier);
		}
	}
}

void UGameplayStatics::PopSoundMixModifier(USoundMix* InSoundMixModifier)
{
	if (InSoundMixModifier)
	{
		FAudioDevice* AudioDevice = GEngine->GetAudioDevice();
		if( AudioDevice != NULL )
		{
			AudioDevice->PopSoundMixModifier(InSoundMixModifier);
		}
	}
}

void UGameplayStatics::ClearSoundMixModifiers()
{
	FAudioDevice* AudioDevice = GEngine->GetAudioDevice();
	if( AudioDevice != NULL )
	{
		AudioDevice->ClearSoundMixModifiers();
	}
}

void UGameplayStatics::SetSoundMode(FName InSoundModeName)
{
}

void UGameplayStatics::ActivateReverbEffect(class UReverbEffect* ReverbEffect, FName TagName, float Priority, float Volume, float FadeTime)
{
	if (ReverbEffect)
	{
		FAudioDevice* AudioDevice = GEngine->GetAudioDevice();
		if( AudioDevice != NULL )
		{
			AudioDevice->ActivateReverbEffect(ReverbEffect, TagName, Priority, Volume, FadeTime);
		}
	}
}

void UGameplayStatics::DeactivateReverbEffect(FName TagName)
{
	FAudioDevice* AudioDevice = GEngine->GetAudioDevice();
	if( AudioDevice != NULL )
	{
		AudioDevice->DeactivateReverbEffect(TagName);
	}
}

UDecalComponent* CreateDecalComponent(class UMaterialInterface* DecalMaterial, FVector DecalSize, UWorld* World, AActor* Actor, float LifeSpan)
{
	const FMatrix DecalInternalTransform = FRotationMatrix(FRotator(0, 90.0f, -90.0f));

	UDecalComponent* DecalComp = ConstructObject<UDecalComponent>(UDecalComponent::StaticClass(), (Actor ? Actor : (UObject*)GetTransientPackage()));
	DecalComp->DecalMaterial = DecalMaterial;
	DecalComp->RelativeScale3D = DecalInternalTransform.TransformVector(DecalSize);
	DecalComp->bAbsoluteScale = true;
	DecalComp->RegisterComponentWithWorld(World);

	if (LifeSpan > 0)
	{
		World->GetTimerManager().SetTimer(DecalComp, &UDecalComponent::DestroyComponent, LifeSpan, false);
	}

	return DecalComp;
}

UDecalComponent* UGameplayStatics::SpawnDecalAtLocation(UObject* WorldContextObject, class UMaterialInterface* DecalMaterial, FVector DecalSize, FVector Location, FRotator Rotation, float LifeSpan)
{
	UDecalComponent* DecalComp = NULL;

	if (DecalMaterial)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
		DecalComp = CreateDecalComponent(DecalMaterial, DecalSize, World, NULL, LifeSpan);
		DecalComp->SetWorldLocationAndRotation(Location, Rotation);
	}

	return DecalComp;
}

UDecalComponent* UGameplayStatics::SpawnDecalAttached(class UMaterialInterface* DecalMaterial, FVector DecalSize, class USceneComponent* AttachToComponent, FName AttachPointName, FVector Location, FRotator Rotation, EAttachLocation::Type LocationType, float LifeSpan)
{
	UDecalComponent* DecalComp = NULL;

	if (DecalMaterial)
	{
		if (AttachToComponent == NULL)
		{
			UE_LOG(LogScript, Warning, TEXT("UGameplayStatics::SpawnDecalAttached: NULL AttachComponent specified!"));
		}
		else
		{
			UPrimitiveComponent* AttachToPrimitive = Cast<UPrimitiveComponent>(AttachToComponent);
			if (AttachToPrimitive == NULL || AttachToPrimitive->bReceivesDecals)
			{
				const bool bOnBSPBrush = (Cast<AWorldSettings>(AttachToPrimitive->GetOwner()) != NULL);
				if (bOnBSPBrush)
				{
					// special case: don't attach to component when it's owned by invisible WorldSettings (decals on BSP brush)
					DecalComp = SpawnDecalAtLocation(AttachToPrimitive->GetOwner(), DecalMaterial, DecalSize, Location, Rotation, LifeSpan);
				}
				else
				{
					DecalComp = CreateDecalComponent(DecalMaterial, DecalSize, AttachToComponent->GetWorld(), AttachToComponent->GetOwner(), LifeSpan);
					DecalComp->AttachTo(AttachToComponent, AttachPointName);

					if (LocationType == EAttachLocation::KeepWorldPosition)
					{
						DecalComp->SetWorldLocationAndRotation(Location, Rotation);
					}
					else
					{
						DecalComp->SetRelativeLocationAndRotation(Location, Rotation);
					}
				}
			}
		}
	}

	return DecalComp;
}

//////////////////////////////////////////////////////////////////////////

USaveGame* UGameplayStatics::CreateSaveGameObject(TSubclassOf<USaveGame> SaveGameClass)
{
	USaveGame* OutSaveGameObject = NULL;

	// don't save if no class or if class is the abstract base class
	if ( (*SaveGameClass != NULL) && (*SaveGameClass != USaveGame::StaticClass()) )
	{
		OutSaveGameObject = NewObject<USaveGame>(GetTransientPackage(), SaveGameClass);
	}

	return OutSaveGameObject;
}

USaveGame* UGameplayStatics::CreateSaveGameObjectFromBlueprint(UBlueprint* SaveGameBlueprint)
{
	USaveGame* OutSaveGameObject = NULL;

	if( SaveGameBlueprint != NULL &&
		SaveGameBlueprint->GeneratedClass != NULL &&
		SaveGameBlueprint->GeneratedClass->IsChildOf(USaveGame::StaticClass()) )
	{
		OutSaveGameObject = NewObject<USaveGame>(GetTransientPackage(), SaveGameBlueprint->GeneratedClass);
	}

	return OutSaveGameObject;
}

bool UGameplayStatics::SaveGameToSlot(USaveGame* SaveGameObject, const FString& SlotName)
{
	bool bSuccess = false;

	ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
	// If we have a system and an object to save and a save name...
	if(SaveSystem && (SaveGameObject != NULL) && (SlotName.Len() > 0))
	{
		TArray<uint8> ObjectBytes;
		FMemoryWriter MemoryWriter(ObjectBytes, true);

		// First write the class name so we know what class to load to
		FString SaveGameClassName = SaveGameObject->GetClass()->GetName();
		MemoryWriter << SaveGameClassName;

		// Then save the object state, replacing object refs and names with strings
		FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
		SaveGameObject->Serialize(Ar);

		// Stuff that data into the save system with the desired file name
		bSuccess = SaveSystem->SaveGame(false, *SlotName, ObjectBytes);
	}

	return bSuccess;
}

bool UGameplayStatics::DoesSaveGameExist(const FString& SlotName)
{
	bool bExists = false;

	ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
	if(SaveSystem != NULL)
	{
		bExists = SaveSystem->DoesSaveGameExist(*SlotName);
	}

	return bExists;
}


USaveGame* UGameplayStatics::LoadGameFromSlot(const FString& SlotName)
{
	USaveGame* OutSaveGameObject = NULL;

	ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
	// If we have a save system and a valid name..
	if(SaveSystem && (SlotName.Len() > 0))
	{
		// Load raw data from slot
		TArray<uint8> ObjectBytes;
		bool bSuccess = SaveSystem->LoadGame(false, *SlotName, ObjectBytes);
		if(bSuccess)
		{
			FMemoryReader MemoryReader(ObjectBytes, true);

			// Get the class name
			FString SaveGameClassName;
			MemoryReader << SaveGameClassName;

			// Try and find it, and failing that, load it
			UClass* SaveGameClass = FindObject<UClass>(ANY_PACKAGE, *SaveGameClassName);
			if(SaveGameClass == NULL)
			{
				SaveGameClass = LoadObject<UClass>(NULL, *SaveGameClassName);
			}

			// If we have a class, try and load it.
			if(SaveGameClass != NULL)
			{
				OutSaveGameObject = NewObject<USaveGame>(GetTransientPackage(), SaveGameClass);

				FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
				OutSaveGameObject->Serialize(Ar);
			}
		}
	}

	return OutSaveGameObject;
}

float UGameplayStatics::GetWorldDeltaSeconds(UObject* WorldContextObject)
{
	return GEngine->GetWorldFromContextObject(WorldContextObject)->GetDeltaSeconds();
}

float UGameplayStatics::GetRealTimeSeconds(UObject* WorldContextObject)
{
	return GEngine->GetWorldFromContextObject(WorldContextObject)->GetRealTimeSeconds();
}

float UGameplayStatics::GetAudioTimeSeconds(UObject* WorldContextObject)
{
	return GEngine->GetWorldFromContextObject(WorldContextObject)->GetAudioTimeSeconds();
}

void UGameplayStatics::GetAccurateRealTime(UObject* WorldContextObject, int32& Seconds, float& PartialSeconds)
{
	double TimeSeconds = FPlatformTime::Seconds();
	Seconds = floor(TimeSeconds);
	PartialSeconds = TimeSeconds - double(Seconds);
}

void UGameplayStatics::EnableLiveStreaming(bool Enable)
{
	IDVRStreamingSystem* StreamingSystem = IPlatformFeaturesModule::Get().GetStreamingSystem();
	if (StreamingSystem)
	{
		StreamingSystem->EnableStreaming(Enable);
	}
}

bool UGameplayStatics::BlueprintSuggestProjectileVelocity(UObject* WorldContextObject, FVector& OutTossVelocity, FVector StartLocation, FVector EndLocation, float LaunchSpeed, float OverrideGravityZ, ESuggestProjVelocityTraceOption::Type TraceOption, float CollisionRadius, bool bFavorHighArc, bool bDrawDebug)
{
	// simple pass-through to the C++ interface
	return UGameplayStatics::SuggestProjectileVelocity(WorldContextObject, OutTossVelocity, StartLocation, EndLocation, LaunchSpeed, bFavorHighArc, CollisionRadius, OverrideGravityZ, TraceOption, bDrawDebug);
}

// Based on analytic solution to ballistic angle of launch http://en.wikipedia.org/wiki/Trajectory_of_a_projectile#Angle_required_to_hit_coordinate_.28x.2Cy.29
bool UGameplayStatics::SuggestProjectileVelocity(UObject* WorldContextObject, FVector& OutTossVelocity, FVector Start, FVector End, float TossSpeed, bool bFavorHighArc, float CollisionRadius, float OverrideGravityZ, ESuggestProjVelocityTraceOption::Type TraceOption, bool bDrawDebug)
{
	const FVector FlightDelta = End - Start;
	const FVector DirXY = FlightDelta.SafeNormal2D();
	const float DeltaXY = FlightDelta.Size2D();

	const float DeltaZ = FlightDelta.Z;

	const float TossSpeedSq = FMath::Square(TossSpeed);

	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject);

	const float GravityZ = (OverrideGravityZ != 0.f) ? OverrideGravityZ : World->GetGravityZ();


	// v^4 - g*(g*x^2 + 2*y*v^2)
	const float InsideTheSqrt = FMath::Square(TossSpeedSq) - GravityZ * ( (GravityZ * FMath::Square(DeltaXY)) + (2.f * DeltaZ * TossSpeedSq) );
	if (InsideTheSqrt < 0.f)
	{
		// sqrt will be imaginary, therefore no solutions
		return false;
	}

	// if we got here, there are 2 solutions: one high-angle and one low-angle.

	const float SqrtPart = FMath::Sqrt(InsideTheSqrt);

	// this is the tangent of the firing angle for the first (+) solution
	const float TanSolutionAngleA = (TossSpeedSq + SqrtPart) / (GravityZ * DeltaXY);
	// this is the tangent of the firing angle for the second (-) solution
	const float TanSolutionAngleB = (TossSpeedSq - SqrtPart) / (GravityZ * DeltaXY);
	
	// mag in the XY dir = sqrt( TossSpeedSq / (TanSolutionAngle^2 + 1) );
	const float MagXYSq_A = TossSpeedSq / (FMath::Square(TanSolutionAngleA) + 1.f);
	const float MagXYSq_B = TossSpeedSq / (FMath::Square(TanSolutionAngleB) + 1.f);

	bool bFoundAValidSolution = false;

	// trace if desired
	if (TraceOption == ESuggestProjVelocityTraceOption::DoNotTrace)
	{
		// choose which arc
		const float FavoredMagXYSq = bFavorHighArc ? FMath::Min(MagXYSq_A, MagXYSq_B) : FMath::Max(MagXYSq_A, MagXYSq_B);

		// finish calculations
		const float MagXY = FMath::Sqrt(FavoredMagXYSq);
		const float MagZ = FMath::Sqrt(TossSpeedSq - FavoredMagXYSq);		// pythagorean

		// final answer!
		OutTossVelocity = (DirXY * MagXY) + (FVector::UpVector * MagZ);
		bFoundAValidSolution = true;

	 	if (bDrawDebug)
	 	{
	 		static const float StepSize = 0.125f;
	 		FVector TraceStart = Start;
	 		for ( float Step=0.f; Step<1.f; Step+=StepSize )
	 		{
	 			const float TimeInFlight = (Step+StepSize) * DeltaXY/MagXY;
	 
	 			// d = vt + .5 a t^2
	 			const FVector TraceEnd = Start + OutTossVelocity*TimeInFlight + FVector(0.f, 0.f, 0.5f * GravityZ * FMath::Square(TimeInFlight) - CollisionRadius);
	 
	 			DrawDebugLine( World, TraceStart, TraceEnd, (bFoundAValidSolution ? FColor::Yellow : FColor::Red), true );
	 			TraceStart = TraceEnd;
	 		}
	 	}
	}
	else
	{
		// need to trace to validate

		// sort potential solutions by priority
		float PrioritizedSolutionsMagXYSq[2];
		PrioritizedSolutionsMagXYSq[0] = bFavorHighArc ? FMath::Min(MagXYSq_A, MagXYSq_B) : FMath::Max(MagXYSq_A, MagXYSq_B);
		PrioritizedSolutionsMagXYSq[1] = bFavorHighArc ? FMath::Max(MagXYSq_A, MagXYSq_B) : FMath::Min(MagXYSq_A, MagXYSq_B);

		FVector PrioritizedProjVelocities[2];

		// try solutions in priority order
		int32 ValidSolutionIdx = INDEX_NONE;
		for (int32 CurrentSolutionIdx=0; (CurrentSolutionIdx<2); ++CurrentSolutionIdx)
		{
			const float MagXY = FMath::Sqrt( PrioritizedSolutionsMagXYSq[CurrentSolutionIdx] );
			const float MagZ = FMath::Sqrt( TossSpeedSq - PrioritizedSolutionsMagXYSq[CurrentSolutionIdx] );		// pythagorean

			PrioritizedProjVelocities[CurrentSolutionIdx] = (DirXY * MagXY) + (FVector::UpVector * MagZ);

			// iterate along the arc, doing stepwise traces
			bool bFailedTrace = false;
			static const float StepSize = 0.125f;
			FVector TraceStart = Start;
			for ( float Step=0.f; Step<1.f; Step+=StepSize )
			{
				const float TimeInFlight = (Step+StepSize) * DeltaXY/MagXY;

				// d = vt + .5 a t^2
				const FVector TraceEnd = Start + PrioritizedProjVelocities[CurrentSolutionIdx]*TimeInFlight + FVector(0.f, 0.f, 0.5f * GravityZ * FMath::Square(TimeInFlight) - CollisionRadius);

				if ( (TraceOption == ESuggestProjVelocityTraceOption::OnlyTraceWhileAsceding) && (TraceEnd.Z < TraceStart.Z) )
				{
					// falling, we are done tracing
					if (!bDrawDebug)
					{
						// if we're drawing, we continue stepping without the traces
						// else we can just trivially end the iteration loop
						break;
					}
				}
				else
				{
					// note: this will automatically fall back to line test if radius is small enough
					static const FName NAME_SuggestProjVelTrace = FName(TEXT("SuggestProjVelTrace"));
					if ( World->SweepTest( TraceStart, TraceEnd, FQuat::Identity, FCollisionShape::MakeSphere(CollisionRadius), FCollisionQueryParams(NAME_SuggestProjVelTrace, true), FCollisionObjectQueryParams(ECC_WorldStatic) ) )
					{
						// hit something, failed
						bFailedTrace = true;

						if (bDrawDebug)
						{
							// draw failed segment in red
							DrawDebugLine( World, TraceStart, TraceEnd, FColor::Red, true );
						}

						break;
					}

				}

				if (bDrawDebug)
				{
					DrawDebugLine( World, TraceStart, TraceEnd, FColor::Yellow, true );
				}

				// advance
				TraceStart = TraceEnd;
			}

			if (bFailedTrace == false)
			{
				// passes all traces along the arc, we have a valid solution and can be done
				ValidSolutionIdx = CurrentSolutionIdx;
				break;
			}
		}

		if (ValidSolutionIdx != INDEX_NONE)
		{
			OutTossVelocity = PrioritizedProjVelocities[ValidSolutionIdx];
			bFoundAValidSolution = true;
		}
	}

	return bFoundAValidSolution;
}