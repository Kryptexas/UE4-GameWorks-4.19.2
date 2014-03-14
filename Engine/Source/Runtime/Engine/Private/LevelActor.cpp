// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
#include "EngineLevelScriptClasses.h"
#include "EngineKismetLibraryClasses.h"
#include "Collision.h"

#if WITH_PHYSX
	#include "PhysicsEngine/PhysXSupport.h"
#endif // WITH_PHYSX


#if LINE_CHECK_TRACING

/** Is tracking enabled */
int32 LineCheckTracker::bIsTrackingEnabled = false;
/** If this count is nonzero, dump the log when we exceed this number in any given frame */
int32 LineCheckTracker::TraceCountForSpikeDump = 0;
/** Number of traces recorded this frame */
int32 LineCheckTracker::CurrentCountForSpike = 0;

FStackTracker* LineCheckTracker::LineCheckStackTracker = NULL;
FScriptStackTracker* LineCheckTracker::LineCheckScriptStackTracker = NULL;

/** Updates an existing call stack trace with new data for this particular call*/
static void LineCheckUpdateFn(const FStackTracker::FCallStack& CallStack, void* UserData)
{
	if (UserData)
	{
		//Callstack has been called more than once, aggregate the data
		LineCheckTracker::FLineCheckData* NewLCData = static_cast<LineCheckTracker::FLineCheckData*>(UserData);
		LineCheckTracker::FLineCheckData* OldLCData = static_cast<LineCheckTracker::FLineCheckData*>(CallStack.UserData);

		OldLCData->Flags |= NewLCData->Flags;
		OldLCData->IsNonZeroExtent |= NewLCData->IsNonZeroExtent;

		if (NewLCData->LineCheckObjsMap.Num() > 0)
		{
			for (TMap<const FName, LineCheckTracker::FLineCheckData::LineCheckObj>::TConstIterator It(NewLCData->LineCheckObjsMap); It; ++It)
			{
				const LineCheckTracker::FLineCheckData::LineCheckObj &NewObj = It.Value();

				LineCheckTracker::FLineCheckData::LineCheckObj * OldObj = OldLCData->LineCheckObjsMap.Find(NewObj.ObjectName);
				if (OldObj)
				{
					OldObj->Count += NewObj.Count;
				}
				else
				{
					OldLCData->LineCheckObjsMap.Add(NewObj.ObjectName, NewObj);
				}
			}
		}
	}
}

/** After the stack tracker reports a given stack trace, it calls this function
*  which appends data particular to line checks
*/
static void LineCheckReportFn(const FStackTracker::FCallStack& CallStack, uint64 TotalStackCount, FOutputDevice& Ar)
{
	//Output to a csv file any relevant data
	LineCheckTracker::FLineCheckData* const LCData = static_cast<LineCheckTracker::FLineCheckData*>(CallStack.UserData);
	if (LCData)
	{
		FString UserOutput = LINE_TERMINATOR TEXT(",,,");
		UserOutput += (LCData->IsNonZeroExtent ? TEXT( "NonZeroExtent") : TEXT("ZeroExtent"));

		for (TMap<const FName, LineCheckTracker::FLineCheckData::LineCheckObj>::TConstIterator It(LCData->LineCheckObjsMap); It; ++It)
		{
			UserOutput += LINE_TERMINATOR TEXT(",,,");
			const LineCheckTracker::FLineCheckData::LineCheckObj &CurObj = It.Value();
			UserOutput += FString::Printf(TEXT("%s (%d) : %s"), *CurObj.ObjectName.ToString(), CurObj.Count, *CurObj.DetailedInfo);
		}

		UserOutput += LINE_TERMINATOR TEXT(",,,");
		
		Ar.Log(*UserOutput);
	}
}

/** Called at the beginning of each frame to check/reset spike count */
void LineCheckTracker::Tick()
{
	if(bIsTrackingEnabled && LineCheckStackTracker)
	{
		//Spike logging is enabled
		if (TraceCountForSpikeDump > 0)
		{
			//Dump if we exceeded the threshold this frame
			if (CurrentCountForSpike > TraceCountForSpikeDump)
			{
				DumpLineChecks(5);
			}
			//Reset for next frame
			ResetLineChecks();
		}

		CurrentCountForSpike = 0;
	}
}

/** Set the value which, if exceeded, will cause a dump of the line checks this frame */
void LineCheckTracker::SetSpikeMinTraceCount(int32 MinTraceCount)
{
	TraceCountForSpikeDump = FMath::Max(0, MinTraceCount);
	UE_LOG(LogSpawn, Log, TEXT("Line trace spike count is %d."), TraceCountForSpikeDump);
}

