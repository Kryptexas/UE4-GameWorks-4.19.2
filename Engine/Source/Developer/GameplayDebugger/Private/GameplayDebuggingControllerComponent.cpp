// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "GameplayDebuggerPrivate.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/HUD.h"
#include "BehaviorTreeDelegates.h"
#include "VisualLog.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogGameplayDebugging, Log, All);

#define BUGIT_VIEWS (1<<EAIDebugDrawDataView::Basic) | (1 << EAIDebugDrawDataView::OverHead)
#define BREAK_LINE_TEXT TEXT("________________________________________________________________")

UGameplayDebuggingControllerComponent::UGameplayDebuggingControllerComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	bAutoActivate = false;
	bTickInEditor=true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	DebugAITargetActor = NULL;
	
	bToolActivated = false;
	bWaitingForOwnersComponent = false;

	ControlKeyPressedTime = 0;
}

void UGameplayDebuggingControllerComponent::OnRegister()
{
	Super::OnRegister();
	BindActivationKeys();
}

AGameplayDebuggingReplicator* UGameplayDebuggingControllerComponent::GetDebuggingReplicator()
{
	return Cast<AGameplayDebuggingReplicator>(GetOuter());
}

void UGameplayDebuggingControllerComponent::BeginDestroy()
{
	AHUD* GameHUD = PlayerOwner.IsValid() ? PlayerOwner->GetHUD() : NULL;
	if (GameHUD)
	{
		GameHUD->bShowHUD = true;
	}

	if (PlayerOwner.IsValid() && PlayerOwner->InputComponent && !PlayerOwner->IsPendingKill())
	{
		for (int32 Index = PlayerOwner->InputComponent->KeyBindings.Num() - 1; Index >= 0; --Index)
		{
			const FInputKeyBinding& KeyBind = PlayerOwner->InputComponent->KeyBindings[Index];
			if (KeyBind.KeyDelegate.IsBoundToObject(this))
			{
				PlayerOwner->InputComponent->KeyBindings.RemoveAtSwap(Index);
			}
		}
	}

	if (GetDebuggingReplicator() && PlayerOwner.IsValid() && !PlayerOwner->IsPendingKill())
	{
		APawn* Pawn = PlayerOwner->GetPawn();
		if (Pawn && !Pawn->IsPendingKill())
		{
			GetDebuggingReplicator()->ServerReplicateMessage(Pawn, EDebugComponentMessage::DeactivateReplilcation, 0);
		}

		if (DebugAITargetActor != NULL && !DebugAITargetActor->IsPendingKill())
		{
			for (uint32 Index = 0; Index < EAIDebugDrawDataView::MAX; ++Index)
			{
				GetDebuggingReplicator()->ServerReplicateMessage(DebugAITargetActor, EDebugComponentMessage::DeactivateDataView, (EAIDebugDrawDataView::Type)Index);
			}
			GetDebuggingReplicator()->ServerReplicateMessage(DebugAITargetActor, EDebugComponentMessage::DisableExtendedView);
		}
	}

	Super::BeginDestroy();
}

bool UGameplayDebuggingControllerComponent::IsViewActive(EAIDebugDrawDataView::Type View) const
{
	return FGameplayDebuggerSettings::CheckFlag(View);
}

uint32 UGameplayDebuggingControllerComponent::GetActiveViews()
{
	return FGameplayDebuggerSettings::DebuggerShowFlags;
}

void UGameplayDebuggingControllerComponent::SetActiveViews(uint32 InActiveViews)
{
	FGameplayDebuggerSettings::DebuggerShowFlags = InActiveViews;

	if (DebugAITargetActor && GetDebuggingReplicator())
	{
		GetDebuggingReplicator();
		for (uint32 Index = 0; Index < EAIDebugDrawDataView::MAX; ++Index)
		{
			GetDebuggingReplicator()->ServerReplicateMessage(DebugAITargetActor, IsViewActive((EAIDebugDrawDataView::Type)Index) ? EDebugComponentMessage::ActivateDataView : EDebugComponentMessage::DeactivateDataView, (EAIDebugDrawDataView::Type)Index);
		}
	}
}

