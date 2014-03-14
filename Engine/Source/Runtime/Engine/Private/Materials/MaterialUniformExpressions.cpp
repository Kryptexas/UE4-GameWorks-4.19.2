// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MaterialShared.cpp: Shared material implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "MaterialUniformExpressions.h"

TLinkedList<FMaterialUniformExpressionType*>*& FMaterialUniformExpressionType::GetTypeList()
{
	static TLinkedList<FMaterialUniformExpressionType*>* TypeList = NULL;
	return TypeList;
}

TMap<FName,FMaterialUniformExpressionType*>& FMaterialUniformExpressionType::GetTypeMap()
{
	static TMap<FName,FMaterialUniformExpressionType*> TypeMap;

	// Move types from the type list to the type map.
	TLinkedList<FMaterialUniformExpressionType*>* TypeListLink = GetTypeList();
	while(TypeListLink)
	{
		TLinkedList<FMaterialUniformExpressionType*>* NextLink = TypeListLink->Next();
		FMaterialUniformExpressionType* Type = **TypeListLink;

		TypeMap.Add(FName(Type->Name),Type);
		TypeListLink->Unlink();
		delete TypeListLink;

		TypeListLink = NextLink;
	}

	return TypeMap;
}

FMaterialUniformExpressionType::FMaterialUniformExpressionType(
	const TCHAR* InName,
	SerializationConstructorType InSerializationConstructor
	):
	Name(InName),
	SerializationConstructor(InSerializationConstructor)
{
	// Put the type in the type list until the name subsystem/type map are initialized.
	(new TLinkedList<FMaterialUniformExpressionType*>(this))->Link(GetTypeList());
}

FArchive& operator<<(FArchive& Ar,FMaterialUniformExpression*& Ref)
{
	// Serialize the expression type.
	if(Ar.IsSaving())
	{
		// Write the type name.
		check(Ref);
		FName TypeName(Ref->GetType()->Name);
		Ar << TypeName;
	}
	else if(Ar.IsLoading())
	{
		// Read the type name.
		FName TypeName = NAME_None;
		Ar << TypeName;

		// Find the expression type with a matching name.
		FMaterialUniformExpressionType* Type = FMaterialUniformExpressionType::GetTypeMap().FindRef(TypeName);
		check(Type);

		// Construct a new instance of the expression type.
		Ref = (*Type->SerializationConstructor)();
	}

	// Serialize the expression.
	Ref->Serialize(Ar);

	return Ar;
}

FArchive& operator<<(FArchive& Ar,FMaterialUniformExpressionTexture*& Ref)
{
	Ar << (FMaterialUniformExpression*&)Ref;
	return Ar;
}

void FUniformExpressionSet::Serialize(FArchive& Ar)
{
	Ar << UniformVectorExpressions;
	Ar << UniformScalarExpressions;
	Ar << Uniform2DTextureExpressions;
	Ar << UniformCubeTextureExpressions;

	Ar << ParameterCollections;

	// Recreate the uniform buffer struct after loading.
	if(Ar.IsLoading())
	{
		CreateBufferStruct();
	}
}

bool FUniformExpressionSet::IsEmpty() const
{
	return UniformVectorExpressions.Num() == 0
		&& UniformScalarExpressions.Num() == 0
		&& Uniform2DTextureExpressions.Num() == 0
		&& UniformCubeTextureExpressions.Num() == 0
		&& ParameterCollections.Num() == 0;
}

bool FUniformExpressionSet::operator==(const FUniformExpressionSet& ReferenceSet) const
{
	if(UniformVectorExpressions.Num() != ReferenceSet.UniformVectorExpressions.Num()
	|| UniformScalarExpressions.Num() != ReferenceSet.UniformScalarExpressions.Num()
	|| Uniform2DTextureExpressions.Num() != ReferenceSet.Uniform2DTextureExpressions.Num()
	|| UniformCubeTextureExpressions.Num() != ReferenceSet.UniformCubeTextureExpressions.Num()
	|| ParameterCollections.Num() != ReferenceSet.ParameterCollections.Num())
	{
		return false;
	}

	for (int32 i = 0; i < UniformVectorExpressions.Num(); i++)
	{
		if (!UniformVectorExpressions[i]->IsIdentical(ReferenceSet.UniformVectorExpressions[i]))
		{
			return false;
		}
	}

	for (int32 i = 0; i < UniformScalarExpressions.Num(); i++)
	{
		if (!UniformScalarExpressions[i]->IsIdentical(ReferenceSet.UniformScalarExpressions[i]))
		{
			return false;
		}
	}

	for (int32 i = 0; i < Uniform2DTextureExpressions.Num(); i++)
	{
		if (!Uniform2DTextureExpressions[i]->IsIdentical(ReferenceSet.Uniform2DTextureExpressions[i]))
		{
			return false;
		}
	}

	for (int32 i = 0; i < UniformCubeTextureExpressions.Num(); i++)
	{
		if (!UniformCubeTextureExpressions[i]->IsIdentical(ReferenceSet.UniformCubeTextureExpressions[i]))
		{
			return false;
		}
	}

	for (int32 i = 0; i < ParameterCollections.Num(); i++)
	{
		if (ParameterCollections[i] != ReferenceSet.ParameterCollections[i])
		{
			return false;
		}
	}

	return true;
}