/** Dump out the results of all line checks called in the game since the last call to ResetLineChecks() */
void LineCheckTracker::DumpLineChecks(int32 Threshold)
{
	if( LineCheckStackTracker )
	{
		const FString Filename = FString::Printf(TEXT("%sLineCheckLog-%s.csv"), *FPaths::GameLogDir(), *FDateTime::Now().ToString());
		FOutputDeviceFile OutputFile(*Filename);
		LineCheckStackTracker->DumpStackTraces( Threshold, OutputFile );
		OutputFile.TearDown();
	}

	if( LineCheckScriptStackTracker )
	{
		const FString Filename = FString::Printf(TEXT("%sScriptLineCheckLog-%s.csv"), *FPaths::GameLogDir(), *FDateTime::Now().ToString());
		FOutputDeviceFile OutputFile(*Filename);
		LineCheckScriptStackTracker->DumpStackTraces( Threshold, OutputFile );
		OutputFile.TearDown();
	}
}

/** Reset the line check stack tracker (calls FMemory::Free() on all user data pointers)*/
void LineCheckTracker::ResetLineChecks()
{
	if( LineCheckStackTracker )
	{
		LineCheckStackTracker->ResetTracking();
	}

	if( LineCheckScriptStackTracker )
	{
		LineCheckScriptStackTracker->ResetTracking();
	}
}

/** Turn line check stack traces on and off, does not reset the actual data */
void LineCheckTracker::ToggleLineChecks()
{
	bIsTrackingEnabled = !bIsTrackingEnabled;
	UE_LOG(LogSpawn, Log, TEXT("Line tracing is now %s."), bIsTrackingEnabled ? TEXT("enabled") : TEXT("disabled"));
	
	CurrentCountForSpike = 0;
	if (LineCheckStackTracker == NULL)
	{
		FPlatformStackWalk::InitStackWalking();
		LineCheckStackTracker = new FStackTracker(LineCheckUpdateFn, LineCheckReportFn);
	}

	if (LineCheckScriptStackTracker == NULL)
	{
		LineCheckScriptStackTracker = new FScriptStackTracker();
	}

	LineCheckStackTracker->ToggleTracking();
	LineCheckScriptStackTracker->ToggleTracking();
}

/** Captures a single stack trace for a line check */
void LineCheckTracker::CaptureLineCheck(int32 LineCheckFlags, const FVector* Extent, const FFrame* ScriptStackFrame, const UObject * Object)
{
	if (LineCheckStackTracker == NULL || LineCheckScriptStackTracker == NULL)
	{
		return;
	}

	if (ScriptStackFrame)
	{
		int32 EntriesToIgnore = 0;
		LineCheckScriptStackTracker->CaptureStackTrace(ScriptStackFrame, EntriesToIgnore);
	}
	else
	{		   
		FLineCheckData* const LCData = static_cast<FLineCheckData*>(FMemory::Malloc(sizeof(FLineCheckData)));
		FMemory::Memset(LCData, 0, sizeof(FLineCheckData));
		LCData->Flags = LineCheckFlags;
		LCData->IsNonZeroExtent = (Extent && !Extent->IsZero()) ? true : false;
		FLineCheckData::LineCheckObj LCObj;
		if (Object)
		{
			LCObj = FLineCheckData::LineCheckObj(Object->GetFName(), 1, Object->GetDetailedInfo());
		}
		else
		{
			LCObj = FLineCheckData::LineCheckObj(NAME_None, 1, TEXT("Unknown"));
		}
		
		LCData->LineCheckObjsMap.Add(LCObj.ObjectName, LCObj);		

		int32 EntriesToIgnore = 3;
		LineCheckStackTracker->CaptureStackTrace(EntriesToIgnore, static_cast<void*>(LCData));
		//Only increment here because execTrace() will lead to here also
		CurrentCountForSpike++;
	}
}
#endif //LINE_CHECK_TRACING

/*-----------------------------------------------------------------------------
	Level actor management.
-----------------------------------------------------------------------------*/
// LOOKING_FOR_PERF_ISSUES
#define PERF_SHOW_MULTI_PAWN_SPAWN_FRAMES ((1 && !(UE_BUILD_SHIPPING || UE_BUILD_TEST)) || !WITH_EDITORONLY_DATA)

#if PERF_SHOW_MULTI_PAWN_SPAWN_FRAMES
	/** Array showing names of pawns spawned this frame. */
	TArray<FString>	ThisFramePawnSpawns;
#endif	//PERF_SHOW_MULTI_PAWN_SPAWN_FRAMES

