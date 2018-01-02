// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "HierarchicalLOD.h"
#include "Engine/World.h"
#include "Stats/StatsMisc.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/PackageName.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "Logging/TokenizedMessage.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Misc/MapErrors.h"
#include "GameFramework/WorldSettings.h"

#if WITH_EDITOR
#include "Engine/LODActor.h"
#include "ObjectTools.h"
#include "IHierarchicalLODUtilities.h"
#include "HierarchicalLODUtilitiesModule.h"
#include "../Classes/Editor/EditorEngine.h"
#include "UnrealEdGlobals.h"
#endif // WITH_EDITOR


#include "HierarchicalLODVolume.h"


DEFINE_LOG_CATEGORY_STATIC(LogLODGenerator, Warning, All);

#define LOCTEXT_NAMESPACE "HierarchicalLOD"
#define CM_TO_METER		0.01f
#define METER_TO_CM		100.0f

UHierarchicalLODSettings::UHierarchicalLODSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), bForceSettingsInAllMaps(false)
{	
}

FHierarchicalLODBuilder::FHierarchicalLODBuilder(UWorld* InWorld)
:	World(InWorld)
{
	checkf(InWorld != nullptr, TEXT("Invalid nullptr world provided"));
	HLODSettings = GetDefault<UHierarchicalLODSettings>();
}

FHierarchicalLODBuilder::FHierarchicalLODBuilder()
	: World(nullptr), 
	  HLODSettings(nullptr)
{
	EnsureRetrievingVTablePtrDuringCtor(TEXT("FHierarchicalLODBuilder()"));
}

void FHierarchicalLODBuilder::Build()
{
	check(World);
	bool bVisibleLevelsWarning = false;

	const TArray<ULevel*>& Levels = World->GetLevels();
	for (ULevel* LevelIter : Levels)
	{
		// Only build clusters for levels that are visible, and throw warning if any are hidden
		if (LevelIter->bIsVisible)
		{
			BuildClusters(LevelIter, true);
		}	
	
		bVisibleLevelsWarning |= !LevelIter->bIsVisible;	
	}


	// Fire map check warnings for hidden levels 
	if (bVisibleLevelsWarning)
	{
		FMessageLog MapCheck("HLODResults");
		MapCheck.Warning()
			->AddToken(FUObjectToken::Create(World->GetWorldSettings()))
			->AddToken(FTextToken::Create(LOCTEXT("MapCheck_Message_NoBuildHLODHiddenLevels", "Certain levels are marked as hidden, Hierarchical LODs will not be build for hidden levels.")));
	}
	
}

void FHierarchicalLODBuilder::PreviewBuild()
{
	check(World);
	bool bVisibleLevelsWarning = false;

	const TArray<ULevel*>& Levels = World->GetLevels();
	for (ULevel* LevelIter : Levels)
	{
		// Only build clusters for levels that are visible
		if (LevelIter->bIsVisible)
		{
			LevelIter->MarkPackageDirty();
			BuildClusters(LevelIter, false);
		}

		bVisibleLevelsWarning |= !LevelIter->bIsVisible;
	}

	// Fire map check warnings for hidden levels 
	if (bVisibleLevelsWarning)
	{
		FMessageLog MapCheck("HLODResults");
		MapCheck.Warning()
			->AddToken(FUObjectToken::Create(World->GetWorldSettings()))
			->AddToken(FTextToken::Create(LOCTEXT("MapCheck_Message_PreviewBuild_HLODHiddenLevels", "Certain levels are marked as hidden, Hierarchical LODs will not be built for hidden levels.")));
	}
}

