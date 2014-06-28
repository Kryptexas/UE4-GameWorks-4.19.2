// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Actions/PawnActionsComponent.h"

DEFINE_LOG_CATEGORY(LogPawnAction);

//----------------------------------------------------------------------//
// helpers
//----------------------------------------------------------------------//

namespace
{
	FString GetEventName(int32 Index)
	{
		static const UEnum* Enum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPawnActionEventType"));
		check(Enum);
		return Enum->GetEnumName(Index);
	}

	FString GetPriorityName(int32 Index)
	{
		static const UEnum* Enum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EAIRequestPriority"));
		check(Enum);
		return Enum->GetEnumName(Index);
	}

	FString GetActionSignature(UPawnAction* Action)
	{
		if (Action == NULL)
		{
			return TEXT("NULL");
		}
		
		return FString::Printf(TEXT("[%s, %s]"), *Action->GetName(), *GetPriorityName(Action->GetPriority()));
	}
}

//----------------------------------------------------------------------//
// FPawnActionEvent
//----------------------------------------------------------------------//

namespace
{
	struct FPawnActionEvenSort
	{
		FORCEINLINE bool operator()(const FPawnActionEvent& A, const FPawnActionEvent& B) const
		{
			return A.Priority < B.Priority
				|| (A.Priority == B.Priority
					&& (A.EventType < B.EventType
						|| (A.EventType == B.EventType && A.Index < B.Index)));
		}
	};
}

FPawnActionEvent::FPawnActionEvent(UPawnAction* InAction, EPawnActionEventType::Type InEventType, uint32 InIndex)
	: Action(InAction), EventType(InEventType), Index(InIndex)
{
	ensure(InAction);
	if (InAction)
	{
		Priority = InAction->GetPriority();
	}
}

//----------------------------------------------------------------------//
// FPawnActionStack
//----------------------------------------------------------------------//

void FPawnActionStack::Pause()
{
	if (TopAction != NULL)
	{
		TopAction->Pause();
	}
}

void FPawnActionStack::Resume()
{
	if (TopAction != NULL)
	{
		TopAction->Resume();
	}
}

void FPawnActionStack::PushAction(UPawnAction* NewTopAction)
{
	if (TopAction != NULL)
	{
		TopAction->Pause();
		ensure(TopAction->ChildAction == NULL);
		TopAction->ChildAction = NewTopAction;
		NewTopAction->ParentAction = TopAction;
	}

	TopAction = NewTopAction;
	NewTopAction->OnPushed();
}

void FPawnActionStack::PopAction(UPawnAction* ActionToPop)
{
	if (ActionToPop == NULL)
	{
		return;
	}

	// first check if it's there
	UPawnAction* CutPoint = TopAction;
	while (CutPoint != NULL && CutPoint != ActionToPop)
	{
		CutPoint = CutPoint->ParentAction;
	}

	if (CutPoint == ActionToPop)
	{
		UPawnAction* ActionBeingRemoved = TopAction;
		// note StopAction can be null
		UPawnAction* StopAction = ActionToPop->ParentAction;

		while (ActionBeingRemoved != StopAction)
		{
			UPawnAction* NextAction = ActionBeingRemoved->ParentAction;
			
			ActionBeingRemoved->OnPopped();
			if (ActionBeingRemoved->ParentAction)
			{
				ActionBeingRemoved->ParentAction->OnChildFinished(ActionBeingRemoved, ActionBeingRemoved->FinishResult);
			}
			ActionBeingRemoved = NextAction;
		}

		TopAction = StopAction;
	}
}

//----------------------------------------------------------------------//
// UPawnActionsComponent
//----------------------------------------------------------------------//

UPawnActionsComponent::UPawnActionsComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	bAutoActivate = true;
	bLockedAILogic = false;

	ActionEventIndex = 0;

	ActionStacks.AddZeroed(EAIRequestPriority::MAX);
}

void UPawnActionsComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ControlledPawn == NULL)
	{
		CacheControlledPawn();
	}

	if (ActionEvents.Num() > 1)
	{
		ActionEvents.Sort(FPawnActionEvenSort());
	}

	if (ActionEvents.Num() > 0)
	{
		for (int32 EventIndex = 0; EventIndex < ActionEvents.Num(); ++EventIndex)
		{
			FPawnActionEvent& Event = ActionEvents[EventIndex];
			switch (Event.EventType)
			{
			case EPawnActionEventType::FinishedAborting:
			case EPawnActionEventType::FinishedExecution:
			case EPawnActionEventType::FailedToStart:
				ActionStacks[Event.Priority].PopAction(Event.Action);
				break;
			case EPawnActionEventType::Push:
				ActionStacks[Event.Priority].PushAction(Event.Action);
				break;
			default:
				break;
			}
		}

		ActionEvents.Reset();

		UpdateCurrentAction();
	}

	if (CurrentAction)
	{
		CurrentAction->TickAction(DeltaTime);
	}

	// it's possible we got new events with CurrentAction's tick
	if (ActionEvents.Num() == 0 && (CurrentAction == NULL || CurrentAction->WantsTick() == false))
	{
		SetComponentTickEnabled(false);
	}
}

void UPawnActionsComponent::UpdateCurrentAction()
{
	UE_VLOG(ControlledPawn, LogPawnAction, Log, TEXT("Picking new current actions. Old CurrentAction %s")
		, *GetActionSignature(CurrentAction));

	UPawnAction* NewCurrentAction = NULL;
	int32 Priority = EAIRequestPriority::MAX - 1;
	do 
	{
		NewCurrentAction = ActionStacks[Priority].GetTop();

	} while (NewCurrentAction == NULL && --Priority >= 0);

	if (CurrentAction != NewCurrentAction)
	{
		UE_VLOG(ControlledPawn, LogPawnAction, Log, TEXT("New action: %s")
			, *GetActionSignature(NewCurrentAction));

		if (CurrentAction != NULL && CurrentAction->IsActive())
		{
			CurrentAction->Pause();
		}
		CurrentAction = NewCurrentAction;
		bool bNewActionStartedSuccessfully = true;
		if (CurrentAction != NULL)
		{
			bNewActionStartedSuccessfully = CurrentAction->Activate();
		}

		if (bNewActionStartedSuccessfully == false)
		{
			UE_VLOG(ControlledPawn, LogPawnAction, Warning, TEXT("CurrentAction %s failed to activate. Removing and re-running action selection")
				, *GetActionSignature(NewCurrentAction));

			CurrentAction = NULL;			
		}
		// @HACK temporary solution to have actions and old BT tasks work together
		else if (CurrentAction == NULL || CurrentAction->GetPriority() != EAIRequestPriority::Logic)
		{
			UpdateAILogicLock();
		}
	}
	else
	{
		if (CurrentAction == NULL)
		{
			UpdateAILogicLock();
		}
		else
		{ 
			UE_VLOG(ControlledPawn, LogPawnAction, Warning, TEXT("Still doing the same action"));
		}
	}
}

void UPawnActionsComponent::UpdateAILogicLock()
{
	if (ControlledPawn && ControlledPawn->GetController())
	{
		UBrainComponent* BrainComp = ControlledPawn->GetController()->FindComponentByClass<UBrainComponent>();
		if (BrainComp)
		{
			if (CurrentAction != NULL && CurrentAction->GetPriority() > EAIRequestPriority::Logic)
			{
				UE_VLOG(ControlledPawn, LogPawnAction, Log, TEXT("Locking AI logic"));
				BrainComp->LockResource(EAILockSource::Script);
				bLockedAILogic = true;
			}
			else if (bLockedAILogic)
			{
				UE_VLOG(ControlledPawn, LogPawnAction, Log, TEXT("Clearing AI logic lock"));
				bLockedAILogic = false;
				BrainComp->ClearResourceLock(EAILockSource::Script);
				if (BrainComp->IsResourceLocked() == false)
				{
					UE_VLOG(ControlledPawn, LogPawnAction, Log, TEXT("Reseting AI logic"));
					BrainComp->RestartLogic();
				}
				// @todo consider checking if lock priority is < Logic
				else
				{
					UE_VLOG(ControlledPawn, LogPawnAction, Log, TEXT("AI logic still locked with other priority"));
					BrainComp->RequestLogicRestartOnUnlock();
				}
			}
		}
	}
}

bool UPawnActionsComponent::AbortAction(UPawnAction* ActionToAbort)
{
	if (ActionToAbort == NULL)
	{
		return false;
	}

	ActionToAbort->Abort(EAIForceParam::DoNotForce);
	return true;
}

