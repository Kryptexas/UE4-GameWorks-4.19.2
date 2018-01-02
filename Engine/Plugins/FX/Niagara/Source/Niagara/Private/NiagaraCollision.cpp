// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraCollision.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraStats.h"
#include "NiagaraComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

DECLARE_CYCLE_STAT(TEXT("Collision"), STAT_NiagaraCollision, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Event Emission"), STAT_NiagaraEventWrite, STATGROUP_Niagara);

void FNiagaraCollisionBatch::KickoffNewBatch(FNiagaraEmitterInstance *Sim, float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraCollision);
	FNiagaraVariable PosVar(FNiagaraTypeDefinition::GetVec3Def(), "Position");
	FNiagaraVariable VelVar(FNiagaraTypeDefinition::GetVec3Def(), "Velocity");
	FNiagaraVariable SizeVar(FNiagaraTypeDefinition::GetVec2Def(), "SpriteSize");
	FNiagaraVariable TstVar(FNiagaraTypeDefinition::GetBoolDef(), "PerformCollision");
	FNiagaraDataSetIterator<FVector> PosIt(Sim->GetData(), PosVar, 0, false);
	FNiagaraDataSetIterator<FVector> VelIt(Sim->GetData(), VelVar, 0, false);
	FNiagaraDataSetIterator<FVector2D> SizeIt(Sim->GetData(), SizeVar, 0, false);
	//FNiagaraDataSetIterator<int32> TstIt(Sim->GetData(), TstVar, 0, false);

	bool bUseSize = SizeIt.IsValid();

	if (!PosIt.IsValid() || !VelIt.IsValid() /*|| !TstIt.IsValid()*/)
	{
		return;
	}

	UWorld *SystemWorld = Sim->GetParentSystemInstance()->GetComponent()->GetWorld();
	if (SystemWorld)
	{
		CollisionTraces.Empty();

		for (uint32 i = 0; i < Sim->GetData().GetPrevNumInstances(); i++)
		{
			//int32 TestCollision = *TstIt;
			//if (TestCollision)
			{
				check(PosIt.IsValid() && VelIt.IsValid() && (bUseSize == false || SizeIt.IsValid()));
				FVector Position;
				FVector EndPosition;
				FVector Velocity;

				PosIt.Get(Position);
				VelIt.Get(Velocity);
				EndPosition = Position + Velocity * DeltaSeconds;

				if (bUseSize)
				{
					//TODO:  Handle mesh particles too.  Also this can probably be better and or faster.
					FVector2D SpriteSize;
					SizeIt.Get(SpriteSize);
					float MaxSize = FMath::Max(SpriteSize.X, SpriteSize.Y);

					float Length;
					FVector Direction;
					Velocity.ToDirectionAndLength(Direction, Length);

					Position -= Direction * (MaxSize / 2);
					EndPosition += Direction * (MaxSize / 2);
				}

				FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NiagraAsync));
				QueryParams.OwnerTag = "Niagara";
				FTraceHandle Handle = SystemWorld->AsyncLineTraceByChannel(EAsyncTraceType::Single, Position, EndPosition, ECollisionChannel::ECC_WorldStatic, QueryParams, FCollisionResponseParams::DefaultResponseParam, nullptr, i);
				FNiagaraCollisionTrace Trace;
				Trace.CollisionTraceHandle = Handle;
				Trace.SourceParticleIndex = i;
				Trace.OriginalVelocity = Velocity;
				CollisionTraces.Add(Trace);
			}

			//TstIt.Advance();
			PosIt.Advance();
			VelIt.Advance();
		}
	}
}

