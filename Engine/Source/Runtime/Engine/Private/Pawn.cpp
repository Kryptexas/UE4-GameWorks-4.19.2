// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Pawn.cpp: APawn AI implementation
	This contains both C++ methods (movement and reachability), as well as 
	some AI related natives
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
#include "ConfigCacheIni.h"
#include "NavigationPathBuilder.h"
#include "EngineInterpolationClasses.h"
#include "EngineKismetLibraryClasses.h"
#include "ParticleDefinitions.h"
#include "EngineComponentClasses.h"

DEFINE_LOG_CATEGORY(LogDamage);
DEFINE_LOG_CATEGORY_STATIC(LogPawn, Warning, All);

#if LOG_DETAILED_PATHFINDING_STATS
/** Global detailed pathfinding stats. */
FDetailedTickStats GDetailedPathFindingStats( 30, 10, 1, 20, TEXT("pathfinding") );
#endif

APawn::APawn(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	AIControllerClass = AAIController::StaticClass();
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	GameplayDebuggingComponentClass = UGameplayDebuggingComponent::StaticClass();
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	bCanBeDamaged = true;
	
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	NetPriority = 3.0f;
	NetUpdateFrequency = 100.f;
	bReplicateMovement = true;
	BaseEyeHeight = 64.0f;
	AllowedYawError = 10.99f;
	bCollideWhenPlacing = true;
	bProcessingOutsideWorldBounds = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	bInputEnabled = true;
}

void APawn::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	Instigator = this;

	if (AutoPossess != EAutoReceiveInput::Disabled && GetNetMode() != NM_Client )
	{
		const int32 PlayerIndex = int32(AutoPossess.GetValue()) - 1;

		APlayerController* PC = UGameplayStatics::GetPlayerController(this, PlayerIndex);
		if (PC)
		{
			PC->Possess(this);
		}
		else
		{
			GetWorld()->PersistentLevel->RegisterActorForAutoReceiveInput(this, PlayerIndex);
		}
	}
}

void APawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (!IsPendingKill())
	{
		GetWorld()->AddPawn( this );

		// automatically add controller to pawns which were placed in level
		// NOTE: pawns spawned during gameplay are not automatically possessed by a controller
		if ( GetWorld()->bStartup )
		{
			SpawnDefaultController();
		}

		UPawnMovementComponent* MovementComponent = GetMovementComponent();
		if (MovementComponent != NULL)
		{
			// Update Nav Agent props with collision component's setup if it's not set yet
			float BoundRadius, BoundHalfHeight;
			GetSimpleCollisionCylinder(BoundRadius, BoundHalfHeight);
			FNavAgentProperties* NavAgent = MovementComponent->GetNavAgentProperties();
			NavAgent->AgentRadius = BoundRadius;
			NavAgent->AgentHeight = BoundHalfHeight * 2.f;
		}
	}
}


void APawn::PostInitProperties()
{
	Super::PostInitProperties();

	UPawnMovementComponent* MovementComponent = GetMovementComponent();
	if (MovementComponent != NULL)
	{
		// Update Nav Agent props with collision component's setup if it's not set yet
		if (RootComponent)
		{
			// Can't call GetSimpleCollisionCylinder(), because no components will be registered.
			float BoundRadius, BoundHalfHeight;
			RootComponent->UpdateBounds();
			RootComponent->CalcBoundingCylinder(BoundRadius, BoundHalfHeight);
			FNavAgentProperties* NavAgent = MovementComponent->GetNavAgentProperties();
			NavAgent->AgentRadius = BoundRadius;
			NavAgent->AgentHeight = BoundHalfHeight * 2.f;
		}	
	}
}

void APawn::PostLoad()
{
	Super::PostLoad();

	// A pawn should never have this enabled, so we'll aggressively disable it if it did occur.
	AutoReceiveInput = EAutoReceiveInput::Disabled;
}

void APawn::PawnStartFire(uint8 FireModeNum) {}

class UGameplayDebuggingComponent* APawn::GetDebugComponent(bool bCreateIfNotFound)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (DebugComponent == NULL && bCreateIfNotFound)
	{
		DebugComponent = ConstructObject<UGameplayDebuggingComponent>(GameplayDebuggingComponentClass, this, UGameplayDebuggingComponent::DefaultComponentName);
		DebugComponent->RegisterComponent();
		DebugComponent->SetIsReplicated(true);
	}
	return DebugComponent;