void FHierarchicalLODBuilder::BuildClusters(ULevel* InLevel, const bool bCreateMeshes)
{	
	SCOPE_LOG_TIME(TEXT("STAT_HLOD_BuildClusters"), nullptr);

	const TArray<FHierarchicalSimplification>& BuildLODLevelSettings = InLevel->GetWorldSettings()->GetHierarchicalLODSetup();
	
	LODLevelLODActors.Empty();
	ValidStaticMeshActorsInLevel.Empty();

	// I'm using stack mem within this scope of the function
	// so we need this
	FMemMark Mark(FMemStack::Get());
	
	DeleteLODActors(InLevel, !bCreateMeshes);
	
	LODLevelLODActors.AddDefaulted(BuildLODLevelSettings.Num());
	const int32 TotalNumLOD = BuildLODLevelSettings.Num();

	LODLevelLODActors.Empty();
	LODLevelLODActors.AddZeroed(TotalNumLOD);

	// only build if it's enabled
	if (InLevel->GetWorldSettings()->bEnableHierarchicalLODSystem && BuildLODLevelSettings.Num() > 0)
	{
		if (InLevel->GetWorldSettings()->bGenerateSingleClusterForLevel)
		{
			Clusters.Empty();

			FLODCluster LevelCluster;

			TArray<AActor*> GenerationActors;
			for (int32 ActorId = 0; ActorId < InLevel->Actors.Num(); ++ActorId)
			{
				AActor* Actor = InLevel->Actors[ActorId];
				if (ShouldGenerateCluster(Actor, !bCreateMeshes))
				{
					FLODCluster ActorCluster(Actor);
					ValidStaticMeshActorsInLevel.Add(Actor);

					LevelCluster += ActorCluster;
				}
			}

			if (LevelCluster.IsValid())
			{
				ALODActor* LODActor = LevelCluster.BuildActor(InLevel, 0, bCreateMeshes);
			}
		}
		else
		{
			// Handle HierachicalLOD volumes first
			HandleHLODVolumes(InLevel);

			for (int32 LODId = 0; LODId < TotalNumLOD; ++LODId)
			{
				// we use meter for bound. Otherwise it's very easy to get to overflow and have problem with filling ratio because
				// bound is too huge
				const float DesiredBoundRadius = BuildLODLevelSettings[LODId].DesiredBoundRadius * CM_TO_METER;
				const float DesiredFillingRatio = BuildLODLevelSettings[LODId].DesiredFillingPercentage * 0.01f;
				ensure(DesiredFillingRatio != 0.f);
				const float HighestCost = FMath::Pow(DesiredBoundRadius, 3) / (DesiredFillingRatio);
				const int32 MinNumActors = BuildLODLevelSettings[LODId].MinNumberOfActorsToBuild;
				check(MinNumActors > 0);
				// test parameter I was playing with to cull adding to the array
				// intialization can have too many elements, decided to cull
				// the problem can be that we can create disconnected tree
				// my assumption is that if the merge cost is too high, then it's not worth merge anyway
				static int32 CullMultiplier = 1;

				// since to show progress of initialization, I'm scoping it
				{
					FString LevelName = FPackageName::GetShortName(InLevel->GetOutermost()->GetName());
					FFormatNamedArguments Arguments;
					Arguments.Add(TEXT("LODIndex"), FText::AsNumber(LODId + 1));
					Arguments.Add(TEXT("LevelName"), FText::FromString(LevelName));

					FScopedSlowTask SlowTask(100, FText::Format(LOCTEXT("HierarchicalLOD_InitializeCluster", "Initializing Clusters for LOD {LODIndex} of {LevelName}..."), Arguments));
					SlowTask.MakeDialog();

					// initialize Clusters
					InitializeClusters(InLevel, LODId, HighestCost*CullMultiplier, !bCreateMeshes, BuildLODLevelSettings[LODId].bOnlyGenerateClustersForVolumes);

					// move a half way - I know we can do this better but as of now this is small progress
					SlowTask.EnterProgressFrame(50);

					// now we have all pair of nodes
					FindMST();
				}

				// now we have to calculate merge clusters and build actors
				MergeClustersAndBuildActors(InLevel, LODId, HighestCost, MinNumActors, bCreateMeshes);
			}
		}
	}
	else
	{
		// Fire map check warnings if HLOD System is not enabled
		FMessageLog MapCheck("HLODResults");
		MapCheck.Warning()
			->AddToken(FUObjectToken::Create(InLevel->GetWorldSettings()))
			->AddToken(FTextToken::Create(LOCTEXT("MapCheck_Message_HLODSystemNotEnabled", "Hierarchical LOD System is disabled, unable to build LOD actors.")))
			->AddToken(FMapErrorToken::Create(FMapErrors::HLODSystemNotEnabled));
	}

	// Clear Clusters. It is using stack mem, so it won't be good after this
	Clusters.Empty();
	Clusters.Shrink();
}

