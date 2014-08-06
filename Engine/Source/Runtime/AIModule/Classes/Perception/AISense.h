// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AIPerceptionTypes.h"
#include "AISense.generated.h"

DECLARE_DELEGATE_OneParam(FOnPerceptionListenerUpdateDelegate, const FPerceptionListener&);

UCLASS(ClassGroup=AI, abstract)
class AIMODULE_API UAISense : public UObject
{
	GENERATED_UCLASS_BODY()

	static const float SuspendNextUpdate;

private:
	UPROPERTY()
	class UAIPerceptionSystem* PerceptionSystemInstance;

	/** then this count reaches 0 sense will be updated */
	float TimeUntilNextUpdate;

protected:
	/**	If bound will be called when new FPerceptionListener gets registers with AIPerceptionSystem */
	FOnPerceptionListenerUpdateDelegate OnNewListenerDelegate;

	/**	If bound will be called when a FPerceptionListener's in AIPerceptionSystem change */
	FOnPerceptionListenerUpdateDelegate OnListenerUpdateDelegate;

	/**	If bound will be called when a FPerceptionListener's in removed from AIPerceptionSystem */
	FOnPerceptionListenerUpdateDelegate OnListenerRemovedDelegate;
			
public:
	static FAISenseId GetSenseIndex() { check(0 && "This should never get called!"); return AIPerception::InvalidSenseIndex; }

	virtual void PostInitProperties() override;

	/** 
	 *	@return should this sense be ticked now
	 */
	bool ProgressTime(float DeltaSeconds)
	{
		TimeUntilNextUpdate -= DeltaSeconds;
		return TimeUntilNextUpdate <= 0.f;
	}

	void Tick()
	{
		if (TimeUntilNextUpdate <= 0.f)
		{
			TimeUntilNextUpdate = Update();
		}
	}

	virtual void RegisterSource(class AActor* SourceActor) {}

	FORCEINLINE void OnNewListener(const FPerceptionListener& NewListener) { OnNewListenerDelegate.ExecuteIfBound(NewListener); }
	FORCEINLINE void OnListenerUpdate(const FPerceptionListener& NewListener) { OnListenerUpdateDelegate.ExecuteIfBound(NewListener); }
	FORCEINLINE void OnListenerRemoved(const FPerceptionListener& NewListener) { OnListenerRemovedDelegate.ExecuteIfBound(NewListener); }

protected:
	/** @return time until next update */
	virtual float Update() { return FLT_MAX; }

	/** will result in updating as soon as possible */
	FORCEINLINE void RequestImmediateUpdate() { TimeUntilNextUpdate = 0.f; }

	/** will result in updating in specified number of seconds */
	FORCEINLINE void RequestUpdateInSeconds(float UpdateInSeconds) { TimeUntilNextUpdate = UpdateInSeconds; }

	FORCEINLINE UAIPerceptionSystem* GetPerceptionSystem() { return PerceptionSystemInstance; }

	/** returning pointer rather then a reference to prevent users from
	 *	accidentally creating copies by creating non-reference local vars */
	AIPerception::FListenerMap* GetListeners();
};
