// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalVertexDeclaration.cpp: Metal vertex declaration RHI implementation.
=============================================================================*/

#include "MetalRHIPrivate.h"

static MTLVertexFormat TranslateElementTypeToMTLType(EVertexElementType Type)
{
	switch (Type)
	{
		case VET_Float1:		return MTLVertexFormatFloat;
		case VET_Float2:		return MTLVertexFormatFloat2;
		case VET_Float3:		return MTLVertexFormatFloat3;
		case VET_Float4:		return MTLVertexFormatFloat4;
		case VET_PackedNormal:	return MTLVertexFormatUChar4Normalized;
		case VET_UByte4:		return MTLVertexFormatUChar4;
		case VET_UByte4N:		return MTLVertexFormatUChar4Normalized;
		case VET_Color:			return MTLVertexFormatUChar4Normalized;
		case VET_Short2:		return MTLVertexFormatShort2;
		case VET_Short4:		return MTLVertexFormatShort4;
		case VET_Short2N:		return MTLVertexFormatShort2Normalized;
		case VET_Half2:			return MTLVertexFormatHalf2;
		case VET_Half4:			return MTLVertexFormatHalf4;
		default:				UE_LOG(LogMetal, Fatal, TEXT("Unknown vertex element type!")); return MTLVertexFormatFloat;
	};

}

static uint32 TranslateElementTypeToSize(EVertexElementType Type)
{
	switch (Type)
	{
		case VET_Float1:		return 4;
		case VET_Float2:		return 8;
		case VET_Float3:		return 12;
		case VET_Float4:		return 16;
		case VET_PackedNormal:	return 4;
		case VET_UByte4:		return 4;
		case VET_UByte4N:		return 4;
		case VET_Color:			return 4;
		case VET_Short2:		return 4;
		case VET_Short4:		return 8;
		case VET_Short2N:		return 8;
		case VET_Half2:			return 4;
		case VET_Half4:			return 8;
		default:				UE_LOG(LogMetal, Fatal, TEXT("Unknown vertex element type!")); return 0;
	};
}

FMetalVertexDeclaration::FMetalVertexDeclaration(const FVertexDeclarationElementList& InElements)
	: Elements(InElements)
	, Layout(nil)
{
	GenerateLayout(InElements);
}

FMetalVertexDeclaration::~FMetalVertexDeclaration()
{
	FMetalManager::ReleaseObject(Layout);
}

static TMap<uint32, FVertexDeclarationRHIRef> GVertexDeclarationCache;

FVertexDeclarationRHIRef FMetalDynamicRHI::RHICreateVertexDeclaration(const FVertexDeclarationElementList& Elements)
{
	uint32 Key = FCrc::MemCrc32(Elements.GetData(), Elements.Num() * sizeof(FVertexElement));
	// look up an existing declaration
	FVertexDeclarationRHIRef* VertexDeclarationRefPtr = GVertexDeclarationCache.Find(Key);
	if (VertexDeclarationRefPtr == NULL)
	{
//		NSLog(@"VertDecl Key: %x", Key);

		// create and add to the cache if it doesn't exist.
		VertexDeclarationRefPtr = &GVertexDeclarationCache.Add(Key, new FMetalVertexDeclaration(Elements));
	}

	return *VertexDeclarationRefPtr;
}


void FMetalVertexDeclaration::GenerateLayout(const FVertexDeclarationElementList& Elements)
{
	Layout = [[MTLVertexDescriptor alloc] init];
	TRACK_OBJECT(Layout);

	TMap<uint32, uint32> BufferStrides;
	for (uint32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
	{
		const FVertexElement& Element = Elements[ElementIndex];
		
		// @todo urban: for zero stride elements, assume a repeated color
		// @todo urban: Make sure zero stride actually works now!! If so, then we can remove the hack zero stride buffer
		uint32 Stride = Element.Stride;// ? Element.Stride : 4;

		checkf(Element.Offset + TranslateElementTypeToSize(Element.Type) <= Stride, TEXT("Stream component is bigger than stride: Offset: %d, Size: %d [Type %d], Stride: %d"), Element.Offset, TranslateElementTypeToSize(Element.Type), (uint32)Element.Type, Stride);

		// we offset 6 buffers to leave space for uniform buffers
		uint32 ShaderBufferIndex = UNREAL_TO_METAL_BUFFER_INDEX(Element.StreamIndex);

		// track the buffer stride, making sure all elements with the same buffer have the same stride
		uint32* ExistingStride = BufferStrides.Find(ShaderBufferIndex);
		if (ExistingStride == NULL)
		{
			BufferStrides.Add(ShaderBufferIndex, Stride);
		}
		else
		{
			check(Stride == *ExistingStride);
		}

		[Layout setVertexFormat:TranslateElementTypeToMTLType(Element.Type) offset:Element.Offset vertexBufferIndex:ShaderBufferIndex atAttributeIndex:Element.AttributeIndex];
	}

	for (auto It = BufferStrides.CreateIterator(); It; ++It)
	{
		uint32 Stride = It.Value();
		if (Stride == 0xFFFF)
		{
			NSLog(@"Setting illegal stride - break here if you want to find out why, but this won't break until we try to render with it");
			Stride = 200;// 16;
		}
		[Layout setStride:Stride stepFunction:(Stride == 0 ? MTLVertexStepFunctionConstant : MTLVertexStepFunctionPerVertex) stepRate:0 atVertexBufferIndex:It.Key()];
	}
}
