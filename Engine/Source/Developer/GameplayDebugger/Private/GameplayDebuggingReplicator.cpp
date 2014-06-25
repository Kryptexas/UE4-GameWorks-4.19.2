// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayDebuggerPrivate.h"
#include "GameFramework/HUD.h"
#include "GameplayDebuggingComponent.h"
#include "GameplayDebuggingHUDComponent.h"
#include "GameplayDebuggingReplicator.h"
#include "BehaviorTreeDelegates.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif // WITH_EDITOR
#include "UnrealNetwork.h"

AGameplayDebuggingReplicator::AGameplayDebuggingReplicator(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, LastDrawAtFrame(0)
{
	TSubobjectPtr<USceneComponent> SceneComponent = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComponent"));
	RootComponent = SceneComponent;

#if WITH_EDITORONLY_DATA
	SetTickableWhenPaused(true);
	SetIsTemporarilyHiddenInEditor(true);
	SetActorHiddenInGame(false);
	bHiddenEdLevel = true;
	bHiddenEdLayer = true;
	bHiddenEd = true;
	bEditable = false;
#endif

	DebuggerShowFlags =  GameplayDebuggerSettings().DebuggerShowFlags;

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		SetActorTickEnabled(true);

		bReplicates = false;
		SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
		SetReplicates(true);
	}
}

void AGameplayDebuggingReplicator::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		DOREPLIFETIME_CONDITION(AGameplayDebuggingReplicator, DebugComponent, COND_OwnerOnly);
		DOREPLIFETIME_CONDITION(AGameplayDebuggingReplicator, LocalPlayerOwner, COND_OwnerOnly);
#endif
}

bool AGameplayDebuggingReplicator::IsNetRelevantFor(class APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation)
{
	return LocalPlayerOwner == RealViewer;
}

void AGameplayDebuggingReplicator::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (GetWorld() && GetNetMode() != ENetMode::NM_DedicatedServer)
	{
		if (GIsEditor)
		{
			UDebugDrawService::Register(TEXT("DebugAI"), FDebugDrawDelegate::CreateUObject(this, &AGameplayDebuggingReplicator::OnDebugAIDelegate));
		}
		UDebugDrawService::Register(TEXT("Game"), FDebugDrawDelegate::CreateUObject(this, &AGameplayDebuggingReplicator::DrawDebugDataDelegate));

		if (!DebugComponentHUDClass.IsValid())
		{
			DebugComponentHUDClass = StaticLoadClass(AGameplayDebuggingHUDComponent::StaticClass(), NULL, *DebugComponentHUDClassName, NULL, LOAD_None, NULL);
			if (!DebugComponentHUDClass.IsValid())
			{
				DebugComponentHUDClass = AGameplayDebuggingHUDComponent::StaticClass();
			}
		}
	}

	if (!DebugComponentClass.IsValid() && GetWorld() && GetNetMode() < ENetMode::NM_Client)
	{
		DebugComponentClass = StaticLoadClass(UGameplayDebuggingComponent::StaticClass(), NULL, *DebugComponentClassName, NULL, LOAD_None, NULL);
		if (!DebugComponentClass.IsValid())
		{
			DebugComponentClass = UGameplayDebuggingComponent::StaticClass();
		}
	}
}

UGameplayDebuggingComponent* AGameplayDebuggingReplicator::GetDebugComponent()
{
	if (!DebugComponent && DebugComponentClass.IsValid() && GetNetMode() < ENetMode::NM_Client)
	{
		DebugComponent = ConstructObject<UGameplayDebuggingComponent>(DebugComponentClass.Get(), this);
		DebugComponent->SetIsReplicated(true);
		DebugComponent->RegisterComponent();
		DebugComponent->Activate();
	}

	return DebugComponent;
}

void AGameplayDebuggingReplicator::BeginPlay()
{
	Super::BeginPlay();
}

class UNetConnection* AGameplayDebuggingReplicator::GetNetConnection()
{
	if (LocalPlayerOwner)
	{
		return LocalPlayerOwner->GetNetConnection();
	}

	return NULL;
}

