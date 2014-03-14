// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VertexFactory.cpp: Vertex factory implementation
=============================================================================*/

#include "ShaderCore.h"
#include "Shader.h"
#include "VertexFactory.h"

uint32 FVertexFactoryType::NextHashIndex = 0;
bool FVertexFactoryType::bInitializedSerializationHistory = false;

/**
 * @return The global shader factory list.
 */
TLinkedList<FVertexFactoryType*>*& FVertexFactoryType::GetTypeList()
{
	static TLinkedList<FVertexFactoryType*>* TypeList = NULL;
	return TypeList;
}

/**
 * Finds a FVertexFactoryType by name.
 */
FVertexFactoryType* FVertexFactoryType::GetVFByName(const FString& VFName)
{
	for(TLinkedList<FVertexFactoryType*>::TIterator It(GetTypeList()); It; It.Next())
	{
		FString CurrentVFName = FString(It->GetName());
		if (CurrentVFName == VFName)
		{
			return *It;
		}
	}
	return NULL;
}

void FVertexFactoryType::Initialize(const TMap<FString, TArray<const TCHAR*> >& ShaderFileToUniformBufferVariables)
{
	if (!FPlatformProperties::RequiresCookedData())
	{
		// Cache serialization history for each VF type
		// This history is used to detect when shader serialization changes without a corresponding .usf change
		for(TLinkedList<FVertexFactoryType*>::TIterator It(FVertexFactoryType::GetTypeList()); It; It.Next())
		{
			FVertexFactoryType* Type = *It;
			GenerateReferencedUniformBuffers(Type->ShaderFilename, Type->Name, ShaderFileToUniformBufferVariables, Type->ReferencedUniformBufferStructsCache);

			for (int32 Frequency = 0; Frequency < SF_NumFrequencies; Frequency++)
			{
				// Construct a temporary shader parameter instance, which is initialized to safe values for serialization
				FVertexFactoryShaderParameters* Parameters = Type->CreateShaderParameters((EShaderFrequency)Frequency);

				if (Parameters)
				{
					// Serialize the temp shader to memory and record the number and sizes of serializations
					TArray<uint8> TempData;
					FMemoryWriter Ar(TempData, true);
					FShaderSaveArchive SaveArchive(Ar, Type->SerializationHistory[Frequency]);
					Parameters->Serialize(SaveArchive);
					delete Parameters;
				}
			}
		}
	}

	bInitializedSerializationHistory = true;
}

void FVertexFactoryType::Uninitialize()
{
	for(TLinkedList<FVertexFactoryType*>::TIterator It(FVertexFactoryType::GetTypeList()); It; It.Next())
	{
		FVertexFactoryType* Type = *It;

		for (int32 Frequency = 0; Frequency < SF_NumFrequencies; Frequency++)
		{
			Type->SerializationHistory[Frequency] = FSerializationHistory();
		}
	}

	bInitializedSerializationHistory = false;
}

FVertexFactoryType::FVertexFactoryType(
	const TCHAR* InName,
	const TCHAR* InShaderFilename,
	bool bInUsedWithMaterials,
	bool bInSupportsStaticLighting,
	bool bInSupportsDynamicLighting,
	bool bInSupportsPrecisePrevWorldPos,
	bool bInSupportsPositionOnly,
	ConstructParametersType InConstructParameters,
	ShouldCacheType InShouldCache,
	ModifyCompilationEnvironmentType InModifyCompilationEnvironment,
	SupportsTessellationShadersType InSupportsTessellationShaders
	):
	Name(InName),
	ShaderFilename(InShaderFilename),
	TypeName(InName),
	bUsedWithMaterials(bInUsedWithMaterials),
	bSupportsStaticLighting(bInSupportsStaticLighting),
	bSupportsDynamicLighting(bInSupportsDynamicLighting),
	bSupportsPrecisePrevWorldPos(bInSupportsPrecisePrevWorldPos),
	bSupportsPositionOnly(bInSupportsPositionOnly),
	ConstructParameters(InConstructParameters),
	ShouldCacheRef(InShouldCache),
	ModifyCompilationEnvironmentRef(InModifyCompilationEnvironment),
	SupportsTessellationShadersRef(InSupportsTessellationShaders),
	GlobalListLink(this)
{
	for (int32 Platform = 0; Platform < SP_NumPlatforms; Platform++)
	{
		bCachedUniformBufferStructDeclarations[Platform] = false;
	}

	// This will trigger if an IMPLEMENT_VERTEX_FACTORY_TYPE was in a module not loaded before InitializeShaderTypes
	// Vertex factory types need to be implemented in modules that are loaded before that
	check(!bInitializedSerializationHistory);

	// Add this vertex factory type to the global list.
	GlobalListLink.Link(GetTypeList());

	// Assign the vertex factory type the next unassigned hash index.
	HashIndex = NextHashIndex++;
}

