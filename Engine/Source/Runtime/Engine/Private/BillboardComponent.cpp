// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "LevelUtils.h"

/** Represents a billboard sprite to the scene manager. */
class FSpriteSceneProxy : public FPrimitiveSceneProxy
{
public:
	/** Initialization constructor. */
	FSpriteSceneProxy(const UBillboardComponent* InComponent, float SpriteScale)
		: FPrimitiveSceneProxy(InComponent)
		, ScreenSize(InComponent->ScreenSize)
		, U(InComponent->U)
		, V(InComponent->V)
		, Color(255,255,255)
		, LevelColor(255,255,255)
		, PropertyColor(255,255,255)
		, bIsScreenSizeScaled(InComponent->bIsScreenSizeScaled)
	{
#if WITH_EDITOR
		// Extract the sprite category from the component if in the editor
		if ( GIsEditor )
		{
			SpriteCategoryIndex = GEngine->GetSpriteCategoryIndex( InComponent->SpriteInfo.Category );
		}
#endif //WITH_EDITOR

		bWillEverBeLit = false;

		// Calculate the scale factor for the sprite.
		float Scale = InComponent->ComponentToWorld.GetMaximumAxisScale();

		if(InComponent->Sprite)
		{
			Texture = InComponent->Sprite;
			// Set UL and VL to the size of the texture if they are set to 0.0, otherwise use the given value
			UL = InComponent->UL == 0.0f ? InComponent->Sprite->GetSurfaceWidth() : InComponent->UL;
			VL = InComponent->VL == 0.0f ? InComponent->Sprite->GetSurfaceHeight() : InComponent->VL;
			SizeX = Scale * UL * SpriteScale * 0.25f;
			SizeY = Scale * VL * SpriteScale * 0.25f;
		}
		else
		{
			Texture = NULL;
			SizeX = SizeY = UL = VL = 0;
		}

		if (AActor* Owner = InComponent->GetOwner())
		{
			// If the owner of this sprite component is an ALight, tint the sprite
			// to match the lights color.  
			if (ALight* Light = Cast<ALight>(Owner))
			{
				if (Light->LightComponent)
				{
					Color = Light->LightComponent->LightColor.ReinterpretAsLinear();
					Color.A = 255;
				}
			}
#if WITH_EDITORONLY_DATA
			else if (GIsEditor)
			{
				Color.R = 255;
				Color.G = 255;
				Color.B = 255;
				// Ensure sprite is not transparent.
				Color.A = 255;
			}
#endif // WITH_EDITORONLY_DATA

			//save off override states
#if WITH_EDITORONLY_DATA
			bIsActorLocked = Owner->bLockLocation;
#else // WITH_EDITORONLY_DATA
			bIsActorLocked = false;
#endif // WITH_EDITORONLY_DATA

			// Level colorization
			ULevel* Level = Owner->GetLevel();
			if (ULevelStreaming* LevelStreaming = FLevelUtils::FindStreamingLevel(Level))
			{
				// Selection takes priority over level coloration.
				LevelColor = LevelStreaming->DrawColor;
			}
		}

		GEngine->GetPropertyColorationColor( (UObject*)InComponent, PropertyColor );
	}

	// FPrimitiveSceneProxy interface.
	