FString FUniformExpressionSet::GetSummaryString() const
{
	return FString::Printf(TEXT("(%u vectors, %u scalars, %u 2d tex, %u cube tex, %u collections)"),
		UniformVectorExpressions.Num(), 
		UniformScalarExpressions.Num(),
		Uniform2DTextureExpressions.Num(),
		UniformCubeTextureExpressions.Num(),
		ParameterCollections.Num()
		);
}

void FUniformExpressionSet::GetResourcesString(EShaderPlatform ShaderPlatform,FString& InputsString) const
{
	for(int32 TextureIndex = 0;TextureIndex < Uniform2DTextureExpressions.Num();TextureIndex++)
	{
		InputsString += FString::Printf(TEXT("Texture2D MaterialTexture2D_%i;\r\n"), TextureIndex);
		InputsString += FString::Printf(TEXT("SamplerState MaterialTexture2D_%iSampler;\r\n"), TextureIndex);
	}
	for(int32 TextureIndex = 0;TextureIndex < UniformCubeTextureExpressions.Num();TextureIndex++)
	{
		InputsString += FString::Printf(TEXT("TextureCube MaterialTextureCube_%i;\r\n"),TextureIndex);
		InputsString += FString::Printf(TEXT("SamplerState MaterialTextureCube_%iSampler;\r\n"),TextureIndex);
	}
}

void FUniformExpressionSet::SetParameterCollections(const TArray<UMaterialParameterCollection*>& InCollections)
{
	ParameterCollections.Empty(InCollections.Num());

	for (int32 CollectionIndex = 0; CollectionIndex < InCollections.Num(); CollectionIndex++)
	{
		ParameterCollections.Add(InCollections[CollectionIndex]->StateId);
	}
}

FShaderUniformBufferParameter* ConstructMaterialUniformBufferParameter() { return NULL; }

void FUniformExpressionSet::CreateBufferStruct()
{
	TArray<FUniformBufferStruct::FMember> Members;
	uint32 NextMemberOffset = 0;

	if(UniformVectorExpressions.Num())
	{
		new(Members) FUniformBufferStruct::FMember(TEXT("VectorExpressions"),NextMemberOffset,UBMT_FLOAT32,EShaderPrecisionModifier::Half,1,4,UniformVectorExpressions.Num(),NULL);
		const uint32 VectorArraySize = UniformVectorExpressions.Num() * sizeof(FVector4);
		NextMemberOffset += VectorArraySize;
	}

	if(UniformScalarExpressions.Num())
	{
		new(Members) FUniformBufferStruct::FMember(TEXT("ScalarExpressions"),NextMemberOffset,UBMT_FLOAT32,EShaderPrecisionModifier::Half,1,4,(UniformScalarExpressions.Num() + 3) / 4,NULL);
		const uint32 ScalarArraySize = (UniformScalarExpressions.Num() + 3) / 4 * sizeof(FVector4);
		NextMemberOffset += ScalarArraySize;
	}

	const uint32 StructSize = Align(NextMemberOffset,UNIFORM_BUFFER_STRUCT_ALIGNMENT);
	UniformBufferStruct = new FUniformBufferStruct(
		TEXT("MaterialUniforms"),
		TEXT("Material"),
		ConstructMaterialUniformBufferParameter,
		StructSize,
		Members,
		false
		);
}

const FUniformBufferStruct& FUniformExpressionSet::GetUniformBufferStruct() const
{
	check(UniformBufferStruct);
	return *UniformBufferStruct;
}