FVertexFactoryType::~FVertexFactoryType()
{
	GlobalListLink.Unlink();
}

/** Calculates a Hash based on this vertex factory type's source code and includes */
const FSHAHash& FVertexFactoryType::GetSourceHash() const
{
	return GetShaderFileHash(GetShaderFilename());
}

FArchive& operator<<(FArchive& Ar,FVertexFactoryType*& TypeRef)
{
	if(Ar.IsSaving())
	{
		FName TypeName = TypeRef ? FName(TypeRef->GetName()) : NAME_None;
		Ar << TypeName;
	}
	else if(Ar.IsLoading())
	{
		FName TypeName = NAME_None;
		Ar << TypeName;
		TypeRef = FindVertexFactoryType(TypeName);
	}
	return Ar;
}

FVertexFactoryType* FindVertexFactoryType(FName TypeName)
{
	// Search the global vertex factory list for a type with a matching name.
	for(TLinkedList<FVertexFactoryType*>::TIterator VertexFactoryTypeIt(FVertexFactoryType::GetTypeList());VertexFactoryTypeIt;VertexFactoryTypeIt.Next())
	{
		if(VertexFactoryTypeIt->GetFName() == TypeName)
		{
			return *VertexFactoryTypeIt;
		}
	}
	return NULL;
}

void FVertexFactory::Set() const
{
	check(IsInitialized());
	for(int32 StreamIndex = 0;StreamIndex < Streams.Num();StreamIndex++)
	{
		const FVertexStream& Stream = Streams[StreamIndex];
		checkf(Stream.VertexBuffer->IsInitialized(), TEXT("Vertex buffer was not initialized! Stream %u, Stride %u, Name %s"), StreamIndex, Stream.Stride, *Stream.VertexBuffer->GetFriendlyName());
		RHISetStreamSource(StreamIndex,Stream.VertexBuffer->VertexBufferRHI,Stream.Stride,Stream.Offset);
	}
}

void FVertexFactory::SetPositionStream() const
{
	check(IsInitialized());
	// Set the predefined vertex streams.
	for(int32 StreamIndex = 0;StreamIndex < PositionStream.Num();StreamIndex++)
	{
		const FVertexStream& Stream = PositionStream[StreamIndex];
		check(Stream.VertexBuffer->IsInitialized());
		RHISetStreamSource(StreamIndex,Stream.VertexBuffer->VertexBufferRHI,Stream.Stride,Stream.Offset);
	}
}

void FVertexFactory::ReleaseRHI()
{
	Declaration.SafeRelease();
	PositionDeclaration.SafeRelease();
	Streams.Empty();
	PositionStream.Empty();
}

/**
* Fill in array of strides from this factory's vertex streams without shadow/light maps
* @param OutStreamStrides - output array of # MaxVertexElementCount stream strides to fill
*/
int32 FVertexFactory::GetStreamStrides(uint32 *OutStreamStrides, bool bPadWithZeroes) const
{
	int32 StreamIndex;
	for(StreamIndex = 0;StreamIndex < Streams.Num();++StreamIndex)
	{
		OutStreamStrides[StreamIndex] = Streams[StreamIndex].Stride;
	}
	if (bPadWithZeroes)
	{
		// Pad stream strides with 0's to be safe (they can be used in hashes elsewhere)
		for (;StreamIndex < MaxVertexElementCount;++StreamIndex)
		{
			OutStreamStrides[StreamIndex] = 0;
		}
	}
	return StreamIndex;
}

