// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "SceneManagement.h"

/** Emits draw events for a given FMeshBatch and the FPrimitiveSceneProxy corresponding to that mesh element. */
void EmitMeshDrawEvents(const FPrimitiveSceneProxy* PrimitiveSceneProxy, const FMeshBatch& Mesh)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	extern bool GShowMaterialDrawEvents;
	if ( GShowMaterialDrawEvents )
	{
		// Only show material name at the top level
		// Note: this is the parent's material name, not the material instance
		SCOPED_DRAW_EVENTF(MaterialEvent, DEC_SCENE_ITEMS, *Mesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->GetFriendlyName());
		if (PrimitiveSceneProxy)
		{
			// Show Actor, level and resource name inside the material name
			// These are separate draw events since some platforms only allow 32 character event names (xenon)
			{
				SCOPED_CONDITIONAL_DRAW_EVENTF(LevelEvent, PrimitiveSceneProxy->GetLevelName() != NAME_None, DEC_SCENE_ITEMS, PrimitiveSceneProxy->GetLevelName().IsValid() ? *PrimitiveSceneProxy->GetLevelName().ToString() : TEXT(""));
			}
			{
				SCOPED_CONDITIONAL_DRAW_EVENTF(OwnerEvent,PrimitiveSceneProxy->GetOwnerName() != NAME_None, DEC_SCENE_ITEMS, *PrimitiveSceneProxy->GetOwnerName().ToString());
			}
			SCOPED_CONDITIONAL_DRAW_EVENTF(ResourceEvent,PrimitiveSceneProxy->GetResourceName() != NAME_None, DEC_SCENE_ITEMS, PrimitiveSceneProxy->GetResourceName().IsValid() ? *PrimitiveSceneProxy->GetResourceName().ToString() : TEXT(""));
		}
	}
#endif
}

void DrawPlane10x10(class FPrimitiveDrawInterface* PDI,const FMatrix& ObjectToWorld, float Radii, FVector2D UVMin, FVector2D UVMax, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriorityGroup)
{
	// -> TileCount * TileCount * 2 triangles
	const uint32 TileCount = 10;

	FDynamicMeshBuilder MeshBuilder;

	// todo: reserve or better cache the mesh

	float Step = 2.0f / TileCount;

	for(int32 y = 0; y < TileCount; ++y)
	{
		// implemented this way to avoid cracks, could be optimized
		float y0 = y * Step - 1.0f;
		float y1 = (y + 1) * Step - 1.0f;

		float V0 = FMath::Lerp(UVMin.Y, UVMax.Y, y0 * 0.5f + 0.5f);
		float V1 = FMath::Lerp(UVMin.Y, UVMax.Y, y1 * 0.5f + 0.5f);

		for(int32 x = 0; x < TileCount; ++x)
		{
			// implemented this way to avoid cracks, could be optimized
			float x0 = x * Step - 1.0f;
			float x1 = (x + 1) * Step - 1.0f;

			float U0 = FMath::Lerp(UVMin.X, UVMax.X, x0 * 0.5f + 0.5f);
			float U1 = FMath::Lerp(UVMin.X, UVMax.X, x1 * 0.5f + 0.5f);

			// Calculate verts for a face pointing down Z
			MeshBuilder.AddVertex(FVector(x0, y0, 0), FVector2D(U0, V0), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White);
			MeshBuilder.AddVertex(FVector(x0, y1, 0), FVector2D(U0, V1), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White);
			MeshBuilder.AddVertex(FVector(x1, y1, 0), FVector2D(U1, V1), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White);
			MeshBuilder.AddVertex(FVector(x1, y0, 0), FVector2D(U1, V0), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White);

			int Index = (x + y * TileCount) * 4;
			MeshBuilder.AddTriangle(Index + 0, Index + 1, Index + 2);
			MeshBuilder.AddTriangle(Index + 0, Index + 2, Index + 3);
		}
	}	

	MeshBuilder.Draw(PDI, FScaleMatrix(Radii) * ObjectToWorld, MaterialRenderProxy, DepthPriorityGroup, 0.f);
}

void DrawBox(FPrimitiveDrawInterface* PDI,const FMatrix& BoxToWorld,const FVector& Radii,const FMaterialRenderProxy* MaterialRenderProxy,uint8 DepthPriorityGroup)
{
	// Calculate verts for a face pointing down Z
	FVector Positions[4] =
	{
		FVector(-1, -1, +1),
		FVector(-1, +1, +1),
		FVector(+1, +1, +1),
		FVector(+1, -1, +1)
	};
	FVector2D UVs[4] =
	{
		FVector2D(0,0),
		FVector2D(0,1),
		FVector2D(1,1),
		FVector2D(1,0),
	};

	// Then rotate this face 6 times
	FRotator FaceRotations[6];
	FaceRotations[0] = FRotator(0,		0,	0);
	FaceRotations[1] = FRotator(90.f,	0,	0);
	FaceRotations[2] = FRotator(-90.f,	0,  0);
	FaceRotations[3] = FRotator(0,		0,	90.f);
	FaceRotations[4] = FRotator(0,		0,	-90.f);
	FaceRotations[5] = FRotator(180.f,	0,	0);

	FDynamicMeshBuilder MeshBuilder;

	for(int32 f=0; f<6; f++)
	{
		FMatrix FaceTransform = FRotationMatrix(FaceRotations[f]);

		int32 VertexIndices[4];
		for(int32 VertexIndex = 0;VertexIndex < 4;VertexIndex++)
		{
			VertexIndices[VertexIndex] = MeshBuilder.AddVertex(
				FaceTransform.TransformPosition( Positions[VertexIndex] ),
				UVs[VertexIndex],
				FaceTransform.TransformVector(FVector(1,0,0)),
				FaceTransform.TransformVector(FVector(0,1,0)),
				FaceTransform.TransformVector(FVector(0,0,1)),
				FColor::White
				);
		}

		MeshBuilder.AddTriangle(VertexIndices[0],VertexIndices[1],VertexIndices[2]);
		MeshBuilder.AddTriangle(VertexIndices[0],VertexIndices[2],VertexIndices[3]);
	}

	MeshBuilder.Draw(PDI,FScaleMatrix(Radii) * BoxToWorld,MaterialRenderProxy,DepthPriorityGroup,0.f);
}



void DrawSphere(FPrimitiveDrawInterface* PDI,const FVector& Center,const FVector& Radii,int32 NumSides,int32 NumRings,const FMaterialRenderProxy* MaterialRenderProxy,uint8 DepthPriority,bool bDisableBackfaceCulling)
{
	// Use a mesh builder to draw the sphere.
	FDynamicMeshBuilder MeshBuilder;
	{
		// The first/last arc are on top of each other.
		int32 NumVerts = (NumSides+1) * (NumRings+1);
		FDynamicMeshVertex* Verts = (FDynamicMeshVertex*)FMemory::Malloc( NumVerts * sizeof(FDynamicMeshVertex) );

		// Calculate verts for one arc
		FDynamicMeshVertex* ArcVerts = (FDynamicMeshVertex*)FMemory::Malloc( (NumRings+1) * sizeof(FDynamicMeshVertex) );

		for(int32 i=0; i<NumRings+1; i++)
		{
			FDynamicMeshVertex* ArcVert = &ArcVerts[i];

			float angle = ((float)i/NumRings) * PI;

			// Note- unit sphere, so position always has mag of one. We can just use it for normal!			
			ArcVert->Position.X = 0.0f;
			ArcVert->Position.Y = FMath::Sin(angle);
			ArcVert->Position.Z = FMath::Cos(angle);

			ArcVert->SetTangents(
				FVector(1,0,0),
				FVector(0.0f,-ArcVert->Position.Z,ArcVert->Position.Y),
				ArcVert->Position
				);

			ArcVert->TextureCoordinate.X = 0.0f;
			ArcVert->TextureCoordinate.Y = ((float)i/NumRings);
		}

		// Then rotate this arc NumSides+1 times.
		for(int32 s=0; s<NumSides+1; s++)
		{
			FRotator ArcRotator(0, 360.f * (float)s/NumSides, 0);
			FRotationMatrix ArcRot( ArcRotator );
			float XTexCoord = ((float)s/NumSides);

			for(int32 v=0; v<NumRings+1; v++)
			{
				int32 VIx = (NumRings+1)*s + v;

				Verts[VIx].Position = ArcRot.TransformPosition( ArcVerts[v].Position );
				
				Verts[VIx].SetTangents(
					ArcRot.TransformVector( ArcVerts[v].TangentX ),
					ArcRot.TransformVector( ArcVerts[v].GetTangentY() ),
					ArcRot.TransformVector( ArcVerts[v].TangentZ )
					);

				Verts[VIx].TextureCoordinate.X = XTexCoord;
				Verts[VIx].TextureCoordinate.Y = ArcVerts[v].TextureCoordinate.Y;
			}
		}

		// Add all of the vertices we generated to the mesh builder.
		for(int32 VertIdx=0; VertIdx < NumVerts; VertIdx++)
		{
			MeshBuilder.AddVertex(Verts[VertIdx]);
		}
		
		// Add all of the triangles we generated to the mesh builder.
		for(int32 s=0; s<NumSides; s++)
		{
			int32 a0start = (s+0) * (NumRings+1);
			int32 a1start = (s+1) * (NumRings+1);

			for(int32 r=0; r<NumRings; r++)
			{
				MeshBuilder.AddTriangle(a0start + r + 0, a1start + r + 0, a0start + r + 1);
				MeshBuilder.AddTriangle(a1start + r + 0, a1start + r + 1, a0start + r + 1);
			}
		}

		// Free our local copy of verts and arc verts
		FMemory::Free(Verts);
		FMemory::Free(ArcVerts);
	}
	MeshBuilder.Draw(PDI, FScaleMatrix( Radii ) * FTranslationMatrix( Center ), MaterialRenderProxy, DepthPriority,bDisableBackfaceCulling);
}