void FNiagaraCollisionBatch::GenerateEventsFromResults(FNiagaraEmitterInstance *Sim)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraCollision);
	//CollisionEventDataSet.Allocate(CollisionTraceHandles.Num());

	UWorld *SystemWorld = Sim->GetParentSystemInstance()->GetComponent()->GetWorld();
	if (SystemWorld)
	{
		TArray<FNiagaraCollisionEventPayload> Payloads;

		// generate events for last frame's collisions
		//
		for (FNiagaraCollisionTrace CurCheck: CollisionTraces)
		{
			FTraceHandle Handle = CurCheck.CollisionTraceHandle;
			FTraceDatum CurTrace;
			// wait for trace handles; this should block rarely to never
			bool bReady = false;
			while (!bReady)
			{
				bReady = SystemWorld->QueryTraceData(Handle, CurTrace);
				if (!bReady)
				{
					// if the query came back false, it's possible that the hanle is invalid for some reason; skip in that case
					// TODO: handle this more gracefully
					if (!SystemWorld->IsTraceHandleValid(Handle, false))
					{
						break;
					}
					break;
				}
			}

			if (bReady && CurTrace.OutHits.Num())
			{
				// grab the first hit that blocks
				FHitResult *Hit = FHitResult::GetFirstBlockingHit(CurTrace.OutHits);
				if (Hit && Hit->IsValidBlockingHit())
				{
					FNiagaraCollisionEventPayload Event;
					Event.CollisionNormal = Hit->ImpactNormal;
					Event.CollisionPos = Hit->ImpactPoint;
					Event.CollisionVelocity = CurCheck.OriginalVelocity;
					Event.ParticleIndex = CurTrace.UserData;
					check(!Event.CollisionNormal.ContainsNaN());
					check(Event.CollisionNormal.IsNormalized());
					check(!Event.CollisionPos.ContainsNaN());
					check(!Event.CollisionVelocity.ContainsNaN());

					// TODO add to unique list of physical materials for Blueprint
					Event.PhysicalMaterialIndex = 0;// Hit->PhysMaterial->GetUniqueID();

					Payloads.Add(Event);
				}
			}
		}

		if (Payloads.Num())
		{
			// now allocate the data set and write all the event structs
			//
			CollisionEventDataSet->Allocate(Payloads.Num());
			CollisionEventDataSet->SetNumInstances(Payloads.Num());
			//FNiagaraVariable ValidVar(FNiagaraTypeDefinition::GetIntDef(), "Valid");
			FNiagaraVariable PosVar(FNiagaraTypeDefinition::GetVec3Def(), "CollisionLocation");
			FNiagaraVariable VelVar(FNiagaraTypeDefinition::GetVec3Def(), "CollisionVelocity");
			FNiagaraVariable NormVar(FNiagaraTypeDefinition::GetVec3Def(), "CollisionNormal");
			FNiagaraVariable PhysMatIdxVar(FNiagaraTypeDefinition::GetIntDef(), "PhysicalMaterialIndex");
			FNiagaraVariable ParticleIndexVar(FNiagaraTypeDefinition::GetIntDef(), "ParticleIndex");
			//FNiagaraDataSetIterator<int32> ValidItr(*CollisionEventDataSet, ValidVar, 0, true);
			FNiagaraDataSetIterator<FVector> PosItr(*CollisionEventDataSet, PosVar, 0, true);
			FNiagaraDataSetIterator<FVector> NormItr(*CollisionEventDataSet, NormVar, 0, true);
			FNiagaraDataSetIterator<FVector> VelItr(*CollisionEventDataSet, VelVar, 0, true);
			FNiagaraDataSetIterator<int32> PhysMatItr(*CollisionEventDataSet, PhysMatIdxVar, 0, true);
			FNiagaraDataSetIterator<int32> ParticleIndexItr(*CollisionEventDataSet, ParticleIndexVar, 0, true);

			for (FNiagaraCollisionEventPayload &Payload : Payloads)
			{
				SCOPE_CYCLE_COUNTER(STAT_NiagaraEventWrite);

				check(/*ValidItr.IsValid() && */PosItr.IsValid() && VelItr.IsValid() && NormItr.IsValid() && PhysMatItr.IsValid());
				//ValidItr.Set(1);
				PosItr.Set(Payload.CollisionPos);
				VelItr.Set(Payload.CollisionVelocity);
				NormItr.Set(Payload.CollisionNormal);
				ParticleIndexItr.Set(Payload.ParticleIndex);
				PhysMatItr.Set(0);
				//ValidItr.Advance();
				PosItr.Advance();
				VelItr.Advance();
				NormItr.Advance();
				PhysMatItr.Advance();
				ParticleIndexItr.Advance();
			}
		}
		else
		{
			CollisionEventDataSet->SetNumInstances(0);
		}

	}
}








