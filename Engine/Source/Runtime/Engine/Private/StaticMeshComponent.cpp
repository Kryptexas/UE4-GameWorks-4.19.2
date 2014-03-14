// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineSplineClasses.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"

#define LOCTEXT_NAMESPACE "StaticMeshComponent"

UStaticMeshComponent::UStaticMeshComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = false;

	BodyInstance.bEnableCollision_DEPRECATED = true;
	// check BaseEngine.ini for profile setup
	SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);

	WireframeColorOverride = FColor(255, 255, 255, 255);

	bOverrideLightMapRes = false;
	OverriddenLightMapRes = 64;
	SubDivisionStepSize = 32;
	bUseSubDivisions = true;
	StreamingDistanceMultiplier = 1.0f;
	bBoundsChangeTriggersStreamingDataRebuild = true;
	bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::Yes;

#if WITH_EDITORONLY_DATA
	SelectedEditorSection = INDEX_NONE;
#endif
}

bool UStaticMeshComponent::HasAnySockets() const
{
	return (StaticMesh != NULL) && (StaticMesh->Sockets.Num() > 0);
}

void UStaticMeshComponent::QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const
{
	if (StaticMesh != NULL)
	{
		for (int32 SocketIdx = 0; SocketIdx < StaticMesh->Sockets.Num(); ++SocketIdx)
		{
			if (UStaticMeshSocket* Socket = StaticMesh->Sockets[SocketIdx])
			{
				new (OutSockets) FComponentSocketDescription(Socket->SocketName, EComponentSocketType::Socket);
			}
		}
	}
}

TArray<FName> UStaticMeshComponent::GetAllSocketNames() const
{
	TArray<FName> SocketNames;
	if( StaticMesh )
	{
		for( auto It=StaticMesh->Sockets.CreateConstIterator(); It; ++It )
		{
			SocketNames.Add( FName( *((*It)->GetName()) ) );
		}
	}
	return SocketNames;
}

FString UStaticMeshComponent::GetDetailedInfoInternal() const
{
	FString Result;  

	if( StaticMesh != NULL )
	{
		Result = StaticMesh->GetPathName( NULL );
	}
	else
	{
		Result = TEXT("No_StaticMesh");
	}

	return Result;  
}


void UStaticMeshComponent::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{	
	UStaticMeshComponent* This = CastChecked<UStaticMeshComponent>(InThis);
	for (int32 LODIndex = 0; LODIndex < This->LODData.Num(); LODIndex++)
	{
		if (This->LODData[LODIndex].LightMap != NULL)
		{
			This->LODData[LODIndex].LightMap->AddReferencedObjects(Collector);
		}

		if (This->LODData[LODIndex].ShadowMap != NULL)
		{
			This->LODData[LODIndex].ShadowMap->AddReferencedObjects(Collector);
		}
	}

	Super::AddReferencedObjects(This, Collector);
}


void UStaticMeshComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

#if WITH_EDITORONLY_DATA
	if( Ar.IsCooking() )
	{
		// This is needed for data stripping
		for( int32 LODIndex = 0; LODIndex < LODData.Num(); LODIndex++ )
		{
			LODData[ LODIndex ].Owner = this;
		}
	}
#endif // WITH_EDITORONLY_DATA
	Ar << LODData;

	if (Ar.UE4Ver() < VER_UE4_COMBINED_LIGHTMAP_TEXTURES)
	{
		check(AttachmentCounter.GetValue() == 0);
		// Irrelevant lights were incorrect before VER_UE4_TOSS_IRRELEVANT_LIGHTS
		IrrelevantLights.Empty();
	}
}



bool UStaticMeshComponent::AreNativePropertiesIdenticalTo( UObject* Other ) const
{
	bool bNativePropertiesAreIdentical = Super::AreNativePropertiesIdenticalTo( Other );
	UStaticMeshComponent* OtherSMC = CastChecked<UStaticMeshComponent>(Other);

	if( bNativePropertiesAreIdentical )
	{
		// Components are not identical if they have lighting information.
		if( LODData.Num() || OtherSMC->LODData.Num() )
		{
			bNativePropertiesAreIdentical = false;
		}
	}
	
	return bNativePropertiesAreIdentical;
}

void UStaticMeshComponent::PreSave()
{
	Super::PreSave();
#if WITH_EDITORONLY_DATA
	CachePaintedDataIfNecessary();
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITOR
void UStaticMeshComponent::CheckForErrors()
{
	Super::CheckForErrors();

	// Get the mesh owner's name.
	AActor* Owner = GetOwner();
	FString OwnerName(*(GNone.ToString()));
	if ( Owner )
	{
		OwnerName = Owner->GetName();
	}

	// Make sure any simplified meshes can still find their high res source mesh
	if( StaticMesh != NULL && StaticMesh->RenderData )
	{
		int32 ZeroTriangleElements = 0;

		// Check for element material index/material mismatches
		for (int32 LODIndex = 0; LODIndex < StaticMesh->RenderData->LODResources.Num(); ++LODIndex)
		{
			FStaticMeshLODResources& LODData = StaticMesh->RenderData->LODResources[LODIndex];
			for (int32 SectionIndex = 0; SectionIndex < LODData.Sections.Num(); SectionIndex++)
			{
				FStaticMeshSection& Element = LODData.Sections[SectionIndex];
				if (Element.NumTriangles == 0)
				{
					ZeroTriangleElements++;
				}
			}
		}

		if (Materials.Num() > StaticMesh->Materials.Num())
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("OverridenCount"), Materials.Num());
			Arguments.Add(TEXT("ReferencedCount"), StaticMesh->Materials.Num());
			Arguments.Add(TEXT("MeshName"), FText::FromString(StaticMesh->GetName()));
			FMessageLog("MapCheck").Warning()
				->AddToken(FUObjectToken::Create(Owner))
				->AddToken(FTextToken::Create(FText::Format(LOCTEXT( "MapCheck_Message_MoreMaterialsThanReferenced", "More overriden materials ({OverridenCount}) on static mesh component than are referenced ({ReferencedCount}) in source mesh '{MeshName}'" ), Arguments ) ))
				->AddToken(FMapErrorToken::Create(FMapErrors::MoreMaterialsThanReferenced));
		}
		if (ZeroTriangleElements > 0)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("ElementCount"), ZeroTriangleElements);
			Arguments.Add(TEXT("MeshName"), FText::FromString(StaticMesh->GetName()));
			FMessageLog("MapCheck").Warning()
				->AddToken(FUObjectToken::Create(Owner))
				->AddToken(FTextToken::Create(FText::Format(LOCTEXT( "MapCheck_Message_ElementsWithZeroTriangles", "{ElementCount} element(s) with zero triangles in static mesh '{MeshName}'" ), Arguments ) ))
				->AddToken(FMapErrorToken::Create(FMapErrors::ElementsWithZeroTriangles));
		}
	}

	if (!StaticMesh && (!Owner || !Owner->IsA(AWorldSettings::StaticClass())))	// Ignore worldsettings
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("OwnerName"), FText::FromString(OwnerName));
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(Owner))
			->AddToken(FTextToken::Create(FText::Format(LOCTEXT( "MapCheck_Message_StaticMeshNull", "{OwnerName} : Static mesh actor has NULL StaticMesh property" ), Arguments ) ))
			->AddToken(FMapErrorToken::Create(FMapErrors::StaticMeshNull));
	}

	// Make sure any non uniform scaled meshes have appropriate collision
	if ( IsCollisionEnabled() && StaticMesh != NULL && StaticMesh->BodySetup != NULL && Owner != NULL )
	{
		// Overall scale factor for this mesh.
		const FVector& TotalScale3D = ComponentToWorld.GetScale3D();
		if ( !TotalScale3D.IsUniform() &&
			 (StaticMesh->BodySetup->AggGeom.BoxElems.Num() > 0   ||
			  StaticMesh->BodySetup->AggGeom.SphylElems.Num() > 0 ||
			  StaticMesh->BodySetup->AggGeom.SphereElems.Num() > 0) )

		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("MeshName"), FText::FromString(StaticMesh->GetName()));
			FMessageLog("MapCheck").Warning()
				->AddToken(FUObjectToken::Create(Owner))
				->AddToken(FTextToken::Create(FText::Format(LOCTEXT( "MapCheck_Message_SimpleCollisionButNonUniformScale", "'{MeshName}' has simple collision but is being scaled non-uniformly - collision creation will fail" ), Arguments)))
				->AddToken(FMapErrorToken::Create(FMapErrors::SimpleCollisionButNonUniformScale));
		}
	}

	if ( BodyInstance.bSimulatePhysics && StaticMesh != NULL && StaticMesh->BodySetup != NULL && StaticMesh->BodySetup->AggGeom.GetElementCount() == 0) 
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(LOCTEXT( "MapCheck_Message_SimulatePhyNoSimpleCollision", "{ActorName} : Using bSimulatePhysics but StaticMesh has not simple collision."), Arguments ) ));
	}

	if( Mobility == EComponentMobility::Movable &&
		CastShadow && 
		bCastDynamicShadow && 
		IsRegistered() && 
		Bounds.SphereRadius > 2000.0f )
	{
		// Large shadow casting objects that create preshadows will cause a massive performance hit, since preshadows are meant for small shadow casters.
		FMessageLog("MapCheck").PerformanceWarning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT( "MapCheck_Message_ActorLargeShadowCaster", "Large actor receives a pre-shadow and will cause an extreme performance hit unless bCastDynamicShadow is set to false.") ))
			->AddToken(FMapErrorToken::Create(FMapErrors::ActorLargeShadowCaster));
	}
}
#endif