AActor* UWorld::SpawnActor( UClass* Class, FVector const* Location, FRotator const* Rotation, const FActorSpawnParameters& SpawnParameters )
{
	SCOPE_CYCLE_COUNTER(STAT_SpawnActorTime);
	check( CurrentLevel ); 	
	check(GIsEditor || (CurrentLevel == PersistentLevel));

	// Make sure this class is spawnable.
	if( !Class )
	{
		UE_LOG(LogSpawn, Warning, TEXT("SpawnActor failed because no class was specified") );
		return NULL;
	}
	if( Class->HasAnyClassFlags(CLASS_Deprecated) )
	{
		UE_LOG(LogSpawn, Warning, TEXT("SpawnActor failed because class %s is deprecated"), *Class->GetName() );
		return NULL;
	}
	if( Class->HasAnyClassFlags(CLASS_Abstract) )
	{
		UE_LOG(LogSpawn, Warning, TEXT("SpawnActor failed because class %s is abstract"), *Class->GetName() );
		return NULL;
	}
	else if( !Class->IsChildOf(AActor::StaticClass()) )
	{
		UE_LOG(LogSpawn, Warning, TEXT("SpawnActor failed because %s is not an actor class"), *Class->GetName() );
		return NULL;
	}
	else if (SpawnParameters.Template != NULL && SpawnParameters.Template->GetClass() != Class)
	{
		UE_LOG(LogSpawn, Warning, TEXT("SpawnActor failed because template class (%s) does not match spawn class (%s)"), *SpawnParameters.Template->GetClass()->GetName(), *Class->GetName());
		if (!SpawnParameters.bNoFail)
		{
			return NULL;
		}
	}
	else if (bIsRunningConstructionScript && !SpawnParameters.bAllowDuringConstructionScript)
	{
		UE_LOG(LogSpawn, Warning, TEXT("SpawnActor failed because we are running a ConstructionScript (%s)"), *Class->GetName() );
		return NULL;
	}

	AActor* Template = SpawnParameters.Template;
	// Use class's default actor as a template.
	if( !Template )
	{
		Template = Class->GetDefaultObject<AActor>();
	}
	check(Template!=NULL);

	// See if we can spawn on ded.server/client only etc (check NeedsLoadForClient & NeedsLoadForServer)
	if(!CanCreateInCurrentContext(Template))
	{
		UE_LOG(LogSpawn, Log, TEXT("Unable to spawn class '%s' due to client/server context."), *Class->GetName() );
		return NULL;
	}

	FVector NewLocation = Location ? *Location : (Template->GetRootComponent() ? Template->GetRootComponent()->RelativeLocation : FVector::ZeroVector);
	FRotator NewRotation = Rotation ? *Rotation :	(Template->GetRootComponent() ? Template->GetRootComponent()->RelativeRotation : FRotator::ZeroRotator);

	// If we can fail, and we have a root component, and either the component should collide or bCollideWhenPlacing is true, test
	if( !SpawnParameters.bNoCollisionFail && Template->GetRootComponent() && Template->bCollideWhenPlacing && Template->GetRootComponent()->ShouldCollideWhenPlacing() )
	{
		// Try to find a spawn position
		if ( !FindTeleportSpot(Template, NewLocation, NewRotation) )
		{
			UE_LOG(LogSpawn, Log, TEXT("SpawnActor failed because of collision at the spawn location [%s] for [%s]"), *NewLocation.ToString(), *Class->GetName() );
			return NULL;
		}
	}

	ULevel* LevelToSpawnIn = SpawnParameters.OverrideLevel;
	if (LevelToSpawnIn == NULL)
	{
		// Spawn in the same level as the owner if we have one. @warning: this relies on the outer of an actor being the level.
		LevelToSpawnIn = (SpawnParameters.Owner != NULL) ? CastChecked<ULevel>(SpawnParameters.Owner->GetOuter()) : CurrentLevel;
	}
	AActor* Actor = ConstructObject<AActor>( Class, LevelToSpawnIn, SpawnParameters.Name, SpawnParameters.ObjectFlags, Template );
	check(Actor);

#if WITH_EDITOR
	Actor->ClearActorLabel(); // Clear label on newly spawned actors
#endif // WITH_EDITOR

	if ( GUndo )
	{
		ModifyLevel( LevelToSpawnIn );
	}
	LevelToSpawnIn->Actors.Add( Actor );

	// Add this newly spawned actor to the network actor list
	AddNetworkActor( Actor );

	// Broadcast notification of spawn
	OnActorSpawned.Broadcast( Actor );

	if( GIsEditor )
	{
		GEngine->BroadcastLevelActorAdded(Actor);
	}

#if PERF_SHOW_MULTI_PAWN_SPAWN_FRAMES
	if( Cast<APawn>(Actor) )
	{
		FString PawnName = FString::Printf(TEXT("%d: %s"), ThisFramePawnSpawns.Num(), *Actor->GetPathName());
		ThisFramePawnSpawns.Add(PawnName);
	}
#endif

	Actor->PostSpawnInitialize(NewLocation, NewRotation, SpawnParameters.Owner, SpawnParameters.Instigator, SpawnParameters.bRemoteOwned, SpawnParameters.bNoFail, SpawnParameters.bDeferConstruction);

	if (Actor->IsPendingKill() && !SpawnParameters.bNoFail)
	{
		return NULL;
	}
	Actor->CheckDefaultSubobjects();
	return Actor;
}


ABrush* UWorld::SpawnBrush()
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.bNoCollisionFail = true;
	ABrush* Result = SpawnActor<ABrush>( SpawnInfo );
	check(Result);
	return Result;
}

/**
 * Wrapper for DestroyActor() that should be called in the editor.
 *
 * @param	bShouldModifyLevel		If true, Modify() the level before removing the actor.
 */