#else
	return NULL;
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void APawn::RemoveDebugComponent()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (DebugComponent != NULL)
	{
		DebugComponent->DestroyComponent();
		DebugComponent = NULL;
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

AActor* APawn::GetMovementBaseActor(const APawn* Pawn)
{
	if (Pawn != NULL && Pawn->GetMovementBase())
	{
		return Pawn->GetMovementBase()->GetOwner();
	}

	return NULL;
}

bool APawn::CanBeBaseForCharacter(class APawn* APawn) const
{
	UPrimitiveComponent* RootPrimitive = GetRootPrimitiveComponent();
	if (RootPrimitive && RootPrimitive->CanBeCharacterBase != ECB_Owner)
	{
		return RootPrimitive->CanBeCharacterBase == ECB_Yes;
	}

	return Super::CanBeBaseForCharacter(APawn);
}

FVector APawn::GetVelocity() const
{
	if(GetRootComponent() && GetRootComponent()->IsSimulatingPhysics())
	{
		return GetRootComponent()->GetComponentVelocity();
	}

	const UPawnMovementComponent* MovementComponent = GetMovementComponent();
	return MovementComponent ? MovementComponent->Velocity : FVector::ZeroVector;
}

bool APawn::IsLocallyControlled() const
{
	return ( Controller && Controller->IsLocalController() );
}

bool APawn::ReachedDesiredRotation()
{
	// Only base success on Yaw 
	FRotator DesiredRotation = Controller ? Controller->GetDesiredRotation() : GetActorRotation();
	float YawDiff = FMath::Abs(FRotator::ClampAxis(DesiredRotation.Yaw) - FRotator::ClampAxis(GetActorRotation().Yaw));
	return ( (YawDiff < AllowedYawError) || (YawDiff > 360.f - AllowedYawError) );
}

float APawn::GetDefaultHalfHeight() const
{
	USceneComponent* DefaultRoot = GetClass()->GetDefaultObject<APawn>()->RootComponent;
	if (DefaultRoot)
	{
		float Radius, HalfHeight;
		DefaultRoot->UpdateBounds(); // Since it's the default object, it wouldn't have been registered to ever do this.
		DefaultRoot->CalcBoundingCylinder(Radius, HalfHeight);
		return HalfHeight;
	}
	else
	{
		// This will probably fail to return anything useful, since default objects won't have registered components,
		// but at least it will spit out a warning if so.
		return GetClass()->GetDefaultObject<APawn>()->GetSimpleCollisionHalfHeight();
	}
}

void APawn::SetRemoteViewPitch(float NewRemoteViewPitch)
{
	// Compress pitch to 1 byte
	NewRemoteViewPitch = FRotator::ClampAxis(NewRemoteViewPitch);
	RemoteViewPitch = (uint8)(NewRemoteViewPitch * 255.f/360.f);
}


UPawnNoiseEmitterComponent* APawn::GetPawnNoiseEmitterComponent() const
{
	 UPawnNoiseEmitterComponent* NoiseEmitterComponent = FindComponentByClass<UPawnNoiseEmitterComponent>();
	 if (!NoiseEmitterComponent && Controller)
	 {
		 NoiseEmitterComponent = Controller->FindComponentByClass<UPawnNoiseEmitterComponent>();
	 }

	 return NoiseEmitterComponent;
}

FVector APawn::GetGravityDirection()
{
	return FVector(0.f,0.f,-1.f);
}


float APawn::GetNetPriority(const FVector& ViewPos, const FVector& ViewDir, APlayerController* Viewer, UActorChannel* InChannel, float Time, bool bLowBandwidth)
{
	if ( !bHidden )
	{
		FVector Dir = GetActorLocation() - ViewPos;
		float DistSq = Dir.SizeSquared();

		// adjust priority based on distance and whether pawn is in front of viewer or is controlled
		if ( (ViewDir | Dir) < 0.f )
		{
			if ( DistSq > NEARSIGHTTHRESHOLDSQUARED )
				Time *= 0.3f;
			else if ( DistSq > CLOSEPROXIMITYSQUARED )
				Time *= 0.5f;
		}
		else if ( Controller && (DistSq < FARSIGHTTHRESHOLDSQUARED) && (FMath::Square(ViewDir | Dir) > 0.5f * DistSq) )
		{
			Time *= 2.f;
		}
		else if (DistSq > MEDSIGHTTHRESHOLDSQUARED)
		{
			Time *= 0.5f;
		}
	}
	return NetPriority * Time;
}


FVector APawn::GetPawnViewLocation() const
{
	return GetActorLocation() + FVector(0.f,0.f,BaseEyeHeight);
}

FRotator APawn::GetViewRotation() const
{
	if (Controller != NULL)
	{
		return Controller->GetControlRotation();
	}
	else if (Role < ROLE_Authority)
	{
		// check if being spectated
		for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
		{
			APlayerController* PlayerController = *Iterator;
			if(PlayerController && PlayerController->PlayerCameraManager->GetViewTargetPawn() == this)
			{
				return PlayerController->BlendedTargetViewRotation;
			}
		}
	}

	return GetActorRotation();
}

void APawn::SpawnDefaultController()
{
	if ( Controller != NULL || GetNetMode() == NM_Client)
	{
		return;
	}
	if ( AIControllerClass != NULL )
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Instigator = Instigator;
		SpawnInfo.bNoCollisionFail = true;
		SpawnInfo.OverrideLevel = GetLevel();
		Controller = GetWorld()->SpawnActor<AController>( AIControllerClass, GetActorLocation(), GetActorRotation(), SpawnInfo );
		if ( Controller != NULL )
		{
			Controller->Possess( this );
		}
	}
}

