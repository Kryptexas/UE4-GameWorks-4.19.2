// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysXSupport.h"

static const int32 DrawCollisionSides = 16;
static const int32 DrawConeLimitSides = 40;

static const float DebugJointPosSize = 5.0f;
static const float DebugJointAxisSize = 20.0f;

static const float JointRenderSize = 5.0f;
static const float LimitRenderSize = 16.0f;

static const FColor JointUnselectedColor(255,0,255);
static const FColor	JointFrame1Color(255,0,0);
static const FColor JointFrame2Color(0,0,255);
static const FColor	JointLimitColor(0,255,0);
static const FColor	JointRefColor(255,255,0);
static const FColor JointLockedColor(255,128,10);

/////////////////////////////////////////////////////////////////////////////////////
// FKSphereElem
/////////////////////////////////////////////////////////////////////////////////////

// NB: ElemTM is assumed to have no scaling in it!
void FKSphereElem::DrawElemWire(FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, float Scale, const FColor Color)
{
	FVector ElemCenter = ElemTM.GetLocation();
	FVector X = ElemTM.GetScaledAxis( EAxis::X );
	FVector Y = ElemTM.GetScaledAxis( EAxis::Y );
	FVector Z = ElemTM.GetScaledAxis( EAxis::Z );

	DrawCircle(PDI,ElemCenter, X, Y, Color, Scale*Radius, DrawCollisionSides, SDPG_World);
	DrawCircle(PDI,ElemCenter, X, Z, Color, Scale*Radius, DrawCollisionSides, SDPG_World);
	DrawCircle(PDI,ElemCenter, Y, Z, Color, Scale*Radius, DrawCollisionSides, SDPG_World);
}

void FKSphereElem::DrawElemSolid(class FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, float Scale, const FMaterialRenderProxy* MaterialRenderProxy)
{
	DrawSphere(PDI, ElemTM.GetLocation(), FVector( this->Radius * Scale ), DrawCollisionSides, DrawCollisionSides/2, MaterialRenderProxy, SDPG_World );
}


/////////////////////////////////////////////////////////////////////////////////////
// FKBoxElem
/////////////////////////////////////////////////////////////////////////////////////

// NB: ElemTM is assumed to have no scaling in it!
void FKBoxElem::DrawElemWire(FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, float Scale, const FColor Color)
{
	FVector	B[2], P, Q, Radii;

	// X,Y,Z member variables are LENGTH not RADIUS
	Radii.X = Scale*0.5f*X;
	Radii.Y = Scale*0.5f*Y;
	Radii.Z = Scale*0.5f*Z;

	B[0] = Radii; // max
	B[1] = -1.0f * Radii; // min

	for( int32 i=0; i<2; i++ )
	{
		for( int32 j=0; j<2; j++ )
		{
			P.X=B[i].X; Q.X=B[i].X;
			P.Y=B[j].Y; Q.Y=B[j].Y;
			P.Z=B[0].Z; Q.Z=B[1].Z;
			PDI->DrawLine( ElemTM.TransformPosition(P), ElemTM.TransformPosition(Q), Color, SDPG_World);

			P.Y=B[i].Y; Q.Y=B[i].Y;
			P.Z=B[j].Z; Q.Z=B[j].Z;
			P.X=B[0].X; Q.X=B[1].X;
			PDI->DrawLine( ElemTM.TransformPosition(P), ElemTM.TransformPosition(Q), Color, SDPG_World);

			P.Z=B[i].Z; Q.Z=B[i].Z;
			P.X=B[j].X; Q.X=B[j].X;
			P.Y=B[0].Y; Q.Y=B[1].Y;
			PDI->DrawLine( ElemTM.TransformPosition(P), ElemTM.TransformPosition(Q), Color, SDPG_World);
		}
	}
}

void FKBoxElem::DrawElemSolid(class FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, float Scale, const FMaterialRenderProxy* MaterialRenderProxy)
{
	DrawBox(PDI, ElemTM.ToMatrixWithScale(), 0.5f * FVector(X, Y, Z), MaterialRenderProxy, SDPG_World );
}

/////////////////////////////////////////////////////////////////////////////////////
// FKSphylElem
/////////////////////////////////////////////////////////////////////////////////////