bool UWorld::EditorDestroyActor( AActor* ThisActor, bool bShouldModifyLevel )
{
#if !WITH_RUNTIME_NAVIGATION_BUILDING
	// @NOTE: this piece needs to be here to have in-editor navmesh react to 
	// actors being deleted, while not doing so in pure game mode.
	if (ThisActor->GetWorld() != NULL && ThisActor->GetWorld()->GetNavigationSystem() != NULL)
	{
		ThisActor->GetWorld()->GetNavigationSystem()->UnregisterNavigationRelevantActor(ThisActor);
	}
#endif // !WITH_RUNTIME_NAVIGATION_BUILDING
	bool bReturnValue = DestroyActor( ThisActor, false, bShouldModifyLevel );
	ThisActor->GetWorld()->BroadcastLevelsChanged();
	return bReturnValue;
}

/**
 * Removes the actor from its level's actor list and generally cleans up the engine's internal state.
 * What this function does not do, but is handled via garbage collection instead, is remove references
 * to this actor from all other actors, and kill the actor's resources.  This function is set up so that
 * no problems occur even if the actor is being destroyed inside its recursion stack.
 *
 * @param	ThisActor				Actor to remove.
 * @param	bNetForce				[opt] Ignored unless called during play.  Default is false.
 * @param	bShouldModifyLevel		[opt] If true, Modify() the level before removing the actor.  Default is true.
 * @return							true if destroy, false if actor couldn't be destroyed.
 */
bool UWorld::DestroyActor( AActor* ThisActor, bool bNetForce, bool bShouldModifyLevel )
{
	check(ThisActor);
	check(ThisActor->IsValidLowLevel());
	//UE_LOG(LogSpawn, Log,  "Destroy %s", *ThisActor->GetClass()->GetName() );

#if WITH_RUNTIME_NAVIGATION_BUILDING
	if (ThisActor->GetWorld() != NULL && ThisActor->GetWorld()->GetNavigationSystem() != NULL)
	{
		ThisActor->GetWorld()->GetNavigationSystem()->UnregisterNavigationRelevantActor(ThisActor);
	}
#endif // WITH_RUNTIME_NAVIGATION_BUILDING
	if (ThisActor->GetWorld() == NULL)
	{
		UE_LOG(LogSpawn, Warning, TEXT("Destroying %s, which doesn't have a valid world pointer"), *ThisActor->GetPathName());
	}

	// In-game deletion rules.
	if( HasBegunPlay() )
	{
		// Never destroy the world settings actor. This used to be enforced by bNoDelete and is actually needed for 
		// seamless travel and network games.
		if (GetWorldSettings() == ThisActor)
		{
			return false;
		}

		// If already on list to be deleted, pretend the call was successful.
		if( ThisActor->IsPendingKill() )
		{
			return true;
		}

		// Can't kill if wrong role.
		if( ThisActor->Role!=ROLE_Authority && !bNetForce && !ThisActor->bNetTemporary )
		{
			return false;
		}

		// Don't destroy player actors.
		APlayerController* PC = Cast<APlayerController>(ThisActor);
		if ( PC )
		{
			UNetConnection* C = Cast<UNetConnection>(PC->Player);
			if( C )
			{	
				if( C->Channels[0] && C->State!=USOCK_Closed )
				{
					C->bPendingDestroy = true;
					C->Channels[0]->Close();
				}
				return false;
			}
		}
	}
	else
	{
		ThisActor->Modify();
	}
	ThisActor->bPendingKillPending = true;

	// Notify the texture streaming manager about the destruction of this actor.
	GStreamingManager->NotifyActorDestroyed( ThisActor );

	// Tell this actor it's about to be destroyed.
	ThisActor->Destroyed();

	// Detach this actor's children
	TArray<AActor*> AttachedActors;
	ThisActor->GetAttachedActors(AttachedActors);

	TArray<USceneComponent*> SceneComponents;
	ThisActor->GetComponents(SceneComponents);

	if (AttachedActors.Num() > 0)
	{

		for (TArray< AActor* >::TConstIterator AttachedActorIt(AttachedActors); AttachedActorIt; ++AttachedActorIt)
		{
			AActor* ChildActor = *AttachedActorIt;
			if (ChildActor != NULL)
			{
				for (auto SceneCompIter = SceneComponents.CreateIterator(); SceneCompIter; ++SceneCompIter)
				{
					ChildActor->DetachSceneComponentsFromParent(*SceneCompIter, true);
				}

				if( GIsEditor )
				{
					GEngine->BroadcastLevelActorDetached(ChildActor, ThisActor);
				}
			}
		}
	}

	// Detach from anything we were attached to
	USceneComponent* RootComp = ThisActor->GetRootComponent();
	if( RootComp != NULL && RootComp->AttachParent != NULL)
	{
		AActor* OldParentActor = RootComp->AttachParent->GetOwner();
		if (OldParentActor)
		{
			OldParentActor->Modify();
		}

		ThisActor->DetachRootComponentFromParent(false);

		if( GIsEditor )
		{
			GEngine->BroadcastLevelActorDetached(ThisActor, OldParentActor);
		}
	}

	if( ThisActor->IsPendingKill() )
	{
		return true;
	}

	// Remove owned components from overlap tracking
	// We don't traverse the RootComponent attachment tree since that might contain
	// components owned by other actors.
	for (int32 Index = 0; Index < SceneComponents.Num(); ++Index)
	{
		UPrimitiveComponent* const PrimComp = Cast<UPrimitiveComponent>(SceneComponents[Index]);
		if (PrimComp)
		{
			TArray<UPrimitiveComponent*> OverlappingComponents;
			PrimComp->GetOverlappingComponents(OverlappingComponents);

			for (auto OverlapIt = OverlappingComponents.CreateIterator(); OverlapIt; ++OverlapIt)
			{
				UPrimitiveComponent* const OverlapComp = *OverlapIt;
				if (OverlapComp)
				{
					PrimComp->EndComponentOverlap(OverlapComp, true, true);

					if (ThisActor->IsPendingKill())
					{
						return true;
					}
				}
			}
		}
	}

	// If this actor has an owner, notify it that it has lost a child.
	if( ThisActor->GetOwner() )
	{
		ThisActor->SetOwner(NULL);
		if( ThisActor->IsPendingKill() )
		{
			return true;
		}
	}
	// Notify net players that this guy has been destroyed.
	if( NetDriver )
	{
		NetDriver->NotifyActorDestroyed( ThisActor );
	}

	// Remove the actor from the actor list.
	RemoveActor( ThisActor, bShouldModifyLevel );
	
	// Clean up the actor's components.
	ThisActor->UnregisterAllComponents();

	// Mark the actor and its direct components as pending kill.
	ThisActor->MarkPendingKill();
	ThisActor->MarkPackageDirty();
	ThisActor->MarkComponentsAsPendingKill();

	// Unregister the actor's tick function
	const bool bRegisterTickFunctions = false;
	const bool bIncludeComponents = true;
	ThisActor->RegisterAllActorTickFunctions(bRegisterTickFunctions, bIncludeComponents);

	// Invalidate the lighting cache in the Editor.  We need to check for GIsEditor as play has not begun in network game and objects get destroyed on switching levels
	if( GIsEditor )
	{
		ThisActor->InvalidateLightingCache();
		GEngine->BroadcastLevelActorDeleted(ThisActor);
	}

	// Return success.
	return true;
}

