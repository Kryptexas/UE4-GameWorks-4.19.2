// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameplayDebuggingReplicator.generated.h"

/**
*	Transient actor used to communicate between server and client, mostly for RPC
*/

class UGameplayDebuggingComponent;
class AGameplayDebuggingHUDComponent;

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

	UPROPERTY(Replicated, Transient)
	APlayerController* LocalPlayerOwner;

	UFUNCTION(reliable, server, WithValidation)
	void ServerReplicateMessage(class AActor* Actor, uint32 InMessage, uint32 DataView = 0);

	UFUNCTION(reliable, client, WithValidation)
	void ClientReplicateMessage(class AActor* Actor, uint32 InMessage, uint32 DataView = 0);

	UFUNCTION(reliable, server, WithValidation)
	void ServerEnableTargetSelection(bool bEnable, APlayerController* Context);

	virtual class UNetConnection* GetNetConnection() override;

	virtual bool IsNetRelevantFor(class APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation) override;

	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	UGameplayDebuggingComponent* GetDebugComponent();

	bool IsToolCreated();
	void CreateTool();
	void EnableTool();
	bool IsDrawEnabled();
	void EnableDraw(bool bEnable);

	void SetLocalPlayerOwner(APlayerController* PC) { LocalPlayerOwner = PC; }
	APlayerController* GetLocalPlayerOwner() { return LocalPlayerOwner; }

	uint32 DebuggerShowFlags;

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
	TWeakObjectPtr<AActor>	LastSelectedActorInSimulation;
};
