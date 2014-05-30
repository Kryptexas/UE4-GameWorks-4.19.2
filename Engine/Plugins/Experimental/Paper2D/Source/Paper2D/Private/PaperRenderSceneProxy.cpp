// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperRenderSceneProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "Rendering/PaperBatchManager.h"
#include "Rendering/PaperBatchSceneProxy.h"
#include "PhysicsEngine/BodySetup2D.h"

static FPackedNormal PackedNormalX(FVector(1.0f, 0.0f, 0.0f));
static FPackedNormal PackedNormalY(FVector(0.0f, 1.0f, 0.0f));

/** A material sprite vertex. */
struct FPaperSpriteVertex
{
	FVector Position;
	FPackedNormal TangentX;
	FPackedNormal TangentZ;
	FLinearColor Color;
	FVector2D TexCoords;

	FPaperSpriteVertex() {}

	FPaperSpriteVertex(const FVector& InPosition, const FVector2D& InTextureCoordinate, const FLinearColor& InColor)
		: Position(InPosition)
		, TangentX(PackedNormalX)
		, TangentZ(PackedNormalY)
		, Color(InColor)
		, TexCoords(InTextureCoordinate)
	{}
};

/** A dummy vertex buffer used to give the FMaterialSpriteVertexFactory something to reference as a stream source. */
class FPaperSpriteVertexBuffer : public FVertexBuffer
{
public:
	virtual void InitRHI() OVERRIDE
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(sizeof(FPaperSpriteVertex), BUF_Static, CreateInfo);
	}
};
static TGlobalResource<FPaperSpriteVertexBuffer> GDummyMaterialSpriteVertexBuffer;

/** The vertex factory used to draw paper sprites. */
class FPaperSpriteVertexFactory : public FLocalVertexFactory
{
public:
	void AllocateStuff()
	{
		FLocalVertexFactory::DataType Data;

		Data.PositionComponent = FVertexStreamComponent(
			&GDummyMaterialSpriteVertexBuffer,
			STRUCT_OFFSET(FPaperSpriteVertex,Position),
			sizeof(FPaperSpriteVertex),
			VET_Float3
			);
		Data.TangentBasisComponents[0] = FVertexStreamComponent(
			&GDummyMaterialSpriteVertexBuffer,
			STRUCT_OFFSET(FPaperSpriteVertex,TangentX),
			sizeof(FPaperSpriteVertex),
			VET_PackedNormal
			);
		Data.TangentBasisComponents[1] = FVertexStreamComponent(
			&GDummyMaterialSpriteVertexBuffer,
			STRUCT_OFFSET(FPaperSpriteVertex,TangentZ),
			sizeof(FPaperSpriteVertex),
			VET_PackedNormal
			);
		Data.ColorComponent = FVertexStreamComponent(
			&GDummyMaterialSpriteVertexBuffer,
			STRUCT_OFFSET(FPaperSpriteVertex,Color),
			sizeof(FPaperSpriteVertex),
			VET_Float4
			);
		Data.TextureCoordinates.Empty();
		Data.TextureCoordinates.Add(FVertexStreamComponent(
			&GDummyMaterialSpriteVertexBuffer,
			STRUCT_OFFSET(FPaperSpriteVertex,TexCoords),
			sizeof(FPaperSpriteVertex),
			VET_Float2
			));

		SetData(Data);
	}

	FPaperSpriteVertexFactory()
	{
		AllocateStuff();
	}
};

//@TODO: Figure out how to do this properly at module load time
//static TGlobalResource<FPaperSpriteVertexFactory> GPaperSpriteVertexFactory;

//////////////////////////////////////////////////////////////////////////
// FTextureOverrideRenderProxy

/**
 * A material render proxy which overrides a named texture parameter.
 */
class FTextureOverrideRenderProxy : public FDynamicPrimitiveResource, public FMaterialRenderProxy
{
public:
	const FMaterialRenderProxy* const Parent;
	const UTexture* Texture;
	FName TextureParameterName;

	/** Initialization constructor. */
	FTextureOverrideRenderProxy(const FMaterialRenderProxy* InParent, const UTexture* InTexture, const FName& InParameterName)
		: Parent(InParent)
		, Texture(InTexture)
		, TextureParameterName(InParameterName)
	{}

	virtual ~FTextureOverrideRenderProxy()
	{
	}

	// FDynamicPrimitiveResource interface.
	virtual void InitPrimitiveResource()
	{
	}
	virtual void ReleasePrimitiveResource()
	{
		delete this;
	}

	// FMaterialRenderProxy interface.
	virtual const FMaterial* GetMaterial(ERHIFeatureLevel::Type InFeatureLevel) const OVERRIDE
	{
		return Parent->GetMaterial(InFeatureLevel);
	}