FUniformBufferRHIRef FUniformExpressionSet::CreateUniformBuffer(const FMaterialRenderContext& MaterialRenderContext) const
{
	check(UniformBufferStruct);
	check(IsInRenderingThread());
	
	FUniformBufferRHIRef UniformBuffer;

	if (UniformBufferStruct->GetSize() > 0)
	{
		FMemMark Mark(FMemStack::Get());
		void* const TempBuffer = FMemStack::Get().PushBytes(UniformBufferStruct->GetSize(),UNIFORM_BUFFER_STRUCT_ALIGNMENT);

		FLinearColor* TempVectorBuffer = (FLinearColor*)TempBuffer;
		for(int32 VectorIndex = 0;VectorIndex < UniformVectorExpressions.Num();++VectorIndex)
		{
			TempVectorBuffer[VectorIndex] = FLinearColor(0,0,0,0);
			UniformVectorExpressions[VectorIndex]->GetNumberValue(MaterialRenderContext,TempVectorBuffer[VectorIndex]);
		}

		float* TempScalarBuffer = (float*)(TempVectorBuffer + UniformVectorExpressions.Num());
		for(int32 ScalarIndex = 0;ScalarIndex < UniformScalarExpressions.Num();++ScalarIndex)
		{
			FLinearColor VectorValue(0,0,0,0);
			UniformScalarExpressions[ScalarIndex]->GetNumberValue(MaterialRenderContext,VectorValue);
			TempScalarBuffer[ScalarIndex] = VectorValue.R;
		}

		UniformBuffer = RHICreateUniformBuffer(TempBuffer,UniformBufferStruct->GetSize(),UniformBuffer_MultiUse);
	}

	return UniformBuffer;
}

FMaterialUniformExpressionTexture::FMaterialUniformExpressionTexture() :
	TextureIndex(INDEX_NONE),
	TransientOverrideValue_GameThread(NULL),
	TransientOverrideValue_RenderThread(NULL)
{}

FMaterialUniformExpressionTexture::FMaterialUniformExpressionTexture(int32 InTextureIndex) :
	TextureIndex(InTextureIndex),
	TransientOverrideValue_GameThread(NULL),
	TransientOverrideValue_RenderThread(NULL)
{
}

void FMaterialUniformExpressionTexture::Serialize(FArchive& Ar)
{
	Ar << TextureIndex;
}

void FMaterialUniformExpressionTexture::SetTransientOverrideTextureValue( UTexture* InOverrideTexture )
{
	TransientOverrideValue_GameThread = InOverrideTexture;
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(SetTransientOverrideTextureValueCommand,
											   FMaterialUniformExpressionTexture*, ExpressionTexture, this,
											   UTexture*, InOverrideTexture, InOverrideTexture,
											   {
												   ExpressionTexture->TransientOverrideValue_RenderThread = InOverrideTexture;
											   });
}

void FMaterialUniformExpressionTexture::GetTextureValue(const FMaterialRenderContext& Context,const FMaterial& Material,const UTexture*& OutValue) const
{
	check(IsInRenderingThread());
	if( TransientOverrideValue_RenderThread != NULL )
	{
		OutValue = TransientOverrideValue_RenderThread;
	}
	else
	{
		OutValue = GetIndexedTexture(Material, TextureIndex);
	}
}

void FMaterialUniformExpressionTexture::GetGameThreadTextureValue(const UMaterialInterface* MaterialInterface,const FMaterial& Material,UTexture*& OutValue,bool bAllowOverride) const
{
	check(IsInGameThread());
	if (bAllowOverride && TransientOverrideValue_GameThread)
	{
		OutValue = TransientOverrideValue_GameThread;
	}
	else
	{
		OutValue = GetIndexedTexture(Material, TextureIndex);
	}
}

bool FMaterialUniformExpressionTexture::IsIdentical(const FMaterialUniformExpression* OtherExpression) const
{
	if (GetType() != OtherExpression->GetType())
	{
		return false;
	}
	FMaterialUniformExpressionTexture* OtherTextureExpression = (FMaterialUniformExpressionTexture*)OtherExpression;

	return TextureIndex == OtherTextureExpression->TextureIndex;
}

IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionTexture);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionConstant);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionTime);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionRealTime);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionVectorParameter);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionScalarParameter);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionTextureParameter);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionFlipBookTextureParameter);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionSine);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionSquareRoot);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionLength);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionFoldedMath);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionPeriodic);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionAppendVector);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionMin);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionMax);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionClamp);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionFloor);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionCeil);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionFrac);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionFmod);
IMPLEMENT_MATERIALUNIFORMEXPRESSION_TYPE(FMaterialUniformExpressionAbs);
