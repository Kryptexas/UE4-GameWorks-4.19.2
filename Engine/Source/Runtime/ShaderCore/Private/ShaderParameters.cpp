// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ShaderParameters.cpp: Shader parameter implementation.
=============================================================================*/

#include "ShaderCore.h"
#include "Shader.h"
#include "VertexFactory.h"

void FShaderParameter::Bind(const FShaderParameterMap& ParameterMap,const TCHAR* ParameterName,EShaderParameterFlags Flags)
{
#if UE_BUILD_DEBUG
	bInitialized = true;
#endif

	if (!ParameterMap.FindParameterAllocation(ParameterName,BufferIndex,BaseIndex,NumBytes) && Flags == SPF_Mandatory)
	{
		if (!UE_LOG_ACTIVE(LogShaders, Log))
		{
			UE_LOG(LogShaders, Fatal,TEXT("Failure to bind non-optional shader parameter %s!  The parameter is either not present in the shader, or the shader compiler optimized it out."),ParameterName);
		}
		else
		{
			// We use a non-Slate message box to avoid problem where we haven't compiled the shaders for Slate.
			FPlatformMisc::MessageBoxExt( EAppMsgType::Ok, *FText::Format(
				NSLOCTEXT("UnrealEd", "Error_FailedToBindShaderParameter", "Failure to bind non-optional shader parameter {0}! The parameter is either not present in the shader, or the shader compiler optimized it out. This will be an assert with LogShaders suppressed!"),
				FText::FromString(ParameterName)).ToString(), TEXT("Warning"));
		}
	}
}

FArchive& operator<<(FArchive& Ar,FShaderParameter& P)
{
#if UE_BUILD_DEBUG
	if (Ar.IsLoading())
	{
		P.bInitialized = true;
	}
#endif

	uint16& PBufferIndex = P.BufferIndex;
	return Ar << P.BaseIndex << P.NumBytes << PBufferIndex;
}

void FShaderResourceParameter::Bind(const FShaderParameterMap& ParameterMap,const TCHAR* ParameterName,EShaderParameterFlags Flags)
{
	uint16 UnusedBufferIndex = 0;

#if UE_BUILD_DEBUG
	bInitialized = true;
#endif

	if(!ParameterMap.FindParameterAllocation(ParameterName,UnusedBufferIndex,BaseIndex,NumResources) && Flags == SPF_Mandatory)
	{
		if (!UE_LOG_ACTIVE(LogShaders, Log))
		{
			UE_LOG(LogShaders, Fatal,TEXT("Failure to bind non-optional shader resource parameter %s!  The parameter is either not present in the shader, or the shader compiler optimized it out."),ParameterName);
		}
		else
		{
			// We use a non-Slate message box to avoid problem where we haven't compiled the shaders for Slate.
			FPlatformMisc::MessageBoxExt( EAppMsgType::Ok, *FText::Format(
				NSLOCTEXT("UnrealEd", "Error_FailedToBindShaderParameter", "Failure to bind non-optional shader parameter {0}! The parameter is either not present in the shader, or the shader compiler optimized it out. This will be an assert with LogShaders suppressed!"),
				FText::FromString(ParameterName)).ToString(), TEXT("Warning"));
		}
	}
}

FArchive& operator<<(FArchive& Ar,FShaderResourceParameter& P)
{
#if UE_BUILD_DEBUG
	if (Ar.IsLoading())
	{
		P.bInitialized = true;
	}
#endif

	return Ar << P.BaseIndex << P.NumResources;
}

void FShaderUniformBufferParameter::ModifyCompilationEnvironment(const TCHAR* ParameterName,const FUniformBufferStruct& Struct,EShaderPlatform Platform,FShaderCompilerEnvironment& OutEnvironment)
{
	const FString IncludeName = FString::Printf(TEXT("UniformBuffers/%s.usf"),ParameterName);
	// Add the uniform buffer declaration to the compilation environment as an include: UniformBuffers/<ParameterName>.usf
	OutEnvironment.IncludeFileNameToContentsMap.Add(
		*IncludeName,
		CreateUniformBufferShaderDeclaration(ParameterName,Struct,Platform)
		);

	FString& GeneratedUniformBuffersInclude = OutEnvironment.IncludeFileNameToContentsMap.FindOrAdd("GeneratedUniformBuffers.usf");
	GeneratedUniformBuffersInclude += FString::Printf(TEXT("#include \"UniformBuffers/%s.usf\"")LINE_TERMINATOR, ParameterName);
}

