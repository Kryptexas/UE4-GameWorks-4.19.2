// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperRenderSceneProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "Rendering/PaperBatchManager.h"
#include "Rendering/PaperBatchSceneProxy.h"
#include "PhysicsEngine/BodySetup2D.h"
#include "LocalVertexFactory.h"

//////////////////////////////////////////////////////////////////////////
// FPaperSpriteVertex

FPackedNormal FPaperSpriteVertex::PackedNormalX(FVector(1.0f, 0.0f, 0.0f));
FPackedNormal FPaperSpriteVertex::PackedNormalZ(FVector(0.0f, -1.0f, 0.0f));

void FPaperSpriteVertex::SetTangentsFromPaperAxes()
{
	PackedNormalX = PaperAxisX;
	PackedNormalZ = PaperAxisZ;
	// store determinant of basis in w component of normal vector
	PackedNormalZ.Vector.W = (GetBasisDeterminantSign(PaperAxisX, PaperAxisY, PaperAxisZ) < 0.0f) ? 0 : 255;
}

//////////////////////////////////////////////////////////////////////////
// FPaperSpriteVertexBuffer

/** A dummy vertex buffer used to give the FPaperSpriteVertexFactory something to reference as a stream source. */
class FPaperSpriteVertexBuffer : public FVertexBuffer
{
public:
	virtual void InitRHI() override
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
	virtual const FMaterial* GetMaterial(ERHIFeatureLevel::Type InFeatureLevel) const override
	{
		return Parent->GetMaterial(InFeatureLevel);
	}

