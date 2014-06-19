// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "Components/SplineComponent.h"

#include "PaperRenderSceneProxy.h"

#define PAPER_USE_MATERIAL_SLOPES 1

//////////////////////////////////////////////////////////////////////////

FBox2D GetSpriteRenderDataBounds2D(const TArray<FVector4>& Data)
{
	FBox2D Bounds(ForceInit);
	for (const FVector4& XYUV : Data)
	{
		Bounds += FVector2D(XYUV.X, XYUV.Y);
	}

	return Bounds;
}

//////////////////////////////////////////////////////////////////////////
// FPaperTerrainSceneProxy

class FPaperTerrainSceneProxy : public FPaperRenderSceneProxy
{
public:
	FPaperTerrainSceneProxy(const UPaperTerrainComponent* InComponent, const TArray<FPaperTerrainMaterialPair>& InDrawingData);

	// FPrimitiveSceneProxy interface.
	//virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) override;
	// End of FPrimitiveSceneProxy interface.

protected:
	TArray<FPaperTerrainMaterialPair> DrawingData;
protected:
	// FPaperRenderSceneProxy interface
	virtual void DrawDynamicElements_RichMesh(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor) override;
	// End of FPaperRenderSceneProxy interface
};

FPaperTerrainSceneProxy::FPaperTerrainSceneProxy(const UPaperTerrainComponent* InComponent, const TArray<FPaperTerrainMaterialPair>& InDrawingData)
	: FPaperRenderSceneProxy(InComponent)
{
	DrawingData = InDrawingData;

	// Combine the material relevance for all materials
	for (const FPaperTerrainMaterialPair& Batch : DrawingData)
	{
		const UMaterialInterface* MaterialInterface = (Batch.Material != nullptr) ? Batch.Material : UMaterial::GetDefaultMaterial(MD_Surface);
		MaterialRelevance |= MaterialInterface->GetRelevance_Concurrent();
	}
}