/*-----------------------------------------------------------------------------
	Player spawning.
-----------------------------------------------------------------------------*/

APlayerController* UWorld::SpawnPlayActor(UPlayer* Player, ENetRole RemoteRole, const FURL& InURL, const TSharedPtr<FUniqueNetId>& UniqueId, FString& Error, uint8 InNetPlayerIndex)
{
	Error = TEXT("");

	// Make the option string.
	FString Options;
	for (int32 i = 0; i < InURL.Op.Num(); i++)
	{
		Options += TEXT('?');
		Options += InURL.Op[i];
	}

	// Give the GameMode a chance to accept the login
	APlayerController* const Actor = GetAuthGameMode()->Login(*InURL.Portal, Options, UniqueId, Error);
	if (Actor == NULL)
	{
		UE_LOG(LogSpawn, Warning, TEXT("Login failed: %s"), *Error);
		return NULL;
	}

	// Possess the newly-spawned player.
	Actor->NetPlayerIndex = InNetPlayerIndex;
	Actor->SetPlayer(Player);
	//UE_LOG(LogSpawn, Log, TEXT("%s got player %s"), *Actor->GetName(), *Player->GetName());
	Actor->Role = ROLE_Authority;
	Actor->SetReplicates(RemoteRole != ROLE_None);
	if (RemoteRole == ROLE_AutonomousProxy)
	{
		Actor->SetAutonomousProxy(true);
	}
	GetAuthGameMode()->PostLogin(Actor);

	return Actor;
}

/*-----------------------------------------------------------------------------
	Level actor moving/placing.
-----------------------------------------------------------------------------*/

bool UWorld::FindTeleportSpot(AActor* TestActor, FVector& TestLocation, FRotator TestRotation)
{
	if( !TestActor || !TestActor->GetRootComponent() )
	{
		return true;
	}
	FVector Adjust(0.f);

	// check if fits at desired location
	if( !EncroachingBlockingGeometry(TestActor, TestLocation, TestRotation, &Adjust) )
	{
		return true;
	}

	// first do only Z
	if ( Adjust.Z != 0.f )
	{
		TestLocation.Z += Adjust.Z;
		if( !EncroachingBlockingGeometry(TestActor, TestLocation, TestRotation, &Adjust) )
		{
			return true;
		}
	}

	// now try just XY
	if ( (Adjust.X != 0.f) || (Adjust.Y != 0.f) )
	{
		TestLocation.X += Adjust.X;
		TestLocation.Y += Adjust.Y;
		if( !EncroachingBlockingGeometry(TestActor, TestLocation, TestRotation, &Adjust) )
		{
			return true;
		}
	}

	// now z again
	if ( Adjust.Z != 0.f )
	{
		TestLocation.Z += Adjust.Z;
		if( !EncroachingBlockingGeometry(TestActor, TestLocation, TestRotation, &Adjust) )
		{
			return true;
		}
	}
	if ( Adjust.IsZero() )
	{
		return false;
	}

	// Now try full adjustment
	TestLocation += Adjust;
	//DrawDebugSphere(this, TestLocation, 32, 10, FLinearColor::Blue, true);
	return !EncroachingBlockingGeometry(TestActor, TestLocation, TestRotation, &Adjust);
}

