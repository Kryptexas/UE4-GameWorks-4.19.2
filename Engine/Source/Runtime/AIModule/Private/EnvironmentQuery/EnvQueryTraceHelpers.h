// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace FEQSHelpers
{
	enum class ETraceMode : uint8
	{
		Keep,
		Discard,
	};

	struct FBatchTrace
	{
		UWorld* World;
		const FVector Extent;
		const FCollisionQueryParams Params;
		enum ECollisionChannel Channel;
		ETraceMode TraceMode;

		FBatchTrace(UWorld* InWorld, enum ECollisionChannel InChannel, const FCollisionQueryParams& InParams,
			const FVector& InExtent, ETraceMode InTraceMode)
			: World(InWorld), Extent(InExtent), Params(InParams), Channel(InChannel), TraceMode(InTraceMode)
		{

		}

		FORCEINLINE_DEBUGGABLE bool RunLineTrace(const FVector& StartPos, const FVector& EndPos, FVector& HitPos)
		{
			FHitResult OutHit;
			const bool bHit = World->LineTraceSingleByChannel(OutHit, StartPos, EndPos, Channel, Params);
			HitPos = OutHit.ImpactPoint;
			return bHit;
		}

		FORCEINLINE_DEBUGGABLE bool RunSphereTrace(const FVector& StartPos, const FVector& EndPos, FVector& HitPos)
		{
			FHitResult OutHit;
			const bool bHit = World->SweepSingleByChannel(OutHit, StartPos, EndPos, FQuat::Identity, Channel, FCollisionShape::MakeSphere(Extent.X), Params);
			HitPos = OutHit.ImpactPoint;
			return bHit;
		}

		FORCEINLINE_DEBUGGABLE bool RunCapsuleTrace(const FVector& StartPos, const FVector& EndPos, FVector& HitPos)
		{
			FHitResult OutHit;
			const bool bHit = World->SweepSingleByChannel(OutHit, StartPos, EndPos, FQuat::Identity, Channel, FCollisionShape::MakeCapsule(Extent.X, Extent.Z), Params);
			HitPos = OutHit.ImpactPoint;
			return bHit;
		}

		FORCEINLINE_DEBUGGABLE bool RunBoxTrace(const FVector& StartPos, const FVector& EndPos, FVector& HitPos)
		{
			FHitResult OutHit;
			const bool bHit = World->SweepSingleByChannel(OutHit, StartPos, EndPos, FQuat((EndPos - StartPos).Rotation()), Channel, FCollisionShape::MakeBox(Extent), Params);
			HitPos = OutHit.ImpactPoint;
			return bHit;
		}

		template<EEnvTraceShape::Type TraceType>
		void DoSingleSourceMultiDestinations(const FVector& Source, TArray<FNavLocation>& Points)
		{
			UE_LOG(LogEQS, Error, TEXT("FBatchTrace::DoSingleSourceMultiDestinations called with unhandled trace type: %d"), int32(TraceType));
		}

		template<EEnvTraceShape::Type TraceType>
		void DoProject(TArray<FNavLocation>& Points, float StartOffsetZ, float EndOffsetZ, float HitOffsetZ)
		{
			UE_LOG(LogEQS, Error, TEXT("FBatchTrace::DoSingleSourceMultiDestinations called with unhandled trace type: %d"), int32(TraceType));
		}
	};

	template<>
	void FBatchTrace::DoSingleSourceMultiDestinations<EEnvTraceShape::Line>(const FVector& Source, TArray<FNavLocation>& Points)
	{
		FVector HitPos(FVector::ZeroVector);
		for (int32 Idx = Points.Num() - 1; Idx >= 0; Idx--)
		{
			const bool bHit = RunLineTrace(Source, Points[Idx].Location, HitPos);
			if (bHit)
			{
				Points[Idx] = HitPos;
			}
			else if (TraceMode == ETraceMode::Discard)
			{
				Points.RemoveAt(Idx, 1, false);
			}
		}
	}

	template<>
	void FBatchTrace::DoSingleSourceMultiDestinations<EEnvTraceShape::Box>(const FVector& Source, TArray<FNavLocation>& Points)
	{
		FVector HitPos(FVector::ZeroVector);
		for (int32 Idx = Points.Num() - 1; Idx >= 0; Idx--)
		{
			const bool bHit = RunBoxTrace(Source, Points[Idx].Location, HitPos);
			if (bHit)
			{
				Points[Idx] = HitPos;
			}
			else if (TraceMode == ETraceMode::Discard)
			{
				Points.RemoveAt(Idx, 1, false);
			}
		}
	}

	template<>
	void FBatchTrace::DoSingleSourceMultiDestinations<EEnvTraceShape::Sphere>(const FVector& Source, TArray<FNavLocation>& Points)
	{
		FVector HitPos(FVector::ZeroVector);
		for (int32 Idx = Points.Num() - 1; Idx >= 0; Idx--)
		{
			const bool bHit = RunSphereTrace(Source, Points[Idx].Location, HitPos);
			if (bHit)
			{
				Points[Idx] = HitPos;
			}
			else if (TraceMode == ETraceMode::Discard)
			{
				Points.RemoveAt(Idx, 1, false);
			}
		}
	}

	template<>
	void FBatchTrace::DoSingleSourceMultiDestinations<EEnvTraceShape::Capsule>(const FVector& Source, TArray<FNavLocation>& Points)
	{
		FVector HitPos(FVector::ZeroVector);
		for (int32 Idx = Points.Num() - 1; Idx >= 0; Idx--)
		{
			const bool bHit = RunCapsuleTrace(Source, Points[Idx].Location, HitPos);
			if (bHit)
			{
				Points[Idx] = HitPos;
			}
			else if (TraceMode == ETraceMode::Discard)
			{
				Points.RemoveAt(Idx, 1, false);
			}
		}
	}

	template<>
	void FBatchTrace::DoProject<EEnvTraceShape::Line>(TArray<FNavLocation>& Points, float StartOffsetZ, float EndOffsetZ, float HitOffsetZ)
	{
		FVector HitPos(FVector::ZeroVector);
		for (int32 Idx = Points.Num() - 1; Idx >= 0; Idx--)
		{
			const bool bHit = RunLineTrace(Points[Idx].Location + FVector(0, 0, StartOffsetZ), Points[Idx].Location + FVector(0, 0, EndOffsetZ), HitPos);
			if (bHit)
			{
				Points[Idx] = HitPos + FVector(0, 0, HitOffsetZ);
			}
			else if (TraceMode == ETraceMode::Discard)
			{
				Points.RemoveAt(Idx, 1, false);
			}
		}
	}

	template<>
	void FBatchTrace::DoProject<EEnvTraceShape::Box>(TArray<FNavLocation>& Points, float StartOffsetZ, float EndOffsetZ, float HitOffsetZ)
	{
		FVector HitPos(FVector::ZeroVector);
		for (int32 Idx = Points.Num() - 1; Idx >= 0; Idx--)
		{
			const bool bHit = RunBoxTrace(Points[Idx].Location + FVector(0, 0, StartOffsetZ), Points[Idx].Location + FVector(0, 0, EndOffsetZ), HitPos);
			if (bHit)
			{
				Points[Idx] = HitPos + FVector(0, 0, HitOffsetZ);
			}
			else if (TraceMode == ETraceMode::Discard)
			{
				Points.RemoveAt(Idx, 1, false);
			}
		}
	}

	template<>
	void FBatchTrace::DoProject<EEnvTraceShape::Sphere>(TArray<FNavLocation>& Points, float StartOffsetZ, float EndOffsetZ, float HitOffsetZ)
	{
		FVector HitPos(FVector::ZeroVector);
		for (int32 Idx = Points.Num() - 1; Idx >= 0; Idx--)
		{
			const bool bHit = RunSphereTrace(Points[Idx].Location + FVector(0, 0, StartOffsetZ), Points[Idx].Location + FVector(0, 0, EndOffsetZ), HitPos);
			if (bHit)
			{
				Points[Idx] = HitPos + FVector(0, 0, HitOffsetZ);
			}
			else if (TraceMode == ETraceMode::Discard)
			{
				Points.RemoveAt(Idx, 1, false);
			}
		}
	}

	template<>
	void FBatchTrace::DoProject<EEnvTraceShape::Capsule>(TArray<FNavLocation>& Points, float StartOffsetZ, float EndOffsetZ, float HitOffsetZ)
	{
		FVector HitPos(FVector::ZeroVector);
		for (int32 Idx = Points.Num() - 1; Idx >= 0; Idx--)
		{
			const bool bHit = RunCapsuleTrace(Points[Idx].Location + FVector(0, 0, StartOffsetZ), Points[Idx].Location + FVector(0, 0, EndOffsetZ), HitPos);
			if (bHit)
			{
				Points[Idx] = HitPos + FVector(0, 0, HitOffsetZ);
			}
			else if (TraceMode == ETraceMode::Discard)
			{
				Points.RemoveAt(Idx, 1, false);
			}
		}
	}

	void RunNavRaycasts(ANavigationData* NavData, const FEnvTraceData& TraceData, const FVector& SourcePt, TArray<FNavLocation>& Points, ETraceMode TraceMode = ETraceMode::Keep);
	void RunNavProjection(ANavigationData* NavData, const FEnvTraceData& TraceData, TArray<FNavLocation>& Points, ETraceMode TraceMode = ETraceMode::Discard);
	void RunPhysRaycasts(UWorld* World, const FEnvTraceData& TraceData, const FVector& SourcePt, TArray<FNavLocation>& Points, ETraceMode TraceMode = ETraceMode::Keep);
	void RunPhysProjection(UWorld* World, const FEnvTraceData& TraceData, TArray<FNavLocation>& Points, ETraceMode TraceMode = ETraceMode::Discard);
}