void FHierarchicalLODBuilder::InitializeClusters(ULevel* InLevel, const int32 LODIdx, float CullCost, const bool bPreviewBuild, bool const bVolumesOnly)
{
	SCOPE_LOG_TIME(TEXT("STAT_HLOD_InitializeClusters"), nullptr);
	if (InLevel->Actors.Num() > 0)
	{
		if (LODIdx == 0)
		{
			Clusters.Empty();

			TArray<AActor*> GenerationActors;			
			for (int32 ActorId = 0; ActorId < InLevel->Actors.Num(); ++ActorId)
			{
				AActor* Actor = InLevel->Actors[ActorId];
				if (ShouldGenerateCluster(Actor, bPreviewBuild))
				{
					// Check whether or not this actor falls within a HierarchicalLODVolume, if so add to the Volume's cluster and exclude from normal process
					bool bAdded = bVolumesOnly;
					for (TPair<AHierarchicalLODVolume*, FLODCluster>& Cluster : HLODVolumeClusters)
					{
						if (Cluster.Key->EncompassesPoint(Actor->GetActorLocation(), Cluster.Key->bIncludeOverlappingActors ? Actor->GetComponentsBoundingBox().GetSize().Size() : 0.0f, nullptr))
						{			
							FBox BoundingBox = Actor->GetComponentsBoundingBox(true);
							FBox VolumeBox = Cluster.Key->GetComponentsBoundingBox(true);

							if (VolumeBox.IsInside(BoundingBox)|| (Cluster.Key->bIncludeOverlappingActors && VolumeBox.Intersect(BoundingBox)))
							{
								FLODCluster ActorCluster(Actor);
								Cluster.Value += ActorCluster;
								bAdded = true;
								break;
							}							
						}
					}

					if (!bAdded)
					{
						ValidStaticMeshActorsInLevel.Add(Actor);
					}
				}
			}
			
			// Create clusters using actor pairs
			for (int32 ActorId = 0; ActorId<ValidStaticMeshActorsInLevel.Num(); ++ActorId)
			{
				AActor* Actor1 = ValidStaticMeshActorsInLevel[ActorId];

				for (int32 SubActorId = ActorId + 1; SubActorId<ValidStaticMeshActorsInLevel.Num(); ++SubActorId)
				{
					AActor* Actor2 = ValidStaticMeshActorsInLevel[SubActorId];

					FLODCluster NewClusterCandidate = FLODCluster(Actor1, Actor2);
					float NewClusterCost = NewClusterCandidate.GetCost();

					if ( NewClusterCost <= CullCost)
					{
						Clusters.Add(NewClusterCandidate);
					}
				}
			}
		}
		else // at this point we only care for LODActors
		{
			Clusters.Empty();

			// we filter the LOD index first
			TArray<AActor*> Actors;

			Actors.Append(LODLevelLODActors[LODIdx - 1]);
			Actors.Append(ValidStaticMeshActorsInLevel);

			// first we generate graph with 2 pair nodes
			// this is very expensive when we have so many actors
			// so we'll need to optimize later @todo
			for (int32 ActorId = 0; ActorId < Actors.Num(); ++ActorId)
			{
				AActor* Actor1 = (Actors[ActorId]);
				for (int32 SubActorId = ActorId + 1; SubActorId < Actors.Num(); ++SubActorId)
				{
					AActor* Actor2 = Actors[SubActorId];

					// create new cluster
					FLODCluster NewClusterCandidate = FLODCluster(Actor1, Actor2);
					Clusters.Add(NewClusterCandidate);
				}
			}

			// shrink after adding actors
			// LOD 0 has lots of actors, and subsequence LODs tend to have a lot less actors
			// so this should save a lot more. 
			Clusters.Shrink();
		}
	}
}

