// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "LevelUtils.h"
#include "LocalVertexFactory.h"

/** A material sprite vertex. */
struct FMaterialSpriteVertex
{
	FVector Position;
	FPackedNormal TangentX;
	FPackedNormal TangentZ;
	FLinearColor Color;
	FVector2D TexCoords;
};

/** A dummy vertex buffer used to give the FMaterialSpriteVertexFactory something to reference as a stream source. */
class FMaterialSpriteVertexBuffer : public FVertexBuffer
{
public:

	virtual void InitRHI()
	{
		VertexBufferRHI = RHICreateVertexBuffer(sizeof(FMaterialSpriteVertex),NULL,BUF_Static);
	}
};
static TGlobalResource<FMaterialSpriteVertexBuffer> GDummyMaterialSpriteVertexBuffer;

/** The vertex factory used to draw material sprites. */
class FMaterialSpriteVertexFactory : public FLocalVertexFactory
{
public:

	FMaterialSpriteVertexFactory()
	{
		FLocalVertexFactory::DataType Data;

		Data.PositionComponent = FVertexStreamComponent(
			&GDummyMaterialSpriteVertexBuffer,
			STRUCT_OFFSET(FMaterialSpriteVertex,Position),
			sizeof(FMaterialSpriteVertex),
			VET_Float3
			);
		Data.TangentBasisComponents[0] = FVertexStreamComponent(
			&GDummyMaterialSpriteVertexBuffer,
			STRUCT_OFFSET(FMaterialSpriteVertex,TangentX),
			sizeof(FMaterialSpriteVertex),
			VET_PackedNormal
			);
		Data.TangentBasisComponents[1] = FVertexStreamComponent(
			&GDummyMaterialSpriteVertexBuffer,
			STRUCT_OFFSET(FMaterialSpriteVertex,TangentZ),
			sizeof(FMaterialSpriteVertex),
			VET_PackedNormal
			);
		Data.ColorComponent = FVertexStreamComponent(
			&GDummyMaterialSpriteVertexBuffer,
			STRUCT_OFFSET(FMaterialSpriteVertex,Color),
			sizeof(FMaterialSpriteVertex),
			VET_Float4
			);
		Data.TextureCoordinates.Empty();
		Data.TextureCoordinates.Add(FVertexStreamComponent(
			&GDummyMaterialSpriteVertexBuffer,
			STRUCT_OFFSET(FMaterialSpriteVertex,TexCoords),
			sizeof(FMaterialSpriteVertex),
			VET_Float2
			));

		SetData(Data);
	}
};
static TGlobalResource<FMaterialSpriteVertexFactory> GMaterialSpriteVertexFactory;

/** Represents a sprite to the scene manager. */
class FMaterialSpriteSceneProxy : public FPrimitiveSceneProxy
{
public:

