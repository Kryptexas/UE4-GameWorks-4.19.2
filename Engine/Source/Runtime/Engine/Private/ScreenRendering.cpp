// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ScreenRendering.cpp: D3D render target implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScreenRendering.h"

/** Vertex declaration for screen-space rendering. */
TGlobalResource<FScreenVertexDeclaration> GScreenVertexDeclaration;

// Shader implementations.
IMPLEMENT_SHADER_TYPE(,FScreenPS,TEXT("ScreenPixelShader"),TEXT("Main"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(,FScreenVS,TEXT("ScreenVertexShader"),TEXT("Main"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(,FScreenVSForGS,TEXT("ScreenVertexShader"),TEXT("MainForGS"),SF_Vertex);

FGlobalBoundShaderState ScreenBoundShaderState;

/**
 * Draws a texture rectangle on the screen using normalized (-1 to 1) device coordinates.
 */
void DrawScreenQuad( float X0, float Y0, float U0, float V0, float X1, float Y1, float U1, float V1, const FTexture* Texture )
{ 
	// Set the screen shaders
	TShaderMapRef<FScreenVS> ScreenVertexShader(GetGlobalShaderMap());
	TShaderMapRef<FScreenPS> ScreenPixelShader(GetGlobalShaderMap());
	SetGlobalBoundShaderState(ScreenBoundShaderState, GScreenVertexDeclaration.VertexDeclarationRHI, *ScreenVertexShader, *ScreenPixelShader);

	ScreenPixelShader->SetParameters(Texture);

	// Generate the vertices used
	FScreenVertex Vertices[4];

	Vertices[0].Position.X = X1;
	Vertices[0].Position.Y = Y0;
	Vertices[0].UV.X       = U1;
	Vertices[0].UV.Y       = V0;

	Vertices[1].Position.X = X1;
	Vertices[1].Position.Y = Y1;
	Vertices[1].UV.X       = U1;
	Vertices[1].UV.Y       = V1;

	Vertices[2].Position.X = X0;
	Vertices[2].Position.Y = Y0;
	Vertices[2].UV.X       = U0;
	Vertices[2].UV.Y       = V0;

	Vertices[3].Position.X = X0;
	Vertices[3].Position.Y = Y1;
	Vertices[3].UV.X       = U0;
	Vertices[3].UV.Y       = V1;

	RHIDrawPrimitiveUP(PT_TriangleStrip,2,Vertices,sizeof(Vertices[0]));
}

void DrawNormalizedScreenQuad( float X, float Y, float U0, float V0, float SizeX, float SizeY, float U1, float V1, FIntPoint TargetSize, FTexture2DRHIRef TextureRHI )
{
	// Set the screen shaders 
	TShaderMapRef<FScreenVS> ScreenVertexShader(GetGlobalShaderMap());
	TShaderMapRef<FScreenPS> ScreenPixelShader(GetGlobalShaderMap());
	SetGlobalBoundShaderState(ScreenBoundShaderState, GScreenVertexDeclaration.VertexDeclarationRHI, *ScreenVertexShader, *ScreenPixelShader);

	ScreenPixelShader->SetParameters(TStaticSamplerState<SF_Bilinear>::GetRHI(), TextureRHI);

	// Generate the vertices used
	FScreenVertex Vertices[4];

	Vertices[0].Position.X = X + SizeX;
	Vertices[0].Position.Y = Y;
	Vertices[0].UV.X       = U1;
	Vertices[0].UV.Y       = V0;

	Vertices[1].Position.X = X + SizeX;
	Vertices[1].Position.Y = Y + SizeY;
	Vertices[1].UV.X       = U1;
	Vertices[1].UV.Y       = V1;

	Vertices[2].Position.X = X;
	Vertices[2].Position.Y = Y;
	Vertices[2].UV.X       = U0;
	Vertices[2].UV.Y       = V0;

	Vertices[3].Position.X = X;
	Vertices[3].Position.Y = Y + SizeY;
	Vertices[3].UV.X       = U0;
	Vertices[3].UV.Y       = V1;
	
	for(int32 VertexIndex = 0;VertexIndex < 4;VertexIndex++)
	{
		Vertices[VertexIndex].Position.X = -1.0f + 2.0f * (Vertices[VertexIndex].Position.X) / (float)TargetSize.X;
		Vertices[VertexIndex].Position.Y = (+1.0f - 2.0f * (Vertices[VertexIndex].Position.Y) / (float)TargetSize.Y);
	}

	RHIDrawPrimitiveUP(PT_TriangleStrip,2,Vertices,sizeof(Vertices[0]));
}