void FHierarchicalLODBuilder::FindMST() 
{
	SCOPE_LOG_TIME(TEXT("STAT_HLOD_FindMST"), nullptr);
	if (Clusters.Num() > 0)
	{
		// now sort edge in the order of weight
		struct FCompareCluster
		{
			FORCEINLINE bool operator()(const FLODCluster& A, const FLODCluster& B) const
			{
				return (A.GetCost() < B.GetCost());
			}
		};

		Clusters.HeapSort(FCompareCluster());
	}
}

void FHierarchicalLODBuilder::HandleHLODVolumes(ULevel* InLevel)
{	
	HLODVolumeClusters.Reset();
	for (int32 ActorId = 0; ActorId < InLevel->Actors.Num(); ++ActorId)
	{
		if (AHierarchicalLODVolume* Actor = Cast<AHierarchicalLODVolume>(InLevel->Actors[ActorId]))
		{
			// Came across a HLOD volume			
			FLODCluster& NewCluster = HLODVolumeClusters.Add(Actor);

			FVector Origin, Extent;
			Actor->GetActorBounds(false, Origin, Extent);
			NewCluster.Bound = FSphere(Origin * CM_TO_METER, Extent.Size() * CM_TO_METER);


			// calculate new filling factor
			NewCluster.FillingFactor = 1.f;
			NewCluster.ClusterCost = FMath::Pow(NewCluster.Bound.W, 3) / NewCluster.FillingFactor;
		}
	}
}

bool FHierarchicalLODBuilder::ShouldGenerateCluster(AActor* Actor, const bool bPreviewBuild)
{
	if (!Actor)
	{
		return false;
	}

	if (Actor->bHidden)
	{
		return false;
	}

	if( Actor->HasAnyFlags( RF_Transient ) )
	{
		return false;
	}

	if( Actor->IsTemplate() )
	{
		return false;
	}
	
	if( Actor->IsPendingKill() )
	{
		return false;
	}

	if (!Actor->bEnableAutoLODGeneration)
	{
		return false;
	}

	ALODActor* LODActor = Cast<ALODActor>(Actor);
	if (bPreviewBuild && LODActor)
	{
		if (LODActor->GetStaticMeshComponent()->GetStaticMesh())
		{
			return false;
		}
	}

	FVector Origin, Extent;
	Actor->GetActorBounds(false, Origin, Extent);
	if (Extent.SizeSquared() <= 0.1)
	{
		return false;
	}	

	// for now only consider staticmesh - I don't think skel mesh would work with simplygon merge right now @fixme
	TInlineComponentArray<UStaticMeshComponent*> Components;
	for (UActorComponent* Component : Actor->GetComponents())
	{
		UStaticMeshComponent* SMComponent = Cast<UStaticMeshComponent>(Component);
		// TODO: support instanced static meshes
		if (SMComponent && !Component->IsA<UInstancedStaticMeshComponent>())
		{
			Components.Add(SMComponent);
		}
	}

	int32 ValidComponentCount = 0;
	// now make sure you check parent primitive, so that we don't build for the actor that already has built. 
	if (Components.Num() > 0)
	{
		for (UStaticMeshComponent* Component : Components)
		{			
			if (Component->GetLODParentPrimitive())
			{
				ALODActor* ParentActor = CastChecked<ALODActor>(Component->GetLODParentPrimitive()->GetOwner());
				
				if (ParentActor && bPreviewBuild)
				{
					return false;
				}
			}

			if (Component->bHiddenInGame)
			{
				return false;
			}

			// see if we should generate it
			if (Component->ShouldGenerateAutoLOD())
			{
				++ValidComponentCount;
				break;
			}
		}
	}

	return (ValidComponentCount > 0);
}

void FHierarchicalLODBuilder::ClearHLODs()
{
	bool bVisibleLevelsWarning = false;

	for (ULevel* Level : World->GetLevels())
	{
		bVisibleLevelsWarning |= !Level->bIsVisible;
		if (Level->bIsVisible)
		{
			DeleteLODActors(Level, false);
		}
	}


	// Fire map check warnings for hidden levels 
	if (bVisibleLevelsWarning)
	{
		FMessageLog MapCheck("MapCheck");
		MapCheck.Warning()
			->AddToken(FUObjectToken::Create(World->GetWorldSettings()))
			->AddToken(FTextToken::Create(LOCTEXT("MapCheck_Message_NoDeleteHLODHiddenLevels", "Certain levels are marked as hidden, Hierarchical LODs will not be deleted for hidden levels.")));
	}
}

