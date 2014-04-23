// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Character.cpp: ACharacter implementation
	TODO: Put description here
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
#include "DisplayDebugHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacter, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogAvatar, Log, All);

FName ACharacter::MeshComponentName(TEXT("CharacterMesh0"));
FName ACharacter::CharacterMovementComponentName(TEXT("CharMoveComp"));
FName ACharacter::CapsuleComponentName(TEXT("CollisionCylinder"));

ACharacter::ACharacter(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FName ID_Characters;
		FText NAME_Characters;
		FConstructorStatics()
			: ID_Characters(TEXT("Characters"))
			, NAME_Characters(NSLOCTEXT( "SpriteCategory", "Characters", "Characters" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	// Character rotation only changes in Yaw, to prevent the capsule from changing orientation.
	// Ask the Controller for the full rotation if desired (ie for aiming).
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = true;

	CapsuleComponent = PCIP.CreateDefaultSubobject<UCapsuleComponent>(this, ACharacter::CapsuleComponentName);
	CapsuleComponent->InitCapsuleSize(34.0f, 88.0f);
	CapsuleComponent->BodyInstance.bEnableCollision_DEPRECATED = true;

	static FName CollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->SetCollisionProfileName(CollisionProfileName);

	CapsuleComponent->CanBeCharacterBase = ECB_No;
	CapsuleComponent->bShouldUpdatePhysicsVolume = true;
	CapsuleComponent->bCheckAsyncSceneOnMove = false;	
	RootComponent = CapsuleComponent;

#if WITH_EDITORONLY_DATA
	ArrowComponent = PCIP.CreateEditorOnlyDefaultSubobject<UArrowComponent>(this, TEXT("Arrow"));
	if (ArrowComponent)
	{
		ArrowComponent->ArrowColor = FColor(150, 200, 255);
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->SpriteInfo.Category = ConstructorStatics.ID_Characters;
		ArrowComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Characters;
		ArrowComponent->AttachParent = CapsuleComponent;
		ArrowComponent->bIsScreenSizeScaled = true;
	}
#endif // WITH_EDITORONLY_DATA

	CharacterMovement = PCIP.CreateDefaultSubobject<UCharacterMovementComponent>(this, ACharacter::CharacterMovementComponentName);
	if (CharacterMovement)
	{
		CharacterMovement->UpdatedComponent = CapsuleComponent;
		CharacterMovement->MaxStepHeight = 45.f;
		CrouchedEyeHeight = CharacterMovement->CrouchedHalfHeight * 0.80f;

		CharacterMovement->GetNavAgentProperties()->bCanJump = true;
		CharacterMovement->GetNavAgentProperties()->bCanWalk = true;
		CharacterMovement->SetJumpAllowed(true);
	}

	Mesh = PCIP.CreateOptionalDefaultSubobject<USkeletalMeshComponent>(this, ACharacter::MeshComponentName);
	if (Mesh)
	{
		Mesh->AlwaysLoadOnClient = true;
		Mesh->AlwaysLoadOnServer = true;
		Mesh->bOwnerNoSee = false;
		Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPose;
		Mesh->bCastDynamicShadow = true;
		Mesh->bAffectDynamicIndirectLighting = true;
		Mesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		// force tick after movement component updates
		if( CharacterMovement )
		{
			Mesh->PrimaryComponentTick.AddPrerequisite(this, CharacterMovement->PrimaryComponentTick); 
		}
		Mesh->bChartDistanceFactor = true;
		Mesh->AttachParent = CapsuleComponent;
		static FName CollisionProfileName(TEXT("CharacterMesh"));
		Mesh->SetCollisionProfileName(CollisionProfileName);
		Mesh->bGenerateOverlapEvents = false;
	}
}


void ACharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (!IsPendingKill())
	{
		if (Mesh)
		{
			BaseTranslationOffset = Mesh->RelativeLocation;
		}

		if (CharacterMovement.IsValid() && CapsuleComponent.IsValid())
		{
			CharacterMovement->UpdateNavAgent(CapsuleComponent);
		}
	}
}


void ACharacter::SetupPlayerInputComponent(UInputComponent* InputComponent)
{
	check(InputComponent);
}


void ACharacter::GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (IsTemplate())
	{
		UE_LOG(LogCharacter, Log, TEXT("WARNING ACharacter::GetSimpleCollisionCylinder : Called on default object '%s'. Will likely return zero size. Consider using GetDefaultHalfHeight() instead."), *this->GetPathName());
	}
#endif

	if (RootComponent == CapsuleComponent && IsRootComponentCollisionRegistered())
	{
		// Note: we purposefully ignore the component transform here aside from scale, always treating it as vertically aligned.
		// This improves performance and is also how we stated the CapsuleComponent would be used.
		CapsuleComponent->GetScaledCapsuleSize(CollisionRadius, CollisionHalfHeight);
	}
	else
	{
		Super::GetSimpleCollisionCylinder(CollisionRadius, CollisionHalfHeight);
	}
}

void ACharacter::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	Super::ApplyWorldOffset(InOffset, bWorldShift);

	// Update cached location values in movement component
	if (CharacterMovement.IsValid())
	{
		CharacterMovement->OldBaseLocation+= InOffset;
	}
}

