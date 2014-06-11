// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayDebuggerPrivate.h"
#include "GameplayDebuggingControllerComponent.h"
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#include "Misc/CoreMisc.h"

struct FGameplayDebuggerExec : public FSelfRegisteringExec
{
	FGameplayDebuggerExec()
	{
		CachedDebuggingReplicator = NULL;
	}

	TWeakObjectPtr<AGameplayDebuggingReplicator> GetDebuggingReplicator(UWorld* InWorld);

	// Begin FExec Interface
	virtual bool Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar) OVERRIDE;
	// End FExec Interface

	TWeakObjectPtr<AGameplayDebuggingReplicator> CachedDebuggingReplicator;
};
FGameplayDebuggerExec GameplayDebuggerExecInstance;


TWeakObjectPtr<AGameplayDebuggingReplicator> FGameplayDebuggerExec::GetDebuggingReplicator(UWorld* InWorld)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CachedDebuggingReplicator.IsValid() && CachedDebuggingReplicator->GetWorld() == InWorld)
	{
		return CachedDebuggingReplicator;
	}

	for (FActorIterator It(InWorld); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor != NULL && !Actor->IsPendingKill() && Actor->IsA(AGameplayDebuggingReplicator::StaticClass()))
		{
			CachedDebuggingReplicator = Cast<AGameplayDebuggingReplicator>(Actor);
			return CachedDebuggingReplicator;
		}
	}
#endif
	return NULL;
}

bool FGameplayDebuggerExec::Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	bool bHandled = false;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APlayerController* PC = Inworld ? Inworld->GetFirstPlayerController() : NULL;
	if (!Inworld || !PC)
	{
		return bHandled;
	}

	if (FParse::Command(&Cmd, TEXT("cheat EnableGDT")))
	{
		if (Inworld->GetNetMode() != NM_DedicatedServer && GetDebuggingReplicator(Inworld).IsValid() && !CachedDebuggingReplicator->IsToolCreated())
		{
			CachedDebuggingReplicator->CreateTool(PC);
			CachedDebuggingReplicator->EnableTool();
		}
	}
	else if (FParse::Command(&Cmd, TEXT("ToggleGameplayDebugView")))
	{
		static TArray<FString> ViewNames;
		if (ViewNames.Num() == 0)
		{
			const UEnum* ViewlEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EAIDebugDrawDataView"), true);
			ViewNames.AddZeroed(EAIDebugDrawDataView::MAX);
			for (int32 Index = 0; Index < EAIDebugDrawDataView::MAX; ++Index)
			{
				ViewNames[Index] = ViewlEnum->GetEnumName(Index);
			}
		}

		FString InViewName = FParse::Token(Cmd, 0);
		int32 ViewIndex = ViewNames.Find(InViewName);
		FGameplayDebuggerSettings::ShowFlagIndex = FEngineShowFlags::FindIndexByName(TEXT("GameplayDebug"));
		if (ViewIndex != INDEX_NONE)
		{
			FGameplayDebuggerSettings::CheckFlag(ViewIndex) ? FGameplayDebuggerSettings::ClearFlag(ViewIndex) : FGameplayDebuggerSettings::SetFlag(ViewIndex);
#if WITH_EDITOR
			if (ViewIndex == EAIDebugDrawDataView::EQS && GCurrentLevelEditingViewportClient)
			{
				GCurrentLevelEditingViewportClient->EngineShowFlags.SetSingleFlag(FGameplayDebuggerSettings::ShowFlagIndex, FGameplayDebuggerSettings::CheckFlag(ViewIndex));
			}
#endif
			PC->ClientMessage(FString::Printf(TEXT("View %s %s")
				, *InViewName
				, FGameplayDebuggerSettings::CheckFlag(ViewIndex) ? TEXT("enabled") : TEXT("disabled")));
		}
		else
		{
			PC->ClientMessage(TEXT("Unknown debug view name. Valid options are:"));
			for (int32 Index = 0; Index < EAIDebugDrawDataView::MAX; ++Index)
			{
				PC->ClientMessage(*ViewNames[Index]);
			}
		}
		bHandled = true;
	}
#endif

	return bHandled;
}

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)