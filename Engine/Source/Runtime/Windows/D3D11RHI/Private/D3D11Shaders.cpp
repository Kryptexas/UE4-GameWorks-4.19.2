// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	D3D11Shaders.cpp: D3D shader RHI implementation.
=============================================================================*/

#include "D3D11RHIPrivate.h"

FVertexShaderRHIRef FD3D11DynamicRHI::RHICreateVertexShader(const TArray<uint8>& Code)
{
	check(Code.Num());
	TRefCountPtr<ID3D11VertexShader> D3DShader;
	VERIFYD3D11RESULT(Direct3DDevice->CreateVertexShader((void*)Code.GetTypedData(),Code.Num() - 1,NULL,D3DShader.GetInitReference()));
	return new FD3D11VertexShader(D3DShader,Code);
}

FPixelShaderRHIRef FD3D11DynamicRHI::RHICreatePixelShader(const TArray<uint8>& Code)
{
	check(Code.Num());
	TRefCountPtr<ID3D11PixelShader> D3DShader;
	// bGlobalUniformBufferUsed is in the last byte, see CompileD3D11Shader
	VERIFYD3D11RESULT(Direct3DDevice->CreatePixelShader((void*)Code.GetTypedData(),Code.Num() - 1,NULL,(ID3D11PixelShader**)D3DShader.GetInitReference()));
	return new FD3D11PixelShader(D3DShader, Code);
}

FHullShaderRHIRef FD3D11DynamicRHI::RHICreateHullShader(const TArray<uint8>& Code) 
{ 
	check(Code.Num());
	TRefCountPtr<ID3D11HullShader> D3DShader;
	// bGlobalUniformBufferUsed is in the last byte, see CompileD3D11Shader
	VERIFYD3D11RESULT(Direct3DDevice->CreateHullShader((void*)Code.GetTypedData(),Code.Num() - 1,NULL,(ID3D11HullShader**)D3DShader.GetInitReference()));
	return new FD3D11HullShader(D3DShader, Code);
}

FDomainShaderRHIRef FD3D11DynamicRHI::RHICreateDomainShader(const TArray<uint8>& Code) 
{ 
	check(Code.Num());
	TRefCountPtr<ID3D11DomainShader> D3DShader;
	// bGlobalUniformBufferUsed is in the last byte, see CompileD3D11Shader
	VERIFYD3D11RESULT(Direct3DDevice->CreateDomainShader((void*)Code.GetTypedData(),Code.Num() - 1,NULL,(ID3D11DomainShader**)D3DShader.GetInitReference()));
	return new FD3D11DomainShader(D3DShader, Code);
}

FGeometryShaderRHIRef FD3D11DynamicRHI::RHICreateGeometryShader(const TArray<uint8>& Code) 
{ 
	check(Code.Num());
	TRefCountPtr<ID3D11GeometryShader> D3DShader;
	// bGlobalUniformBufferUsed is in the last byte, see CompileD3D11Shader
	VERIFYD3D11RESULT(Direct3DDevice->CreateGeometryShader((void*)Code.GetTypedData(),Code.Num() - 1,NULL,(ID3D11GeometryShader**)D3DShader.GetInitReference()));
	return new FD3D11GeometryShader(D3DShader, Code);
}

