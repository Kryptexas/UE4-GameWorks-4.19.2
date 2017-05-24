// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraCommon.h"
#include "NiagaraParameters.h"

class UNiagaraNode;
class UNiagaraGraph;
class UEdGraphPin;


extern void SHADERFORMATVECTORVM_API CompileShader_VectorVM(const struct FShaderCompilerInput& Input, struct FShaderCompilerOutput& Output, const class FString& WorkingDirectory, uint8 Version);

//Cheating hack version. To be removed when we add all the plumbing for VVM scripts to be treat like real shaders.
extern void SHADERFORMATVECTORVM_API CompileShader_VectorVM(const struct FShaderCompilerInput& Input,struct FShaderCompilerOutput& Output,const class FString& WorkingDirectory, uint8 Version, struct FNiagaraCompilationOutput& NiagaraOutput);
