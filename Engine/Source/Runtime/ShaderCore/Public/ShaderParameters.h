// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ShaderParameters.h: Shader parameter definitions.
=============================================================================*/

#pragma once

#include "ShaderCore.h"
#include "RenderResource.h"
#include "UniformBuffer.h"
#include "RHIStaticStates.h"

enum EShaderParameterFlags
{
	// no shader error if the parameter is not used
	SPF_Optional,
	// shader error if the parameter is not used
	SPF_Mandatory
};

/** A shader parameter's register binding. */
class FShaderParameter
{
public:
	FShaderParameter()
	:	BufferIndex(0)
	,	BaseIndex(0)
	,	NumBytes(0)
#if UE_BUILD_DEBUG
	,	bInitialized(false)
#endif
	{}

	SHADERCORE_API void Bind(const FShaderParameterMap& ParameterMap,const TCHAR* ParameterName, EShaderParameterFlags Flags = SPF_Optional);
	friend SHADERCORE_API FArchive& operator<<(FArchive& Ar,FShaderParameter& P);
	bool IsBound() const { return NumBytes > 0; }

	inline bool IsInitialized() const 
	{ 
#if UE_BUILD_DEBUG
		return bInitialized; 
#else 
		return true;
#endif
	}

	uint32 GetBufferIndex() const { return BufferIndex; }
	uint32 GetBaseIndex() const { return BaseIndex; }
	uint32 GetNumBytes() const { return NumBytes; }

private:
	uint16 BufferIndex;
	uint16 BaseIndex;
	// 0 if the parameter wasn't bound
	uint16 NumBytes;

#if UE_BUILD_DEBUG
	bool bInitialized;
#endif
};

/** A shader resource binding (textures or samplerstates). */
class FShaderResourceParameter
{
public:
	FShaderResourceParameter()
	:	BaseIndex(0)
	,	NumResources(0) 
#if UE_BUILD_DEBUG
	,	bInitialized(false)
#endif
	{}

	SHADERCORE_API void Bind(const FShaderParameterMap& ParameterMap,const TCHAR* ParameterName,EShaderParameterFlags Flags = SPF_Optional);
	friend SHADERCORE_API FArchive& operator<<(FArchive& Ar,FShaderResourceParameter& P);
	bool IsBound() const { return NumResources > 0; }

	inline bool IsInitialized() const 
	{ 
#if UE_BUILD_DEBUG
		return bInitialized; 
#else 
		return true;
#endif
	}

	uint32 GetBaseIndex() const { return BaseIndex; }
	uint32 GetNumResources() const { return NumResources; }
private:
	uint16 BaseIndex;
	uint16 NumResources;

#if UE_BUILD_DEBUG
	bool bInitialized;
#endif
};

/**
 * Sets the value of a  shader parameter.  Template'd on shader type
 * A template parameter specified the type of the parameter value.
 * NOTE: Shader should be the param ref type, NOT the param type, since Shader is passed by value. 
 * Otherwise AddRef/ReleaseRef will be called many times.
 */
template<typename ShaderRHIParamRef, class ParameterType>
void SetShaderValue(
	ShaderRHIParamRef Shader,
	const FShaderParameter& Parameter,
	const ParameterType& Value,
	uint32 ElementIndex = 0
	)
{
	checkAtCompileTime(!TIsPointerType<ParameterType>::Value, ErrorPassByValue);

	const uint32 AlignedTypeSize = Align(sizeof(ParameterType),ShaderArrayElementAlignBytes);
	const int32 NumBytesToSet = FMath::Min<int32>(sizeof(ParameterType),Parameter.GetNumBytes() - ElementIndex * AlignedTypeSize);

	// This will trigger if the parameter was not serialized
	checkSlow(Parameter.IsInitialized());

	if(NumBytesToSet > 0)
	{
		RHISetShaderParameter(
			Shader,
			Parameter.GetBufferIndex(),
			Parameter.GetBaseIndex() + ElementIndex * AlignedTypeSize,
			(uint32)NumBytesToSet,
			&Value
			);
	}
}

