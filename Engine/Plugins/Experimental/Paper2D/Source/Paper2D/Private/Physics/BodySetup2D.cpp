// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// UBodySetup2D

UBodySetup2D::UBodySetup2D(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}


#if WITH_EDITOR

void UBodySetup2D::InvalidatePhysicsData()
{
	//ClearPhysicsMeshes();
	//BodySetupGuid = FGuid::NewGuid(); // change the guid
	//CookedFormatData.FlushData();
}

#endif


void UBodySetup2D::CreatePhysicsMeshes()
{
}

class UPhysicalMaterial* UBodySetup2D::GetPhysMaterial() const
{
	UPhysicalMaterial* PhysMat = PhysMaterial;

	if (PhysMat == NULL && GEngine != NULL)
	{
		PhysMat = GEngine->DefaultPhysMaterial;
	}
	return PhysMat;
}

float UBodySetup2D::CalculateMass(const UPrimitiveComponent2D* Component) const
{
	FVector ComponentScale(1.0f, 1.0f, 1.0f);
	UPhysicalMaterial* PhysMat = GetPhysMaterial();
	const FBodyInstance2D* BodyInstance = NULL;

	const UPrimitiveComponent2D* OuterComp = Component != NULL ? Component : Cast<UPrimitiveComponent2D>(GetOuter());
	if (OuterComp)
	{
		ComponentScale = OuterComp->GetComponentScale();

		BodyInstance = OuterComp->GetBodyInstance2D();

		//@TODO:
// 		const USkinnedMeshComponent* SkinnedMeshComp = Cast<const USkinnedMeshComponent>(OuterComp);
// 		if (SkinnedMeshComp != NULL)
// 		{
// 			const FBodyInstance* Body = SkinnedMeshComp->GetBodyInstance2D(BoneName);
// 
// 			if (Body != NULL)
// 			{
// 				BodyInstance = Body;
// 			}
// 		}
	}
	else
	{
		BodyInstance = &DefaultInstance;
	}

	PhysMat = BodyInstance->PhysMaterialOverride != NULL ? BodyInstance->PhysMaterialOverride : PhysMat;
	const float MassScale = 1.0f;//@TODO: BodyInstance->MassScale;

	// physical material - nothing can weigh less than hydrogen (0.09 kg/m^3)
	float DensityKGPerCubicUU = 1.0f;
	float RaiseMassToPower = 0.75f;
	float FictionalThickness = 1.0f;
	if (PhysMat)
	{
		DensityKGPerCubicUU = FMath::Max(0.00009f, PhysMat->Density * 0.001f);
		RaiseMassToPower = PhysMat->RaiseMassToPower;
	}

	// Then scale mass to avoid big differences between big and small objects.
	const float BasicMass = AggGeom2D.GetArea(ComponentScale) * FictionalThickness * DensityKGPerCubicUU;
	ensureMsgf(BasicMass >= 0.0f, TEXT("UBodySetup2D::CalculateMass(%s) - The volume of the aggregate geometry is negative"), *Component->GetReadableName());

	const float UsePow = FMath::Clamp<float>(RaiseMassToPower, KINDA_SMALL_NUMBER, 1.f);
	const float RealMass = FMath::Pow(FMath::Max<float>(BasicMass, 0.0f), UsePow);

	return RealMass * MassScale;
}
