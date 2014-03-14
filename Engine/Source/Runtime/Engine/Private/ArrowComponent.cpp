// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#define ARROW_SCALE	16.0f

/** Represents a UArrowComponent to the scene manager. */
class FArrowSceneProxy : public FPrimitiveSceneProxy
{
public:

	FArrowSceneProxy(UArrowComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, ArrowColor(Component->ArrowColor)
		, ArrowSize(Component->ArrowSize)
#if WITH_EDITORONLY_DATA
		, bLightAttachment(Component->bLightAttachment)
		, bTreatAsASprite(Component->bTreatAsASprite)
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
		DrawDirectionalArrow(PDI,EffectiveLocalToWorld,ArrowColor,ArrowSize * 3.0f * ARROW_SCALE,ARROW_SCALE,GetDepthPriorityGroup(View));
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
	virtual uint32 GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
	uint32 GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

private:
	FColor ArrowColor;
	float ArrowSize;
#if WITH_EDITORONLY_DATA
	bool bLightAttachment;
	bool bTreatAsASprite;
	int32 SpriteCategoryIndex;
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
#if WITH_EDITORONLY_DATA
	SpriteInfo.Category = ConstructorStatics.ID_Misc;
	SpriteInfo.DisplayName = ConstructorStatics.NAME_Misc;
	bLightAttachment = false;
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
