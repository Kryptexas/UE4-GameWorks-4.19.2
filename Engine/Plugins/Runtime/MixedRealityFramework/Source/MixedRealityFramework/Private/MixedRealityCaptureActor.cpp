// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MixedRealityCaptureActor.h"
#include "MixedRealityCaptureComponent.h"
#include "Engine/Engine.h" // for GEngine
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "MixedRealityUtilLibrary.h"
#include "GameFramework/Pawn.h"
#include "Tickable.h" // for FTickableGameObject
#include "MixedRealityBillboard.h"
#include "ISpectatorScreenController.h"
#include "Engine/TextureRenderTarget2D.h"
#include "IXRTrackingSystem.h" // for GetHMDDevice()
#include "IHeadMountedDisplay.h" // for GetSpectatorScreenController()

#define LOCTEXT_NAMESPACE "MRCaptureActor"

/* MixedRealityCaptureActor_Impl
 *****************************************************************************/

namespace MixedRealityCaptureActor_Impl
{
	static bool AssignTargetPlayer(AMixedRealityCaptureActor* CaptureActor, const bool bAutoAttach);
}

static bool MixedRealityCaptureActor_Impl::AssignTargetPlayer(AMixedRealityCaptureActor* CaptureActor, const bool bAutoAttach)
{
	bool bSuccess = false;

	if (UWorld* TargetWorld = CaptureActor->GetWorld())
	{
		APawn* FallbackPlayer = nullptr;

		const TArray<ULocalPlayer*>& LocalPlayers = GEngine->GetGamePlayers(TargetWorld);
		for (ULocalPlayer* Player : LocalPlayers)
		{
			if (APlayerController* Controller = Player->GetPlayerController(TargetWorld))
			{
				APawn* PlayerPawn = Controller->GetPawn();
				if (FallbackPlayer == nullptr)
				{
					FallbackPlayer = PlayerPawn;
				}

				USceneComponent* TrackingOrigin = UMixedRealityUtilLibrary::GetHMDRootComponent(PlayerPawn);
				if (TrackingOrigin)
				{
					bSuccess = CaptureActor->SetTargetPlayer(PlayerPawn, bAutoAttach ? TrackingOrigin : nullptr);
					if (bSuccess)
					{
						FallbackPlayer = nullptr;
						break;
					}
				}
			}
		}

		if (FallbackPlayer)
		{
			bSuccess = CaptureActor->SetTargetPlayer(FallbackPlayer, bAutoAttach ? FallbackPlayer->GetRootComponent() : nullptr);
		}
	}

	return bSuccess;
}

/* FMRCaptureAutoTargeter
 *****************************************************************************/

class FMRCaptureAutoTargeter : public FTickableGameObject
{
public:
	FMRCaptureAutoTargeter(AMixedRealityCaptureActor* Owner, const bool bAutoAttach);

public:
	//~ FTickableObjectBase interface

	virtual bool IsTickable() const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	AMixedRealityCaptureActor* Owner;
	bool bAutoAttach;
};

FMRCaptureAutoTargeter::FMRCaptureAutoTargeter(AMixedRealityCaptureActor* InOwner, const bool bAutoAttachIn)
	: Owner(InOwner)
	, bAutoAttach(bAutoAttachIn)
{}


bool FMRCaptureAutoTargeter::IsTickable() const
{
	return Owner && (Owner->GetRootComponent()->GetAttachParent() == nullptr);
}

void FMRCaptureAutoTargeter::Tick(float /*DeltaTime*/)
{
	MixedRealityCaptureActor_Impl::AssignTargetPlayer(Owner, bAutoAttach);
}

TStatId FMRCaptureAutoTargeter::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FPlayerAttachment, STATGROUP_ThreadPoolAsyncTasks);
}

/* AMixedRealityCaptureActor
 *****************************************************************************/

AMixedRealityCaptureActor::AMixedRealityCaptureActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bAutoAttachToVRPlayer(true)
	, bAutoHidePlayer(true)
	, bHideAttachmentsWithPlayer(true)
	, bAutoBroadcast(true)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TrackingSpaceOrigin"));

	CaptureComponent = CreateDefaultSubobject<UMixedRealityCaptureComponent>(TEXT("CaptureComponent"));
	CaptureComponent->SetupAttachment(RootComponent);
}