void FHierarchicalLODBuilder::ClearPreviewBuild()
{
	bool bVisibleLevelsWarning = false;
	for (ULevel* Level : World->GetLevels())
	{
		bVisibleLevelsWarning |= !Level->bIsVisible;
		if ( Level->bIsVisible )
		{
			DeleteLODActors(Level, true);
		}		
	}

	// Fire map check warnings for hidden levels 
	if (bVisibleLevelsWarning)
	{
		FMessageLog MapCheck("MapCheck");
		MapCheck.Warning()
			->AddToken(FUObjectToken::Create(World->GetWorldSettings()))
			->AddToken(FTextToken::Create(LOCTEXT("MapCheck_Message_NoDeleteHLODHiddenLevels", "Certain levels are marked as hidden, Hierarchical LODs will not be deleted for hidden levels.")));
	}
}

void FHierarchicalLODBuilder::BuildMeshesForLODActors(bool bForceAll)
{	
	bool bVisibleLevelsWarning = false;

	const TArray<ULevel*>& Levels = World->GetLevels();
	for (const ULevel* LevelIter : Levels)
	{
		// Only meshes for clusters that are in a visible level
		if (!LevelIter->bIsVisible)
		{
			bVisibleLevelsWarning = true;
			continue;
		}

		FScopedSlowTask SlowTask(105, (LOCTEXT("HierarchicalLOD_BuildLODActorMeshes", "Building LODActor meshes")));
		SlowTask.MakeDialog();

		const TArray<FHierarchicalSimplification>& BuildLODLevelSettings = LevelIter->GetWorldSettings()->GetHierarchicalLODSetup();

		TArray<TArray<ALODActor*>> LODLevelActors;
		LODLevelActors.AddDefaulted(BuildLODLevelSettings.Num());

		if (LevelIter->Actors.Num() > 0)
		{
			FHierarchicalLODUtilitiesModule& Module = FModuleManager::LoadModuleChecked<FHierarchicalLODUtilitiesModule>("HierarchicalLODUtilities");
			IHierarchicalLODUtilities* Utilities = Module.GetUtilities();

			// Retrieve LOD actors from the level
			uint32 NumLODActors = 0;
			for (int32 ActorId = 0; ActorId < LevelIter->Actors.Num(); ++ActorId)
			{
				AActor* Actor = LevelIter->Actors[ActorId];
				if (Actor && Actor->IsA<ALODActor>())
				{
					ALODActor* LODActor = CastChecked<ALODActor>(Actor);
						
					if (bForceAll || (LODActor->IsDirty() && LODActor->HasValidSubActors()))
					{
						LODLevelActors[LODActor->LODLevel - 1].Add(LODActor);
						NumLODActors++;
					}
				}
			}		

			// If there are any available process them
			bool bBuildSuccesfull = true;
			if (NumLODActors)
			{
				// Only create the outer package if we are going to save something to it (otherwise we end up with an empty HLOD folder)
				UPackage* AssetsOuter = Utilities->CreateOrRetrieveLevelHLODPackage(LevelIter);
				checkf(AssetsOuter != nullptr, TEXT("Failed to created outer for generated HLOD assets"));
				const int32 NumLODLevels = LODLevelActors.Num();

				LevelIter->MarkPackageDirty();

				for (int32 LODIndex = 0; LODIndex < NumLODLevels; ++LODIndex)
				{
					int32 CurrentLODLevel = LODIndex;
					int32 LODActorIndex = 0;
					TArray<ALODActor*>& LODLevel = LODLevelActors[CurrentLODLevel];
					for (ALODActor* Actor : LODLevel)
					{
						bBuildSuccesfull &= Utilities->BuildStaticMeshForLODActor(Actor, AssetsOuter, BuildLODLevelSettings[CurrentLODLevel]);
						SlowTask.EnterProgressFrame(100.0f / (float)NumLODActors, FText::Format(LOCTEXT("HierarchicalLOD_BuildLODActorMeshesProgress", "Building LODActor Mesh {1} of {2} (LOD Level {0})"), FText::AsNumber(LODIndex + 1), FText::AsNumber(LODActorIndex), FText::AsNumber(LODLevelActors[CurrentLODLevel].Num())));
						++LODActorIndex;
					}
				}
			}
						
			check(bBuildSuccesfull);
		}		
	}

	// Fire map check warnings for hidden levels 
	if (bVisibleLevelsWarning)
	{
		FMessageLog MapCheck("MapCheck");
		MapCheck.Warning()
			->AddToken(FUObjectToken::Create(World->GetWorldSettings()))
			->AddToken(FTextToken::Create(LOCTEXT("MapCheck_Message_NoBuildHLODHiddenLevels", "Certain levels are marked as hidden, Hierarchical LODs will not be build for hidden levels.")));
	}

}