FBoxSphereBounds UStaticMeshComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	if(StaticMesh)
	{
		// Graphics bounds.
		FBoxSphereBounds NewBounds = StaticMesh->GetBounds().TransformBy(LocalToWorld);
		
		// Add bounds of collision geometry (if present).
		if(StaticMesh->BodySetup)
		{
			FBox AggGeomBox = StaticMesh->BodySetup->AggGeom.CalcAABB(LocalToWorld);
			if (AggGeomBox.IsValid)
			{
				NewBounds = Union(NewBounds,FBoxSphereBounds(AggGeomBox));
			}
		}

		NewBounds.BoxExtent *= BoundsScale;
		NewBounds.SphereRadius *= BoundsScale;

		return NewBounds;
	}
	else
	{
		return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector::ZeroVector, 0.f);
	}
}

void UStaticMeshComponent::OnRegister()
{
	if(StaticMesh != NULL && StaticMesh->RenderData)
	{
		// Check that the static-mesh hasn't been changed to be incompatible with the cached light-map.
		int32 NumLODs = StaticMesh->RenderData->LODResources.Num();
		bool bLODsShareStaticLighting = StaticMesh->RenderData->bLODsShareStaticLighting;
		if (!bLODsShareStaticLighting && NumLODs != LODData.Num())
		{
			for(int32 i=0;i<LODData.Num();i++)
			{
				LODData[i].LightMap = NULL;
				LODData[i].ShadowMap = NULL;
			}
		}
	}

	Super::OnRegister();
}

void UStaticMeshComponent::GetStreamingTextureInfo(TArray<FStreamingTexturePrimitiveInfo>& OutStreamingTextures) const
{
	if ( StaticMesh && !bIgnoreInstanceForTextureStreaming )
	{
		bool bHasValidLightmapCoordinates = ((StaticMesh->LightMapCoordinateIndex >= 0)
			&& StaticMesh->RenderData
			&& StaticMesh->RenderData->LODResources.Num() > 0
			&& ((uint32)StaticMesh->LightMapCoordinateIndex < StaticMesh->RenderData->LODResources[0].VertexBuffer.GetNumTexCoords()));

		// We need to come up with a compensation factor for spline deformed meshes
		float SplineDeformFactor = 1.f;
		const USplineMeshComponent* SplineComp = Cast<const USplineMeshComponent>(this);
		if (SplineComp)
		{
			// We do this by looking at the ratio between current bounds (including deformation) and undeformed (straight from staticmesh)
			const float MinExtent = 1.0f;
			FBoxSphereBounds UndeformedBounds = StaticMesh->GetBounds().TransformBy(ComponentToWorld);
			if (UndeformedBounds.BoxExtent.X >= MinExtent)
			{
				SplineDeformFactor = FMath::Max(SplineDeformFactor, Bounds.BoxExtent.X / UndeformedBounds.BoxExtent.X);
			}
			if (UndeformedBounds.BoxExtent.Y >= MinExtent)
			{
				SplineDeformFactor = FMath::Max(SplineDeformFactor, Bounds.BoxExtent.Y / UndeformedBounds.BoxExtent.Y);
			}
			if (UndeformedBounds.BoxExtent.Z >= MinExtent)
			{
				SplineDeformFactor = FMath::Max(SplineDeformFactor, Bounds.BoxExtent.Z / UndeformedBounds.BoxExtent.Z);
			}
		}

		const FSphere BoundingSphere	= Bounds.GetSphere();
		const float LocalTexelFactor	= StaticMesh->GetStreamingTextureFactor(0) * StreamingDistanceMultiplier;
		const float LocalLightmapFactor	= bHasValidLightmapCoordinates ? StaticMesh->GetStreamingTextureFactor(StaticMesh->LightMapCoordinateIndex) : 1.0f;
		const float WorldTexelFactor	= SplineDeformFactor * LocalTexelFactor * ComponentToWorld.GetMaximumAxisScale();
		const float WorldLightmapFactor	= SplineDeformFactor * LocalLightmapFactor * ComponentToWorld.GetMaximumAxisScale();

		for (int32 LODIndex = 0; LODIndex < StaticMesh->RenderData->LODResources.Num(); ++LODIndex)
		{
			FStaticMeshLODResources& LOD = StaticMesh->RenderData->LODResources[LODIndex];

			// Process each material applied to the top LOD of the mesh.
			for( int32 ElementIndex = 0; ElementIndex < LOD.Sections.Num(); ElementIndex++ )
			{
				const FStaticMeshSection& Element = LOD.Sections[ElementIndex];
				UMaterialInterface* Material = GetMaterial(Element.MaterialIndex);
				if(!Material)
				{
					Material = UMaterial::GetDefaultMaterial(MD_Surface);
				}

				// Enumerate the textures used by the material.
				TArray<UTexture*> Textures;

				Material->GetUsedTextures(Textures, EMaterialQualityLevel::Num, false);

				// Add each texture to the output with the appropriate parameters.
				// TODO: Take into account which UVIndex is being used.
				for(int32 TextureIndex = 0;TextureIndex < Textures.Num();TextureIndex++)
				{
					FStreamingTexturePrimitiveInfo& StreamingTexture = *new(OutStreamingTextures) FStreamingTexturePrimitiveInfo;
					StreamingTexture.Bounds = BoundingSphere;
					StreamingTexture.TexelFactor = WorldTexelFactor;
					StreamingTexture.Texture = Textures[TextureIndex];
				}
			}

			// Add in the lightmaps and shadowmaps.
			if ( LODData.IsValidIndex(LODIndex) && bHasValidLightmapCoordinates )
			{
				const FStaticMeshComponentLODInfo& LODInfo = LODData[LODIndex];
				FLightMap2D* Lightmap = LODInfo.LightMap ? LODInfo.LightMap->GetLightMap2D() : NULL;
				uint32 LightmapIndex = AllowHighQualityLightmaps() ? 0 : 1;
				if ( Lightmap != NULL && Lightmap->IsValid(LightmapIndex) )
				{
					const FVector2D& Scale = Lightmap->GetCoordinateScale();
					if ( Scale.X > SMALL_NUMBER && Scale.Y > SMALL_NUMBER )
					{
						float LightmapFactorX		 = WorldLightmapFactor / Scale.X;
						float LightmapFactorY		 = WorldLightmapFactor / Scale.Y;
						FStreamingTexturePrimitiveInfo& StreamingTexture = *new(OutStreamingTextures) FStreamingTexturePrimitiveInfo;
						StreamingTexture.Bounds		 = BoundingSphere;
						StreamingTexture.TexelFactor = FMath::Max(LightmapFactorX, LightmapFactorY);
						StreamingTexture.Texture	 = Lightmap->GetTexture(LightmapIndex);
					}
				}

				FShadowMap2D* Shadowmap = LODInfo.ShadowMap ? LODInfo.ShadowMap->GetShadowMap2D() : NULL;

				if ( Shadowmap != NULL && Shadowmap->IsValid() )
				{
					const FVector2D& Scale = Shadowmap->GetCoordinateScale();
					if ( Scale.X > SMALL_NUMBER && Scale.Y > SMALL_NUMBER )
					{
						float ShadowmapFactorX		 = WorldLightmapFactor / Scale.X;
						float ShadowmapFactorY		 = WorldLightmapFactor / Scale.Y;
						FStreamingTexturePrimitiveInfo& StreamingTexture = *new(OutStreamingTextures) FStreamingTexturePrimitiveInfo;
						StreamingTexture.Bounds		 = BoundingSphere;
						StreamingTexture.TexelFactor = FMath::Max(ShadowmapFactorX, ShadowmapFactorY);
						StreamingTexture.Texture	 = Shadowmap->GetTexture();
					}
				}
			}
		}
	}
}

