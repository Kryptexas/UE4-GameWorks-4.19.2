// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayDebuggerPrivate.h"
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#include "GameplayDebuggingControllerComponent.h"
#include "Misc/CoreMisc.h"

#if WITH_EDITOR
#include "LevelEditorViewport.h"
#endif //WITH_EDITOR


struct FGameplayDebuggerExec : public FSelfRegisteringExec
{
	FGameplayDebuggerExec()
	{
		CachedDebuggingReplicator = NULL;
	}

	TWeakObjectPtr<AGameplayDebuggingReplicator> GetDebuggingReplicator(UWorld* InWorld);

	// Begin FExec Interface
	virtual bool Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	// End FExec Interface

	TWeakObjectPtr<AGameplayDebuggingReplicator> CachedDebuggingReplicator;
};
FGameplayDebuggerExec GameplayDebuggerExecInstance;


TWeakObjectPtr<AGameplayDebuggingReplicator> FGameplayDebuggerExec::GetDebuggingReplicator(UWorld* InWorld)
{
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

	return NULL;
}

bool FGameplayDebuggerExec::Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	bool bHandled = false;

	APlayerController* PC = Inworld ? Inworld->GetFirstPlayerController() : NULL;
	if (!Inworld || !PC)
	{
		return bHandled;
	}

	if (FParse::Command(&Cmd, TEXT("cheat EnableGDT")))
	{
		if (Inworld->GetNetMode() != NM_DedicatedServer)
		{
			AGameplayDebuggingReplicator* Replicator = NULL;
			for (FActorIterator It(Inworld); It; ++It)
			{
				AActor* A = *It;
				if (A && A->IsA(AGameplayDebuggingReplicator::StaticClass()) && !A->IsPendingKill())
				{
					Replicator = Cast<AGameplayDebuggingReplicator>(A);
					if (Replicator && Replicator->GetLocalPlayerOwner() == PC)
					{
						break;
					}
				}
			}

			if (Replicator && !Replicator->IsToolCreated())
			{
				Replicator->CreateTool();
				Replicator->EnableTool();
				bHandled = true;
			}
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

		const bool bActivePIE = Inworld && Inworld->IsGameWorld();
		AGameplayDebuggingReplicator* DebuggingReplicator = NULL;
		if (bActivePIE)
		{
			for (FActorIterator It(Inworld); It; ++It)
			{
				AActor* A = *It;
				if (A && A->IsA(AGameplayDebuggingReplicator::StaticClass()) && !A->IsPendingKill())
				{
					AGameplayDebuggingReplicator* Replicator = Cast<AGameplayDebuggingReplicator>(A);
					if (Replicator->GetLocalPlayerOwner() == PC)
					{
						DebuggingReplicator = Replicator;
						break;
					}
				}
			}
		}

		FGameplayDebuggerSettings DebuggerSettings = GameplayDebuggerSettings(DebuggingReplicator);
		FString InViewName = FParse::Token(Cmd, 0);
		int32 ViewIndex = ViewNames.Find(InViewName);
		FGameplayDebuggerSettings::ShowFlagIndex = FEngineShowFlags::FindIndexByName(TEXT("GameplayDebug"));
		if (ViewIndex != INDEX_NONE)
		{
			DebuggerSettings.CheckFlag(ViewIndex) ? DebuggerSettings.ClearFlag(ViewIndex) : DebuggerSettings.SetFlag(ViewIndex);
			if (bActivePIE)
			{
				GameplayDebuggerSettings().CheckFlag(ViewIndex) ? GameplayDebuggerSettings().ClearFlag(ViewIndex) : GameplayDebuggerSettings().SetFlag(ViewIndex);
			}
#if WITH_EDITOR
			if (ViewIndex == EAIDebugDrawDataView::EQS && GCurrentLevelEditingViewportClient)
			{
				GCurrentLevelEditingViewportClient->EngineShowFlags.SetSingleFlag(FGameplayDebuggerSettings::ShowFlagIndex, DebuggerSettings.CheckFlag(ViewIndex));
			}
#endif
			PC->ClientMessage(FString::Printf(TEXT("View %s %s")
				, *InViewName
				, DebuggerSettings.CheckFlag(ViewIndex) ? TEXT("enabled") : TEXT("disabled")));
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

	return bHandled;
}

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)