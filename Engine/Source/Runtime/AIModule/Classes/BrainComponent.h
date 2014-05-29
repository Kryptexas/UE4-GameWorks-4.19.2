// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AITypes.h"
#include "AIResourceInterface.h"
#include "BrainComponent.generated.h"


DECLARE_DELEGATE_TwoParams(FOnAIMessage, class UBrainComponent*, const struct FAIMessage&);

DECLARE_LOG_CATEGORY_EXTERN(LogBrain, Warning, All);

struct AIMODULE_API FAIMessage
{
	enum EStatus
	{
		Failure,
		Success,
	};

	/** type of message */
	FName MessageName;

	/** message source */
	UObject* Sender;

	/** message param: ID */
	FAIRequestID RequestID;

	/** message param: status */
	TEnumAsByte<EStatus> Status;

	FAIMessage() : MessageName(NAME_None), Sender(NULL), RequestID(0), Status(FAIMessage::Success) {}
	FAIMessage(FName InMessage, UObject* InSender) : MessageName(InMessage), Sender(InSender), RequestID(0), Status(FAIMessage::Success) {}
	FAIMessage(FName InMessage, UObject* InSender, FAIRequestID InID, EStatus InStatus) : MessageName(InMessage), Sender(InSender), RequestID(InID), Status(InStatus) {}
	FAIMessage(FName InMessage, UObject* InSender, FAIRequestID InID, bool bSuccess) : MessageName(InMessage), Sender(InSender), RequestID(InID), Status(bSuccess ? Success : Failure) {}
	FAIMessage(FName InMessage, UObject* InSender, EStatus InStatus) : MessageName(InMessage), Sender(InSender), RequestID(0), Status(InStatus) {}
	FAIMessage(FName InMessage, UObject* InSender, bool bSuccess) : MessageName(InMessage), Sender(InSender), RequestID(0), Status(bSuccess ? Success : Failure) {}

	static void Send(class AController* Controller, const FAIMessage& Message);
	static void Send(class APawn* Pawn, const FAIMessage& Message);
	static void Send(class UBrainComponent* BrainComp, const FAIMessage& Message);

	static void Broadcast(class UObject* WorldContextObject, const FAIMessage& Message);
};

typedef TSharedPtr<struct FAIMessageObserver, ESPMode::Fast> FAIMessageObserverHandle;

struct AIMODULE_API FAIMessageObserver : public TSharedFromThis<FAIMessageObserver>
{
public:

	static FAIMessageObserverHandle Create(class AController* Controller, FName MessageType, FOnAIMessage const& Delegate);
	static FAIMessageObserverHandle Create(class AController* Controller, FName MessageType, FAIRequestID MessageID, FOnAIMessage const& Delegate);

	static FAIMessageObserverHandle Create(APawn* Pawn, FName MessageType, FOnAIMessage const& Delegate);
	static FAIMessageObserverHandle Create(APawn* Pawn, FName MessageType, FAIRequestID MessageID, FOnAIMessage const& Delegate);

	static FAIMessageObserverHandle Create(UBrainComponent* BrainComp, FName MessageType, FOnAIMessage const& Delegate);
	static FAIMessageObserverHandle Create(UBrainComponent* BrainComp, FName MessageType, FAIRequestID MessageID, FOnAIMessage const& Delegate);

	~FAIMessageObserver();

	void OnMessage(const FAIMessage& Message);
	FString DescribeObservedMessage() const;
	
	FORCEINLINE FName GetObservedMessageType() const { return MessageType; }
	FORCEINLINE FAIRequestID GetObservedMessageID() const { return MessageID; }
	FORCEINLINE bool IsObservingMessageID() const { return bFilterByID; }

private:

	void Register(class UBrainComponent* OwnerComp);
	void Unregister();

	/** observed message type */
	FName MessageType;

	/** filter: message ID */
	FAIRequestID MessageID;
	bool bFilterByID;

	/** delegate to call */
	FOnAIMessage ObserverDelegate;

	/** brain component owning this observer */
	TWeakObjectPtr<UBrainComponent> Owner;
};

UCLASS(abstract, BlueprintType)
class AIMODULE_API UBrainComponent : public UActorComponent, public IAIResourceInterface
{
	GENERATED_UCLASS_BODY()

protected:
	/** blackboard component */
	UPROPERTY(transient)
	class UBlackboardComponent* BlackboardComp;

	// @TODO this is a temp contraption to implement delayed messages delivering
	// until proper AI messaging is implemented
	TArray<FAIMessage> MessagesToProcess;

public:
	virtual FString GetDebugInfoString() const { return TEXT(""); }

	/** To be called in case we want to restart AI logic while it's still being locked.
	 *	On subsequent ResumeLogic instead RestartLogic will be called. 
	 *	@note this call does nothing if logic is not locked at the moment of call */
	void RequestLogicRestartOnUnlock();
	virtual void RestartLogic() {}
protected:
	virtual void StopLogic(const FString& Reason) {}
	virtual void PauseLogic(const FString& Reason) {}
	/** MUST be called by child implementations!
	 *	@return indicates whether child class' ResumeLogic should be called (true) or has it been 
	 *	handled in a different way and no other actions are required (false)*/
	virtual EAILogicResuming::Type ResumeLogic(const FString& Reason);
public:
	virtual bool IsRunning() const { return false; }
	virtual bool IsPaused() const { return false; }

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG
	
	// IAIResourceInterface begin
	virtual void LockResource(EAILockSource::Type LockSource) OVERRIDE;
	virtual void ClearResourceLock(EAILockSource::Type LockSource) OVERRIDE;
	virtual void ForceUnlockResource() OVERRIDE;
	virtual bool IsResourceLocked() const OVERRIDE;
	// IAIResourceInterface end
	
	virtual void HandleMessage(const FAIMessage& Message);
	
	/** BEGIN UActorComponent overrides */
	virtual void InitializeComponent() OVERRIDE;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction);
	/** END UActorComponent overrides */

	/** caches BlackboardComponent's pointer to be used with this brain component */
	void CacheBlackboardComponent(class UBlackboardComponent* BBComp);

	/** @return blackboard used with this component */
	class UBlackboardComponent* GetBlackboardComponent();

	/** @return blackboard used with this component */
	const class UBlackboardComponent* GetBlackboardComponent() const;

protected:

	/** active message observers */
	TArray<FAIMessageObserver*> MessageObservers;

	friend struct FAIMessageObserver;
	friend struct FAIMessage;

private:
	/** used to keep track of which subsystem requested this AI resource be locked */
	FAIResourceLock ResourceLock;

	uint32 bDoLogicRestartOnUnlock : 1;

public:
	// static names to be used with SendMessage. Fell free to define game-specific
	// messages anywhere you want
	static const FName AIMessage_MoveFinished;
	static const FName AIMessage_RepathFailed;
	static const FName AIMessage_QueryFinished;
};

//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE class UBlackboardComponent* UBrainComponent::GetBlackboardComponent()
{
	return BlackboardComp;
}

FORCEINLINE const class UBlackboardComponent* UBrainComponent::GetBlackboardComponent() const
{
	return BlackboardComp;
}