static void DrawHalfCircle(FPrimitiveDrawInterface* PDI, const FVector& Base, const FVector& X, const FVector& Y, const FColor Color, float Radius)
{
	float	AngleDelta = 2.0f * (float)PI / ((float)DrawCollisionSides);
	FVector	LastVertex = Base + X * Radius;

	for(int32 SideIndex = 0; SideIndex < (DrawCollisionSides/2); SideIndex++)
	{
		FVector	Vertex = Base + (X * FMath::Cos(AngleDelta * (SideIndex + 1)) + Y * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		PDI->DrawLine(LastVertex, Vertex, Color, SDPG_World);
		LastVertex = Vertex;
	}	
}

// NB: ElemTM is assumed to have no scaling in it!
void FKSphylElem::DrawElemWire(FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, float Scale, const FColor Color)
{
	FVector Origin = ElemTM.GetLocation();
	FVector XAxis = ElemTM.GetScaledAxis( EAxis::X );
	FVector YAxis = ElemTM.GetScaledAxis( EAxis::Y );
	FVector ZAxis = ElemTM.GetScaledAxis( EAxis::Z );

	// Draw top and bottom circles
	FVector TopEnd = Origin + Scale*0.5f*Length*ZAxis;
	FVector BottomEnd = Origin - Scale*0.5f*Length*ZAxis;

	DrawCircle(PDI,TopEnd, XAxis, YAxis, Color, Scale*Radius, DrawCollisionSides, SDPG_World);
	DrawCircle(PDI,BottomEnd, XAxis, YAxis, Color, Scale*Radius, DrawCollisionSides, SDPG_World);

	// Draw domed caps
	DrawHalfCircle(PDI, TopEnd, YAxis, ZAxis, Color,Scale* Radius);
	DrawHalfCircle(PDI, TopEnd, XAxis, ZAxis, Color, Scale*Radius);

	FVector NegZAxis = -ZAxis;

	DrawHalfCircle(PDI, BottomEnd, YAxis, NegZAxis, Color, Scale*Radius);
	DrawHalfCircle(PDI, BottomEnd, XAxis, NegZAxis, Color, Scale*Radius);

	// Draw connecty lines
	PDI->DrawLine(TopEnd + Scale*Radius*XAxis, BottomEnd + Scale*Radius*XAxis, Color, SDPG_World);
	PDI->DrawLine(TopEnd - Scale*Radius*XAxis, BottomEnd - Scale*Radius*XAxis, Color, SDPG_World);
	PDI->DrawLine(TopEnd + Scale*Radius*YAxis, BottomEnd + Scale*Radius*YAxis, Color, SDPG_World);
	PDI->DrawLine(TopEnd - Scale*Radius*YAxis, BottomEnd - Scale*Radius*YAxis, Color, SDPG_World);
}

void FKSphylElem::DrawElemSolid(FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, float Scale, const FMaterialRenderProxy* MaterialRenderProxy)
{
	const int32 NumSides = DrawCollisionSides;
	const int32 NumRings = (DrawCollisionSides/2) + 1;

	// The first/last arc are on top of each other.
	const int32 NumVerts = (NumSides+1) * (NumRings+1);
	FDynamicMeshVertex* Verts = (FDynamicMeshVertex*)FMemory::Malloc( NumVerts * sizeof(FDynamicMeshVertex) );

	// Calculate verts for one arc
	FDynamicMeshVertex* ArcVerts = (FDynamicMeshVertex*)FMemory::Malloc( (NumRings+1) * sizeof(FDynamicMeshVertex) );

	for(int32 RingIdx=0; RingIdx<NumRings+1; RingIdx++)
	{
		FDynamicMeshVertex* ArcVert = &ArcVerts[RingIdx];

		float Angle;
		float ZOffset;
		if( RingIdx <= DrawCollisionSides/4 )
		{
			Angle = ((float)RingIdx/(NumRings-1)) * PI;
			ZOffset = 0.5 * Scale * Length;
		}
		else
		{
			Angle = ((float)(RingIdx-1)/(NumRings-1)) * PI;
			ZOffset = -0.5 * Scale * Length;
		}

		// Note- unit sphere, so position always has mag of one. We can just use it for normal!		
		FVector SpherePos;
		SpherePos.X = 0.0f;
		SpherePos.Y = Scale * Radius * FMath::Sin(Angle);
		SpherePos.Z = Scale * Radius * FMath::Cos(Angle);

		ArcVert->Position = SpherePos + FVector(0,0,ZOffset);

		ArcVert->SetTangents(
			FVector(1,0,0),
			FVector(0.0f, -SpherePos.Z, SpherePos.Y),
			SpherePos
			);

		ArcVert->TextureCoordinate.X = 0.0f;
		ArcVert->TextureCoordinate.Y = ((float)RingIdx/NumRings);
	}

	// Then rotate this arc NumSides+1 times.
	for(int32 SideIdx=0; SideIdx<NumSides+1; SideIdx++)
	{
		const FRotator ArcRotator(0, 360.f * ((float)SideIdx/NumSides), 0);
		const FRotationMatrix ArcRot( ArcRotator );
		const float XTexCoord = ((float)SideIdx/NumSides);

		for(int32 VertIdx=0; VertIdx<NumRings+1; VertIdx++)
		{
			int32 VIx = (NumRings+1)*SideIdx + VertIdx;

			Verts[VIx].Position = ArcRot.TransformPosition( ArcVerts[VertIdx].Position );

			Verts[VIx].SetTangents(
				ArcRot.TransformVector( ArcVerts[VertIdx].TangentX ),
				ArcRot.TransformVector( ArcVerts[VertIdx].GetTangentY() ),
				ArcRot.TransformVector( ArcVerts[VertIdx].TangentZ )
				);

			Verts[VIx].TextureCoordinate.X = XTexCoord;
			Verts[VIx].TextureCoordinate.Y = ArcVerts[VertIdx].TextureCoordinate.Y;
		}
	}

	FDynamicMeshBuilder MeshBuilder;
	{
		// Add all of the vertices to the mesh.
		for(int32 VertIdx=0; VertIdx<NumVerts; VertIdx++)
		{
			MeshBuilder.AddVertex(Verts[VertIdx]);
		}

		// Add all of the triangles to the mesh.
		for(int32 SideIdx=0; SideIdx<NumSides; SideIdx++)
		{
			const int32 a0start = (SideIdx+0) * (NumRings+1);
			const int32 a1start = (SideIdx+1) * (NumRings+1);

			for(int32 RingIdx=0; RingIdx<NumRings; RingIdx++)
			{
				MeshBuilder.AddTriangle(a0start + RingIdx + 0, a1start + RingIdx + 0, a0start + RingIdx + 1);
				MeshBuilder.AddTriangle(a1start + RingIdx + 0, a1start + RingIdx + 1, a0start + RingIdx + 1);
			}
		}

	}
	MeshBuilder.Draw(PDI, ElemTM.ToMatrixWithScale(), MaterialRenderProxy, SDPG_World,0.f);


	FMemory::Free(Verts);
	FMemory::Free(ArcVerts);
}

/////////////////////////////////////////////////////////////////////////////////////
// FKConvexElem
/////////////////////////////////////////////////////////////////////////////////////

// NB: ElemTM is assumed to have no scaling in it!
void FKConvexElem::DrawElemWire(FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, const FVector& Scale3D, const FColor Color)
{
#if WITH_PHYSX

	FTransform LocalToWorld = ElemTM;
	LocalToWorld.SetScale3D(Scale3D);

	PxConvexMesh* Mesh = ConvexMesh;

	if(Mesh)
	{
		// Draw each triangle that makes up the convex hull
		PxU32 NbVerts = Mesh->getNbVertices();
		const PxVec3* Vertices = Mesh->getVertices();
		
		TArray<FVector> TransformedVerts;
		TransformedVerts.AddUninitialized(NbVerts);
		for(PxU32 i=0; i<NbVerts; i++)
		{
			TransformedVerts[i] = LocalToWorld.TransformPosition( P2UVector(Vertices[i]) );
		}
						
		const PxU8* PIndexBuffer = Mesh->getIndexBuffer();
		PxU32 NbPolygons = Mesh->getNbPolygons();

		for(PxU32 i=0;i<NbPolygons;i++)
		{
			PxHullPolygon Data;
			bool bStatus = Mesh->getPolygonData(i, Data);
			check(bStatus);

			const PxU8* PIndices = PIndexBuffer + Data.mIndexBase;
		
			for(PxU16 j=0;j<Data.mNbVerts;j++)
			{
				// Get the verts that make up this line.
				int32 I0 = PIndices[j];
				int32 I1 = PIndices[j+1];

				// Loop back last and first vertices
				if(j==Data.mNbVerts - 1)
				{
					I1 = PIndices[0];
				}

				PDI->DrawLine( TransformedVerts[I0], TransformedVerts[I1], Color, SDPG_World );
			}
		}
	}
	else
	{
		UE_LOG(LogPhysics, Log, TEXT("FKConvexElem::DrawElemWire : No ConvexMesh, so unable to draw."));
	}
#endif // WITH_PHYSX
}

void FKConvexElem::AddCachedSolidConvexGeom(TArray<FDynamicMeshVertex>& VertexBuffer, TArray<int32>& IndexBuffer, const FColor VertexColor)
{
#if WITH_PHYSX
	if(ConvexMesh)
	{

		int32 StartVertOffset = VertexBuffer.Num();

		// get PhysX data
		const PxVec3* PVertices = ConvexMesh->getVertices();
		const PxU8* PIndexBuffer = ConvexMesh->getIndexBuffer();
		PxU32 NbPolygons = ConvexMesh->getNbPolygons();

		for(PxU32 i=0;i<NbPolygons;i++)
		{
			PxHullPolygon Data;
			bool bStatus = ConvexMesh->getPolygonData(i, Data);
			check(bStatus);

			const PxU8* indices = PIndexBuffer + Data.mIndexBase;

			// create tangents from the first and second vertices of each polygon
			const FVector TangentX = P2UVector(PVertices[indices[1]]-PVertices[indices[0]]).SafeNormal();
			const FVector TangentZ = FVector(Data.mPlane[0], Data.mPlane[1], Data.mPlane[2]).SafeNormal();
			const FVector TangentY = (TangentX ^ TangentZ).SafeNormal();

			// add vertices 
			for(PxU32 j=0;j<Data.mNbVerts;j++)
			{
				int32 VertIndex = indices[j];

				FDynamicMeshVertex Vert1;
				Vert1.Position = P2UVector(PVertices[VertIndex]);
				Vert1.Color = VertexColor;
				Vert1.SetTangents(
					TangentX,
					TangentY,
					TangentZ
					);
				VertexBuffer.Add(Vert1);
			}

			// Add indices
			PxU32 nbTris = Data.mNbVerts - 2;
			for(PxU32 j=0;j<nbTris;j++)
			{
				IndexBuffer.Add(StartVertOffset+0);
				IndexBuffer.Add(StartVertOffset+j+2);
				IndexBuffer.Add(StartVertOffset+j+1);
			}

			StartVertOffset += Data.mNbVerts;
		}
	}
	else
	{
		UE_LOG(LogPhysics, Log, TEXT("FKConvexElem::AddCachedSolidConvexGeom : No ConvexMesh, so unable to draw."));
	}
#endif // WITH_PHYSX
}

/////////////////////////////////////////////////////////////////////////////////////
// FKAggregateGeom
/////////////////////////////////////////////////////////////////////////////////////


void FConvexCollisionVertexBuffer::InitRHI()
{
	VertexBufferRHI = RHICreateVertexBuffer(Vertices.Num() * sizeof(FDynamicMeshVertex),NULL,BUF_Static);

	// Copy the vertex data into the vertex buffer.
	void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI,0,Vertices.Num() * sizeof(FDynamicMeshVertex), RLM_WriteOnly);
	FMemory::Memcpy(VertexBufferData,Vertices.GetTypedData(),Vertices.Num() * sizeof(FDynamicMeshVertex));
	RHIUnlockVertexBuffer(VertexBufferRHI);
}