bool UWorld::EncroachingBlockingGeometry(AActor* TestActor, FVector TestLocation, FRotator TestRotation, FVector* ProposedAdjustment )
{
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("p.EncroachEpsilon"));
	float Epsilon = CVar ? CVar->GetValueOnGameThread() : 0.f;

	// currently just tests RootComponent.  @TODO FIXME should test all colliding components?  Cost/benefit?
	UPrimitiveComponent* PrimComp = TestActor ? Cast<UPrimitiveComponent>(TestActor->GetRootComponent()) : NULL;
	bool bFoundBlockingHit = false;
	if (ProposedAdjustment)
	{
		*ProposedAdjustment = FVector::ZeroVector;
	}
	if ( PrimComp )
	{
		TArray<FOverlapResult> Overlaps;
		static FName NAME_EncroachingBlockingGeometry = FName(TEXT("EncroachingBlockingGeometry"));
		ECollisionChannel BlockingChannel = PrimComp->GetCollisionObjectType();

		// @TODO support more than just capsules for spawning, don't yet have - add primitive component function to calculate these, use boundingbox as fallback
		UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(PrimComp);

		if ( Capsule )//&& (Capsule->BodyInstance.GetPxRigidActor() == NULL) )
		{
			FCollisionQueryParams Params(NAME_EncroachingBlockingGeometry, false, TestActor);
			bFoundBlockingHit = OverlapMulti(Overlaps, TestLocation, FQuat::Identity, BlockingChannel, FCollisionShape::MakeCapsule(FMath::Max(Capsule->GetScaledCapsuleRadius() - Epsilon, 0.1f), FMath::Max(Capsule->GetScaledCapsuleHalfHeight() - Epsilon, 0.1f)), Params);
		}
		else
		{
			if(ensureMsgf(PrimComp->IsRegistered(), TEXT("Components must be registered in order to be used in a ComponentOverlapMulti call. PriComp: %s TestActor: %s"), *PrimComp->GetName(), *TestActor->GetName()))
			{
				FComponentQueryParams Params(NAME_EncroachingBlockingGeometry, TestActor);
				bFoundBlockingHit = ComponentOverlapMulti(Overlaps, PrimComp, TestLocation, TestActor->GetActorRotation(), Params);
			}
		}
		for( int32 HitIdx = 0; HitIdx < Overlaps.Num(); HitIdx++ )
		{
			if ( Overlaps[HitIdx].Component.IsValid() && (Overlaps[HitIdx].Component.Get()->GetCollisionResponseToChannel(BlockingChannel) == ECR_Block) )
			{
				bFoundBlockingHit = true;
				break;
			}
		}
		if ( !bFoundBlockingHit )
		{
			return false;
		}

		if ( ProposedAdjustment && Capsule )
		{
			// if encroaching, propose adjustment based on overlaps, treat PrimComp as box
			float CapsuleRadius, CapsuleHalfHeight;
			Capsule->GetScaledCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
			float XRadius = CapsuleRadius;
			float YRadius = CapsuleRadius;
			float ZHeight = CapsuleHalfHeight;
			float ClosestXPos = XRadius;
			float ClosestXNeg = -1.f*ClosestXPos;
			float ClosestYPos = YRadius;
			float ClosestYNeg = -1.f*ClosestYPos;
			float ClosestZPos = ZHeight;
			float ClosestZNeg = -1.f*ClosestZPos;
			float ExtraZAdjust = 0.f;
			for( int32 HitIdx = 0; HitIdx < Overlaps.Num(); HitIdx++ )
			{
				FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
		
				// first determine closest impact point along each axis
				if ( Overlaps[HitIdx].Component.IsValid()  && (Overlaps[HitIdx].Component.Get()->GetCollisionResponseToChannel(BlockingChannel) == ECR_Block) )
				{
					// @TODO - really want to use MTD rather than nearest overlap point once that is exposed
// 					FVector OverlapPoint;
// 					float OverlapDist = Overlaps[HitIdx].Component->GetDistanceToCollision(TestLocation, OverlapPoint);
// 					FVector Offset = OverlapPoint - TestLocation;
					FVector OverlapComponentLoc = Overlaps[HitIdx].Component->GetComponentToWorld().GetLocation();
					FVector Offset;
					FHitResult HitResult;
					if (Overlaps[HitIdx].Component->SweepComponent(HitResult, TestLocation, OverlapComponentLoc, CapsuleShape, true))
					{
						// we have blocking hit
						Offset = HitResult.ImpactPoint - TestLocation;
					}
					else
					{
						// we didnt' hit anything, so offset = 0.f;? Seems it should not happen?
						Offset = FVector::ZeroVector;
					}
					
					//DrawDebugSphere(this, OverlapPoint, 20, 10, FLinearColor::White, true);
					if ( Offset.X >= 0.f )
					{
						ClosestXPos = FMath::Min(Offset.X, ClosestXPos);
					}
					else
					{
						ClosestXNeg = FMath::Max(Offset.X, ClosestXNeg);
					}

					if ( Offset.Y >= 0.f )
					{
						ClosestYPos = FMath::Min(Offset.Y, ClosestYPos);
					}
					else
					{
						ClosestYNeg = FMath::Max(Offset.Y, ClosestYNeg);
					}

					if ( Offset.Z >= 0.f )
					{
						ClosestZPos = FMath::Min(Offset.Z, ClosestZPos);
					}
					else
					{
						ClosestZNeg = FMath::Max(Offset.Z, ClosestZNeg);
					}
				}
			}

			if ( ClosestXPos >= XRadius )
			{
				ProposedAdjustment->X = -1.f*(XRadius+ClosestXNeg);
			}
			else if ( ClosestXNeg <= -1.f*XRadius )
			{
				ProposedAdjustment->X = XRadius - ClosestXPos;
			}
			else
			{
				// overlaps on both sides of X axis - assume overlaps are low, push adjustment up
				ExtraZAdjust = FMath::Max(XRadius - ClosestXPos, XRadius+ClosestXNeg);
			}

			if ( ClosestYPos >= YRadius )
			{
				ProposedAdjustment->Y = -1.f*(YRadius+ClosestYNeg);
			}
			else if ( ClosestYNeg <= -1.f*YRadius )
			{
				ProposedAdjustment->Y = YRadius - ClosestYPos;
			}
			else
			{
				// overlaps on both sides of X axis - assume overlaps are low, push adjustment up
				ExtraZAdjust = FMath::Max(YRadius - ClosestYPos, YRadius+ClosestYNeg);
			}

			if ( ClosestZPos >= ZHeight || (FMath::Abs(ClosestZPos) < KINDA_SMALL_NUMBER) )
			{
				ProposedAdjustment->Z = -1.f*(ZHeight+ClosestZNeg);
			}
			else if ( ClosestZNeg <= -1.f*ZHeight )
			{
				ProposedAdjustment->Z = ZHeight - ClosestZPos + ExtraZAdjust;
			}

			*ProposedAdjustment *= -1.f;
		}
	}
	return bFoundBlockingHit;
}


