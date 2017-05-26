// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NiagaraDataInterface.h"
#include "NiagaraCommon.h"
#include "VectorVM.h"
#include "Components/SplineComponent.h"
#include "NiagaraDataInterfaceSpline.generated.h"



/** Data Interface allowing sampling of static meshes. */
UCLASS(EditInlineNew, Category = "Splines", meta = (DisplayName = "Spline"))
class NIAGARA_API UNiagaraDataInterfaceSpline : public UNiagaraDataInterface
{
	GENERATED_UCLASS_BODY()

public:
	
	/** The source actor from which to sample.  */
	UPROPERTY(EditAnywhere, Category = "Spline")
	AActor* Source;
	
	//UObject Interface
	virtual void PostInitProperties()override;
	//UObject Interface End

	//UNiagaraDataInterface Interface
	virtual bool PreSimulationTick(FNiagaraEffectInstance* EffectInstance, float DeltaSeconds) override;
	virtual bool HasPreSimulationTick()const override;
	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)override;
	virtual FVMExternalFunction GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo)override;
	virtual bool CopyTo(UNiagaraDataInterface* Destination) override;
	virtual bool Equals(UNiagaraDataInterface* Other) override;
	virtual bool RequiresPerInstanceData() const override { return true; }
	//UNiagaraDataInterface Interface End

	template<typename TransformHandlerType, typename SplineSampleType>
	void SampleSplinePositionByUnitDistance(FVectorVMContext& Context);
	template<typename TransformHandlerType, typename SplineSampleType>
	void SampleSplineUpVectorByUnitDistance(FVectorVMContext& Context);
	template<typename TransformHandlerType, typename SplineSampleType>
	void SampleSplineRightVectorByUnitDistance(FVectorVMContext& Context);
	template<typename TransformHandlerType, typename SplineSampleType>
	void SampleSplineDirectionByUnitDistance(FVectorVMContext& Context);
	template<typename TransformHandlerType, typename SplineSampleType>
	void SampleSplineTangentByUnitDistance(FVectorVMContext& Context);
	template<typename PosXType, typename PosYType, typename PosZType>
	void FindClosestUnitDistanceFromPositionWS(FVectorVMContext& Context);
	
	void GetLocalToWorld(FVectorVMContext& Context);
	void GetLocalToWorldInverseTransposed(FVectorVMContext& Context);

private:
	
	FORCEINLINE bool IsValid()const { return Component != nullptr; }

	void WriteTransform(const FMatrix& ToWrite, FVectorVMContext& Context);

	//Cached ptr to component we sample from. 
	USplineComponent* Component;
		
	//Cached GetComponentTransform().
	FMatrix Transform;
	//InverseTranspose of above for transforming normals/tangents.
	FMatrix TransformInverseTransposed;
};