void UGameplayDebuggingControllerComponent::EnableActiveView(EAIDebugDrawDataView::Type View, bool bEnable)
{
	bEnable ? FGameplayDebuggerSettings::SetFlag(View) : FGameplayDebuggerSettings::ClearFlag(View);

	if (DebugAITargetActor && GetDebuggingReplicator())
	{
		GetDebuggingReplicator()->ServerReplicateMessage(DebugAITargetActor, bEnable ? EDebugComponentMessage::ActivateDataView : EDebugComponentMessage::DeactivateDataView, View);
#if WITH_EQS
		if (DebugAITargetActor->GetDebugComponent() && View == EAIDebugDrawDataView::EQS)
		{
			DebugAITargetActor->GetDebugComponent()->EnableClientEQSSceneProxy(IsViewActive(EAIDebugDrawDataView::EQS));
		}
#endif // WITH_EQS
	}
}

void UGameplayDebuggingControllerComponent::BindActivationKeys()
{
	if (PlayerOwner.IsValid() && PlayerOwner->InputComponent && PlayerOwner->PlayerInput)
	{
		// find current activation key used for 'EnableGDT' binding
		FInputChord ActivationKey(EKeys::Quote, false, false, false);
		for (uint32 BindIndex = 0; BindIndex < (uint32)PlayerOwner->PlayerInput->DebugExecBindings.Num(); BindIndex++)
		{
			if (PlayerOwner->PlayerInput->DebugExecBindings[BindIndex].Command == TEXT("cheat EnableGDT"))
			{
				ActivationKey.Key = PlayerOwner->PlayerInput->DebugExecBindings[BindIndex].Key;
				ActivationKey.bCtrl = PlayerOwner->PlayerInput->DebugExecBindings[BindIndex].Control;
				ActivationKey.bAlt = PlayerOwner->PlayerInput->DebugExecBindings[BindIndex].Alt;
				ActivationKey.bShift = PlayerOwner->PlayerInput->DebugExecBindings[BindIndex].Shift;
				break;
			}
		}

		PlayerOwner->InputComponent->BindKey(ActivationKey, IE_Pressed, this, &UGameplayDebuggingControllerComponent::OnActivationKeyPressed);
		PlayerOwner->InputComponent->BindKey(ActivationKey, IE_Released, this, &UGameplayDebuggingControllerComponent::OnActivationKeyReleased);
	}
}

void UGameplayDebuggingControllerComponent::OnActivationKeyPressed()
{
	if (GetDebuggingReplicator() && PlayerOwner.IsValid())
	{
		if (!bToolActivated)
		{
			Activate();
			SetComponentTickEnabled(true);

			BindAIDebugViewKeys();
			GetDebuggingReplicator()->EnableDraw(true);
		}

		EnableTargetSelection(true);
		ControlKeyPressedTime = GetWorld()->GetTimeSeconds();
	}
}

void UGameplayDebuggingControllerComponent::OnActivationKeyReleased()
{
	const float KeyPressedTime = GetWorld()->GetTimeSeconds() - ControlKeyPressedTime;

	EnableTargetSelection(false);
	if (GetDebuggingReplicator() && bToolActivated)
	{
		if (KeyPressedTime < 0.5f)
		{
			CloseDebugTool();
		}
		else
		{
			APawn* TargetPawn = GetDebuggingReplicator()->GetDebugComponent() ? Cast<APawn>(GetDebuggingReplicator()->GetDebugComponent()->GetSelectedActor()) : NULL;
			if (TargetPawn != NULL)
			{
				FBehaviorTreeDelegates::OnDebugLocked.Broadcast(TargetPawn);
			}
		}
	}
}

void UGameplayDebuggingControllerComponent::OpenDebugTool()
{
	if (GetDebuggingReplicator() && IsActive())
	{
		bToolActivated = true;
	}

}

void UGameplayDebuggingControllerComponent::CloseDebugTool()
{
	if (GetDebuggingReplicator())
	{
		Deactivate();
		SetComponentTickEnabled(false);
		GetDebuggingReplicator()->EnableDraw(false);
		bToolActivated = false;
	}
}

void UGameplayDebuggingControllerComponent::EnableTargetSelection(bool bEnable)
{
	GetDebuggingReplicator()->ServerEnableTargetSelection(bEnable, PlayerOwner.Get());
}

void UGameplayDebuggingControllerComponent::BindAIDebugViewKeys()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!AIDebugViewInputComponent)
	{
		AIDebugViewInputComponent = ConstructObject<UInputComponent>(UInputComponent::StaticClass(), GetOwner(), TEXT("AIDebugViewInputComponent0"));
		AIDebugViewInputComponent->RegisterComponent();

		AIDebugViewInputComponent->BindKey(EKeys::NumPadZero, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView0);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView1);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView2);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView3);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadFour, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView4);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadFive, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView5);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadSix, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView6);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadSeven, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView7);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadEight, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView8);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadNine, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView9);
	}
	
	if (PlayerOwner.IsValid())
	{
		PlayerOwner->PushInputComponent(AIDebugViewInputComponent);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

bool UGameplayDebuggingControllerComponent::CanToggleView()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	return  (PlayerOwner.IsValid() && bToolActivated /*&& DebugAITargetActor*/);
#else
	return false;
#endif
}

void UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView0()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APawn* Pawn = PlayerOwner.IsValid() ? PlayerOwner->GetPawn() : NULL;

	if (PlayerOwner.IsValid() && Pawn && bToolActivated && GetDebuggingReplicator())
	{
		if (UGameplayDebuggingComponent* OwnerComp = GetDebuggingReplicator() ? GetDebuggingReplicator()->GetDebugComponent() : NULL)
		{
			FGameplayDebuggerSettings::DebuggerShowFlags ^= 1 << EAIDebugDrawDataView::NavMesh;

			if (IsViewActive(EAIDebugDrawDataView::NavMesh))
			{
				GetWorld()->GetTimerManager().SetTimer(this, &UGameplayDebuggingControllerComponent::UpdateNavMeshTimer, 5.0f, true);
				UpdateNavMeshTimer();

				GetDebuggingReplicator()->ServerReplicateMessage(Pawn, EDebugComponentMessage::ActivateDataView, EAIDebugDrawDataView::NavMesh);
				GetDebuggingReplicator()->ServerReplicateMessage(Pawn, EDebugComponentMessage::ActivateReplication, EAIDebugDrawDataView::Empty);
				OwnerComp->SetVisibility(true, true);
				OwnerComp->SetHiddenInGame(false, true);
				OwnerComp->MarkRenderStateDirty();
			}
			else
			{
				GetWorld()->GetTimerManager().ClearTimer(this, &UGameplayDebuggingControllerComponent::UpdateNavMeshTimer);

				GetDebuggingReplicator()->ServerReplicateMessage(Pawn, EDebugComponentMessage::DeactivateDataView, EAIDebugDrawDataView::NavMesh);
				GetDebuggingReplicator()->ServerReplicateMessage(Pawn, EDebugComponentMessage::DeactivateReplilcation, EAIDebugDrawDataView::Empty);
				OwnerComp->ServerDiscardNavmeshData();
				OwnerComp->SetVisibility(false, true);
				OwnerComp->SetHiddenInGame(true, true);
				OwnerComp->MarkRenderStateDirty();
			}
		}
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#	define DEFAULT_TOGGLE_HANDLER(__func_name__, __DataView__) \
	void UGameplayDebuggingControllerComponent::__func_name__() \
	{ \
		if (CanToggleView()) \
		{ \
			ToggleActiveView(__DataView__); \
		} \
	}

#else
#	define DEFAULT_TOGGLE_HANDLER(__func_name__, __DataView__) \
	void UGameplayDebuggingControllerComponent::__func_name__() { }
#endif 

DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView1, EAIDebugDrawDataView::Basic);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView2, EAIDebugDrawDataView::BehaviorTree);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView3, EAIDebugDrawDataView::EQS);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView4, EAIDebugDrawDataView::Perception);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView5, EAIDebugDrawDataView::GameView1);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView6, EAIDebugDrawDataView::GameView2);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView7, EAIDebugDrawDataView::GameView3);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView8, EAIDebugDrawDataView::GameView4);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView9, EAIDebugDrawDataView::GameView5);

#ifdef DEFAULT_TOGGLE_HANDLER
# undef DEFAULT_TOGGLE_HANDLER
#endif

void UGameplayDebuggingControllerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetWorld()->GetTimeSeconds() - ControlKeyPressedTime > 0.5f && !bToolActivated)
	{
		OpenDebugTool();
	}

	if (!bToolActivated)
	{
		return;
	}
}

void UGameplayDebuggingControllerComponent::UpdateNavMeshTimer()
{
	APawn* Pawn = PlayerOwner.IsValid() ? PlayerOwner->GetPawn() : NULL;
	UGameplayDebuggingComponent* DebuggingComponent = GetDebuggingReplicator() ? GetDebuggingReplicator()->GetDebugComponent() : NULL;
	if (DebuggingComponent)
	{
		const FVector AdditionalTargetLoc =
			DebugAITargetActor ? DebugAITargetActor->GetNavAgentLocation() :
			Pawn ? Pawn->GetNavAgentLocation() : 
			FVector::ZeroVector;

		if (AdditionalTargetLoc != FVector::ZeroVector)
		{
			DebuggingComponent->ServerCollectNavmeshData(AdditionalTargetLoc);
		}
	}
}