/** Specialization of the above for C++ bool type. */
template<typename ShaderRHIParamRef>
void SetShaderValue(
	ShaderRHIParamRef Shader,
	const FShaderParameter& Parameter,
	const bool& Value,
	uint32 ElementIndex = 0
	)
{
	const uint32 BoolValue = Value;
	SetShaderValue(Shader, Parameter, BoolValue, ElementIndex);
}

/**
 * Sets the value of a shader parameter array.  Template'd on shader type
 * A template parameter specified the type of the parameter value.
 * NOTE: Shader should be the param ref type, NOT the param type, since Shader is passed by value. 
 * Otherwise AddRef/ReleaseRef will be called many times.
 */
template<typename ShaderRHIParamRef,class ParameterType>
void SetShaderValueArray(
	ShaderRHIParamRef Shader,
	const FShaderParameter& Parameter,
	const ParameterType* Values,
	uint32 NumElements,
	uint32 BaseElementIndex = 0
	)
{
	const uint32 AlignedTypeSize = Align(sizeof(ParameterType),ShaderArrayElementAlignBytes);
	const int32 NumBytesToSet = FMath::Min<int32>(NumElements * AlignedTypeSize,Parameter.GetNumBytes() - BaseElementIndex * AlignedTypeSize);

	// This will trigger if the parameter was not serialized
	checkSlow(Parameter.IsInitialized());

	if(NumBytesToSet > 0)
	{
		RHISetShaderParameter(
			Shader,
			Parameter.GetBufferIndex(),
			Parameter.GetBaseIndex() + BaseElementIndex * AlignedTypeSize,
			(uint32)NumBytesToSet,
			Values
			);
	}
}

/** Specialization of the above for C++ bool type. */
template<typename ShaderRHIParamRef>
void SetShaderValueArray(
	ShaderRHIParamRef Shader,
	const FShaderParameter& Parameter,
	const bool* Values,
	uint32 NumElements,
	uint32 BaseElementIndex = 0
	)
{
	UE_LOG(LogShaders, Fatal, TEXT("SetShaderValueArray does not support bool arrays."));
}

/**
 * Sets the value of a pixel shader bool parameter.
 */
inline void SetPixelShaderBool(
	FPixelShaderRHIParamRef PixelShader,
	const FShaderParameter& Parameter,
	bool Value
	)
{
	// This will trigger if the parameter was not serialized
	checkSlow(Parameter.IsInitialized());

	if (Parameter.GetNumBytes() > 0)
	{
		// Convert to uint32 before passing to RHI
		uint32 BoolValue = Value;
		RHISetShaderParameter(
			PixelShader,
			Parameter.GetBufferIndex(),
			Parameter.GetBaseIndex(),
			sizeof(BoolValue),
			&BoolValue
			);
	}
}

/**
 * Sets the value of a shader texture parameter.  Template'd on shader type
 */
template<typename ShaderTypeRHIParamRef>
FORCEINLINE void SetTextureParameter(
	ShaderTypeRHIParamRef Shader,
	const FShaderResourceParameter& TextureParameter,
	const FShaderResourceParameter& SamplerParameter,
	const FTexture* Texture,
	uint32 ElementIndex = 0
	)
{
	// This will trigger if the parameter was not serialized
	checkSlow(TextureParameter.IsInitialized());
	checkSlow(SamplerParameter.IsInitialized());
	if(TextureParameter.IsBound())
	{
		Texture->LastRenderTime = GCurrentTime;

		if (ElementIndex < TextureParameter.GetNumResources())
		{
			RHISetShaderTexture(Shader,TextureParameter.GetBaseIndex() + ElementIndex,Texture->TextureRHI);
		}
	}
	
	// @todo ue4 samplerstate Should we maybe pass in two separate values? SamplerElement and TextureElement? Or never allow an array of samplers? Unsure best
	// if there is a matching sampler for this texture array index (ElementIndex), then set it. This will help with this case:
	//			Texture2D LightMapTextures[NUM_LIGHTMAP_COEFFICIENTS];
	//			SamplerState LightMapTexturesSampler;
	// In this case, we only set LightMapTexturesSampler when ElementIndex is 0, we don't set the sampler state for all 4 textures
	// This assumes that the all textures want to use the same sampler state
	if(SamplerParameter.IsBound())
	{
		if (ElementIndex < SamplerParameter.GetNumResources())
		{
			RHISetShaderSampler(Shader,SamplerParameter.GetBaseIndex() + ElementIndex,Texture->SamplerStateRHI);
		}
	}
}