UBodySetup* UStaticMeshComponent::GetBodySetup()
{
	if(StaticMesh)
	{
		return StaticMesh->BodySetup;
	}

	return NULL;
}

FColor UStaticMeshComponent::GetWireframeColor() const
{
	if(bOverrideWireframeColor)
	{
		return WireframeColorOverride;
	}
	else
	{
		if(Mobility == EComponentMobility::Static)
		{
			return FColor(0, 255, 255, 255);
		}
		else if(Mobility == EComponentMobility::Stationary)
		{
			return FColor(128, 128, 255, 255);
		}
		else // Movable
		{
			if(BodyInstance.bSimulatePhysics)
			{
				return FColor(0, 255, 128, 255);
			}
			else
			{
				return FColor(255, 0, 255, 255);
			}
		}
	}
}


bool UStaticMeshComponent::DoesSocketExist(FName InSocketName) const 
{
	return (GetSocketByName(InSocketName)  != NULL);
}

class UStaticMeshSocket const* UStaticMeshComponent::GetSocketByName(FName InSocketName) const
{
	UStaticMeshSocket const* Socket = NULL;

	if( StaticMesh )
	{
		Socket = StaticMesh->FindSocket( InSocketName );
	}

	return Socket;
}

FTransform UStaticMeshComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const
{
	if (InSocketName != NAME_None)
	{
		UStaticMeshSocket const* const Socket = GetSocketByName(InSocketName);
		if (Socket)
		{
			FTransform SocketWorldTransform;
			if ( Socket->GetSocketTransform(SocketWorldTransform, this) )
			{
				switch(TransformSpace)
				{
					case RTS_World:
					{
						return SocketWorldTransform;
					}
					case RTS_Actor:
					{
						if( const AActor* Actor = GetOwner() )
						{
							return SocketWorldTransform.GetRelativeTransform( GetOwner()->GetTransform() );
						}
						break;
					}
					case RTS_Component:
					{
						return SocketWorldTransform.GetRelativeTransform(ComponentToWorld);
					}
				}
			}
		}
	}

	return ComponentToWorld;
}


bool UStaticMeshComponent::RequiresOverrideVertexColorsFixup( TArray<int32>& OutLODIndices )
{
	OutLODIndices.Empty();
	bool bFixupRequired = false;

#if WITH_EDITORONLY_DATA
	if ( StaticMesh && StaticMesh->RenderData
		&& StaticMesh->RenderData->DerivedDataKey != StaticMeshDerivedDataKey )
	{
		int32 NumLODs = StaticMesh->GetNumLODs();
		// Iterate over each LOD to confirm which ones, if any, actually need to have their colors updated
		for ( TArray<FStaticMeshComponentLODInfo>::TConstIterator LODIter( LODData ); LODIter; ++LODIter )
		{
			const FStaticMeshComponentLODInfo& CurCompLODInfo = *LODIter;

			// Confirm that the LOD has override colors and cached vertex data. If it doesn't have both, it can't be fixed up.
			if ( CurCompLODInfo.OverrideVertexColors && 
				CurCompLODInfo.OverrideVertexColors->GetNumVertices() > 0 && 
				CurCompLODInfo.PaintedVertices.Num() > 0 && 
				LODIter.GetIndex() < NumLODs )
			{
				OutLODIndices.Add( LODIter.GetIndex() );
				bFixupRequired = true;
			}
		}
	}
#endif // WITH_EDITORONLY_DATA

	return bFixupRequired;
}

void UStaticMeshComponent::RemoveInstanceVertexColorsFromLOD( int32 LODToRemoveColorsFrom )
{
#if WITH_EDITORONLY_DATA
	if (( LODToRemoveColorsFrom < StaticMesh->GetNumLODs() ) &&
		( LODToRemoveColorsFrom < LODData.Num() ))
	{
		FStaticMeshComponentLODInfo& CurrentLODInfo = LODData[LODToRemoveColorsFrom];

		CurrentLODInfo.ReleaseOverrideVertexColorsAndBlock();
		CurrentLODInfo.PaintedVertices.Empty();
		StaticMeshDerivedDataKey = StaticMesh->RenderData->DerivedDataKey;
	}
#endif
}

void UStaticMeshComponent::RemoveInstanceVertexColors()
{
#if WITH_EDITORONLY_DATA
	for ( int32 i=0; i < StaticMesh->GetNumLODs(); i++ )
	{
		RemoveInstanceVertexColorsFromLOD( i );
	}
#endif
}

void UStaticMeshComponent::CopyInstanceVertexColorsIfCompatible( UStaticMeshComponent* SourceComponent )
{
#if WITH_EDITORONLY_DATA
	// The static mesh assets have to match, currently.
	if (( StaticMesh->GetPathName() == SourceComponent->StaticMesh->GetPathName() ) &&
		( SourceComponent->LODData.Num() != 0 ))
	{
		Modify();

		// Remove any and all vertex colors from the target static mesh, if they exist.
		if ( IsRenderStateCreated() )
		{
			FComponentReregisterContext ComponentReregisterContext( this );
			RemoveInstanceVertexColors();
		}
		else
		{
			RemoveInstanceVertexColors();
		}

		int32 NumSourceLODs = SourceComponent->StaticMesh->GetNumLODs();

		// This this will set up the LODData for all the LODs
		SetLODDataCount( NumSourceLODs, NumSourceLODs );

		// Copy vertex colors from Source to Target (this)
		for ( int32 CurrentLOD = 0; CurrentLOD != NumSourceLODs; CurrentLOD++ )
		{
			FStaticMeshLODResources& SourceLODModel = SourceComponent->StaticMesh->RenderData->LODResources[CurrentLOD];
			FStaticMeshComponentLODInfo& SourceLODInfo = SourceComponent->LODData[CurrentLOD];

			FStaticMeshLODResources& TargetLODModel = StaticMesh->RenderData->LODResources[CurrentLOD];
			FStaticMeshComponentLODInfo& TargetLODInfo = LODData[CurrentLOD];

			if ( SourceLODInfo.OverrideVertexColors != NULL )
			{
				// Copy vertex colors from source to target.
				FColorVertexBuffer* SourceColorBuffer = SourceLODInfo.OverrideVertexColors;

				TArray< FColor > CopiedColors;
				for ( uint32 ColorVertexIndex = 0; ColorVertexIndex < SourceColorBuffer->GetNumVertices(); ColorVertexIndex++ )
				{
					CopiedColors.Add( SourceColorBuffer->VertexColor( ColorVertexIndex ) );
				}

				FColorVertexBuffer* TargetColorBuffer = &TargetLODModel.ColorVertexBuffer;

				if ( TargetLODInfo.OverrideVertexColors != NULL )
				{
					TargetLODInfo.BeginReleaseOverrideVertexColors();
					FlushRenderingCommands();
				}
				else
				{
					TargetLODInfo.OverrideVertexColors = new FColorVertexBuffer;
					TargetLODInfo.OverrideVertexColors->InitFromColorArray( CopiedColors );
				}
				BeginInitResource( TargetLODInfo.OverrideVertexColors );
			}
		}

		CachePaintedDataIfNecessary();
		StaticMeshDerivedDataKey = StaticMesh->RenderData->DerivedDataKey;

		MarkRenderStateDirty();
	}
#endif
}