bool AMixedRealityCaptureActor::SetTargetPlayer(APawn* PlayerPawn, USceneComponent* AttachTo)
{
	ensure(!AttachTo || AttachTo->GetOwner() == PlayerPawn);

	if (TargetPlayer.IsValid())
	{
		TargetPlayer->OnDestroyed.RemoveDynamic(this, &AMixedRealityCaptureActor::OnTargetDestroyed);
		CaptureComponent->HiddenActors.Remove(TargetPlayer.Get());
		TargetPlayer.Reset();
	}

	bool bSuccess = true;
	if (AttachTo)
	{
		AttachToComponent(AttachTo, FAttachmentTransformRules::KeepRelativeTransform);
		bSuccess = RootComponent->IsAttachedTo(AttachTo);
	}

	TargetPlayer = PlayerPawn;
	if (bAutoHidePlayer)
	{
		CaptureComponent->HiddenActors.AddUnique(PlayerPawn);

		TArray<AActor*> PlayerAttachments;
		if (bHideAttachmentsWithPlayer)
		{
			PlayerPawn->GetAttachedActors(PlayerAttachments);
		}
		else
		{
			PlayerPawn->GetAllChildActors(PlayerAttachments);
		}

		for (AActor* Attachment : PlayerAttachments)
		{
			if (Attachment != this)
			{
				CaptureComponent->HiddenActors.AddUnique(Attachment);
			}
		}
	}
	PlayerPawn->OnDestroyed.AddDynamic(this, &AMixedRealityCaptureActor::OnTargetDestroyed);

	AMixedRealityProjectionActor* ProjectionActor = CaptureComponent->GetProjectionActor();
	if (ProjectionActor)
	{
		ProjectionActor->SetDepthTarget(PlayerPawn);
	}

	if (bSuccess)
	{
		AutoTargeter.Reset();
	}

	return bSuccess;
}

void AMixedRealityCaptureActor::SetAutoBroadcast(const bool bNewValue)
{
	if (bAutoBroadcast != bNewValue)
	{
		if (HasActorBegunPlay())
		{
			if (!bNewValue)
			{
				BroadcastManager.EndCasting();
			}
			else
			{
				BroadcastManager.BeginCasting(CaptureComponent->TextureTarget);
			}
		}
		bAutoBroadcast = bNewValue;
	}
}

void AMixedRealityCaptureActor::BeginPlay()
{
	if (bAutoAttachToVRPlayer || bAutoHidePlayer)
	{
		const bool bPlayerFound = MixedRealityCaptureActor_Impl::AssignTargetPlayer(this, bAutoAttachToVRPlayer);
		if (!bPlayerFound)
		{
			AutoTargeter = MakeShareable(new FMRCaptureAutoTargeter(this, bAutoAttachToVRPlayer));
		}
	}

	if (bAutoBroadcast)
	{
		BroadcastManager.BeginCasting(CaptureComponent->TextureTarget);
	}

	Super::BeginPlay();
}

void AMixedRealityCaptureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	BroadcastManager.EndCasting();
}

void AMixedRealityCaptureActor::OnTargetDestroyed(AActor* DestroyedActor)
{
	USceneComponent* CurrentAttachment = RootComponent->GetAttachParent();
	if (CurrentAttachment && CurrentAttachment->GetOwner() == DestroyedActor)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		
		if (bAutoAttachToVRPlayer)
		{
			AutoTargeter = MakeShareable(new FMRCaptureAutoTargeter(this, bAutoAttachToVRPlayer));
		}
	}
}

/* AMixedRealityCaptureActor::FCastingModeRestore
 *****************************************************************************/

namespace CastingModeRestore_Impl
{
	ISpectatorScreenController* GetSpectatorScreenController()
	{
		IHeadMountedDisplay* HMD = GEngine->XRSystem.IsValid() ? GEngine->XRSystem->GetHMDDevice() : nullptr;
		if (HMD)
		{
			return HMD->GetSpectatorScreenController();
		}
		return nullptr;
	}
}

AMixedRealityCaptureActor::FCastingModeRestore::FCastingModeRestore()
	: RestoreTexture(nullptr)
	, RestoreMode(ESpectatorScreenMode::SingleEyeCroppedToFill)
	, bIsCasting(false)
{}

bool AMixedRealityCaptureActor::FCastingModeRestore::BeginCasting(UTexture* DisplayTexture)
{
	EndCasting();

	ISpectatorScreenController* const Controller = CastingModeRestore_Impl::GetSpectatorScreenController();
	if (Controller)
	{
		RestoreTexture = Controller->GetSpectatorScreenTexture();
		Controller->SetSpectatorScreenTexture(DisplayTexture);

		RestoreMode = Controller->GetSpectatorScreenMode();
		Controller->SetSpectatorScreenMode(ESpectatorScreenMode::Texture);
	}

	return bIsCasting;
}

void AMixedRealityCaptureActor::FCastingModeRestore::EndCasting()
{
	// @TODO: not perfect, if someone external (say Blueprints) overwrote the spectator screen, then this may be undesired
	if (bIsCasting)
	{
		ISpectatorScreenController* const Controller = CastingModeRestore_Impl::GetSpectatorScreenController();
		if (Controller)
		{
			Controller->SetSpectatorScreenMode(RestoreMode);
			Controller->SetSpectatorScreenTexture(RestoreTexture);
		}
		bIsCasting = false;
	}
}

#undef LOCTEXT_NAMESPACE
