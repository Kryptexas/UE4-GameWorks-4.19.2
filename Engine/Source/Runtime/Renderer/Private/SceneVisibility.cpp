// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SceneVisibility.cpp: Scene visibility determination.
=============================================================================*/

#include "RendererPrivate.h"
#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "FXSystem.h"
#include "../../Engine/Private/SkeletalRenderGPUSkin.h"		// GPrevPerBoneMotionBlur

/*------------------------------------------------------------------------------
	Globals
------------------------------------------------------------------------------*/

static float GWireframeCullThreshold = 5.0f;
static FAutoConsoleVariableRef CVarWireframeCullThreshold(
	TEXT("r.WireframeCullThreshold"),
	GWireframeCullThreshold,
	TEXT("Threshold below which objects in ortho wireframe views will be culled."),
	ECVF_RenderThreadSafe
	);

float GMinScreenRadiusForLights = 0.03f;
static FAutoConsoleVariableRef CVarMinScreenRadiusForLights(
	TEXT("r.MinScreenRadiusForLights"),
	GMinScreenRadiusForLights,
	TEXT("Threshold below which lights will be culled."),
	ECVF_RenderThreadSafe
	);

float GMinScreenRadiusForDepthPrepass = 0.03f;
static FAutoConsoleVariableRef CVarMinScreenRadiusForDepthPrepass(
	TEXT("r.MinScreenRadiusForDepthPrepass"),
	GMinScreenRadiusForDepthPrepass,
	TEXT("Threshold below which meshes will be culled from depth only pass."),
	ECVF_RenderThreadSafe
	);

float GMinScreenRadiusForCSMDepth = 0.01f;
static FAutoConsoleVariableRef CVarMinScreenRadiusForCSMDepth(
	TEXT("r.MinScreenRadiusForCSMDepth"),
	GMinScreenRadiusForCSMDepth,
	TEXT("Threshold below which meshes will be culled from CSM depth pass."),
	ECVF_RenderThreadSafe
	);

#if PLATFORM_MAC // @todo: disabled until rendering problems with HZB occlusion in OpenGL are solved
static int32 GHZBOcclusion = 0;
#else
static int32 GHZBOcclusion = 1;
#endif
static FAutoConsoleVariableRef CVarHZBOcclusion(
	TEXT("r.HZBOcclusion"),
	GHZBOcclusion,
	TEXT("Defines which occlusion system is used.\n")
	TEXT(" 0: Hardware occlusion queries\n")
	TEXT(" 1: Use HZB occlusion system (default, less GPU and CPU cost, more conservative results)"),
	ECVF_RenderThreadSafe
	);

static int32 GVisualizeOccludedPrimitives = 0;
static FAutoConsoleVariableRef CVarVisualizeOccludedPrimitives(
	TEXT("r.VisualizeOccludedPrimitives"),
	GVisualizeOccludedPrimitives,
	TEXT("Draw boxes for all occluded primitives"),
	ECVF_RenderThreadSafe | ECVF_Cheat
	);

