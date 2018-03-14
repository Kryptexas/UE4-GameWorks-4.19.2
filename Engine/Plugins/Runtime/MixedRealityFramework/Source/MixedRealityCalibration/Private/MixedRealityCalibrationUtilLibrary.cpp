// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MixedRealityCalibrationUtilLibrary.h"
#include "Math/NumericLimits.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Slate/SceneViewport.h"
#include "InputCoreTypes.h"


/* MixedRealityUtilLibrary_Impl 
 *****************************************************************************/

namespace MRCalibrationUtilLibrary_Impl
{
	static FVector FindAvgVector(const TArray<FVector>& VectorSet);

	static void ComputeDivergenceField(const TArray<FVector>& VectorSet, TArray<float>& Out);

	template<typename T> 
	static void FindOutliers(const TArray<T> DataSet, TArray<int32>& OutlierIndices)
	{
		TArray<int32> IndexBuffer;
		IndexBuffer.Reserve(DataSet.Num());

		for (int32 SampleIndex = 0; SampleIndex < DataSet.Num(); ++SampleIndex)
		{
			IndexBuffer.Add(SampleIndex);
		}

		IndexBuffer.Sort([&DataSet](const int32& A, const int32& B) {
			return DataSet[A] < DataSet[B]; // sort by divergence (smallest to largest)
		});

		auto GetValue = [&DataSet, &IndexBuffer](const int32 IndiceIndex)->T
		{
			return IndiceIndex;
		};

		const int32 IsEven = 1 - (IndexBuffer.Num() % 2);
		const int32 FirstHalfEnd = (IndexBuffer.Num() / 2) - IsEven;
		const int32 SecondHalfStart = FirstHalfEnd + IsEven;
		const int32 SecondHalfEnd = IndexBuffer.Num() - 1;

		auto ComputeMedian = [&GetValue](const int32 StartIndx, const int32 LastIndx)->T
		{
			const int32 ValCount = (LastIndx - StartIndx) + 1;
			const int32 MedianIndex = StartIndx + (ValCount / 2);

			T MedianVal = GetValue(MedianIndex);
			if (ValCount % 2 == 0)
			{
				MedianVal = (MedianVal + GetValue(MedianIndex - 1)) / 2.f;
			}
			return MedianVal;
		};

		// compute the 1st and 3rd quartile
		const T Q1 = ComputeMedian(0, FirstHalfEnd);
		const T Q3 = ComputeMedian(SecondHalfStart, SecondHalfEnd);
		// compute the interquartile range
		const T IQR = Q3 - Q1;

		const T UpperLimit = Q3 + 1.5 * IQR;
		const T LowerLimit = Q1 - 1.5 * IQR;

		for (int32 IndiceIndex = 0; IndiceIndex < IndexBuffer.Num(); ++IndiceIndex)
		{
			if (GetValue(IndiceIndex) < LowerLimit)
			{
				OutlierIndices.Add(IndexBuffer[IndiceIndex]);
			}
			else
			{
				break;
			}
		}

		for (int32 IndiceIndex = IndexBuffer.Num() - 1; IndiceIndex >= 0; --IndiceIndex)
		{
			if (GetValue(IndiceIndex) > UpperLimit)
			{
				OutlierIndices.Add(IndexBuffer[IndiceIndex]);
			}
			else
			{
				break;
			}
		}
	}
}

static FVector MRCalibrationUtilLibrary_Impl::FindAvgVector(const TArray<FVector>& VectorSet)
{
	FVector AvgVec = FVector::ZeroVector;
	for (const FVector& Vec : VectorSet)
	{
		AvgVec += Vec;
	}
	if (VectorSet.Num() > 0)
	{
		AvgVec /= VectorSet.Num();
	}
	return AvgVec;
}

static void MRCalibrationUtilLibrary_Impl::ComputeDivergenceField(const TArray<FVector>& VectorSet, TArray<float>& Out)
{
	Out.Empty(VectorSet.Num());
	FVector AvgVec = FindAvgVector(VectorSet);

	for (const FVector& Vec : VectorSet)
	{
		Out.Add(FVector::Distance(Vec, AvgVec));
	}
}

/* FMRAlignmentSample 
 *****************************************************************************/

