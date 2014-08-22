// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameplayDebuggingTypes.h"
#include "GameplayDebuggingReplicator.generated.h"
/**
*	Transient actor used to communicate between server and client, mostly for RPC
*/

class UGameplayDebuggingComponent;
class AGameplayDebuggingHUDComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSelectionChanged, class AActor*);

UCLASS(config = Engine, NotBlueprintable, Transient, hidecategories = Actor)
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

	UPROPERTY(Replicated, Transient)
	bool bIsGlobalInWorld;

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

	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

	UGameplayDebuggingComponent* GetDebugComponent();

	bool IsToolCreated();
	void CreateTool();
	void EnableTool();
	bool IsDrawEnabled();
	void EnableDraw(bool bEnable);
	void SetAsGlobalInWorld(bool IsGlobal) { bIsGlobalInWorld = IsGlobal;  }
	bool IsGlobalInWorld() { return bIsGlobalInWorld;  }

	void SetLocalPlayerOwner(APlayerController* PC) { LocalPlayerOwner = PC; }
	APlayerController* GetLocalPlayerOwner() { return LocalPlayerOwner; }

	FORCEINLINE AActor* GetSelectedActorToDebug() { return LastSelectedActorInSimulation.Get(); }
	void SetActorToDebug(AActor* InActor);

	uint32 DebuggerShowFlags;

	static FOnSelectionChanged OnSelectionChangedDelegate;
	FOnChangeEQSQuery OnChangeEQSQuery;
protected:
	void OnDebugAIDelegate(class UCanvas* Canvas, class APlayerController* PC);
	void DrawDebugDataDelegate(class UCanvas* Canvas, class APlayerController* PC);
	void DrawDebugData(class UCanvas* Canvas, class APlayerController* PC);

private:
	uint32 bEnabledDraw : 1;
	uint32 LastDrawAtFrame;
	float PlayerControllersUpdateDelay;

	TWeakObjectPtr<UClass> DebugComponentClass;
	TWeakObjectPtr<UClass> DebugComponentHUDClass;

	TWeakObjectPtr<AGameplayDebuggingHUDComponent>	DebugRenderer;
	TWeakObjectPtr<AActor>	LastSelectedActorInSimulation;
};