/**
* Fill in array of strides from this factory's position only vertex streams
* @param OutStreamStrides - output array of # MaxVertexElementCount stream strides to fill
*/
void FVertexFactory::GetPositionStreamStride(uint32 *OutStreamStrides) const
{
	int32 StreamIndex;
	for(StreamIndex = 0;StreamIndex < PositionStream.Num();++StreamIndex)
	{
		OutStreamStrides[StreamIndex] = PositionStream[StreamIndex].Stride;
	}
	// Pad stream strides with 0's to be safe (they can be used in hashes elsewhere)
	for (;StreamIndex < MaxVertexElementCount;++StreamIndex)
	{
		OutStreamStrides[StreamIndex] = 0;
	}
}

FVertexElement FVertexFactory::AccessStreamComponent(const FVertexStreamComponent& Component,uint8 AttributeIndex)
{
	FVertexStream VertexStream;
	VertexStream.VertexBuffer = Component.VertexBuffer;
	VertexStream.Stride = Component.Stride;
	VertexStream.Offset = 0;

	return FVertexElement(Streams.AddUnique(VertexStream),Component.Offset,Component.Type,AttributeIndex,Component.bUseInstanceIndex);
}

FVertexElement FVertexFactory::AccessPositionStreamComponent(const FVertexStreamComponent& Component,uint8 AttributeIndex)
{
	FVertexStream VertexStream;
	VertexStream.VertexBuffer = Component.VertexBuffer;
	VertexStream.Stride = Component.Stride;
	VertexStream.Offset = 0;

	return FVertexElement(PositionStream.AddUnique(VertexStream),Component.Offset,Component.Type,AttributeIndex,Component.bUseInstanceIndex);
}

//TTP:51684
/**
 *	GetVertexElementSize returns the size of data based on its type
 *	it's needed for FixFXCardDeclarator function
 */
static uint8 GetVertexElementSize( uint8 Type )
{
	switch( Type )
	{
		case VET_Float1:
			return 4;
		case VET_Float2:
			return 8;
		case VET_Float3:
			return 12;
		case VET_Float4:
			return 16;
		case VET_PackedNormal:
		case VET_UByte4:
		case VET_UByte4N:
		case VET_Color:
		case VET_Short2:
		case VET_Short2N:
		case VET_Short4:
		case VET_Half2:
		default:
			return 0;
	}
}

/**
 * Patches the declaration so vertex stream offsets are unique. This is required for e.g. GeForce FX cards, which don't support redundant 
 * offsets in the declaration. We're unable to make that many vertex elements point to the same offset so the function moves redundant 
 * declarations to higher offsets, pointing to garbage data.
 */
static void PatchVertexStreamOffsetsToBeUnique( FVertexDeclarationElementList& Elements )
{
	// check every vertex element
	for ( int32 e = 0; e < Elements.Num(); e++ )
	{
		// check if there's an element that reads from the same offset
		for ( int32 i = 0; i < Elements.Num(); i++ )
		{
			// but only in the same stream and if it's not the same element
			if ( ( Elements[ i ].StreamIndex == Elements[ e ].StreamIndex ) && ( Elements[ i ].Offset == Elements[ e ].Offset ) && ( e != i ) )
			{
				// the id of the highest offset element is stored here (it doesn't need to be the last element in the declarator because the last element may belong to another StreamIndex
				uint32 MaxOffsetID = i;

				// find the highest offset element
				for ( int32 j = 0; j < Elements.Num(); j++ )
				{
					if ( ( Elements[ j ].StreamIndex == Elements[ e ].StreamIndex ) && ( Elements[ MaxOffsetID ].Offset < Elements[ j ].Offset ) )
					{
						MaxOffsetID = j;
					}
				}

				// get the size of the highest offset element, it's needed for the redundant element new offset
				uint8 PreviousElementSize = GetVertexElementSize( Elements[ MaxOffsetID ].Type );

				// prepare a new vertex element
				FVertexElement VertElement;
				VertElement.Offset		= Elements[ MaxOffsetID ].Offset + PreviousElementSize;
				VertElement.StreamIndex = Elements[ i ].StreamIndex;
				VertElement.Type		= Elements[ i ].Type;
				VertElement.AttributeIndex	= Elements[ i ].AttributeIndex;
				VertElement.bUseInstanceIndex = Elements[i].bUseInstanceIndex;

				// remove the old redundant element
				Elements.RemoveAt( i );

				// add a new element with "correct" offset
				Elements.Add( VertElement );

				// make sure that when the element has been removed its index is taken by the next element, so we must take care of it too
				i = i == 0 ? 0 : i - 1;
			}
		}
	}
}

