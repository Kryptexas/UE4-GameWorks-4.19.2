// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NiagaraCommon.h"
#include "NiagaraShared.h"
#include "VectorVM.h"
#include "StaticMeshResources.h"
#include "Curves/RichCurve.h"
#include "NiagaraMergeable.h"
#include "NiagaraDataInterface.generated.h"

class INiagaraCompiler;
class UCurveVector;
class UCurveLinearColor;
class UCurveFloat;
class FNiagaraSystemInstance;

USTRUCT()
struct FNiagaraDataInterfaceBufferData
{
	GENERATED_BODY()
public:
	FNiagaraDataInterfaceBufferData()
		:UniformName(TEXT("Undefined"))
	{}

	FNiagaraDataInterfaceBufferData(FName InName)
		:UniformName(InName)
	{
	}
	FRWBuffer Buffer;
	FName UniformName;
};

struct FNDITransformHandlerNoop
{
	FORCEINLINE void TransformPosition(FVector& V, FMatrix& M) {  }
	FORCEINLINE void TransformVector(FVector& V, FMatrix& M) { }
};

struct FNDITransformHandler
{
	FORCEINLINE void TransformPosition(FVector& P, FMatrix& M) { P = M.TransformPosition(P); }
	FORCEINLINE void TransformVector(FVector& V, FMatrix& M) { V = M.TransformVector(V).GetUnsafeNormal3(); }
};

//////////////////////////////////////////////////////////////////////////
// Some helper classes allowing neat, init time binding of templated vm external functions.

// Adds a known type to the parameters
template<typename DirectType, typename NextBinder>
struct TNDIExplicitBinder
{
	template<typename... ParamTypes>
	static FVMExternalFunction Bind(UNiagaraDataInterface* Interface, const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData)
	{
		return NextBinder::template Bind<ParamTypes..., DirectType>(Interface, BindingInfo, InstanceData);
	}
};

// Binder that tests the location of an operand and adds the correct handler type to the Binding parameters.
template<int32 ParamIdx, typename DataType, typename NextBinder>
struct TNDIParamBinder
{
	template<typename... ParamTypes>
	static FVMExternalFunction Bind(UNiagaraDataInterface* Interface, const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData)
	{
		if (BindingInfo.InputParamLocations[ParamIdx])
		{
			return NextBinder::template Bind<ParamTypes..., FConstantHandler<DataType>>(Interface, BindingInfo, InstanceData);
		}
		else
		{
			return NextBinder::template Bind<ParamTypes..., FRegisterHandler<DataType>>(Interface, BindingInfo, InstanceData);
		}
	}
};

//Helper macros allowing us to define the final binding structs for each vm external function function more concisely.
#define NDI_FUNC_BINDER(ClassName, FuncName) T##ClassName##_##FuncName##Binder

#define DEFINE_NDI_FUNC_BINDER(ClassName, FuncName)\
struct NDI_FUNC_BINDER(ClassName, FuncName)\
{\
	template<typename ... ParamTypes>\
	static FVMExternalFunction Bind(UNiagaraDataInterface* Interface, const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData)\
	{\
		return FVMExternalFunction::CreateUObject(CastChecked<ClassName>(Interface), &ClassName::FuncName<ParamTypes...>);\
	}\
};

//////////////////////////////////////////////////////////////////////////

/** Base class for all Niagara data interfaces. */
UCLASS(abstract, EditInlineNew)
class NIAGARA_API UNiagaraDataInterface : public UNiagaraMergeable
{
	GENERATED_UCLASS_BODY()
		 
public: 

	/** Initializes the per instance data for this interface. Returns false if there was some error and the simulation should be disabled. */
	virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) { return true; }

	/** Destroys the per instence data for this interface. */
	virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) {}

	/** Ticks the per instance data for this interface, if it has any. */
	virtual bool PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) { return false; }
	virtual bool PerInstanceTickPostSimulate(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) { return false; }

	/** 
	Returns the size of the per instance data for this interface. 0 if this interface has no per instance data. 
	Must depend solely on the class of the interface and not on any particular member data of a individual interface.
	*/
	virtual int32 PerInstanceDataSize()const { return 0; }

	/** Gets all the available functions for this data interface. */
	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) {}

	/** Returns the delegate for the passed function signature. */
	virtual FVMExternalFunction GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData) { return FVMExternalFunction(); };

	/** Copies the contents of this DataInterface to another.*/
	bool CopyTo(UNiagaraDataInterface* Destination) const;

	/** Determines if this DataInterface is the same as another.*/
	virtual bool Equals(const UNiagaraDataInterface* Other) const;

	virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target)const { return false; }

	/** Determines if this type definition matches to a known data interface type.*/
	static bool IsDataInterfaceType(const FNiagaraTypeDefinition& TypeDef);

	virtual bool GetFunctionHLSL(const FName& DefinitionFunctionName, FString InstanceFunctionName, TArray<FDIGPUBufferParamDescriptor> &Descriptors, FString &HLSLInterfaceID, FString &OutHLSL)
	{
		check("Unefined HLSL in data interface. Interfaces need to be able to return HLSL for each function they define in GetFunctions.");
		return false;
	}
	virtual void GetBufferDefinitionHLSL(FString DataInterfaceID, TArray<FDIGPUBufferParamDescriptor> &BufferDescriptors, FString &OutHLSL)
	{
		check("Unefined HLSL in data interface. Interfaces need to define HLSL for uniforms their functions access.");
	}
	virtual TArray<FNiagaraDataInterfaceBufferData> &GetBufferDataArray()
	{
		check("Undefined buffer array access.");
		return GPUBuffers;
	}

	virtual void SetupBuffers(FDIBufferDescriptorStore &BufferDescriptors)
	{
		check("Undefined buffer setup.");
	}