void APawn::TurnOff()
{
	if (Role == ROLE_Authority)
	{
		SetReplicates(true);
	}
	
	// do not block anything, just ignore
	SetActorEnableCollision(false);

	if (GetMovementComponent())
	{
		GetMovementComponent()->StopMovementImmediately();
		GetMovementComponent()->SetComponentTickEnabled(false);
	}

	DisableComponentsSimulatePhysics();
}

void APawn::BecomeViewTarget(APlayerController* PC)
{
	if (GetNetMode() != NM_Client)
	{
		PC->ForceSingleNetUpdateFor(this);
	}
}

void APawn::PawnClientRestart()
{
	Restart();

	APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC && PC->IsLocalController())
	{
		// Handle camera possession
		if (PC->bAutoManageActiveCameraTarget)
		{
			PC->SetViewTarget(this);
		}

		// Set up player input component, if there isn't one already.
		if (InputComponent == NULL)
		{
			InputComponent = CreatePlayerInputComponent();
			if (InputComponent)
			{
				SetupPlayerInputComponent(InputComponent);
				InputComponent->RegisterComponent();
				UBlueprintGeneratedClass* BGClass = Cast<UBlueprintGeneratedClass>(GetClass());
				if(BGClass != NULL)
				{
					InputComponent->bBlockInput = bBlockInput;
					UInputDelegateBinding::BindInputDelegates(BGClass, InputComponent);
				}
			}
		}
	}
}

void APawn::Destroyed()
{
	DetachFromControllerPendingDestroy();

	GetWorld()->RemovePawn( this );
	Super::Destroyed();
}

bool APawn::ShouldTakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const
{
	if ( (Role < ROLE_Authority) || !bCanBeDamaged || !GetWorld()->GetAuthGameMode() || (Damage == 0.f) )
	{
		return false;
	}

	return true;
}

float APawn::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!ShouldTakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser))
	{
		return 0.f;
	}

	// do not modify damage parameters after this
	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	// respond to the damage
	if (ActualDamage != 0.f)
	{
		if ( EventInstigator && EventInstigator != Controller )
		{
			LastHitBy = EventInstigator;
		}
	}

	return ActualDamage;
}

bool APawn::IsControlled() const
{
	APlayerController* const PC = Cast<APlayerController>(Controller);
	return(PC != NULL);
}

/** For K2 access to the controller. */
AController* APawn::GetController() const
{
	return Controller;
}

FRotator APawn::GetControlRotation() const
{
	return Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator;
}

