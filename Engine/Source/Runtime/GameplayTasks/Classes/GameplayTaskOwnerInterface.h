// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "Interface.h"
#include "GameplayTaskOwnerInterface.generated.h"

class UGameplayTasksComponent;
class UGameplayTask;
class AActor;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UGameplayTaskOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

class GAMEPLAYTASKS_API IGameplayTaskOwnerInterface
{
	GENERATED_BODY()
public:
	virtual void OnTaskInitialized(UGameplayTask& Task);
	virtual UGameplayTasksComponent* GetGameplayTasksComponent() = 0;
	virtual void TaskStarted(UGameplayTask& NewTask) = 0;
	virtual void TaskEnded(UGameplayTask& Task) = 0;
	virtual AActor* GetOwnerActor() const = 0;
	virtual AActor* GetAvatarActor() const;
};