void UStaticMeshComponent::CachePaintedDataIfNecessary()
{
#if WITH_EDITORONLY_DATA
	// Only cache the vertex positions if we're in the editor
	if ( GIsEditor && StaticMesh )
	{
		// Iterate over each component LOD info checking for the existence of override colors
		int32 NumLODs = StaticMesh->GetNumLODs();
		for ( TArray<FStaticMeshComponentLODInfo>::TIterator LODIter( LODData ); LODIter; ++LODIter )
		{
			FStaticMeshComponentLODInfo& CurCompLODInfo = *LODIter;

			// Workaround for a copy-paste bug. If the number of painted vertices is <= 1 we know the data is garbage.
			if ( CurCompLODInfo.PaintedVertices.Num() <= 1 )
			{
				CurCompLODInfo.PaintedVertices.Empty();
			}

			// If the mesh has override colors but no cached vertex positions, then the current vertex positions should be cached to help preserve instanced vertex colors during mesh tweaks
			// NOTE: We purposefully do *not* cache the positions if cached positions already exist, as this would result in the loss of the ability to fixup the component if the source mesh
			// were changed multiple times before a fix-up operation was attempted
			if ( CurCompLODInfo.OverrideVertexColors && 
				 CurCompLODInfo.OverrideVertexColors->GetNumVertices() > 0 &&
				 CurCompLODInfo.PaintedVertices.Num() == 0 &&
				 LODIter.GetIndex() < NumLODs ) 
			{
				FStaticMeshLODResources* CurRenderData = &(StaticMesh->RenderData->LODResources[ LODIter.GetIndex() ]);
				if ( CurRenderData->GetNumVertices() == CurCompLODInfo.OverrideVertexColors->GetNumVertices() )
				{
					// Cache the data.
					CurCompLODInfo.PaintedVertices.Reserve( CurRenderData->GetNumVertices() );
					for ( int32 VertIndex = 0; VertIndex < CurRenderData->GetNumVertices(); ++VertIndex )
					{
						FPaintedVertex* Vertex = new( CurCompLODInfo.PaintedVertices ) FPaintedVertex;
						Vertex->Position = CurRenderData->PositionVertexBuffer.VertexPosition( VertIndex );
						Vertex->Normal = CurRenderData->VertexBuffer.VertexTangentZ( VertIndex );
						Vertex->Color = CurCompLODInfo.OverrideVertexColors->VertexColor( VertIndex );
					}
				}
				else
				{
					UE_LOG(LogStaticMesh, Warning, TEXT("Unable to cache painted data for mesh component. Vertex color overrides will be lost if the mesh is modified. %s %s LOD%d."), *GetFullName(), *StaticMesh->GetFullName(), LODIter.GetIndex() );
				}
			}
		}
	}
#endif // WITH_EDITORONLY_DATA
}

bool UStaticMeshComponent::FixupOverrideColorsIfNecessary( bool bRebuildingStaticMesh )
{
#if WITH_EDITORONLY_DATA

	// Detect if there is a version mismatch between the source mesh and the component. If so, the component's LODs potentially
	// need to have their override colors updated to match changes in the source mesh.
	TArray<int32> LODsToUpdate;

	if ( RequiresOverrideVertexColorsFixup( LODsToUpdate ) )
	{
		// Check if we are building the static mesh.  If so we dont need to reregister this component as its already unregistered and will be reregistered
		// when the static mesh is done building.  Having nested reregister contexts is not supported.
		if( bRebuildingStaticMesh )
		{
			PrivateFixupOverrideColors( LODsToUpdate );
		}
		else
		{
			// Detach this component because rendering changes are about to be applied
			FComponentReregisterContext ReregisterContext( this );
			PrivateFixupOverrideColors( LODsToUpdate );
		}
	}
	return LODsToUpdate.Num() > 0 ? true : false;
#else
	return false;
#endif // WITH_EDITORONLY_DATA

}

void UStaticMeshComponent::PrivateFixupOverrideColors( const TArray<int32>& LODsToUpdate )
{
#if WITH_EDITORONLY_DATA
	UE_LOG(LogStaticMesh,Verbose,TEXT("Fixing up override colors for %s [%s]"),*GetPathName(),*StaticMesh->GetPathName());
	for ( TArray<int32>::TConstIterator LODIdxIter( LODsToUpdate ); LODIdxIter; ++LODIdxIter )
	{
		FStaticMeshComponentLODInfo& CurCompLODInfo = LODData[ *LODIdxIter ];
		FStaticMeshLODResources& CurRenderData = StaticMesh->RenderData->LODResources[ *LODIdxIter ];
		TArray<FColor> NewOverrideColors;

		CurCompLODInfo.BeginReleaseOverrideVertexColors();
		FlushRenderingCommands();
		RemapPaintedVertexColors(
			CurCompLODInfo.PaintedVertices,
			*CurCompLODInfo.OverrideVertexColors,
			CurRenderData.PositionVertexBuffer,
			&CurRenderData.VertexBuffer,
			NewOverrideColors
			);
		if (NewOverrideColors.Num())
		{
			CurCompLODInfo.OverrideVertexColors->InitFromColorArray(NewOverrideColors);
		}
		// Note: In order to avoid data loss we dont clear the colors if they cannot be fixed up.

		// Initialize the vert. colors
		BeginInitResource( CurCompLODInfo.OverrideVertexColors );
	}
	StaticMeshDerivedDataKey = StaticMesh->RenderData->DerivedDataKey;
#endif // WITH_EDITORONLY_DATA
}

void UStaticMeshComponent::InitResources()
{
	for(int32 LODIndex = 0; LODIndex < LODData.Num(); LODIndex++)
	{
		FStaticMeshComponentLODInfo &LODInfo = LODData[LODIndex];
		if(LODInfo.OverrideVertexColors)
		{
			BeginInitResource(LODInfo.OverrideVertexColors);
			INC_DWORD_STAT_BY( STAT_InstVertexColorMemory, LODInfo.OverrideVertexColors->GetAllocatedSize() );
		}
	}
}



void UStaticMeshComponent::ReleaseResources()
{
	for(int32 LODIndex = 0;LODIndex < LODData.Num();LODIndex++)
	{
		LODData[LODIndex].BeginReleaseOverrideVertexColors();
	}

	DetachFence.BeginFence();
}

void UStaticMeshComponent::BeginDestroy()
{
	Super::BeginDestroy();
	ReleaseResources();
}

void UStaticMeshComponent::ExportCustomProperties(FOutputDevice& Out, uint32 Indent)
{
	for (int32 LODIdx = 0; LODIdx < LODData.Num(); ++LODIdx)
	{
		Out.Logf(TEXT("%sCustomProperties "), FCString::Spc(Indent));

		FStaticMeshComponentLODInfo& LODInfo = LODData[LODIdx];

		if ((LODInfo.PaintedVertices.Num() > 0) || LODInfo.OverrideVertexColors)
		{
			Out.Logf( TEXT("CustomLODData LOD=%d "), LODIdx );
		}

		// Export the PaintedVertices array
		if (LODInfo.PaintedVertices.Num() > 0)
		{
			FString	ValueStr;
			LODInfo.ExportText(ValueStr);
			Out.Log(ValueStr);
		}
		
		// Export the OverrideVertexColors buffer
		if(LODInfo.OverrideVertexColors && LODInfo.OverrideVertexColors->GetNumVertices() > 0)
		{
			FString	Value;
			LODInfo.OverrideVertexColors->ExportText(Value);

			Out.Log(Value);
		}
		Out.Logf(TEXT("\r\n"));
	}
}