void UWorld::LoadSecondaryLevels(bool bForce, TSet<FString>* CookedPackages)
{
	check( GIsEditor );

	// streamingServer
	// Only load secondary levels in the Editor, and not for commandlets.
	if( (!IsRunningCommandlet() || bForce)
	// Don't do any work for world info actors that are part of secondary levels being streamed in! 
	&&	!GIsAsyncLoading )
	{
		for( int32 LevelIndex=0; LevelIndex<StreamingLevels.Num(); LevelIndex++ )
		{
			bool bLoadedLevelPackage = false;
			ULevelStreaming* const StreamingLevel = StreamingLevels[LevelIndex];
			if( StreamingLevel )
			{
				// If we are cooking don't cook sub levels multiple times if they've already been cooked
				FString PackageFilename;
				if (CookedPackages)
				{
					if (FPackageName::DoesPackageExist(StreamingLevel->PackageName.ToString(), NULL, &PackageFilename))
					{
						PackageFilename = FPaths::ConvertRelativePathToFull(PackageFilename);
					}
				}
				if (CookedPackages == NULL || !CookedPackages->Contains(PackageFilename))
				{
					// Load the package and find the world object.
					if( FPackageName::IsShortPackageName(StreamingLevel->PackageName) == false )
					{
						ULevel::StreamedLevelsOwningWorld.Add(StreamingLevel->PackageName, this);
						UPackage* const LevelPackage = LoadPackage( NULL, *StreamingLevel->PackageName.ToString(), LOAD_None );
					
						if( LevelPackage )
						{
							bLoadedLevelPackage = true;

							// Find the world object in the loaded package.
							UWorld* const LoadedWorld	= UWorld::FindWorldInPackage(LevelPackage);
							check(LoadedWorld);

							// LoadedWorld won't be serialized as there's a BeginLoad on the stack so we manually serialize it here.
							check( LoadedWorld->GetLinker() );
							LoadedWorld->GetLinker()->Preload( LoadedWorld );

							// Keep reference to prevent garbage collection.
							check( LoadedWorld->PersistentLevel );
						
							ULevel* NewLoadedLevel = LoadedWorld->PersistentLevel;
							NewLoadedLevel->OwningWorld = this;
					
							StreamingLevel->SetLoadedLevel(NewLoadedLevel);
						}
					}
					else
					{
						UE_LOG(LogSpawn, Warning, TEXT("Streaming level uses short package name (%s). Level will not be loaded."), *StreamingLevel->PackageName.ToString());
					}

					// Remove this level object if the file couldn't be found.
					if ( !bLoadedLevelPackage )
					{		
						StreamingLevels.RemoveAt( LevelIndex-- );
						MarkPackageDirty();
					}
				}
			}
		}
	}
}