void APawn::OnRep_Controller()
{
	if ( (Controller != NULL) && (Controller->GetPawn() == NULL) )
	{
		// This ensures that APawn::OnRep_Pawn is called. Since we cant ensure replication order of APawn::Controller and AController::Pawn,
		// if APawn::Controller is repped first, it will set AController::Pawn locally. When AController::Pawn is repped, the rep value will not
		// be different from the just set local value, and OnRep_Pawn will not be called. This can cause problems if OnRep_Pawn does anything important.
		//
		// It would be better to never ever set replicated properties locally, but this is pretty core in the gameplay framework and I think there are
		// lots of assumptions made in the code base that the Pawn and Controller will always be linked both ways.
		Controller->SetPawnFromRep(this); 

		APlayerController* const PC = Cast<APlayerController>(Controller);
		if ( (PC != NULL) && PC->bAutoManageActiveCameraTarget && (PC->PlayerCameraManager->ViewTarget.Target == Controller) )
		{
			PC->SetViewTarget(this);
		}
	}
}

void APawn::OnRep_PlayerState() {}

void APawn::PossessedBy(AController* NewController)
{
	AController* const OldController = Controller;

	Controller = NewController;
	ForceNetUpdate();

	if (Controller->PlayerState != NULL)
	{
		PlayerState = Controller->PlayerState;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (GetNetMode() != NM_Standalone)
		{
			SetReplicates(true);
			SetAutonomousProxy(true);
		}
	}
	else
	{
		CopyRemoteRoleFrom(GetDefault<APawn>());
	}

	// dispatch Blueprint event if necessary
	if (OldController != NewController)
	{
		ReceivePossessed(Controller);
	}
}

void APawn::UnPossessed()
{
	AController* const OldController = Controller;

	ForceNetUpdate();

	PlayerState = NULL;
	SetOwner(NULL);
	Controller = NULL;

	// Unregister input component if we created one
	DestroyPlayerInputComponent();

	// dispatch Blueprint event if necessary
	if (OldController)
	{
		ReceiveUnpossessed(OldController);
	}
}


class UNetConnection* APawn::GetNetConnection()
{
	// if have a controller, it has the net connection
	if ( Controller )
	{
		return Controller->GetNetConnection();
	}
	return Super::GetNetConnection();
}


class UPlayer* APawn::GetNetOwningPlayer()
{
	if (Role == ROLE_Authority)
	{
		if (Controller)
		{
			APlayerController* PC = Cast<APlayerController>(Controller);
			return PC ? PC->Player : NULL;
		}
	}

	return Super::GetNetOwningPlayer();
}


UInputComponent* APawn::CreatePlayerInputComponent()
{
	static const FName InputComponentName(TEXT("PawnInputComponent0"));
	return ConstructObject<UInputComponent>(UInputComponent::StaticClass(), this, InputComponentName);
}

void APawn::DestroyPlayerInputComponent()
{
	if (InputComponent)
	{
		InputComponent->DestroyComponent();
		InputComponent = NULL;
	}
}


void APawn::AddMovementInput(FVector WorldDirection, float ScaleValue)
{
	UPawnMovementComponent* MovementComponent = GetMovementComponent();
	if (MovementComponent)
	{
		MovementComponent->AddInputVector(WorldDirection * ScaleValue);
	}
}

void APawn::AddControllerPitchInput(float Val)
{
	if (Controller && Controller->IsLocalPlayerController())
	{
		APlayerController* const PC = CastChecked<APlayerController>(Controller);
		PC->AddPitchInput(Val);
	}
}

void APawn::AddControllerYawInput(float Val)
{
	if (Controller && Controller->IsLocalPlayerController())
	{
		APlayerController* const PC = CastChecked<APlayerController>(Controller);
		PC->AddYawInput(Val);
	}
}

void APawn::AddControllerRollInput(float Val)
{
	if (Controller && Controller->IsLocalPlayerController())
	{
		APlayerController* const PC = CastChecked<APlayerController>(Controller);
		PC->AddRollInput(Val);
	}
}


void APawn::Restart()
{
	if (GetMovementComponent())
	{
		GetMovementComponent()->StopMovementImmediately();
	}
	RecalculateBaseEyeHeight();
}

class APhysicsVolume* APawn::GetPawnPhysicsVolume() const
{
	const UPawnMovementComponent* MovementComponent = GetMovementComponent();
	if (MovementComponent)
	{
		return MovementComponent->GetPhysicsVolume();
	}
	else if (GetRootComponent())
	{
		return GetRootComponent()->GetPhysicsVolume();
	}
	return GetWorld()->GetDefaultPhysicsVolume();
}