void UStaticMeshComponent::ImportCustomProperties(const TCHAR* SourceText, FFeedbackContext* Warn)
{
	check(SourceText);
	check(Warn);

	if (FParse::Command(&SourceText,TEXT("CustomLODData")))
	{
		int32 MaxLODIndex = -1;
		int32 LODIndex;
		FString TmpStr;

		if (FParse::Value(SourceText, TEXT("LOD="), LODIndex))
		{
			TmpStr = FString::Printf(TEXT("%d"), LODIndex);
			SourceText += TmpStr.Len() + 5;

			// See if we need to add a new element to the LODData array
			if (LODIndex > MaxLODIndex)
			{
				SetLODDataCount(LODIndex + 1, LODIndex + 1);
				MaxLODIndex = LODIndex;
			}
		}

		FStaticMeshComponentLODInfo& LODInfo = LODData[LODIndex];

		// Populate the PaintedVertices array
		LODInfo.ImportText(&SourceText);

		// Populate the OverrideVertexColors buffer
		check(!LODInfo.OverrideVertexColors);
		const TCHAR* Result = FCString::Stristr(SourceText, TEXT("ColorVertexData"));
		if (Result)
		{
			SourceText = Result;

			LODInfo.OverrideVertexColors = new FColorVertexBuffer;
			LODInfo.OverrideVertexColors->ImportText(SourceText);
		}
	}
}

#if WITH_EDITOR

void UStaticMeshComponent::PreEditUndo()
{
	Super::PreEditUndo();

	// Undo can result in a resize of LODData which can calls ~FStaticMeshComponentLODInfo.
	// To safely delete FStaticMeshComponentLODInfo::OverrideVertexColors we
	// need to make sure the RT thread has no access to it any more.
	check(!IsRegistered());
	ReleaseResources();
	DetachFence.Wait();
}

void UStaticMeshComponent::PostEditUndo()
{
	// If the StaticMesh was also involved in this transaction, it may need reinitialization first
	if (StaticMesh)
	{
		StaticMesh->InitResources();
	}

	// The component's light-maps are loaded from the transaction, so their resources need to be reinitialized.
	InitResources();

	// Debug check command trying to track down undo related uninitialized resource
	if (StaticMesh != NULL && StaticMesh->RenderData->LODResources.Num() > 0)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			ResourceCheckCommand,
			FRenderResource*, Resource, &StaticMesh->RenderData->LODResources[0].IndexBuffer,
			{
				check( Resource->IsInitialized() );
			}
		);
	}

	Super::PostEditUndo();
}

void UStaticMeshComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// Ensure that OverriddenLightMapRes is a factor of 4
	OverriddenLightMapRes = FMath::Max(OverriddenLightMapRes + 3 & ~3,4);

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged)
	{
		if (((PropertyThatChanged->GetName().Contains(TEXT("OverriddenLightMapRes")) ) && (bOverrideLightMapRes == true)) ||
			(PropertyThatChanged->GetName().Contains(TEXT("bOverrideLightMapRes")) ))
		{
			InvalidateLightingCache();
		}

		if ( PropertyThatChanged->GetName().Contains(TEXT("bIgnoreInstanceForTextureStreaming")) ||
			 PropertyThatChanged->GetName().Contains(TEXT("StreamingDistanceMultiplier")) )
		{
			GEngine->TriggerStreamingDataRebuild();
		}

		// Automatically change collision profile based on mobility and physics settings (if it is currently one of the default profiles)
		bool bMobilityChanged = PropertyThatChanged->GetName().Contains(TEXT("Mobility"));
		bool bSimulatePhysicsChanged = PropertyThatChanged->GetName().Contains(TEXT("bSimulatePhysics"));
		if( bMobilityChanged || bSimulatePhysicsChanged )
		{
			// If we enabled physics simulation, but we are not marked movable, do that for them
			if(bSimulatePhysicsChanged && BodyInstance.bSimulatePhysics && (Mobility != EComponentMobility::Movable))
			{
				SetMobility(EComponentMobility::Movable);
			}
			// If we made the component no longer movable, but simulation was enabled, disable that for them
			else if(bMobilityChanged && (Mobility != EComponentMobility::Movable) && BodyInstance.bSimulatePhysics)
			{
				BodyInstance.bSimulatePhysics = false;
			}

			// If the collision profile is one of the 'default' ones for a StaticMeshActor, make sure it is the correct one
			// If user has changed it to something else, don't touch it
			FName CurrentProfileName = BodyInstance.GetCollisionProfileName();
			if(	CurrentProfileName == UCollisionProfile::BlockAll_ProfileName ||
				CurrentProfileName == UCollisionProfile::BlockAllDynamic_ProfileName ||
				CurrentProfileName == UCollisionProfile::PhysicsActor_ProfileName )
			{
				if(Mobility == EComponentMobility::Movable)
				{
					if(BodyInstance.bSimulatePhysics)
					{
						BodyInstance.SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
					}
					else
					{
						BodyInstance.SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
					}
				}
				else
				{
					BodyInstance.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
				}
			}
		}
	}

	LightmassSettings.EmissiveBoost = FMath::Max(LightmassSettings.EmissiveBoost, 0.0f);
	LightmassSettings.DiffuseBoost = FMath::Max(LightmassSettings.DiffuseBoost, 0.0f);

	// Ensure properties are in sane range.
	SubDivisionStepSize = FMath::Clamp( SubDivisionStepSize, 1, 128 );

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR


void UStaticMeshComponent::PostLoad()
{
	Super::PostLoad();

	if ( StaticMesh )
	{
		StaticMesh->ConditionalPostLoad();
		CachePaintedDataIfNecessary();
		if ( FixupOverrideColorsIfNecessary() )
		{
#if WITH_EDITORONLY_DATA
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("MeshName"), FText::FromString(GetName()));
			FMessageLog("MapCheck").Info()
				->AddToken(FUObjectToken::Create(GetOuter()))
				->AddToken(FTextToken::Create(FText::Format( LOCTEXT( "MapCheck_Message_RepairedPaintedVertexColors", "{MeshName} : Repaired painted vertex colors" ), Arguments ) ))
				->AddToken(FMapErrorToken::Create(FMapErrors::RepairedPaintedVertexColors));
#endif
		}
	}

#if WITH_EDITORONLY_DATA
	// Remap the materials array if the static mesh materials may have been remapped to remove zero triangle sections.
	if (StaticMesh && GetLinkerUE4Version() < VER_UE4_REMOVE_ZERO_TRIANGLE_SECTIONS && Materials.Num())
	{
		StaticMesh->ConditionalPostLoad();
		if (StaticMesh->HasValidRenderData()
			&& StaticMesh->RenderData->MaterialIndexToImportIndex.Num())
		{
			TArray<UMaterialInterface*> OldMaterials;
			const TArray<int32>& MaterialIndexToImportIndex = StaticMesh->RenderData->MaterialIndexToImportIndex;

			Exchange(Materials,OldMaterials);
			Materials.Empty(MaterialIndexToImportIndex.Num());
			for (int32 MaterialIndex = 0; MaterialIndex < MaterialIndexToImportIndex.Num(); ++MaterialIndex)
			{
				UMaterialInterface* Material = NULL;
				int32 OldMaterialIndex = MaterialIndexToImportIndex[MaterialIndex];
				if (OldMaterials.IsValidIndex(OldMaterialIndex))
				{
					Material = OldMaterials[OldMaterialIndex];
				}
				Materials.Add(Material);
			}
		}

		if (Materials.Num() > StaticMesh->Materials.Num())
		{
			Materials.RemoveAt(StaticMesh->Materials.Num(), Materials.Num() - StaticMesh->Materials.Num());
		}
	}
#endif // #if WITH_EDITORONLY_DATA

	// Legacy content may contain a lightmap resolution of 0, which was valid when vertex lightmaps were supported, but not anymore with only texture lightmaps
	OverriddenLightMapRes = FMath::Max(OverriddenLightMapRes, 4);

	// Initialize the resources for the freshly loaded component.
	InitResources();
}