	/** 
	 * Draw the scene proxy as a dynamic element
	 *
	 * @param	PDI - draw interface to render to
	 * @param	View - current view
	 */
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) OVERRIDE
	{
		QUICK_SCOPE_CYCLE_COUNTER( STAT_SpriteSceneProxy_DrawDynamicElements );

		FTexture* TextureResource = (Texture != NULL) ? Texture->Resource : NULL;
		if (TextureResource)
		{
			// Calculate the view-dependent scaling factor.
			float ViewedSizeX = SizeX;
			float ViewedSizeY = SizeY;
			if (bIsScreenSizeScaled && (View->ViewMatrices.ProjMatrix.M[3][3] != 1.0f))
			{
				const float ZoomFactor	= FMath::Min<float>(View->ViewMatrices.ProjMatrix.M[0][0], View->ViewMatrices.ProjMatrix.M[1][1]);
				const float Radius		= View->WorldToScreen(Origin).W * (ScreenSize / ZoomFactor);
				if (Radius < 1.0f)
				{
					ViewedSizeX *= Radius;
					ViewedSizeY *= Radius;
				}
			}

			FLinearColor ColorToUse = Color;

			// Set the selection/hover color from the current engine setting.
			// The color is multiplied by 10 because this value is normally expected to be blended
			// additively, this is not how the sprites work and therefore need an extra boost
			// to appear the same color as previously.
			if (IsSelected())
			{
				ColorToUse = FLinearColor::White + (GEngine->GetSelectedMaterialColor() * GEngine->SelectionHighlightIntensityBillboards * 10);
			}
			else if (IsHovered())
			{
				ColorToUse = FLinearColor::White + (GEngine->GetHoveredMaterialColor() * GEngine->HoverHighlightIntensity * 10);
			}

			// Sprites of locked actors draw in red.
			if (bIsActorLocked)
			{
				ColorToUse = FColor(255,0,0);
			}
			FLinearColor LevelColorToUse = IsSelected() ? ColorToUse : (FLinearColor)LevelColor;
			FLinearColor PropertyColorToUse = PropertyColor;

			const FLinearColor& SpriteColor = View->Family->EngineShowFlags.LevelColoration ? LevelColorToUse :
											( (View->Family->EngineShowFlags.PropertyColoration) ? PropertyColorToUse : ColorToUse );

			PDI->DrawSprite(
				Origin,
				ViewedSizeX,
				ViewedSizeY,
				TextureResource,
				SpriteColor,
				GetDepthPriorityGroup(View),
				U,UL,V,VL
				);
		}
	}
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) OVERRIDE
	{
		bool bVisible = View->Family->EngineShowFlags.BillboardSprites;
#if WITH_EDITOR
		if ( GIsEditor && bVisible && SpriteCategoryIndex != INDEX_NONE && SpriteCategoryIndex < View->SpriteCategoryVisibility.Num() )
		{
			bVisible = View->SpriteCategoryVisibility[ SpriteCategoryIndex ];
		}
#endif
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View) && bVisible;
		Result.bOpaqueRelevance = true;
		Result.bNormalTranslucencyRelevance = false;
		Result.bDynamicRelevance = true;
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
		return Result;
	}
	virtual void OnTransformChanged() OVERRIDE
	{
		Origin = GetLocalToWorld().GetOrigin();
	}
	virtual uint32 GetMemoryFootprint(void) const OVERRIDE { return(sizeof(*this) + GetAllocatedSize()); }
	
	uint32 GetAllocatedSize(void) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

private:
	FVector Origin;
	float SizeX;
	float SizeY;
	const float ScreenSize;
	const UTexture2D* Texture;
	const float U;
	float UL;
	const float V;
	float VL;
	FColor Color;
	FColor LevelColor;
	FColor PropertyColor;
	const uint32 bIsScreenSizeScaled : 1;
	uint32 bIsActorLocked : 1;
#if WITH_EDITOR
	int32 SpriteCategoryIndex;
#endif // #if WITH_EDITOR
};

UBillboardComponent::UBillboardComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinder<UTexture2D> SpriteTexture;
		FName ID_Misc;
		FText NAME_Misc;
		FConstructorStatics()
			: SpriteTexture(TEXT("/Engine/EditorResources/S_Actor"))
			, ID_Misc(TEXT("Misc"))
			, NAME_Misc(NSLOCTEXT( "SpriteCategory", "Misc", "Misc" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	BodyInstance.bEnableCollision_DEPRECATED = false;
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Sprite = ConstructorStatics.SpriteTexture.Object;
	bAbsoluteScale = true;

	bIsScreenSizeScaled = false;
	ScreenSize = 0.1;
	U = 0;
	V = 0;
	UL = 0;
	VL = 0;
	bHiddenInGame = true;
	bGenerateOverlapEvents = false;
	bUseEditorCompositing = true;

#if WITH_EDITORONLY_DATA
	SpriteInfo.Category = ConstructorStatics.ID_Misc;
	SpriteInfo.DisplayName = ConstructorStatics.NAME_Misc;
#endif // WITH_EDITORONLY_DATA
}

FPrimitiveSceneProxy* UBillboardComponent::CreateSceneProxy()
{
	float SpriteScale = 1.0f;
#if WITH_EDITOR
	if (GetOwner())
	{
		SpriteScale = GetOwner()->SpriteScale;
	}
#endif
	return new FSpriteSceneProxy(this, SpriteScale);
}

FBoxSphereBounds UBillboardComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	const float NewScale = LocalToWorld.GetScale3D().GetMax() * (Sprite ? (float)FMath::Max(Sprite->GetSizeX(),Sprite->GetSizeY()) : 1.0f);
	return FBoxSphereBounds(LocalToWorld.GetLocation(),FVector(NewScale,NewScale,NewScale),FMath::Sqrt(3.0f * FMath::Square(NewScale)));
}

void UBillboardComponent::SetSprite(UTexture2D* NewSprite)
{
	Sprite = NewSprite;
	MarkRenderStateDirty();
}

void UBillboardComponent::SetUV(int32 NewU, int32 NewUL, int32 NewV, int32 NewVL)
{
	U = NewU;
	UL = NewUL;
	V = NewV;
	VL = NewVL;
	MarkRenderStateDirty();
}

void UBillboardComponent::SetSpriteAndUV(UTexture2D* NewSprite, int32 NewU, int32 NewUL, int32 NewV, int32 NewVL)
{
	U = NewU;
	UL = NewUL;
	V = NewV;
	VL = NewVL;
	SetSprite(NewSprite);
}