float ACharacter::GetDefaultHalfHeight() const
{
	UCapsuleComponent* DefaultCapsule = GetClass()->GetDefaultObject<ACharacter>()->CapsuleComponent;
	if (DefaultCapsule)
	{
		return DefaultCapsule->GetScaledCapsuleHalfHeight();
	}
	else
	{
		return Super::GetDefaultHalfHeight();
	}
}


UActorComponent* ACharacter::FindComponentByClass(const UClass* Class) const
{
	// If the character has a Mesh, treat it as the first 'hit' when finding components
	if (Class && Mesh && Mesh->IsA(Class))
		{
			return Mesh;
		}

	return Super::FindComponentByClass(Class);
}


void ACharacter::Landed(const FHitResult& Hit)
{
	OnLanded(Hit);
}

bool ACharacter::CanJump_Implementation() const
{
	return !bIsCrouched && CharacterMovement && CharacterMovement->IsMovingOnGround() && CharacterMovement->CanEverJump() && !CharacterMovement->bWantsToCrouch;
}

bool ACharacter::DoJump( bool bReplayingMoves )
{
	return CanJump() && CharacterMovement->DoJump();
}


void ACharacter::RecalculateBaseEyeHeight()
{
	if (!bIsCrouched)
	{
		Super::RecalculateBaseEyeHeight();
	}
	else
	{
		BaseEyeHeight = CrouchedEyeHeight;
	}
}


void ACharacter::OnRep_IsCrouched()
{
	if (CharacterMovement)
	{
		if (bIsCrouched)
		{
			CharacterMovement->Crouch(true);
		}
		else
		{
			CharacterMovement->UnCrouch(true);
		}
	}
}

bool ACharacter::CanCrouch()
{
	return !bIsCrouched && CharacterMovement && CharacterMovement->CanEverCrouch() && (CapsuleComponent->GetUnscaledCapsuleHalfHeight() > CharacterMovement->CrouchedHalfHeight) && GetRootComponent() && !GetRootComponent()->IsSimulatingPhysics();
}

void ACharacter::Crouch(bool bClientSimulation)
{
	if (CharacterMovement)
	{
		if (CanCrouch())
		{
			CharacterMovement->bWantsToCrouch = true;
			if (CharacterMovement->CanCrouchInCurrentState())
			{
				CharacterMovement->Crouch(bClientSimulation);
			}
		}
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		else if (!CharacterMovement->CanEverCrouch())
		{
			UE_LOG(LogCharacter, Log, TEXT("%s is trying to crouch, but crouching is disabled on this character! (check CharacterMovement NavAgentSettings)"), *GetName());
		}
#endif
	}
}

void ACharacter::UnCrouch(bool bClientSimulation)
{
	if (CharacterMovement)
	{
		CharacterMovement->bWantsToCrouch = false;
		CharacterMovement->UnCrouch(bClientSimulation);
	}
}


void ACharacter::OnEndCrouch( float HeightAdjust, float ScaledHeightAdjust )
{
	RecalculateBaseEyeHeight();

	if (Mesh)
	{
		Mesh->RelativeLocation.Z = GetDefault<ACharacter>(GetClass())->Mesh->RelativeLocation.Z;
		BaseTranslationOffset.Z = Mesh->RelativeLocation.Z;
	}
	else
	{
		BaseTranslationOffset.Z = GetDefault<ACharacter>(GetClass())->BaseTranslationOffset.Z;
	}

	K2_OnEndCrouch(HeightAdjust, ScaledHeightAdjust);
}

void ACharacter::OnStartCrouch( float HeightAdjust, float ScaledHeightAdjust )
{
	RecalculateBaseEyeHeight();

	if (Mesh)
	{
		Mesh->RelativeLocation.Z = GetDefault<ACharacter>(GetClass())->Mesh->RelativeLocation.Z + HeightAdjust;
		BaseTranslationOffset.Z = Mesh->RelativeLocation.Z;
	}
	else
	{
		BaseTranslationOffset.Z = GetDefault<ACharacter>(GetClass())->BaseTranslationOffset.Z + HeightAdjust;
	}

	K2_OnStartCrouch(HeightAdjust, ScaledHeightAdjust);
}

void ACharacter::ApplyDamageMomentum(float DamageTaken, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser)
{
	UDamageType const* const DmgTypeCDO = DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>();
	float const ImpulseScale = DmgTypeCDO->DamageImpulse;

	if ( (ImpulseScale > 3.f) && (CharacterMovement != NULL) )
	{
		FHitResult HitInfo;
		FVector ImpulseDir;
		DamageEvent.GetBestHitInfo(this, PawnInstigator, HitInfo, ImpulseDir);

		FVector Impulse = ImpulseDir * ImpulseScale;
		bool const bMassIndependentImpulse = !DmgTypeCDO->bScaleMomentumByMass;
		CharacterMovement->AddMomentum(Impulse, HitInfo.ImpactPoint, bMassIndependentImpulse);
	}
}

void ACharacter::TeleportSucceeded(bool bIsATest)
{
	if (!bIsATest && CharacterMovement)
	{
		CharacterMovement->OnTeleported();
	}

	Super::TeleportSucceeded(bIsATest);
}