uint32 UPawnActionsComponent::AbortActionsInstigatedBy(UObject* const Instigator, EAIRequestPriority::Type Priority)
{
	uint32 AbortedActionsCount = 0;

	if (Priority == EAIRequestPriority::MAX)
	{
		// call for every regular priority 
		for (int32 PriorityIndex = 0; PriorityIndex < EAIRequestPriority::MAX; ++PriorityIndex)
		{
			AbortedActionsCount += AbortActionsInstigatedBy(Instigator, EAIRequestPriority::Type(PriorityIndex));
		}
	}
	else
	{
		UPawnAction* Action = ActionStacks[Priority].GetTop();
		while (Action)
		{
			if (Action->GetInstigator() == Instigator)
			{
				Action->Abort(EAIForceParam::Force);
				++AbortedActionsCount;
			}
			Action = Action->ParentAction;
		}
	}

	return AbortedActionsCount;
}

bool UPawnActionsComponent::PushAction(UPawnAction* NewAction, EAIRequestPriority::Type Priority, UObject* const Instigator)
{
	NewAction->ExecutionPriority = Priority;
	NewAction->SetOwnerComponent(this);
	NewAction->SetInstigator(Instigator);
	return OnEvent(NewAction, EPawnActionEventType::Push);
}

bool UPawnActionsComponent::OnEvent(UPawnAction* Action, EPawnActionEventType::Type Event)
{
	if (Action == NULL || Event == EPawnActionEventType::Invalid)
	{
		// ignore
		UE_VLOG(ControlledPawn, LogPawnAction, Warning, TEXT("Ignoring Action Event: Action %s Event %s")
			, *GetNameSafe(Action), *GetEventName(Event));
		return false;
	}
	ActionEvents.Add(FPawnActionEvent(Action, Event, ActionEventIndex++));

	// if it's a first even enable tick
	if (ActionEvents.Num() == 1)
	{
		SetComponentTickEnabled(true);
	}

	return true;
}

void UPawnActionsComponent::SetControlledPawn(APawn* NewPawn)
{
	if (ControlledPawn != NULL && ControlledPawn != NewPawn)
	{
		UE_VLOG(ControlledPawn, LogPawnAction, Warning, TEXT("Trying to set ControlledPawn to new value while ControlledPawn is already set!"));
	}
	else
	{
		ControlledPawn = NewPawn;
	}
}

APawn* UPawnActionsComponent::CacheControlledPawn()
{
	if (ControlledPawn == NULL)
	{
		AActor* ActorOwner = GetOwner();
		if (ActorOwner)
		{
			ControlledPawn = Cast<APawn>(ActorOwner);
			if (ControlledPawn == NULL)
			{
				AController* Controller = Cast<AController>(ActorOwner);
				if (Controller != NULL)
				{
					ControlledPawn = Controller->GetPawn();
				}
			}
		}
	}

	return ControlledPawn;
}


//----------------------------------------------------------------------//
// blueprint interface
//----------------------------------------------------------------------//
bool UPawnActionsComponent::PerformAction(APawn* Pawn, UPawnAction* Action, TEnumAsByte<EAIRequestPriority::Type> Priority)
{
	bool bSuccess = false;

	ensure(Priority < EAIRequestPriority::MAX);

	if (Pawn && Pawn->GetController() && Action)
	{
		UPawnActionsComponent* ActionComp = Pawn->GetController()->FindComponentByClass<UPawnActionsComponent>();
		if (ActionComp)
		{
			ActionComp->PushAction(Action, Priority);
			bSuccess = true;
		}
	}

	return bSuccess;
}

//----------------------------------------------------------------------//
// debug
//----------------------------------------------------------------------//
#if ENABLE_VISUAL_LOG
void UPawnActionsComponent::DescribeSelfToVisLog(FVisLogEntry* Snapshot) const
{
	static const FString Category = TEXT("PawnActions");

	if (IsPendingKill())
	{
		return;
	}

	for (int32 PriorityIndex = 0; PriorityIndex < ActionStacks.Num(); ++PriorityIndex)
	{
		const UPawnAction* Action = ActionStacks[PriorityIndex].GetTop();
		if (Action == NULL)
		{
			continue;
		}

		FVisLogEntry::FStatusCategory StatusCategory;
		StatusCategory.Category = Category + TEXT(": ") + GetPriorityName(PriorityIndex);

		while (Action)
		{
			StatusCategory.Add(Action->GetName(), Action->GetStateDescription());
			Action = Action->GetParentAction();
		}

		Snapshot->Status.Add(StatusCategory);
	}
}
#endif // ENABLE_VISUAL_LOG