void FShaderUniformBufferParameter::Bind(const FShaderParameterMap& ParameterMap,const TCHAR* ParameterName,EShaderParameterFlags Flags)
{
	uint16 UnusedBaseIndex = 0;
	uint16 UnusedNumBytes = 0;

#if UE_BUILD_DEBUG
	bInitialized = true;
#endif

	if(!ParameterMap.FindParameterAllocation(ParameterName,BaseIndex,UnusedBaseIndex,UnusedNumBytes))
	{
		bIsBound = false;
		if(Flags == SPF_Mandatory)
		{
			if (!UE_LOG_ACTIVE(LogShaders, Log))
			{
				UE_LOG(LogShaders, Fatal,TEXT("Failure to bind non-optional shader resource parameter %s!  The parameter is either not present in the shader, or the shader compiler optimized it out."),ParameterName);
			}
			else
			{
				// We use a non-Slate message box to avoid problem where we haven't compiled the shaders for Slate.
				FPlatformMisc::MessageBoxExt( EAppMsgType::Ok, *FText::Format(
					NSLOCTEXT("UnrealEd", "Error_FailedToBindShaderParameter", "Failure to bind non-optional shader parameter {0}! The parameter is either not present in the shader, or the shader compiler optimized it out. This will be an assert with LogShaders suppressed!"),
					FText::FromString(ParameterName)).ToString(), TEXT("Warning"));
			}
		}
	}
	else
	{
		bIsBound = true;
	}
}

/** Generates a HLSL struct declaration for a uniform buffer struct. */
static FString CreateHLSLUniformBufferStructMembersDeclaration(const FUniformBufferStruct& UniformBufferStruct, bool bIgnorePadding)
{
	FString Result;
	uint32 HLSLBaseOffset = 0;

	const TArray<FUniformBufferStruct::FMember>& StructMembers = UniformBufferStruct.GetMembers();
	for(int32 MemberIndex = 0;MemberIndex < StructMembers.Num();++MemberIndex)
	{
		const FUniformBufferStruct::FMember& Member = StructMembers[MemberIndex];
		
		FString ArrayDim;
		if(Member.GetNumElements() > 0)
		{
			ArrayDim = FString::Printf(TEXT("[%u]"),Member.GetNumElements());
		}

		if(Member.GetBaseType() == UBMT_STRUCT)
		{
			Result += TEXT("struct {\r\n");
			Result += CreateHLSLUniformBufferStructMembersDeclaration(*Member.GetStruct(), bIgnorePadding);
			Result += FString::Printf(TEXT("} %s%s;\r\n"),Member.GetName(),*ArrayDim);
			HLSLBaseOffset += Member.GetStruct()->GetSize() * Member.GetNumElements();
		}
		else
		{
			// Generate the base type name.
			FString BaseTypeName;
			switch(Member.GetBaseType())
			{
			case UBMT_BOOL:    BaseTypeName = TEXT("bool"); break;
			case UBMT_INT32:   BaseTypeName = TEXT("int"); break;
			case UBMT_UINT32:  BaseTypeName = TEXT("uint"); break;
			case UBMT_FLOAT32: 
				if (Member.GetPrecision() == EShaderPrecisionModifier::Float)
				{
					BaseTypeName = TEXT("float"); 
				}
				else if (Member.GetPrecision() == EShaderPrecisionModifier::Half)
				{
					BaseTypeName = TEXT("half"); 
				}
				else if (Member.GetPrecision() == EShaderPrecisionModifier::Fixed)
				{
					BaseTypeName = TEXT("fixed"); 
				}
				break;
			default:           UE_LOG(LogShaders, Fatal,TEXT("Unrecognized uniform buffer struct member base type."));
			};

			// Generate the type dimensions for vectors and matrices.
			FString TypeDim;
			uint32 HLSLMemberSize = 4;
			if(Member.GetNumRows() > 1)
			{
				TypeDim = FString::Printf(TEXT("%ux%u"),Member.GetNumRows(),Member.GetNumColumns());

				// Each row of a matrix is 16 byte aligned.
				HLSLMemberSize = (Member.GetNumRows() - 1) * 16 + Member.GetNumColumns() * 4;
			}
			else if(Member.GetNumColumns() > 1)
			{
				TypeDim = FString::Printf(TEXT("%u"),Member.GetNumColumns());
				HLSLMemberSize = Member.GetNumColumns() * 4;
			}

			// Array elements are 16 byte aligned.
			if(Member.GetNumElements() > 0)
			{
				HLSLMemberSize = (Member.GetNumElements() - 1) * Align(HLSLMemberSize,16) + HLSLMemberSize;
			}

			// If the HLSL offset doesn't match the C++ offset, generate padding to fix it.
			if(HLSLBaseOffset != Member.GetOffset())
			{
				check(HLSLBaseOffset < Member.GetOffset());
				while(HLSLBaseOffset < Member.GetOffset())
				{
					if(!bIgnorePadding)
						Result += FString::Printf(TEXT("\tfloat1 _PrePadding%u;\r\n"),HLSLBaseOffset);
					HLSLBaseOffset += 4;
				};
				check(HLSLBaseOffset == Member.GetOffset());
			}

			HLSLBaseOffset = Member.GetOffset() + HLSLMemberSize;

			// Generate the member declaration.
			Result += FString::Printf(TEXT("\t%s%s %s%s;\r\n"),*BaseTypeName,*TypeDim,Member.GetName(),*ArrayDim);
		}
	}

	return Result;
}

/**
 * Creates a HLSL declaration of a uniform buffer with the given structure.
 * @param bIgnoreRowMajor on OpenGL this is the default so we suppress it (can be moved to the HLSL2GLSL converter)
 */