void ACharacter::ClearCrossLevelReferences()
{
	if( MovementBase != NULL && GetOutermost() != MovementBase->GetOutermost() )
	{
		SetBase( NULL );
	}

	Super::ClearCrossLevelReferences();
}


// Helper to set up tick dependencies on a movement base
namespace MovementBaseUtility
{
	void AddTickDependency(FTickFunction& BasedObjectTick, class UPrimitiveComponent* NewBase)
	{
		if (NewBase && MovementBaseUtility::UseRelativePosition(NewBase))
		{
			BasedObjectTick.AddPrerequisite(NewBase, NewBase->PrimaryComponentTick);
			AActor* NewBaseOwner = NewBase->GetOwner();
			if (NewBaseOwner)
			{
				BasedObjectTick.AddPrerequisite(NewBaseOwner, NewBaseOwner->PrimaryActorTick);
			}
		}
	}

	void RemoveTickDependency(FTickFunction& BasedObjectTick, class UPrimitiveComponent* OldBase)
	{
		if (OldBase && MovementBaseUtility::UseRelativePosition(OldBase))
		{
			BasedObjectTick.RemovePrerequisite(OldBase, OldBase->PrimaryComponentTick);
			AActor* OldBaseOwner = OldBase->GetOwner();
			if (OldBaseOwner)
			{
				BasedObjectTick.RemovePrerequisite(OldBaseOwner, OldBaseOwner->PrimaryActorTick);
			}
		}
	}

	FVector GetMovementBaseVelocity(const class UPrimitiveComponent* MovementBase)
	{
		FVector BaseVelocity = FVector::ZeroVector;
		if (MovementBaseUtility::IsDynamicBase(MovementBase))
		{
			BaseVelocity = MovementBase->GetComponentVelocity();
			if (BaseVelocity.IsZero())
			{
				// Fall back to actor's Root component
				const AActor* Owner = MovementBase->GetOwner();
				if (Owner)
				{
					// Component might be moved manually (not by simulated physics or a movement component), see if the root component of the actor has a velocity.
					BaseVelocity = MovementBase->GetOwner()->GetVelocity();
				}				
			}

			// Fall back to physics velocity.
			if (BaseVelocity.IsZero() && MovementBase->GetBodyInstance())
			{
				BaseVelocity = MovementBase->GetBodyInstance()->GetUnrealWorldVelocity();
			}
		}
		
		return BaseVelocity;
	}

	FVector GetMovementBaseTangentialVelocity(const class UPrimitiveComponent* MovementBase, const FVector& WorldLocation)
	{
		if (MovementBaseUtility::IsDynamicBase(MovementBase))
		{
			if (MovementBase->GetBodyInstance())
			{
				const FVector BaseAngVel = MovementBase->GetBodyInstance()->GetUnrealWorldAngularVelocity();
				if (!BaseAngVel.IsZero())
				{
					const FVector RadialDistanceToBase = WorldLocation - MovementBase->GetComponentLocation();
					const FVector TangentialVel = FMath::DegreesToRadians(BaseAngVel) ^ RadialDistanceToBase;
					return TangentialVel;
				}
			}			
		}
		
		return FVector::ZeroVector;
	}
}


/**	Change the Pawn's base. */
void ACharacter::SetBase( UPrimitiveComponent* NewBaseComponent, bool bNotifyPawn )
{
	if (NewBaseComponent != MovementBase)
	{
		// Verify no recursion.
		APawn* Loop = (NewBaseComponent ? Cast<APawn>(NewBaseComponent->GetOwner()) : NULL);
		for(  ; Loop!=NULL; Loop=Cast<APawn>(Loop->GetMovementBase()) )
		{
			if( Loop == this )
			{
				UE_LOG(LogCharacter, Warning, TEXT(" SetBase failed! Recursion detected. Pawn %s already based on %s."), *GetName(), *NewBaseComponent->GetName());
				return;
			}
		}

		// Set base.
		UPrimitiveComponent* OldBase = MovementBase;
		MovementBase = NewBaseComponent;

		// Update relative location/rotation
		if ( Role == ROLE_Authority || Role == ROLE_AutonomousProxy )
		{
			UE_LOG(LogCharacter, Verbose, TEXT("Setting base on %s for '%s' to '%s'"), Role == ROLE_Authority ? TEXT("Server") : TEXT("AutoProxy"), *GetName(), *GetFullNameSafe(NewBaseComponent));
			RelativeMovement.MovementBase = NewBaseComponent;
			RelativeMovement.bServerHasBaseComponent = (NewBaseComponent != NULL); // Flag whether the server had a non-null base.

			if (MovementBaseUtility::UseRelativePosition(MovementBase))
			{
				RelativeMovement.Location = GetActorLocation() - MovementBase->GetComponentLocation();
				RelativeMovement.Rotation = (FRotationMatrix(GetActorRotation()) * FRotationMatrix(MovementBase->GetComponentRotation()).GetTransposed()).Rotator();
			}
		}
		else
		{
			UE_LOG(LogCharacter, Verbose, TEXT("Setting base on Client for '%s' to '%s'"), *GetName(), *GetFullNameSafe(NewBaseComponent));
		}

		// Notify this actor of his new floor.
		if ( bNotifyPawn )
		{
			BaseChange();
		}

		if (CharacterMovement)
		{
			MovementBaseUtility::RemoveTickDependency(CharacterMovement->PrimaryComponentTick, OldBase);
			MovementBaseUtility::AddTickDependency(CharacterMovement->PrimaryComponentTick, MovementBase);

			if (MovementBase)
			{
				// Update OldBaseLocation/Rotation as those were referring to a different base
				CharacterMovement->OldBaseLocation = MovementBase->GetComponentLocation();
				CharacterMovement->OldBaseRotation = MovementBase->GetComponentRotation();
			}
			else
			{
				// Base is NULL, invalidate floor.
				CharacterMovement->CurrentFloor.Clear();
			}
		}
	}
}

