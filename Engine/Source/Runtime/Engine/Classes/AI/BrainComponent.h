// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BrainComponent.generated.h"

DECLARE_DELEGATE_TwoParams(FOnAIMessage, UBrainComponent*, const struct FAIMessage&);

struct ENGINE_API FAIMessage
{
	enum EStatus
	{
		Failure,
		Success,
	};

	/** type of message */
	FName Message;

	/** message source */
	UObject* Sender;

	/** message param: ID */
	int32 MessageID;

	/** message param: status */
	TEnumAsByte<EStatus> Status;

	FAIMessage() : Message(NAME_None), Sender(NULL), MessageID(0), Status(FAIMessage::Success) {}
	FAIMessage(FName InMessage, UObject* InSender) : Message(InMessage), Sender(InSender), MessageID(0), Status(FAIMessage::Success) {}
	FAIMessage(FName InMessage, UObject* InSender, int32 InID, EStatus InStatus) : Message(InMessage), Sender(InSender), MessageID(InID), Status(InStatus) {}
	FAIMessage(FName InMessage, UObject* InSender, int32 InID, bool bSuccess) : Message(InMessage), Sender(InSender), MessageID(InID), Status(bSuccess ? Success : Failure) {}
	FAIMessage(FName InMessage, UObject* InSender, EStatus InStatus) : Message(InMessage), Sender(InSender), MessageID(0), Status(InStatus) {}
	FAIMessage(FName InMessage, UObject* InSender, bool bSuccess) : Message(InMessage), Sender(InSender), MessageID(0), Status(bSuccess ? Success : Failure) {}

	static void Send(AController* Controller, const FAIMessage& Message);
	static void Send(APawn* Pawn, const FAIMessage& Message);
	static void Send(UBrainComponent* BrainComp, const FAIMessage& Message);

	static void Broadcast(UObject* WorldContextObject, const FAIMessage& Message);
};

typedef TSharedPtr<struct FAIMessageObserver, ESPMode::Fast> FAIMessageObserverHandle;

struct ENGINE_API FAIMessageObserver : public TSharedFromThis<FAIMessageObserver>
{
public:

	static FAIMessageObserverHandle Create(AController* Controller, FName MessageType, FOnAIMessage const& Delegate);
	static FAIMessageObserverHandle Create(AController* Controller, FName MessageType, int32 MessageID, FOnAIMessage const& Delegate);

	static FAIMessageObserverHandle Create(APawn* Pawn, FName MessageType, FOnAIMessage const& Delegate);
	static FAIMessageObserverHandle Create(APawn* Pawn, FName MessageType, int32 MessageID, FOnAIMessage const& Delegate);

	static FAIMessageObserverHandle Create(UBrainComponent* BrainComp, FName MessageType, FOnAIMessage const& Delegate);
	static FAIMessageObserverHandle Create(UBrainComponent* BrainComp, FName MessageType, int32 MessageID, FOnAIMessage const& Delegate);

	~FAIMessageObserver();

	void OnMessage(const FAIMessage& Message);
	FString DescribeObservedMessage() const;
	
	FORCEINLINE FName GetObservedMessageType() const { return MessageType; }
	FORCEINLINE int32 GetObservedMessageID() const { return MessageID; }
	FORCEINLINE bool IsObservingMessageID() const { return bFilterByID; }

private:

	void Register(class UBrainComponent* OwnerComp);
	void Unregister();

	/** observed message type */
	FName MessageType;

	/** filter: message ID */
	int32 MessageID;
	bool bFilterByID;

	/** delegate to call */
	FOnAIMessage ObserverDelegate;

	/** brain component owning this observer */
	TWeakObjectPtr<UBrainComponent> Owner;
};

UCLASS(HeaderGroup=Component, abstract, BlueprintType)
class ENGINE_API UBrainComponent : public UActorComponent, public IAIResourceInterface
{
	GENERATED_UCLASS_BODY()

	virtual FString GetDebugInfoString() const { return TEXT(""); }

	virtual void RestartLogic() {}
protected:
	virtual void StopLogic(const FString& Reason) {}
	virtual void PauseLogic(const FString& Reason) {}
	virtual void ResumeLogic(const FString& Reason) {}
public:
	virtual bool IsRunning() const { return false; }

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG
	
	// IAIResourceInterface begin
	virtual void LockResource(EAILockSource::Type LockSource) OVERRIDE;
	virtual void ClearResourceLock(EAILockSource::Type LockSource) OVERRIDE;
	virtual void ForceUnlockResource() OVERRIDE;
	virtual bool IsResourceLocked() const OVERRIDE;
	// IAIResourceInterface end

protected:

	/** active message observers */
	TArray<FAIMessageObserver*> MessageObservers;

	friend struct FAIMessageObserver;
	friend struct FAIMessage;

private:
	/** used to keep track of which subsystem requested this AI resource be locked */
	FAIResourceLock ResourceLock;

public:
	// static names to be used with SendMessage. Fell free to define game-specific
	// messages anywhere you want
	static const FName AIMessage_MoveFinished;
	static const FName AIMessage_RepathFailed;
	static const FName AIMessage_QueryFinished;
};