protected:
	virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const;

protected:
	UPROPERTY()
	TArray<FNiagaraDataInterfaceBufferData> GPUBuffers;  // not sure whether storing this on the base is the right idea
};

/** Base class for curve data interfaces which facilitates handling the curve data in a standardized way. */
UCLASS(EditInlineNew, Category = "Curves", meta = (DisplayName = "Float Curve"))
class NIAGARA_API UNiagaraDataInterfaceCurveBase : public UNiagaraDataInterface
{
protected:
	GENERATED_BODY()
	UPROPERTY()
	bool GPUBufferDirty;
	UPROPERTY()
	TArray<float> ShaderLUT;
	UPROPERTY()
	float LUTMinTime;
	UPROPERTY()
	float LUTMaxTime;
	UPROPERTY()
	float LUTInvTimeRange;

	/** Remap a sample time for this curve to 0 to 1 between first and last keys for LUT access.*/
	FORCEINLINE float NormalizeTime(float T)
	{
		return bAllowUnnormalizedLUT ? (T - LUTMinTime) * LUTInvTimeRange : T;
	}

	/** Remap a 0 to 1 value between the first and last keys to a real sample time for this curve. */
	FORCEINLINE float UnnormalizeTime(float T)
	{
		return bAllowUnnormalizedLUT ? (T / LUTInvTimeRange) + LUTMinTime : T;
	}

public:
	UNiagaraDataInterfaceCurveBase()
		: GPUBufferDirty(false)
		, LUTMinTime(0.0f)
		, LUTMaxTime(1.0f)
		, LUTInvTimeRange(1.0f)
		, bUseLUT(true)
		, bAllowUnnormalizedLUT(false)
	{
	}

	UNiagaraDataInterfaceCurveBase(FObjectInitializer const& ObjectInitializer)
		: GPUBufferDirty(false)
		, LUTMinTime(0.0f)
		, LUTMaxTime(1.0f)
		, LUTInvTimeRange(1.0f)
		, bUseLUT(true)
		, bAllowUnnormalizedLUT(false)
	{}

	UPROPERTY(EditAnywhere, Category = "Curve")
	uint32 bUseLUT : 1;
	
	/** Allow the LUT to capture a curve outside the 0 to 1 range. Currently does not work for GPU particles. */
	UPROPERTY(EditAnywhere, Category = "Curve")
	uint32 bAllowUnnormalizedLUT : 1;

	enum
	{
		CurveLUTWidth = 128,
		CurveLUTWidthMinusOne = 127,
	};
	/** Structure to facilitate getting standardized curve information from a curve data interface. */
	struct FCurveData
	{
		FCurveData(FRichCurve* InCurve, FName InName, FLinearColor InColor)
			: Curve(InCurve)
			, Name(InName)
			, Color(InColor)
		{
		}
		/** A pointer to the curve. */
		FRichCurve* Curve;
		/** The name of the curve, unique within the data interface, which identifies the curve in the UI. */
		FName Name;
		/** The color to use when displaying this curve in the UI. */
		FLinearColor Color;
	};

	/** Gets information for all of the curves owned by this curve data interface. */
	virtual void GetCurveData(TArray<FCurveData>& OutCurveData) { }

	virtual bool GetFunctionHLSL(const FName&  DefinitionFunctionName, FString InstanceFunctionName, TArray<FDIGPUBufferParamDescriptor> &Descriptors, FString &HLSLInterfaceID, FString &OutHLSL)
	{
		check("Undefined HLSL in data interface. All curve interfaces need to define this function and return SampleCurve code");
		return false;
	}
	virtual void GetBufferDefinitionHLSL(FString DataInterfaceID, TArray<FDIGPUBufferParamDescriptor> &BufferDescriptors, FString &OutHLSL)
	{
		check("Unefined HLSL in data interface. Interfaces need to define HLSL for uniforms their functions access.");
	}
	virtual TArray<FNiagaraDataInterfaceBufferData> &GetBufferDataArray()
	{
		check("Undefined buffer array access.");
		return GPUBuffers;
	}
	virtual void SetupBuffers(FDIBufferDescriptorStore &BufferDescriptors)
	{
		check("Undefined buffer setup.");
	}

	//UNiagaraDataInterface interface
	virtual bool Equals(const UNiagaraDataInterface* Other) const override;
	virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target)const override { return true; }

protected:
	virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;
	virtual bool CompareLUTS(const TArray<float>& OtherLUT) const;
	//UNiagaraDataInterface interface END
};

//External function binder choosing between template specializations based on if a curve should use the LUT over full evaluation.
template<typename NextBinder>
struct TCurveUseLUTBinder
{
	template<typename... ParamTypes>
	static FVMExternalFunction Bind(UNiagaraDataInterface* Interface, const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData)
	{
		UNiagaraDataInterfaceCurveBase* CurveInterface = CastChecked<UNiagaraDataInterfaceCurveBase>(Interface);
		if (CurveInterface->bUseLUT)
		{
			return NextBinder::template Bind<ParamTypes..., TIntegralConstant<bool, true>>(Interface, BindingInfo, InstanceData);
		}
		else
		{
			return NextBinder::template Bind<ParamTypes..., TIntegralConstant<bool, false>>(Interface, BindingInfo, InstanceData);
		}
	}
};