void ACharacter::TurnOff()
{
	if (CharacterMovement != NULL)
	{
		CharacterMovement->StopMovementImmediately();
		CharacterMovement->DisableMovement();
	}

	if (GetNetMode() != NM_DedicatedServer && Mesh != NULL)
	{
		Mesh->bPauseAnims = true;
		if (Mesh->IsSimulatingPhysics())
		{
			Mesh->bBlendPhysics = true;
			Mesh->KinematicBonesUpdateType = EKinematicBonesUpdateToPhysics::SkipAllBones;
		}
	}

	Super::TurnOff();
}

void ACharacter::Restart()
{
	Super::Restart();
	bPressedJump = false;
}

void ACharacter::PawnClientRestart()
{
	if (CharacterMovement != NULL)
	{
		CharacterMovement->StopMovementImmediately();
		CharacterMovement->ResetPredictionData_Client();
	}

	Super::PawnClientRestart();
}

void ACharacter::UnPossessed()
{
	Super::UnPossessed();

	if (CharacterMovement)
	{
		CharacterMovement->ResetPredictionData_Client();
		CharacterMovement->ResetPredictionData_Server();
	}
}


void ACharacter::BaseChange()
{
	if (CharacterMovement && CharacterMovement->MovementMode != MOVE_None)
	{
		AActor* ActualMovementBase = GetMovementBaseActor(this);
		if ((ActualMovementBase != NULL) && !ActualMovementBase->CanBeBaseForCharacter(this))
		{
			CharacterMovement->JumpOff(ActualMovementBase);
		}
	}
}

void ACharacter::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	float Indent = 0.f;

	UFont* RenderFont = GEngine->GetSmallFont();

	static FName NAME_Physics = FName(TEXT("Physics"));
	if (DebugDisplay.IsDisplayOn(NAME_Physics) )
	{
		FIndenter PhysicsIndent(Indent);

		FString BaseString;
		if ( CharacterMovement == NULL || MovementBase == NULL )
		{
			BaseString = "Not Based";
		}
		else
		{
			BaseString = MovementBase->IsWorldGeometry() ? "World Geometry" : MovementBase->GetName();
			BaseString = FString::Printf(TEXT("Based On %s"), *BaseString);
		}
		
		Canvas->DrawText(RenderFont, FString::Printf(TEXT("RelativeLoc: %s RelativeRot: %s %s"), *RelativeMovement.Location.ToString(), *RelativeMovement.Rotation.ToString(), *BaseString), Indent, YPos);
		YPos += YL;

		if ( CharacterMovement != NULL )
		{
			CharacterMovement->DisplayDebug(Canvas, DebugDisplay, YL, YPos);
		}
		const bool Crouched = CharacterMovement && CharacterMovement->IsCrouching();
		FString T = FString::Printf(TEXT("Crouched %i"), Crouched);
		Canvas->DrawText(RenderFont, T, Indent, YPos );
		YPos += YL;
	}
}

void ACharacter::LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride)
{
	UE_LOG(LogCharacter, Verbose, TEXT("ACharacter::LaunchCharacter '%s' (%f,%f,%f)"), *GetName(), LaunchVelocity.X, LaunchVelocity.Y, LaunchVelocity.Z);

	if (CharacterMovement)
	{
		FVector FinalVel = LaunchVelocity;
		const FVector Velocity = GetVelocity();

		if (!bXYOverride)
		{
			FinalVel.X += Velocity.X;
			FinalVel.Y += Velocity.Y;
		}
		if (!bZOverride)
		{
			FinalVel.Z += Velocity.Z;
		}

		CharacterMovement->Launch(FinalVel);

		OnLaunched(LaunchVelocity, bXYOverride, bZOverride);
	}
}


void ACharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode)
{
	K2_OnMovementModeChanged(PrevMovementMode, CharacterMovement->MovementMode, PrevCustomMode, CharacterMovement->CustomMovementMode);
}


/** Don't process landed notification if updating client position by replaying moves. 
 * Allow event to be called if Pawn was initially falling (before starting to replay moves), 
 * and this is going to cause him to land. . */
bool ACharacter::NotifyLanded(const FHitResult& Hit)
{
	if (bClientUpdating && !bClientWasFalling)
	{
		return false;
	}

	// Just in case, only allow Landed() to be called once when replaying moves.
	bClientWasFalling = false;
	return true;
}


void ACharacter::CheckJumpInput(float DeltaTime)
{
	if (bPressedJump)
	{
		DoJump(bClientUpdating);
	}
}