/** Utility for returning the ULevelStreaming object for a particular sub-level, specified by package name */
ULevelStreaming* UWorld::GetLevelStreamingForPackageName(FName InPackageName)
{
	// iterate over each level streaming object
	for( int32 LevelIndex=0; LevelIndex<StreamingLevels.Num(); LevelIndex++ )
	{
		ULevelStreaming* LevelStreaming = StreamingLevels[LevelIndex];
		// see if name matches
		if(LevelStreaming && LevelStreaming->PackageName == InPackageName)
		{
			// it doesn, return this one
			return LevelStreaming;
		}
	}

	// failed to find one
	return NULL;
}

#if WITH_EDITOR


void UWorld::RefreshStreamingLevels( const TArray<class ULevelStreaming*>& InLevelsToRefresh )
{
	// Reassociate levels in case we changed streaming behavior. Editor-only!
	if( GIsEditor )
	{
		// Load and associate levels if necessary.
		FlushLevelStreaming();

		// Remove all currently visible levels.
		for( int32 LevelIndex=0; LevelIndex<InLevelsToRefresh.Num(); LevelIndex++ )
		{
			ULevelStreaming* StreamingLevel = InLevelsToRefresh[LevelIndex];
			ULevel* LoadedLevel = StreamingLevel->GetLoadedLevel();

			if( StreamingLevel 
				&&	LoadedLevel 
				&&	LoadedLevel->bIsVisible )
			{
				RemoveFromWorld( LoadedLevel );
			}
		}

		// Load and associate levels if necessary.
		FlushLevelStreaming();

		// Update the level browser so it always contains valid data
		FEditorSupportDelegates::WorldChange.Broadcast();
	}
}

void UWorld::RefreshStreamingLevels()
{
	RefreshStreamingLevels( StreamingLevels );
}
#endif // WITH_EDITOR


/**
 * Finds the reverb settings to use for a given view location, taking into account the world's default
 * settings and the reverb volumes in the world.
 *
 * @param	ViewLocation			Current view location.
 * @param	OutReverbSettings		[out] Upon return, the reverb settings for a camera at ViewLocation.
 * @param	OutInteriorSettings		[out] Upon return, the interior settings for a camera at ViewLocation.
 * @return							If the settings came from a reverb volume, the reverb volume's object index is returned.
 */
AReverbVolume* UWorld::GetAudioSettings( const FVector& ViewLocation, FReverbSettings* OutReverbSettings, FInteriorSettings* OutInteriorSettings )
{
	// Find the highest priority volume encompassing the current view location. This is made easier by the linked
	// list being sorted by priority. @todo: it remains to be seen whether we should trade off sorting for constant
	// time insertion/ removal time via e.g. TLinkedList.
	AReverbVolume* Volume = HighestPriorityReverbVolume;
	while( Volume )
	{
		// Volume encompasses, break out of loop.
		if (Volume->bEnabled && Volume->EncompassesPoint(ViewLocation))
		{
			break;
		}
		// Volume doesn't encompass location, further traverse linked list.
		else
		{
			Volume = Volume->NextLowerPriorityVolume;
		}
	}

	if( Volume )
	{
		if( OutReverbSettings )
		{
			*OutReverbSettings = Volume->Settings;
		}

		if( OutInteriorSettings )
		{
			*OutInteriorSettings = Volume->AmbientZoneSettings;
		}
	}
	else
	{
		// If first level is a FakePersistentLevel (see CommitMapChange for more info)
		// then use its world info for reverb settings
		AWorldSettings* CurrentWorldSettings = GetWorldSettings(true);

		if( OutReverbSettings )
		{
			*OutReverbSettings = CurrentWorldSettings->DefaultReverbSettings;
		}

		if( OutInteriorSettings )
		{
			*OutInteriorSettings = CurrentWorldSettings->DefaultAmbientZoneSettings;
		}
	}
	return Volume;
}

/**
 * Sets bMapNeedsLightingFullyRebuild to the specified value.  Marks the worldsettings package dirty if the value changed.
 *
 * @param	bInMapNeedsLightingFullyRebuild			The new value.
 */
void UWorld::SetMapNeedsLightingFullyRebuilt(int32 InNumLightingUnbuiltObjects)
{
	static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));
	const bool bAllowStaticLighting = (!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnGameThread() != 0);

	if (bAllowStaticLighting && !GetWorldSettings()->bForceNoPrecomputedLighting)
	{
		check(IsInGameThread());
		if (NumLightingUnbuiltObjects != InNumLightingUnbuiltObjects && (NumLightingUnbuiltObjects == 0 || InNumLightingUnbuiltObjects == 0))
		{
			// Save the lighting invalidation for transactions.
			Modify(false);
		}

		NumLightingUnbuiltObjects = InNumLightingUnbuiltObjects;

		// Update last time unbuilt lighting was encountered.
		if (NumLightingUnbuiltObjects > 0)
		{
			LastTimeUnbuiltLightingWasEncountered = GCurrentTime;
		}
	}
}