void FVertexFactory::InitDeclaration(
	FVertexDeclarationElementList& Elements, 
	const DataType& InData)
{
	// Make a copy of the vertex factory data.
	Data = InData;

	// If GFFX detected, patch up the declarator
	if( !GVertexElementsCanShareStreamOffset )
	{
		PatchVertexStreamOffsetsToBeUnique( Elements );
	}

	// Create the vertex declaration for rendering the factory normally.
	Declaration = RHICreateVertexDeclaration(Elements);
}

void FVertexFactory::InitPositionDeclaration(const FVertexDeclarationElementList& Elements)
{
	// Create the vertex declaration for rendering the factory normally.
	PositionDeclaration = RHICreateVertexDeclaration(Elements);
}

FVertexFactoryParameterRef::FVertexFactoryParameterRef(FVertexFactoryType* InVertexFactoryType,const FShaderParameterMap& ParameterMap, EShaderFrequency InShaderFrequency)
: Parameters(NULL)
, VertexFactoryType(InVertexFactoryType)
, ShaderFrequency(InShaderFrequency)
{
	Parameters = VertexFactoryType->CreateShaderParameters(InShaderFrequency);
	VFHash = GetShaderFileHash(VertexFactoryType->GetShaderFilename());

	if(Parameters)
	{
		Parameters->Bind(ParameterMap);
	}
}

bool operator<<(FArchive& Ar,FVertexFactoryParameterRef& Ref)
{
	bool bShaderHasOutdatedParameters = false;

	Ar << Ref.VertexFactoryType;

	uint8 ShaderFrequencyByte = Ref.ShaderFrequency;
	Ar << ShaderFrequencyByte;
	if(Ar.IsLoading())
	{
		Ref.ShaderFrequency = (EShaderFrequency)ShaderFrequencyByte;
	}

	Ar << Ref.VFHash;


	if (Ar.IsLoading())
	{
		delete Ref.Parameters;
		if (Ref.VertexFactoryType)
		{
			Ref.Parameters = Ref.VertexFactoryType->CreateShaderParameters(Ref.ShaderFrequency);
		}
		else
		{
			bShaderHasOutdatedParameters = true;
			Ref.Parameters = NULL;
		}
	}

	// Need to be able to skip over parameters for no longer existing vertex factories.
	int32 SkipOffset = Ar.Tell();
	// Write placeholder.
	Ar << SkipOffset;

	if(Ref.Parameters)
	{
		Ref.Parameters->Serialize(Ar);
	}
	else if(Ar.IsLoading())
	{
		Ar.Seek( SkipOffset );
	}

	if( Ar.IsSaving() )
	{
		int32 EndOffset = Ar.Tell();
		Ar.Seek( SkipOffset );
		Ar << EndOffset;
		Ar.Seek( EndOffset );
	}

	return bShaderHasOutdatedParameters;
}

/** Returns the hash of the vertex factory shader file that this shader was compiled with. */
const FSHAHash& FVertexFactoryParameterRef::GetHash() const 
{ 
	return VFHash;
}