void ACharacter::ClearJumpInput()
{
	bPressedJump = false;
}

bool ACharacter::ServerMove_Validate(float TimeStamp, FVector_NetQuantize100 InAccel, FVector_NetQuantize100 ClientLoc, uint8 MoveFlags, uint8 ClientRoll, uint32 View, class UPrimitiveComponent* ClientMovementBase, uint8 ClientMovementMode)
{
	return true;
}

void ACharacter::ServerMove_Implementation(float TimeStamp, FVector_NetQuantize100 InAccel, FVector_NetQuantize100 ClientLoc, uint8 MoveFlags, uint8 ClientRoll, uint32 View, class UPrimitiveComponent* ClientMovementBase, uint8 ClientMovementMode)
{
	if (CharacterMovement)
	{
		CharacterMovement->ServerMove_Implementation(TimeStamp, InAccel, ClientLoc, MoveFlags, ClientRoll, View, ClientMovementBase, ClientMovementMode);
	}
}

bool ACharacter::ServerMoveDual_Validate(float TimeStamp0, FVector_NetQuantize100 InAccel0, uint8 PendingFlags, uint32 View0, float TimeStamp, FVector_NetQuantize100 InAccel, FVector_NetQuantize100 ClientLoc, uint8 NewFlags, uint8 ClientRoll, uint32 View, class UPrimitiveComponent* ClientMovementBase, uint8 ClientMovementMode)
{
	return true;
}

void ACharacter::ServerMoveDual_Implementation(float TimeStamp0, FVector_NetQuantize100 InAccel0, uint8 PendingFlags, uint32 View0, float TimeStamp, FVector_NetQuantize100 InAccel, FVector_NetQuantize100 ClientLoc, uint8 NewFlags, uint8 ClientRoll, uint32 View, class UPrimitiveComponent* ClientMovementBase, uint8 ClientMovementMode)
{
	if (CharacterMovement)
	{
		CharacterMovement->ServerMoveDual_Implementation(TimeStamp0, InAccel0, PendingFlags, View0, TimeStamp, InAccel, ClientLoc, NewFlags, ClientRoll, View, ClientMovementBase, ClientMovementMode);
	}
}

bool ACharacter::ServerMoveOld_Validate(float OldTimeStamp, FVector_NetQuantize100 OldAccel, uint8 OldMoveFlags)
{
	return true;
}

void ACharacter::ServerMoveOld_Implementation(float OldTimeStamp, FVector_NetQuantize100 OldAccel, uint8 OldMoveFlags)
{
	if (CharacterMovement)
	{
		CharacterMovement->ServerMoveOld_Implementation(OldTimeStamp, OldAccel, OldMoveFlags);
	}
}

void ACharacter::ClientAckGoodMove_Implementation(float TimeStamp)
{
	if (CharacterMovement)
	{
		CharacterMovement->ClientAckGoodMove_Implementation(TimeStamp);
	}
}

void ACharacter::ClientAdjustPosition_Implementation(float TimeStamp, FVector NewLoc, FVector NewVel, class UPrimitiveComponent* NewBase, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode)
{
	if (CharacterMovement)
	{
		CharacterMovement->ClientAdjustPosition_Implementation(TimeStamp, NewLoc, NewVel, NewBase, bHasBase, bBaseRelativePosition, ServerMovementMode);
	}
}

void ACharacter::ClientVeryShortAdjustPosition_Implementation(float TimeStamp, FVector NewLoc, class UPrimitiveComponent* NewBase, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode)
{
	if (CharacterMovement)
	{
		CharacterMovement->ClientVeryShortAdjustPosition_Implementation(TimeStamp, NewLoc, NewBase, bHasBase, bBaseRelativePosition, ServerMovementMode);
	}
}

void ACharacter::ClientAdjustRootMotionPosition_Implementation(float TimeStamp, float ServerMontageTrackPosition, FVector ServerLoc, FVector_NetQuantizeNormal ServerRotation, float ServerVelZ, class UPrimitiveComponent * ServerBase, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode)
{
	if( CharacterMovement )
	{
		CharacterMovement->ClientAdjustRootMotionPosition_Implementation(TimeStamp, ServerMontageTrackPosition, ServerLoc, ServerRotation, ServerVelZ, ServerBase, bHasBase, bBaseRelativePosition, ServerMovementMode);
	}
}

//
// Static variables for networking.
//
static FVector_NetQuantize100	SavedRelativeLocation;
static FRotator					SavedRelativeRotation;
static uint8					SavedMovementMode;

void ACharacter::PreNetReceive()
{
	SavedRelativeLocation = RelativeMovement.Location;
	SavedRelativeRotation = RelativeMovement.Rotation;
	SavedMovementMode = ReplicatedMovementMode;

	Super::PreNetReceive();
}

void ACharacter::PostNetReceive()
{
	if (Role == ROLE_SimulatedProxy)
	{
		CharacterMovement->bNetworkUpdateReceived = true;
		CharacterMovement->bNetworkMovementModeChanged = (SavedMovementMode != ReplicatedMovementMode);
	}

	Super::PostNetReceive();

	// Skip base updates while playing root motion, it is handled inside of OnRep_RootMotion
	if (Role == ROLE_SimulatedProxy && !IsPlayingRootMotion())
	{
		PostNetReceiveBase();
	}
}

