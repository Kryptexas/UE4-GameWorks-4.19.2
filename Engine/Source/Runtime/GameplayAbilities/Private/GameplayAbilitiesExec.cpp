// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#include "Misc/CoreMisc.h"

struct FGameplayAbilitiesExec : public FSelfRegisteringExec
{
	FGameplayAbilitiesExec()
	{
	}

	// Begin FExec Interface
	virtual bool Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	// End FExec Interface
};

FGameplayAbilitiesExec GameplayAbilitiesExecInstance;

bool FGameplayAbilitiesExec::Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (Inworld == NULL)
	{
		return false;
	}

	UAbilitySystemGlobals& AbilitySystemGlobals = UAbilitySystemGlobals::Get();
	bool bHandled = false;

	if (FParse::Command(&Cmd, TEXT("ToggleIgnoreAbilitySystemCooldowns")))
	{
		AbilitySystemGlobals.ToggleIgnoreAbilitySystemCooldowns();
		bHandled = true;
	}
	else if (FParse::Command(&Cmd, TEXT("ToggleIgnoreAbilitySystemCosts")))
	{
		AbilitySystemGlobals.ToggleIgnoreAbilitySystemCosts();
		bHandled = true;
	}

	return bHandled;
}

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)