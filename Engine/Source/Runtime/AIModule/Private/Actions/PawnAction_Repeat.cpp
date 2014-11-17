// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Actions/PawnAction_Repeat.h"

UPawnAction_Repeat::UPawnAction_Repeat(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ChildFailureHandlingMode = EPawnActionFailHandling::IgnoreFailure;
}

UPawnAction_Repeat* UPawnAction_Repeat::CreateAction(UWorld* World, UPawnAction* ActionToRepeat, int32 NumberOfRepeats)
{
	if (ActionToRepeat == NULL || !(NumberOfRepeats > 0 || NumberOfRepeats == UPawnAction_Repeat::LoopForever))
	{
		return NULL;
	}

	UPawnAction_Repeat* Action = UPawnAction::CreateActionInstance<UPawnAction_Repeat>(World);
	if (Action)
	{
		Action->ActionToRepeat = ActionToRepeat;
		Action->RepeatsLeft = NumberOfRepeats;
	}

	return Action;
}

bool UPawnAction_Repeat::Start()
{
	bool bResult = Super::Start();
	
	if (bResult)
	{
		UE_VLOG(GetPawn(), LogPawnAction, Log, TEXT("Starting repeating action: %s. Requested repeats: %d")
			, *GetNameSafe(ActionToRepeat), RepeatsLeft);
		bResult = PushActionCopy();
	}

	return bResult;
}

bool UPawnAction_Repeat::Resume()
{
	bool bResult = Super::Resume();

	if (bResult)
	{
		bResult = PushActionCopy();
	}

	return bResult;
}

void UPawnAction_Repeat::OnChildFinished(UPawnAction* Action, EPawnActionResult::Type WithResult)
{
	if (RecentActionCopy == Action)
	{
		if (WithResult == EPawnActionResult::Success || (WithResult == EPawnActionResult::Failed && ChildFailureHandlingMode == EPawnActionFailHandling::IgnoreFailure))
		{
			PushActionCopy();
		}
		else
		{
			Finish(EPawnActionResult::Failed);
		}
	}

	Super::OnChildFinished(Action, WithResult);
}

bool UPawnAction_Repeat::PushActionCopy()
{
	if (ActionToRepeat == NULL)
	{
		Finish(EPawnActionResult::Failed);
		return false;
	}
	else if (RepeatsLeft == 0)
	{
		Finish(EPawnActionResult::Success);
		return true;
	}

	if (RepeatsLeft > 0)
	{
		--RepeatsLeft;
	}

	UPawnAction* ActionCopy = Cast<UPawnAction>(StaticDuplicateObject(ActionToRepeat, this, NULL));
	UE_VLOG(GetPawn(), LogPawnAction, Log, TEXT("%s> pushing repeted action copy %s, repeats left: %d")
		, *GetName(), *GetNameSafe(ActionCopy), RepeatsLeft);
	ensure(ActionCopy);
	RecentActionCopy = ActionCopy;
	return PushChildAction(ActionCopy); 
}