void ACharacter::PostNetReceiveBase()
{	
	if (Role != ROLE_SimulatedProxy)
	{
		return;
	}

	const bool bBaseChanged = (RelativeMovement.MovementBase != MovementBase);
	if (bBaseChanged)
	{
		SetBase(RelativeMovement.MovementBase);
	}

	if (RelativeMovement.HasRelativePosition())
	{
		if (bBaseChanged || (RelativeMovement.Location != SavedRelativeLocation) || (RelativeMovement.Rotation != SavedRelativeRotation))
		{
			const FVector OldLocation = GetActorLocation();
			const FRotator NewRotation = (FRotationMatrix(RelativeMovement.Rotation) * FRotationMatrix(MovementBase->GetComponentRotation())).Rotator();
			SetActorLocationAndRotation( MovementBase->GetComponentLocation() + RelativeMovement.Location, NewRotation, false);
			
			// When position or base changes, movement mode will need to be updated. This assumes rotation changes don't affect that.
			CharacterMovement->bJustTeleported |= (bBaseChanged || GetActorLocation() != OldLocation);

			INetworkPredictionInterface* PredictionInterface = InterfaceCast<INetworkPredictionInterface>(GetMovementComponent());
			if (PredictionInterface)
			{
				PredictionInterface->SmoothCorrection(OldLocation);
			}
		}
	}
}

void ACharacter::OnRep_ReplicatedMovement()
{
	// Skip standard position correction if we are playing root motion, OnRep_RootMotion will handle it.
	if( !IsPlayingRootMotion() )
	{
		Super::OnRep_ReplicatedMovement();
	}
}

/** Get FAnimMontageInstance playing RootMotion */
FAnimMontageInstance * ACharacter::GetRootMotionAnimMontageInstance() const
{
	return (Mesh && Mesh->AnimScriptInstance) ? Mesh->AnimScriptInstance->GetRootMotionMontageInstance() : NULL;
}

void ACharacter::OnRep_RootMotion()
{
	if( Role == ROLE_SimulatedProxy )
	{
		UE_LOG(LogRootMotion, Log,  TEXT("ACharacter::OnRep_RootMotion"));

		// Save received move in queue, we'll try to use it during Tick().
		if( RepRootMotion.AnimMontage )
		{
			// Add new move
			FSimulatedRootMotionReplicatedMove NewMove;
			NewMove.RootMotion = RepRootMotion;
			NewMove.Time = GetWorld()->GetTimeSeconds();
			RootMotionRepMoves.Add(NewMove);
		}
		else
		{
			// Clear saved moves.
			RootMotionRepMoves.Empty();
		}
	}
}

void ACharacter::SimulatedRootMotionPositionFixup(float DeltaSeconds)
{
	const FAnimMontageInstance * ClientMontageInstance = GetRootMotionAnimMontageInstance();
	if( ClientMontageInstance && CharacterMovement && Mesh )
	{
		// Find most recent buffered move that we can use.
		const int32 MoveIndex = FindRootMotionRepMove(*ClientMontageInstance);
		if( MoveIndex != INDEX_NONE )
		{
			const FVector OldLocation = GetActorLocation();
			// Move Actor back to position of that buffered move. (server replicated position).
			const FSimulatedRootMotionReplicatedMove & RootMotionRepMove = RootMotionRepMoves[MoveIndex];
			if( RestoreReplicatedMove(RootMotionRepMove) )
			{
				const float ServerPosition = RootMotionRepMove.RootMotion.Position;
				const float ClientPosition = ClientMontageInstance->Position;
				const float DeltaPosition = (ClientPosition - ServerPosition);
				if( FMath::Abs(DeltaPosition) > KINDA_SMALL_NUMBER )
				{
					// Find Root Motion delta move to get back to where we were on the client.
					const FTransform LocalRootMotionTransform = ClientMontageInstance->Montage->ExtractRootMotionFromTrackRange(ServerPosition, ClientPosition);

					// Simulate Root Motion for delta move.
					if( CharacterMovement )
					{
						// Guess time it takes for this delta track position, so we can get falling physics accurate.
						float DeltaTime = DeltaPosition / ClientMontageInstance->PlayRate;
						check( DeltaTime > 0.f );
						CharacterMovement->SimulateRootMotion(DeltaTime, LocalRootMotionTransform);

						// After movement correction, smooth out error in position if any.
						INetworkPredictionInterface* PredictionInterface = InterfaceCast<INetworkPredictionInterface>(GetMovementComponent());
						if (PredictionInterface)
						{
							PredictionInterface->SmoothCorrection(OldLocation);
						}
					}
				}
			}

			// Delete this move and any prior one, we don't need them anymore.
			UE_LOG(LogRootMotion, Log,  TEXT("\tClearing old moves (%d)"), MoveIndex+1);
			RootMotionRepMoves.RemoveAt(0, MoveIndex+1);
		}
	}
}