int32 FNiagaraDICollisionQueryBatch::SubmitQuery(FVector Position, FVector Velocity, float CollisionSize, float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraCollision);
	int32 Ret = INDEX_NONE;
	if (CollisionWorld)
	{
		//int32 TestCollision = *TstIt;
		//if (TestCollision)
		{
			FVector EndPosition = Position + Velocity * DeltaSeconds;

			float Length;
			FVector Direction;
			Velocity.ToDirectionAndLength(Direction, Length);
			Position -= Direction * (CollisionSize / 2);
			EndPosition += Direction * (CollisionSize/ 2);

			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NiagraAsync));
			QueryParams.OwnerTag = "Niagara";
			QueryParams.bTraceAsyncScene = true;
			QueryParams.bFindInitialOverlaps = false;
			QueryParams.bReturnFaceIndex = false;
			QueryParams.bReturnPhysicalMaterial = true;
			QueryParams.bTraceComplex = false;
			QueryParams.bIgnoreTouches = true;
			FTraceHandle Handle = CollisionWorld->AsyncLineTraceByChannel(EAsyncTraceType::Single, Position, EndPosition, ECollisionChannel::ECC_WorldStatic, QueryParams, FCollisionResponseParams::DefaultResponseParam, nullptr, TraceID);
			FNiagaraCollisionTrace Trace;
			Trace.CollisionTraceHandle = Handle;
			Trace.SourceParticleIndex = TraceID;
			Trace.OriginalVelocity = Velocity;

			int32 TraceIdx = CollisionTraces[GetWriteBufferIdx()].Add(Trace);
			IdToTraceIdx[GetWriteBufferIdx()].Add(TraceID) = TraceIdx;
			Ret = TraceID;
			TraceID++;
		}
	}

	return Ret;
}


bool FNiagaraDICollisionQueryBatch::GetQueryResult(uint32 InTraceID, FNiagaraDICollsionQueryResult &Result)
{
	int32 *TraceIdxPtr = IdToTraceIdx[GetReadBufferIdx()].Find(InTraceID);
	int32 TraceIdx = INDEX_NONE;

	if (TraceIdxPtr)
	{
		TraceIdx = *TraceIdxPtr;
		check(CollisionTraces[GetReadBufferIdx()].IsValidIndex(TraceIdx));
		FNiagaraCollisionTrace &CurTrace = CollisionTraces[GetReadBufferIdx()][TraceIdx];
		FTraceHandle Handle = CurTrace.CollisionTraceHandle;
		FTraceDatum CurData;
		// wait for trace handles; this should block rarely to never
		bool bReady = CollisionWorld->QueryTraceData(Handle, CurData);
		/*
		if (!bReady)
		{
			// if the query came back false, it's possible that the hanle is invalid for some reason; skip in that case
			// TODO: handle this more gracefully
			if (!CollisionWorld->IsTraceHandleValid(Handle, false))
			{
				break;
			}
			break;
			}
		}
		*/

		if (bReady && CurData.OutHits.Num())
		{
			// grab the first hit that blocks
			FHitResult *Hit = FHitResult::GetFirstBlockingHit(CurData.OutHits);
			if (Hit && Hit->IsValidBlockingHit())
			{
				/*
				FNiagaraCollisionEventPayload Event;
				Event.CollisionNormal = Hit->ImpactNormal;
				Event.CollisionPos = Hit->ImpactPoint;
				Event.CollisionVelocity = CurCheck.OriginalVelocity;
				Event.ParticleIndex = CurData.UserData;
				check(!Event.CollisionNormal.ContainsNaN());
				check(Event.CollisionNormal.IsNormalized());
				check(!Event.CollisionPos.ContainsNaN());
				check(!Event.CollisionVelocity.ContainsNaN());

				// TODO add to unique list of physical materials for Blueprint
				Event.PhysicalMaterialIndex = 0;// Hit->PhysMaterial->GetUniqueID();

				Payloads.Add(Event);
				*/
				Result.CollisionPos = Hit->ImpactPoint;
				Result.CollisionNormal = Hit->ImpactNormal;
				Result.CollisionVelocity = CurTrace.OriginalVelocity;
				Result.TraceID = InTraceID;
				Result.PhysicalMaterialIdx = Hit->PhysMaterial->GetUniqueID();
				Result.Friction = Hit->PhysMaterial->Friction;
				Result.Restitution = Hit->PhysMaterial->Restitution;
				return true;
			}
		}
	}

	return false;
}


/*
int32 FNiagaraDICollisionQueryBatch::Tick()
{

}
/*/