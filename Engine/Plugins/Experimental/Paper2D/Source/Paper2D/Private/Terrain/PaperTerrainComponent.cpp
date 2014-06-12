// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "Components/SplineComponent.h"

//////////////////////////////////////////////////////////////////////////
// UPaperTerrainComponent

UPaperTerrainComponent::UPaperTerrainComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	TestScaleFactor = 1.0f;
}

#if WITH_EDITORONLY_DATA
void UPaperTerrainComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UPaperTerrainComponent::OnSplineEdited()
{
	DestroyExistingStuff();

	if ((AssociatedSpline != nullptr) && (TerrainMaterial != nullptr))
	{
		FRandomStream RandomStream(RandomSeed);

		const FInterpCurveVector& SplineInfo = AssociatedSpline->SplineInfo;

		float SplineLength = AssociatedSpline->GetSplineLength();

		float RemainingSegStart = 0.0f;
		float RemainingSegEnd = SplineLength;

		SpawnSegment(TerrainMaterial->LeftCap, /*dir=*/ 1.0f, /*inout*/ RemainingSegStart, /*inout*/ RemainingSegEnd);
		SpawnSegment(TerrainMaterial->RightCap, /*dir=*/ -1.0f, /*inout*/ RemainingSegStart, /*inout*/ RemainingSegEnd);

		if (TerrainMaterial->BodySegments.Num() > 0)
		{
			while (RemainingSegEnd - RemainingSegStart > 0.0f)
			{
				const int32 BodySegmentIndex = RandomStream.GetUnsignedInt() % TerrainMaterial->BodySegments.Num();
				UPaperSprite* BodySegment = TerrainMaterial->BodySegments[BodySegmentIndex];
				SpawnSegment(BodySegment, /*dir=*/ 1.0f, /*inout*/ RemainingSegStart, /*inout*/ RemainingSegEnd);
			}
		}
	}
}

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
	DestroyExistingStuff();
}

void UPaperTerrainComponent::DestroyExistingStuff()
{
	for (UPaperRenderComponent* Component : SpawnedComponents)
	{
		Component->DetachFromParent();
		Component->DestroyComponent();
	}
	SpawnedComponents.Empty();
}

void UPaperTerrainComponent::SpawnSegment(class UPaperSprite* NewSprite, float Direction, float& RemainingSegStart, float& RemainingSegEnd)
{
	if (NewSprite)
	{
		UPaperRenderComponent* NewRenderComp = ConstructObject<UPaperRenderComponent>(UPaperRenderComponent::StaticClass(), GetOuter(), NAME_None, RF_Transactional);
		NewRenderComp->AttachTo(this);
		NewRenderComp->bCreatedByConstructionScript = bCreatedByConstructionScript;

		NewRenderComp->SetSprite(NewSprite);


		//@TODO: Hacky!
		const float SpriteWidthUnsafe = NewSprite->GetRenderBounds().BoxExtent.ProjectOnTo(PaperAxisX).Size() * 2.0f;
		const float SpriteWidth = (SpriteWidthUnsafe < KINDA_SMALL_NUMBER) ? 1.0f : SpriteWidthUnsafe; // ensure we terminate in the outer loop even with malformed render data

		float Position = 0.0f;
		if (Direction < 0.0f)
		{
			Position = RemainingSegEnd - SpriteWidth * 0.5f;
			RemainingSegEnd -= SpriteWidth;
		}
		else
		{
			Position = RemainingSegStart + SpriteWidth * 0.5f;
			RemainingSegStart += SpriteWidth;
		}

		// Position and orient the sprite
		const float Param = AssociatedSpline->SplineReparamTable.Eval(Position, 0.f);
		const FVector Position3D = AssociatedSpline->SplineInfo.Eval(Param, FVector::ZeroVector);
		
		const FVector Tangent = AssociatedSpline->SplineInfo.EvalDerivative(Param, FVector(1.0f, 0.0f, 0.0f)).SafeNormal();
		const FVector NormalEst = AssociatedSpline->SplineInfo.EvalSecondDerivative(Param, FVector(0.0f, 1.0f, 0.0f)).SafeNormal();
		const FVector Bitangent = FVector::CrossProduct(Tangent, NormalEst);
		const FVector Normal = FVector::CrossProduct(Bitangent, Tangent);
		const FVector Floop = FVector::CrossProduct(PaperAxisZ, Tangent);
		//const FRotator Rotation3D(Tangent.Rotation());
		
		FTransform LocalTransform(Tangent, PaperAxisZ, Floop, Position3D);

		const FRotator Rotation3D(LocalTransform.GetRotation());
		const FVector WorldPos = ComponentToWorld.TransformPosition(Position3D);
		
		DrawDebugLine(GetWorld(), WorldPos, WorldPos + ComponentToWorld.TransformVector(Tangent) * 10.0f, FColor::Red, true, 1.0f);
// 		DrawDebugLine(GetWorld(), WorldPos, WorldPos + ComponentToWorld.TransformVector(NormalEst) * 10.0f, FColor::Green, true, 1.0f);
// 		DrawDebugLine(GetWorld(), WorldPos, WorldPos + ComponentToWorld.TransformVector(Bitangent) * 10.0f, FColor::Blue, true, 1.0f);
		DrawDebugLine(GetWorld(), WorldPos, WorldPos + ComponentToWorld.TransformVector(Floop) * 10.0f, FColor::Yellow, true, 1.0f);
// 		DrawDebugPoint(GetWorld(), WorldPos, 4.0f, FColor::White, true, 1.0f);

		NewRenderComp->RelativeLocation = Position3D;
		NewRenderComp->RelativeRotation = Rotation3D;
		NewRenderComp->RelativeScale3D = FVector(TestScaleFactor, TestScaleFactor, TestScaleFactor);

		NewRenderComp->RegisterComponentWithWorld(GetWorld());
		SpawnedComponents.Add(NewRenderComp);

		
	}
}
