// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PawnAction_Repeat.generated.h"

UCLASS()
class AIMODULE_API UPawnAction_Repeat : public UPawnAction
{
	GENERATED_UCLASS_BODY()

	enum
	{
		LoopForever = -1
	};

	/** Action to repeat. This instance won't really be run, it's a source for copying actions to be actually performed */
	UPROPERTY()
	UPawnAction* ActionToRepeat;

	UPROPERTY(Transient)
	UPawnAction* RecentActionCopy;

	UPROPERTY(Category = PawnAction, EditAnywhere, BlueprintReadOnly)
	TEnumAsByte<EPawnActionFailHandling::Type> ChildFailureHandlingMode;

	int32 RepeatsLeft;

	/** @param NumberOfRepeats number of times to repeat action. UPawnAction_Repeat::LoopForever loops forever */
	static UPawnAction_Repeat* CreateAction(class UWorld* World, class UPawnAction* ActionToRepeat, int32 NumberOfRepeats);

protected:
	virtual bool Start() override;
	virtual bool Resume() override;
	virtual void OnChildFinished(UPawnAction* Action, EPawnActionResult::Type WithResult) override;

	bool PushActionCopy();
};