FMRAlignmentSample::FMRAlignmentSample() 
	: PlanarId(INDEX_NONE) 
{}

FVector FMRAlignmentSample::GetAdjustedSamplePoint() const
{
	return SampledWorldPosition + SampleAdjustmentOffset;
}

FVector FMRAlignmentSample::GetTargetPositionInWorldSpace(const FVector& ViewOrigin, const FRotator& ViewOrientation) const
{
	return ViewOrigin + ViewOrientation.RotateVector(RelativeTargetPosition);
}

/* UMRCalibrationUtilLibrary 
 *****************************************************************************/

UMRCalibrationUtilLibrary::UMRCalibrationUtilLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMRCalibrationUtilLibrary::FindOutliers(const TArray<float>& DataSet, TArray<int32>& OutlierIndices)
{
	if (DataSet.Num() > 1)
	{
		MRCalibrationUtilLibrary_Impl::FindOutliers(DataSet, OutlierIndices);
	}
}

int32 UMRCalibrationUtilLibrary::FindBestAnchorPoint(const TArray<FMRAlignmentSample>& AlignmentPoints, const FVector& ViewOrigin, const FRotator& ViewOrientation)
{
	int32 BestAlignmentPoint = INDEX_NONE;
	if (AlignmentPoints.Num() <= 2)
	{
		// if there's only two points, the divergence at both will be the same...
		// arbitrarily, go with the newest
		BestAlignmentPoint = AlignmentPoints.Num() - 1;
	}
	else
	{
		float BestAvgDivergence = MAX_flt;
		for (int32 OriginIndex = 0; OriginIndex < AlignmentPoints.Num(); ++OriginIndex)
		{
			const FMRAlignmentSample& PerspectiveAncor = AlignmentPoints[OriginIndex];
			const FVector AlignmentOffset = PerspectiveAncor.GetAdjustedSamplePoint() - PerspectiveAncor.GetTargetPositionInWorldSpace(ViewOrigin, ViewOrientation);
			const FVector OffsetOrigin = ViewOrigin + AlignmentOffset;

			float AvgDivergence = 0.f;
			for (int32 PointIndex = 0; PointIndex < AlignmentPoints.Num(); ++PointIndex)
			{
				if (PointIndex != OriginIndex)
				{
					const FMRAlignmentSample& OtherPoint = AlignmentPoints[PointIndex];

					const FVector ToTarget = OtherPoint.GetTargetPositionInWorldSpace(ViewOrigin, ViewOrientation) - OffsetOrigin;
					const FVector ToSample = OtherPoint.GetAdjustedSamplePoint() - OffsetOrigin;

					AvgDivergence += FVector::Distance(ToSample, ToTarget);
				}
			}
			AvgDivergence /= AlignmentPoints.Num() - 1;

			if (AvgDivergence < BestAvgDivergence)
			{
				BestAlignmentPoint = OriginIndex;
				BestAvgDivergence = AvgDivergence;
			}
		}
	}

	return BestAlignmentPoint;
}