	virtual bool GetVectorValue(const FName ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const OVERRIDE
	{
		return Parent->GetVectorValue(ParameterName, OutValue, Context);
	}

	virtual bool GetScalarValue(const FName ParameterName, float* OutValue, const FMaterialRenderContext& Context) const OVERRIDE
	{
		return Parent->GetScalarValue(ParameterName, OutValue, Context);
	}

	virtual bool GetTextureValue(const FName ParameterName,const UTexture** OutValue, const FMaterialRenderContext& Context) const OVERRIDE
	{
		if (ParameterName == TextureParameterName)
		{
			*OutValue = Texture;
			return true;
		}
		else
		{
			return Parent->GetTextureValue(ParameterName, OutValue, Context);
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// FPaperRenderSceneProxy

FPaperRenderSceneProxy::FPaperRenderSceneProxy(const UPrimitiveComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
	, Material(NULL)
	, Owner(InComponent->GetOwner())
	, SourceSprite(NULL)
	, WireframeColor(FLinearColor::White)
	, CollisionResponse(InComponent->GetCollisionResponseToChannels())
{
	if (const UPaperRenderComponent* RenderComp = Cast<const UPaperRenderComponent>(InComponent))
	{
		WireframeColor = RenderComp->GetWireframeColor();

		SourceSprite = RenderComp->SourceSprite; //@TODO: This is totally not threadsafe, and won't keep up to date if the actor's sprite changes, etc....
		if (SourceSprite)
		{
			Material = SourceSprite->GetDefaultMaterial();
		}
		
		if (RenderComp->MaterialOverride)
		{
			Material = RenderComp->MaterialOverride;
		}

		if (Material)
		{
			MaterialRelevance = Material->GetRelevance();
		}
	}
}

FPaperRenderSceneProxy::~FPaperRenderSceneProxy()
{
#if TEST_BATCHING
	if (FPaperBatchSceneProxy* Batcher = FPaperBatchManager::GetBatcher(GetScene()))
	{
		Batcher->UnregisterManagedProxy(this);
	}
#endif
}

void FPaperRenderSceneProxy::CreateRenderThreadResources()
{
#if TEST_BATCHING
	if (FPaperBatchSceneProxy* Batcher = FPaperBatchManager::GetBatcher(GetScene()))
	{
		Batcher->RegisterManagedProxy(this);
	}
#endif
}

void FPaperRenderSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_PaperRenderSceneProxy_DrawDynamicElements);

	checkSlow(IsInRenderingThread());

	if (SourceSprite != NULL)//const UPaperRenderComponent* TypedFoo = Cast<const UPaperRenderComponent>(OwnerComponent))
	{
		// Show 3D physics
		if ((View->Family->EngineShowFlags.Collision /*@TODO: && bIsCollisionEnabled*/) && AllowDebugViewmodes())
		{
			if (UBodySetup2D* BodySetup2D = Cast<UBodySetup2D>(SourceSprite->BodySetup))
			{
				//@TODO: Draw 2D debugging geometry
			}
			else if (UBodySetup* BodySetup = SourceSprite->BodySetup)
			{
				if (FMath::Abs(GetLocalToWorld().Determinant()) < SMALL_NUMBER)
				{
					// Catch this here or otherwise GeomTransform below will assert
					// This spams so commented out
					//UE_LOG(LogStaticMesh, Log, TEXT("Zero scaling not supported (%s)"), *StaticMesh->GetPathName());
				}
				else
				{
					// Make a material for drawing solid collision stuff
					const UMaterial* LevelColorationMaterial = View->Family->EngineShowFlags.Lighting 
						? GEngine->ShadedLevelColorationLitMaterial : GEngine->ShadedLevelColorationUnlitMaterial;

					const FColoredMaterialRenderProxy CollisionMaterialInstance(
						LevelColorationMaterial->GetRenderProxy(IsSelected(), IsHovered()),
						WireframeColor
						);

					// Draw the static mesh's body setup.

					// Get transform without scaling.
					FTransform GeomTransform(GetLocalToWorld());

					// In old wireframe collision mode, always draw the wireframe highlighted (selected or not).
					bool bDrawWireSelected = IsSelected();
					if (View->Family->EngineShowFlags.Collision)
					{
						bDrawWireSelected = true;
					}

					// Differentiate the color based on bBlockNonZeroExtent.  Helps greatly with skimming a level for optimization opportunities.
					const FColor CollisionColor = FColor(220,149,223,255);

					const bool bUseSeparateColorPerHull = (Owner == NULL);
					const bool bDrawSolid = false;
					BodySetup->AggGeom.DrawAggGeom(PDI, GeomTransform, GetSelectionColor(CollisionColor, bDrawWireSelected, IsHovered()), &CollisionMaterialInstance, bUseSeparateColorPerHull, bDrawSolid);
				}
			}
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (View->Family->EngineShowFlags.Paper2DSprites)
	{
		RenderBounds(PDI, View->Family->EngineShowFlags, GetBounds(), (Owner == NULL) || IsSelected());
	}
#endif

	// Set the selection/hover color from the current engine setting.
	FLinearColor OverrideColor;
	bool bUseOverrideColor = false;

	const bool bShowAsSelected = !(GIsEditor && View->Family->EngineShowFlags.Selection) || IsSelected();
	if (bShowAsSelected || IsHovered())
	{
		bUseOverrideColor = true;
		OverrideColor = GetSelectionColor(FLinearColor::White, bShowAsSelected, IsHovered());
	}

	bUseOverrideColor = false;

	// Sprites of locked actors draw in red.
	//FLinearColor LevelColorToUse = IsSelected() ? ColorToUse : (FLinearColor)LevelColor;
	//FLinearColor PropertyColorToUse = PropertyColor;

#if TEST_BATCHING
#else
	DrawDynamicElements_RichMesh(PDI, View, bUseOverrideColor, OverrideColor);
#endif
}

void FPaperRenderSceneProxy::DrawDynamicElements_RichMesh(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor)
{
	if (Material == nullptr)
	{
		return;
	}

	const uint8 DPG = GetDepthPriorityGroup(View);
	
	static TGlobalResource<FPaperSpriteVertexFactory> GPaperSpriteVertexFactory;

	for (int32 BatchIndex = 0; BatchIndex < BatchedSprites.Num(); ++BatchIndex)
	{
		const FSpriteDrawCallRecord& Record = BatchedSprites[BatchIndex];

		FTexture* TextureResource = (Record.Texture != NULL) ? Record.Texture->Resource : NULL;
		if ((TextureResource != NULL) && (Record.RenderVerts.Num() > 0))
		{
			FLinearColor SpriteColor = bUseOverrideColor ? OverrideColor : Record.Color;

			const FVector EffectiveOrigin = Record.Destination;

			TArray< FPaperSpriteVertex, TInlineAllocator<6> > Vertices;
			Vertices.Empty(Record.RenderVerts.Num());
			for (int32 SVI = 0; SVI < Record.RenderVerts.Num(); ++SVI)
			{
				const FVector4& SourceVert = Record.RenderVerts[SVI];

				const FVector Pos((PaperAxisX * SourceVert.X) + (PaperAxisY * SourceVert.Y) + EffectiveOrigin);
				const FVector2D UV(SourceVert.Z, SourceVert.W);

				new (Vertices) FPaperSpriteVertex(Pos, UV, SpriteColor);
			}

			// Set up the FMeshElement.
			FMeshBatch Mesh;

			Mesh.UseDynamicData = true;
			Mesh.DynamicVertexData = Vertices.GetTypedData();
			Mesh.DynamicVertexStride = sizeof(FPaperSpriteVertex);

			Mesh.VertexFactory = &GPaperSpriteVertexFactory;

// 			const FColoredMaterialRenderProxy VertexColorVisualizationMaterialInstance(
// 				VertexColorVisualizationMaterial->GetRenderProxy( MeshElement.MaterialRenderProxy->IsSelected(),MeshElement.MaterialRenderProxy->IsHovered() ),
// 				GetSelectionColor( FLinearColor::White, bIsSelected, IsHovered() )
// 				);



			FMaterialRenderProxy* ParentMaterialProxy = Material->GetRenderProxy((View->Family->EngineShowFlags.Selection) && IsSelected(), IsHovered());

			// Create a texture override material proxy and register it as a dynamic resource so that it won't be deleted until the rendering thread has finished with it
			FTextureOverrideRenderProxy* TextureOverrideMaterialProxy = new FTextureOverrideRenderProxy(ParentMaterialProxy, Record.Texture, TEXT("SpriteTexture"));
			PDI->RegisterDynamicResource( TextureOverrideMaterialProxy );

			Mesh.MaterialRenderProxy = TextureOverrideMaterialProxy;
			Mesh.LCI = NULL;
			Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative() ? true : false;
			Mesh.CastShadow = false;
			Mesh.DepthPriorityGroup = DPG;
			Mesh.Type = PT_TriangleList;
			Mesh.bDisableBackfaceCulling = true;

			// Set up the FMeshBatchElement.
			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			BatchElement.IndexBuffer = NULL;
			BatchElement.DynamicIndexData = NULL;
			BatchElement.DynamicIndexStride = 0;
			BatchElement.FirstIndex = 0;
			BatchElement.MinVertexIndex = 0;
			BatchElement.MaxVertexIndex = Vertices.Num();
			BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
			BatchElement.NumPrimitives = Vertices.Num() / 3;

			const bool bIsWireframeView = View->Family->EngineShowFlags.Wireframe;

			DrawRichMesh(
				PDI, 
				Mesh,
				WireframeColor,
				FLinearColor(1.0f, 1.0f, 1.0f),//LevelColor,
				FLinearColor(1.0f, 1.0f, 1.0f),//PropertyColor,
				this,
				IsSelected(),
				bIsWireframeView
				);
		}
	}
}

FPrimitiveViewRelevance FPaperRenderSceneProxy::GetViewRelevance(const FSceneView* View)
{
	checkSlow(IsInRenderingThread());

	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View) && View->Family->EngineShowFlags.Paper2DSprites;
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bRenderInMainPass = ShouldRenderInMainPass();

	Result.bShadowRelevance = IsShadowCast(View);

	MaterialRelevance.SetPrimitiveViewRelevance(Result);

#undef SUPPORT_EXTRA_RENDERING
#define SUPPORT_EXTRA_RENDERING !(UE_BUILD_SHIPPING || UE_BUILD_TEST) || WITH_EDITOR
	

#if SUPPORT_EXTRA_RENDERING
	bool bDrawSimpleCollision = false;
	bool bDrawComplexCollision = false;
	const bool bInCollisionView = IsCollisionView(View, bDrawSimpleCollision, bDrawComplexCollision);
#endif

	Result.bDynamicRelevance = true;

// 	if (
// #if SUPPORT_EXTRA_RENDERING
// 		IsRichView(View) ||
// 		View->Family->EngineShowFlags.Collision ||
// 		bInCollisionView ||
// 		View->Family->EngineShowFlags.Bounds ||
// #endif
// 		// Force down dynamic rendering path if invalid lightmap settings, so we can apply an error material in DrawRichMesh
// 		(HasStaticLighting() && !HasValidSettingsForStaticLighting()) ||
// 		HasViewDependentDPG()
// #if WITH_EDITOR
// 		//only check these in the editor
// 		|| IsSelected() || IsHovered()
// #endif
// 		)
// 	{
//		Result.bDynamicRelevance = true;
// 	}
// 	else
// 	{
// 		Result.bStaticRelevance = true;
// 	}

	if (!View->Family->EngineShowFlags.Materials
#if SUPPORT_EXTRA_RENDERING
		|| bInCollisionView
#endif
		)
	{
		Result.bOpaqueRelevance = true;
	}

	return Result;
}

void FPaperRenderSceneProxy::OnTransformChanged()
{
	Origin = GetLocalToWorld().GetOrigin();
}

uint32 FPaperRenderSceneProxy::GetMemoryFootprint() const
{
	return sizeof(*this) + GetAllocatedSize();
}

bool FPaperRenderSceneProxy::CanBeOccluded() const
{
	return !MaterialRelevance.bDisableDepthTest;
}

void FPaperRenderSceneProxy::SetDrawCall_RenderThread(const FSpriteDrawCallRecord& NewDynamicData)
{
	BatchedSprites.Empty();

	FSpriteDrawCallRecord& Record = *new (BatchedSprites) FSpriteDrawCallRecord;
	Record = NewDynamicData;
}

bool FPaperRenderSceneProxy::IsCollisionView(const FSceneView* View, bool& bDrawSimpleCollision, bool& bDrawComplexCollision) const
{
	bDrawSimpleCollision = false;
	bDrawComplexCollision = false;

	// If in a 'collision view' and collision is enabled
	const bool bInCollisionView = View->Family->EngineShowFlags.CollisionVisibility || View->Family->EngineShowFlags.CollisionPawn;
	if (bInCollisionView && IsCollisionEnabled())
	{
		// See if we have a response to the interested channel
		bool bHasResponse = View->Family->EngineShowFlags.CollisionPawn && (CollisionResponse.GetResponse(ECC_Pawn) != ECR_Ignore);
		bHasResponse |= View->Family->EngineShowFlags.CollisionVisibility && (CollisionResponse.GetResponse(ECC_Visibility) != ECR_Ignore);

		if (bHasResponse)
		{
			bDrawComplexCollision = View->Family->EngineShowFlags.CollisionVisibility;
			bDrawSimpleCollision = View->Family->EngineShowFlags.CollisionPawn;
		}
	}

	return bInCollisionView;
}