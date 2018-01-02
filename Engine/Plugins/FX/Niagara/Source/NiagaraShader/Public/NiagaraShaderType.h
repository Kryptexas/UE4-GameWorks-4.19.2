// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NiagaraShaderType.h: Niagara shader type definition.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "Engine/EngineTypes.h"

/** A macro to implement Niagara shaders. */
#define IMPLEMENT_NIAGARA_SHADER_TYPE(TemplatePrefix,ShaderClass,SourceFilename,FunctionName,Frequency) \
	IMPLEMENT_SHADER_TYPE( \
		TemplatePrefix, \
		ShaderClass, \
		SourceFilename, \
		FunctionName, \
		Frequency \
		);

class FNiagaraScript;
class FShaderCommonCompileJob;
class FShaderCompileJob;
class FUniformExpressionSet;


/** Called for every Niagara shader to update the appropriate stats. */
extern void UpdateNiagaraShaderCompilingStats(const FNiagaraScript* Script);

/**
 * Dump shader stats for a given platform.
 * 
 * @param	Platform	Platform to dump stats for.
 */
extern ENGINE_API void DumpComputeShaderStats( EShaderPlatform Platform );




/**
 * A shader meta type for niagara-linked shaders.
 */
class FNiagaraShaderType : public FShaderType
{
public:
	struct CompiledShaderInitializerType : FGlobalShaderType::CompiledShaderInitializerType
	{
		//const FUniformExpressionSet& UniformExpressionSet;
		const FString DebugDescription;
		TArray< FDIBufferDescriptorStore > DIBufferDescriptors;

		CompiledShaderInitializerType(
			FShaderType* InType,
			int32 InPermutationId,
			const FShaderCompilerOutput& CompilerOutput,
			FShaderResource* InResource,
			const FSHAHash& InNiagaraShaderMapHash,
			const FString& InDebugDescription,
			const TArray< FDIBufferDescriptorStore > &InDataInterfaceBufferDescriptors
			)
		: FGlobalShaderType::CompiledShaderInitializerType(InType,InPermutationId,CompilerOutput,InResource, InNiagaraShaderMapHash,nullptr,nullptr)
		, DebugDescription(InDebugDescription)
		, DIBufferDescriptors(InDataInterfaceBufferDescriptors)
		{}
	};
	typedef FShader* (*ConstructCompiledType)(const CompiledShaderInitializerType&);
	typedef bool (*ShouldCompilePermutationType)(EShaderPlatform,const FNiagaraScript*);
	typedef void (*ModifyCompilationEnvironmentType)(EShaderPlatform,const FNiagaraScript*, FShaderCompilerEnvironment&);

	FNiagaraShaderType(
		const TCHAR* InName,
		const TCHAR* InSourceFilename,
		const TCHAR* InFunctionName,
		uint32 InFrequency,					// ugly - ignored for Niagara shaders but needed for IMPLEMENT_SHADER_TYPE macro magic
		int32 InTotalPermutationCount,
		ConstructSerializedType InConstructSerializedRef,
		ConstructCompiledType InConstructCompiledRef,
		ModifyCompilationEnvironmentType InModifyCompilationEnvironmentRef,
		ShouldCompilePermutationType InShouldCompilePermutationRef,
		GetStreamOutElementsType InGetStreamOutElementsRef
		):
		FShaderType(EShaderTypeForDynamicCast::Niagara, InName, InSourceFilename, InFunctionName, SF_Compute, InTotalPermutationCount, InConstructSerializedRef, InGetStreamOutElementsRef),
		ConstructCompiledRef(InConstructCompiledRef),
		ShouldCompilePermutationRef(InShouldCompilePermutationRef),
		ModifyCompilationEnvironmentRef(InModifyCompilationEnvironmentRef)
	{
		check(InTotalPermutationCount == 1);
	}

	/**
	 * Enqueues a compilation for a new shader of this type.
	 * @param Script - The script to link the shader with.
	 */
	class FNiagaraShaderCompileJob* BeginCompileShader(
		uint32 ShaderMapId,
		const FNiagaraScript* Script,
		FShaderCompilerEnvironment* ScriptEnvironment,
		EShaderPlatform Platform,
		TArray<FNiagaraShaderCompileJob*>& NewJobs,
		FShaderTarget Target,
		TArray< FDIBufferDescriptorStore >&InBufferDescriptors
		);

	/**
	 * Either creates a new instance of this type or returns an equivalent existing shader.
	 * @param script - The script to link the shader with.
	 * @param CurrentJob - Compile job that was enqueued by BeginCompileShader.
	 */
	FShader* FinishCompileShader(
		const FSHAHash& NiagaraShaderMapHash,
		const FNiagaraShaderCompileJob& CurrentJob,
		const FString& InDebugDescription
		);

	/**
	 * Checks if the shader type should be cached for a particular platform and script.
	 * @param Platform - The platform to check.
	 * @param script - The script to check.
	 * @return True if this shader type should be cached.
	 */
	bool ShouldCache(EShaderPlatform Platform,const FNiagaraScript* Script) const
	{
		return (*ShouldCompilePermutationRef)(Platform, Script);
	}

protected:

	/**
	 * Sets up the environment used to compile an instance of this shader type.
	 * @param Platform - Platform to compile for.
	 * @param Environment - The shader compile environment that the function modifies.
	 */
	void SetupCompileEnvironment(EShaderPlatform Platform, const FNiagaraScript* Script, FShaderCompilerEnvironment& Environment)
	{
		// Allow the shader type to modify its compile environment.
		(*ModifyCompilationEnvironmentRef)(Platform, Script, Environment);
	}

private:
	ConstructCompiledType ConstructCompiledRef;
	ShouldCompilePermutationType ShouldCompilePermutationRef;
	ModifyCompilationEnvironmentType ModifyCompilationEnvironmentRef;
};