	virtual bool GetVectorValue(const FName ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const override
	{
		return Parent->GetVectorValue(ParameterName, OutValue, Context);
	}

	virtual bool GetScalarValue(const FName ParameterName, float* OutValue, const FMaterialRenderContext& Context) const override
	{
		return Parent->GetScalarValue(ParameterName, OutValue, Context);
	}

	virtual bool GetTextureValue(const FName ParameterName,const UTexture** OutValue, const FMaterialRenderContext& Context) const override
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
	, bCastShadow(InComponent->CastShadow)
	, WireframeColor(FLinearColor::White)
	, CollisionResponse(InComponent->GetCollisionResponseToChannels())
{
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

FVertexFactory* FPaperRenderSceneProxy::GetPaperSpriteVertexFactory() const
{
	static bool bInitAxes = false;
	if (!bInitAxes)
	{
		bInitAxes = true;
		FPaperSpriteVertex::SetTangentsFromPaperAxes();
	}

	static TGlobalResource<FPaperSpriteVertexFactory> GPaperSpriteVertexFactory;
	return &GPaperSpriteVertexFactory;
}

void FPaperRenderSceneProxy::DrawDynamicElements_RichMesh(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor)
{
	if (Material != nullptr)
	{
		DrawBatch(PDI, View, bUseOverrideColor, OverrideColor, Material, BatchedSprites);
	}
	DrawNewBatches(PDI, View, bUseOverrideColor, OverrideColor);
}

void FPaperRenderSceneProxy::DrawNewBatches(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor)
{
	//@TODO: Doesn't support OverrideColor yet
	if (BatchedSections.Num() == 0)
	{
		return;
	}

	const uint8 DPG = GetDepthPriorityGroup(View);
	
	FVertexFactory* VertexFactory = GetPaperSpriteVertexFactory();

	FMeshBatch Mesh;

	Mesh.UseDynamicData = true;
	Mesh.DynamicVertexData = BatchedVertices.GetTypedData();
	Mesh.DynamicVertexStride = sizeof(FPaperSpriteVertex);

	Mesh.VertexFactory = VertexFactory;
	Mesh.LCI = NULL;
	Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative() ? true : false;
	Mesh.CastShadow = bCastShadow;
	Mesh.DepthPriorityGroup = DPG;
	Mesh.Type = PT_TriangleList;
	Mesh.bDisableBackfaceCulling = true;

	// Set up the FMeshBatchElement.
	FMeshBatchElement& BatchElement = Mesh.Elements[0];
	BatchElement.IndexBuffer = NULL;
	BatchElement.DynamicIndexData = NULL;
	BatchElement.DynamicIndexStride = 0;
	BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();

	const bool bIsWireframeView = View->Family->EngineShowFlags.Wireframe;

	for (const FSpriteRenderSection& Batch : BatchedSections)
	{
		const FLinearColor EffectiveWireframeColor = (Batch.Material->GetBlendMode() != BLEND_Opaque) ? WireframeColor : FLinearColor::Green;
		FMaterialRenderProxy* ParentMaterialProxy = Batch.Material->GetRenderProxy((View->Family->EngineShowFlags.Selection) && IsSelected(), IsHovered());

		FTexture* TextureResource = (Batch.Texture != nullptr) ? Batch.Texture->Resource : nullptr;
		if ((TextureResource != nullptr) && (Batch.NumVertices > 0))
		{
			// Create a texture override material proxy and register it as a dynamic resource so that it won't be deleted until the rendering thread has finished with it
			FTextureOverrideRenderProxy* TextureOverrideMaterialProxy = new FTextureOverrideRenderProxy(ParentMaterialProxy, Batch.Texture, TEXT("SpriteTexture"));
			PDI->RegisterDynamicResource(TextureOverrideMaterialProxy);

			Mesh.MaterialRenderProxy = TextureOverrideMaterialProxy;

			BatchElement.FirstIndex = Batch.VertexOffset;
			BatchElement.MinVertexIndex = Batch.VertexOffset;
			BatchElement.MaxVertexIndex = Batch.VertexOffset + Batch.NumVertices;
			BatchElement.NumPrimitives = Batch.NumVertices / 3;

			DrawRichMesh(
				PDI, 
				Mesh,
				EffectiveWireframeColor,
				FLinearColor(1.0f, 1.0f, 1.0f),//LevelColor,
				FLinearColor(1.0f, 1.0f, 1.0f),//PropertyColor,
				this,
				IsSelected(),
				bIsWireframeView
				);
		}
	}
}

void FPaperRenderSceneProxy::DrawBatch(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor, class UMaterialInterface* BatchMaterial, const TArray<FSpriteDrawCallRecord>& Batch)
{
	const uint8 DPG = GetDepthPriorityGroup(View);
	
	FVertexFactory* VertexFactory = GetPaperSpriteVertexFactory();

	const bool bIsWireframeView = View->Family->EngineShowFlags.Wireframe;
	const FLinearColor EffectiveWireframeColor = (BatchMaterial->GetBlendMode() != BLEND_Opaque) ? WireframeColor : FLinearColor::Green;

	for (const FSpriteDrawCallRecord& Record : Batch)
	{
		FTexture* TextureResource = (Record.Texture != nullptr) ? Record.Texture->Resource : nullptr;
		if ((TextureResource != nullptr) && (Record.RenderVerts.Num() > 0))
		{
			const FLinearColor SpriteColor = bUseOverrideColor ? OverrideColor : Record.Color;

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

			Mesh.VertexFactory = VertexFactory;

			FMaterialRenderProxy* ParentMaterialProxy = BatchMaterial->GetRenderProxy((View->Family->EngineShowFlags.Selection) && IsSelected(), IsHovered());

			// Create a texture override material proxy and register it as a dynamic resource so that it won't be deleted until the rendering thread has finished with it
			FTextureOverrideRenderProxy* TextureOverrideMaterialProxy = new FTextureOverrideRenderProxy(ParentMaterialProxy, Record.Texture, TEXT("SpriteTexture"));
			PDI->RegisterDynamicResource( TextureOverrideMaterialProxy );

			Mesh.MaterialRenderProxy = TextureOverrideMaterialProxy;
			Mesh.LCI = NULL;
			Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative() ? true : false;
			Mesh.CastShadow = bCastShadow;
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

			DrawRichMesh(
				PDI, 
				Mesh,
				EffectiveWireframeColor,
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