	/** Initialization constructor. */
	FMaterialSpriteSceneProxy(const UMaterialBillboardComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
	, Elements(InComponent->Elements)
	, BaseColor(FLinearColor::White)
	, LevelColor(255,255,255)
	, PropertyColor(255,255,255)
	{
		AActor* Owner = InComponent->GetOwner();
		if (Owner)
		{
			// Level colorization
			ULevel* Level = Owner->GetLevel();
			ULevelStreaming* LevelStreaming = FLevelUtils::FindStreamingLevel( Level );
			if ( LevelStreaming )
			{
				// Selection takes priority over level coloration.
				LevelColor = LevelStreaming->DrawColor;
			}
		}

		for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
		{
			UMaterialInterface* Material = Elements[ElementIndex].Material;
			if (Material)
			{
				MaterialRelevance |= Material->GetRelevance();
			}
		}

		GEngine->GetPropertyColorationColor( (UObject*)InComponent, PropertyColor );
	}

	// FPrimitiveSceneProxy interface.
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View)
	{
		QUICK_SCOPE_CYCLE_COUNTER( STAT_MaterialSpriteSceneProxy_DrawDynamicElements );

		const bool bIsWireframe = View->Family->EngineShowFlags.Wireframe;
		// Determine the position of the source
		const FVector SourcePosition = GetLocalToWorld().GetOrigin();
		const FVector CameraToSource = View->ViewMatrices.ViewOrigin - SourcePosition;
		const float DistanceToSource = CameraToSource.Size();

		const FVector CameraUp      = -View->InvViewProjectionMatrix.TransformVector(FVector(1.0f,0.0f,0.0f));
		const FVector CameraRight   = -View->InvViewProjectionMatrix.TransformVector(FVector(0.0f,1.0f,0.0f));
		const FVector CameraForward = -View->InvViewProjectionMatrix.TransformVector(FVector(0.0f,0.0f,1.0f));
		const FMatrix WorldToLocal = GetLocalToWorld().Inverse();
		const FVector LocalCameraUp = WorldToLocal.TransformVector(CameraUp);
		const FVector LocalCameraRight = WorldToLocal.TransformVector(CameraRight);
		const FVector LocalCameraForward = WorldToLocal.TransformVector(CameraForward);

		// Draw the elements ordered so the last is on top of the first.
		for(int32 ElementIndex = 0;ElementIndex < Elements.Num();++ElementIndex)
		{
			const FMaterialSpriteElement& Element = Elements[ElementIndex];
			if(Element.Material)
			{
				// Evaluate the size of the sprite.
				float SizeX = Element.BaseSizeX;
				float SizeY = Element.BaseSizeY;
				if(Element.DistanceToSizeCurve)
				{
					const float SizeFactor = Element.DistanceToSizeCurve->GetFloatValue(DistanceToSource);
					SizeX *= SizeFactor;
					SizeY *= SizeFactor;
				}
						
				// Convert the size into world-space.
				const float W = View->ViewProjectionMatrix.TransformPosition(SourcePosition).W;
				const float AspectRatio = CameraRight.Size() / CameraUp.Size();
				const float WorldSizeX = Element.bSizeIsInScreenSpace ? (SizeX               * W) : (SizeX / CameraRight.Size());
				const float WorldSizeY = Element.bSizeIsInScreenSpace ? (SizeY * AspectRatio * W) : (SizeY / CameraUp.Size());
			
				// Evaluate the color/opacity of the sprite.
				FLinearColor Color = BaseColor;
				if(Element.DistanceToOpacityCurve)
				{
					Color.A *= Element.DistanceToOpacityCurve->GetFloatValue(DistanceToSource);
				}

				// Set up the sprite vertex attributes that are constant across the sprite.
				FMaterialSpriteVertex Vertices[4];
				for(uint32 VertexIndex = 0;VertexIndex < 4;++VertexIndex)
				{
					Vertices[VertexIndex].Color = Color;
					Vertices[VertexIndex].TangentX = FPackedNormal(LocalCameraRight.SafeNormal());
					Vertices[VertexIndex].TangentZ = FPackedNormal(-LocalCameraForward.SafeNormal());
				}

				// Set up the sprite vertex positions and texture coordinates.
				Vertices[0].Position  = -WorldSizeX * LocalCameraRight + +WorldSizeY * LocalCameraUp;
				Vertices[0].TexCoords = FVector2D(0,0);
				Vertices[1].Position  = +WorldSizeX * LocalCameraRight + +WorldSizeY * LocalCameraUp;
				Vertices[1].TexCoords = FVector2D(1,0);
				Vertices[2].Position  = -WorldSizeX * LocalCameraRight + -WorldSizeY * LocalCameraUp;
				Vertices[2].TexCoords = FVector2D(0,1);
				Vertices[3].Position  = +WorldSizeX * LocalCameraRight + -WorldSizeY * LocalCameraUp;
				Vertices[3].TexCoords = FVector2D(1,1);
			
				// Set up the FMeshElement.
				FMeshBatch Mesh;
				Mesh.UseDynamicData      = true;
				Mesh.DynamicVertexData   = Vertices;
				Mesh.DynamicVertexStride = sizeof(FMaterialSpriteVertex);

				Mesh.VertexFactory           = &GMaterialSpriteVertexFactory;
				Mesh.MaterialRenderProxy     = Element.Material->GetRenderProxy((View->Family->EngineShowFlags.Selection) && IsSelected(),IsHovered());
				Mesh.LCI                     = NULL;
				Mesh.ReverseCulling          = IsLocalToWorldDeterminantNegative() ? true : false;
				Mesh.CastShadow              = false;
				Mesh.DepthPriorityGroup      = (ESceneDepthPriorityGroup)GetDepthPriorityGroup(View);
				Mesh.Type                    = PT_TriangleStrip;
				Mesh.bDisableBackfaceCulling = true;

				// Set up the FMeshBatchElement.
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer            = NULL;
				BatchElement.DynamicIndexData       = NULL;
				BatchElement.DynamicIndexStride     = 0;
				BatchElement.FirstIndex             = 0;
				BatchElement.MinVertexIndex         = 0;
				BatchElement.MaxVertexIndex         = 3;
				BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
				BatchElement.NumPrimitives          = 2;

				// Draw the sprite.
				DrawRichMesh(
					PDI, 
					Mesh, 
					FLinearColor(1.0f, 0.0f, 0.0f),
					LevelColor,
					PropertyColor,
					this,
					IsSelected(),
					bIsWireframe
					);
			}
		}
	}
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		bool bVisible = View->Family->EngineShowFlags.BillboardSprites;
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bDynamicRelevance = true;
		Result.bShadowRelevance = IsShadowCast(View);
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}
	virtual bool CanBeOccluded() const { return !MaterialRelevance.bDisableDepthTest; }
	virtual uint32 GetMemoryFootprint() const { return sizeof(*this) + GetAllocatedSize(); }
	uint32 GetAllocatedSize() const { return FPrimitiveSceneProxy::GetAllocatedSize(); }

private:
	TArray<FMaterialSpriteElement> Elements;
	FMaterialRelevance MaterialRelevance;
	FColor BaseColor;
	FColor LevelColor;
	FColor PropertyColor;
};

UMaterialBillboardComponent::UMaterialBillboardComponent(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	BodyInstance.bEnableCollision_DEPRECATED = false;

	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

FPrimitiveSceneProxy* UMaterialBillboardComponent::CreateSceneProxy()
{
	return new FMaterialSpriteSceneProxy(this);
}

FBoxSphereBounds UMaterialBillboardComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	const float BoundsSize = 32.0f;
	return FBoxSphereBounds(LocalToWorld.GetLocation(),FVector(BoundsSize,BoundsSize,BoundsSize),FMath::Sqrt(3.0f * FMath::Square(BoundsSize)));
}

void UMaterialBillboardComponent::AddElement(
	class UMaterialInterface* Material,
	UCurveFloat* DistanceToOpacityCurve,
	bool bSizeIsInScreenSpace,
	float BaseSizeX,
	float BaseSizeY,
	UCurveFloat* DistanceToSizeCurve
	)
{
	FMaterialSpriteElement* Element = new(Elements) FMaterialSpriteElement;
	Element->Material = Material;
	Element->DistanceToOpacityCurve = DistanceToOpacityCurve;
	Element->bSizeIsInScreenSpace = bSizeIsInScreenSpace;
	Element->BaseSizeX = BaseSizeX;
	Element->BaseSizeY = BaseSizeY;
	Element->DistanceToSizeCurve = DistanceToSizeCurve;
}
