// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SceneFilterRendering.cpp: Filter rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessWeightedSampleSum.h"

/** Vertex declaration for the light function fullscreen 2D quad. */
TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;

static void DoDrawRectangleFlagOverride(EDrawRectangleFlags& Flags)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// Determine triangle draw mode
	static auto* TriangleModeCVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.DrawRectangleOptimization"));
	int Value = TriangleModeCVar->GetValueOnRenderThread();

	if(!Value)
	{
		// don't use triangle optimization
		Flags = EDRF_Default;
	}
#endif
}


void DrawRectangle(
	float X,
	float Y,
	float SizeX,
	float SizeY,
	float U,
	float V,
	float SizeU,
	float SizeV,
	FIntPoint TargetSize,
	FIntPoint TextureSize,
	EDrawRectangleFlags Flags
	)
{
	float ClipSpaceQuadZ = 0.0f;

	DoDrawRectangleFlagOverride(Flags);

	// triangle if extending to left and top of the given rectangle, if it's not left top of the viewport it can cause artifacts
	if(X > 0.0f || Y > 0.0f)
	{
		// don't use triangle optimization
		Flags = EDRF_Default;
	}

	if (Flags == EDRF_UseTriangleOptimization)
	{
		// A single triangle spans the entire viewport this results in a quad that fill the viewport. This can increase rasterization efficiency
		// as we do not have a diagonal edge (through the center) for the rasterizer/span-dispatch. Although the actual benefit of this technique is dependent upon hardware.
		FFilterVertex Vertices[3];

		Vertices[0].Position = FVector4(X + SizeX,	Y - SizeY, ClipSpaceQuadZ,	1);
		Vertices[1].Position = FVector4(X + SizeX,  Y + SizeY,	ClipSpaceQuadZ,	1);
		Vertices[2].Position = FVector4(X - SizeX,  Y + SizeY ,	ClipSpaceQuadZ,	1);

		Vertices[0].UV = FVector2D(U + SizeU, V - SizeV);
		Vertices[1].UV = FVector2D(U + SizeU, V + SizeV);
		Vertices[2].UV = FVector2D(U - SizeU, V + SizeV);

		for (int32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
		{
			Vertices[VertexIndex].Position.X = -1.0f + 2.0f * (Vertices[VertexIndex].Position.X - GPixelCenterOffset) / (float)TargetSize.X;
			Vertices[VertexIndex].Position.Y = (+1.0f - 2.0f * (Vertices[VertexIndex].Position.Y - GPixelCenterOffset) / (float)TargetSize.Y) * GProjectionSignY;

			Vertices[VertexIndex].UV.X = Vertices[VertexIndex].UV.X / (float)TextureSize.X;
			Vertices[VertexIndex].UV.Y = Vertices[VertexIndex].UV.Y / (float)TextureSize.Y;
		}
		
		static const uint16 Indices[] = { 0, 1, 2 };

		RHIDrawIndexedPrimitiveUP(PT_TriangleList, 0, 3, 1, Indices, sizeof(Indices[0]), Vertices, sizeof(Vertices[0]));
	}
	else
	{
		FFilterVertex Vertices[4];

		Vertices[0].Position = FVector4(X,			Y,			ClipSpaceQuadZ,	1);
		Vertices[1].Position = FVector4(X + SizeX,	Y,			ClipSpaceQuadZ,	1);
		Vertices[2].Position = FVector4(X,			Y + SizeY,	ClipSpaceQuadZ,	1);
		Vertices[3].Position = FVector4(X + SizeX,	Y + SizeY,	ClipSpaceQuadZ,	1);

		Vertices[0].UV = FVector2D(U,			V);
		Vertices[1].UV = FVector2D(U + SizeU,	V);
		Vertices[2].UV = FVector2D(U,			V + SizeV);
		Vertices[3].UV = FVector2D(U + SizeU,	V + SizeV);

		for (int32 VertexIndex = 0; VertexIndex < 4; VertexIndex++)
		{
			Vertices[VertexIndex].Position.X = -1.0f + 2.0f * (Vertices[VertexIndex].Position.X - GPixelCenterOffset) / (float)TargetSize.X;
			Vertices[VertexIndex].Position.Y = (+1.0f - 2.0f * (Vertices[VertexIndex].Position.Y - GPixelCenterOffset) / (float)TargetSize.Y) * GProjectionSignY;

			Vertices[VertexIndex].UV.X = Vertices[VertexIndex].UV.X / (float)TextureSize.X;
			Vertices[VertexIndex].UV.Y = Vertices[VertexIndex].UV.Y / (float)TextureSize.Y;
		}

		static const uint16 Indices[] = { 0, 1, 3, 0, 3, 2 };

		RHIDrawIndexedPrimitiveUP(PT_TriangleList, 0, 4, 2, Indices, sizeof(Indices[0]), Vertices, sizeof(Vertices[0]));
	}
}

void DrawTransformedRectangle(
    float X,
    float Y,
    float SizeX,
    float SizeY,
    const FMatrix& PosTransform,
    float U,
    float V,
    float SizeU,
    float SizeV,
    const FMatrix& TexTransform,
    FIntPoint TargetSize,
    FIntPoint TextureSize
    )
{
	float ClipSpaceQuadZ = 0.0f;

	// we don't do the triangle optimization as this case is rare for the DrawTransformedRectangle case

	FFilterVertex Vertices[4];

	Vertices[0].Position = PosTransform.TransformFVector4(FVector4(X,			Y,			ClipSpaceQuadZ,	1));
	Vertices[1].Position = PosTransform.TransformFVector4(FVector4(X + SizeX,	Y,			ClipSpaceQuadZ,	1));
	Vertices[2].Position = PosTransform.TransformFVector4(FVector4(X,			Y + SizeY,	ClipSpaceQuadZ,	1));
	Vertices[3].Position = PosTransform.TransformFVector4(FVector4(X + SizeX,	Y + SizeY,	ClipSpaceQuadZ,	1));

	Vertices[0].UV = FVector2D(TexTransform.TransformFVector4(FVector(U,			V,         0)));
	Vertices[1].UV = FVector2D(TexTransform.TransformFVector4(FVector(U + SizeU,	V,         0)));
	Vertices[2].UV = FVector2D(TexTransform.TransformFVector4(FVector(U,			V + SizeV, 0)));
	Vertices[3].UV = FVector2D(TexTransform.TransformFVector4(FVector(U + SizeU,	V + SizeV, 0)));

	for (int32 VertexIndex = 0; VertexIndex < 4; VertexIndex++)
	{
		Vertices[VertexIndex].Position.X = -1.0f + 2.0f * (Vertices[VertexIndex].Position.X - GPixelCenterOffset) / (float)TargetSize.X;
		Vertices[VertexIndex].Position.Y = (+1.0f - 2.0f * (Vertices[VertexIndex].Position.Y - GPixelCenterOffset) / (float)TargetSize.Y) * GProjectionSignY;

		Vertices[VertexIndex].UV.X = Vertices[VertexIndex].UV.X / (float)TextureSize.X;
		Vertices[VertexIndex].UV.Y = Vertices[VertexIndex].UV.Y / (float)TextureSize.Y;
	}

	static const uint16 Indices[] =	{ 0, 1, 3, 0, 3, 2 };

	RHIDrawIndexedPrimitiveUP(PT_TriangleList, 0, 4, 2, Indices, sizeof(Indices[0]), Vertices, sizeof(Vertices[0]));
}