void APawn::SetPlayerDefaults()
{
}

void APawn::Tick( float DeltaSeconds )
{
	Super::Tick(DeltaSeconds);

	if (Role == ROLE_Authority && GetController())
	{
		SetRemoteViewPitch(GetController()->GetControlRotation().Pitch);
	}
}

void APawn::RecalculateBaseEyeHeight()
{
	BaseEyeHeight = GetDefault<APawn>(GetClass())->BaseEyeHeight;
}

void APawn::Reset()
{
	if ( (Controller == NULL) || (Controller->PlayerState != NULL) )
	{
		DetachFromControllerPendingDestroy();
		Destroy();
	}
	else
	{
		Super::Reset();
	}
}

FString APawn::GetHumanReadableName() const
{
	return PlayerState ? PlayerState->PlayerName : Super::GetHumanReadableName();
}


void APawn::DisplayDebug(class UCanvas* Canvas, const TArray<FName>& DebugDisplay, float& YL, float& YPos)
{
// 	for (FTouchingComponentsMap::TIterator It(TouchingComponents); It; ++It)
// 	{
// 		UPrimitiveComponent* const MyComp = It.Key().Get();
// 		UPrimitiveComponent* const OtherComp = It.Value().Get();
// 		AActor* OtherActor = OtherComp ? OtherComp->GetOwner() : NULL;
// 
// 		HUD->Canvas->DrawText(FString::Printf(TEXT("TOUCHING my %s to %s's %s"), *GetNameSafe(MyComp), *GetNameSafe(OtherActor), *GetNameSafe(OtherComp)));
// 		YPos += YL;
// 		HUD->Canvas->SetPos(4,YPos);
// 	}

	UFont* RenderFont = GEngine->GetSmallFont();
	if ( PlayerState == NULL )
	{
		Canvas->DrawText(RenderFont, TEXT("NO PlayerState"), 4.0f, YPos );
		YPos += YL;
	}
	else
	{
		PlayerState->DisplayDebug(Canvas, DebugDisplay, YL, YPos);
	}

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	Canvas->SetDrawColor(255,255,255);


	if (DebugDisplay.Contains(NAME_Camera))
	{
		Canvas->DrawText(RenderFont, FString::Printf(TEXT("BaseEyeHeight %f"), BaseEyeHeight), 4.0f, YPos);
		YPos += YL;
	}

	// Controller
	if ( Controller == NULL )
	{
		Canvas->SetDrawColor(255,0,0);
		Canvas->DrawText(RenderFont, TEXT("NO Controller"), 4.0f, YPos);
		YPos += YL;
		//HUD->PlayerOwner->DisplayDebug(Canvas, DebugDisplay, YL, YPos);
	}
	else
	{
		Controller->DisplayDebug(Canvas, DebugDisplay, YL, YPos);
	}
}


void APawn::GetActorEyesViewPoint( FVector& out_Location, FRotator& out_Rotation ) const
{
	out_Location = GetPawnViewLocation();
	out_Rotation = GetViewRotation();
}


FRotator APawn::GetBaseAimRotation() const
{
	// If we have a controller, by default we aim at the player's 'eyes' direction
	// that is by default Controller.Rotation for AI, and camera (crosshair) rotation for human players.
	FVector POVLoc;
	FRotator POVRot;
	if( Controller != NULL && !InFreeCam() )
	{
		Controller->GetPlayerViewPoint(POVLoc, POVRot);
		return POVRot;
	}

	// If we have no controller, we simply use our rotation
	POVRot = GetActorRotation();

	// If our Pitch is 0, then use RemoteViewPitch
	if( POVRot.Pitch == 0.f )
	{
		POVRot.Pitch = RemoteViewPitch;
		POVRot.Pitch = POVRot.Pitch * 360.f/255.f;
	}

	return POVRot;
}