bool UStaticMeshComponent::SetStaticMesh(UStaticMesh* NewMesh)
{
	// Do nothing if we are already using the supplied static mesh
	if(NewMesh == StaticMesh)
	{
		return false;
	}

	// Don't allow changing static meshes if "static" and registered
	AActor* Owner = GetOwner();
	if(Mobility == EComponentMobility::Static && IsRegistered() && Owner != NULL)
	{
		FMessageLog("PIE").Warning(FText::Format(LOCTEXT("SetMeshOnStatic", "Calling SetStaticMesh on '{0}' but Mobility is Static."), 
			FText::FromString(GetPathName(this))));
		return false;
	}


	StaticMesh = NewMesh;

	// Need to send this to render thread at some point
	MarkRenderStateDirty();

	// Update physics representation right away
	RecreatePhysicsState();

	// Notify the streaming system. Don't use Update(), because this may be the first time the mesh has been set
	// and the component may have to be added to the streaming system for the first time.
	GStreamingManager->NotifyPrimitiveAttached( this, DPT_Spawned );

	// Since we have new mesh, we need to update bounds
	UpdateBounds();
	return true;
}

void UStaticMeshComponent::GetLocalBounds(FVector & Min, FVector & Max) const
{
	if (StaticMesh)
	{
		FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
		Min = MeshBounds.Origin - MeshBounds.BoxExtent;
		Max = MeshBounds.Origin + MeshBounds.BoxExtent;
	}
}

bool UStaticMeshComponent::UsesOnlyUnlitMaterials() const
{
	if (StaticMesh && StaticMesh->RenderData)
	{
		// Figure out whether any of the sections has a lit material assigned.
		bool bUsesOnlyUnlitMaterials = true;
		for (int32 LODIndex = 0; bUsesOnlyUnlitMaterials && LODIndex < StaticMesh->RenderData->LODResources.Num(); ++LODIndex)
		{
			FStaticMeshLODResources& LOD = StaticMesh->RenderData->LODResources[LODIndex];
			for (int32 ElementIndex=0; bUsesOnlyUnlitMaterials && ElementIndex<LOD.Sections.Num(); ElementIndex++)
			{
				UMaterialInterface*	MaterialInterface	= GetMaterial(LOD.Sections[ElementIndex].MaterialIndex);
				UMaterial*			Material			= MaterialInterface ? MaterialInterface->GetMaterial() : NULL;

				bUsesOnlyUnlitMaterials = Material && Material->GetLightingModel_Internal() == MLM_Unlit;
			}
		}
		return bUsesOnlyUnlitMaterials;
	}
	else
	{
		return false;
	}
}


bool UStaticMeshComponent::GetLightMapResolution( int32& Width, int32& Height ) const
{
	bool bPadded = false;
	if( StaticMesh )
	{
		// Use overriden per component lightmap resolution.
		if( bOverrideLightMapRes )
		{
			Width	= OverriddenLightMapRes;
			Height	= OverriddenLightMapRes;
		}
		// Use the lightmap resolution defined in the static mesh.
		else
		{
			Width	= StaticMesh->LightMapResolution;
			Height	= StaticMesh->LightMapResolution;
		}
		bPadded = true;
	}
	// No associated static mesh!
	else
	{
		Width	= 0;
		Height	= 0;
	}

	return bPadded;
}


void UStaticMeshComponent::GetEstimatedLightMapResolution(int32& Width, int32& Height) const
{
	if (StaticMesh)
	{
		ELightMapInteractionType LMIType = GetStaticLightingType();

		bool bUseSourceMesh = false;

		// Use overriden per component lightmap resolution.
		// If the overridden LM res is > 0, then this is what would be used...
		if (bOverrideLightMapRes == true)
		{
			if (OverriddenLightMapRes != 0)
			{
				Width	= OverriddenLightMapRes;
				Height	= OverriddenLightMapRes;
			}
		}
		else
		{
			bUseSourceMesh = true;
		}

		// Use the lightmap resolution defined in the static mesh.
		if (bUseSourceMesh == true)
		{
			Width	= StaticMesh->LightMapResolution;
			Height	= StaticMesh->LightMapResolution;
		}

		// If it was not set by anything, give it a default value...
		if (Width == 0)
		{
			int32 TempInt = 0;
			verify(GConfig->GetInt(TEXT("DevOptions.StaticLighting"), TEXT("DefaultStaticMeshLightingRes"), TempInt, GLightmassIni));

			Width	= TempInt;
			Height	= TempInt;
		}
	}
	else
	{
		Width	= 0;
		Height	= 0;
	}
}


int32 UStaticMeshComponent::GetStaticLightMapResolution() const
{
	int32 Width, Height;
	GetLightMapResolution(Width, Height);
	return FMath::Max<int32>(Width, Height);
}

bool UStaticMeshComponent::HasValidSettingsForStaticLighting() const
{
	int32 LightMapWidth = 0;
	int32 LightMapHeight = 0;
	GetLightMapResolution(LightMapWidth, LightMapHeight);

	return Super::HasValidSettingsForStaticLighting() 
		&& StaticMesh
		&& UsesTextureLightmaps(LightMapWidth, LightMapHeight);
}

bool UStaticMeshComponent::UsesTextureLightmaps(int32 InWidth, int32 InHeight) const
{
	return (
		(HasLightmapTextureCoordinates()) &&
		(InWidth > 0) && 
		(InHeight > 0)
		);
}


bool UStaticMeshComponent::HasLightmapTextureCoordinates() const
{
	if ((StaticMesh != NULL) &&
		(StaticMesh->LightMapCoordinateIndex >= 0) &&
		(StaticMesh->RenderData != NULL) &&
		(StaticMesh->RenderData->LODResources.Num() > 0) &&
		(StaticMesh->LightMapCoordinateIndex >= 0) &&	
		((uint32)StaticMesh->LightMapCoordinateIndex < StaticMesh->RenderData->LODResources[0].VertexBuffer.GetNumTexCoords()))
	{
		return true;
	}
	return false;
}


void UStaticMeshComponent::GetTextureLightAndShadowMapMemoryUsage(int32 InWidth, int32 InHeight, int32& OutLightMapMemoryUsage, int32& OutShadowMapMemoryUsage) const
{
	// Stored in texture.
	const float MIP_FACTOR = 1.33f;
	OutShadowMapMemoryUsage = FMath::Trunc(MIP_FACTOR * InWidth * InHeight); // G8
	if( AllowHighQualityLightmaps() )
	{
		OutLightMapMemoryUsage = FMath::Trunc(NUM_HQ_LIGHTMAP_COEF * MIP_FACTOR * InWidth * InHeight); // DXT5
	}
	else
	{
		OutLightMapMemoryUsage = FMath::Trunc(NUM_LQ_LIGHTMAP_COEF * MIP_FACTOR * InWidth * InHeight / 2); // DXT1
	}
}


void UStaticMeshComponent::GetLightAndShadowMapMemoryUsage( int32& LightMapMemoryUsage, int32& ShadowMapMemoryUsage ) const
{
	// Zero initialize.
	ShadowMapMemoryUsage = 0;
	LightMapMemoryUsage = 0;

	// Cache light/ shadow map resolution.
	int32 LightMapWidth = 0;
	int32	LightMapHeight = 0;
	GetLightMapResolution(LightMapWidth, LightMapHeight);

	// Determine whether static mesh/ static mesh component has static shadowing.
	if (HasStaticLighting() && StaticMesh)
	{
		// Determine whether we are using a texture or vertex buffer to store precomputed data.
		if (UsesTextureLightmaps(LightMapWidth, LightMapHeight) == true)
		{
			GetTextureLightAndShadowMapMemoryUsage(LightMapWidth, LightMapHeight, LightMapMemoryUsage, ShadowMapMemoryUsage);
		}
	}
}