static TAutoConsoleVariable<int32> CVarLightShaftQuality(
	TEXT("r.LightShaftQuality"),
	1,
	TEXT("Defines the light shaft quality.\n")
	TEXT("  0: off\n")
	TEXT("  1: on (default)\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

/** Distance fade cvars */
static int32 GDisableLODFade = false;
static FAutoConsoleVariableRef CVarDisableLODFade( TEXT("r.DisableLODFade"), GDisableLODFade, TEXT("Disable fading for distance culling"), ECVF_RenderThreadSafe );

static float GFadeTime = 0.25f;
static FAutoConsoleVariableRef CVarLODFadeTime( TEXT("r.LODFadeTime"), GFadeTime, TEXT("How long LOD takes to fade (in seconds)."), ECVF_RenderThreadSafe );

static float GDistanceFadeMaxTravel = 1000.0f;
static FAutoConsoleVariableRef CVarDistanceFadeMaxTravel( TEXT("r.DistanceFadeMaxTravel"), GDistanceFadeMaxTravel, TEXT("Max distance that the player can travel during the fade time."), ECVF_RenderThreadSafe );

/*------------------------------------------------------------------------------
	Visibility determination.
------------------------------------------------------------------------------*/

int8 ComputeLODForMeshes(
	const TIndirectArray<FStaticMesh>& StaticMeshes,
	float DistanceSquared,
	float MaxDrawDistanceScaleSquared,
	int32 ForcedLODLevel
	)
{
	int8 LODToRender = INDEX_NONE;
	int32 NumStaticMeshes = StaticMeshes.Num();

	if (ForcedLODLevel >= 0)
	{
		int8 MaxLOD = 0;
		for(int32 MeshIndex = 0;MeshIndex < NumStaticMeshes;MeshIndex++)
		{
			const FStaticMesh& StaticMesh = StaticMeshes[MeshIndex];
			MaxLOD = FMath::Max(MaxLOD, StaticMesh.LODIndex);
		}
		LODToRender = FMath::Clamp<int8>((int8)ForcedLODLevel, 0, MaxLOD);
	}
	else
	{
		for(int32 MeshIndex = 0;MeshIndex < NumStaticMeshes;MeshIndex++)
		{
			const FStaticMesh& StaticMesh = StaticMeshes[MeshIndex];
			if (DistanceSquared >= StaticMesh.MinDrawDistanceSquared 
				&& DistanceSquared < StaticMesh.MaxDrawDistanceSquared * MaxDrawDistanceScaleSquared)
			{
				LODToRender = StaticMesh.LODIndex;
				break;
			}
		}
	}
	return LODToRender;
}

/**
 * Update a primitive's fading state.
 * @param FadingState - State to update.
 * @param View - The view for which to update.
 * @param bVisible - Whether the primitive should be visible in the view.
 */
static void UpdatePrimitiveFadingState(FPrimitiveFadingState& FadingState, FViewInfo& View, bool bVisible)
{
	if (FadingState.bValid)
	{
		if (FadingState.bIsVisible != bVisible)
		{
			float CurrentRealTime = View.Family->CurrentRealTime;

			// Need to kick off a fade, so make sure that we have fading state for that
			if( !IsValidRef(FadingState.UniformBuffer) )
			{
				// Primitive is not currently fading.  Start a new fade!
				FadingState.EndTime = CurrentRealTime + GFadeTime;

				if( bVisible )
				{
					// Fading in
					// (Time - StartTime) / FadeTime
					FadingState.FadeTimeScaleBias.X = 1.0f / GFadeTime;
					FadingState.FadeTimeScaleBias.Y = -CurrentRealTime / GFadeTime;
				}
				else
				{
					// Fading out
					// 1 - (Time - StartTime) / FadeTime
					FadingState.FadeTimeScaleBias.X = -1.0f / GFadeTime;
					FadingState.FadeTimeScaleBias.Y = 1.0f + CurrentRealTime / GFadeTime;
				}

				FDistanceCullFadeUniformShaderParameters Uniforms;
				Uniforms.FadeTimeScaleBias = FadingState.FadeTimeScaleBias;
				FadingState.UniformBuffer = FDistanceCullFadeUniformBufferRef::CreateUniformBufferImmediate( Uniforms, UniformBuffer_MultiUse );
			}
			else
			{
				// Reverse fading direction but maintain current opacity
				// Solve for d: a*x+b = -a*x+d
				FadingState.FadeTimeScaleBias.Y = 2.0f * CurrentRealTime * FadingState.FadeTimeScaleBias.X + FadingState.FadeTimeScaleBias.Y;
				FadingState.FadeTimeScaleBias.X = -FadingState.FadeTimeScaleBias.X;
				
				if( bVisible )
				{
					// Fading in
					// Solve for x: a*x+b = 1
					FadingState.EndTime = ( 1.0f - FadingState.FadeTimeScaleBias.Y ) / FadingState.FadeTimeScaleBias.X;
				}
				else
				{
					// Fading out
					// Solve for x: a*x+b = 0
					FadingState.EndTime = -FadingState.FadeTimeScaleBias.Y / FadingState.FadeTimeScaleBias.X;
				}

				FDistanceCullFadeUniformShaderParameters Uniforms;
				Uniforms.FadeTimeScaleBias = FadingState.FadeTimeScaleBias;
				FadingState.UniformBuffer = FDistanceCullFadeUniformBufferRef::CreateUniformBufferImmediate( Uniforms, UniformBuffer_MultiUse );
			}
		}
	}

	FadingState.FrameNumber = View.FrameNumber;
	FadingState.bIsVisible = bVisible;
	FadingState.bValid = true;
}

bool FViewInfo::IsDistanceCulled( float DistanceSquared, float MinDrawDistance, float InMaxDrawDistance, const FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	float MaxDrawDistanceScale = GetCachedScalabilityCVars().ViewDistanceScale;
	float FadeRadius = GDisableLODFade ? 0.0f : GDistanceFadeMaxTravel;
	float MaxDrawDistance = InMaxDrawDistance * MaxDrawDistanceScale;

	// If cull distance is disabled, always show
	if (Family->EngineShowFlags.DistanceCulledPrimitives)
	{
		return false;
	}

	// The primitive is always culled if it exceeds the max fade distance.
	if (DistanceSquared > FMath::Square(MaxDrawDistance + FadeRadius) ||
		DistanceSquared < FMath::Square(MinDrawDistance))
	{
		return true;
	}

	const bool bDistanceCulled = (DistanceSquared > FMath::Square(MaxDrawDistance));
	const bool bMayBeFading = (DistanceSquared > FMath::Square(MaxDrawDistance - FadeRadius));

	bool bStillFading = false;
	if( !GDisableLODFade && bMayBeFading && State != NULL && !bDisableDistanceBasedFadeTransitions )
	{
		// Update distance-based visibility and fading state if it has not already been updated.
		int32 PrimitiveIndex = PrimitiveSceneInfo->GetIndex();
		FRelativeBitReference PrimitiveBit(PrimitiveIndex);
		if (PotentiallyFadingPrimitiveMap.AccessCorrespondingBit(PrimitiveBit) == false)
		{
			FPrimitiveFadingState& FadingState = ((FSceneViewState*)State)->PrimitiveFadingStates.FindOrAdd(PrimitiveSceneInfo->PrimitiveComponentId);
			UpdatePrimitiveFadingState(FadingState, *this, !bDistanceCulled);
			FUniformBufferRHIParamRef UniformBuffer = FadingState.UniformBuffer;
			bStillFading = (UniformBuffer != NULL);
			PrimitiveFadeUniformBuffers[PrimitiveIndex] = UniformBuffer;
			PotentiallyFadingPrimitiveMap.AccessCorrespondingBit(PrimitiveBit) = true;
		}
	}

	// If we're still fading then make sure the object is still drawn, even if it's beyond the max draw distance
	return ( bDistanceCulled && !bStillFading );
}

/**
 * Frustum cull primitives in the scene against the view.
 */
static int32 FrustumCull(const FScene* Scene, FViewInfo& View)
{
	SCOPE_CYCLE_COUNTER(STAT_FrustumCull);

	int32 NumCulledPrimitives = 0;
	FVector ViewOriginForDistanceCulling = View.ViewMatrices.ViewOrigin;
	float MaxDrawDistanceScale = GetCachedScalabilityCVars().ViewDistanceScale;
	float FadeRadius = GDisableLODFade ? 0.0f : GDistanceFadeMaxTravel;

	for (FSceneBitArray::FIterator BitIt(View.PrimitiveVisibilityMap); BitIt; ++BitIt)
	{
		const FPrimitiveBounds& Bounds = Scene->PrimitiveBounds[BitIt.GetIndex()];
		float DistanceSquared = (Bounds.Origin - ViewOriginForDistanceCulling).SizeSquared();
		float MaxDrawDistance = Bounds.MaxDrawDistance * MaxDrawDistanceScale;

		// If cull distance is disabled, always show
		if (View.Family->EngineShowFlags.DistanceCulledPrimitives)
		{
			MaxDrawDistance = FLT_MAX;
		}

		// The primitive is always culled if it exceeds the max fade distance or lay outside the view frustum.
		if (DistanceSquared > FMath::Square(MaxDrawDistance + FadeRadius) ||
			DistanceSquared < Bounds.MinDrawDistanceSq ||
			View.ViewFrustum.IntersectSphere(Bounds.Origin, Bounds.SphereRadius) == false ||
			View.ViewFrustum.IntersectBox(Bounds.Origin, Bounds.BoxExtent) == false)
		{
			STAT(NumCulledPrimitives++);
			continue;
		}

		if (DistanceSquared > FMath::Square(MaxDrawDistance))
		{
			View.PotentiallyFadingPrimitiveMap.AccessCorrespondingBit(BitIt) = true;
		}
		else
		{
			// The primitive is visible!
			BitIt.GetValue() = true;
			if (DistanceSquared > FMath::Square(MaxDrawDistance - FadeRadius))
			{
				View.PotentiallyFadingPrimitiveMap.AccessCorrespondingBit(BitIt) = true;
			}
		}
	}

	return NumCulledPrimitives;
}

/**
 * Updated primitive fading states for the view.
 */
static void UpdatePrimitiveFading(const FScene* Scene, FViewInfo& View)
{
	SCOPE_CYCLE_COUNTER(STAT_UpdatePrimitiveFading);

	FSceneViewState* ViewState = (FSceneViewState*)View.State;

	if (ViewState)
	{
		uint32 PrevFrameNumber = ViewState->PrevFrameNumber;
		float CurrentRealTime = View.Family->CurrentRealTime;

		// First clear any stale fading states.
		for (FPrimitiveFadingStateMap::TIterator It(ViewState->PrimitiveFadingStates); It; ++It)
		{
			FPrimitiveFadingState& FadingState = It.Value();
			if (FadingState.FrameNumber != PrevFrameNumber ||
				(IsValidRef(FadingState.UniformBuffer) && CurrentRealTime >= FadingState.EndTime))
			{
				It.RemoveCurrent();
			}
		}

		// Should we allow fading transitions at all this frame?  For frames where the camera moved
		// a large distance or where we haven't rendered a view in awhile, it's best to disable
		// fading so users don't see unexpected object transitions.
		if (!GDisableLODFade && !View.bDisableDistanceBasedFadeTransitions)
		{
			// Do a pass over potentially fading primitives and update their states.
			for (FSceneSetBitIterator BitIt(View.PotentiallyFadingPrimitiveMap); BitIt; ++BitIt)
			{
				bool bVisible = View.PrimitiveVisibilityMap.AccessCorrespondingBit(BitIt);
				FPrimitiveFadingState& FadingState = ViewState->PrimitiveFadingStates.FindOrAdd(Scene->PrimitiveComponentIds[BitIt.GetIndex()]);
				UpdatePrimitiveFadingState(FadingState, View, bVisible);
				FUniformBufferRHIParamRef UniformBuffer = FadingState.UniformBuffer;
				if (UniformBuffer && !bVisible)
				{
					// If the primitive is fading out make sure it remains visible.
					View.PrimitiveVisibilityMap.AccessCorrespondingBit(BitIt) = true;
				}
				View.PrimitiveFadeUniformBuffers[BitIt.GetIndex()] = UniformBuffer;
			}
		}
	}
}

/**
 * Cull occluded primitives in the view.
 */
static int32 OcclusionCull(const FScene* Scene, FViewInfo& View)
{
	SCOPE_CYCLE_COUNTER(STAT_OcclusionCull);

	// INITVIEWS_TODO: This could be more efficient if broken up in to separate concerns:
	// - What is occluded?
	// - For which primitives should we render occlusion queries?
	// - Generate occlusion query geometry.

	int32 NumOccludedPrimitives = 0;
	FSceneViewState* ViewState = (FSceneViewState*)View.State;
	
	// Disable HZB on OpenGL platforms to avoid rendering artefacts
	bool bHZBOcclusion = !IsOpenGLPlatform(GRHIShaderPlatform) && GHZBOcclusion;

	// Use precomputed visibility data if it is available.
	if (View.PrecomputedVisibilityData)
	{
		uint8 PrecomputedVisibilityFlags = EOcclusionFlags::CanBeOccluded | EOcclusionFlags::HasPrecomputedVisibility;
		for (FSceneSetBitIterator BitIt(View.PrimitiveVisibilityMap); BitIt; ++BitIt)
		{
			if ((Scene->PrimitiveOcclusionFlags[BitIt.GetIndex()] & PrecomputedVisibilityFlags) == PrecomputedVisibilityFlags)
			{
				FPrimitiveVisibilityId VisibilityId = Scene->PrimitiveVisibilityIds[BitIt.GetIndex()];
				if ((View.PrecomputedVisibilityData[VisibilityId.ByteIndex] & VisibilityId.BitMask) == 0)
				{
					View.PrimitiveVisibilityMap.AccessCorrespondingBit(BitIt) = false;
					INC_DWORD_STAT_BY(STAT_StaticallyOccludedPrimitives,1);
					STAT(NumOccludedPrimitives++);
				}
			}
		}
	}

	float CurrentRealTime = View.Family->CurrentRealTime;
	if (ViewState)
	{
		if (GRHIFeatureLevel >= ERHIFeatureLevel::SM3)
		{
			bool bClearQueries = !View.Family->EngineShowFlags.HitProxies;
			bool bSubmitQueries = !View.bDisableQuerySubmissions;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			bSubmitQueries = bSubmitQueries && !ViewState->HasViewParent() && !ViewState->bIsFrozen;
#endif

			if( bHZBOcclusion )
			{
				check(!ViewState->HZBOcclusionTests.IsValidFrame(View.FrameNumber));
				ViewState->HZBOcclusionTests.MapResults();
			}

			FViewElementPDI OcclusionPDI(&View, NULL);

			for (FSceneSetBitIterator BitIt(View.PrimitiveVisibilityMap); BitIt; ++BitIt)
			{
				uint8 OcclusionFlags = Scene->PrimitiveOcclusionFlags[BitIt.GetIndex()];
				bool bCanBeOccluded = (OcclusionFlags & EOcclusionFlags::CanBeOccluded) != 0;

				if(GIsEditor)
				{
					FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[BitIt.GetIndex()];

					if(PrimitiveSceneInfo->Proxy->IsSelected())
					{
						// to render occluded outline for selected objects
						bCanBeOccluded = false;
					}
				}

				FPrimitiveComponentId PrimitiveId = Scene->PrimitiveComponentIds[BitIt.GetIndex()];
				FPrimitiveOcclusionHistory* PrimitiveOcclusionHistory = ViewState->PrimitiveOcclusionHistorySet.Find(PrimitiveId);
				bool bIsOccluded = false;
				bool bOcclusionStateIsDefinite = false;
				if (!PrimitiveOcclusionHistory)
				{
					// If the primitive doesn't have an occlusion history yet, create it.
					PrimitiveOcclusionHistory = &ViewState->PrimitiveOcclusionHistorySet(
						ViewState->PrimitiveOcclusionHistorySet.Add(FPrimitiveOcclusionHistory(PrimitiveId))
						);

					// If the primitive hasn't been visible recently enough to have a history, treat it as unoccluded this frame so it will be rendered as an occluder and its true occlusion state can be determined.
					// already set bIsOccluded = false;

					// Flag the primitive's occlusion state as indefinite, which will force it to be queried this frame.
					// The exception is if the primitive isn't occludable, in which case we know that it's definitely unoccluded.
					bOcclusionStateIsDefinite = bCanBeOccluded ? false : true;
				}
				else
				{
					if (View.bIgnoreExistingQueries)
					{
						// If the view is ignoring occlusion queries, the primitive is definitely unoccluded.
						// already set bIsOccluded = false;
						bOcclusionStateIsDefinite = View.bDisableQuerySubmissions;
					}
					else if (bCanBeOccluded)
					{
						if( bHZBOcclusion )
						{
							if( ViewState->HZBOcclusionTests.IsValidFrame(PrimitiveOcclusionHistory->HZBTestFrameNumber) )
							{
								bIsOccluded = !ViewState->HZBOcclusionTests.IsVisible( PrimitiveOcclusionHistory->HZBTestIndex );
								bOcclusionStateIsDefinite = true;
							}
						}
						else
						{
							// Read the occlusion query results.
							uint64 NumSamples = 0;
							FRenderQueryRHIRef& PastQuery = PrimitiveOcclusionHistory->GetPastQuery(View.FrameNumber);
							if (IsValidRef(PastQuery))
							{
								// NOTE: RHIGetOcclusionQueryResult should never fail when using a blocking call, rendering artifacts may show up.
								if ( RHIGetRenderQueryResult(PastQuery,NumSamples,true) )
								{
									// we render occlusion without MSAA
									uint32 NumPixels = (uint32)NumSamples;

									// The primitive is occluded if none of its bounding box's pixels were visible in the previous frame's occlusion query.
									bIsOccluded = (NumPixels == 0);
									if (!bIsOccluded)
									{
										checkSlow(View.OneOverNumPossiblePixels > 0.0f);
										PrimitiveOcclusionHistory->LastPixelsPercentage = NumPixels * View.OneOverNumPossiblePixels;
									}
									else
									{
										PrimitiveOcclusionHistory->LastPixelsPercentage = 0.0f;
									}


									// Flag the primitive's occlusion state as definite if it wasn't grouped.
									bOcclusionStateIsDefinite = !PrimitiveOcclusionHistory->bGroupedQuery;
								}
								else
								{
									// If the occlusion query failed, treat the primitive as visible.  
									// already set bIsOccluded = false;
								}
							}
							else
							{
								// If there's no occlusion query for the primitive, set it's visibility state to whether it has been unoccluded recently.
								bIsOccluded = (PrimitiveOcclusionHistory->LastVisibleTime + GEngine->PrimitiveProbablyVisibleTime < CurrentRealTime);
								if (bIsOccluded)
								{
									PrimitiveOcclusionHistory->LastPixelsPercentage = 0.0f;
								}
								else
								{
									PrimitiveOcclusionHistory->LastPixelsPercentage = GEngine->MaxOcclusionPixelsFraction;
								}

								// the state was definite last frame, otherwise we would have ran a query
								bOcclusionStateIsDefinite = true;
							}
						}

						if( GVisualizeOccludedPrimitives && bIsOccluded )
						{
							const FBoxSphereBounds& Bounds = Scene->PrimitiveOcclusionBounds[ BitIt.GetIndex() ];
							DrawWireBox( &OcclusionPDI, Bounds.GetBox(), FColor(50, 255, 50), SDPG_Foreground );
						}
					}
					else
					{
						// Primitives that aren't occludable are considered definitely unoccluded.
						// already set bIsOccluded = false;
						bOcclusionStateIsDefinite = true;
					}

					if (bClearQueries)
					{
						ViewState->OcclusionQueryPool.ReleaseQuery( PrimitiveOcclusionHistory->GetPastQuery(View.FrameNumber) );
					}
				}

				// Set the primitive's considered time to keep its occlusion history from being trimmed.
				PrimitiveOcclusionHistory->LastConsideredTime = CurrentRealTime;

				if (bSubmitQueries && bCanBeOccluded)
				{
					bool bAllowBoundsTest;
					const FBoxSphereBounds& OcclusionBounds = Scene->PrimitiveOcclusionBounds[BitIt.GetIndex()];
					if (View.bHasNearClippingPlane)
					{
						bAllowBoundsTest = View.NearClippingPlane.PlaneDot(OcclusionBounds.Origin) < 
							-(FVector::BoxPushOut(View.NearClippingPlane,OcclusionBounds.BoxExtent));
					}
					else
					{
						bAllowBoundsTest = OcclusionBounds.SphereRadius < HALF_WORLD_MAX;
					}

					if (bAllowBoundsTest)
					{
						if( bHZBOcclusion )
						{
							// Always run
							PrimitiveOcclusionHistory->HZBTestIndex = ViewState->HZBOcclusionTests.AddBounds( OcclusionBounds.Origin, OcclusionBounds.BoxExtent );
							PrimitiveOcclusionHistory->HZBTestFrameNumber = View.FrameNumber;
						}
						else
						{
							// decide if a query should be run this frame
							bool bRunQuery,bGroupedQuery;

							if (OcclusionFlags & EOcclusionFlags::AllowApproximateOcclusion)
							{
								if (bIsOccluded)
								{
									// Primitives that were occluded the previous frame use grouped queries.
									bGroupedQuery = true;
									bRunQuery = true;
								}
								else if (bOcclusionStateIsDefinite)
								{
									// If the primitive's is definitely unoccluded, only requery it occasionally.
									float FractionMultiplier = FMath::Max(PrimitiveOcclusionHistory->LastPixelsPercentage/GEngine->MaxOcclusionPixelsFraction, 1.0f);
									bRunQuery = (FractionMultiplier * GOcclusionRandomStream.GetFraction() < GEngine->MaxOcclusionPixelsFraction);
									bGroupedQuery = false;
								}
								else
								{
									bGroupedQuery = false;
									bRunQuery = true;
								}
							}
							else
							{
								// Primitives that need precise occlusion results use individual queries.
								bGroupedQuery = false;
								bRunQuery = true;
							}

							if (bRunQuery)
							{
								PrimitiveOcclusionHistory->SetCurrentQuery(View.FrameNumber, 
									bGroupedQuery ? 
									View.GroupedOcclusionQueries.BatchPrimitive(OcclusionBounds.Origin + View.ViewMatrices.PreViewTranslation,OcclusionBounds.BoxExtent) :
									View.IndividualOcclusionQueries.BatchPrimitive(OcclusionBounds.Origin + View.ViewMatrices.PreViewTranslation,OcclusionBounds.BoxExtent)
									);	
							}
							PrimitiveOcclusionHistory->bGroupedQuery = bGroupedQuery;
						}
					}
					else
					{
						// If the primitive's bounding box intersects the near clipping plane, treat it as definitely unoccluded.
						bIsOccluded = false;
						bOcclusionStateIsDefinite = true;
					}
				}

				if (bIsOccluded)
				{
					View.PrimitiveVisibilityMap.AccessCorrespondingBit(BitIt) = false;
					STAT(NumOccludedPrimitives++);
				}
				else if (bOcclusionStateIsDefinite)
				{
					PrimitiveOcclusionHistory->LastVisibleTime = CurrentRealTime;
					View.PrimitiveDefinitelyUnoccludedMap.AccessCorrespondingBit(BitIt) = true;
				}
			}

			if( bHZBOcclusion )
			{
				ViewState->HZBOcclusionTests.UnmapResults();

				if( bSubmitQueries )
				{
					ViewState->HZBOcclusionTests.SetValidFrameNumber(View.FrameNumber);
				}
			}
		}
		else
		{
			// No occlusion queries, so mark primitives as not occluded
			for (FSceneSetBitIterator BitIt(View.PrimitiveVisibilityMap); BitIt; ++BitIt)
			{
				View.PrimitiveDefinitelyUnoccludedMap.AccessCorrespondingBit(BitIt) = true;
			}
		}
	}

	return NumOccludedPrimitives;
}

/** List of relevant static primitives. Indexes into Scene->Primitives. */
typedef TArray<int32, SceneRenderingAllocator> FRelevantStaticPrimitives;

/**
 * Masks indicating for which views a primitive needs to have PreRenderView
 * called. One entry per primitive in the scene.
 */
typedef TArray<uint8, SceneRenderingAllocator> FPrimitivePreRenderViewMasks;

/**
 * Computes view relevance for visible primitives in the view and adds them to
 * appropriate per-view rendering lists.
 * @param Scene - The scene being rendered.
 * @param View - The view for which to compute relevance.
 * @param ViewBit - Bit mask: 1 << ViewIndex where Views(ViewIndex) == View.
 * @param OutRelevantStaticPrimitives - Upon return contains a list of relevant
 *                                      static primitives.
 * @param OutPreRenderViewMasks - Primitives requiring the PreRenderView
 *                                callback for this view will have ViewBit set.
 */
static void ComputeRelevanceForView(
	const FScene* Scene,
	FViewInfo& View,
	uint8 ViewBit,
	FRelevantStaticPrimitives& OutRelevantStaticPrimitives,
	FPrimitivePreRenderViewMasks& OutPreRenderViewMasks
	)
{
	SCOPE_CYCLE_COUNTER(STAT_ComputeViewRelevance);

	// INITVIEWS_TODO: This may be more efficient to break in to:
	// - Compute view relevance.
	// - Gather relevant primitives.
	// - Update last render time.
	// But it's hard to say since the view relevance is going to be available right now.

	check(OutPreRenderViewMasks.Num() == Scene->Primitives.Num());

	float CurrentWorldTime = View.Family->CurrentWorldTime;
	float DeltaWorldTime = View.Family->DeltaWorldTime;

	//OutRelevantStaticPrimitives.Empty(NumPrimitives);
	for (FSceneSetBitIterator BitIt(View.PrimitiveVisibilityMap); BitIt; ++BitIt)
	{
		FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[BitIt.GetIndex()];
		FPrimitiveViewRelevance& ViewRelevance = View.PrimitiveViewRelevanceMap[BitIt.GetIndex()];
		ViewRelevance = PrimitiveSceneInfo->Proxy->GetViewRelevance(&View);
		ViewRelevance.bInitializedThisFrame = true;

		const bool bStaticRelevance = ViewRelevance.bStaticRelevance;
		const bool bDrawRelevance = ViewRelevance.bDrawRelevance;
		const bool bDynamicRelevance = ViewRelevance.bDynamicRelevance;
		const bool bShadowRelevance = ViewRelevance.bShadowRelevance;
		const bool bEditorRelevance = ViewRelevance.bEditorPrimitiveRelevance;
		const bool bTranslucentRelevance = ViewRelevance.HasTranslucency();

		if (bStaticRelevance && (bDrawRelevance || bShadowRelevance))
		{
			OutRelevantStaticPrimitives.Add(BitIt.GetIndex());
		}

		if (!bDrawRelevance)
		{
			View.PrimitiveVisibilityMap.AccessCorrespondingBit(BitIt) = false;
			continue;
		}

		if (ViewRelevance.bNeedsPreRenderView)
		{
			OutPreRenderViewMasks[BitIt.GetIndex()] |= ViewBit;
		}

		if (bEditorRelevance)
		{
			// Editor primitives are rendered after post processing and composited onto the scene
			View.VisibleEditorPrimitives.Add(PrimitiveSceneInfo);
		}
		else if(bDynamicRelevance)
		{
			// Keep track of visible dynamic primitives.
			View.VisibleDynamicPrimitives.Add(PrimitiveSceneInfo);
		}

		if (ViewRelevance.HasTranslucency() && !bEditorRelevance && ViewRelevance.bRenderInMainPass)
		{
			// Add to set of dynamic translucent primitives
			View.TranslucentPrimSet.AddScenePrimitive(PrimitiveSceneInfo, View, ViewRelevance.bNormalTranslucencyRelevance, ViewRelevance.bSeparateTranslucencyRelevance);

			if (ViewRelevance.bDistortionRelevance)
			{
				// Add to set of dynamic distortion primitives
				View.DistortionPrimSet.AddScenePrimitive(PrimitiveSceneInfo->Proxy, View);
			}
		}
		
		if (ViewRelevance.bRenderCustomDepth)
		{
			// Add to set of dynamic distortion primitives
			View.CustomDepthSet.AddScenePrimitive(PrimitiveSceneInfo->Proxy, View);
		}

		// INITVIEWS_TODO: Do this in a separate pass? There are no dependencies
		// here except maybe ParentPrimitives. This could be done in a 
		// low-priority background task and forgotten about.

		// If the primitive's last render time is older than last frame, consider
		// it newly visible and update its visibility change time
		if (PrimitiveSceneInfo->LastRenderTime < CurrentWorldTime - DeltaWorldTime - DELTA)
		{
			PrimitiveSceneInfo->LastVisibilityChangeTime = CurrentWorldTime;
		}
		PrimitiveSceneInfo->LastRenderTime = CurrentWorldTime;

		// If the primitive is definitely unoccluded or if in Wireframe mode and the primitive is estimated
		// to be unoccluded, then update the primitive components's LastRenderTime 
		// on the game thread. This signals that the primitive is visible.
		if (View.PrimitiveDefinitelyUnoccludedMap.AccessCorrespondingBit(BitIt) || (View.Family->EngineShowFlags.Wireframe && View.PrimitiveVisibilityMap.AccessCorrespondingBit(BitIt)))
		{
			// Update the PrimitiveComponent's LastRenderTime.
			*(PrimitiveSceneInfo->ComponentLastRenderTime) = CurrentWorldTime;
		}

		// Cache the nearest reflection proxy if needed
		if (PrimitiveSceneInfo->bNeedsCachedReflectionCaptureUpdate
			// In ES2, the per-object reflection is used for everything
			// Otherwise it is just used on translucency
			&& (GRHIFeatureLevel == ERHIFeatureLevel::ES2 || bTranslucentRelevance))
		{
			PrimitiveSceneInfo->CachedReflectionCaptureProxy = Scene->FindClosestReflectionCapture(Scene->PrimitiveBounds[BitIt.GetIndex()].Origin);
			PrimitiveSceneInfo->bNeedsCachedReflectionCaptureUpdate = false;
		}

		PrimitiveSceneInfo->ConditionalUpdateStaticMeshes();
	}
}

static void MarkAllPrimitivesForReflectionProxyUpdate(FScene* Scene)
{
	if (Scene->ReflectionSceneData.bRegisteredReflectionCapturesHasChanged)
	{
		// Mark all primitives as needing an update
		// Note: Only visible primitives will actually update their reflection proxy
		for (int32 PrimitiveIndex = 0; PrimitiveIndex < Scene->Primitives.Num(); PrimitiveIndex++)
		{
			Scene->Primitives[PrimitiveIndex]->bNeedsCachedReflectionCaptureUpdate = true;
		}

		Scene->ReflectionSceneData.bRegisteredReflectionCapturesHasChanged = false;
	}
}

/**
 * Determines which static meshes in the scene are relevant to the view based on
 * for each primitive in RelevantStaticPrimitives.
 */
static void MarkRelevantStaticMeshesForView(
	const FScene* Scene,
	FViewInfo& View,
	const FRelevantStaticPrimitives& RelevantStaticPrimitives
	)
{
	SCOPE_CYCLE_COUNTER(STAT_StaticRelevance);

	FVector ViewOrigin = View.ViewMatrices.ViewOrigin;

	float MaxDrawDistanceScaleSquared = GetCachedScalabilityCVars().ViewDistanceScaleSquared;

	// outside of the loop to be more efficient
	int32 ForcedLODLevel = (View.Family->EngineShowFlags.LOD) ? GetCVarForceLOD() : 0;
	extern TAutoConsoleVariable<int32> CVarEarlyZPass;
	const bool bForceEarlyZPass = CVarEarlyZPass.GetValueOnRenderThread() == 2;

	for (int32 StaticPrimIndex = 0; StaticPrimIndex < RelevantStaticPrimitives.Num(); ++StaticPrimIndex)
	{
		int32 PrimitiveIndex = RelevantStaticPrimitives[StaticPrimIndex];
		FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[PrimitiveIndex];
		const FPrimitiveBounds& Bounds = Scene->PrimitiveBounds[PrimitiveIndex];
		const FPrimitiveViewRelevance& ViewRelevance = View.PrimitiveViewRelevanceMap[PrimitiveIndex];

		static const auto* StaticMeshLODDistanceScale = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.StaticMeshLODDistanceScale"));
		check(StaticMeshLODDistanceScale);
		const float LODScale = StaticMeshLODDistanceScale->GetValueOnRenderThread();

		float DistanceSquared = (Bounds.Origin - ViewOrigin).SizeSquared();
		const float LODFactorDistanceSquared = DistanceSquared * FMath::Square(View.LODDistanceFactor * (1.0f / LODScale));

		// Go through the meshes and find the LOD to render
		int8 LODToRender = ComputeLODForMeshes(
			PrimitiveSceneInfo->StaticMeshes,
			LODFactorDistanceSquared,
			MaxDrawDistanceScaleSquared,
			ForcedLODLevel
			);

		const bool bDrawShadowDepth = FMath::Square(Bounds.SphereRadius) > GMinScreenRadiusForCSMDepth * GMinScreenRadiusForCSMDepth * LODFactorDistanceSquared;
		const bool bDrawDepthOnly = bForceEarlyZPass || FMath::Square(Bounds.SphereRadius) > GMinScreenRadiusForDepthPrepass * GMinScreenRadiusForDepthPrepass * LODFactorDistanceSquared;
		
		// We could compute this only if required by the following code. Not sure if that is worth.
		const bool bShouldRenderVelocity = PrimitiveSceneInfo->ShouldRenderVelocity(View);

		const int32 NumStaticMeshes = PrimitiveSceneInfo->StaticMeshes.Num();
		for(int32 MeshIndex = 0;MeshIndex < NumStaticMeshes;MeshIndex++)
		{
			const FStaticMesh& StaticMesh = PrimitiveSceneInfo->StaticMeshes[MeshIndex];
			if (StaticMesh.LODIndex == LODToRender)
			{
				bool bNeedsBatchVisibility = false;

				if (ViewRelevance.bShadowRelevance && bDrawShadowDepth && StaticMesh.CastShadow)
				{
					// Mark static mesh as visible in shadows.
					View.StaticMeshShadowDepthMap[StaticMesh.Id] = true;
					bNeedsBatchVisibility = true;
				}
				
				if(ViewRelevance.bDrawRelevance && !StaticMesh.bShadowOnly && (ViewRelevance.bRenderInMainPass || ViewRelevance.bRenderCustomDepth))
				{
					// Mark static mesh as visible for rendering
					View.StaticMeshVisibilityMap[StaticMesh.Id] = true;
					View.StaticMeshVelocityMap[StaticMesh.Id] = bShouldRenderVelocity;
					++View.NumVisibleStaticMeshElements;

					// If the static mesh is an occluder, check whether it covers enough of the screen to be used as an occluder.
					if(	StaticMesh.bUseAsOccluder && bDrawDepthOnly )
					{
						View.StaticMeshOccluderMap[StaticMesh.Id] = true;
					}
					bNeedsBatchVisibility = true;
				}

				// Static meshes with a single element always draw, as if the mask were 0x1.
				if(bNeedsBatchVisibility && StaticMesh.Elements.Num() > 1)
				{
					View.StaticMeshBatchVisibility[StaticMesh.Id] = StaticMesh.VertexFactory->GetStaticBatchElementVisibility(View, &StaticMesh);
				}
			}
		}
	}
}

/**
 * Dispatches PreRenderView callbacks to all primitives with the provided view
 * masks.
 */
static void DispatchPreRenderView(
	const FScene* Scene,
	const FSceneViewFamily* ViewFamily,
	const FPrimitivePreRenderViewMasks& PreRenderViewMasks,
	uint32 FrameNumber
	)
{
	SCOPE_CYCLE_COUNTER(STAT_PreRenderView);

	int32 NumPrimitives = Scene->Primitives.Num();
	check(PreRenderViewMasks.Num() == NumPrimitives);

	for (int32 PrimitiveIndex = 0; PrimitiveIndex < NumPrimitives; ++PrimitiveIndex)
	{
		uint8 ViewMask = PreRenderViewMasks[PrimitiveIndex];
		if (ViewMask != 0)
		{
			FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[PrimitiveIndex];
			FScopeCycleCounter Context(PrimitiveSceneInfo->Proxy->GetStatId());
			PrimitiveSceneInfo->Proxy->PreRenderView(ViewFamily, ViewMask, FrameNumber);
		}
	}
}

/**
 * Helper for InitViews to detect large camera movement, in both angle and position.
 */
static bool IsLargeCameraMovement(FSceneView& View, const FMatrix& PrevViewMatrix, const FVector& PrevViewOrigin, float CameraRotationThreshold, float CameraTranslationThreshold)
{
	float RotationThreshold = FMath::Cos(CameraRotationThreshold * PI / 180.0f);
	float ViewRightAngle = View.ViewMatrices.ViewMatrix.GetColumn(0) | PrevViewMatrix.GetColumn(0);
	float ViewUpAngle = View.ViewMatrices.ViewMatrix.GetColumn(1) | PrevViewMatrix.GetColumn(1);
	float ViewDirectionAngle = View.ViewMatrices.ViewMatrix.GetColumn(2) | PrevViewMatrix.GetColumn(2);

	FVector Distance = FVector(View.ViewMatrices.ViewOrigin) - PrevViewOrigin;
	return 
		ViewRightAngle < RotationThreshold ||
		ViewUpAngle < RotationThreshold ||
		ViewDirectionAngle < RotationThreshold ||
		Distance.SizeSquared() > CameraTranslationThreshold * CameraTranslationThreshold;
}

float Halton( int32 Index, int32 Base )
{
	float Result = 0.0f;
	float InvBase = 1.0f / Base;
	float Fraction = InvBase;
	while( Index > 0 )
	{
		Result += ( Index % Base ) * Fraction;
		Index /= Base;
		Fraction *= InvBase;
	}
	return Result;
}

void FSceneRenderer::PreVisibilityFrameSetup()
{
	// Notify the FX system that the scene is about to perform visibility checks.
	if (Scene->FXSystem)
	{
		Scene->FXSystem->PreInitViews();
	}

	// Draw lines to lights affecting this mesh if its selected.
	if (ViewFamily.EngineShowFlags.LightInfluences)
	{
		for (TArray<FPrimitiveSceneInfo*>::TConstIterator It(Scene->Primitives); It; ++It)
		{
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = *It;
			if (PrimitiveSceneInfo->Proxy->IsSelected())
			{
				FLightPrimitiveInteraction *LightList = PrimitiveSceneInfo->LightList;
				while (LightList)
				{
					const FLightSceneInfo* LightSceneInfo = LightList->GetLight();

					bool bDynamic = true;
					bool bRelevant = false;
					bool bLightMapped = true;
					bool bShadowMapped = false;
					PrimitiveSceneInfo->Proxy->GetLightRelevance(LightSceneInfo->Proxy, bDynamic, bRelevant, bLightMapped, bShadowMapped);

					if (bRelevant)
					{
						// Draw blue for light-mapped lights and orange for dynamic lights
						const FColor LineColor = bLightMapped ? FColor(0,140,255) : FColor(255,140,0);
						for (int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
						{
							FViewInfo& View = Views[ViewIndex];
							FViewElementPDI LightInfluencesPDI(&View,NULL);
							LightInfluencesPDI.DrawLine(PrimitiveSceneInfo->Proxy->GetBounds().Origin, LightSceneInfo->Proxy->GetLightToWorld().GetOrigin(), LineColor, SDPG_World);
						}
					}
					LightList = LightList->GetNextLight();
				}
			}
		}
	}

	// Setup motion blur parameters (also check for camera movement thresholds)
	for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		FViewInfo& View = Views[ViewIndex];
		View.FrameNumber = FrameNumber;
		FSceneViewState* ViewState = (FSceneViewState*) View.State;
		static bool bEnableTimeScale = true;

		// HighResScreenshot should get best results so we don't do the occlusion optimization based on the former frame
		extern bool GIsHighResScreenshot;
		const bool bIsHitTesting = ViewFamily.EngineShowFlags.HitProxies;
		if (GIsHighResScreenshot || !DoOcclusionQueries() || bIsHitTesting)
		{
			View.bDisableQuerySubmissions = true;
			View.bIgnoreExistingQueries = true;
		}

		// set up the screen area for occlusion
		float NumPossiblePixels = GSceneRenderTargets.UseDownsizedOcclusionQueries() && IsValidRef(GSceneRenderTargets.GetSmallDepthSurface()) ? 
			(float)View.ViewRect.Width() / GSceneRenderTargets.GetSmallColorDepthDownsampleFactor() * (float)View.ViewRect.Height() / GSceneRenderTargets.GetSmallColorDepthDownsampleFactor() :
			View.ViewRect.Width() * View.ViewRect.Height();
		View.OneOverNumPossiblePixels = NumPossiblePixels > 0.0 ? 1.0f / NumPossiblePixels : 0.0f;


		// Subpixel jitter for temporal AA
		static const auto CVarTemporalAASamples = IConsoleManager::Get().FindTConsoleVariableDataInt( TEXT("r.TemporalAASamples") );

		// Still need no jitter to be set for temporal feedback on SSR (it is enabled even when temporal AA is off).
		View.TemporalJitterPixelsX = 0.0f;
		View.TemporalJitterPixelsY = 0.0f;

		if( View.FinalPostProcessSettings.AntiAliasingMethod == AAM_TemporalAA && ViewState )
		{
			int32 TemporalAASamples = CVarTemporalAASamples->GetValueOnRenderThread();
		
			if( TemporalAASamples > 1 )
			{
				float SampleX, SampleY;

				if( GRHIFeatureLevel < ERHIFeatureLevel::SM4 )
				{
					// Only support 2 samples for mobile temporal AA.
					TemporalAASamples = 2;
				}

				if( TemporalAASamples == 2 )
				{
					#if 0
						// 2xMSAA
						// Pattern docs: http://msdn.microsoft.com/en-us/library/windows/desktop/ff476218(v=vs.85).aspx
						//   N.
						//   .S
						float SamplesX[] = { -4.0f/16.0f, 4.0/16.0f };
						float SamplesY[] = { -4.0f/16.0f, 4.0/16.0f };
					#else
						// This pattern is only used for mobile.
						// Shift to reduce blur.
						float SamplesX[] = { -8.0f/16.0f, 0.0/16.0f };
						float SamplesY[] = { /* - */ 0.0f/16.0f, 8.0/16.0f };
					#endif
					ViewState->SetupTemporalAA(ARRAY_COUNT(SamplesX));
					uint32 Index = ViewState->GetCurrentTemporalAASampleIndex();
					SampleX = SamplesX[ Index ];
					SampleY = SamplesY[ Index ];
				}
				else if( TemporalAASamples == 3 )
				{
					// 3xMSAA
					//   A..
					//   ..B
					//   .C.
					// Rolling circle pattern (A,B,C).
					float SamplesX[] = { -2.0f/3.0f,  2.0/3.0f,  0.0/3.0f };
					float SamplesY[] = { -2.0f/3.0f,  0.0/3.0f,  2.0/3.0f };
					ViewState->SetupTemporalAA(ARRAY_COUNT(SamplesX));
					uint32 Index = ViewState->GetCurrentTemporalAASampleIndex();
					SampleX = SamplesX[ Index ];
					SampleY = SamplesY[ Index ];
				}
				else if( TemporalAASamples == 4 )
				{
					// 4xMSAA
					// Pattern docs: http://msdn.microsoft.com/en-us/library/windows/desktop/ff476218(v=vs.85).aspx
					//   .N..
					//   ...E
					//   W...
					//   ..S.
					// Rolling circle pattern (N,E,S,W).
					float SamplesX[] = { -2.0f/16.0f,  6.0/16.0f, 2.0/16.0f, -6.0/16.0f };
					float SamplesY[] = { -6.0f/16.0f, -2.0/16.0f, 6.0/16.0f,  2.0/16.0f };
					ViewState->SetupTemporalAA(ARRAY_COUNT(SamplesX));
					uint32 Index = ViewState->GetCurrentTemporalAASampleIndex();
					SampleX = SamplesX[ Index ];
					SampleY = SamplesY[ Index ];
				}
				else if( TemporalAASamples == 5 )
				{
					// Compressed 4 sample pattern on same vertical and horizontal line (less temporal flicker).
					// Compressed 1/2 works better than correct 2/3 (reduced temporal flicker).
					//   . N .
					//   W . E
					//   . S .
					// Rolling circle pattern (N,E,S,W).
					float SamplesX[] = {  0.0f/2.0f,  1.0/2.0f,  0.0/2.0f, -1.0/2.0f };
					float SamplesY[] = { -1.0f/2.0f,  0.0/2.0f,  1.0/2.0f,  0.0/2.0f };
					ViewState->SetupTemporalAA(ARRAY_COUNT(SamplesX));
					uint32 Index = ViewState->GetCurrentTemporalAASampleIndex();
					SampleX = SamplesX[ Index ];
					SampleY = SamplesY[ Index ];
				}
				else if( TemporalAASamples == 8 )
				{
					// This works better than various orderings of 8xMSAA.
					ViewState->SetupTemporalAA(8);
					uint32 Index = ViewState->GetCurrentTemporalAASampleIndex();
					SampleX = Halton( Index, 2 ) - 0.5f;
					SampleY = Halton( Index, 3 ) - 0.5f;
				}
				else
				{
					// More than 8 samples can improve quality.
					ViewState->SetupTemporalAA(TemporalAASamples);
					uint32 Index = ViewState->GetCurrentTemporalAASampleIndex();
					SampleX = Halton( Index, 2 ) - 0.5f;
					SampleY = Halton( Index, 3 ) - 0.5f;
				}

				View.TemporalJitterPixelsX = SampleX;
				View.TemporalJitterPixelsY = SampleY;

				float JitterX = ( SampleX ) * 2.0f / View.ViewRect.Width();
				float JitterY = ( SampleY ) * 2.0f / View.ViewRect.Height();

				View.ViewMatrices.ProjMatrix.M[2][0] += JitterX;
				View.ViewMatrices.ProjMatrix.M[2][1] += JitterY;

				// Compute the view projection matrix and its inverse.
				View.ViewProjectionMatrix = View.ViewMatrices.ViewMatrix * View.ViewMatrices.ProjMatrix;
				View.InvViewProjectionMatrix = View.ViewMatrices.GetInvProjMatrix() * View.InvViewMatrix;

				/** The view transform, starting from world-space points translated by -ViewOrigin. */
				FMatrix TranslatedViewMatrix = FTranslationMatrix(-View.ViewMatrices.PreViewTranslation) * View.ViewMatrices.ViewMatrix;

				// Compute a transform from view origin centered world-space to clip space.
				View.ViewMatrices.TranslatedViewProjectionMatrix = TranslatedViewMatrix * View.ViewMatrices.ProjMatrix;
				View.ViewMatrices.InvTranslatedViewProjectionMatrix = View.ViewMatrices.TranslatedViewProjectionMatrix.InverseSafe();
			}
		}
		else if(ViewState)
		{
			// no TemporalAA
			ViewState->SetupTemporalAA(1);
		}

		if ( ViewState )
		{
			// determine if we are initializing or we should reset the persistent state
			const float DeltaTime = View.Family->CurrentRealTime - ViewState->LastRenderTime;
			const bool bFirstFrameOrTimeWasReset = DeltaTime < -0.0001f || ViewState->LastRenderTime < 0.0001f;

			// detect conditions where we should reset occlusion queries
			if (bFirstFrameOrTimeWasReset || 
				ViewState->LastRenderTime + GEngine->PrimitiveProbablyVisibleTime < View.Family->CurrentRealTime ||
				View.bCameraCut ||
				IsLargeCameraMovement(
					View, 
				    ViewState->PrevViewMatrixForOcclusionQuery, 
				    ViewState->PrevViewOriginForOcclusionQuery, 
				    GEngine->CameraRotationThreshold, GEngine->CameraTranslationThreshold))
			{
				View.bIgnoreExistingQueries = true;
				View.bDisableDistanceBasedFadeTransitions = true;
			}
			ViewState->PrevViewMatrixForOcclusionQuery = View.ViewMatrices.ViewMatrix;
			ViewState->PrevViewOriginForOcclusionQuery = View.ViewMatrices.ViewOrigin;
				
			// store old view matrix and detect conditions where we should reset motion blur 
			{
				bool bResetCamera = bFirstFrameOrTimeWasReset
					|| View.bCameraCut
					|| IsLargeCameraMovement(View, ViewState->PrevViewMatrices.ViewMatrix, ViewState->PrevViewMatrices.ViewOrigin, 45.0f, 10000.0f);

				if (bResetCamera)
				{
					ViewState->PrevViewMatrices = View.ViewMatrices;

					ViewState->PendingPrevViewMatrices = ViewState->PrevViewMatrices;

					// PT: If the motion blur shader is the last shader in the post-processing chain then it is the one that is
					//     adjusting for the viewport offset.  So it is always required and we can't just disable the work the
					//     shader does.  The correct fix would be to disable the effect when we don't need it and to properly mark
					//     the uber-postprocessing effect as the last effect in the chain.

					View.bPrevTransformsReset				= true;
				}
				else
				{
					// check for pause so we can keep motion blur in paused mode (doesn't work in editor)
					if(!GRenderingRealtimeClock.GetGamePaused())
					{
						// pending is needed as we are in init view and still need to render.
						ViewState->PrevViewMatrices = ViewState->PendingPrevViewMatrices;
						ViewState->PendingPrevViewMatrices = View.ViewMatrices;
					}
				}
				// we don't use DeltaTime as it can be 0 (in editor) and is computed by subtracting floats (loses precision over time)
				// Clamp DeltaWorldTime to reasonable values for the purposes of motion blur, things like TimeDilation can make it very small
				if(!GRenderingRealtimeClock.GetGamePaused())
				{
					ViewState->MotionBlurTimeScale			= bEnableTimeScale ? (1.0f / (FMath::Max(View.Family->DeltaWorldTime, .00833f) * 30.0f)) : 1.0f;
				}

				View.PrevViewMatrices = ViewState->PrevViewMatrices;

				View.PrevViewProjMatrix = ViewState->PrevViewMatrices.GetViewProjMatrix();
				View.PrevViewRotationProjMatrix = ViewState->PrevViewMatrices.GetViewRotationProjMatrix();
			}

			ViewState->PrevFrameNumber = ViewState->PendingPrevFrameNumber;
			ViewState->PendingPrevFrameNumber = View.FrameNumber;

			// This finishes the update of view state
			ViewState->UpdateLastRenderTime(*View.Family);
		}

		// Initialize the view's RHI resources.
		View.InitRHIResources();
	}
}

void FSceneRenderer::ComputeViewVisibility()
{
	SCOPE_CYCLE_COUNTER(STAT_ViewVisibilityTime);

	STAT(int32 NumProcessedPrimitives = 0);
	STAT(int32 NumCulledPrimitives = 0);
	STAT(int32 NumOccludedPrimitives = 0);

	// Allocate the visible light info.
	if (Scene->Lights.GetMaxIndex() > 0)
	{
		VisibleLightInfos.AddZeroed(Scene->Lights.GetMaxIndex());
	}

	int32 NumPrimitives = Scene->Primitives.Num();
	float CurrentRealTime = ViewFamily.CurrentRealTime;

	// This array contains a per-primitive mask specifying for which views a
	// primitive must have PreRenderView called.
	FPrimitivePreRenderViewMasks PreRenderViewMasks;
	PreRenderViewMasks.AddZeroed(NumPrimitives);

	// This array contains a list of relevant static primities.
	FRelevantStaticPrimitives RelevantStaticPrimitives;

	uint8 ViewBit = 0x1;
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex, ViewBit <<= 1)
	{
		STAT(NumProcessedPrimitives += NumPrimitives);

		FViewInfo& View = Views[ViewIndex];
		FSceneViewState* ViewState = (FSceneViewState*)View.State;

		// Allocate the view's visibility maps.
		View.PrimitiveVisibilityMap.Init(false,Scene->Primitives.Num());
		View.PrimitiveDefinitelyUnoccludedMap.Init(false,Scene->Primitives.Num());
		View.PotentiallyFadingPrimitiveMap.Init(false,Scene->Primitives.Num());
		View.PrimitiveFadeUniformBuffers.AddZeroed(Scene->Primitives.Num());
		View.StaticMeshVisibilityMap.Init(false,Scene->StaticMeshes.GetMaxIndex());
		View.StaticMeshOccluderMap.Init(false,Scene->StaticMeshes.GetMaxIndex());
		View.StaticMeshVelocityMap.Init(false,Scene->StaticMeshes.GetMaxIndex());
		View.StaticMeshShadowDepthMap.Init(false,Scene->StaticMeshes.GetMaxIndex());
		View.StaticMeshBatchVisibility.AddZeroed(Scene->StaticMeshes.GetMaxIndex());

		View.VisibleLightInfos.Empty(Scene->Lights.GetMaxIndex());

		for(int32 LightIndex = 0;LightIndex < Scene->Lights.GetMaxIndex();LightIndex++)
		{
			if( LightIndex+2 < Scene->Lights.GetMaxIndex() )
			{
				if (LightIndex > 2)
				{
					FLUSH_CACHE_LINE(&View.VisibleLightInfos(LightIndex-2));
				}
				// @todo optimization These prefetches cause asserts since LightIndex > View.VisibleLightInfos.Num() - 1
				//FPlatformMisc::Prefetch(&View.VisibleLightInfos[LightIndex+2]);
				//FPlatformMisc::Prefetch(&View.VisibleLightInfos[LightIndex+1]);
			}
			new(View.VisibleLightInfos) FVisibleLightViewInfo();
		}

		View.PrimitiveViewRelevanceMap.Empty(Scene->Primitives.Num());
		View.PrimitiveViewRelevanceMap.AddZeroed(Scene->Primitives.Num());

		// If this is the visibility-parent of other views, reset its ParentPrimitives list.
		const bool bIsParent = ViewState && ViewState->IsViewParent();
		if ( bIsParent )
		{
			ViewState->ParentPrimitives.Empty();
		}

		if (ViewState)
		{	
			SCOPE_CYCLE_COUNTER(STAT_DecompressPrecomputedOcclusion);
			View.PrecomputedVisibilityData = ViewState->GetPrecomputedVisibilityData(View, Scene);
		}
		else
		{
			View.PrecomputedVisibilityData = NULL;
		}

		if (View.PrecomputedVisibilityData)
		{
			bUsedPrecomputedVisibility = true;
		}

		bool bNeedsFrustumCulling = true;

		// Development builds sometimes override frustum culling, e.g. dependent views in the editor.
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if( ViewState )
		{
#if WITH_EDITOR
			// For visibility child views, check if the primitive was visible in the parent view.
			const FSceneViewState* const ViewParent = (FSceneViewState*)ViewState->GetViewParent();
			if(ViewParent)
			{
				bNeedsFrustumCulling = false;
				for (FSceneBitArray::FIterator BitIt(View.PrimitiveVisibilityMap); BitIt; ++BitIt)
				{
					if (ViewParent->ParentPrimitives.Contains(Scene->PrimitiveComponentIds[BitIt.GetIndex()]))
					{
						BitIt.GetValue() = true;
					}
				}
			}
#endif
			// For views with frozen visibility, check if the primitive is in the frozen visibility set.
			if(ViewState->bIsFrozen)
			{
				bNeedsFrustumCulling = false;
				for (FSceneBitArray::FIterator BitIt(View.PrimitiveVisibilityMap); BitIt; ++BitIt)
				{
					if (ViewState->FrozenPrimitives.Contains(Scene->PrimitiveComponentIds[BitIt.GetIndex()]))
					{
						BitIt.GetValue() = true;
					}
				}
			}
		}
#endif

		// Most views use standard frustum culling.
		if (bNeedsFrustumCulling)
		{
			int32 NumCulledPrimitivesForView = FrustumCull(Scene, View);
			STAT(NumCulledPrimitives += NumCulledPrimitivesForView);
			UpdatePrimitiveFading(Scene, View);			
		}

		// If any primitives are explicitly hidden, remove them now.
		if (View.HiddenPrimitives.Num())
		{
			for (FSceneSetBitIterator BitIt(View.PrimitiveVisibilityMap); BitIt; ++BitIt)
			{
				if (View.HiddenPrimitives.Contains(Scene->PrimitiveComponentIds[BitIt.GetIndex()]))
				{
					View.PrimitiveVisibilityMap.AccessCorrespondingBit(BitIt) = false;
				}
			}
		}

		if (View.bIsReflectionCapture)
		{
			for (FSceneSetBitIterator BitIt(View.PrimitiveVisibilityMap); BitIt; ++BitIt)
			{
				// Reflection captures should only capture objects that won't move, since reflection captures won't update at runtime
				if (!Scene->Primitives[BitIt.GetIndex()]->Proxy->HasStaticLighting())
				{
					View.PrimitiveVisibilityMap.AccessCorrespondingBit(BitIt) = false;
				}
			}
		}

		// Cull small objects in wireframe in ortho views
		// This is important for performance in the editor because wireframe disables any kind of occlusion culling
		if (View.Family->EngineShowFlags.Wireframe)
		{
			float ScreenSizeScale = FMath::Max(View.ViewMatrices.ProjMatrix.M[0][0] * View.ViewRect.Width(), View.ViewMatrices.ProjMatrix.M[1][1] * View.ViewRect.Height());
			for (FSceneSetBitIterator BitIt(View.PrimitiveVisibilityMap); BitIt; ++BitIt)
			{
				if (ScreenSizeScale * Scene->PrimitiveBounds[BitIt.GetIndex()].SphereRadius <= GWireframeCullThreshold)
				{
					View.PrimitiveVisibilityMap.AccessCorrespondingBit(BitIt) = false;
				}
			}
		}

		// Occlusion cull for all primitives in the view frustum, but not in wireframe.
		if (!View.Family->EngineShowFlags.Wireframe)
		{
			int32 NumOccludedPrimitivesInView = OcclusionCull(Scene, View);
			STAT(NumOccludedPrimitives += NumOccludedPrimitivesInView);
		}

		MarkAllPrimitivesForReflectionProxyUpdate(Scene);
		Scene->ConditionalMarkStaticMeshElementsForUpdate();

		// Compute view relevance for all visible primitives.
		RelevantStaticPrimitives.Reset();
		ComputeRelevanceForView(Scene, View, ViewBit, RelevantStaticPrimitives, PreRenderViewMasks);
		MarkRelevantStaticMeshesForView(Scene, View, RelevantStaticPrimitives);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		// Store the primitive for parent occlusion rendering.
		if (FPlatformProperties::SupportsWindowedMode() && ViewState && ViewState->IsViewParent())
		{
			for (FSceneDualSetBitIterator BitIt(View.PrimitiveVisibilityMap, View.PrimitiveDefinitelyUnoccludedMap); BitIt; ++BitIt)
			{
				ViewState->ParentPrimitives.Add(Scene->PrimitiveComponentIds[BitIt.GetIndex()]);
			}
		}
#endif

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		// if we are freezing the scene, then remember the primitives that are rendered.
		if (ViewState && ViewState->bIsFreezing)
		{
			for (FSceneSetBitIterator BitIt(View.PrimitiveVisibilityMap); BitIt; ++BitIt)
			{
				ViewState->FrozenPrimitives.Add(Scene->PrimitiveComponentIds[BitIt.GetIndex()]);
			}
		}
#endif
	}

	DispatchPreRenderView(Scene, &ViewFamily, PreRenderViewMasks, FrameNumber);

	INC_DWORD_STAT_BY(STAT_ProcessedPrimitives,NumProcessedPrimitives);
	INC_DWORD_STAT_BY(STAT_CulledPrimitives,NumCulledPrimitives);
	INC_DWORD_STAT_BY(STAT_OccludedPrimitives,NumOccludedPrimitives);
}

void FSceneRenderer::PostVisibilityFrameSetup()
{
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{		
		FViewInfo& View = Views[ViewIndex];

		// sort the translucent primitives
		View.TranslucentPrimSet.SortPrimitives();
	}

	bool bCheckLightShafts = false;
	if(GRHIFeatureLevel <= ERHIFeatureLevel::ES2)
	{
		// Clear the mobile light shaft data.
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{		
			FViewInfo& View = Views[ViewIndex];
			View.bLightShaftUse = false;
			View.LightShaftCenter.X = 0.0f;
			View.LightShaftCenter.Y = 0.0f;
			View.LightShaftColorMask = FLinearColor(0.0f,0.0f,0.0f);
			View.LightShaftColorApply = FLinearColor(0.0f,0.0f,0.0f);
		}
		
		bCheckLightShafts = ViewFamily.EngineShowFlags.LightShafts && CVarLightShaftQuality.GetValueOnRenderThread() > 0;
	}

	if (ViewFamily.EngineShowFlags.HitProxies == 0)
	{
		Scene->IndirectLightingCache.UpdateCache(Scene, *this, true);
	}

	// determine visibility of each light
	for(TSparseArray<FLightSceneInfoCompact>::TConstIterator LightIt(Scene->Lights);LightIt;++LightIt)
	{
		const FLightSceneInfoCompact& LightSceneInfoCompact = *LightIt;
		const FLightSceneInfo* LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

		// view frustum cull lights in each view
		for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{		
			const FLightSceneProxy* Proxy = LightSceneInfo->Proxy;
			FViewInfo& View = Views[ViewIndex];
			FVisibleLightViewInfo& VisibleLightViewInfo = View.VisibleLightInfos[LightIt.GetIndex()];
			// dir lights are always visible, and point/spot only if in the frustum
			if (Proxy->GetLightType() == LightType_Point  
				|| Proxy->GetLightType() == LightType_Spot)
			{
				const float Radius = Proxy->GetRadius();

				if (View.ViewFrustum.IntersectSphere(Proxy->GetOrigin(), Radius))
				{
					FSphere Bounds = Proxy->GetBoundingSphere();
					float DistanceSquared = (Bounds.Center - View.ViewMatrices.ViewOrigin).SizeSquared();
					const bool bDrawLight = FMath::Square( FMath::Min( 0.0002f, GMinScreenRadiusForLights / Bounds.W ) * View.LODDistanceFactor ) * DistanceSquared < 1.0f;
					VisibleLightViewInfo.bInViewFrustum = bDrawLight;
				}
			}
			else
			{
				VisibleLightViewInfo.bInViewFrustum = true;

				// Setup single sun-shaft from direction lights for mobile.
				if(bCheckLightShafts && LightSceneInfo->bEnableLightShaftBloom)
				{
					// Find directional light for sun shafts.
					// Tweaked values from UE3 implementation.
					const float PointLightFadeDistanceIncrease = 200.0f;
					const float PointLightRadiusFadeFactor = 5.0f;

					const FVector WorldSpaceBlurOrigin = LightSceneInfo->Proxy->GetPosition();
					// Transform into post projection space
					FVector4 ProjectedBlurOrigin = View.WorldToScreen(WorldSpaceBlurOrigin);

					const float DistanceToBlurOrigin = (View.ViewMatrices.ViewOrigin - WorldSpaceBlurOrigin).Size() + PointLightFadeDistanceIncrease;

					// Don't render if the light's origin is behind the view
					if(ProjectedBlurOrigin.W >= 0.0f
						// Don't render point lights that have completely faded out
							&& (LightSceneInfo->Proxy->GetLightType() == LightType_Directional 
							|| DistanceToBlurOrigin < LightSceneInfo->Proxy->GetRadius() * PointLightRadiusFadeFactor))
					{
						View.bLightShaftUse = true;
						View.LightShaftCenter.X = ProjectedBlurOrigin.X / ProjectedBlurOrigin.W;
						View.LightShaftCenter.Y = ProjectedBlurOrigin.Y / ProjectedBlurOrigin.W;
						// TODO: Might want to hookup different colors for these.
						View.LightShaftColorMask = LightSceneInfo->BloomTint;
						View.LightShaftColorApply = LightSceneInfo->BloomTint;
					}
				}
			}
		}
	}

	// Initialize the fog constants.
	InitFogConstants();
	InitAtmosphereConstants();
}

uint32 GetShadowQuality();

/**
 * Initialize scene's views.
 * Check visibility, sort translucent items, etc.
 */
void FDeferredShadingSceneRenderer::InitViews()
{
	SCOPED_DRAW_EVENT(InitViews, DEC_SCENE_ITEMS);

	SCOPE_CYCLE_COUNTER(STAT_InitViewsTime);

	PreVisibilityFrameSetup();
	ComputeViewVisibility();
	PostVisibilityFrameSetup();

	FVector AverageViewPosition(0);

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{		
		FViewInfo& View = Views[ViewIndex];
		AverageViewPosition += View.ViewMatrices.ViewOrigin / Views.Num();
	}

	SortBasePassStaticData(AverageViewPosition);

	bool bDynamicShadows = ViewFamily.EngineShowFlags.DynamicShadows && GetShadowQuality() > 0;

	if (bDynamicShadows && !IsSimpleDynamicLightingEnabled())
	{
		// Setup dynamic shadows.
		InitDynamicShadows();
	}

	if(ViewFamily.EngineShowFlags.MotionBlur && GRenderingRealtimeClock.GetGamePaused())
	{
		// so we can keep motion blur in paused mode

		// per object motion blur
		Scene->MotionBlurInfoData.RestoreForPausedMotionBlur();
		// per bone motion blur
		GPrevPerBoneMotionBlur.RestoreForPausedMotionBlur();
	}

	OnStartFrame();
}