void UMRCalibrationUtilLibrary::FindBalancingAlignmentOffset(const TArray<FMRAlignmentSample>& AlignmentPoints, const FVector& ViewOrigin, const FRotator& ViewOrientation, bool bOmitOutliers, FVector& BalancingOffset)
{
	BalancingOffset = FVector::ZeroVector;

	if (AlignmentPoints.Num() == 1)
	{
		const FVector SamplePt = AlignmentPoints[0].GetAdjustedSamplePoint();
		const FVector TargetPt = AlignmentPoints[0].GetTargetPositionInWorldSpace(ViewOrigin, ViewOrientation);

		// the offset it'll take to align the TargetPt with SamplePt
		BalancingOffset = SamplePt - TargetPt;
	}
	else if (AlignmentPoints.Num() > 0)
	{
		FVector MaxDivergences(-MAX_flt);
		FVector MinDivergences( MAX_flt);

		if (bOmitOutliers)
		{
			const int32 AnchorPtIndex = FindBestAnchorPoint(AlignmentPoints, ViewOrigin, ViewOrientation);
			const FMRAlignmentSample& AnchorPt = AlignmentPoints[AnchorPtIndex];
			const FVector AnchorOffset = AnchorPt.GetAdjustedSamplePoint() - AnchorPt.GetTargetPositionInWorldSpace(ViewOrigin, ViewOrientation);
			// want to align with an anchor first, to minimize divergences (else some points might get inadvertently rejected as outliers)
			BalancingOffset = AnchorOffset;

			const FVector AnchoredOrigin = ViewOrigin + AnchorOffset;

			TArray<float> XYZDivergences[3];
			for (const FMRAlignmentSample& AlignmentPt : AlignmentPoints)
			{
				const FVector DivergenceVec = AlignmentPt.GetAdjustedSamplePoint() - AlignmentPt.GetTargetPositionInWorldSpace(AnchoredOrigin, ViewOrientation);
				for (int32 CoordIndex = 0; CoordIndex < 3; ++CoordIndex)
				{
					XYZDivergences[CoordIndex].Add(DivergenceVec[CoordIndex]);
				}
			}

			for (int32 CoordIndex = 0; CoordIndex < 3; ++CoordIndex)
			{
				TArray<float>& CoordDivergences = XYZDivergences[CoordIndex];
				CoordDivergences.Sort();

				TArray<int32> OutlierIndices;
				MRCalibrationUtilLibrary_Impl::FindOutliers(CoordDivergences, OutlierIndices);

				for (int32 IndiceIndex = OutlierIndices.Num() - 1; IndiceIndex >= 0; --IndiceIndex)
				{
					CoordDivergences.RemoveAtSwap(OutlierIndices[IndiceIndex]);
				}
				if (ensure(CoordDivergences.Num() > 0))
				{
					if (CoordDivergences.Last() > MaxDivergences[CoordIndex])
					{
						MaxDivergences[CoordIndex] = CoordDivergences.Last();
					}
					if (CoordDivergences[0] < MinDivergences[CoordIndex])
					{
						MinDivergences[CoordIndex] = CoordDivergences[0];
					}
				}
			}
		}
		else // if !bOmitOutliers
		{
			for (const FMRAlignmentSample& AlignmentPt : AlignmentPoints)
			{
				const FVector DivergenceVec = AlignmentPt.GetAdjustedSamplePoint() - AlignmentPt.GetTargetPositionInWorldSpace(ViewOrigin, ViewOrientation);

				for (int32 CoordIndex = 0; CoordIndex < 3; ++CoordIndex)
				{
					if (DivergenceVec[CoordIndex] > MaxDivergences[CoordIndex])
					{
						MaxDivergences[CoordIndex] = DivergenceVec[CoordIndex];
					}
					if (DivergenceVec[CoordIndex] < MinDivergences[CoordIndex])
					{
						MinDivergences[CoordIndex] = DivergenceVec[CoordIndex];
					}
				}
			}
		}

		BalancingOffset += (MaxDivergences + MinDivergences) / 2.0f;
	}
}

void UMRCalibrationUtilLibrary::CalculateAlignmentNormals(const TArray<FMRAlignmentSample>& AlignmentPoints, const FVector& ViewOrigin, TArray<FVector>& PlanarNormals, const bool bOmitOutliers)
{
	PlanarNormals.Empty(AlignmentPoints.Num());

	for (int32 PointIndex = 0; PointIndex < AlignmentPoints.Num(); ++PointIndex)
	{
		const FMRAlignmentSample& RootPt = AlignmentPoints[PointIndex];
		const FVector PlanarOrigin = RootPt.GetAdjustedSamplePoint();

		// NOTE: this assumes the ViewOrigin is already in the correct half-space
		const FVector ToOrigin = ViewOrigin - PlanarOrigin;

		for (int32 PtIndexA = PointIndex + 1; PtIndexA < AlignmentPoints.Num(); ++PtIndexA)
		{
			const FMRAlignmentSample& PtA = AlignmentPoints[PtIndexA];
			if (PtA.PlanarId != RootPt.PlanarId)
			{
				continue;
			}
			const FVector ToPtA = PtA.GetAdjustedSamplePoint() - PlanarOrigin;

			for (int32 PtIndexB = PtIndexA + 1; PtIndexB < AlignmentPoints.Num(); ++PtIndexB)
			{
				const FMRAlignmentSample& PtB = AlignmentPoints[PtIndexB];
				if (PtB.PlanarId != RootPt.PlanarId)
				{
					continue;
				}
				const FVector ToPtB = PtB.GetAdjustedSamplePoint() - PlanarOrigin;

				FVector PlaneNormal = FVector::CrossProduct(ToPtA, ToPtB);
				PlaneNormal.Normalize();

				// half-space test => if the normal we computed is facing back to the origin...
				if (FVector::DotProduct(PlaneNormal, ToOrigin) >= 0)
				{
					// flip it (equivalent to crossing the other way)
					PlaneNormal = -PlaneNormal;
				}

				PlanarNormals.Add(PlaneNormal);
			}
		}
	}

	if (bOmitOutliers && PlanarNormals.Num() > 0)
	{
		TArray<float> Divergences;
		MRCalibrationUtilLibrary_Impl::ComputeDivergenceField(PlanarNormals, Divergences);

		TArray<int32> OutlierIndices;
		MRCalibrationUtilLibrary_Impl::FindOutliers(Divergences, OutlierIndices);

		OutlierIndices.Sort([](const int32& A, const int32& B) {
			return A > B; // reverse sort (largest index to smallest) so we can RemoveAtSwap()
		});

		for (int32& NormalIndex : OutlierIndices)
		{
			PlanarNormals.RemoveAtSwap(NormalIndex);
		}
	}
}