bool UStaticMeshComponent::GetEstimatedLightAndShadowMapMemoryUsage( 
	int32& TextureLightMapMemoryUsage, int32& TextureShadowMapMemoryUsage,
	int32& VertexLightMapMemoryUsage, int32& VertexShadowMapMemoryUsage,
	int32& StaticLightingResolution, bool& bIsUsingTextureMapping, bool& bHasLightmapTexCoords) const
{
	TextureLightMapMemoryUsage	= 0;
	TextureShadowMapMemoryUsage	= 0;
	VertexLightMapMemoryUsage	= 0;
	VertexShadowMapMemoryUsage	= 0;
	bIsUsingTextureMapping		= false;
	bHasLightmapTexCoords		= false;

	// Cache light/ shadow map resolution.
	int32 LightMapWidth			= 0;
	int32 LightMapHeight		= 0;
	GetEstimatedLightMapResolution(LightMapWidth, LightMapHeight);
	StaticLightingResolution = LightMapWidth;

	int32 TrueLightMapWidth		= 0;
	int32 TrueLightMapHeight	= 0;
	GetLightMapResolution(TrueLightMapWidth, TrueLightMapHeight);

	// Determine whether static mesh/ static mesh component has static shadowing.
	if (HasStaticLighting() && StaticMesh)
	{
		// Determine whether we are using a texture or vertex buffer to store precomputed data.
		bHasLightmapTexCoords = HasLightmapTextureCoordinates();
		// Determine whether we are using a texture or vertex buffer to store precomputed data.
		bIsUsingTextureMapping = UsesTextureLightmaps(TrueLightMapWidth, TrueLightMapHeight);
		// Stored in texture.
		GetTextureLightAndShadowMapMemoryUsage(LightMapWidth, LightMapHeight, TextureLightMapMemoryUsage, TextureShadowMapMemoryUsage);

		return true;
	}

	return false;
}

int32 UStaticMeshComponent::GetNumMaterials() const
{
	if(StaticMesh)
	{
		return StaticMesh->Materials.Num();
	}
	else
	{
		return 0;
	}
}


UMaterialInterface* UStaticMeshComponent::GetMaterial(int32 MaterialIndex) const
{
	// If we have a base materials array, use that
	if(MaterialIndex < Materials.Num() && Materials[MaterialIndex])
	{
		return Materials[MaterialIndex];
	}
	// Otherwise get from static mesh
	else
	{
		return StaticMesh ? StaticMesh->GetMaterial(MaterialIndex) : NULL;
	}
}

void UStaticMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const
{
	if( StaticMesh && StaticMesh->RenderData )
	{
		for (int32 LODIndex = 0; LODIndex < StaticMesh->RenderData->LODResources.Num(); LODIndex++)
		{
			FStaticMeshLODResources& LODResources = StaticMesh->RenderData->LODResources[LODIndex];
			for (int32 SectionIndex = 0; SectionIndex < LODResources.Sections.Num(); SectionIndex++)
			{
				// Get the material for each element at the current lod index
				OutMaterials.AddUnique(GetMaterial(LODResources.Sections[SectionIndex].MaterialIndex));
			}
		}
	}
}

// Init type name static
const FName FLightMapInstanceData::LightMapInstanceDataTypeName(TEXT("LightMapInstanceData"));

void UStaticMeshComponent::GetComponentInstanceData(FComponentInstanceDataCache& Cache) const
{
	// Don't back up static lighting if there isn't any
	if(bHasCachedStaticLighting)
	{
		// Allocate new struct for holding light map data
		TSharedRef<FLightMapInstanceData> LightMapData = MakeShareable(new FLightMapInstanceData());

		// Fill in info
		LightMapData->StaticMesh = StaticMesh;
		LightMapData->Transform = ComponentToWorld;
		LightMapData->IrrelevantLights = IrrelevantLights;
		LightMapData->LODDataLightMap.Empty(LODData.Num());
		for (int32 i = 0; i < LODData.Num(); ++i)
		{
			LightMapData->LODDataLightMap.Add(LODData[i].LightMap);
			LightMapData->LODDataShadowMap.Add(LODData[i].ShadowMap);
		}

		// Add to cache
		Cache.AddInstanceData(LightMapData);
	}
}

void UStaticMeshComponent::ApplyComponentInstanceData(const FComponentInstanceDataCache& Cache)
{
	TArray< TSharedPtr<FComponentInstanceDataBase> > CachedData;
	Cache.GetInstanceDataOfType(FLightMapInstanceData::LightMapInstanceDataTypeName, CachedData);

	// Note: ApplyComponentInstanceData is called while the component is registered so the rendering thread is already using this component
	// That means all component state that is modified here must be mirrored on the scene proxy, which will be recreated to receive the changes later due to MarkRenderStateDirty.
	for(int32 DataIdx=0; DataIdx<CachedData.Num(); DataIdx++)
	{
		check(CachedData[DataIdx].IsValid());
		check(CachedData[DataIdx]->GetDataTypeName() == FLightMapInstanceData::LightMapInstanceDataTypeName);
		TSharedPtr<FLightMapInstanceData> LightMapData = StaticCastSharedPtr<FLightMapInstanceData>(CachedData[DataIdx]);

		// See if data matches current state
		if(	LightMapData->StaticMesh == StaticMesh &&
			LightMapData->Transform.Equals(ComponentToWorld) )
		{
			const int32 NumLODLightMaps = LightMapData->LODDataLightMap.Num();
			SetLODDataCount(NumLODLightMaps, NumLODLightMaps);
			for (int32 i = 0; i < NumLODLightMaps; ++i)
			{
				LODData[i].LightMap = LightMapData->LODDataLightMap[i];
				LODData[i].ShadowMap = LightMapData->LODDataShadowMap[i];
			}

			IrrelevantLights = LightMapData->IrrelevantLights;
			bHasCachedStaticLighting = true;

			MarkRenderStateDirty();
		}
	}
}

void UStaticMeshComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	if (StaticMesh != NULL && StaticMesh->SpeedTreeWind.IsValid())
	{
		GetScene()->AddSpeedTreeWind(StaticMesh);
	}
}

void UStaticMeshComponent::DestroyRenderState_Concurrent()
{
	Super::DestroyRenderState_Concurrent();

	if (StaticMesh != NULL && StaticMesh->SpeedTreeWind.IsValid())
	{
		GetScene()->RemoveSpeedTreeWind(StaticMesh);
	}
}