/**
 * Sets the value of a shader texture parameter. Template'd on shader type.
 */
template<typename ShaderTypeRHIParamRef>
FORCEINLINE void SetTextureParameter(
	ShaderTypeRHIParamRef Shader,
	const FShaderResourceParameter& TextureParameter,
	const FShaderResourceParameter& SamplerParameter,
	FSamplerStateRHIParamRef SamplerStateRHI,
	FTextureRHIParamRef TextureRHI,
	uint32 ElementIndex = 0
	)
{
	// This will trigger if the parameter was not serialized
	checkSlow(TextureParameter.IsInitialized());
	checkSlow(SamplerParameter.IsInitialized());
	if(TextureParameter.IsBound())
	{
		if (ElementIndex < TextureParameter.GetNumResources())
		{
			RHISetShaderTexture(Shader,TextureParameter.GetBaseIndex() + ElementIndex,TextureRHI);
		}
	}
	// @todo ue4 samplerstate Should we maybe pass in two separate values? SamplerElement and TextureElement? Or never allow an array of samplers? Unsure best
	// if there is a matching sampler for this texture array index (ElementIndex), then set it. This will help with this case:
	//			Texture2D LightMapTextures[NUM_LIGHTMAP_COEFFICIENTS];
	//			SamplerState LightMapTexturesSampler;
	// In this case, we only set LightMapTexturesSampler when ElementIndex is 0, we don't set the sampler state for all 4 textures
	// This assumes that the all textures want to use the same sampler state
	if(SamplerParameter.IsBound())
	{
		if (ElementIndex < SamplerParameter.GetNumResources())
		{
			RHISetShaderSampler(Shader,SamplerParameter.GetBaseIndex() + ElementIndex,SamplerStateRHI);
		}
	}
}

/**
 * Sets the value of a shader surface parameter (e.g. to access MSAA samples).
 * Template'd on shader type (e.g. pixel shader or compute shader).
 */
template<typename ShaderTypeRHIParamRef>
FORCEINLINE void SetTextureParameter(
	ShaderTypeRHIParamRef Shader,
	const FShaderResourceParameter& Parameter,
	FTextureRHIParamRef NewTextureRHI
	)
{
	if(Parameter.IsBound())
	{
		RHISetShaderTexture(
			Shader,
			Parameter.GetBaseIndex(),
			NewTextureRHI
			);
	}
}

/**
 * Sets the value of a shader sampler parameter. Template'd on shader type.
 */
template<typename ShaderTypeRHIParamRef>
FORCEINLINE void SetSamplerParameter(
	ShaderTypeRHIParamRef Shader,
	const FShaderResourceParameter& Parameter,
	FSamplerStateRHIParamRef SamplerStateRHI
	)
{
	if(Parameter.IsBound())
	{
		RHISetShaderSampler(
			Shader,
			Parameter.GetBaseIndex(),
			SamplerStateRHI
			);
	}
}

/**
 * Sets the value of a shader resource view parameter
 * Template'd on shader type (e.g. pixel shader or compute shader).
 */
template<typename ShaderTypeRHIParamRef>
FORCEINLINE void SetSRVParameter(
	ShaderTypeRHIParamRef Shader,
	const FShaderResourceParameter& Parameter,
	FShaderResourceViewRHIParamRef NewShaderResourceViewRHI
	)
{
	if(Parameter.IsBound())
	{
		RHISetShaderResourceViewParameter(
			Shader,
			Parameter.GetBaseIndex(),
			NewShaderResourceViewRHI
			);
	}
}

/**
 * Sets the value of a unordered access view parameter
 */
FORCEINLINE void SetUAVParameter(
	FComputeShaderRHIParamRef ComputeShader,
	const FShaderResourceParameter& Parameter,
	FUnorderedAccessViewRHIParamRef NewUnorderedAccessViewRHI
	)
{
	if(Parameter.IsBound())
	{
		RHISetUAVParameter(
			ComputeShader,
			Parameter.GetBaseIndex(),
			NewUnorderedAccessViewRHI
			);
	}
}