bool APawn::InFreeCam() const
{
	const APlayerController* PC = Cast<const APlayerController>(Controller);
	static FName NAME_FreeCam =  FName(TEXT("FreeCam"));
	static FName NAME_FreeCamDefault = FName(TEXT("FreeCam_Default"));
	return (PC != NULL && PC->PlayerCameraManager != NULL && (PC->PlayerCameraManager->CameraStyle == NAME_FreeCam || PC->PlayerCameraManager->CameraStyle == NAME_FreeCamDefault) );
}

void APawn::OutsideWorldBounds()
{
	if ( !bProcessingOutsideWorldBounds )
	{
		bProcessingOutsideWorldBounds = true;
		// AI pawns on the server just destroy
		if (Role == ROLE_Authority && Cast<APlayerController>(Controller) == NULL)
		{
			Destroy();
		}
		else
		{
			DetachFromControllerPendingDestroy();
			TurnOff();
			SetActorHiddenInGame(true);
			SetLifeSpan( FMath::Clamp(InitialLifeSpan, 0.1f, 1.0f) );
		}
		bProcessingOutsideWorldBounds = false;
	}
}

void APawn::ClientSetRotation( FRotator NewRotation )
{
	if ( Controller != NULL )
	{
		Controller->ClientSetRotation(NewRotation);
	}
}

void APawn::FaceRotation(FRotator NewControlRotation, float DeltaTime)
{
	const FRotator CurrentRotation = GetActorRotation();

	if (!bUseControllerRotationPitch)
	{
		NewControlRotation.Pitch = CurrentRotation.Pitch;
	}

	if (!bUseControllerRotationYaw)
	{
		NewControlRotation.Yaw = CurrentRotation.Yaw;
	}

	if (!bUseControllerRotationRoll)
	{
		NewControlRotation.Roll = CurrentRotation.Roll;
	}

	SetActorRotation(NewControlRotation);
}

void APawn::DetachFromControllerPendingDestroy()
{
	if ( Controller != NULL && Controller->GetPawn() == this )
	{
		AController* OldController = Controller;
		Controller->PawnPendingDestroy(this);
		if (Controller != NULL)
		{
			Controller->UnPossess();
			Controller = NULL;
		}
	}
}

AController* APawn::GetDamageInstigator(AController* InstigatedBy, const UDamageType& DamageType) const
{
	if ( (InstigatedBy != NULL) && (InstigatedBy != Controller) )
	{
		return InstigatedBy;
	}
	else if ( DamageType.bCausedByWorld && (LastHitBy != NULL) )
	{
		return LastHitBy;
	}
	return InstigatedBy;
}


#if WITH_EDITOR
void APawn::EditorApplyRotation(const FRotator& DeltaRotation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	Super::EditorApplyRotation(DeltaRotation, bAltDown, bShiftDown, bCtrlDown);

	// Forward new rotation on to the pawn's controller.
	if( Controller )
	{
		Controller->SetControlRotation( GetActorRotation() );
	}
}
#endif

void APawn::EnableInput(class APlayerController* PlayerController)
{
	if (PlayerController == Controller || PlayerController == NULL)
	{
		bInputEnabled = true;
	}
	else
	{
		UE_LOG(LogPawn, Error, TEXT("EnableInput can only be specified on a Pawn for its Controller"));
	}
}

void APawn::DisableInput(class APlayerController* PlayerController)
{
	if (PlayerController == Controller || PlayerController == NULL)
	{
		bInputEnabled = true;
	}
	else
	{
		UE_LOG(LogPawn, Error, TEXT("EnableInput can only be specified on a Pawn for its Controller"));
	}
}

void APawn::GetMoveGoalReachTest(class AActor* MovingActor, const FVector& MoveOffset, FVector& GoalOffset, float& GoalRadius, float& GoalHalfHeight) const 
{
	GoalOffset = FVector::ZeroVector;
	GetSimpleCollisionCylinder(GoalRadius, GoalHalfHeight);
}

// @TODO: Deprecated, remove me.
void APawn::LaunchPawn(FVector LaunchVelocity, bool bXYOverride, bool bZOverride)
{
	ACharacter* Character = Cast<ACharacter>(this);
	if (Character)
	{
		Character->LaunchCharacter(LaunchVelocity, bXYOverride, bZOverride);
	}
}

bool APawn::IsWalking() const
{
	// @todo: Deprecated, remove
	return GetMovementComponent() ? GetMovementComponent()->IsMovingOnGround() : false;
}