bool UMRCalibrationUtilLibrary::FindAverageLookAtDirection(const TArray<FMRAlignmentSample>& AlignmentPoints, const FVector& ViewOrigin, FVector& LookAtDir, const bool bOmitOutliers)
{
	TArray<FVector> ProspectiveNormals;
	CalculateAlignmentNormals(AlignmentPoints, ViewOrigin, ProspectiveNormals, bOmitOutliers);

	const bool bComputedNormals = ProspectiveNormals.Num() > 0;
	if (bComputedNormals)
	{
		LookAtDir = MRCalibrationUtilLibrary_Impl::FindAvgVector(ProspectiveNormals);
	}
	return bComputedNormals;
}


void UMRCalibrationUtilLibrary::GetCommandKeyStates(UObject* WorldContextObj, bool& bIsCtlDown, bool& bIsAltDown, bool& bIsShiftDown)
{
	bIsCtlDown = bIsAltDown = bIsShiftDown = false;

	UWorld* World = WorldContextObj->GetWorld();
	if (World && World->IsGameWorld())
	{
		if (UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			if (FSceneViewport* GameViewport = ViewportClient->GetGameViewport())
			{
				bIsCtlDown = GameViewport->KeyState(EKeys::LeftControl) || GameViewport->KeyState(EKeys::RightControl);
				bIsAltDown = GameViewport->KeyState(EKeys::LeftAlt) || GameViewport->KeyState(EKeys::RightAlt);
				bIsShiftDown = GameViewport->KeyState(EKeys::LeftShift) || GameViewport->KeyState(EKeys::RightShift);
			}
		}
	}
}

UActorComponent* UMRCalibrationUtilLibrary::AddComponentFromClass(AActor* Owner, TSubclassOf<UActorComponent> ComponentClass, FName ComponentName, bool bManualAttachment)
{
	UActorComponent* NewComponent = nullptr;
	if (Owner && ComponentClass)
	{
		const FName UniqueComponentName = MakeUniqueObjectName(Owner, ComponentClass, ComponentName);
		NewComponent = NewObject<UActorComponent>(Owner, ComponentClass, UniqueComponentName, RF_Transient | RF_TextExportTransient);
		Owner->AddOwnedComponent(NewComponent);

		NewComponent->OnComponentCreated();

		if (!bManualAttachment)
		{
			if (USceneComponent* AsSceneComponent = Cast<USceneComponent>(NewComponent))
			{
				if (USceneComponent* Root = Owner->GetRootComponent())
				{
					AsSceneComponent->SetupAttachment(Root);
				}
				else
				{
					Owner->SetRootComponent(AsSceneComponent);
				}
			}
		}

		if (NewComponent->bAutoRegister)
		{
			NewComponent->RegisterComponent();
		}
	}
	return NewComponent;
}

bool UMRCalibrationUtilLibrary::ClassImplementsInterface(UClass* ObjectClass, TSubclassOf<UInterface> InterfaceClass)
{
	return ObjectClass && InterfaceClass && ObjectClass->ImplementsInterface(InterfaceClass);
}