void FConvexCollisionIndexBuffer::InitRHI()
{
	IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32),Indices.Num() * sizeof(int32),NULL,BUF_Static);

	// Write the indices to the index buffer.
	void* Buffer = RHILockIndexBuffer(IndexBufferRHI,0,Indices.Num() * sizeof(int32),RLM_WriteOnly);
	FMemory::Memcpy(Buffer,Indices.GetTypedData(),Indices.Num() * sizeof(int32));
	RHIUnlockIndexBuffer(IndexBufferRHI);
}

void FConvexCollisionVertexFactory::InitConvexVertexFactory(const FConvexCollisionVertexBuffer* VertexBuffer)
{
	if(IsInRenderingThread())
	{
		// Initialize the vertex factory's stream components.
		DataType NewData;
		NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,Position,VET_Float3);
		NewData.TextureCoordinates.Add(
			FVertexStreamComponent(VertexBuffer,STRUCT_OFFSET(FDynamicMeshVertex,TextureCoordinate),sizeof(FDynamicMeshVertex),VET_Float2)
			);
		NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,TangentX,VET_PackedNormal);
		NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,TangentZ,VET_PackedNormal);
		SetData(NewData);
	}
	else
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			InitConvexCollisionVertexFactory,
			FConvexCollisionVertexFactory*,VertexFactory,this,
			const FConvexCollisionVertexBuffer*,VertexBuffer,VertexBuffer,
			{
				// Initialize the vertex factory's stream components.
				DataType NewData;
				NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,Position,VET_Float3);
				NewData.TextureCoordinates.Add(
					FVertexStreamComponent(VertexBuffer,STRUCT_OFFSET(FDynamicMeshVertex,TextureCoordinate),sizeof(FDynamicMeshVertex),VET_Float2)
					);
				NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,TangentX,VET_PackedNormal);
				NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,TangentZ,VET_PackedNormal);
				VertexFactory->SetData(NewData);
			});
	}
}