bool APawn::IsFalling() const
{
	// @todo: Deprecated, remove
	return GetMovementComponent() ? GetMovementComponent()->IsFalling() : false;
}

bool APawn::IsCrouched() const
{
	// @todo: Deprecated, remove
	return GetMovementComponent() ? GetMovementComponent()->IsCrouching() : false;
}

// REPLICATION

void APawn::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	if (Role == ROLE_SimulatedProxy)
	{
		if ( GetMovementComponent() )
		{
			GetMovementComponent()->Velocity = NewVelocity;
		}
	}
}

void APawn::PostNetReceiveLocation()
{
	// always consider Location as changed if we were spawned this tick as in that case our replicated Location was set as part of spawning, before PreNetReceive()
	if( (ReplicatedMovement.Location == GetActorLocation()) && (CreationTime != GetWorld()->TimeSeconds) )
	{
		return;
	}

	if( Role == ROLE_SimulatedProxy )
	{
		// Correction to make sure pawn doesn't penetrate floor after replication rounding
		ReplicatedMovement.Location.Z += 0.01f;

		FVector OldLocation = GetActorLocation();
		TeleportTo( ReplicatedMovement.Location, GetActorRotation(), false, true );

		INetworkPredictionInterface* PredictionInterface = InterfaceCast<INetworkPredictionInterface>(GetMovementComponent());
		if (PredictionInterface)
		{
			PredictionInterface->SmoothCorrection(OldLocation);
		}
	}
}

bool APawn::IsNetRelevantFor(APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation)
{
	if( bAlwaysRelevant || RealViewer == Controller || IsOwnedBy(Viewer) || IsOwnedBy(RealViewer) || this==Viewer || Viewer==Instigator
		|| IsBasedOn(Viewer) || (Viewer && Viewer->IsBasedOn(this)) )
	{
		return true;
	}
	else if( (bHidden || bOnlyRelevantToOwner) && (!GetRootComponent() || !GetRootComponent()->IsCollisionEnabled()) ) 
	{
		return false;
	}
	else
	{
		UPrimitiveComponent* MovementBase = GetMovementBase();
		AActor* BaseActor = MovementBase ? MovementBase->GetOwner() : NULL;
		if ( MovementBase && BaseActor && GetMovementComponent() && ((Cast<const USkeletalMeshComponent>(MovementBase)) || (BaseActor == GetOwner())) )
		{
			return BaseActor->IsNetRelevantFor( RealViewer, Viewer, SrcLocation );
		}
	}

	return ((SrcLocation - GetActorLocation()).SizeSquared() < NetCullDistanceSquared);
}

void APawn::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( APawn, PlayerState ); 

	DOREPLIFETIME_CONDITION( APawn, Controller,			COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( APawn, RemoteViewPitch, 	COND_SkipOwner );

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	DOREPLIFETIME(APawn, DebugComponent);
#endif
}

void APawn::MoveIgnoreActorAdd(AActor * ActorToIgnore)
{
	UPrimitiveComponent * RootPrimitiveComponent = GetRootPrimitiveComponent();
	if( RootPrimitiveComponent )
	{
		// Remove dead references first
		RootPrimitiveComponent->MoveIgnoreActors.Remove(NULL);
		if( ActorToIgnore )
		{
			RootPrimitiveComponent->MoveIgnoreActors.AddUnique(ActorToIgnore);
		}
	}
}

void APawn::MoveIgnoreActorRemove(AActor * ActorToIgnore)
{
	UPrimitiveComponent * RootPrimitiveComponent = GetRootPrimitiveComponent();
	if( RootPrimitiveComponent )
	{
		// Remove dead references first
		RootPrimitiveComponent->MoveIgnoreActors.Remove(NULL);
		if( ActorToIgnore )
		{
			RootPrimitiveComponent->MoveIgnoreActors.Remove(ActorToIgnore);
		}
	}
}

void APawn::PawnMakeNoise(float Loudness, FVector NoiseLocation, bool bUseNoiseMakerLocation, AActor* NoiseMaker)
{
	if (NoiseMaker == NULL)
	{
		NoiseMaker = this;
	}
	NoiseMaker->MakeNoise(Loudness, this, bUseNoiseMakerLocation ? NoiseMaker->GetActorLocation() : NoiseLocation);
}
