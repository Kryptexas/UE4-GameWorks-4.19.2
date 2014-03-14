// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineClasses.h"
#include "VisualLog.h"

const FName UBrainComponent::AIMessage_MoveFinished = TEXT("MoveFinished");
const FName UBrainComponent::AIMessage_RepathFailed = TEXT("RepathFailed");
const FName UBrainComponent::AIMessage_QueryFinished = TEXT("QueryFinished");

//////////////////////////////////////////////////////////////////////////
// Messages

static UBrainComponent* FindBrainComponentHelper(const AController* Controller)
{
	return Controller ? Controller->FindComponentByClass<UBrainComponent>() : NULL;
}

static UBrainComponent* FindBrainComponentHelper(const APawn* Pawn)
{
	if (Pawn == NULL)
	{
		return NULL;
	}

	UBrainComponent* Comp = NULL;

	if (Pawn->Controller)
	{
		Comp = FindBrainComponentHelper(Pawn->Controller);
	}

	if (Pawn && Comp == NULL)
	{
		Comp = Pawn->FindComponentByClass<UBrainComponent>();
	}

	return Comp;
}

void FAIMessage::Send(AController* Controller, const FAIMessage& Message)
{
	UBrainComponent* BrainComp = FindBrainComponentHelper(Controller);
	Send(BrainComp, Message);
}

void FAIMessage::Send(APawn* Pawn, const FAIMessage& Message)
{
	UBrainComponent* BrainComp = FindBrainComponentHelper(Pawn);
	Send(BrainComp, Message);
}

void FAIMessage::Send(UBrainComponent* BrainComp, const FAIMessage& Message)
{
	if (BrainComp)
	{
		for (int32 i = 0; i < BrainComp->MessageObservers.Num(); i++)
		{
			BrainComp->MessageObservers[i]->OnMessage(Message);
		}
	}
}

void FAIMessage::Broadcast(UObject* WorldContextObject, const FAIMessage& Message)
{
	UWorld* MyWorld = GEngine->GetWorldFromContextObject(WorldContextObject);
	if (MyWorld)
	{
		for (FConstControllerIterator It = MyWorld->GetControllerIterator(); It; ++It)
		{
			FAIMessage::Send(*It, Message);
		}
	}
}

FAIMessageObserverHandle FAIMessageObserver::Create(AController* Controller, FName MessageType, FOnAIMessage const& Delegate)
{
	UBrainComponent* BrainComp = FindBrainComponentHelper(Controller);
	return Create(BrainComp, MessageType, Delegate);
}

FAIMessageObserverHandle FAIMessageObserver::Create(AController* Controller, FName MessageType, int32 MessageID, FOnAIMessage const& Delegate)
{
	UBrainComponent* BrainComp = FindBrainComponentHelper(Controller);
	return Create(BrainComp, MessageType, MessageID, Delegate);
}

FAIMessageObserverHandle FAIMessageObserver::Create(APawn* Pawn, FName MessageType, FOnAIMessage const& Delegate)
{
	UBrainComponent* BrainComp = FindBrainComponentHelper(Pawn);
	return Create(BrainComp, MessageType, Delegate);
}

FAIMessageObserverHandle FAIMessageObserver::Create(APawn* Pawn, FName MessageType, int32 MessageID, FOnAIMessage const& Delegate)
{
	UBrainComponent* BrainComp = FindBrainComponentHelper(Pawn);
	return Create(BrainComp, MessageType, MessageID, Delegate);
}

FAIMessageObserverHandle FAIMessageObserver::Create(UBrainComponent* BrainComp, FName MessageType, FOnAIMessage const& Delegate)
{
	FAIMessageObserverHandle ObserverHandle;
	if (BrainComp)
	{
		FAIMessageObserver* NewObserver = new FAIMessageObserver();
		NewObserver->MessageType = MessageType;
		NewObserver->bFilterByID = false;
		NewObserver->ObserverDelegate = Delegate;
		NewObserver->Register(BrainComp);

		ObserverHandle = MakeShareable(NewObserver);
	}

	return ObserverHandle;
}

FAIMessageObserverHandle FAIMessageObserver::Create(UBrainComponent* BrainComp, FName MessageType, int32 MessageID, FOnAIMessage const& Delegate)
{
	FAIMessageObserverHandle ObserverHandle;
	if (BrainComp)
	{
		FAIMessageObserver* NewObserver = new FAIMessageObserver();
		NewObserver->MessageType = MessageType;
		NewObserver->MessageID = MessageID;
		NewObserver->bFilterByID = true;
		NewObserver->ObserverDelegate = Delegate;
		NewObserver->Register(BrainComp);

		ObserverHandle = MakeShareable(NewObserver);
	}

	return ObserverHandle;
}

FAIMessageObserver::~FAIMessageObserver()
{
	Unregister();
}

void FAIMessageObserver::Register(class UBrainComponent* OwnerComp)
{
	OwnerComp->MessageObservers.Add(this);
	Owner = OwnerComp;
}

void FAIMessageObserver::Unregister()
{
	UBrainComponent* OwnerComp = Owner.Get();
	if (OwnerComp)
	{
		OwnerComp->MessageObservers.RemoveSingleSwap(this);
	}
}

void FAIMessageObserver::OnMessage(const FAIMessage& Message)
{
	if (Message.Message == MessageType)
	{
		if (!bFilterByID || Message.MessageID == MessageID)
		{
			ObserverDelegate.ExecuteIfBound(Owner.Get(), Message);
		}
	}
}

FString FAIMessageObserver::DescribeObservedMessage() const
{
	FString Description = MessageType.ToString();
	if (bFilterByID)
	{
		Description.AppendChar(TEXT(':'));
		Description.AppendInt(MessageID);
	}

	return Description;
}

//////////////////////////////////////////////////////////////////////////
// Brain component

UBrainComponent::UBrainComponent(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;
}

#if ENABLE_VISUAL_LOG
void UBrainComponent::DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const
{

}
#endif // ENABLE_VISUAL_LOG

void UBrainComponent::LockResource(EAILockSource::Type LockSource)
{
	const bool bWasLocked = ResourceLock.IsLocked();
	ResourceLock.SetLock(LockSource);
	if (bWasLocked == false)
	{
		PauseLogic(FString::Printf(TEXT("Locking Resource with source %s"), *ResourceLock.GetLockSourceName()));
	}
}

void UBrainComponent::ClearResourceLock(EAILockSource::Type LockSource)
{
	ResourceLock.ClearLock(LockSource);

	if (ResourceLock.IsLocked() == false)
	{
		ResumeLogic(TEXT("unlocked"));
	}
}

void UBrainComponent::ForceUnlockResource()
{
	ResourceLock.ForceClearAllLocks();
	ResumeLogic(TEXT("unlocked: forced"));
}

bool UBrainComponent::IsResourceLocked() const
{
	return ResourceLock.IsLocked();
}