void DrawCone(FPrimitiveDrawInterface* PDI,const FMatrix& ConeToWorld, float Angle1, float Angle2, int32 NumSides, bool bDrawSideLines, const FLinearColor& SideLineColor, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriority)
{
	float ang1 = FMath::Clamp<float>(Angle1, 0.01f, (float)PI - 0.01f);
	float ang2 = FMath::Clamp<float>(Angle2, 0.01f, (float)PI - 0.01f);

	float sinX_2 = FMath::Sin(0.5f * ang1);
	float sinY_2 = FMath::Sin(0.5f * ang2);

	float sinSqX_2 = sinX_2 * sinX_2;
	float sinSqY_2 = sinY_2 * sinY_2;

	float tanX_2 = FMath::Tan(0.5f * ang1);
	float tanY_2 = FMath::Tan(0.5f * ang2);

	TArray<FVector> ConeVerts;
	ConeVerts.AddUninitialized(NumSides);

	for(int32 i = 0; i < NumSides; i++)
	{
		float Fraction = (float)i/(float)(NumSides);
		float thi = 2.f*PI*Fraction;
		float phi = FMath::Atan2(FMath::Sin(thi)*sinY_2, FMath::Cos(thi)*sinX_2);
		float sinPhi = FMath::Sin(phi);
		float cosPhi = FMath::Cos(phi);
		float sinSqPhi = sinPhi*sinPhi;
		float cosSqPhi = cosPhi*cosPhi;

		float rSq, r, Sqr, alpha, beta;

		rSq = sinSqX_2*sinSqY_2/(sinSqX_2*sinSqPhi + sinSqY_2*cosSqPhi);
		r = FMath::Sqrt(rSq);
		Sqr = FMath::Sqrt(1-rSq);
		alpha = r*cosPhi;
		beta  = r*sinPhi;

		ConeVerts[i].X = (1-2*rSq);
		ConeVerts[i].Y = 2*Sqr*alpha;
		ConeVerts[i].Z = 2*Sqr*beta;
	}

	FDynamicMeshBuilder MeshBuilder;
	{
		for(int32 i=0; i < NumSides; i++)
		{
			FDynamicMeshVertex V0, V1, V2;

			// Normal of the previous face connected to this face
			FVector TriTangentZPrev = ConeVerts[i] ^ ConeVerts [ i == 0 ? NumSides - 1 : i-1 ];

			// Normal of the current face 
			FVector TriTangentZ = ConeVerts[(i+1)%NumSides] ^ ConeVerts[ i ]; // aka triangle normal

			// Normal of the next face connected to this face
			FVector TriTangentZNext = ConeVerts[(i+2)%NumSides] ^ ConeVerts[(i+1)%NumSides];

			FVector TriTangentY = ConeVerts[i];
			FVector TriTangentX = TriTangentZ ^ TriTangentY;

			{
				V0.Position = FVector(0);
				V0.TextureCoordinate.X = 0.0f;
				V0.TextureCoordinate.Y = (float)i/NumSides;

				V0.SetTangents(TriTangentX,TriTangentY,FVector(-1,0,0));
			}
		
			{
				V1.Position = ConeVerts[i];
				V1.TextureCoordinate.X = 1.0f;
				V1.TextureCoordinate.Y = (float)i/NumSides;

				FVector VertexNormal = (TriTangentZPrev + TriTangentZ).SafeNormal();
				V1.SetTangents(TriTangentX,TriTangentY,VertexNormal);
			}

	
			{
				V2.Position = ConeVerts[(i+1)%NumSides];
				V2.TextureCoordinate.X = 1.0f;
				V2.TextureCoordinate.Y = (float)((i+1)%NumSides)/NumSides;
				V2.SetTangents(TriTangentX,TriTangentY,TriTangentZ);
			
				FVector VertexNormal = (TriTangentZNext + TriTangentZ).SafeNormal();
				V2.SetTangents(TriTangentX,TriTangentY,VertexNormal);
			}

			const int32 VertexStart = MeshBuilder.AddVertex(V0);
			MeshBuilder.AddVertex(V1);
			MeshBuilder.AddVertex(V2);

			MeshBuilder.AddTriangle(VertexStart,VertexStart+1,VertexStart+2);
		}
	}
	MeshBuilder.Draw(PDI, ConeToWorld, MaterialRenderProxy, DepthPriority, 0.f);


	if(bDrawSideLines)
	{
		// Draw lines down major directions
		for(int32 i=0; i<4; i++)
		{
			PDI->DrawLine( ConeToWorld.GetOrigin(), ConeToWorld.TransformPosition( ConeVerts[ (i*NumSides/4)%NumSides ] ), SideLineColor, DepthPriority );
		}
	}
}


void DrawCylinder(FPrimitiveDrawInterface* PDI,const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis,
	float Radius, float HalfHeight, int32 Sides, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriority)
{
	DrawCylinder( PDI, FMatrix::Identity, Base, XAxis, YAxis, ZAxis, Radius, HalfHeight, Sides, MaterialRenderProxy, DepthPriority );
}

void DrawCylinder(FPrimitiveDrawInterface* PDI, const FMatrix& CylToWorld, const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis,
	float Radius, float HalfHeight, int32 Sides, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriority)
{
	const float	AngleDelta = 2.0f * PI / Sides;
	FVector	LastVertex = Base + XAxis * Radius;

	FVector2D TC = FVector2D(0.0f, 0.0f);
	float TCStep = 1.0f / Sides;

	FVector TopOffset = HalfHeight * ZAxis;

	FDynamicMeshBuilder MeshBuilder;


	//Compute vertices for base circle.
	for(int32 SideIndex = 0;SideIndex < Sides;SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;

		MeshVertex.Position = Vertex - TopOffset;
		MeshVertex.TextureCoordinate = TC;

		MeshVertex.SetTangents(
			-ZAxis,
			(-ZAxis) ^ Normal,
			Normal
			);

		MeshBuilder.AddVertex(MeshVertex); //Add bottom vertex

		LastVertex = Vertex;
		TC.X += TCStep;
	}

	LastVertex = Base + XAxis * Radius;
	TC = FVector2D(0.0f, 1.0f);

	//Compute vertices for the top circle
	for(int32 SideIndex = 0;SideIndex < Sides;SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;

		MeshVertex.Position = Vertex + TopOffset;
		MeshVertex.TextureCoordinate = TC;

		MeshVertex.SetTangents(
			-ZAxis,
			(-ZAxis) ^ Normal,
			Normal
			);

		MeshBuilder.AddVertex(MeshVertex); //Add top vertex

		LastVertex = Vertex;
		TC.X += TCStep;
	}

	//Add top/bottom triangles, in the style of a fan.
	//Note if we wanted nice rendering of the caps then we need to duplicate the vertices and modify
	//texture/tangent coordinates.
	for(int32 SideIndex = 1; SideIndex < Sides; SideIndex++)
	{
		int32 V0 = 0;
		int32 V1 = SideIndex;
		int32 V2 = (SideIndex + 1) % Sides;

		MeshBuilder.AddTriangle(V0, V1, V2); //bottom
		MeshBuilder.AddTriangle(Sides + V2, Sides + V1 , Sides + V0); //top
	}

	//Add sides.

	for(int32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		int32 V0 = SideIndex;
		int32 V1 = (SideIndex + 1) % Sides;
		int32 V2 = V0 + Sides;
		int32 V3 = V1 + Sides;

		MeshBuilder.AddTriangle(V0, V2, V1);
		MeshBuilder.AddTriangle(V2, V3, V1);
	}

	MeshBuilder.Draw(PDI, CylToWorld, MaterialRenderProxy, DepthPriority,0.f);
}

/**
 * Draws a circle using triangles.
 *
 * @param	PDI						Draw interface.
 * @param	Base					Center of the circle.
 * @param	XAxis					X alignment axis to draw along.
 * @param	YAxis					Y alignment axis to draw along.
 * @param	Color					Color of the circle.
 * @param	Radius					Radius of the circle.
 * @param	NumSides				Numbers of sides that the circle has.
 * @param	MaterialRenderProxy		Material to use for render 
 * @param	DepthPriority			Depth priority for the circle.
 */