void FKAggregateGeom::DrawAggGeom(FPrimitiveDrawInterface* PDI, const FTransform& Transform, const FColor Color, const FMaterialRenderProxy* MatInst, bool bPerHullColor, bool bDrawSolid)
{
	const FVector Scale3D = Transform.GetScale3D();
	FTransform ParentTM = Transform;
	ParentTM.RemoveScaling();

	if( Scale3D.GetAbs().IsUniform() )
	{
		for(int32 i=0; i<SphereElems.Num(); i++)
		{
			FTransform ElemTM = SphereElems[i].GetTransform();
			ElemTM.ScaleTranslation(Scale3D);
			ElemTM *= ParentTM;

			if(bDrawSolid)
				SphereElems[i].DrawElemSolid(PDI, ElemTM, Scale3D.X, MatInst);
			else
				SphereElems[i].DrawElemWire(PDI, ElemTM, Scale3D.X, Color);
		}

		for(int32 i=0; i<BoxElems.Num(); i++)
		{
			FTransform ElemTM = BoxElems[i].GetTransform();
			ElemTM.ScaleTranslation(Scale3D);
			ElemTM *= ParentTM;

			if(bDrawSolid)
				BoxElems[i].DrawElemSolid(PDI, ElemTM, Scale3D.X, MatInst);
			else
				BoxElems[i].DrawElemWire(PDI, ElemTM, Scale3D.X, Color);
		}

		for(int32 i=0; i<SphylElems.Num(); i++)
		{
			FTransform ElemTM = SphylElems[i].GetTransform();
			ElemTM.ScaleTranslation(Scale3D);
			ElemTM *= ParentTM;

			if(bDrawSolid)
				SphylElems[i].DrawElemSolid(PDI, ElemTM, Scale3D.X, MatInst);
			else
				SphylElems[i].DrawElemWire(PDI, ElemTM, Scale3D.X, Color);
		}
	}

	if(ConvexElems.Num() > 0)
	{
		if(bDrawSolid)
		{
			// Cache collision vertex/index buffer
			if(!RenderInfo)
			{
				RenderInfo = new FKConvexGeomRenderInfo();
				RenderInfo->VertexBuffer = new FConvexCollisionVertexBuffer();
				RenderInfo->IndexBuffer = new FConvexCollisionIndexBuffer();

				for(int32 i=0; i<ConvexElems.Num(); i++)
				{
					// Get vertices/triangles from this hull.
					ConvexElems[i].AddCachedSolidConvexGeom(RenderInfo->VertexBuffer->Vertices, RenderInfo->IndexBuffer->Indices, FColor(255,255,255));
				}

				RenderInfo->VertexBuffer->InitResource();
				RenderInfo->IndexBuffer->InitResource();
                
				RenderInfo->CollisionVertexFactory = new FConvexCollisionVertexFactory(RenderInfo->VertexBuffer);
				RenderInfo->CollisionVertexFactory->InitResource();
			}

			// Calculate transform
			FTransform LocalToWorld = FTransform( FQuat::Identity, FVector::ZeroVector, Scale3D ) * ParentTM;

			// Draw the mesh.
			FMeshBatch Mesh;
			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			BatchElement.IndexBuffer = RenderInfo->IndexBuffer;
			Mesh.VertexFactory = RenderInfo->CollisionVertexFactory;
			Mesh.MaterialRenderProxy = MatInst;
			FBoxSphereBounds WorldBounds, LocalBounds;
			CalcBoxSphereBounds(WorldBounds, LocalToWorld);
			CalcBoxSphereBounds(LocalBounds, FTransform::Identity);
			BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(LocalToWorld.ToMatrixWithScale(), WorldBounds, LocalBounds, true);
			// previous l2w not used so treat as static
			BatchElement.FirstIndex = 0;
			BatchElement.NumPrimitives = RenderInfo->IndexBuffer->Indices.Num() / 3;
			BatchElement.MinVertexIndex = 0;
			BatchElement.MaxVertexIndex = RenderInfo->VertexBuffer->Vertices.Num() - 1;
			Mesh.ReverseCulling = LocalToWorld.GetDeterminant() < 0.0f ? true : false;
			Mesh.Type = PT_TriangleList;
			Mesh.DepthPriorityGroup = SDPG_World;
			PDI->DrawMesh(Mesh);
		}
		else
		{
			for(int32 i=0; i<ConvexElems.Num(); i++)
			{
				FColor ConvexColor = bPerHullColor ? DebugUtilColor[i%NUM_DEBUG_UTIL_COLORS] : Color;
				ConvexElems[i].DrawElemWire(PDI, ParentTM, Scale3D, ConvexColor);
			}
		}
	}
}