static FString CreateHLSLSM5UniformBufferDeclaration(const TCHAR* Name,const FUniformBufferStruct& UniformBufferStruct, bool bIgnorePadding)
{
	const FString StructMemberDeclaration = CreateHLSLUniformBufferStructMembersDeclaration(UniformBufferStruct, bIgnorePadding);

	return FString::Printf(
		TEXT("#ifndef __UniformBuffer_%s_Definition__\r\n")
		TEXT("#define __UniformBuffer_%s_Definition__\r\n")
		TEXT("cbuffer %s\r\n")
		TEXT("{\r\n")
		TEXT("\tstruct\r\n")
		TEXT("\t{\r\n")
		TEXT("%s")
		TEXT("\t} %s;\r\n")
		TEXT("}\r\n")
		TEXT("#endif\r\n"),
		Name,
		Name,
		Name,
		*StructMemberDeclaration,
		Name
		);
}

FString CreateUniformBufferShaderDeclaration(const TCHAR* Name,const FUniformBufferStruct& UniformBufferStruct,EShaderPlatform Platform)
{
	switch(Platform)
	{
	default:
	case SP_PCD3D_SM5:
		return CreateHLSLSM5UniformBufferDeclaration(Name,UniformBufferStruct, false);
	case SP_OPENGL_SM4:
	case SP_OPENGL_SM5:
		return CreateHLSLSM5UniformBufferDeclaration(Name,UniformBufferStruct, true);
	}
}

void CacheUniformBufferIncludes(TMap<const TCHAR*,FCachedUniformBufferDeclaration>& Cache, EShaderPlatform Platform)
{
	for (TMap<const TCHAR*,FCachedUniformBufferDeclaration>::TIterator It(Cache); It; ++It)
	{
		FCachedUniformBufferDeclaration& BufferDeclaration = It.Value();
		check(BufferDeclaration.Declaration[Platform].Len() == 0);

		for (TLinkedList<FUniformBufferStruct*>::TIterator StructIt(FUniformBufferStruct::GetStructList()); StructIt; StructIt.Next())
		{
			if (It.Key() == StructIt->GetShaderVariableName())
			{
				BufferDeclaration.Declaration[Platform] = CreateUniformBufferShaderDeclaration(StructIt->GetShaderVariableName(), **StructIt, Platform);
				break;
			}
		}
	}
}

void FShaderType::AddReferencedUniformBufferIncludes(FShaderCompilerEnvironment& OutEnvironment, FString& OutSourceFilePrefix, EShaderPlatform Platform)
{
	// Cache uniform buffer struct declarations referenced by this shader type's files
	if (!bCachedUniformBufferStructDeclarations[Platform])
	{
		CacheUniformBufferIncludes(ReferencedUniformBufferStructsCache, Platform);
		bCachedUniformBufferStructDeclarations[Platform] = true;
	}

	FString UniformBufferIncludes;

	for (TMap<const TCHAR*,FCachedUniformBufferDeclaration>::TConstIterator StructIt(ReferencedUniformBufferStructsCache); StructIt; ++StructIt)
	{
		check(StructIt.Value().Declaration[Platform].Len() > 0);
		UniformBufferIncludes += FString::Printf(TEXT("#include \"UniformBuffers/%s.usf\"")LINE_TERMINATOR, StructIt.Key());
		OutEnvironment.IncludeFileNameToContentsMap.Add(
			*FString::Printf(TEXT("UniformBuffers/%s.usf"),StructIt.Key()),
			StructIt.Value().Declaration[Platform]
			);
	}

	FString& GeneratedUniformBuffersInclude = OutEnvironment.IncludeFileNameToContentsMap.FindOrAdd("GeneratedUniformBuffers.usf");
	GeneratedUniformBuffersInclude += UniformBufferIncludes;
}

void FVertexFactoryType::AddReferencedUniformBufferIncludes(FShaderCompilerEnvironment& OutEnvironment, FString& OutSourceFilePrefix, EShaderPlatform Platform)
{
	// Cache uniform buffer struct declarations referenced by this shader type's files
	if (!bCachedUniformBufferStructDeclarations[Platform])
	{
		CacheUniformBufferIncludes(ReferencedUniformBufferStructsCache, Platform);
		bCachedUniformBufferStructDeclarations[Platform] = true;
	}

	FString UniformBufferIncludes;

	for (TMap<const TCHAR*,FCachedUniformBufferDeclaration>::TConstIterator StructIt(ReferencedUniformBufferStructsCache); StructIt; ++StructIt)
	{
		check(StructIt.Value().Declaration[Platform].Len() > 0);
		UniformBufferIncludes += FString::Printf(TEXT("#include \"UniformBuffers/%s.usf\"")LINE_TERMINATOR, StructIt.Key());
		OutEnvironment.IncludeFileNameToContentsMap.Add(
			*FString::Printf(TEXT("UniformBuffers/%s.usf"),StructIt.Key()),
			StructIt.Value().Declaration[Platform]
		);
	}

	FString& GeneratedUniformBuffersInclude = OutEnvironment.IncludeFileNameToContentsMap.FindOrAdd("GeneratedUniformBuffers.usf");
	GeneratedUniformBuffersInclude += UniformBufferIncludes;
}