void DrawDisc(class FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& XAxis,const FVector& YAxis,FColor Color,float Radius,int32 NumSides,const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriority)
{
	check (NumSides >= 3);

	const float	AngleDelta = 2.0f * PI / NumSides;

	FVector2D TC = FVector2D(0.0f, 0.0f);
	float TCStep = 1.0f / NumSides;
	
	FVector ZAxis = (XAxis) ^ YAxis;

	FDynamicMeshBuilder MeshBuilder;

	//Compute vertices for base circle.
	for(int32 SideIndex = 0;SideIndex < NumSides;SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex)) + YAxis * FMath::Sin(AngleDelta * (SideIndex))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;
		MeshVertex.Position = Vertex;
		MeshVertex.Color = Color;
		MeshVertex.TextureCoordinate = TC;
		MeshVertex.TextureCoordinate.X += TCStep * SideIndex;

		MeshVertex.SetTangents(
			-ZAxis,
			(-ZAxis) ^ Normal,
			Normal
			);

		MeshBuilder.AddVertex(MeshVertex); //Add bottom vertex
	}
	
	//Add top/bottom triangles, in the style of a fan.
	for(int32 SideIndex = 0; SideIndex < NumSides-1; SideIndex++)
	{
		int32 V0 = 0;
		int32 V1 = SideIndex;
		int32 V2 = (SideIndex + 1);

		MeshBuilder.AddTriangle(V0, V1, V2);
		MeshBuilder.AddTriangle(V0, V2, V1);
	}

	MeshBuilder.Draw(PDI, FMatrix::Identity, MaterialRenderProxy, DepthPriority,0.f);
}

/**
 * Draws a flat arrow with an outline.
 *
 * @param	PDI						Draw interface.
 * @param	Base					Base of the arrow.
 * @param	XAxis					X alignment axis to draw along.
 * @param	YAxis					Y alignment axis to draw along.
 * @param	Color					Color of the circle.
 * @param	Length					Length of the arrow, from base to tip.
 * @param	Width					Width of the base of the arrow, head of the arrow will be 2x.
 * @param	MaterialRenderProxy		Material to use for render 
 * @param	DepthPriority			Depth priority for the circle.
 */

/*
x-axis is from point 0 to point 2
y-axis is from point 0 to point 1
        6
        /\
       /  \
      /    \
     4_2  3_5
       |  |
       0__1
*/


void DrawFlatArrow(class FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& XAxis,const FVector& YAxis,FColor Color,float Length,int32 Width, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriority)
{
	float DistanceFromBaseToHead = Length/3.0f;
	float DistanceFromBaseToTip = DistanceFromBaseToHead*2.0f;
	float WidthOfBase = Width;
	float WidthOfHead = 2*Width;

	FVector ArrowPoints[7];
	//base points
	ArrowPoints[0] = Base - YAxis*(WidthOfBase*.5f);
	ArrowPoints[1] = Base + YAxis*(WidthOfBase*.5f);
	//inner head
	ArrowPoints[2] = ArrowPoints[0] + XAxis*DistanceFromBaseToHead;
	ArrowPoints[3] = ArrowPoints[1] + XAxis*DistanceFromBaseToHead;
	//outer head
	ArrowPoints[4] = ArrowPoints[2] - YAxis*(WidthOfBase*.5f);
	ArrowPoints[5] = ArrowPoints[3] + YAxis*(WidthOfBase*.5f);
	//tip
	ArrowPoints[6] = Base + XAxis*Length;

	//Draw lines
	{
		//base
		PDI->DrawLine(ArrowPoints[0], ArrowPoints[1], Color, DepthPriority);
		//base sides
		PDI->DrawLine(ArrowPoints[0], ArrowPoints[2], Color, DepthPriority);
		PDI->DrawLine(ArrowPoints[1], ArrowPoints[3], Color, DepthPriority);
		//head base
		PDI->DrawLine(ArrowPoints[2], ArrowPoints[4], Color, DepthPriority);
		PDI->DrawLine(ArrowPoints[3], ArrowPoints[5], Color, DepthPriority);
		//head sides
		PDI->DrawLine(ArrowPoints[4], ArrowPoints[6], Color, DepthPriority);
		PDI->DrawLine(ArrowPoints[5], ArrowPoints[6], Color, DepthPriority);

	}

	FDynamicMeshBuilder MeshBuilder;

	//Compute vertices for base circle.
	for(int32 i = 0; i< 7; ++i)
	{
		FDynamicMeshVertex MeshVertex;
		MeshVertex.Position = ArrowPoints[i];
		MeshVertex.Color = Color;
		MeshVertex.TextureCoordinate = FVector2D(0.0f, 0.0f);;
		MeshVertex.SetTangents(XAxis^YAxis, YAxis, XAxis);
		MeshBuilder.AddVertex(MeshVertex); //Add bottom vertex
	}
	
	//Add triangles / double sided
	{
		MeshBuilder.AddTriangle(0, 2, 1); //base
		MeshBuilder.AddTriangle(0, 1, 2); //base
		MeshBuilder.AddTriangle(1, 2, 3); //base
		MeshBuilder.AddTriangle(1, 3, 2); //base
		MeshBuilder.AddTriangle(4, 5, 6); //head
		MeshBuilder.AddTriangle(4, 6, 5); //head
	}

	MeshBuilder.Draw(PDI, FMatrix::Identity, MaterialRenderProxy, DepthPriority, 0.f);
}

// Line drawing utility functions.

/**
 * Draws a wireframe box.
 *
 * @param	PDI				Draw interface.
 * @param	Box				The FBox to use for drawing.
 * @param	Color			Color of the box.
 * @param	DepthPriority	Depth priority for the circle.
 */
void DrawWireBox(FPrimitiveDrawInterface* PDI,const FBox& Box,const FLinearColor& Color,uint8 DepthPriority)
{
	FVector	B[2],P,Q;
	int32 i,j;

	B[0]=Box.Min;
	B[1]=Box.Max;

	for( i=0; i<2; i++ ) for( j=0; j<2; j++ )
	{
		P.X=B[i].X; Q.X=B[i].X;
		P.Y=B[j].Y; Q.Y=B[j].Y;
		P.Z=B[0].Z; Q.Z=B[1].Z;
		PDI->DrawLine(P,Q,Color,DepthPriority);

		P.Y=B[i].Y; Q.Y=B[i].Y;
		P.Z=B[j].Z; Q.Z=B[j].Z;
		P.X=B[0].X; Q.X=B[1].X;
		PDI->DrawLine(P,Q,Color,DepthPriority);

		P.Z=B[i].Z; Q.Z=B[i].Z;
		P.X=B[j].X; Q.X=B[j].X;
		P.Y=B[0].Y; Q.Y=B[1].Y;
		PDI->DrawLine(P,Q,Color,DepthPriority);
	}
}

/**
 * Draws a circle using lines.
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center of the circle.
 * @param	X				X alignment axis to draw along.
 * @param	Y				Y alignment axis to draw along.
 * @param	Z				Z alignment axis to draw along.
 * @param	Color			Color of the circle.
 * @param	Radius			Radius of the circle.
 * @param	NumSides		Numbers of sides that the circle has.
 * @param	DepthPriority	Depth priority for the circle.
 */