#include "AI/Navigation/RecastHelpers.h"
bool UStaticMeshComponent::DoCustomNavigableGeometryExport(struct FNavigableGeometryExport* GeomExport) const
{
	if (StaticMesh != NULL && StaticMesh->NavCollision != NULL)
	{
		UNavCollision* NavCollision = StaticMesh->NavCollision;
		if (NavCollision->bIsDynamicObstacle)
		{
			FCompositeNavModifier Modifiers;
			NavCollision->GetNavigationModifier(Modifiers, ComponentToWorld);
			GeomExport->AddNavModifiers(Modifiers);
			return false;
		}
				
		if (NavCollision->bHasConvexGeometry)
		{
			const FVector Scale3D = ComponentToWorld.GetScale3D();
			// if any of scales is 0 there's no point in exporting it
			if (!Scale3D.IsZero())
			{
				GeomExport->ExportCustomMesh(NavCollision->ConvexCollision.VertexBuffer.GetData(), NavCollision->ConvexCollision.VertexBuffer.Num(),
					NavCollision->ConvexCollision.IndexBuffer.GetData(), NavCollision->ConvexCollision.IndexBuffer.Num(), ComponentToWorld);

				GeomExport->ExportCustomMesh(NavCollision->TriMeshCollision.VertexBuffer.GetData(), NavCollision->TriMeshCollision.VertexBuffer.Num(),
					NavCollision->TriMeshCollision.IndexBuffer.GetData(), NavCollision->TriMeshCollision.IndexBuffer.Num(), ComponentToWorld);
			}

			// regardless of above we don't want "regular" collision export for this mesh instance
			return false;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
// StaticMeshComponentLODInfo



/** Default constructor */
FStaticMeshComponentLODInfo::FStaticMeshComponentLODInfo()
	: OverrideVertexColors(NULL)
#if WITH_EDITORONLY_DATA
	, Owner(NULL)
#endif
{
}

/** Copy constructor */
FStaticMeshComponentLODInfo::FStaticMeshComponentLODInfo(const FStaticMeshComponentLODInfo &rhs)
	: OverrideVertexColors(NULL)
#if WITH_EDITORONLY_DATA
	, Owner(NULL)
#endif
{
	if(rhs.OverrideVertexColors)
	{
		OverrideVertexColors = new FColorVertexBuffer;
		OverrideVertexColors->InitFromColorArray(&rhs.OverrideVertexColors->VertexColor(0), rhs.OverrideVertexColors->GetNumVertices());
	}
}

/** Destructor */
FStaticMeshComponentLODInfo::~FStaticMeshComponentLODInfo()
{
	ReleaseOverrideVertexColorsAndBlock();
}

void FStaticMeshComponentLODInfo::CleanUp()
{
	if(OverrideVertexColors)
	{
		DEC_DWORD_STAT_BY( STAT_InstVertexColorMemory, OverrideVertexColors->GetAllocatedSize() );
	}

	delete OverrideVertexColors;
	OverrideVertexColors = NULL;

	PaintedVertices.Empty();
}


void FStaticMeshComponentLODInfo::BeginReleaseOverrideVertexColors()
{
	if(OverrideVertexColors)
	{
		// enqueue a rendering command to release
		BeginReleaseResource(OverrideVertexColors);
	}
}

void FStaticMeshComponentLODInfo::ReleaseOverrideVertexColorsAndBlock()
{
	if(OverrideVertexColors)
	{
		// enqueue a rendering command to release
		BeginReleaseResource(OverrideVertexColors);
		// Ensure the RT no longer accessed the data, might slow down
		FlushRenderingCommands();
		// The RT thread has no access to it any more so it's safe to delete it.
		CleanUp();
	}
}

void FStaticMeshComponentLODInfo::ExportText(FString& ValueStr)
{
	ValueStr += FString::Printf(TEXT("PaintedVertices(%d)="), PaintedVertices.Num());

	// Rough approximation
	ValueStr.Reserve(ValueStr.Len() + PaintedVertices.Num() * 125);

	// Export the Position, Normal and Color info for each vertex
	for(int32 i = 0; i < PaintedVertices.Num(); ++i)
	{
		FPaintedVertex& Vert = PaintedVertices[i];

		ValueStr += FString::Printf(TEXT("((Position=(X=%.6f,Y=%.6f,Z=%.6f),"), Vert.Position.X, Vert.Position.Y, Vert.Position.Z);
		ValueStr += FString::Printf(TEXT("(Normal=(X=%d,Y=%d,Z=%d,W=%d),"), Vert.Normal.Vector.X, Vert.Normal.Vector.Y, Vert.Normal.Vector.Z, Vert.Normal.Vector.W);
		ValueStr += FString::Printf(TEXT("(Color=(B=%d,G=%d,R=%d,A=%d))"), Vert.Color.B, Vert.Color.G, Vert.Color.R, Vert.Color.A);

		// Seperate each vertex entry with a comma
		if ((i + 1) != PaintedVertices.Num())
		{
			ValueStr += TEXT(",");
		}
	}

	ValueStr += TEXT(" ");
}

void FStaticMeshComponentLODInfo::ImportText(const TCHAR** SourceText)
{
	FString TmpStr;
	int32 VertCount;
	if (FParse::Value(*SourceText, TEXT("PaintedVertices("), VertCount))
	{
		TmpStr = FString::Printf(TEXT("%d"), VertCount);
		*SourceText += TmpStr.Len() + 18;

		FString SourceTextStr = *SourceText;
		TArray<FString> Tokens;
		int32 TokenIdx = 0;
		bool bValidInput = true;

		// Tokenize the text
		SourceTextStr.ParseIntoArray(&Tokens, TEXT(","), false);

		// There should be 11 tokens per vertex
		check(Tokens.Num() * 11 >= VertCount);

		PaintedVertices.AddUninitialized(VertCount);

		for (int32 Idx = 0; Idx < VertCount; ++Idx)
		{
			// Position
			bValidInput &= FParse::Value(*Tokens[TokenIdx++], TEXT("X="), PaintedVertices[Idx].Position.X);
			bValidInput &= FParse::Value(*Tokens[TokenIdx++], TEXT("Y="), PaintedVertices[Idx].Position.Y);
			bValidInput &= FParse::Value(*Tokens[TokenIdx++], TEXT("Z="), PaintedVertices[Idx].Position.Z);
			// Normal
			bValidInput &= FParse::Value(*Tokens[TokenIdx++], TEXT("X="), PaintedVertices[Idx].Normal.Vector.X);
			bValidInput &= FParse::Value(*Tokens[TokenIdx++], TEXT("Y="), PaintedVertices[Idx].Normal.Vector.Y);
			bValidInput &= FParse::Value(*Tokens[TokenIdx++], TEXT("Z="), PaintedVertices[Idx].Normal.Vector.Z);
			bValidInput &= FParse::Value(*Tokens[TokenIdx++], TEXT("W="), PaintedVertices[Idx].Normal.Vector.W);
			// Color
			bValidInput &= FParse::Value(*Tokens[TokenIdx++], TEXT("B="), PaintedVertices[Idx].Color.B);
			bValidInput &= FParse::Value(*Tokens[TokenIdx++], TEXT("G="), PaintedVertices[Idx].Color.G);
			bValidInput &= FParse::Value(*Tokens[TokenIdx++], TEXT("R="), PaintedVertices[Idx].Color.R);
			bValidInput &= FParse::Value(*Tokens[TokenIdx++], TEXT("A="), PaintedVertices[Idx].Color.A);

			// Verify that the info for this vertex was read correctly
			check(bValidInput);
		}

		// Advance the text pointer past all of the data we just read
		int32 LODDataStrLen = 0;
		for (int32 Idx = 0; Idx < TokenIdx - 1; ++Idx)
		{
			LODDataStrLen += Tokens[Idx].Len() + 1;
		}
		*SourceText += LODDataStrLen;
	}
}

FArchive& operator<<(FArchive& Ar,FStaticMeshComponentLODInfo& I)
{
	const uint8 OverrideColorsStripFlag = 1;
	bool bStrippedOverrideColors = false;
#if WITH_EDITORONLY_DATA
	if( Ar.IsCooking() )
	{
		// Check if override color should be stripped too	
		int32 LODIndex = 0;
		for( ; LODIndex < I.Owner->LODData.Num() && &I != &I.Owner->LODData[ LODIndex ]; LODIndex++ )
		{}
		check( LODIndex < I.Owner->LODData.Num() );

		bStrippedOverrideColors = !I.OverrideVertexColors || 
			( I.Owner->StaticMesh == NULL || 
			I.Owner->StaticMesh->RenderData == NULL ||
			LODIndex >= I.Owner->StaticMesh->RenderData->LODResources.Num() ||
			I.OverrideVertexColors->GetNumVertices() != I.Owner->StaticMesh->RenderData->LODResources[LODIndex].VertexBuffer.GetNumVertices() );
	}
#endif // WITH_EDITORONLY_DATA
	FStripDataFlags StripFlags( Ar, bStrippedOverrideColors ? OverrideColorsStripFlag : 0 );

	if( !StripFlags.IsDataStrippedForServer() )
	{
		Ar << I.LightMap;

		if (Ar.UE4Ver() >= VER_UE4_PRECOMPUTED_SHADOW_MAPS)
		{
			Ar << I.ShadowMap;
		}
	}

	if( !StripFlags.IsClassDataStripped( OverrideColorsStripFlag ) )
	{
		// Bulk serialization (new method)
		uint8 bLoadVertexColorData = (I.OverrideVertexColors != NULL);
		Ar << bLoadVertexColorData;

		if(bLoadVertexColorData)
		{
			if(Ar.IsLoading())
			{
				check(!I.OverrideVertexColors);
				I.OverrideVertexColors = new FColorVertexBuffer;
			}

			I.OverrideVertexColors->Serialize( Ar, true );
		}
	}

	// Serialize out cached vertex information if necessary.
	if ( !StripFlags.IsEditorDataStripped() )
	{
		Ar << I.PaintedVertices;
	}

	// Empty when loading and we don't care about saving it again, like e.g. a client.
	if( Ar.IsLoading() && ( !GIsEditor && !IsRunningCommandlet() ) )
	{
		I.PaintedVertices.Empty();
	}

	return Ar;
}

#undef LOCTEXT_NAMESPACE
