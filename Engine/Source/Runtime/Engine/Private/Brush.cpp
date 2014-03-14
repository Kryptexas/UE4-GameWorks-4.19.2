// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Brush.cpp: Brush Actor implementation
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
#include "ActorEditorUtils.h"

#if WITH_EDITOR
/** Define static delegate */
ABrush::FOnBrushRegistered ABrush::OnBrushRegistered;

/** An array to keep track of all the levels that need rebuilding. This is checked via NeedsRebuild() in the editor tick and triggers a csg rebuild. */
TArray< TWeakObjectPtr< ULevel > > ABrush::LevelsToRebuild;
#endif

ABrush::ABrush(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BrushComponent = PCIP.CreateDefaultSubobject<UBrushComponent>(this, TEXT("BrushComponent0"));
	BrushComponent->Mobility = EComponentMobility::Static;
	BrushComponent->bGenerateOverlapEvents = false;

	RootComponent = BrushComponent;
	
	bHidden = true;
	bNotForClientOrServer = false;
	bWantsInitialize = false;
	bCanBeDamaged = false;
}

#if WITH_EDITOR
void ABrush::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if(Brush)
	{
		Brush->BuildBound();
	}

	if(IsStaticBrush())
	{
		// Trigger a csg rebuild if we're in the editor.
		SetNeedRebuild(GetLevel());
	}

	bool bIsBuilderBrush = FActorEditorUtils::IsABuilderBrush( this );
	if (!bIsBuilderBrush && (BrushType == Brush_Default))
	{
		// Don't allow non-builder brushes to be set to the default brush type
		BrushType = Brush_Add;
	}
	else if (bIsBuilderBrush && (BrushType != Brush_Default))
	{
		// Don't allow the builder brush to be set to the anything other than the default brush type
		BrushType = Brush_Default;
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void ABrush::CopyPosRotScaleFrom( ABrush* Other )
{
	check(BrushComponent);
	check(Other);
	check(Other->BrushComponent);

	SetActorLocation(Other->GetActorLocation(), false);
	SetActorRotation(Other->GetActorRotation());
	if( GetRootComponent() != NULL )
	{
		SetPrePivot( Other->GetPrePivot() );
	}

	if(Brush)
	{
		Brush->BuildBound();
	}

	ReregisterAllComponents();
}

void ABrush::InitPosRotScale()
{
	check(BrushComponent);

	SetActorLocation(FVector::ZeroVector, false);
	SetActorRotation(FRotator::ZeroRotator);
	SetPrePivot( FVector::ZeroVector );
}

void ABrush::SetIsTemporarilyHiddenInEditor( bool bIsHidden )
{
	if (IsTemporarilyHiddenInEditor() != bIsHidden)
	{
		Super::SetIsTemporarilyHiddenInEditor(bIsHidden);
		
		auto* Level = GetLevel();
		auto* Model = Level ? Level->Model : nullptr;

		if (Level && Model)
		{
			bool bAnySurfaceWasFound = false;
			for (int32 SurfIndex = 0; SurfIndex < Model->Surfs.Num(); ++SurfIndex)
			{
				FBspSurf &Surf = Model->Surfs[SurfIndex ];

				if (Surf.Actor == this)
				{
					Model->ModifySurf( SurfIndex , false );
					bAnySurfaceWasFound = true;
					Surf.bHiddenEdTemporary = bIsHidden;
				}
			}

			if (bAnySurfaceWasFound)
			{
				auto* Level = GetLevel();
				Level->UpdateModelComponents();
				Level->InvalidateModelSurface();
			}
		}
	}
}

#endif

FVector ABrush::GetPrePivot() const
{
	FVector Result(0.f);
	if( BrushComponent.IsValid() )
	{
		Result = BrushComponent->PrePivot;
	}
	return Result;
}

void ABrush::SetPrePivot( const FVector& InPrePivot )
{
	if( BrushComponent.IsValid() )
	{
		BrushComponent->PrePivot = InPrePivot;
		BrushComponent->UpdateComponentToWorld();
	}
}


void ABrush::PostLoad()
{
	Super::PostLoad();


	if( GetLinkerUE4Version() < VER_UE4_FIX_BSP_BRUSH_TYPE && BrushType == Brush_Default )
	{
		ECsgOper Oper = CsgOper_DEPRECATED;
		if( Oper == CSG_Add )
		{
			BrushType = Brush_Add;
		}
		else if( Oper == CSG_Subtract )
		{
			BrushType = Brush_Subtract;
		}
	}
#if WITH_EDITOR
	// Assign the default material to brush polys with NULL material references.
	if ( Brush && Brush->Polys )
	{
		if ( IsStaticBrush() )
		{
			for( int32 PolyIndex = 0 ; PolyIndex < Brush->Polys->Element.Num() ; ++PolyIndex )
			{
				FPoly& CurrentPoly = Brush->Polys->Element[PolyIndex];
				if ( !CurrentPoly.Material )
				{
					CurrentPoly.Material = UMaterial::GetDefaultMaterial(MD_Surface);
				}
			}
		}

		// if the polys of the brush have the wrong outer, fix it up to be the UModel (my Brush member)
		// UModelFactory::FactoryCreateText was passing in the ABrush as the Outer instead of the UModel
		if (Brush->Polys->GetOuter() == this)
		{
			Brush->Polys->Rename(*Brush->Polys->GetName(), Brush, REN_ForceNoResetLoaders);
		}
	}

	if ( BrushComponent && !BrushComponent->BrushBodySetup )
	{
		UE_LOG(LogPhysics, Log, TEXT("%s does not have BrushBodySetup. No collision."), *GetName());
	}
#endif
}

void ABrush::Destroyed()
{
	Super::Destroyed();

	if(IsStaticBrush())
	{
		// Trigger a csg rebuild if we're in the editor.
		SetNeedRebuild(GetLevel());
	}
}

void ABrush::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

#if WITH_EDITOR
	if ( GIsEditor )
	{
		OnBrushRegistered.Broadcast(this);
	}
#endif
}

bool ABrush::IsLevelBoundsRelevant() const
{
	// exclude default brush
	ULevel* Level = GetLevel();
	return (Level && this != Level->Actors[1]);
}

FColor ABrush::GetWireColor() const
{
	FColor Color = GEngine->C_BrushWire;

	if( IsStaticBrush() )
	{
		Color = bColored ?						BrushColor :
				BrushType == Brush_Subtract ?	GEngine->C_SubtractWire :
				BrushType != Brush_Add ?		GEngine->C_BrushWire :
				(PolyFlags & PF_Portal) ?		GEngine->C_SemiSolidWire :
				(PolyFlags & PF_NotSolid) ?		GEngine->C_NonSolidWire :
				(PolyFlags & PF_Semisolid) ?	GEngine->C_ScaleBoxHi :
												GEngine->C_AddWire;
	}
	else if( IsVolumeBrush() )
	{
		Color = bColored ? BrushColor : GEngine->C_Volume;
	}
	else if( IsBrushShape() )
	{
		Color = bColored ? BrushColor : GEngine->C_BrushShape;
	}

	return Color;
}

bool ABrush::IsStaticBrush() const
{
	return BrushComponent.IsValid() && (BrushComponent->Mobility == EComponentMobility::Static);
}

bool ABrush::Modify(bool bAlwaysMarkDirty)
{
	bool bSavedToTransactionBuffer = Super::Modify(bAlwaysMarkDirty);

	if(Brush)
	{
		bSavedToTransactionBuffer = Brush->Modify(bAlwaysMarkDirty) || bSavedToTransactionBuffer;
	}
	return bSavedToTransactionBuffer;
}