template<typename TShaderRHIRef>
inline void SetUAVParameterIfCS(TShaderRHIRef,const FShaderResourceParameter& UAVParameter,FUnorderedAccessViewRHIParamRef UAV)
{
}

template<> inline void SetUAVParameterIfCS<FComputeShaderRHIParamRef>(FComputeShaderRHIParamRef ComputeShader,const FShaderResourceParameter& UAVParameter,FUnorderedAccessViewRHIParamRef UAV)
{
	SetUAVParameter(ComputeShader,UAVParameter,UAV);
}

template<> inline void SetUAVParameterIfCS<FComputeShaderRHIRef>(FComputeShaderRHIRef ComputeShader,const FShaderResourceParameter& UAVParameter,FUnorderedAccessViewRHIParamRef UAV)
{
	SetUAVParameter(ComputeShader,UAVParameter,UAV);
}

/** A class that binds either a UAV or SRV of a resource. */
class FRWShaderParameter
{
public:

	void Bind(const FShaderParameterMap& ParameterMap,const TCHAR* BaseName)
	{
		SRVParameter.Bind(ParameterMap,BaseName);

		// If the shader wants to bind the parameter as a UAV, the parameter name must start with "RW"
		FString UAVName = FString(TEXT("RW")) + BaseName;
		UAVParameter.Bind(ParameterMap,*UAVName);

		// Verify that only one of the UAV or SRV parameters is accessed by the shader.
		checkf(!(SRVParameter.GetNumResources() && UAVParameter.GetNumResources()),TEXT("Shader binds SRV and UAV of the same resource: %s"),BaseName);
	}

	bool IsBound() const
	{
		return SRVParameter.IsBound() || UAVParameter.IsBound();
	}

	friend FArchive& operator<<(FArchive& Ar,FRWShaderParameter& Parameter)
	{
		return Ar << Parameter.SRVParameter << Parameter.UAVParameter;
	}

	template<typename TShaderRHIRef>
	void SetBuffer(TShaderRHIRef Shader,const FRWBuffer& RWBuffer) const
	{
		SetSRVParameter(Shader,SRVParameter,RWBuffer.SRV);
		SetUAVParameterIfCS(Shader,UAVParameter,RWBuffer.UAV);
	}

	template<typename TShaderRHIRef>
	void SetTexture(TShaderRHIRef Shader,const FTextureRHIParamRef Texture,FUnorderedAccessViewRHIParamRef UAV) const
	{
		SetTextureParameter(Shader,SRVParameter,Texture);
		SetUAVParameterIfCS(Shader,UAVParameter,UAV);
	}

	void UnsetUAV(FComputeShaderRHIParamRef ComputeShader) const
	{
		SetUAVParameter(ComputeShader,UAVParameter,FUnorderedAccessViewRHIRef());
	}

private:

	FShaderResourceParameter SRVParameter;
	FShaderResourceParameter UAVParameter;
};

/** Creates a shader code declaration of this struct for the given shader platform. */
extern SHADERCORE_API FString CreateUniformBufferShaderDeclaration(const TCHAR* Name,const FUniformBufferStruct& UniformBufferStruct,EShaderPlatform Platform);

class FShaderUniformBufferParameter
{
public:
	FShaderUniformBufferParameter()
	:	SetParametersId(0)
	,	BaseIndex(0)
	,	bIsBound(false) 
#if UE_BUILD_DEBUG
	,	bInitialized(false)
#endif
	{}

	static SHADERCORE_API void ModifyCompilationEnvironment(const TCHAR* ParameterName,const FUniformBufferStruct& Struct,EShaderPlatform Platform,FShaderCompilerEnvironment& OutEnvironment);

	SHADERCORE_API void Bind(const FShaderParameterMap& ParameterMap,const TCHAR* ParameterName,EShaderParameterFlags Flags = SPF_Optional);

	friend FArchive& operator<<(FArchive& Ar,FShaderUniformBufferParameter& P)
	{
		P.Serialize(Ar);
		return Ar;
	}

	bool IsBound() const { return bIsBound; }