FGeometryShaderRHIRef FD3D11DynamicRHI::RHICreateGeometryShaderWithStreamOutput(const TArray<uint8>& Code, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream) 
{ 
	check(Code.Num());

	uint32 D3DRasterizedStream = RasterizedStream;
	if (RasterizedStream == -1)
	{
		D3DRasterizedStream = D3D11_SO_NO_RASTERIZED_STREAM;
	}

	D3D11_SO_DECLARATION_ENTRY StreamOutEntries[D3D11_SO_STREAM_COUNT * D3D11_SO_OUTPUT_COMPONENT_COUNT];

	for (int32 EntryIndex = 0; EntryIndex < ElementList.Num(); EntryIndex++)
	{
		StreamOutEntries[EntryIndex].Stream = ElementList[EntryIndex].Stream;
		StreamOutEntries[EntryIndex].SemanticName = ElementList[EntryIndex].SemanticName;
		StreamOutEntries[EntryIndex].SemanticIndex = ElementList[EntryIndex].SemanticIndex;
		StreamOutEntries[EntryIndex].StartComponent = ElementList[EntryIndex].StartComponent;
		StreamOutEntries[EntryIndex].ComponentCount = ElementList[EntryIndex].ComponentCount;
		StreamOutEntries[EntryIndex].OutputSlot = ElementList[EntryIndex].OutputSlot;
	}

	TRefCountPtr<ID3D11GeometryShader> D3DShader;
	VERIFYD3D11RESULT(Direct3DDevice->CreateGeometryShaderWithStreamOutput(
		(const void*)Code.GetTypedData(),
		// bGlobalUniformBufferUsed is in the last byte, see CompileD3D11Shader
		Code.Num() - 1,
		StreamOutEntries,
		ElementList.Num(),
		Strides,
		NumStrides,
		D3DRasterizedStream,
		NULL,
		(ID3D11GeometryShader**)D3DShader.GetInitReference()));

	return new FD3D11GeometryShader(D3DShader, Code);
}

FComputeShaderRHIRef FD3D11DynamicRHI::RHICreateComputeShader(const TArray<uint8>& Code) 
{ 
	check(Code.Num());
	TRefCountPtr<ID3D11ComputeShader> D3DShader;
	// bGlobalUniformBufferUsed is in the last byte, see CompileD3D11Shader
	VERIFYD3D11RESULT(Direct3DDevice->CreateComputeShader((void*)Code.GetTypedData(),Code.Num() - 1,NULL,(ID3D11ComputeShader**)D3DShader.GetInitReference()));
	return new FD3D11ComputeShader(D3DShader, Code);
}

void FD3D11DynamicRHI::RHISetMultipleViewports(uint32 Count, const FViewportBounds* Data) 
{ 
	check(Count > 0);
	check(Data);

	// structures are chosen to be directly mappable
	D3D11_VIEWPORT* D3DData = (D3D11_VIEWPORT*)Data;

	StateCache.SetViewports(Count, D3DData );

}

