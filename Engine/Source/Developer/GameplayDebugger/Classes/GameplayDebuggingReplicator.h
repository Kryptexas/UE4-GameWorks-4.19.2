// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameplayDebuggingReplicator.generated.h"

/**
*	Transient actor used to communicate between server and client, mostly for RPC
*/

class UGameplayDebuggingComponent;
class AGameplayDebuggingHUDComponent;

struct FDebugContext
{
	explicit FDebugContext(APlayerController* PC) : bEnabledTargetSelection(false), PlayerOwner(PC) {}
	FDebugContext(const FDebugContext& DC) : bEnabledTargetSelection(DC.bEnabledTargetSelection), PlayerOwner(DC.PlayerOwner), DebugTarget(DC.DebugTarget) {}

	uint32 bEnabledTargetSelection : 1;
	TWeakObjectPtr<APlayerController> PlayerOwner;
	TWeakObjectPtr<AActor>	DebugTarget;
};

FORCEINLINE bool operator==(const FDebugContext& Left, const FDebugContext& Right)
{
	return Left.PlayerOwner.Get() == Right.PlayerOwner.Get();
}


UCLASS(config=Engine, NotBlueprintable, Transient)
class GAMEPLAYDEBUGGER_API AGameplayDebuggingReplicator : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(config)
	FString DebugComponentClassName;

	UPROPERTY(config)
	FString DebugComponentHUDClassName;

	UPROPERTY(Replicated, Transient)
	UGameplayDebuggingComponent* DebugComponent;

	UFUNCTION(reliable, server, WithValidation)
	void ServerReplicateMessage(class AActor* Actor, uint32 InMessage, uint32 DataView = 0);

	UFUNCTION(reliable, client, WithValidation)
	void ClientReplicateMessage(class AActor* Actor, uint32 InMessage, uint32 DataView = 0);

	UFUNCTION(reliable, server, WithValidation)
	void ServerEnableTargetSelection(bool bEnable, APlayerController* Context);

	UFUNCTION(reliable, server, WithValidation)
	void ServerRegisterClient(APlayerController *PlayerController);

	UFUNCTION(reliable, server, WithValidation)
	void ServerUnregisterClient(APlayerController* PlayerController);

	virtual class UNetConnection* GetNetConnection() override;

	virtual bool IsNetRelevantFor(class APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation) override;

	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	FORCEINLINE UGameplayDebuggingComponent* GetDebugComponent()
	{
		return DebugComponent;
	}

	bool IsToolCreated();
	void CreateTool(APlayerController *PlayerController);
	void EnableTool();
	bool IsDrawEnabled();
	void EnableDraw(bool bEnable);
	TArray<FDebugContext>& GetClients() { return Clients;  }

protected:
	void OnDebugAIDelegate(class UCanvas* Canvas, class APlayerController* PC);
	void DrawDebugDataDelegate(class UCanvas* Canvas, class APlayerController* PC);
	void DrawDebugData(class UCanvas* Canvas, class APlayerController* PC);

private:
	uint32 bEnabledDraw : 1;
	uint32 LastDrawAtFrame;

	TWeakObjectPtr<UClass> DebugComponentClass;
	TWeakObjectPtr<UClass> DebugComponentHUDClass;

	TWeakObjectPtr<AGameplayDebuggingHUDComponent>	DebugRenderer;
	TArray<FDebugContext> Clients;
	TWeakObjectPtr<APlayerController> LocalPlayerOwner;
	TWeakObjectPtr<AActor>	LastSelectedActorInSimulation;
};
