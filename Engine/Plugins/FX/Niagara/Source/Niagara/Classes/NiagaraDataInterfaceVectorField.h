// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NiagaraDataInterface.h"
#include "VectorField/VectorField.h"
#include "NiagaraDataInterfaceVectorField.generated.h"

class FNiagaraSystemInstance;

/*struct FNDIVectorField_InstanceData
{
	UVectorField* Field;
	uint32 SizeX;
	uint32 SizeY;
	uint32 SizeZ;
	float DeltaSeconds;

	FBox LocalBounds;

	FVector TilingAxes;

	FORCEINLINE_DEBUGGABLE bool ResetRequired(class UNiagaraDataInterfaceVectorField* Interface)const;

	FORCEINLINE_DEBUGGABLE bool Init(class UNiagaraDataInterfaceVectorField* Interface, FNiagaraSystemInstance* SystemInstance);
	FORCEINLINE_DEBUGGABLE bool Cleanup(class UNiagaraDataInterfaceVectorField* Interface, FNiagaraSystemInstance* SystemInstance);
	FORCEINLINE_DEBUGGABLE bool Tick(class UNiagaraDataInterfaceVectorField* Interface, FNiagaraSystemInstance* SystemInstance, float InDeltaSeconds);
	void UpdateTransforms(const FMatrix& LocalToWorld, FMatrix& OutVolumeToWorld, FMatrix& OutWorldToVolume);
	const void* Lock();
	void Unlock();
};*/

UCLASS(EditInlineNew, Category = "Vector Field", meta = (DisplayName = "Vector Field"))
class NIAGARA_API UNiagaraDataInterfaceVectorField : public UNiagaraDataInterface
{
	GENERATED_UCLASS_BODY()

public:
	/** Vector field used to sample from. */
	UPROPERTY(EditAnywhere, Category = VectorField)
	UVectorField* Field;

	UPROPERTY(EditAnywhere, Category = VectorField)
	bool bTileX;
	UPROPERTY(EditAnywhere, Category = VectorField)
	bool bTileY;
	UPROPERTY(EditAnywhere, Category = VectorField)
	bool bTileZ;

public:
	//~ UObject interface
#if WITH_EDITOR
	virtual void PostInitProperties()override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
#endif
	//~ UObject interface END

	//~ UNiagaraDataInterface interface
	virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)override;
	virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)override;
	virtual bool PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;
	virtual int32 PerInstanceDataSize()const override;

	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)override;
	virtual FVMExternalFunction GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData)override;
	virtual bool Equals(const UNiagaraDataInterface* Other) const override;
	virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target)const override;

	// GPU sim functionality
	virtual bool GetFunctionHLSL(const FName&  DefinitionFunctionName, FString InstanceFunctionName, TArray<FDIGPUBufferParamDescriptor> &Descriptors, FString &HLSLInterfaceID, FString &OutHLSL) override;
	virtual void GetBufferDefinitionHLSL(FString DataInterfaceID, TArray<FDIGPUBufferParamDescriptor> &BufferDescriptors, FString &OutHLSL) override;
	virtual TArray<FNiagaraDataInterfaceBufferData> &GetBufferDataArray() override;
	virtual void SetupBuffers(FDIBufferDescriptorStore &BufferDescriptors) override;
	//~ UNiagaraDataInterface interface END

	template<typename XType, typename YType, typename ZType>
	void SampleVectorField(FVectorVMContext& Context);	

	void GetFieldDimensions(FVectorVMContext& Context);

	void GetFieldBounds(FVectorVMContext& Context);

	bool bGPUBufferDirty;

protected:
	const void* Lock();
	void Unlock();
	virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;
	void InitField();

	uint32 SizeX, SizeY, SizeZ;
	FVector TilingAxes;
	FBox LocalBounds;
};