int32 ACharacter::FindRootMotionRepMove(const FAnimMontageInstance & ClientMontageInstance) const
{
	int32 FoundIndex = INDEX_NONE;

	// Start with most recent move and go back in time to find a usable move.
	for(int32 MoveIndex=RootMotionRepMoves.Num()-1; MoveIndex>=0; MoveIndex--)
	{
		if( CanUseRootMotionRepMove(RootMotionRepMoves[MoveIndex], ClientMontageInstance) )
		{
			FoundIndex = MoveIndex;
			break;
		}
	}

	UE_LOG(LogRootMotion, Log,  TEXT("\tACharacter::FindRootMotionRepMove FoundIndex: %d, NumSavedMoves: %d"), FoundIndex, RootMotionRepMoves.Num());
	return FoundIndex;
}

bool ACharacter::CanUseRootMotionRepMove(const FSimulatedRootMotionReplicatedMove & RootMotionRepMove, const FAnimMontageInstance & ClientMontageInstance) const
{
	// Ignore outdated moves.
	if( GetWorld()->TimeSince(RootMotionRepMove.Time) <= 0.5f )
	{
		// Make sure montage being played matched between client and server.
		if( RootMotionRepMove.RootMotion.AnimMontage && (RootMotionRepMove.RootMotion.AnimMontage == ClientMontageInstance.Montage) )
		{
			UAnimMontage * AnimMontage = ClientMontageInstance.Montage;
			const float ServerPosition = RootMotionRepMove.RootMotion.Position;
			const float ClientPosition = ClientMontageInstance.Position;
			const float DeltaPosition = (ClientPosition - ServerPosition);
			const int32 CurrentSectionIndex = AnimMontage->GetSectionIndexFromPosition(ClientPosition);
			if( CurrentSectionIndex != INDEX_NONE )
			{
				const int32 NextSectionIndex = (CurrentSectionIndex < ClientMontageInstance.NextSections.Num()) ? ClientMontageInstance.NextSections[CurrentSectionIndex] : INDEX_NONE;

				// We can only extract root motion if we are within the same section.
				// It's not trivial to jump through sections in a deterministic manner, but that is luckily not frequent. 
				const bool bSameSections = (AnimMontage->GetSectionIndexFromPosition(ServerPosition) == CurrentSectionIndex);
				// if we are looping and just wrapped over, skip. That's also not easy to handle and not frequent.
				const bool bHasLooped = (NextSectionIndex == CurrentSectionIndex) && (FMath::Abs(DeltaPosition) > (AnimMontage->GetSectionLength(CurrentSectionIndex) / 2.f));
				// Can only simulate forward in time, so we need to find a move from the server that is behind the client in time.
				const bool bClientAheadOfServer = ((DeltaPosition * ClientMontageInstance.PlayRate) >= 0.f);

				UE_LOG(LogRootMotion, Log,  TEXT("\t\tACharacter::CanUseRootMotionRepMove ServerPosition: %.3f, ClientPosition: %.3f, DeltaPosition: %.3f, bSameSections: %d, bHasLooped: %d, bClientAheadOfServer: %d"), 
					ServerPosition, ClientPosition, DeltaPosition, bSameSections, bHasLooped, bClientAheadOfServer);

				return bSameSections && !bHasLooped && bClientAheadOfServer;
			}
		}
	}
	return false;
}

bool ACharacter::RestoreReplicatedMove(const FSimulatedRootMotionReplicatedMove & RootMotionRepMove)
{
	UPrimitiveComponent * ServerBase = RootMotionRepMove.RootMotion.MovementBase;

	// Relative Position
	if( RootMotionRepMove.RootMotion.bRelativePosition )
	{
		bool bSuccess = false;
		if( MovementBaseUtility::UseRelativePosition(ServerBase) )
		{
			const FVector ServerLocation = ServerBase->GetComponentLocation() + RootMotionRepMove.RootMotion.Location;
			const FRotator ServerRotation = (FRotationMatrix(RootMotionRepMove.RootMotion.Rotation) * FRotationMatrix(ServerBase->GetComponentRotation())).Rotator();
			UpdateSimulatedPosition(ServerLocation, ServerRotation);
			bSuccess = true;
		}
		// If we received local space position, but can't resolve parent, then move can't be used. :(
		if( !bSuccess )
		{
			return false;
		}
	}
	// Absolute position
	else
	{
		UpdateSimulatedPosition(RootMotionRepMove.RootMotion.Location, RootMotionRepMove.RootMotion.Rotation);
	}

	// Set base
	if( MovementBase != ServerBase )
	{
		SetBase( ServerBase );
	}

	return true;
}

void ACharacter::UpdateSimulatedPosition(const FVector & NewLocation, const FRotator & NewRotation)
{
	// Always consider Location as changed if we were spawned this tick as in that case our replicated Location was set as part of spawning, before PreNetReceive()
	if( (NewLocation != GetActorLocation()) || (CreationTime == GetWorld()->TimeSeconds) )
	{
		FVector FinalLocation = NewLocation;
		if( GetWorld()->EncroachingBlockingGeometry(this, NewLocation, NewRotation) )
		{
			bSimGravityDisabled = true;
		}
		else
		{
			bSimGravityDisabled = false;
		}
		
		// Don't use TeleportTo(), that clears our base.
		SetActorLocationAndRotation(FinalLocation, NewRotation, false);
		CharacterMovement->bJustTeleported = true;
	}
	else if( NewRotation != GetActorRotation() )
	{
		GetRootComponent()->MoveComponent(FVector::ZeroVector, NewRotation, false);
	}
}