void FPaperTerrainSceneProxy::DrawDynamicElements_RichMesh(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor)
{
	for (const FPaperTerrainMaterialPair& Batch : DrawingData)
	{
		if (Batch.Material != nullptr)
		{
			DrawBatch(PDI, View, bUseOverrideColor, OverrideColor, Batch.Material, Batch.Records);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// UPaperTerrainComponent

UPaperTerrainComponent::UPaperTerrainComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, TerrainColor(FLinearColor::White)
	, ReparamStepsPerSegment(4)
{
	TestScaleFactor = 1.0f;
}

const UObject* UPaperTerrainComponent::AdditionalStatObject() const
{
	return TerrainMaterial;
}

#if WITH_EDITORONLY_DATA
void UPaperTerrainComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != nullptr)
	{
		const FName PropertyName(PropertyChangedEvent.Property->GetFName());
		if (PropertyName == GET_MEMBER_NAME_CHECKED(USplineComponent, ReparamStepsPerSegment))
		{
			if (AssociatedSpline != nullptr)
			{
				AssociatedSpline->ReparamStepsPerSegment = ReparamStepsPerSegment;
				AssociatedSpline->UpdateSplineReparamTable();
			}
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UPaperTerrainComponent::OnRegister()
{
	Super::OnRegister();

	if (AssociatedSpline != nullptr)
	{
		AssociatedSpline->OnSplineEdited = FSimpleDelegate::CreateUObject(this, &UPaperTerrainComponent::OnSplineEdited);
	}

	OnSplineEdited();
}

void UPaperTerrainComponent::OnUnregister()
{
	Super::OnUnregister();

	if (AssociatedSpline != nullptr)
	{
		AssociatedSpline->OnSplineEdited.Unbind();
	}
}

FPrimitiveSceneProxy* UPaperTerrainComponent::CreateSceneProxy()
{
	FPaperTerrainSceneProxy* NewProxy = new FPaperTerrainSceneProxy(this, GeneratedSpriteGeometry);
	return NewProxy;
}

FBoxSphereBounds UPaperTerrainComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	// Determine the rendering bounds
	FBoxSphereBounds LocalRenderBounds;
	{
		FBox BoundingBox(ForceInit);

		for (const FPaperTerrainMaterialPair& DrawCall : GeneratedSpriteGeometry)
		{
			for (const FSpriteDrawCallRecord& Record : DrawCall.Records)
			{
				for (const FVector4& VertXYUV : Record.RenderVerts)
				{
					const FVector Vert((PaperAxisX * VertXYUV.X) + (PaperAxisY * VertXYUV.Y));
					BoundingBox += Vert;
				}
			}
		}

		// Make the whole thing a single unit 'deep'
		const FVector HalfThicknessVector = 0.5f * PaperAxisZ;
		BoundingBox.Min -= HalfThicknessVector;
		BoundingBox.Max += HalfThicknessVector;

		LocalRenderBounds = FBoxSphereBounds(BoundingBox);
	}

	// Graphics bounds.
	FBoxSphereBounds NewBounds = LocalRenderBounds.TransformBy(LocalToWorld);

#if 0
	// Add bounds of collision geometry (if present).
	if (UBodySetup* BodySetup = GetBodySetup())
	{
		const FBox AggGeomBox = BodySetup->AggGeom.CalcAABB(LocalToWorld);
		if (AggGeomBox.IsValid)
		{
			NewBounds = Union(NewBounds, FBoxSphereBounds(AggGeomBox));
		}
	}
#endif

	// Apply bounds scale
	NewBounds.BoxExtent *= BoundsScale;
	NewBounds.SphereRadius *= BoundsScale;

	return NewBounds;
}

FTransform UPaperTerrainComponent::GetTransformAtDistance(float InDistance) const
{
	const float SplineLength = AssociatedSpline->GetSplineLength();
	InDistance = FMath::Clamp<float>(InDistance, 0.0f, SplineLength);

	const float Param = AssociatedSpline->SplineReparamTable.Eval(InDistance, 0.0f);
	const FVector Position3D = AssociatedSpline->SplineInfo.Eval(Param, FVector::ZeroVector);

	const FVector Tangent = AssociatedSpline->SplineInfo.EvalDerivative(Param, FVector(1.0f, 0.0f, 0.0f)).SafeNormal();
	const FVector NormalEst = AssociatedSpline->SplineInfo.EvalSecondDerivative(Param, FVector(0.0f, 1.0f, 0.0f)).SafeNormal();
	const FVector Bitangent = FVector::CrossProduct(Tangent, NormalEst);
	const FVector Normal = FVector::CrossProduct(Bitangent, Tangent);
	const FVector Floop = FVector::CrossProduct(PaperAxisZ, Tangent);

	FTransform LocalTransform(Tangent, PaperAxisZ, Floop, Position3D);

	LocalTransform = FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector::ZeroVector) * LocalTransform;

#if PAPER_TERRAIN_DRAW_DEBUG
	FTransform WorldTransform = LocalTransform * ComponentToWorld;

	const float Time = 2.5f;

	DrawDebugCoordinateSystem(GetWorld(), WorldTransform.GetLocation(), FRotator(WorldTransform.GetRotation()), 15.0f, true, Time, SDPG_Foreground);
// 	FVector WorldPos = ComponentToWorld.TransformPosition(Position3D);
// 	WorldPos.Y -= 0.01;
// 
// 	//DrawDebugLine(GetWorld(), WorldPos, WorldPos + ComponentToWorld.TransformVector(Tangent) * 10.0f, FColor::Red, true, Time);
// 	// 		DrawDebugLine(GetWorld(), WorldPos, WorldPos + ComponentToWorld.TransformVector(NormalEst) * 10.0f, FColor::Green, true, Time);
// 	// 		DrawDebugLine(GetWorld(), WorldPos, WorldPos + ComponentToWorld.TransformVector(Bitangent) * 10.0f, FColor::Blue, true, Time);
// 	//DrawDebugLine(GetWorld(), WorldPos, WorldPos + ComponentToWorld.TransformVector(Floop) * 10.0f, FColor::Yellow, true, Time);
// 	// 		DrawDebugPoint(GetWorld(), WorldPos, 4.0f, FColor::White, true, 1.0f);
#endif

	return LocalTransform;
}

void UPaperTerrainComponent::OnSplineEdited()
{
	GeneratedSpriteGeometry.Empty();

	if ((AssociatedSpline != nullptr) && (TerrainMaterial != nullptr))
	{
		if (AssociatedSpline->ReparamStepsPerSegment != ReparamStepsPerSegment)
		{
			AssociatedSpline->ReparamStepsPerSegment = ReparamStepsPerSegment;
			AssociatedSpline->UpdateSplineReparamTable();
		}

		FRandomStream RandomStream(RandomSeed);

		const FInterpCurveVector& SplineInfo = AssociatedSpline->SplineInfo;

		float SplineLength = AssociatedSpline->GetSplineLength();


		struct FSpriteStamp
		{
			const UPaperSprite* Sprite;
			float NominalWidth;
			float Time;
			float Scale;
			bool bCanStretch;

			FSpriteStamp(const UPaperSprite* InSprite, float InTime, bool bIsEndCap)
				: Sprite(InSprite)
				, Time(InTime)
				, Scale(1.0f)
				, bCanStretch(!bIsEndCap)
			{
				const FBox2D Bounds2D = GetSpriteRenderDataBounds2D(InSprite->BakedRenderData);
				NominalWidth = FMath::Max<float>(Bounds2D.GetSize().X, 1.0f);
			}
		};

		struct FTerrainSegment
		{
			float StartTime;
			float EndTime;
			const FPaperTerrainMaterialRule* Rule;
			TArray<FSpriteStamp> Stamps;

			FTerrainSegment()
				: Rule(nullptr)
			{
			}

			void RepositionStampsToFillSpace()
			{
			}

		};

		struct FTerrainRuleHelper
		{
		public:
			FTerrainRuleHelper(const FPaperTerrainMaterialRule* Rule)
				: StartWidth(0.0f)
				, EndWidth(0.0f)
			{
				for (const UPaperSprite* Sprite : Rule->Body)
				{
					if (Sprite != nullptr)
					{
						const float Width = GetSpriteRenderDataBounds2D(Sprite->BakedRenderData).GetSize().X;
						if (Width > 0.0f)
						{
							ValidBodies.Add(Sprite);
							ValidBodyWidths.Add(Width);
						}
					}
				}

				if (Rule->StartCap != nullptr)
				{
					const float Width = GetSpriteRenderDataBounds2D(Rule->StartCap->BakedRenderData).GetSize().X;
					if (Width > 0.0f)
					{
						StartWidth = Width;
					}
				}

				if (Rule->EndCap != nullptr)
				{
					const float Width = GetSpriteRenderDataBounds2D(Rule->EndCap->BakedRenderData).GetSize().X;
					if (Width > 0.0f)
					{
						EndWidth = Width;
					}
				}
			}

			float StartWidth;
			float EndWidth;

			TArray<const UPaperSprite*> ValidBodies;
			TArray<float> ValidBodyWidths;

			int32 GenerateBodyIndex(FRandomStream& RandomStream) const
			{
				check(ValidBodies.Num() > 0);
				return RandomStream.GetUnsignedInt() % ValidBodies.Num();
			}
		};

		// Split the spline into segments based on the slope rules in the material
		TArray<FTerrainSegment> Segments;

		FTerrainSegment* ActiveSegment = new (Segments) FTerrainSegment();
		ActiveSegment->StartTime = 0.0f;
		ActiveSegment->EndTime = SplineLength;

		float CurrentTime = 0.0f;
		while (CurrentTime < SplineLength)
		{
			const FTransform Frame(GetTransformAtDistance(CurrentTime));
			const FVector UnitTangent = Frame.GetUnitAxis(EAxis::X);
			const float RawSlopeAngleRadians = FMath::Atan2(FVector::DotProduct(UnitTangent, PaperAxisY), FVector::DotProduct(UnitTangent, PaperAxisX));
			const float RawSlopeAngle = FMath::RadiansToDegrees(RawSlopeAngleRadians);
			const float SlopeAngle = FMath::Fmod(FMath::UnwindDegrees(RawSlopeAngle) + 360.0f, 360.0f);

			const FPaperTerrainMaterialRule* DesiredRule = (TerrainMaterial->Rules.Num() > 0) ? &(TerrainMaterial->Rules[0]) : nullptr;
			for (const FPaperTerrainMaterialRule& TestRule : TerrainMaterial->Rules)
			{
				if ((SlopeAngle >= TestRule.MinimumAngle) && (SlopeAngle < TestRule.MaximumAngle))
				{
					DesiredRule = &TestRule;
				}
			}

			if (ActiveSegment->Rule != DesiredRule)
			{
				if (ActiveSegment->Rule == nullptr)
				{
					ActiveSegment->Rule = DesiredRule;
				}
				else
				{
					ActiveSegment->EndTime = CurrentTime;

					ActiveSegment = new (Segments) FTerrainSegment();
					ActiveSegment->StartTime = CurrentTime;
					ActiveSegment->EndTime = SplineLength;
					ActiveSegment->Rule = DesiredRule;
				}
			}

			CurrentTime += 10.0f;
		}

		// Convert those segments to actual geometry
		for (FTerrainSegment& Segment : Segments)
		{
			check(Segment.Rule);
			FTerrainRuleHelper RuleHelper(Segment.Rule);

			float RemainingSegStart = Segment.StartTime + RuleHelper.StartWidth;
			float RemainingSegEnd = Segment.EndTime - RuleHelper.EndWidth;
			const float BodyDistance = RemainingSegEnd - RemainingSegStart;
			float DistanceBudget = BodyDistance;

			bool bUseBodySegments = (DistanceBudget > 0.0f) && (RuleHelper.ValidBodies.Num() > 0);

			// Add the start cap
			if (RuleHelper.StartWidth > 0.0f)
			{
				new (Segment.Stamps) FSpriteStamp(Segment.Rule->StartCap, Segment.StartTime + RuleHelper.StartWidth * 0.5f, /*bIsEndCap=*/ bUseBodySegments);
			}

			// Add the end cap
			if (RuleHelper.EndWidth > 0.0f)
			{
				new (Segment.Stamps) FSpriteStamp(Segment.Rule->EndCap, Segment.EndTime - RuleHelper.EndWidth * 0.5f, /*bIsEndCap=*/ bUseBodySegments);
			}
			

			// Add body segments
			if (bUseBodySegments)
			{
				int32 NumSegments = 0;
				float Position = RemainingSegStart;
				
				while (DistanceBudget > 0.0f)
				{
					const int32 BodyIndex = RuleHelper.GenerateBodyIndex(RandomStream);
					const UPaperSprite* Sprite = RuleHelper.ValidBodies[BodyIndex];
					const float Width = RuleHelper.ValidBodyWidths[BodyIndex];

 					if ((NumSegments > 0) && ((Width * 0.5f) > DistanceBudget))
 					{
 						break;
 					}
					new (Segment.Stamps) FSpriteStamp(Sprite, Position + (Width * 0.5f), /*bIsEndCap=*/ false);

					DistanceBudget -= Width;
					Position += Width;
					++NumSegments;
				}

				const float UsedSpace = (BodyDistance - DistanceBudget);
				const float OverallScaleFactor = BodyDistance / UsedSpace;
				
 				// Stretch body segments
				float PositionCorrectionSum = 0.0f;
 				for (int32 Index = 0; Index < NumSegments; ++Index)
 				{
					FSpriteStamp& Stamp = Segment.Stamps[Index + (Segment.Stamps.Num() - NumSegments)];
					
					const float WidthChange = (OverallScaleFactor - 1.0f) * Stamp.NominalWidth;
					const float FirstGapIsSmallerFactor = (Index == 0) ? 0.5f : 1.0f;
					PositionCorrectionSum += WidthChange * FirstGapIsSmallerFactor;

					Stamp.Scale = OverallScaleFactor;
					Stamp.Time += PositionCorrectionSum;
 				}
			}
			else
			{
				// Stretch endcaps
			}

			// Convert stamps into geometry
			for (const FSpriteStamp& Stamp : Segment.Stamps)
			{
				SpawnSegment(Stamp.Sprite, Stamp.Time, Stamp.Scale);
			}
		}

		// Draw debug frames at the start and end of the spline
#if PAPER_TERRAIN_DRAW_DEBUG
		{
			const float Time = 5.0f;
			{
				FTransform WorldTransform = GetTransformAtDistance(0.0f) * ComponentToWorld;
				DrawDebugCoordinateSystem(GetWorld(), WorldTransform.GetLocation(), FRotator(WorldTransform.GetRotation()), 30.0f, true, Time, SDPG_Foreground);
			}
			{
				FTransform WorldTransform = GetTransformAtDistance(SplineLength) * ComponentToWorld;
				DrawDebugCoordinateSystem(GetWorld(), WorldTransform.GetLocation(), FRotator(WorldTransform.GetRotation()), 30.0f, true, Time, SDPG_Foreground);
			}
		}
#endif
	}

	RecreateRenderState_Concurrent();
}

void UPaperTerrainComponent::SpawnSegment(const UPaperSprite* NewSprite, float Position, float HorizontalScale)
{
	if (NewSprite)
	{
		FPaperTerrainMaterialPair& MaterialBatch = *new (GeneratedSpriteGeometry) FPaperTerrainMaterialPair(); //@TODO: Look up the existing one instead
		MaterialBatch.Material = NewSprite->GetDefaultMaterial();

		FSpriteDrawCallRecord& Record = *new (MaterialBatch.Records) FSpriteDrawCallRecord();
		Record.BuildFromSprite(NewSprite);
		Record.Color = TerrainColor;

		for (FVector4& XYUV : Record.RenderVerts)
		{
			const FTransform LocalTransformAtX(GetTransformAtDistance(Position + XYUV.X * HorizontalScale));
			const FVector SourceVector = (XYUV.Y * PaperAxisY);
			const FVector NewVector = LocalTransformAtX.TransformPosition(SourceVector);

			const float NewX = FVector::DotProduct(NewVector, PaperAxisX);
			const float NewY = FVector::DotProduct(NewVector, PaperAxisY);

			XYUV.X = NewX;
			XYUV.Y = NewY;
		}
	}
}

void UPaperTerrainComponent::SetTerrainColor(FLinearColor NewColor)
{
	// Can't set color on a static component
	if (!(IsRegistered() && (Mobility == EComponentMobility::Static)) && (TerrainColor != NewColor))
	{
		TerrainColor = NewColor;

		//@TODO: Should we send immediately?
		MarkRenderDynamicDataDirty();
	}
}