bool AGameplayDebuggingReplicator::ServerEnableTargetSelection_Validate(bool, APlayerController* )
{
	return true;
}

void AGameplayDebuggingReplicator::ServerEnableTargetSelection_Implementation(bool bEnable, APlayerController* Context)
{
	if (GetDebugComponent())
	{
		GetDebugComponent()->ServerEnableTargetSelection(true);
	}
}

bool AGameplayDebuggingReplicator::ClientReplicateMessage_Validate(class AActor* Actor, uint32 InMessage, uint32 DataView)
{
	return true;
}

void AGameplayDebuggingReplicator::ClientReplicateMessage_Implementation(class  AActor* Actor, uint32 InMessage, uint32 DataView)
{

}

bool AGameplayDebuggingReplicator::ServerReplicateMessage_Validate(class AActor* Actor, uint32 InMessage, uint32 DataView)
{
	return true;
}

void AGameplayDebuggingReplicator::ServerReplicateMessage_Implementation(class  AActor* Actor, uint32 InMessage, uint32 DataView)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (GetDebugComponent())
	{
		GetDebugComponent()->ServerReplicateData((EDebugComponentMessage::Type)InMessage, (EAIDebugDrawDataView::Type)DataView);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

bool AGameplayDebuggingReplicator::IsDrawEnabled()
{
	return bEnabledDraw && GetWorld() && GetNetMode() != ENetMode::NM_DedicatedServer;
}

void AGameplayDebuggingReplicator::EnableDraw(bool bEnable)
{
	bEnabledDraw = bEnable;

	if (AHUD* const GameHUD = LocalPlayerOwner ? LocalPlayerOwner->GetHUD() : NULL)
	{
		GameHUD->bShowHUD = bEnable ? false : true;
	}
	GEngine->bEnableOnScreenDebugMessages = bEnable ? false : true;
}

bool AGameplayDebuggingReplicator::IsToolCreated()
{
	UGameplayDebuggingControllerComponent*  GDC = FindComponentByClass<UGameplayDebuggingControllerComponent>();
	return LocalPlayerOwner && GDC;
}

void AGameplayDebuggingReplicator::CreateTool()
{
	if (GetWorld() && GetNetMode() != ENetMode::NM_DedicatedServer)
	{
		UGameplayDebuggingControllerComponent*  GDC = FindComponentByClass<UGameplayDebuggingControllerComponent>();
		if (!GDC)
		{
			GDC = ConstructObject<UGameplayDebuggingControllerComponent>(UGameplayDebuggingControllerComponent::StaticClass(), this);
			GDC->SetPlayerOwner(LocalPlayerOwner);
			GDC->RegisterComponent();
		}
	}
}

void AGameplayDebuggingReplicator::EnableTool()
{
	if (GetWorld() && GetNetMode() != ENetMode::NM_DedicatedServer)
	{
		UGameplayDebuggingControllerComponent*  GDC = FindComponentByClass<UGameplayDebuggingControllerComponent>();
		if (GDC)
		{
			// simulate key press
			GDC->OnActivationKeyPressed();
			GDC->OnActivationKeyReleased();
		}
	}
}

void AGameplayDebuggingReplicator::OnDebugAIDelegate(class UCanvas* Canvas, class APlayerController* PC)
{
#if WITH_EDITOR
	if (!GIsEditor)
	{
		return;
	}
	UEditorEngine* EEngine = Cast<UEditorEngine>(GEngine);
	if (GFrameNumber == LastDrawAtFrame || !EEngine || !EEngine->bIsSimulatingInEditor)
	{
		return;
	}
	LastDrawAtFrame = GFrameNumber;

	FEngineShowFlags EngineShowFlags = Canvas && Canvas->SceneView && Canvas->SceneView->Family ? Canvas->SceneView->Family->EngineShowFlags : FEngineShowFlags(GIsEditor ? EShowFlagInitMode::ESFIM_Editor : EShowFlagInitMode::ESFIM_Game);
	if (!EngineShowFlags.DebugAI)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World && GetDebugComponent() && GetDebugComponent()->GetOwnerRole() == ROLE_Authority)
	{
		bool bBroadcastedNewSelection = false;

		// looks like Simulate in UE4 Editor - let's find selected Pawn to debug
		for (FConstPawnIterator Iterator = World->GetPawnIterator(); Iterator; ++Iterator)
		{
			AActor* NewTarget = Cast<AActor>(*Iterator);

			//We needs to collect data manually in Simulate
			GetDebugComponent()->SetActorToDebug(NewTarget);
			GetDebugComponent()->SelectForDebugging(NewTarget->IsSelected());
			GetDebugComponent()->CollectDataToReplicate(NewTarget->IsSelected());

			if (NewTarget->IsSelected() && LastSelectedActorInSimulation.Get() != NewTarget)
			{
				LastSelectedActorInSimulation = NewTarget;
				UGameplayDebuggingComponent::OnDebuggingTargetChangedDelegate.Broadcast(NewTarget, NewTarget->IsSelected());
				APawn* TargetPawn = Cast<APawn>(NewTarget);
				if (TargetPawn)
				{
					FBehaviorTreeDelegates::OnDebugSelected.Broadcast(TargetPawn);
				}
				bBroadcastedNewSelection = true;
			}
			DrawDebugData(Canvas, PC);
		}

		if (!bBroadcastedNewSelection && LastSelectedActorInSimulation.IsValid() && !LastSelectedActorInSimulation->IsSelected() )
		{
			UGameplayDebuggingComponent::OnDebuggingTargetChangedDelegate.Broadcast(LastSelectedActorInSimulation.Get(), false);
			LastSelectedActorInSimulation = NULL;
		}
	}
#endif
}