FD3D11BoundShaderState::FD3D11BoundShaderState(
	FVertexDeclarationRHIParamRef InVertexDeclarationRHI,
	FVertexShaderRHIParamRef InVertexShaderRHI,
	FPixelShaderRHIParamRef InPixelShaderRHI,
	FHullShaderRHIParamRef InHullShaderRHI,
	FDomainShaderRHIParamRef InDomainShaderRHI,
	FGeometryShaderRHIParamRef InGeometryShaderRHI,
	ID3D11Device* Direct3DDevice
	):
	CacheLink(InVertexDeclarationRHI,InVertexShaderRHI,InPixelShaderRHI,InHullShaderRHI,InDomainShaderRHI,InGeometryShaderRHI,this)
{
	DYNAMIC_CAST_D3D11RESOURCE(VertexDeclaration,InVertexDeclaration);
	DYNAMIC_CAST_D3D11RESOURCE(VertexShader,InVertexShader);
	DYNAMIC_CAST_D3D11RESOURCE(PixelShader,InPixelShader);
	DYNAMIC_CAST_D3D11RESOURCE(HullShader,InHullShader);
	DYNAMIC_CAST_D3D11RESOURCE(DomainShader,InDomainShader);
	DYNAMIC_CAST_D3D11RESOURCE(GeometryShader,InGeometryShader);

	// Create an input layout for this combination of vertex declaration and vertex shader.
	D3D11_INPUT_ELEMENT_DESC NullInputElement;
	FMemory::Memzero(&NullInputElement,sizeof(D3D11_INPUT_ELEMENT_DESC));

	VERIFYD3D11RESULT(Direct3DDevice->CreateInputLayout(
		InVertexDeclaration ? InVertexDeclaration->VertexElements.GetTypedData() : &NullInputElement,
		InVertexDeclaration ? InVertexDeclaration->VertexElements.Num() : 0,
		InVertexShader->Code.GetTypedData(),
		InVertexShader->Code.Num() - 1,
		InputLayout.GetInitReference()
		));

	VertexShader = InVertexShader->Resource;
	PixelShader = InPixelShader ? InPixelShader->Resource : NULL;
	HullShader = InHullShader ? InHullShader->Resource : NULL;
	DomainShader = InDomainShader ? InDomainShader->Resource : NULL;
	GeometryShader = InGeometryShader ? InGeometryShader->Resource : NULL;

	FMemory::Memzero(&bShaderNeedsGlobalConstantBuffer,sizeof(bShaderNeedsGlobalConstantBuffer));

	bShaderNeedsGlobalConstantBuffer[SF_Vertex] = InVertexShader->bShaderNeedsGlobalConstantBuffer;
	bShaderNeedsGlobalConstantBuffer[SF_Hull] = InHullShader ? InHullShader->bShaderNeedsGlobalConstantBuffer : false;
	bShaderNeedsGlobalConstantBuffer[SF_Domain] = InDomainShader ? InDomainShader->bShaderNeedsGlobalConstantBuffer : false;
	bShaderNeedsGlobalConstantBuffer[SF_Pixel] = InPixelShader ? InPixelShader->bShaderNeedsGlobalConstantBuffer : false;
	bShaderNeedsGlobalConstantBuffer[SF_Geometry] = InGeometryShader ? InGeometryShader->bShaderNeedsGlobalConstantBuffer : false;

	checkAtCompileTime(ARRAY_COUNT(bShaderNeedsGlobalConstantBuffer) == SF_NumFrequencies, NumFrequenciesMismatch);
}

/**
* Creates a bound shader state instance which encapsulates a decl, vertex shader, and pixel shader
* @param VertexDeclaration - existing vertex decl
* @param StreamStrides - optional stream strides
* @param VertexShader - existing vertex shader
* @param HullShader - existing hull shader
* @param DomainShader - existing domain shader
* @param PixelShader - existing pixel shader
* @param GeometryShader - existing geometry shader
*/
FBoundShaderStateRHIRef FD3D11DynamicRHI::RHICreateBoundShaderState(
	FVertexDeclarationRHIParamRef VertexDeclarationRHI, 
	FVertexShaderRHIParamRef VertexShaderRHI, 
	FHullShaderRHIParamRef HullShaderRHI, 
	FDomainShaderRHIParamRef DomainShaderRHI, 
	FPixelShaderRHIParamRef PixelShaderRHI,
	FGeometryShaderRHIParamRef GeometryShaderRHI
	)
{
	SCOPE_CYCLE_COUNTER(STAT_D3D11CreateBoundShaderStateTime);

	checkf(GIsRHIInitialized && Direct3DDeviceIMContext,(TEXT("Bound shader state RHI resource was created without initializing Direct3D first")));

	// Check for an existing bound shader state which matches the parameters
	FCachedBoundShaderStateLink* CachedBoundShaderStateLink = GetCachedBoundShaderState(
		VertexDeclarationRHI,
		VertexShaderRHI,
		PixelShaderRHI,
		HullShaderRHI,
		DomainShaderRHI,
		GeometryShaderRHI
		);
	if(CachedBoundShaderStateLink)
	{
		// If we've already created a bound shader state with these parameters, reuse it.
		return CachedBoundShaderStateLink->BoundShaderState;
	}
	else
	{
		SCOPE_CYCLE_COUNTER(STAT_D3D11NewBoundShaderStateTime);
		return new FD3D11BoundShaderState(VertexDeclarationRHI,VertexShaderRHI,PixelShaderRHI,HullShaderRHI,DomainShaderRHI,GeometryShaderRHI,Direct3DDevice);
	}
}