void ACharacter::PostNetReceiveLocation()
{
	if( Role == ROLE_SimulatedProxy )
	{
		// Don't change position if using relative position (it should be the same anyway)
		if (!RelativeMovement.HasRelativePosition())
		{
			const FVector OldLocation = GetActorLocation();
			UpdateSimulatedPosition(ReplicatedMovement.Location, ReplicatedMovement.Rotation);

			INetworkPredictionInterface* PredictionInterface = InterfaceCast<INetworkPredictionInterface>(GetMovementComponent());
			if (PredictionInterface)
			{
				PredictionInterface->SmoothCorrection(OldLocation);
			}
		}
	}
}

void ACharacter::PreReplication( IRepChangedPropertyTracker & ChangedPropertyTracker )
{
	Super::PreReplication( ChangedPropertyTracker );

	const FAnimMontageInstance * RootMotionMontageInstance = GetRootMotionAnimMontageInstance();

	if ( RootMotionMontageInstance )
	{
		// Is position stored in local space?
		RepRootMotion.bRelativePosition = RelativeMovement.HasRelativePosition();
		RepRootMotion.Location			= RepRootMotion.bRelativePosition ? RelativeMovement.Location : GetActorLocation();
		RepRootMotion.Rotation			= RepRootMotion.bRelativePosition ? RelativeMovement.Rotation : GetActorRotation();
		RepRootMotion.MovementBase		= RelativeMovement.MovementBase;
		RepRootMotion.AnimMontage		= RootMotionMontageInstance->Montage;
		RepRootMotion.Position			= RootMotionMontageInstance->Position;

		DOREPLIFETIME_ACTIVE_OVERRIDE( ACharacter, RepRootMotion, true );
	}
	else
	{
		RepRootMotion.Clear();

		DOREPLIFETIME_ACTIVE_OVERRIDE( ACharacter, RepRootMotion, false );
	}

	ReplicatedMovementMode = CharacterMovement->PackNetworkMovementMode();
}

void ACharacter::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME_CONDITION( ACharacter, RepRootMotion,		COND_SimulatedOnly );
	DOREPLIFETIME_CONDITION( ACharacter, RelativeMovement,	COND_SimulatedOnly );
	DOREPLIFETIME_CONDITION( ACharacter, ReplicatedMovementMode, COND_SimulatedOnly );

	DOREPLIFETIME( ACharacter, bIsCrouched ); // TODO: used to be !bNetOwner, now it's going to everyone (see change @revision #94)
}

void ACharacter::UpdateFromCompressedFlags(uint8 Flags)
{
	bPressedJump = ((Flags & FSavedMove_Character::FLAG_JumpPressed) != 0);	
	CharacterMovement->bWantsToCrouch = ((Flags & FSavedMove_Character::FLAG_WantsToCrouch) != 0);
}

bool ACharacter::IsPlayingRootMotion() const
{
	return (GetRootMotionAnimMontageInstance() != NULL);
}

float ACharacter::PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName)
{
	UAnimInstance * AnimInstance = (Mesh)? Mesh->GetAnimInstance() : NULL; 
	if( AnimMontage && AnimInstance )
	{
		float const Duration =  AnimInstance->Montage_Play(AnimMontage, InPlayRate);

		if (Duration > 0.f)
		{
			// Start at a given Section.
			if( StartSectionName != NAME_None )
			{
				AnimInstance->Montage_JumpToSection(StartSectionName);
			}

			return Duration;
		}
	}	

	return 0.f;
}

void ACharacter::StopAnimMontage(class UAnimMontage* AnimMontage)
{
	UAnimInstance * AnimInstance = (Mesh)? Mesh->GetAnimInstance() : NULL; 
	UAnimMontage * MontageToStop = (AnimMontage)? AnimMontage : GetCurrentMontage();
	bool bShouldStopMontage =  AnimInstance && MontageToStop && AnimInstance->Montage_IsPlaying(MontageToStop);

	if ( bShouldStopMontage )
	{
		AnimInstance->Montage_Stop(MontageToStop->BlendOutTime);
	}
}

class UAnimMontage * ACharacter::GetCurrentMontage()
{
	UAnimInstance * AnimInstance = (Mesh)? Mesh->GetAnimInstance() : NULL; 
	if ( AnimInstance )
	{
		return AnimInstance->GetCurrentActiveMontage();
	}

	return NULL;
}

void ACharacter::ClientCheatWalk_Implementation()
{
	SetActorEnableCollision(true);
	if (CharacterMovement)
	{
		CharacterMovement->bCheatFlying = false;
		CharacterMovement->SetMovementMode(MOVE_Falling);
	}
}

void ACharacter::ClientCheatFly_Implementation()
{
	SetActorEnableCollision(true);
	if (CharacterMovement)
	{
		CharacterMovement->bCheatFlying = true;
		CharacterMovement->SetMovementMode(MOVE_Flying);
	}
}

void ACharacter::ClientCheatGhost_Implementation()
{
	SetActorEnableCollision(false);
	if (CharacterMovement)
	{
		CharacterMovement->bCheatFlying = true;
		CharacterMovement->SetMovementMode(MOVE_Flying);
	}
}