// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CustomPhysXPayload.h"

struct FUpdateChunksInfo
{
	int32 ChunkIndex;
	FTransform WorldTM;

	FUpdateChunksInfo(int32 InChunkIndex, const FTransform& InWorldTM) : ChunkIndex(InChunkIndex), WorldTM(InWorldTM) {}
};

#if WITH_APEX

class UDestructibleComponent;

struct FApexDestructionSyncActors : public FCustomPhysXSyncActors
{
	virtual void BuildSyncData_AssumesLocked(int32 SceneType, const TArray<physx::PxRigidActor*>& RigidActors) override;

	virtual void FinalizeSync(int32 SceneType) override;

private:

	/** Sync data for updating physx sim result */
	TMap<TWeakObjectPtr<UDestructibleComponent>, TArray<FUpdateChunksInfo> > ComponentUpdateMapping;
};

struct FApexDestructionCustomPayload : public FCustomPhysXPayload
{
	FApexDestructionCustomPayload()
		: FCustomPhysXPayload(SingletonCustomSync)
	{
	}

	virtual TWeakObjectPtr<UPrimitiveComponent> GetOwningComponent() const override;

	virtual int32 GetItemIndex() const override;

	virtual FName GetBoneName() const override;

	virtual FBodyInstance* GetBodyInstance() const override;

	/** Index of the chunk this data belongs to*/
	int32 ChunkIndex;
	/** Component owning this chunk info*/
	TWeakObjectPtr<UDestructibleComponent> OwningComponent;

private:
	friend class FApexDestructionModule;
	static FApexDestructionSyncActors* SingletonCustomSync;
};

#endif // WITH_APEX