	void Serialize(FArchive& Ar)
	{
#if UE_BUILD_DEBUG
		if (Ar.IsLoading())
		{
			bInitialized = true;
		}
#endif
		Ar << BaseIndex << bIsBound;
	}

	inline bool IsInitialized() const 
	{ 
#if UE_BUILD_DEBUG
		return bInitialized; 
#else 
		return true;
#endif
	}

	inline void SetInitialized() 
	{ 
#if UE_BUILD_DEBUG
		bInitialized = true; 
#endif
	}

	uint32 GetBaseIndex() const { return BaseIndex; }

	/** Used to track when a parameter was set, to detect cases where a bound parameter is used for rendering without being set. */
	mutable uint32 SetParametersId;

private:
	uint16 BaseIndex;
	bool bIsBound;

#if UE_BUILD_DEBUG
	bool bInitialized;
#endif
};

/** Sets the value of a shader uniform buffer parameter to a uniform buffer containing the struct. */
template<typename TShaderRHIRef>
void SetUniformBufferParameter(
	TShaderRHIRef Shader,
	const FShaderUniformBufferParameter& Parameter,
	FUniformBufferRHIParamRef UniformBufferRHI
	)
{
	// This will trigger if the parameter was not serialized
	checkSlow(Parameter.IsInitialized());
	if(Parameter.IsBound())
	{
		RHISetShaderUniformBuffer(Shader,Parameter.GetBaseIndex(),UniformBufferRHI);
	}
}

/** A shader uniform buffer binding with a specific structure. */
template<typename TBufferStruct>
class TShaderUniformBufferParameter : public FShaderUniformBufferParameter
{
public:
	static void ModifyCompilationEnvironment(const TCHAR* ParameterName,EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FShaderUniformBufferParameter::ModifyCompilationEnvironment(ParameterName,TBufferStruct::StaticStruct,Platform,OutEnvironment);
	}

	friend FArchive& operator<<(FArchive& Ar,TShaderUniformBufferParameter& P)
	{
		P.Serialize(Ar);
		return Ar;
	}
};

/** Sets the value of a shader uniform buffer parameter to a uniform buffer containing the struct. */
template<typename TShaderRHIRef,typename TBufferStruct>
void SetUniformBufferParameter(
	TShaderRHIRef Shader,
	const TShaderUniformBufferParameter<TBufferStruct>& Parameter,
	const TUniformBufferRef<TBufferStruct>& UniformBufferRef
	)
{
	// This will trigger if the parameter was not serialized
	checkSlow(Parameter.IsInitialized());
	if(Parameter.IsBound())
	{
		RHISetShaderUniformBuffer(Shader,Parameter.GetBaseIndex(),UniformBufferRef);
	}
}

/** Sets the value of a shader uniform buffer parameter to a uniform buffer containing the struct. */
template<typename TShaderRHIRef,typename TBufferStruct>
void SetUniformBufferParameter(
	TShaderRHIRef Shader,
	const TShaderUniformBufferParameter<TBufferStruct>& Parameter,
	const TUniformBuffer<TBufferStruct>& UniformBuffer
	)
{
	// This will trigger if the parameter was not serialized
	checkSlow(Parameter.IsInitialized());
	if(Parameter.IsBound())
	{
		RHISetShaderUniformBuffer(Shader,Parameter.GetBaseIndex(),UniformBuffer.GetUniformBufferRHI());
	}
}

/** Sets the value of a shader uniform buffer parameter to a value of the struct. */
template<typename TShaderRHIRef,typename TBufferStruct>
void SetUniformBufferParameterImmediate(
	TShaderRHIRef Shader,
	const TShaderUniformBufferParameter<TBufferStruct>& Parameter,
	const TBufferStruct& UniformBufferValue
	)
{
	// This will trigger if the parameter was not serialized
	checkSlow(Parameter.IsInitialized());
	if(Parameter.IsBound())
	{
		RHISetShaderUniformBuffer(
			Shader,
			Parameter.GetBaseIndex(),
			RHICreateUniformBuffer(&UniformBufferValue,sizeof(UniformBufferValue),UniformBuffer_SingleUse)
			);
	}
}