void DrawCircle(FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FLinearColor& Color,float Radius,int32 NumSides,uint8 DepthPriority)
{
	const float	AngleDelta = 2.0f * PI / NumSides;
	FVector	LastVertex = Base + X * Radius;

	for(int32 SideIndex = 0;SideIndex < NumSides;SideIndex++)
	{
		const FVector Vertex = Base + (X * FMath::Cos(AngleDelta * (SideIndex + 1)) + Y * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		PDI->DrawLine(LastVertex,Vertex,Color,DepthPriority);
		LastVertex = Vertex;
	}
}

/**
 * Draws an arc using lines.
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center of the circle.
 * @param	X				Normalized axis from one point to the center
 * @param	Y				Normalized axis from other point to the center
 * @param   MinAngle        The minimum angle
 * @param   MinAngle        The maximum angle
 * @param   Radius          Radius of the arc
 * @param	Sections		Numbers of sides that the circle has.
 * @param	Color			Color of the circle.
 * @param	DepthPriority	Depth priority for the circle.
 */
void DrawArc(FPrimitiveDrawInterface* PDI, const FVector Base, const FVector X, const FVector Y, const float MinAngle, const float MaxAngle, const float Radius, const int32 Sections, const FLinearColor& Color, uint8 DepthPriority)
{
	float AngleStep = (MaxAngle - MinAngle)/((float)(Sections));
	float CurrentAngle = MinAngle;

	FVector LastVertex = Base + Radius * ( FMath::Cos(CurrentAngle * (PI/180.0f)) * X + FMath::Sin(CurrentAngle * (PI/180.0f)) * Y );
	CurrentAngle += AngleStep;

	for(int32 i=0; i<Sections; i++)
	{
		FVector ThisVertex = Base + Radius * ( FMath::Cos(CurrentAngle * (PI/180.0f)) * X + FMath::Sin(CurrentAngle * (PI/180.0f)) * Y );
		PDI->DrawLine( LastVertex, ThisVertex, Color, DepthPriority );
		LastVertex = ThisVertex;
		CurrentAngle += AngleStep;
	}
}

/**
 * Draws a sphere using circles.
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center of the sphere.
 * @param	Color			Color of the sphere.
 * @param	Radius			Radius of the sphere.
 * @param	NumSides		Numbers of sides that the circle has.
 * @param	DepthPriority	Depth priority for the circle.
 */
void DrawWireSphere(class FPrimitiveDrawInterface* PDI, const FVector& Base, const FLinearColor& Color, float Radius, int32 NumSides, uint8 DepthPriority)
{
	DrawCircle(PDI, Base, FVector(1,0,0), FVector(0,1,0), Color, Radius, NumSides, DepthPriority);
	DrawCircle(PDI, Base, FVector(1,0,0), FVector(0,0,1), Color, Radius, NumSides, DepthPriority);
	DrawCircle(PDI, Base, FVector(0,1,0), FVector(0,0,1), Color, Radius, NumSides, DepthPriority);
}

/**
 * Draws a sphere using circles, automatically calculating a reasonable number of sides
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center of the sphere.
 * @param	Color			Color of the sphere.
 * @param	Radius			Radius of the sphere.
 * @param	DepthPriority	Depth priority for the circle.
 */
void DrawWireSphereAutoSides(class FPrimitiveDrawInterface* PDI, const FVector& Base, const FLinearColor& Color, float Radius, uint8 DepthPriority)
{
	// Guess a good number of sides
	int32 NumSides =  FMath::Clamp<int32>(Radius/4.f, 16, 64);
	DrawWireSphere(PDI, Base, Color, Radius, NumSides, DepthPriority);
}

void DrawWireSphere(class FPrimitiveDrawInterface* PDI, const FTransform& Transform, const FLinearColor& Color, float Radius, int32 NumSides, uint8 DepthPriority)
{
	DrawCircle( PDI, Transform.GetLocation(), Transform.GetScaledAxis( EAxis::X ), Transform.GetScaledAxis( EAxis::Y ), Color, Radius, NumSides, SDPG_World );
	DrawCircle( PDI, Transform.GetLocation(), Transform.GetScaledAxis( EAxis::X ), Transform.GetScaledAxis( EAxis::Z ), Color, Radius, NumSides, SDPG_World );
	DrawCircle( PDI, Transform.GetLocation(), Transform.GetScaledAxis( EAxis::Y ), Transform.GetScaledAxis( EAxis::Z ), Color, Radius, NumSides, SDPG_World );
}


void DrawWireSphereAutoSides(class FPrimitiveDrawInterface* PDI, const FTransform& Transform, const FLinearColor& Color, float Radius, uint8 DepthPriority)
{
	// Guess a good number of sides
	int32 NumSides =  FMath::Clamp<int32>(Radius/4.f, 16, 64);
	DrawWireSphere(PDI, Transform, Color, Radius, NumSides, DepthPriority);
}

/**
 * Draws a wireframe cylinder.
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center pointer of the base of the cylinder.
 * @param	X				X alignment axis to draw along.
 * @param	Y				Y alignment axis to draw along.
 * @param	Z				Z alignment axis to draw along.
 * @param	Color			Color of the cylinder.
 * @param	Radius			Radius of the cylinder.
 * @param	HalfHeight		Half of the height of the cylinder.
 * @param	NumSides		Numbers of sides that the cylinder has.
 * @param	DepthPriority	Depth priority for the cylinder.
 */
void DrawWireCylinder(FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z,const FLinearColor& Color,float Radius,float HalfHeight,int32 NumSides,uint8 DepthPriority)
{
	const float	AngleDelta = 2.0f * PI / NumSides;
	FVector	LastVertex = Base + X * Radius;

	for(int32 SideIndex = 0;SideIndex < NumSides;SideIndex++)
	{
		const FVector Vertex = Base + (X * FMath::Cos(AngleDelta * (SideIndex + 1)) + Y * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;

		PDI->DrawLine(LastVertex - Z * HalfHeight,Vertex - Z * HalfHeight,Color,DepthPriority);
		PDI->DrawLine(LastVertex + Z * HalfHeight,Vertex + Z * HalfHeight,Color,DepthPriority);
		PDI->DrawLine(LastVertex - Z * HalfHeight,LastVertex + Z * HalfHeight,Color,DepthPriority);

		LastVertex = Vertex;
	}
}


static void DrawHalfCircle(FPrimitiveDrawInterface* PDI, const FVector& Base, const FVector& X, const FVector& Y, const FLinearColor& Color, float Radius, int32 NumSides)
{
	const float	AngleDelta = (float)PI / ((float)NumSides);
	FVector	LastVertex = Base + X * Radius;

	for(int32 SideIndex = 0; SideIndex < NumSides; SideIndex++)
	{
		const FVector	Vertex = Base + (X * FMath::Cos(AngleDelta * (SideIndex + 1)) + Y * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		PDI->DrawLine(LastVertex, Vertex, Color, SDPG_World);
		LastVertex = Vertex;
	}	
}


/**
 * Draws a wireframe capsule.
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center pointer of the base of the cylinder.
 * @param	X				X alignment axis to draw along.
 * @param	Y				Y alignment axis to draw along.
 * @param	Z				Z alignment axis to draw along.
 * @param	Color			Color of the cylinder.
 * @param	Radius			Radius of the cylinder.
 * @param	HalfHeight		Half of the height of the cylinder.
 * @param	NumSides		Numbers of sides that the cylinder has.
 * @param	DepthPriority	Depth priority for the cylinder.
 */
void DrawWireCapsule(FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z,const FLinearColor& Color,float Radius,float HalfHeight,int32 NumSides,uint8 DepthPriority)
{
	const FVector Origin = Base;
	const FVector XAxis = X.SafeNormal();
	const FVector YAxis = Y.SafeNormal();
	const FVector ZAxis = Z.SafeNormal();

	// because we are drawing a capsule we have to have room for the "domed caps"
	const float XScale = X.Size();
	const float YScale = Y.Size();
	const float ZScale = Z.Size();
	float CapsuleRadius = Radius * FMath::Max( FMath::Max(XScale,YScale), ZScale );
	HalfHeight *= ZScale;
	HalfHeight -= CapsuleRadius;
	HalfHeight = FMath::Max( 0.0f, HalfHeight );

	// Draw top and bottom circles
	const FVector TopEnd = Origin + (HalfHeight * ZAxis);
	const FVector BottomEnd = Origin - (HalfHeight * ZAxis);

	DrawCircle(PDI, TopEnd, XAxis, YAxis, Color, CapsuleRadius, NumSides, DepthPriority);
	DrawCircle(PDI, BottomEnd, XAxis, YAxis, Color, CapsuleRadius, NumSides, DepthPriority);

	// Draw domed caps
	DrawHalfCircle(PDI, TopEnd, YAxis, ZAxis, Color, CapsuleRadius, NumSides/2);
	DrawHalfCircle(PDI, TopEnd, XAxis, ZAxis, Color, CapsuleRadius, NumSides/2);

	const FVector NegZAxis = -ZAxis;

	DrawHalfCircle(PDI, BottomEnd, YAxis, NegZAxis, Color, CapsuleRadius, NumSides/2);
	DrawHalfCircle(PDI, BottomEnd, XAxis, NegZAxis, Color, CapsuleRadius, NumSides/2);

	// we set NumSides to 4 as it makes a nicer looking capsule as we only draw 2 HalfCircles above
	const int32 NumCylinderLines = 4;

	// Draw lines for the cylinder portion 
	const float	AngleDelta = 2.0f * PI / NumCylinderLines;
	FVector	LastVertex = Base + XAxis * CapsuleRadius;

	for( int32 SideIndex = 0; SideIndex < NumCylinderLines; SideIndex++ )
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * CapsuleRadius;

		PDI->DrawLine(LastVertex - ZAxis * HalfHeight,LastVertex + ZAxis * HalfHeight,Color,DepthPriority);

		LastVertex = Vertex;
	}
}



/**
 * Draws a wireframe cone
 *
 * @param	PDI				Draw interface.
 * @param	Transform		Generic transform to apply (ex. a local-to-world transform).
 * @param	ConeRadius		Radius of the cone.
 * @param	ConeAngle		Angle of the cone.
 * @param	ConeSides		Numbers of sides that the cone has.
 * @param	Color			Color of the cone.
 * @param	DepthPriority	Depth priority for the cone.
 * @param	Verts			Out param, the positions of the verts at the cone's base.
 */
void DrawWireCone(FPrimitiveDrawInterface* PDI, const FTransform& Transform, float ConeRadius, float ConeAngle, int32 ConeSides, const FLinearColor& Color, uint8 DepthPriority, TArray<FVector>& Verts)
{
	static const float TwoPI = 2.0f * PI;
	static const float ToRads = PI / 180.0f;
	static const float MaxAngle = 89.0f * ToRads + 0.001f;
	const float ClampedConeAngle = FMath::Clamp(ConeAngle * ToRads, 0.001f, MaxAngle);
	const float SinClampedConeAngle = FMath::Sin( ClampedConeAngle );
	const float CosClampedConeAngle = FMath::Cos( ClampedConeAngle );
	const FVector ConeDirection(1,0,0);
	const FVector ConeUpVector(0,1,0);
	const FVector ConeLeftVector(0,0,1);

	Verts.AddUninitialized( ConeSides );

	for ( int32 i = 0 ; i < Verts.Num() ; ++i )
	{
		const float Theta = static_cast<float>( (TwoPI * i) / Verts.Num() );
		Verts[i] = (ConeDirection * (ConeRadius * CosClampedConeAngle)) +
			((SinClampedConeAngle * ConeRadius * FMath::Cos( Theta )) * ConeUpVector) +
			((SinClampedConeAngle * ConeRadius * FMath::Sin( Theta )) * ConeLeftVector);
	}

	// Transform to world space.
	for ( int32 i = 0 ; i < Verts.Num() ; ++i )
	{
		Verts[i] = Transform.TransformPosition( Verts[i] );
	}

	// Draw spokes.
	for ( int32 i = 0 ; i < Verts.Num(); ++i )
	{
		PDI->DrawLine( Transform.GetLocation(), Verts[i], Color, DepthPriority );
	}

	// Draw rim.
	for ( int32 i = 0 ; i < Verts.Num()-1 ; ++i )
	{
		PDI->DrawLine( Verts[i], Verts[i+1], Color, DepthPriority );
	}
	PDI->DrawLine( Verts[Verts.Num()-1], Verts[0], Color, DepthPriority );
}

void DrawWireCone(FPrimitiveDrawInterface* PDI, const FMatrix& Transform, float ConeRadius, float ConeAngle, int32 ConeSides, const FLinearColor& Color, uint8 DepthPriority, TArray<FVector>& Verts)
{
	DrawWireCone(PDI, FTransform(Transform), ConeRadius, ConeAngle, ConeSides, Color, DepthPriority, Verts);
}

/**
 * Draws a wireframe cone with a arcs on the cap
 *
 * @param	PDI				Draw interface.
 * @param	Transform		Generic transform to apply (ex. a local-to-world transform).
 * @param	ConeRadius		Radius of the cone.
 * @param	ConeAngle		Angle of the cone.
 * @param	ConeSides		Numbers of sides that the cone has.
 * @param   ArcFrequency    How frequently to draw an arc (1 means every vertex, 2 every 2nd etc.)
 * @param	CapSegments		How many lines to use to make the arc
 * @param	Color			Color of the cone.
 * @param	DepthPriority	Depth priority for the cone.
 */
void DrawWireSphereCappedCone(FPrimitiveDrawInterface* PDI, const FTransform& Transform, float ConeRadius, float ConeAngle, int32 ConeSides, int32 ArcFrequency, int32 CapSegments, const FLinearColor& Color, uint8 DepthPriority)
{
	// The cap only works if there are an even number of verts generated so add another if needed 
	if ((ConeSides & 0x1) != 0)
	{
		++ConeSides;
	}

	TArray<FVector> Verts;
	DrawWireCone(PDI, Transform, ConeRadius, ConeAngle, ConeSides, Color, DepthPriority, Verts);

	// Draw arcs
	int32 ArcCount = (int32)(Verts.Num() / 2);
	for ( int32 i = 0; i < ArcCount; i += ArcFrequency )
	{
		const FVector X = Transform.GetUnitAxis( EAxis::X );
		FVector Y = Verts[i] - Verts[ArcCount + i]; Y.Normalize();

		DrawArc(PDI, Transform.GetTranslation(), X, Y, -ConeAngle, ConeAngle, ConeRadius, CapSegments, Color, DepthPriority);
	}
}


/**
 * Draws a wireframe chopped cone(cylinder with independant top and bottom radius).
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center pointer of the base of the cone.
 * @param	X				X alignment axis to draw along.
 * @param	Y				Y alignment axis to draw along.
 * @param	Z				Z alignment axis to draw along.
 * @param	Color			Color of the cone.
 * @param	Radius			Radius of the cone at the bottom.
 * @param	TopRadius		Radius of the cone at the top.
 * @param	HalfHeight		Half of the height of the cone.
 * @param	NumSides		Numbers of sides that the cone has.
 * @param	DepthPriority	Depth priority for the cone.
 */
void DrawWireChoppedCone(FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z,const FLinearColor& Color,float Radius, float TopRadius,float HalfHeight,int32 NumSides,uint8 DepthPriority)
{
	const float	AngleDelta = 2.0f * PI / NumSides;
	FVector	LastVertex = Base + X * Radius;
	FVector LastTopVertex = Base + X * TopRadius;

	for(int32 SideIndex = 0;SideIndex < NumSides;SideIndex++)
	{
		const FVector Vertex = Base + (X * FMath::Cos(AngleDelta * (SideIndex + 1)) + Y * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		const FVector TopVertex = Base + (X * FMath::Cos(AngleDelta * (SideIndex + 1)) + Y * FMath::Sin(AngleDelta * (SideIndex + 1))) * TopRadius;	

		PDI->DrawLine(LastVertex - Z * HalfHeight,Vertex - Z * HalfHeight,Color,DepthPriority);
		PDI->DrawLine(LastTopVertex + Z * HalfHeight,TopVertex + Z * HalfHeight,Color,DepthPriority);
		PDI->DrawLine(LastVertex - Z * HalfHeight,LastTopVertex + Z * HalfHeight,Color,DepthPriority);

		LastVertex = Vertex;
		LastTopVertex = TopVertex;
	}
}

/**
 * Draws an oriented box.
 *
 * @param	PDI				Draw interface.
 * @param	Base			Center point of the box.
 * @param	X				X alignment axis to draw along.
 * @param	Y				Y alignment axis to draw along.
 * @param	Z				Z alignment axis to draw along.
 * @param	Color			Color of the box.
 * @param	Extent			Vector with the half-sizes of the box.
 * @param	DepthPriority	Depth priority for the cone.
 */

void DrawOrientedWireBox(FPrimitiveDrawInterface* PDI,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z, FVector Extent, const FLinearColor& Color,uint8 DepthPriority)
{
	FVector	B[2],P,Q;
	int32 i,j;

	FMatrix m(X, Y, Z, Base);
	B[0] = -Extent;
	B[1] = Extent;

	for( i=0; i<2; i++ ) for( j=0; j<2; j++ )
	{
		P.X=B[i].X; Q.X=B[i].X;
		P.Y=B[j].Y; Q.Y=B[j].Y;
		P.Z=B[0].Z; Q.Z=B[1].Z;
		P = m.TransformPosition(P); Q = m.TransformPosition(Q);
		PDI->DrawLine(P,Q,Color,DepthPriority);

		P.Y=B[i].Y; Q.Y=B[i].Y;
		P.Z=B[j].Z; Q.Z=B[j].Z;
		P.X=B[0].X; Q.X=B[1].X;
		P = m.TransformPosition(P); Q = m.TransformPosition(Q);
		PDI->DrawLine(P,Q,Color,DepthPriority);

		P.Z=B[i].Z; Q.Z=B[i].Z;
		P.X=B[j].X; Q.X=B[j].X;
		P.Y=B[0].Y; Q.Y=B[1].Y;
		P = m.TransformPosition(P); Q = m.TransformPosition(Q);
		PDI->DrawLine(P,Q,Color,DepthPriority);
	}
}


void DrawCoordinateSystem(FPrimitiveDrawInterface* PDI, FVector const& AxisLoc, FRotator const& AxisRot, float Scale, uint8 DepthPriority)
{
	FRotationMatrix R(AxisRot);
	FVector const X = R.GetScaledAxis( EAxis::X );
	FVector const Y = R.GetScaledAxis( EAxis::Y );
	FVector const Z = R.GetScaledAxis( EAxis::Z );

	PDI->DrawLine(AxisLoc, AxisLoc + X*Scale, FLinearColor::Red, DepthPriority );
	PDI->DrawLine(AxisLoc, AxisLoc + Y*Scale, FLinearColor::Green, DepthPriority );
	PDI->DrawLine(AxisLoc, AxisLoc + Z*Scale, FLinearColor::Blue, DepthPriority );
}
/**
 * Draws a directional arrow.
 *
 * @param	PDI				Draw interface.
 * @param	ArrowToWorld	Transform matrix for the arrow.
 * @param	InColor			Color of the arrow.
 * @param	Length			Length of the arrow
 * @param	ArrowSize		Size of the arrow head.
 * @param	DepthPriority	Depth priority for the arrow.
 */
void DrawDirectionalArrow(FPrimitiveDrawInterface* PDI,const FMatrix& ArrowToWorld,const FLinearColor& InColor,float Length,float ArrowSize,uint8 DepthPriority)
{
	PDI->DrawLine(ArrowToWorld.TransformPosition(FVector(Length,0,0)),ArrowToWorld.TransformPosition(FVector::ZeroVector),InColor,DepthPriority);
	PDI->DrawLine(ArrowToWorld.TransformPosition(FVector(Length,0,0)),ArrowToWorld.TransformPosition(FVector(Length-ArrowSize,+ArrowSize,+ArrowSize)),InColor,DepthPriority);
	PDI->DrawLine(ArrowToWorld.TransformPosition(FVector(Length,0,0)),ArrowToWorld.TransformPosition(FVector(Length-ArrowSize,+ArrowSize,-ArrowSize)),InColor,DepthPriority);
	PDI->DrawLine(ArrowToWorld.TransformPosition(FVector(Length,0,0)),ArrowToWorld.TransformPosition(FVector(Length-ArrowSize,-ArrowSize,+ArrowSize)),InColor,DepthPriority);
	PDI->DrawLine(ArrowToWorld.TransformPosition(FVector(Length,0,0)),ArrowToWorld.TransformPosition(FVector(Length-ArrowSize,-ArrowSize,-ArrowSize)),InColor,DepthPriority);
}

/**
 * Draws a axis-aligned 3 line star.
 *
 * @param	PDI				Draw interface.
 * @param	Position		Position of the star.
 * @param	Size			Size of the star
 * @param	InColor			Color of the arrow.
 * @param	DepthPriority	Depth priority for the star.
 */
void DrawWireStar(FPrimitiveDrawInterface* PDI,const FVector& Position, float Size, const FLinearColor& Color,uint8 DepthPriority)
{
	PDI->DrawLine(Position + Size * FVector(1,0,0), Position - Size * FVector(1,0,0), Color, DepthPriority);
	PDI->DrawLine(Position + Size * FVector(0,1,0), Position - Size * FVector(0,1,0), Color, DepthPriority);
	PDI->DrawLine(Position + Size * FVector(0,0,1), Position - Size * FVector(0,0,1), Color, DepthPriority);
}

/**
 * Draws a dashed line.
 *
 * @param	PDI				Draw interface.
 * @param	Start			Start position of the line.
 * @param	End				End position of the line.
 * @param	Color			Color of the arrow.
 * @param	DashSize		Size of each of the dashes that makes up the line.
 * @param	DepthPriority	Depth priority for the line.
 */
void DrawDashedLine(FPrimitiveDrawInterface* PDI, const FVector& Start, const FVector& End, const FLinearColor& Color, float DashSize, uint8 DepthPriority, float DepthBias)
{
	FVector LineDir = End - Start;
	float LineLeft = (End - Start).Size();
	LineDir /= LineLeft;

	const int32 nLines = FMath::Ceil(LineLeft / (DashSize*2));
	PDI->AddReserveLines(DepthPriority, nLines, DepthBias != 0);

	const FVector Dash = (DashSize * LineDir);

	FVector DrawStart = Start;
	while (LineLeft > DashSize)
	{
		const FVector DrawEnd = DrawStart + Dash;

		PDI->DrawLine(DrawStart, DrawEnd, Color, DepthPriority, 0.0f, DepthBias);

		LineLeft -= 2*DashSize;
		DrawStart = DrawEnd + Dash;
	}
	if (LineLeft > 0.0f)
	{
		const FVector DrawEnd = End;

		PDI->DrawLine(DrawStart, DrawEnd, Color, DepthPriority, 0.0f, DepthBias);
	}
}

/**
 * Draws a wireframe diamond.
 *
 * @param	PDI				Draw interface.
 * @param	DiamondMatrix	Transform Matrix for the diamond.
 * @param	Size			Size of the diamond.
 * @param	InColor			Color of the diamond.
 * @param	DepthPriority	Depth priority for the diamond.
 */
void DrawWireDiamond(FPrimitiveDrawInterface* PDI,const FMatrix& DiamondMatrix, float Size, const FLinearColor& InColor,uint8 DepthPriority)
{
	const FVector TopPoint = DiamondMatrix.TransformPosition( FVector(0,0,1) * Size );
	const FVector BottomPoint = DiamondMatrix.TransformPosition( FVector(0,0,-1) * Size );

	const float OneOverRootTwo = FMath::Sqrt(0.5f);

	FVector SquarePoints[4];
	SquarePoints[0] = DiamondMatrix.TransformPosition( FVector(1,1,0) * Size * OneOverRootTwo );
	SquarePoints[1] = DiamondMatrix.TransformPosition( FVector(1,-1,0) * Size * OneOverRootTwo );
	SquarePoints[2] = DiamondMatrix.TransformPosition( FVector(-1,-1,0) * Size * OneOverRootTwo );
	SquarePoints[3] = DiamondMatrix.TransformPosition( FVector(-1,1,0) * Size * OneOverRootTwo );

	PDI->DrawLine(TopPoint, SquarePoints[0], InColor, DepthPriority);
	PDI->DrawLine(TopPoint, SquarePoints[1], InColor, DepthPriority);
	PDI->DrawLine(TopPoint, SquarePoints[2], InColor, DepthPriority);
	PDI->DrawLine(TopPoint, SquarePoints[3], InColor, DepthPriority);

	PDI->DrawLine(BottomPoint, SquarePoints[0], InColor, DepthPriority);
	PDI->DrawLine(BottomPoint, SquarePoints[1], InColor, DepthPriority);
	PDI->DrawLine(BottomPoint, SquarePoints[2], InColor, DepthPriority);
	PDI->DrawLine(BottomPoint, SquarePoints[3], InColor, DepthPriority);

	PDI->DrawLine(SquarePoints[0], SquarePoints[1], InColor, DepthPriority);
	PDI->DrawLine(SquarePoints[1], SquarePoints[2], InColor, DepthPriority);
	PDI->DrawLine(SquarePoints[2], SquarePoints[3], InColor, DepthPriority);
	PDI->DrawLine(SquarePoints[3], SquarePoints[0], InColor, DepthPriority);
}

FLinearColor GetSelectionColor(const FLinearColor& BaseColor,bool bSelected,bool bHovered, bool bUseOverlayIntensity)
{
	FLinearColor FinalColor = BaseColor;
	if( bSelected )
	{
		FinalColor = GEngine->GetSelectedMaterialColor();
	}

	static const float BaseIntensity = 0.5f;
	static const float SelectedIntensity = 0.5f;
	static const float HoverIntensity = 0.15f;

	const float OverlayIntensity = bUseOverlayIntensity ? GEngine->SelectionHighlightIntensity : 1.0f;
	float ResultingIntensity = bSelected ? SelectedIntensity : ( bHovered ? HoverIntensity : 0.0f );

	ResultingIntensity = ( ResultingIntensity * OverlayIntensity ) + BaseIntensity;

	FLinearColor ret = FinalColor * FMath::Pow(ResultingIntensity, 2.2f);
	ret.A = FinalColor.A;

	return ret;
}

bool IsRichView(const FSceneView* View)
{
	// Flags which make the view rich when absent.
	if( !View->Family->EngineShowFlags.LOD ||
		!View->Family->EngineShowFlags.IndirectLightingCache ||
		!View->Family->EngineShowFlags.Lighting ||
		!View->Family->EngineShowFlags.Materials)
	{
		return true;
	}

	// Flags which make the view rich when present.
	if( View->Family->EngineShowFlags.LightComplexity ||
		View->Family->EngineShowFlags.ShaderComplexity ||
		View->Family->EngineShowFlags.StationaryLightOverlap ||
		View->Family->EngineShowFlags.BSPSplit ||
		View->Family->EngineShowFlags.LightMapDensity ||
		View->Family->EngineShowFlags.PropertyColoration ||
		View->Family->EngineShowFlags.MeshEdges ||
		View->Family->EngineShowFlags.LightInfluences ||
		View->Family->EngineShowFlags.Wireframe ||
		View->Family->EngineShowFlags.LevelColoration)
	{
		return true;
	}

	return false;
}


/**
 * Draws a mesh, modifying the material which is used depending on the view's show flags.
 * Meshes with materials irrelevant to the pass which the mesh is being drawn for may be entirely ignored.
 *
 * @param PDI - The primitive draw interface to draw the mesh on.
 * @param Mesh - The mesh to draw.
 * @param WireframeColor - The color which is used when rendering the mesh with EngineShowFlags.Wireframe.
 * @param LevelColor - The color which is used when rendering the mesh with EngineShowFlags.LevelColoration.
 * @param PropertyColor - The color to use when rendering the mesh with EngineShowFlags.PropertyColoration.
 * @param PrimitiveInfo - The FScene information about the UPrimitiveComponent.
 * @param bSelected - True if the primitive is selected.
 * @param ExtraDrawFlags - optional flags to override the view family show flags when rendering
 * @return Number of passes rendered for the mesh
 */
int32 DrawRichMesh(
	FPrimitiveDrawInterface* PDI,
	const FMeshBatch& Mesh,
	const FLinearColor& WireframeColor,
	const FLinearColor& LevelColor,
	const FLinearColor& PropertyColor,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	bool bSelected,
	bool bDrawInWireframe
	)
{
	int32 PassesRendered=0;

#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// Draw the mesh unmodified.
	PassesRendered = PDI->DrawMesh( Mesh );
#else

	// If debug viewmodes are not allowed, skip all of the debug viewmode handling
	if (!AllowDebugViewmodes())
	{
		return PDI->DrawMesh( Mesh );
	}

	const FEngineShowFlags& EngineShowFlags = PDI->View->Family->EngineShowFlags;

	if(bDrawInWireframe)
	{
		// In wireframe mode, draw the edges of the mesh with the specified wireframe color, or
		// with the level or property color if level or property coloration is enabled.
		FLinearColor BaseColor( WireframeColor );
		if (EngineShowFlags.PropertyColoration)
		{
			BaseColor = PropertyColor;
		}
		else if (EngineShowFlags.LevelColoration)
		{
			BaseColor = LevelColor;
		}

		if(Mesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->MaterialModifiesMeshPosition())
		{
			// If the material is mesh-modifying, we cannot rely on substitution
			const FOverrideSelectionColorMaterialRenderProxy WireframeMaterialInstance(
				Mesh.MaterialRenderProxy,
				GetSelectionColor( BaseColor, bSelected, Mesh.MaterialRenderProxy->IsHovered(), /*bUseOverlayIntensity=*/false)
				);

			FMeshBatch ModifiedMesh = Mesh;
			ModifiedMesh.bWireframe = true;
			ModifiedMesh.MaterialRenderProxy = &WireframeMaterialInstance;
			PassesRendered = PDI->DrawMesh(ModifiedMesh);
		}
		else
		{
			const FColoredMaterialRenderProxy WireframeMaterialInstance(
				GEngine->WireframeMaterial->GetRenderProxy(Mesh.MaterialRenderProxy->IsSelected(), Mesh.MaterialRenderProxy->IsHovered()),
				GetSelectionColor( BaseColor, bSelected, Mesh.MaterialRenderProxy->IsHovered(), /*bUseOverlayIntensity=*/false)
				);
			FMeshBatch ModifiedMesh = Mesh;
			ModifiedMesh.bWireframe = true;
			ModifiedMesh.MaterialRenderProxy = &WireframeMaterialInstance;
			PassesRendered = PDI->DrawMesh(ModifiedMesh);
		}
	}
	else if(PDI->View->Family->EngineShowFlags.LightComplexity)
	{
		// Don't render unlit translucency when in 'light complexity' viewmode.
		if (!Mesh.IsTranslucent() || Mesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->GetLightingModel() != MLM_Unlit)
		{
			// Count the number of lights interacting with this primitive.
			int32 NumDynamicLights = GetRendererModule().GetNumDynamicLightsAffectingPrimitive(PrimitiveSceneProxy->GetPrimitiveSceneInfo(),Mesh.LCI);

			// Get a colored material to represent the number of lights.
			// Some component types (BSP) have multiple FLightCacheInterface's per component, so make sure the whole component represents the number of dominant lights affecting
			NumDynamicLights = FMath::Min( NumDynamicLights, GEngine->LightComplexityColors.Num() - 1 );
			const FColor Color = GEngine->LightComplexityColors[NumDynamicLights];

			FColoredMaterialRenderProxy LightComplexityMaterialInstance(
				GEngine->LevelColorationUnlitMaterial->GetRenderProxy(Mesh.MaterialRenderProxy->IsSelected(), Mesh.MaterialRenderProxy->IsHovered()),
				Color
				);

			// Draw the mesh colored by light complexity.
			FMeshBatch ModifiedMesh = Mesh;
			ModifiedMesh.MaterialRenderProxy = &LightComplexityMaterialInstance;
			PassesRendered = PDI->DrawMesh(ModifiedMesh);
		}
	}
	else if(!EngineShowFlags.Materials && !PDI->View->bForceShowMaterials)
	{
		// Don't render unlit translucency when in 'lighting only' viewmode.
		if (Mesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->GetLightingModel() != MLM_Unlit
			// Don't render translucency in 'lighting only', since the viewmode works by overriding with an opaque material
			// This would cause a mismatch of the material's blend mode with the primitive's view relevance,
			// And make faint particles block the view
			&& !IsTranslucentBlendMode(Mesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->GetBlendMode()))
		{
			// When materials aren't shown, apply the same basic material to all meshes.
			FMeshBatch ModifiedMesh = Mesh;

			bool bTextureMapped = false;
			FVector2D LMResolution;

			if (PDI->View->Family->EngineShowFlags.LightMapDensity &&
				ModifiedMesh.LCI &&
				(ModifiedMesh.LCI->GetLightMapInteraction().GetType() == LMIT_Texture) &&
				ModifiedMesh.LCI->GetLightMapInteraction().GetTexture())
			{
				LMResolution.X = Mesh.LCI->GetLightMapInteraction().GetTexture()->GetSizeX();
				LMResolution.Y = Mesh.LCI->GetLightMapInteraction().GetTexture()->GetSizeY();
				bTextureMapped = true;
			}

			if (bTextureMapped == false)
			{
				FMaterialRenderProxy* RenderProxy = GEngine->LevelColorationLitMaterial->GetRenderProxy(Mesh.MaterialRenderProxy->IsSelected(),Mesh.MaterialRenderProxy->IsHovered());
				const FColoredMaterialRenderProxy LightingOnlyMaterialInstance(
					RenderProxy,
					GEngine->LightingOnlyBrightness
					);

				ModifiedMesh.MaterialRenderProxy = &LightingOnlyMaterialInstance;
				PassesRendered = PDI->DrawMesh(ModifiedMesh);
			}
			else
			{
				FMaterialRenderProxy* RenderProxy = GEngine->LightingTexelDensityMaterial->GetRenderProxy(Mesh.MaterialRenderProxy->IsSelected(),Mesh.MaterialRenderProxy->IsHovered());
				const FLightingDensityMaterialRenderProxy LightingDensityMaterialInstance(
					RenderProxy,
					GEngine->LightingOnlyBrightness,
					LMResolution
					);

				ModifiedMesh.MaterialRenderProxy = &LightingDensityMaterialInstance;
				PassesRendered = PDI->DrawMesh(ModifiedMesh);
			}
		}
	}
	else
	{	
		if(EngineShowFlags.PropertyColoration)
		{
			// In property coloration mode, override the mesh's material with a color that was chosen based on property value.
			const UMaterial* PropertyColorationMaterial = EngineShowFlags.Lighting ? GEngine->LevelColorationLitMaterial : GEngine->LevelColorationUnlitMaterial;

			const FColoredMaterialRenderProxy PropertyColorationMaterialInstance(
				PropertyColorationMaterial->GetRenderProxy(Mesh.MaterialRenderProxy->IsSelected(), Mesh.MaterialRenderProxy->IsHovered()),
				GetSelectionColor(PropertyColor,bSelected,Mesh.MaterialRenderProxy->IsHovered())
				);
			FMeshBatch ModifiedMesh = Mesh;
			ModifiedMesh.MaterialRenderProxy = &PropertyColorationMaterialInstance;
			PassesRendered = PDI->DrawMesh(ModifiedMesh);
		}
		else if (EngineShowFlags.LevelColoration)
		{
			const UMaterial* LevelColorationMaterial = EngineShowFlags.Lighting ? GEngine->LevelColorationLitMaterial : GEngine->LevelColorationUnlitMaterial;
			// Draw the mesh with level coloration.
			const FColoredMaterialRenderProxy LevelColorationMaterialInstance(
				LevelColorationMaterial->GetRenderProxy(Mesh.MaterialRenderProxy->IsSelected(), Mesh.MaterialRenderProxy->IsHovered()),
				GetSelectionColor(LevelColor,bSelected,Mesh.MaterialRenderProxy->IsHovered())
				);
			FMeshBatch ModifiedMesh = Mesh;
			ModifiedMesh.MaterialRenderProxy = &LevelColorationMaterialInstance;
			PassesRendered = PDI->DrawMesh(ModifiedMesh);
		}
		else if (EngineShowFlags.BSPSplit
			&& PrimitiveSceneProxy->ShowInBSPSplitViewmode())
		{
			// Determine unique color for model component.
			FLinearColor BSPSplitColor;
			FRandomStream RandomStream(GetTypeHash(PrimitiveSceneProxy->GetPrimitiveComponentId().Value));
			BSPSplitColor.R = RandomStream.GetFraction();
			BSPSplitColor.G = RandomStream.GetFraction();
			BSPSplitColor.B = RandomStream.GetFraction();
			BSPSplitColor.A = 1.0f;

			// Piggy back on the level coloration material.
			const UMaterial* BSPSplitMaterial = EngineShowFlags.Lighting ? GEngine->LevelColorationLitMaterial : GEngine->LevelColorationUnlitMaterial;
			
			// Draw BSP mesh with unique color for each model component.
			const FColoredMaterialRenderProxy BSPSplitMaterialInstance(
				BSPSplitMaterial->GetRenderProxy(Mesh.MaterialRenderProxy->IsSelected(), Mesh.MaterialRenderProxy->IsHovered()),
				GetSelectionColor(BSPSplitColor,bSelected,Mesh.MaterialRenderProxy->IsHovered())
				);
			FMeshBatch ModifiedMesh = Mesh;
			ModifiedMesh.MaterialRenderProxy = &BSPSplitMaterialInstance;
			PassesRendered = PDI->DrawMesh(ModifiedMesh);
		}
		else if (PrimitiveSceneProxy->HasStaticLighting() && !PrimitiveSceneProxy->HasValidSettingsForStaticLighting())
		{
			const FColoredMaterialRenderProxy InvalidSettingsMaterialInstance(
				GEngine->InvalidLightmapSettingsMaterial->GetRenderProxy(bSelected),
				GetSelectionColor(LevelColor,bSelected,Mesh.MaterialRenderProxy->IsHovered())
				);
			FMeshBatch ModifiedMesh = Mesh;
			ModifiedMesh.MaterialRenderProxy = &InvalidSettingsMaterialInstance;
			PassesRendered = PDI->DrawMesh(ModifiedMesh);
		}
		else 
		{
			// Draw the mesh unmodified.
			PassesRendered = PDI->DrawMesh( Mesh );
		}

		//Draw a wireframe overlay last, if requested
		if(EngineShowFlags.MeshEdges)
		{
			// Draw the mesh's edges in blue, on top of the base geometry.
			if(Mesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->MaterialModifiesMeshPosition())
			{
				const FSceneView* View = PDI->View;

				// If the material is mesh-modifying, we cannot rely on substitution
				const FOverrideSelectionColorMaterialRenderProxy WireframeMaterialInstance(
					Mesh.MaterialRenderProxy,
					WireframeColor
					);

				FMeshBatch ModifiedMesh = Mesh;
				ModifiedMesh.bWireframe = true;
				ModifiedMesh.MaterialRenderProxy = &WireframeMaterialInstance;
				PassesRendered = PDI->DrawMesh(ModifiedMesh);
			}
			else
			{
				FColoredMaterialRenderProxy WireframeMaterialInstance(
					GEngine->WireframeMaterial->GetRenderProxy(Mesh.MaterialRenderProxy->IsSelected(), Mesh.MaterialRenderProxy->IsHovered()),
					WireframeColor
					);
				FMeshBatch ModifiedMesh = Mesh;
				ModifiedMesh.bWireframe = true;
				ModifiedMesh.MaterialRenderProxy = &WireframeMaterialInstance;
				PassesRendered = PDI->DrawMesh(ModifiedMesh);
			}
		}
	}
#endif

	return PassesRendered;
}

void ClampUVs(FVector2D* UVs, int32 NumUVs)
{
	const float FudgeFactor = 0.1f;
	FVector2D Bias(0.0f,0.0f);

	float MinU = UVs[0].X;
	float MinV = UVs[0].Y;
	for (int32 i = 1; i < NumUVs; ++i)
	{
		MinU = FMath::Min(MinU,UVs[i].X);
		MinV = FMath::Min(MinU,UVs[i].Y);
	}

	if (MinU < -FudgeFactor || MinU > (1.0f+FudgeFactor))
	{
		Bias.X = FMath::Floor(MinU);
	}
	if (MinV < -FudgeFactor || MinV > (1.0f+FudgeFactor))
	{
		Bias.Y = FMath::Floor(MinV);
	}

	for (int32 i = 0; i < NumUVs; i++)
	{
		UVs[i] += Bias;
	}
}

bool IsUVOutOfBounds(FVector2D UV)
{
	const float FudgeFactor = 1.0f/1024.0f;
	return (UV.X < -FudgeFactor || UV.X > (1.0f+FudgeFactor))
		|| (UV.Y < -FudgeFactor || UV.Y > (1.0f+FudgeFactor));
}

template<class VertexBufferType, class IndexBufferType>
void DrawUVsInternal(FViewport* InViewport, FCanvas* InCanvas, int32 InTextYPos, const int32 LODLevel, int32 UVChannel, TArray<FVector2D> SelectedEdgeTexCoords, VertexBufferType& VertexBuffer, IndexBufferType& Indices )
{
	//draw a string showing what UV channel and LOD is being displayed
	InCanvas->DrawShadowedString( 
		6,
		InTextYPos,
		*FText::Format( NSLOCTEXT("UnrealEd", "UVOverlay_F", "Showing UV channel {0} for LOD {1}"), FText::AsNumber(UVChannel), FText::AsNumber(LODLevel) ).ToString(),
		GEngine->GetSmallFont(),
		FLinearColor::White
		);
	InTextYPos += 18;

	if( ( ( uint32 )UVChannel < VertexBuffer.GetNumTexCoords() ) )
	{
		//calculate scaling
		const uint32 BorderWidth = 5;
		const uint32 MinY = InTextYPos + BorderWidth;
		const uint32 MinX = BorderWidth;
		const FVector2D UVBoxOrigin(MinX, MinY);
		const FVector2D BoxOrigin( MinX - 1, MinY - 1 );
		const uint32 UVBoxScale = FMath::Min(InViewport->GetSizeXY().X - MinX, InViewport->GetSizeXY().Y - MinY) - BorderWidth;
		const uint32 BoxSize = UVBoxScale + 2;
		FCanvasBoxItem BoxItem( BoxOrigin, FVector2D( BoxSize, BoxSize ) );
		BoxItem.SetColor( FLinearColor::Black );
		InCanvas->DrawItem( BoxItem );

		//draw triangles
		uint32 NumIndices = Indices.Num();
		FCanvasLineItem LineItem;
		for (uint32 i = 0; i < NumIndices - 2; i += 3)
		{
			FVector2D UVs[3];
			bool bOutOfBounds[3];

			for (int32 Corner=0; Corner<3; Corner++)
			{
				UVs[Corner] = ( VertexBuffer.GetVertexUV( Indices[ i + Corner ], UVChannel ) );
				bOutOfBounds[Corner] = IsUVOutOfBounds(UVs[Corner]);
			}

			// Clamp the UV triangle to the [0,1] range (with some fudge).
			ClampUVs(UVs,3);

			for (int32 Edge=0; Edge<3; Edge++)
			{
				int32 Corner1 = Edge;
				int32 Corner2 = (Edge + 1) % 3;
				FLinearColor Color = (bOutOfBounds[Corner1] || bOutOfBounds[Corner2]) ? FLinearColor( 0.6f, 0.0f, 0.0f ) : FLinearColor::White;
				LineItem.SetColor( Color );
				LineItem.Draw( InCanvas, UVs[Corner1] * UVBoxScale + UVBoxOrigin, UVs[Corner2] * UVBoxScale + UVBoxOrigin );
			}
		}

		// Draw any edges that are currently selected by the user
		if( SelectedEdgeTexCoords.Num() > 0 )
		{
			LineItem.SetColor( FLinearColor::Yellow );
			for(int32 UVIndex = 0; UVIndex < SelectedEdgeTexCoords.Num(); UVIndex += 2)
			{
				FVector2D UVs[2];
				UVs[0] = ( SelectedEdgeTexCoords[UVIndex] );
				UVs[1] = ( SelectedEdgeTexCoords[UVIndex + 1] );
				ClampUVs(UVs,2);
				
				LineItem.Draw( InCanvas, UVs[0] * UVBoxScale + UVBoxOrigin, UVs[1] * UVBoxScale + UVBoxOrigin );
			}
		}
	}
}

void DrawUVs(FViewport* InViewport, FCanvas* InCanvas, int32 InTextYPos, const int32 LODLevel, int32 UVChannel, TArray<FVector2D> SelectedEdgeTexCoords, FStaticMeshRenderData* StaticMeshRenderData, FStaticLODModel* SkeletalMeshRenderData )
{
	if(StaticMeshRenderData)
	{
		FIndexArrayView IndexBuffer = StaticMeshRenderData->LODResources[LODLevel].IndexBuffer.GetArrayView();
		DrawUVsInternal(InViewport, InCanvas, InTextYPos, LODLevel, UVChannel, SelectedEdgeTexCoords, StaticMeshRenderData->LODResources[LODLevel].VertexBuffer, IndexBuffer);
	}
	else if(SkeletalMeshRenderData)
	{
		TArray<uint32> IndexBuffer;
		SkeletalMeshRenderData->MultiSizeIndexContainer.GetIndexBuffer(IndexBuffer);
		DrawUVsInternal(InViewport, InCanvas, InTextYPos, LODLevel, UVChannel, SelectedEdgeTexCoords, SkeletalMeshRenderData->VertexBufferGPUSkin, IndexBuffer);
	}
	else
	{
		check(false); // Must supply either StaticMeshRenderData or SkeletalMeshRenderData
	}
}