/** Release the RenderInfo (if its there) and safely clean up any resources. Call on the game thread. */
void FKAggregateGeom::FreeRenderInfo()
{
	// See if we have rendering resources to free
	if(RenderInfo)
	{
		// Should always have these if RenderInfo exists
		check(RenderInfo->VertexBuffer);
		check(RenderInfo->IndexBuffer);
		check(RenderInfo->CollisionVertexFactory);

		// Fire off commands to free these resources
		BeginReleaseResource(RenderInfo->VertexBuffer);
		BeginReleaseResource(RenderInfo->IndexBuffer);
		BeginReleaseResource(RenderInfo->CollisionVertexFactory);

		// Wait until those commands have been processed
		FRenderCommandFence Fence;
		Fence.BeginFence();
		Fence.Wait();

		// Release memory.
		delete RenderInfo->VertexBuffer;
		delete RenderInfo->IndexBuffer;
		delete RenderInfo->CollisionVertexFactory;
		delete RenderInfo;
		RenderInfo = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// UPhysicsAsset
/////////////////////////////////////////////////////////////////////////////////////

FTransform GetSkelBoneTransform(int32 BoneIndex, const TArray<FTransform>& SpaceBases, const FTransform& LocalToWorld)
{
	if(BoneIndex != INDEX_NONE && BoneIndex < SpaceBases.Num())
	{
		return SpaceBases[BoneIndex] * LocalToWorld;
	}
	else
	{
		return FTransform::Identity;
	}
}

void UPhysicsAsset::DrawCollision(FPrimitiveDrawInterface* PDI, const USkeletalMesh* SkelMesh, const TArray<FTransform>& SpaceBases, const FTransform& LocalToWorld, float Scale)
{
	for( int32 i=0; i<BodySetup.Num(); i++)
	{
		int32 BoneIndex = SkelMesh->RefSkeleton.FindBoneIndex( BodySetup[i]->BoneName );
		
		FColor* BoneColor = (FColor*)( &BodySetup[i] );

		FTransform BoneTransform = GetSkelBoneTransform(BoneIndex, SpaceBases, LocalToWorld);
		BoneTransform.SetScale3D(FVector(Scale));
		BodySetup[i]->CreatePhysicsMeshes();
		BodySetup[i]->AggGeom.DrawAggGeom( PDI, BoneTransform, *BoneColor, NULL, false, false );
	}
}

void UPhysicsAsset::DrawConstraints(FPrimitiveDrawInterface* PDI, const USkeletalMesh* SkelMesh, const TArray<FTransform>& SpaceBases, const FTransform& LocalToWorld, float Scale)
{
	for( int32 i=0; i<ConstraintSetup.Num(); i++ )
	{
		FConstraintInstance& Instance = ConstraintSetup[i]->DefaultInstance;

		// Get each constraint frame in world space.
		FTransform Con1Frame = FTransform::Identity;
		int32 Bone1Index = SkelMesh->RefSkeleton.FindBoneIndex(Instance.ConstraintBone1);
		if(Bone1Index != INDEX_NONE)
		{	
			FTransform Body1TM = GetSkelBoneTransform(Bone1Index, SpaceBases, LocalToWorld);
			Body1TM.RemoveScaling();
			Con1Frame = Instance.GetRefFrame(EConstraintFrame::Frame1) * Body1TM;
		}

		FTransform Con2Frame = FTransform::Identity;
		int32 Bone2Index = SkelMesh->RefSkeleton.FindBoneIndex(Instance.ConstraintBone2);
		if(Bone2Index != INDEX_NONE)
		{	
			FTransform Body2TM = GetSkelBoneTransform(Bone2Index, SpaceBases, LocalToWorld);
			Body2TM.RemoveScaling();
			Con2Frame = Instance.GetRefFrame(EConstraintFrame::Frame1) * Body2TM;
		}


		Instance.DrawConstraint(PDI, Scale, 1.f, true, true, Con1Frame, Con2Frame, false);
	}
}

static void DrawOrientedStar(FPrimitiveDrawInterface* PDI, const FTransform& Transform, float Size, const FColor Color)
{
	FVector Position = Transform.GetTranslation();

	PDI->DrawLine(Position + Size * Transform.GetScaledAxis( EAxis::X ), Position - Size * Transform.GetScaledAxis( EAxis::X ), Color, SDPG_World);
	PDI->DrawLine(Position + Size * Transform.GetScaledAxis( EAxis::Y ), Position - Size * Transform.GetScaledAxis( EAxis::Y ), Color, SDPG_World);
	PDI->DrawLine(Position + Size * Transform.GetScaledAxis( EAxis::Z ), Position - Size * Transform.GetScaledAxis( EAxis::Z ), Color, SDPG_World);
}

static void DrawLinearLimit(FPrimitiveDrawInterface* PDI, const FVector& Origin, const FVector& Axis, const FVector& Orth, float LinearLimitRadius, bool bLinearLimited, float DrawScale)
{
	float ScaledLimitSize = LimitRenderSize * DrawScale;

	if(bLinearLimited)
	{
		FVector Start = Origin - LinearLimitRadius * Axis;
		FVector End = Origin + LinearLimitRadius * Axis;

		PDI->DrawLine(  Start, End, JointLimitColor, SDPG_World );

		// Draw ends indicating limit.
		PDI->DrawLine(  Start - (0.2f * ScaledLimitSize * Orth), Start + (0.2f * ScaledLimitSize * Orth), JointLimitColor, SDPG_World );
		PDI->DrawLine(  End - (0.2f * ScaledLimitSize * Orth), End + (0.2f * ScaledLimitSize * Orth), JointLimitColor, SDPG_World );
	}
	else
	{
		FVector Start = Origin - 1.5f * ScaledLimitSize * Axis;
		FVector End = Origin + 1.5f * ScaledLimitSize * Axis;

		PDI->DrawLine(  Start, End, JointRefColor, SDPG_World );

		// Draw arrow heads.
		PDI->DrawLine(  Start, Start + (0.2f * ScaledLimitSize * Axis) + (0.2f * ScaledLimitSize * Orth), JointLimitColor, SDPG_World );
		PDI->DrawLine(  Start, Start + (0.2f * ScaledLimitSize * Axis) - (0.2f * ScaledLimitSize * Orth), JointLimitColor, SDPG_World );

		PDI->DrawLine(  End, End - (0.2f * ScaledLimitSize * Axis) + (0.2f * ScaledLimitSize * Orth), JointLimitColor, SDPG_World );
		PDI->DrawLine(  End, End - (0.2f * ScaledLimitSize * Axis) - (0.2f * ScaledLimitSize * Orth), JointLimitColor, SDPG_World );
	}
}

void FConstraintInstance::DrawConstraint(	FPrimitiveDrawInterface* PDI, 
											float Scale, float LimitDrawScale, bool bDrawLimits, bool bDrawSelected,
											const FTransform& Con1Frame, const FTransform& Con2Frame, bool bDrawAsPoint ) const
{
	UMaterialInterface* LimitMaterial = GEngine->ConstraintLimitMaterial;

	FVector Con1Pos = Con1Frame.GetTranslation();
	FVector Con2Pos = Con2Frame.GetTranslation();

	float ScaledLimitSize = LimitRenderSize * LimitDrawScale;

	// Special mode for drawing joints just as points..
	if(bDrawAsPoint)
	{
		if(bDrawSelected)
		{
			PDI->DrawPoint( Con1Frame.GetTranslation(), JointFrame1Color, 3.f, SDPG_World );
			PDI->DrawPoint( Con2Frame.GetTranslation(), JointFrame2Color, 3.f, SDPG_World );
		}
		else
		{
			PDI->DrawPoint( Con1Frame.GetTranslation(), JointUnselectedColor, 3.f, SDPG_World );
			PDI->DrawPoint( Con2Frame.GetTranslation(), JointUnselectedColor, 3.f, SDPG_World );
		}

		// do nothing else in this mode.
		return;
	}

	if(bDrawSelected)
	{
		DrawOrientedStar( PDI, Con1Frame, LimitDrawScale * JointRenderSize, JointFrame1Color );
		DrawOrientedStar( PDI, Con2Frame, LimitDrawScale * JointRenderSize, JointFrame2Color );
	}
	else
	{
		DrawOrientedStar( PDI, Con1Frame, LimitDrawScale * JointRenderSize, JointUnselectedColor );
		DrawOrientedStar( PDI, Con2Frame, LimitDrawScale * JointRenderSize, JointUnselectedColor );
	}

	//////////////////////////////////////////////////////////////////////////
	// LINEAR DRAWING

	bool bLinearXLocked = (LinearXMotion == LCM_Locked) || (LinearXMotion == LCM_Limited && LinearLimitSize < RB_MinSizeToLockDOF);
	bool bLinearYLocked = (LinearYMotion == LCM_Locked) || (LinearYMotion == LCM_Limited && LinearLimitSize < RB_MinSizeToLockDOF);
	bool bLinearZLocked = (LinearZMotion == LCM_Locked) || (LinearZMotion == LCM_Limited && LinearLimitSize < RB_MinSizeToLockDOF);

	if(!bLinearXLocked)
	{
		bool bLinearXLimited = ( LinearXMotion == LCM_Limited && LinearLimitSize >= RB_MinSizeToLockDOF );
		DrawLinearLimit(PDI, Con2Frame.GetTranslation(), Con2Frame.GetScaledAxis( EAxis::X ), Con2Frame.GetScaledAxis( EAxis::Z ), LinearLimitSize, bLinearXLimited, LimitDrawScale);
	}

	if(!bLinearYLocked)
	{
		bool bLinearYLimited = ( LinearYMotion == LCM_Limited && LinearLimitSize >= RB_MinSizeToLockDOF );
		DrawLinearLimit(PDI, Con2Frame.GetTranslation(), Con2Frame.GetScaledAxis( EAxis::Y ), Con2Frame.GetScaledAxis( EAxis::Z ), LinearLimitSize, bLinearYLimited, LimitDrawScale);
	}

	if(!bLinearZLocked)
	{
		bool bLinearZLimited = ( LinearZMotion == LCM_Limited && LinearLimitSize >= RB_MinSizeToLockDOF );
		DrawLinearLimit(PDI, Con2Frame.GetTranslation(), Con2Frame.GetScaledAxis( EAxis::Z ), Con2Frame.GetScaledAxis( EAxis::X ), LinearLimitSize, bLinearZLimited, LimitDrawScale);
	}


	if(!bDrawLimits)
		return;


	//////////////////////////////////////////////////////////////////////////
	// ANGULAR DRAWING

	bool bLockTwist = AngularTwistMotion == ACM_Locked; 
	bool bLockSwing1 = AngularSwing1Motion == ACM_Locked; 
	bool bLockSwing2 = AngularSwing2Motion == ACM_Locked; 
	bool bLockAllSwing = bLockSwing1 && bLockSwing2;

	bool bDrawnAxisLine = false;
	FVector RefLineEnd = Con1Frame.GetTranslation() + (1.2f * ScaledLimitSize * Con1Frame.GetScaledAxis( EAxis::X ));

	//If swing1 locked
	if(bLockSwing1)
	{
		// Draw little red 'V' to indicate locked swing.
		PDI->DrawLine( Con2Frame.GetTranslation(), Con2Frame.TransformPosition( 0.3f * ScaledLimitSize * FVector(1,1,0) ), JointLockedColor, SDPG_World);
		PDI->DrawLine( Con2Frame.GetTranslation(), Con2Frame.TransformPosition( 0.3f * ScaledLimitSize * FVector(1,-1,0) ), JointLockedColor, SDPG_World);
	}

	//If swing2 locked
	if(bLockSwing2)
	{
		// Draw little red 'V' to indicate locked swing.
		PDI->DrawLine( Con2Frame.GetTranslation(), Con2Frame.TransformPosition( 0.3f * ScaledLimitSize * FVector(1,0,1) ), JointLockedColor, SDPG_World);
		PDI->DrawLine( Con2Frame.GetTranslation(), Con2Frame.TransformPosition( 0.3f * ScaledLimitSize * FVector(1,0,-1) ), JointLockedColor, SDPG_World);
	}
	
	// If swing is limited (but not locked) - draw the limit cone.
	if(AngularSwing1Motion==ACM_Limited||AngularSwing2Motion==ACM_Limited)
	{
		FTransform ConeLimitTM = Con2Frame;
		ConeLimitTM.SetTranslation( Con1Frame.GetTranslation() );

		float ang1 = Swing1LimitAngle;
		if(bLockSwing1)
			ang1 = 0.f;
		else
			ang1 *= ((float)PI/180.f); // convert to radians

		float ang2 = Swing2LimitAngle;
		if(bLockSwing2)
			ang2 = 0.f;
		else
			ang2 *= ((float)PI/180.f);

		FMatrix ConeToWorld = FScaleMatrix( FVector(ScaledLimitSize) ) * ConeLimitTM.ToMatrixWithScale();
		DrawCone(PDI, ConeToWorld, ang1, ang2, DrawConeLimitSides, true, JointLimitColor, LimitMaterial->GetRenderProxy(false), SDPG_World );

		// Draw reference line
		PDI->DrawLine( Con1Frame.GetTranslation(), RefLineEnd, JointRefColor, SDPG_World );
		bDrawnAxisLine = true;
	}

	//Handle the twist constraint draw
	if(bLockTwist)
	{
		// If there is no axis line draw already - add a little one now to sit the 'twist locked' cross on
		if(!bDrawnAxisLine)
			PDI->DrawLine( Con1Frame.GetTranslation(), RefLineEnd, JointLockedColor, SDPG_World );

		PDI->DrawLine(  RefLineEnd + Con1Frame.TransformVector( 0.3f * ScaledLimitSize * FVector(0.f,-0.5f,-0.5f) ), 
			RefLineEnd + Con1Frame.TransformVector( 0.3f * ScaledLimitSize * FVector(0.f, 0.5f, 0.5f) ), JointLockedColor, SDPG_World);

		PDI->DrawLine(  RefLineEnd + Con1Frame.TransformVector( 0.3f * ScaledLimitSize * FVector(0.f, 0.5f,-0.5f) ), 
			RefLineEnd + Con1Frame.TransformVector( 0.3f * ScaledLimitSize * FVector(0.f,-0.5f, 0.5f) ), JointLockedColor, SDPG_World);
	}
	else if(AngularTwistMotion==ACM_Limited)
	{
		// If twist is limited, but not completely locked, draw 
		// If no axis yet drawn - do it now
		if(!bDrawnAxisLine)
			PDI->DrawLine( Con1Frame.GetTranslation(), RefLineEnd, JointRefColor, SDPG_World );

		// Draw twist limit.
		FVector ChildTwistRef = Con1Frame.GetScaledAxis( EAxis::Y );
		FVector ParentTwistRef = Con2Frame.GetScaledAxis( EAxis::Y );
		PDI->DrawLine( RefLineEnd, RefLineEnd + ChildTwistRef * ScaledLimitSize, JointRefColor, SDPG_World );

		// Rotate parent twist ref axis
		FQuat ParentToChildRot = FQuat::FindBetween( Con2Frame.GetScaledAxis( EAxis::X ), Con1Frame.GetScaledAxis( EAxis::X ) );
		FVector ChildTwistLimit = ParentToChildRot.RotateVector( ParentTwistRef );

		FQuat RotateLimit = FQuat( Con1Frame.GetScaledAxis( EAxis::X ), TwistLimitAngle * (PI/180.0f) );
		FVector WLimit = RotateLimit.RotateVector(ChildTwistLimit);
		PDI->DrawLine( RefLineEnd, RefLineEnd + WLimit * ScaledLimitSize, JointLimitColor, SDPG_World );

		RotateLimit = FQuat( Con1Frame.GetScaledAxis( EAxis::X ), -TwistLimitAngle * (PI/180.0f) );
		WLimit = RotateLimit.RotateVector(ChildTwistLimit);
		PDI->DrawLine( RefLineEnd, RefLineEnd + WLimit * ScaledLimitSize, JointLimitColor, SDPG_World );

		DrawArc(PDI, RefLineEnd, ChildTwistLimit, -ChildTwistLimit ^ Con1Frame.GetScaledAxis( EAxis::X ), -TwistLimitAngle, TwistLimitAngle, 0.8f * ScaledLimitSize, 8, JointLimitColor, SDPG_World);
	}
	else
	{
		// For convenience, in the hinge case where swing is locked and twist is unlimited, draw the twist axis.
		if(bLockAllSwing)
			PDI->DrawLine(  Con2Frame.GetTranslation() - ScaledLimitSize * Con2Frame.GetScaledAxis( EAxis::X ), 
							Con2Frame.GetTranslation() + ScaledLimitSize * Con2Frame.GetScaledAxis( EAxis::X ), JointLimitColor, SDPG_World );
	}


}
