// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#define DEFAULT_SCREEN_SIZE	(0.0025f)
#define ARROW_SCALE			(80.0f)
#define ARROW_RADIUS_FACTOR	(0.03f)
#define ARROW_HEAD_FACTOR	(0.2f)
#define ARROW_HEAD_ANGLE	(20.f)

#if WITH_EDITORONLY_DATA
float UArrowComponent::EditorScale = 1.0f;
#endif

/** Represents a UArrowComponent to the scene manager. */
class FArrowSceneProxy : public FPrimitiveSceneProxy
{
public:

	FArrowSceneProxy(UArrowComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, ArrowColor(Component->ArrowColor)
		, ArrowSize(Component->ArrowSize)
		, bIsScreenSizeScaled(Component->bIsScreenSizeScaled)
		, ScreenSize(Component->ScreenSize)
#if WITH_EDITORONLY_DATA
		, bLightAttachment(Component->bLightAttachment)
		, bTreatAsASprite(Component->bTreatAsASprite)
		, bUseInEditorScaling(Component->bUseInEditorScaling)
		, EditorScale(Component->EditorScale)
#endif
	{
		bWillEverBeLit = false;
#if WITH_EDITOR
		// If in the editor, extract the sprite category from the component
		if ( GIsEditor )
		{
			SpriteCategoryIndex = GEngine->GetSpriteCategoryIndex( Component->SpriteInfo.Category );
		}
#endif	//WITH_EDITOR
	}

	// FPrimitiveSceneProxy interface.
	
	/** 
	* Draw the scene proxy as a dynamic element
	*
	* @param	PDI - draw interface to render to
	* @param	View - current view
	*/
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View)
	{
		QUICK_SCOPE_CYCLE_COUNTER( STAT_ArrowSceneProxy_DrawDynamicElements );

		FMatrix EffectiveLocalToWorld;
#if WITH_EDITOR
		if (bLightAttachment)
		{
			EffectiveLocalToWorld = GetLocalToWorld().GetMatrixWithoutScale();
		}
		else
#endif	//WITH_EDITOR
		{
			EffectiveLocalToWorld = GetLocalToWorld();
		}

		const FColoredMaterialRenderProxy CollisionMaterialInstance(
			GEngine->ArrowMaterial->GetRenderProxy(IsSelected(), IsHovered()),
			ArrowColor,
			"GizmoColor"
			);

		const float TotalLength = ArrowSize * ARROW_SCALE;
		const float HeadLength = TotalLength * ARROW_HEAD_FACTOR;
		const float ShaftRadius = TotalLength * ARROW_RADIUS_FACTOR;
		const float ShaftLength = (TotalLength - HeadLength) * 1.1f; // 10% overlap between shaft and head
		const FVector Offset(0, 0, 0.5f * ShaftLength);
		const float HeadAngle = FMath::DegreesToRadians(ARROW_HEAD_ANGLE);

		// Calculate the view-dependent scaling factor.
		float ViewScale = 1.0f;
		if (bIsScreenSizeScaled && (View->ViewMatrices.ProjMatrix.M[3][3] != 1.0f))
		{
			const float ZoomFactor	= FMath::Min<float>(View->ViewMatrices.ProjMatrix.M[0][0], View->ViewMatrices.ProjMatrix.M[1][1]);
			if(ZoomFactor != 0.0f)
			{
				const float Radius		= View->WorldToScreen(Origin).W * (ScreenSize / ZoomFactor);
				if (Radius < 1.0f)
				{
					ViewScale *= Radius;
				}
			}
		}

#if WITH_EDITORONLY_DATA
		ViewScale *= EditorScale;
#endif

		const FMatrix CylinderTransform = FScaleMatrix(ViewScale) * FRotationMatrix( FRotator(-90,0.f,0) ) * FTranslationMatrix(FVector(ViewScale, 0.0f, 0.0f)) * EffectiveLocalToWorld;
		DrawCylinder(PDI, CylinderTransform, Offset, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), ShaftRadius, 0.5f * ShaftLength, 16, &CollisionMaterialInstance, SDPG_World);
		
		const FMatrix ConeTransform = FScaleMatrix(ViewScale * -HeadLength) * FTranslationMatrix(FVector(ViewScale * TotalLength,0,0)) * EffectiveLocalToWorld;
		DrawCone(PDI, ConeTransform, HeadAngle, HeadAngle, 32, false, FColor::White, &CollisionMaterialInstance, SDPG_World);
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View) && (View->Family->EngineShowFlags.BillboardSprites);
		Result.bDynamicRelevance = true;
#if WITH_EDITORONLY_DATA
		if (bTreatAsASprite)
		{
			if ( GIsEditor && SpriteCategoryIndex != INDEX_NONE && SpriteCategoryIndex < View->SpriteCategoryVisibility.Num() && !View->SpriteCategoryVisibility[ SpriteCategoryIndex ] )
			{
				Result.bDrawRelevance = false;
			}
		}
#endif
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
		return Result;
	}
	virtual void OnTransformChanged() OVERRIDE
	{
		Origin = GetLocalToWorld().GetOrigin();
	}
	virtual uint32 GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
	uint32 GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

private:
	FVector Origin;
	FColor ArrowColor;
	float ArrowSize;
	bool bIsScreenSizeScaled;
	float ScreenSize;
#if WITH_EDITORONLY_DATA
	bool bLightAttachment;
	bool bTreatAsASprite;
	int32 SpriteCategoryIndex;
	bool bUseInEditorScaling;
	float EditorScale;
#endif // #if WITH_EDITORONLY_DATA
};

UArrowComponent::UArrowComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FName ID_Misc;
		FText NAME_Misc;
		FConstructorStatics()
			: ID_Misc(TEXT("Misc"))
			, NAME_Misc(NSLOCTEXT( "SpriteCategory", "Misc", "Misc" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	BodyInstance.bEnableCollision_DEPRECATED = false;
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	ArrowColor = FColor(255, 0, 0, 255);

	ArrowSize = 1.0f;
	bHiddenInGame = true;
	bUseEditorCompositing = true;
	bGenerateOverlapEvents = false;
	bIsScreenSizeScaled = false;
	ScreenSize = DEFAULT_SCREEN_SIZE;
#if WITH_EDITORONLY_DATA
	SpriteInfo.Category = ConstructorStatics.ID_Misc;
	SpriteInfo.DisplayName = ConstructorStatics.NAME_Misc;
	bLightAttachment = false;
	bUseInEditorScaling = true;
#endif // WITH_EDITORONLY_DATA
}

FPrimitiveSceneProxy* UArrowComponent::CreateSceneProxy()
{
	return new FArrowSceneProxy(this);
}


FBoxSphereBounds UArrowComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	return FBoxSphereBounds(FBox(FVector(0,-ARROW_SCALE,-ARROW_SCALE),FVector(ArrowSize * ARROW_SCALE * 3.0f,ARROW_SCALE,ARROW_SCALE))).TransformBy(LocalToWorld);
}

void UArrowComponent::SetArrowColor_DEPRECATED(FColor NewColor)
{
	ArrowColor = NewColor;
	MarkRenderStateDirty();
}

void UArrowComponent::SetArrowColor_New(FLinearColor NewColor)
{
	ArrowColor = NewColor;
	MarkRenderStateDirty();
}

#if WITH_EDITORONLY_DATA
void UArrowComponent::SetEditorScale(float InEditorScale)
{
	EditorScale = InEditorScale;
	for(TObjectIterator<UArrowComponent> It; It; ++It)
	{
		It->MarkRenderStateDirty();
	}
}
#endif