void FHierarchicalLODBuilder::DeleteLODActors(ULevel* InLevel, const bool bPreviewOnly)
{
	FHierarchicalLODUtilitiesModule& Module = FModuleManager::LoadModuleChecked<FHierarchicalLODUtilitiesModule>("HierarchicalLODUtilities");
	IHierarchicalLODUtilities* Utilities = Module.GetUtilities();

	// you still have to delete all objects just in case they had it and didn't want it anymore
	TArray<UObject*> AssetsToDelete;
	for (int32 ActorId = InLevel->Actors.Num() - 1; ActorId >= 0; --ActorId)
	{
		ALODActor* LodActor = Cast<ALODActor>(InLevel->Actors[ActorId]);
		if (LodActor && ((LodActor->GetStaticMeshComponent()->GetStaticMesh() == nullptr && bPreviewOnly) || !bPreviewOnly))
		{
			Utilities->DestroyLODActor(LodActor);
		}
	}
}

void FHierarchicalLODBuilder::BuildMeshForLODActor(ALODActor* LODActor, const uint32 LODLevel)
{
	const TArray<FHierarchicalSimplification>& BuildLODLevelSettings = LODActor->GetLevel()->GetWorldSettings()->GetHierarchicalLODSetup();
	
	FHierarchicalLODUtilitiesModule& Module = FModuleManager::LoadModuleChecked<FHierarchicalLODUtilitiesModule>("HierarchicalLODUtilities");
	IHierarchicalLODUtilities* Utilities = Module.GetUtilities();

	UPackage* AssetsOuter = Utilities->CreateOrRetrieveLevelHLODPackage(LODActor->GetLevel());
	const bool bResult = Utilities->BuildStaticMeshForLODActor(LODActor, AssetsOuter, BuildLODLevelSettings[LODLevel]);

	if (bResult == false)
	{
		FMessageLog("HLODResults").Error()
			->AddToken(FTextToken::Create(LOCTEXT("HLODError_MeshNotBuildOne", "Cannot create proxy mesh for ")))
			->AddToken(FUObjectToken::Create(LODActor))
			->AddToken(FTextToken::Create(LOCTEXT("HLODError_MeshNotBuildTwo", " this could be caused by incorrect mesh components in the sub actors")));			
	}
}

