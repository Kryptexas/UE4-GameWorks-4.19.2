// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "EnvironmentQuery/EQSQueryResultSourceInterface.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EQSTestingPawn.generated.h"

/** this class is abstract even though it's perfectly functional on its own.
 *	The reason is to stop it from showing as valid player pawn type when configuring 
 *	project's game mode. */
UCLASS(abstract, hidecategories=(Advanced, Attachment, Collision, Animation))
class AIMODULE_API AEQSTestingPawn : public ACharacter, public IEQSQueryResultSourceInterface
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(Category=EQS, EditAnywhere)
	class UEnvQuery* QueryTemplate;

	/** optional parameters for query */
	UPROPERTY(Category=EQS, EditAnywhere)
	TArray<struct FEnvNamedValue> QueryParams;

	UPROPERTY(Category=EQS, EditAnywhere)
	float TimeLimitPerStep;

	UPROPERTY(Category=EQS, EditAnywhere)
	int32 StepToDebugDraw;

	UPROPERTY(Category=EQS, EditAnywhere)
	uint32 bDrawLabels:1;

	UPROPERTY(Category=EQS, EditAnywhere)
	uint32 bDrawFailedItems:1;

	UPROPERTY(Category=EQS, EditAnywhere)
	uint32 bReRunQueryOnlyOnFinishedMove:1;

	UPROPERTY(Category=EQS, EditAnywhere)
	uint32 bShouldBeVisibleInGame:1;

#if WITH_EDITORONLY_DATA
	/** Editor Preview */
	UPROPERTY()
	TSubobjectPtr<class UEQSRenderingComponent> EdRenderComp;
#endif // WITH_EDITORONLY_DATA

protected:
	TSharedPtr<struct FEnvQueryInstance> QueryInstance;

	TArray<FEnvQueryInstance> StepResults;

public:
	/** This pawn class spawns its controller in PostInitProperties to have it available in editor mode*/
	virtual void TickActor( float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction ) override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif // WITH_EDITOR

	// IEQSQueryResultSourceInterface start
	virtual const struct FEnvQueryResult* GetQueryResult() const override;
	virtual const struct FEnvQueryInstance* GetQueryInstance() const  override;

	virtual bool GetShouldDebugDrawLabels() const override { return bDrawLabels; }
	virtual bool GetShouldDrawFailedItems() const override{ return bDrawFailedItems; }
	// IEQSQueryResultSourceInterface end

	void RunEQSQuery();

protected:	
	void Reset();
	void MakeOneStep();

	void UpdateDrawing();

	static void OnEditorSelectionChanged(UObject* NewSelection);
};