void AGameplayDebuggingReplicator::DrawDebugDataDelegate(class UCanvas* Canvas, class APlayerController* PC)
{
#if !(UE_BUILD_SHIPPING && UE_BUILD_TEST)
	if (GetWorld() == NULL || IsPendingKill() || Canvas == NULL || Canvas->IsPendingKill())
	{
		return;
	}

	if (Canvas->SceneView != NULL && !Canvas->SceneView->bIsGameView)
	{
		return;
	}

	if (GFrameNumber == LastDrawAtFrame)
	{
		return;
	}
	LastDrawAtFrame = GFrameNumber;

	const UGameplayDebuggingControllerComponent*  GDC = FindComponentByClass<UGameplayDebuggingControllerComponent>();
	if (!IsDrawEnabled() || !GDC)
	{
		return;
	}

	if (GetWorld()->bPlayersOnly && Role == ROLE_Authority)
	{
		for (FConstPawnIterator Iterator = GetWorld()->GetPawnIterator(); Iterator; ++Iterator)
		{
			AActor* NewTarget = Cast<AActor>(*Iterator);
			GetDebugComponent()->SetActorToDebug(NewTarget);
			GetDebugComponent()->CollectDataToReplicate(true);
		}
	}

	DrawDebugData(Canvas, PC);
}

void AGameplayDebuggingReplicator::DrawDebugData(class UCanvas* Canvas, class APlayerController* PC)
{
	const bool bAllowToDraw = Canvas && Canvas->SceneView && (Canvas->SceneView->ViewActor == LocalPlayerOwner->AcknowledgedPawn || Canvas->SceneView->ViewActor == LocalPlayerOwner->GetPawnOrSpectator());
	if (!bAllowToDraw)
	{
		return;
	}

	if (!DebugRenderer.IsValid() && DebugComponentHUDClass.IsValid())
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = NULL;
		SpawnInfo.Instigator = NULL;
		SpawnInfo.bNoCollisionFail = true;

		DebugRenderer = GetWorld()->SpawnActor<AGameplayDebuggingHUDComponent>(DebugComponentHUDClass.Get(), SpawnInfo);
		DebugRenderer->SetCanvas(Canvas);
		DebugRenderer->SetPlayerOwner(LocalPlayerOwner);
		DebugRenderer->SetWorld(GetWorld());
	}

	if (DebugRenderer != NULL)
	{
		DebugRenderer->SetCanvas(Canvas);
		DebugRenderer->Render();
	}

#endif
}