void FHierarchicalLODBuilder::MergeClustersAndBuildActors(ULevel* InLevel, const int32 LODIdx, float HighestCost, int32 MinNumActors, const bool bCreateMeshes)
{
	if (Clusters.Num() > 0 || HLODVolumeClusters.Num() > 0)
	{
		FString LevelName = FPackageName::GetShortName(InLevel->GetOutermost()->GetName());
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("LODIndex"), FText::AsNumber(LODIdx + 1));
		Arguments.Add(TEXT("LevelName"), FText::FromString(LevelName));
		// merge clusters first
		{
			SCOPE_LOG_TIME(TEXT("HLOD_MergeClusters"), nullptr);
			static int32 TotalIteration = 3;
			const int32 TotalCluster = Clusters.Num();

			FScopedSlowTask SlowTask(100.0f, FText::Format(LOCTEXT("HierarchicalLOD_BuildClusters", "Building Clusters for LOD {LODIndex} of {LevelName}..."), Arguments));
			SlowTask.MakeDialog();

			for (int32 Iteration = 0; Iteration < TotalIteration; ++Iteration)
			{
				bool bChanged = false;
				// now we have minimum Clusters
				for (int32 ClusterId = 0; ClusterId < TotalCluster; ++ClusterId)
				{
					FLODCluster& Cluster = Clusters[ClusterId];
					UE_LOG(LogLODGenerator, Verbose, TEXT("%d. %0.2f {%s}"), ClusterId + 1, Cluster.GetCost(), *Cluster.ToString());

					// progress bar update every percent, if ClustersPerPercent is zero ignore the progress bar as number of iterations is small.
					int32 ClustersPerPercent = (TotalCluster / (100.0f / TotalIteration));
					if ( ClustersPerPercent > 0 && ClusterId % ClustersPerPercent == 0) {
						SlowTask.EnterProgressFrame(1.0f);
					}

					if (Cluster.IsValid())
					{
						for (int32 MergedClusterId = 0; MergedClusterId < ClusterId; ++MergedClusterId)
						{
							// compare with previous clusters
							FLODCluster& MergedCluster = Clusters[MergedClusterId];
							// see if it's valid, if it contains, check the cost
							if (MergedCluster.IsValid())
							{
								if (MergedCluster.Contains(Cluster))
								{
									// if valid, see if it contains any of this actors
									// merge whole clusters
									FLODCluster NewCluster = Cluster + MergedCluster;
									float MergeCost = NewCluster.GetCost();

									// merge two clusters
									if (MergeCost <= HighestCost)
									{
										UE_LOG(LogLODGenerator, Log, TEXT("Merging of Cluster (%d) and (%d) with merge cost (%0.2f) "), ClusterId + 1, MergedClusterId + 1, MergeCost);

										MergedCluster = NewCluster;
										// now this cluster is invalid
										Cluster.Invalidate();

										bChanged = true;
										break;
									}
									else
									{
										Cluster -= MergedCluster;
										bChanged = true;
									}
								}
							}
						}

						UE_LOG(LogLODGenerator, Verbose, TEXT("Processed(%s): %0.2f {%s}"), Cluster.IsValid() ? TEXT("Valid") : TEXT("Invalid"), Cluster.GetCost(), *Cluster.ToString());
					}
				}

				if (bChanged == false)
				{
					break;
				}
			}
		}

		if (LODIdx == 0)
		{
			for (TPair<AHierarchicalLODVolume*, FLODCluster>& Cluster : HLODVolumeClusters)
			{
				Clusters.Add(Cluster.Value);
			}
		}


		{
			SCOPE_LOG_TIME(TEXT("HLOD_BuildActors"), nullptr);
			// print data
			int32 TotalValidCluster = 0;
			for (FLODCluster& Cluster : Clusters)
			{
				if (Cluster.IsValid())
				{
					++TotalValidCluster;
				}
			}

			FScopedSlowTask SlowTask(TotalValidCluster, FText::Format(LOCTEXT("HierarchicalLOD_MergeActors", "Merging Actors for LOD {LODIndex} of {LevelName}..."), Arguments));
			SlowTask.MakeDialog();

			for (FLODCluster& Cluster : Clusters)
			{
				if (Cluster.IsValid())
				{
					SlowTask.EnterProgressFrame();

					if (Cluster.Actors.Num() >= MinNumActors)
					{
						ALODActor* LODActor = Cluster.BuildActor(InLevel, LODIdx, bCreateMeshes);
						if (LODActor)
						{
							LODLevelLODActors[LODIdx].Add(LODActor);
						}

						for (AActor* RemoveActor : Cluster.Actors)
						{
							ValidStaticMeshActorsInLevel.RemoveSingleSwap(RemoveActor, false);
						}
					}
				}				
			}
		}
	}
}
#undef LOCTEXT_NAMESPACE 
