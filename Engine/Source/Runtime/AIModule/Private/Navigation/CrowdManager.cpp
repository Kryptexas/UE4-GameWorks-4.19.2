// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"

#if WITH_RECAST
#include "AI/Navigation/PImplRecastNavMesh.h"
#include "AI/Navigation/RecastHelpers.h"
#include "DetourCrowd/DetourCrowd.h"
#endif

#include "Navigation/CrowdManager.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/CrowdAgentInterface.h"

DECLARE_STATS_GROUP(TEXT("Crowd"), STATGROUP_AICrowd, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("Simulation Time"), STAT_AI_Crowd_Tick, STATGROUP_AICrowd);
DECLARE_CYCLE_STAT(TEXT("Step: corridor update"), STAT_AI_Crowd_StepCorridorTime, STATGROUP_AICrowd);
DECLARE_CYCLE_STAT(TEXT("Step: paths"), STAT_AI_Crowd_StepPathsTime, STATGROUP_AICrowd);
DECLARE_CYCLE_STAT(TEXT("Step: proximity"), STAT_AI_Crowd_StepProximityTime, STATGROUP_AICrowd);
DECLARE_CYCLE_STAT(TEXT("Step: next point"), STAT_AI_Crowd_StepNextPointTime, STATGROUP_AICrowd);
DECLARE_CYCLE_STAT(TEXT("Step: steering"), STAT_AI_Crowd_StepSteeringTime, STATGROUP_AICrowd);
DECLARE_CYCLE_STAT(TEXT("Step: avoidance"), STAT_AI_Crowd_StepAvoidanceTime, STATGROUP_AICrowd);
DECLARE_CYCLE_STAT(TEXT("Step: components"), STAT_AI_Crowd_StepComponentsTime, STATGROUP_AICrowd);
DECLARE_CYCLE_STAT(TEXT("Step: navlinks"), STAT_AI_Crowd_StepNavLinkTime, STATGROUP_AICrowd);
DECLARE_CYCLE_STAT(TEXT("Step: movement"), STAT_AI_Crowd_StepMovementTime, STATGROUP_AICrowd);
DECLARE_CYCLE_STAT(TEXT("Agent Update Time"), STAT_AI_Crowd_AgentUpdateTime, STATGROUP_AICrowd);
DECLARE_DWORD_COUNTER_STAT(TEXT("Num Agents"), STAT_AI_Crowd_NumAgents, STATGROUP_AICrowd);

DEFINE_LOG_CATEGORY_STATIC(LogEngineCrowdFollowing, Warning, All)

namespace CrowdDebugDrawing
{
	/** if set, debug information will be displayed for agent selected in editor */
	bool bDebugSelectedActors = true;
	/** if set, basic debug information will be recorded in VisLog for all agents */
	bool bDebugVisLog = false;

	/** debug flags, works only for selected actor */
	bool bDrawDebugCorners = true;
	bool bDrawDebugCollisionSegments = true;
	bool bDrawDebugPath = true;
	bool bDrawDebugVelocityObstacles = true;
	bool bDrawDebugPathOptimization = true;
	bool bDrawDebugNeighbors = true;

	const FVector Offset(0, 0, 20);

	const FColor Corner(128, 0, 0);
	const FColor CornerLink(192, 0, 0);
	const FColor CollisionRange(192, 0, 128);
	const FColor CollisionSeg0(192, 0, 128);
	const FColor CollisionSeg1(96, 0, 64);
	const FColor Path(255, 255, 255);
	const FColor PathSpecial(255, 192, 203);
	const FColor PathOpt(0, 128, 0);
	const FColor AvoidanceRange(255, 255, 255);
	const FColor Neighbor(0, 192, 128);

	const float LineThickness = 3.f;
}

void FCrowdTickHelper::Tick(float DeltaTime)
{
#if WITH_EDITOR
	if (Owner.IsValid())
	{
		Owner->DebugTick();
	}
#endif // WITH_EDITOR
}

TStatId FCrowdTickHelper::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FCrowdTickHelper, STATGROUP_Tickables);
}

UCrowdManager::UCrowdManager(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	MyNavData = NULL;
#if WITH_RECAST
	DetourCrowd = NULL;

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		DetourAvoidanceDebug = dtAllocObstacleAvoidanceDebugData();
		DetourAvoidanceDebug->init(2048);

		DetourAgentDebug = new dtCrowdAgentDebugInfo();
		FMemory::Memzero(DetourAgentDebug, sizeof(dtCrowdAgentDebugInfo));
		DetourAgentDebug->idx = -1;
		DetourAgentDebug->vod = DetourAvoidanceDebug;
	}
	else
	{
		DetourAgentDebug = NULL;
		DetourAvoidanceDebug = NULL;
	}
#endif
#if WITH_EDITOR
	TickHelper = NULL;
	if (!HasAnyFlags(RF_ClassDefaultObject) && GIsEditor)
	{
		TickHelper = new FCrowdTickHelper();
		TickHelper->Owner = this;
	}
#endif

	MaxAgents = 50;
	MaxAgentRadius = 100.0f;
	NavmeshCheckInterval = 1.0f;
	bSingleAreaVisibilityOptimization = true;
	bPruneStartedOffmeshConnections = false;
	
	FCrowdAvoidanceConfig AvoidanceConfig11;		// 11 samples, ECrowdAvoidanceQuality::Low
	AvoidanceConfig11.VelocityBias = 0.5f;
	AvoidanceConfig11.AdaptiveDivisions = 5;
	AvoidanceConfig11.AdaptiveRings = 2;
	AvoidanceConfig11.AdaptiveDepth = 1;
	AvoidanceConfig.Add(AvoidanceConfig11);
	
	FCrowdAvoidanceConfig AvoidanceConfig22;		// 22 samples, ECrowdAvoidanceQuality::Medium
	AvoidanceConfig22.VelocityBias = 0.5f;
	AvoidanceConfig22.AdaptiveDivisions = 5;
	AvoidanceConfig22.AdaptiveRings = 2;
	AvoidanceConfig22.AdaptiveDepth = 2;
	AvoidanceConfig.Add(AvoidanceConfig22);

	FCrowdAvoidanceConfig AvoidanceConfig45;		// 45 samples, ECrowdAvoidanceQuality::Good
	AvoidanceConfig45.VelocityBias = 0.5f;
	AvoidanceConfig45.AdaptiveDivisions = 7;
	AvoidanceConfig45.AdaptiveRings = 2;
	AvoidanceConfig45.AdaptiveDepth = 3;
	AvoidanceConfig.Add(AvoidanceConfig45);

	FCrowdAvoidanceConfig AvoidanceConfig66;		// 66 samples, ECrowdAvoidanceQuality::High
	AvoidanceConfig66.VelocityBias = 0.5f;
	AvoidanceConfig66.AdaptiveDivisions = 7;
	AvoidanceConfig66.AdaptiveRings = 3;
	AvoidanceConfig66.AdaptiveDepth = 3;
	AvoidanceConfig.Add(AvoidanceConfig66);
}

void UCrowdManager::BeginDestroy()
{
#if WITH_RECAST
	// cleanup allocated link filters
	for (auto It = ActiveAgents.CreateConstIterator(); It; ++It)
	{
		const FCrowdAgentData& AgentData = It.Value();
		delete AgentData.LinkFilter;
	}

	dtFreeObstacleAvoidanceDebugData(DetourAvoidanceDebug);
	delete DetourAgentDebug;
#endif

#if WITH_EDITOR
	delete TickHelper;
#endif

	Super::BeginDestroy();
}

void UCrowdManager::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_Tick);
	INC_DWORD_STAT_BY(STAT_AI_Crowd_NumAgents, ActiveAgents.Num());

#if WITH_RECAST
	if (DetourCrowd)
	{
		int32 NumActive = DetourCrowd->cacheActiveAgents();
		if (NumActive)
		{
			MyNavData->BeginBatchQuery();

			for (auto It = ActiveAgents.CreateConstIterator(); It; ++It)
			{
				// collect position and velocity
				const FCrowdAgentData& AgentData = It.Value();
				UpdateSpatialData(It.Key(), AgentData);
			}

			// corridor update from previous step
			{
				SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_StepCorridorTime);
				DetourCrowd->updateStepCorridor(DeltaTime, DetourAgentDebug);
			}

			// regular steps
			{
				SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_StepPathsTime);
				DetourCrowd->updateStepPaths(DeltaTime, DetourAgentDebug);
			}
			{
				SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_StepProximityTime);
				DetourCrowd->updateStepProximityData(DeltaTime, DetourAgentDebug);
				PostProximityUpdate();
			}
			{
				SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_StepNextPointTime);
				DetourCrowd->updateStepNextMovePoint(DeltaTime, DetourAgentDebug);
			}
			{
				SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_StepSteeringTime);
				DetourCrowd->updateStepSteering(DeltaTime, DetourAgentDebug);
			}
			{
				SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_StepAvoidanceTime);
				DetourCrowd->updateStepAvoidance(DeltaTime, DetourAgentDebug);
			}
			{
				SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_StepComponentsTime);
				UpdateAgentPaths();
			}
			{
				SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_StepNavLinkTime);
				DetourCrowd->updateStepOffMeshVelocity(DeltaTime, DetourAgentDebug);
			}

			// velocity updates
			{
				SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_StepMovementTime);
				for (auto It = ActiveAgents.CreateConstIterator(); It; ++It)
				{
					const FCrowdAgentData& AgentData = It.Value();
					if (AgentData.bIsSimulated)
					{
						const UCrowdFollowingComponent* CrowdComponent = Cast<const UCrowdFollowingComponent>(It.Key()->GetUObjectInterfaceCrowdAgentInterface());
						ApplyVelocity(CrowdComponent, AgentData.AgentIndex);
					}
				}
			}

			MyNavData->FinishBatchQuery();

#if WITH_EDITOR
			// normalize samples only for debug drawing purposes
			DetourAvoidanceDebug->normalizeSamples();
#endif
		}
	}
#endif // WITH_RECAST
}

#if WITH_EDITOR
void UCrowdManager::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
#if WITH_RECAST
	// recreate crowd manger
	DestroyCrowdManager();
	CreateCrowdManager();
#endif // WITH_RECAST
}
#endif // WITH_EDITOR

bool UCrowdManager::RegisterAgent(const ICrowdAgentInterface* Agent)
{
	UpdateNavData();

	FCrowdAgentData AgentData;

#if WITH_RECAST
	if (DetourCrowd)
	{
		AddAgent(Agent, AgentData);
	}
#endif

	const bool bSuccessfullyAdded = (AgentData.AgentIndex >= 0);
	if (bSuccessfullyAdded)
	{
		ActiveAgents.Add(Agent, AgentData);
	}
	
	return bSuccessfullyAdded;
}

void UCrowdManager::UnregisterAgent(const ICrowdAgentInterface* Agent)
{
#if WITH_RECAST
	const FCrowdAgentData* AgentData = ActiveAgents.Find(Agent);
	if (DetourCrowd && AgentData)
	{
		RemoveAgent(Agent, AgentData);
	}
#endif

	ActiveAgents.Remove(Agent);
}

void UCrowdManager::UpdateAgentParams(const ICrowdAgentInterface* Agent) const
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_AgentUpdateTime);

#if WITH_RECAST
	const FCrowdAgentData* AgentData = ActiveAgents.Find(Agent);
	if (DetourCrowd && AgentData)
	{
		dtCrowdAgentParams Params;
		GetAgentParams(Agent, &Params);

		DetourCrowd->updateAgentParameters(AgentData->AgentIndex, &Params);
	}
#endif
}

void UCrowdManager::UpdateAgentState(const class ICrowdAgentInterface* Agent) const
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_AgentUpdateTime);

#if WITH_RECAST
	const FCrowdAgentData* AgentData = ActiveAgents.Find(Agent);
	if (DetourCrowd && AgentData)
	{
		DetourCrowd->updateAgentState(AgentData->AgentIndex, false);
	}
#endif
}

bool UCrowdManager::SetAgentMoveTarget(const UCrowdFollowingComponent* AgentComponent, const FVector& MoveTarget, TSharedPtr<const FNavigationQueryFilter> Filter) const
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_AgentUpdateTime);

	bool bSuccess = false;

#if WITH_RECAST
	const FCrowdAgentData* AgentData = ActiveAgents.Find(AgentComponent);
	if (AgentData && AgentData->bIsSimulated && DetourCrowd)
	{
		FNavLocation ProjectedLoc;
		MyNavData->ProjectPoint(MoveTarget, ProjectedLoc, MyNavData->GetDefaultQueryExtent(), Filter);

		const INavigationQueryFilterInterface* NavFilter = Filter.IsValid() ? Filter->GetImplementation() : MyNavData->GetDefaultQueryFilterImpl();
		const dtQueryFilter* DetourFilter = ((const FRecastQueryFilter*)NavFilter)->GetAsDetourQueryFilter();
		DetourCrowd->updateAgentFilter(AgentData->AgentIndex, DetourFilter);
		DetourCrowd->updateAgentState(AgentData->AgentIndex, false);

		const FVector RcTargetPos = Unreal2RecastPoint(MoveTarget);
		bSuccess = DetourCrowd->requestMoveTarget(AgentData->AgentIndex, ProjectedLoc.NodeRef, &RcTargetPos.X);
	}
#endif

	return bSuccess;
}

bool UCrowdManager::SetAgentMoveDirection(const class UCrowdFollowingComponent* AgentComponent, const FVector& MoveDirection) const
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_AgentUpdateTime);

	bool bSuccess = false;

#if WITH_RECAST
	const FCrowdAgentData* AgentData = ActiveAgents.Find(AgentComponent);
	if (AgentData && AgentData->bIsSimulated && DetourCrowd)
	{
		DetourCrowd->updateAgentState(AgentData->AgentIndex, false);

		const FVector RcTargetVelocity = Unreal2RecastPoint(MoveDirection * AgentComponent->GetCrowdAgentMaxSpeed());
		bSuccess = DetourCrowd->requestMoveVelocity(AgentData->AgentIndex, &RcTargetVelocity.X);
	}
#endif

	return bSuccess;
}

bool UCrowdManager::SetAgentMovePath(const class UCrowdFollowingComponent* AgentComponent, const FNavMeshPath* Path, int32 PathSectionStart, int32 PathSectionEnd) const
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_AgentUpdateTime);

	bool bSuccess = false;

#if WITH_RECAST
	const FCrowdAgentData* AgentData = ActiveAgents.Find(AgentComponent);
	ARecastNavMesh* RecastNavData = Cast<ARecastNavMesh>(MyNavData);
	if (AgentData && AgentData->bIsSimulated && DetourCrowd &&
		RecastNavData && Path && (Path->PathPoints.Num() > 1) &&
		Path->PathCorridor.IsValidIndex(PathSectionStart) && Path->PathCorridor.IsValidIndex(PathSectionEnd))
	{
		FVector TargetPos = Path->PathPoints.Last().Location;
		if (PathSectionEnd < (Path->PathCorridor.Num() - 1))
		{
			RecastNavData->GetPolyCenter(Path->PathCorridor[PathSectionEnd], TargetPos);
		}

		TArray<dtPolyRef> PathRefs;
		for (int32 Idx = PathSectionStart; Idx <= PathSectionEnd; Idx++)
		{
			PathRefs.Add(Path->PathCorridor[Idx]);
		}

		const INavigationQueryFilterInterface* NavFilter = Path->Filter.IsValid() ? Path->Filter->GetImplementation() : MyNavData->GetDefaultQueryFilterImpl();
		const dtQueryFilter* DetourFilter = ((const FRecastQueryFilter*)NavFilter)->GetAsDetourQueryFilter();
		DetourCrowd->updateAgentFilter(AgentData->AgentIndex, DetourFilter);
		DetourCrowd->updateAgentState(AgentData->AgentIndex, false);

		const FVector RcTargetPos = Unreal2RecastPoint(TargetPos);		
		bSuccess = DetourCrowd->requestMoveTarget(AgentData->AgentIndex, PathRefs.Last(), &RcTargetPos.X);
		if (bSuccess)
		{
			bSuccess = DetourCrowd->setAgentCorridor(AgentData->AgentIndex, PathRefs.GetTypedData(), PathRefs.Num());
		}
	}
#endif

	return bSuccess;
}

void UCrowdManager::ClearAgentMoveTarget(const UCrowdFollowingComponent* AgentComponent) const
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_AgentUpdateTime);

#if WITH_RECAST
	const FCrowdAgentData* AgentData = ActiveAgents.Find(AgentComponent);
	if (AgentData && AgentData->bIsSimulated && DetourCrowd)
	{
		DetourCrowd->resetMoveTarget(AgentData->AgentIndex);
		DetourCrowd->resetAgentVelocity(AgentData->AgentIndex);
	}
#endif
}

void UCrowdManager::PauseAgent(const UCrowdFollowingComponent* AgentComponent) const
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_AgentUpdateTime);

#if WITH_RECAST
	const FCrowdAgentData* AgentData = ActiveAgents.Find(AgentComponent);
	if (AgentData && AgentData->bIsSimulated && DetourCrowd)
	{
		DetourCrowd->setAgentWaiting(AgentData->AgentIndex);
		DetourCrowd->resetAgentVelocity(AgentData->AgentIndex);
	}
#endif

}

void UCrowdManager::ResumeAgent(const UCrowdFollowingComponent* AgentComponent, bool bForceReplanPath) const
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_AgentUpdateTime);

#if WITH_RECAST
	const FCrowdAgentData* AgentData = ActiveAgents.Find(AgentComponent);
	if (AgentData && AgentData->bIsSimulated && DetourCrowd)
	{
		DetourCrowd->updateAgentState(AgentData->AgentIndex, bForceReplanPath);
	}
#endif
}

int32 UCrowdManager::GetNumNearbyAgents(const ICrowdAgentInterface* Agent) const
{
	int32 NumNearby = 0;

#if WITH_RECAST
	const FCrowdAgentData* AgentData = ActiveAgents.Find(Agent);
	if (AgentData && AgentData->bIsSimulated && DetourCrowd)
	{
		const dtCrowdAgent* ag = DetourCrowd->getAgent(AgentData->AgentIndex);
		NumNearby = ag ? ag->nneis : 0;
	}
#endif

	return NumNearby;
}

bool UCrowdManager::GetAvoidanceConfig(int32 Idx, FCrowdAvoidanceConfig& Data) const
{
	if (AvoidanceConfig.IsValidIndex(Idx))
	{
		Data = AvoidanceConfig[Idx];
		return true;
	}

	return false;
}

bool UCrowdManager::SetAvoidanceConfig(int32 Idx, const FCrowdAvoidanceConfig& Data)
{
	if (AvoidanceConfig.IsValidIndex(Idx))
	{
		AvoidanceConfig[Idx] = Data;
	}
#if WITH_RECAST
	else if (Idx < DT_CROWD_MAX_OBSTAVOIDANCE_PARAMS)
	{
		AvoidanceConfig.SetNum(Idx + 1);
		AvoidanceConfig[Idx] = Data;
	}
#endif
	else
	{
		return false;
	}

	UpdateAvoidanceConfig();
	return true;
}

void UCrowdManager::AdjustAgentPathStart(const class UCrowdFollowingComponent* AgentComponent, const FNavMeshPath* Path, int32& PathStartIdx) const
{
#if WITH_RECAST
	const FCrowdAgentData* AgentData = ActiveAgents.Find(AgentComponent);
	if (AgentData && AgentData->bIsSimulated && DetourCrowd && 
		Path && Path->PathCorridor.Num() > PathStartIdx)
	{
		const dtCrowdAgent* ag = DetourCrowd->getAgent(AgentData->AgentIndex);
		const dtPolyRef* agPath = ag->corridor.getPath();

		for (int32 Idx = 0; Idx < ag->corridor.getPathCount(); Idx++)
		{
			const dtPolyRef TestRef = ag->corridor.getFirstPoly();

			for (int32 TestIdx = PathStartIdx; TestIdx < Path->PathCorridor.Num(); TestIdx++)
			{
				if (Path->PathCorridor[TestIdx] == TestRef)
				{
					PathStartIdx = TestIdx;
					return;
				}
			}
		}
	}
#endif
}

void UCrowdManager::SetOffmeshConnectionPruning(bool bRemoveFromCorridor)
{
	bPruneStartedOffmeshConnections = bRemoveFromCorridor;
#if WITH_RECAST
	if (DetourCrowd)
	{
		DetourCrowd->setPruneStartedOffmeshConnections(bRemoveFromCorridor);
	}
#endif
}

void UCrowdManager::SetSingleAreaVisibilityOptimization(bool bEnable)
{
	bSingleAreaVisibilityOptimization = bEnable;
#if WITH_RECAST
	if (DetourCrowd)
	{
		DetourCrowd->setSingleAreaVisibilityOptimization(bEnable);
	}
#endif
}

#if WITH_RECAST

void UCrowdManager::AddAgent(const ICrowdAgentInterface* Agent, FCrowdAgentData& AgentData) const
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_AgentUpdateTime);

	dtCrowdAgentParams Params;
	GetAgentParams(Agent, &Params);

	// create link filter for fully simulated agents
	// (used to determine if agent can traverse smart links)
	FRecastSpeciaLinkFilter* MyLinkFilter = NULL;
	const UCrowdFollowingComponent* CrowdComponent = Cast<const UCrowdFollowingComponent>(Agent->GetUObjectInterfaceCrowdAgentInterface());
	if (CrowdComponent)
	{
		UNavigationSystem* NavSys = Cast<UNavigationSystem>(GetOuter());
		MyLinkFilter = new FRecastSpeciaLinkFilter(NavSys, CrowdComponent->GetOuter());
	}

	Params.linkFilter = MyLinkFilter;

	const FVector RcAgentPos = Unreal2RecastPoint(Agent->GetCrowdAgentLocation());
	const dtQueryFilter* DefaultFilter = ((const FRecastQueryFilter*)MyNavData->GetDefaultQueryFilterImpl())->GetAsDetourQueryFilter();

	AgentData.AgentIndex = DetourCrowd->addAgent(&RcAgentPos.X, &Params, DefaultFilter);
	AgentData.bIsSimulated = (Params.collisionQueryRange > 0.0f) && (CrowdComponent == NULL || CrowdComponent->IsCrowdSimulationEnabled());
	AgentData.LinkFilter = MyLinkFilter;
}

void UCrowdManager::RemoveAgent(const ICrowdAgentInterface* Agent, const FCrowdAgentData* AgentData) const
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Crowd_AgentUpdateTime);

	DetourCrowd->removeAgent(AgentData->AgentIndex);
	delete AgentData->LinkFilter;
}

void UCrowdManager::GetAgentParams(const ICrowdAgentInterface* Agent, struct dtCrowdAgentParams* AgentParams) const
{
	float CylRadius = 0.0f, CylHalfHeight = 0.0f;
	Agent->GetCrowdAgentCollisions(CylRadius, CylHalfHeight);

	FMemory::Memzero(AgentParams, sizeof(dtCrowdAgentParams));
	AgentParams->radius = CylRadius;
	AgentParams->height = CylHalfHeight * 2.0f;
	// skip maxSpeed, it will be constantly updated in every tick
	// skip maxAcceleration, we don't use Detour's movement code

	const UCrowdFollowingComponent* CrowdComponent = Cast<const UCrowdFollowingComponent>(Agent->GetUObjectInterfaceCrowdAgentInterface());
	if (CrowdComponent)
	{
		AgentParams->collisionQueryRange = CrowdComponent->GetCrowdCollisionQueryRange();
		AgentParams->pathOptimizationRange = CrowdComponent->GetCrowdPathOptimizationRange();
		AgentParams->separationWeight = CrowdComponent->GetCrowdSeparationWeight();
		AgentParams->obstacleAvoidanceType = CrowdComponent->GetCrowdAvoidanceQuality();
	
		AgentParams->updateFlags =
			(CrowdComponent->IsCrowdAnticipateTurnsEnabled() ? DT_CROWD_ANTICIPATE_TURNS : 0) |
			(CrowdComponent->IsCrowdObstacleAvoidanceEnabled() ? DT_CROWD_OBSTACLE_AVOIDANCE : 0) |
			(CrowdComponent->IsCrowdSeparationEnabled() ? DT_CROWD_SEPARATION : 0) |
			(CrowdComponent->IsCrowdOptimizeVisibilityEnabled() ? (DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_VIS_MULTI) : 0) |
			(CrowdComponent->IsCrowdOptimizeTopologyEnabled() ? DT_CROWD_OPTIMIZE_TOPO : 0);
	}
}

void UCrowdManager::UpdateSpatialData(const ICrowdAgentInterface* Agent, const FCrowdAgentData& AgentData) const
{
	dtCrowdAgent* ag = (dtCrowdAgent*)DetourCrowd->getAgent(AgentData.AgentIndex);
	ag->params.maxSpeed = Agent->GetCrowdAgentMaxSpeed();

	FVector RcLocation = Unreal2RecastPoint(Agent->GetCrowdAgentLocation());
	FVector RcVelocity = Unreal2RecastPoint(Agent->GetCrowdAgentVelocity());

	dtVcopy(ag->npos, &RcLocation.X);
	dtVcopy(ag->vel, &RcVelocity.X);
}

void UCrowdManager::ApplyVelocity(const UCrowdFollowingComponent* AgentComponent, int32 AgentIndex) const
{
	const dtCrowdAgent* ag = DetourCrowd->getAgent(AgentIndex);
	const dtCrowdAgentAnimation* anims = DetourCrowd->getAgentAnims();

	FVector NewVelocity = Recast2UnrealPoint(ag->nvel);
	const float* RcDestCorner = anims[AgentIndex].active ? anims[AgentIndex].endPos : 
		ag->ncorners ? &ag->cornerVerts[0] : &ag->npos[0];

	FVector DestPathCorner = Recast2UnrealPoint(RcDestCorner);
	((UCrowdFollowingComponent*)AgentComponent)->ApplyCrowdAgentVelocity(NewVelocity, DestPathCorner, anims->active != 0);
}

void UCrowdManager::UpdateAgentPaths()
{
	UNavigationSystem* NavSys = Cast<UNavigationSystem>(GetOuter());
	ARecastNavMesh* RecastNavData = Cast<ARecastNavMesh>(MyNavData);
	FPImplRecastNavMesh* PImplNavMesh = RecastNavData ? RecastNavData->RecastNavMeshImpl : NULL;
	if (PImplNavMesh == NULL)
	{
		return;
	}

	const dtCrowdAgentAnimation* AgentAnims = DetourCrowd->getAgentAnims();
	for (auto It = ActiveAgents.CreateIterator(); It; ++It)
	{
		FCrowdAgentData& AgentData = It.Value();
		if (AgentData.bIsSimulated)
		{
			UCrowdFollowingComponent* CrowdComponent = (UCrowdFollowingComponent*)Cast<const UCrowdFollowingComponent>(It.Key()->GetUObjectInterfaceCrowdAgentInterface());

			const dtCrowdAgent* Agent = DetourCrowd->getAgent(AgentData.AgentIndex);
			dtPolyRef AgentPolyRef = Agent->corridor.getFirstPoly();

			// look for newly triggered smart links
			const dtCrowdAgentAnimation& AnimInfo = AgentAnims[AgentData.AgentIndex];
			if (AnimInfo.active)
			{
				AgentPolyRef = AnimInfo.polyRef;

				if (AnimInfo.t == 0)
				{
					const uint32 NavLinkId = PImplNavMesh->GetLinkUserId(AnimInfo.polyRef);
					USmartNavLinkComponent* SmartLink = NavSys->GetSmartLink(NavLinkId);

					if (SmartLink)
					{
						FVector EndPt = Recast2UnrealPoint(AnimInfo.endPos);

						// switch to waiting state
						DetourCrowd->setAgentWaiting(AgentData.AgentIndex);
						DetourCrowd->resetAgentVelocity(AgentData.AgentIndex);

						// start using smart link
						if (CrowdComponent)
						{
							CrowdComponent->SetCurrentSmartLink(SmartLink, EndPt);
						}
					}
				}
			}

			// look for poly updates
			if (AgentPolyRef != AgentData.PrevPoly && CrowdComponent)
			{
				CrowdComponent->OnNavNodeChanged(AgentPolyRef, AgentData.PrevPoly, Agent->corridor.getPathCount());
				AgentData.PrevPoly = AgentPolyRef;
			}
		}
	}
}

void UCrowdManager::UpdateSelectedDebug(const ICrowdAgentInterface* Agent, int32 AgentIndex) const
{
#if WITH_EDITOR
	const UObject* Obj = Agent->GetUObjectInterfaceCrowdAgentInterface();
	if (GIsEditor && Obj)
	{
		const AController* TestController = Cast<const AController>(Obj->GetOuter());
		if (TestController && TestController->GetPawn() && TestController->GetPawn()->IsSelected())
		{
			DetourAgentDebug->idx = AgentIndex;
		}
	}
#endif
}

void UCrowdManager::CreateCrowdManager()
{
	ARecastNavMesh* RecastNavData = Cast<ARecastNavMesh>(MyNavData);
	FPImplRecastNavMesh* PImplNavMesh = RecastNavData ? RecastNavData->RecastNavMeshImpl : NULL;
	dtNavMesh* NavMeshPtr = PImplNavMesh ? PImplNavMesh->GetRecastMesh() : NULL;

	if (NavMeshPtr)
	{
		DetourCrowd = dtAllocCrowd();
	}
		
	if (DetourCrowd)
	{
		DetourCrowd->init(MaxAgents, MaxAgentRadius, NavMeshPtr);
		DetourCrowd->setAgentCheckInterval(NavmeshCheckInterval);
		DetourCrowd->setOffmeshLinkSegmentRemovalRadius(MaxAgentRadius);
		DetourCrowd->setSingleAreaVisibilityOptimization(bSingleAreaVisibilityOptimization);
		DetourCrowd->setPruneStartedOffmeshConnections(bPruneStartedOffmeshConnections);
		UpdateAvoidanceConfig();

		for (auto It = ActiveAgents.CreateIterator(); It; ++It)
		{
			AddAgent(It.Key(), It.Value());
		}
	}
}

void UCrowdManager::DestroyCrowdManager()
{
	dtFreeCrowd(DetourCrowd);
	DetourCrowd = NULL;
}

void UCrowdManager::DrawDebugCorners(const struct dtCrowdAgent* CrowdAgent) const
{
	FVector P0 = Recast2UnrealPoint(CrowdAgent->npos);
	for (int32 Idx = 0; Idx < CrowdAgent->ncorners; Idx++)
	{
		FVector P1 = Recast2UnrealPoint(&CrowdAgent->cornerVerts[Idx * 3]);
		DrawDebugLine(GetWorld(), P0 + CrowdDebugDrawing::Offset, P1 + CrowdDebugDrawing::Offset, CrowdDebugDrawing::Corner, false, -1.0f, SDPG_World, 2.0f);
		P0 = P1;
	}

	if (CrowdAgent->ncorners && (CrowdAgent->cornerFlags[CrowdAgent->ncorners - 1] & DT_STRAIGHTPATH_OFFMESH_CONNECTION))
	{
		FVector P0 = Recast2UnrealPoint(&CrowdAgent->cornerVerts[(CrowdAgent->ncorners - 1) * 3]);
		DrawDebugLine(GetWorld(), P0, P0 + CrowdDebugDrawing::Offset * 2.0f, CrowdDebugDrawing::CornerLink, false, -1.0f, SDPG_World, 2.0f);
	}
}

void UCrowdManager::DrawDebugCollisionSegments(const struct dtCrowdAgent* CrowdAgent) const
{
	FVector Center = Recast2UnrealPoint(CrowdAgent->boundary.getCenter()) + CrowdDebugDrawing::Offset;
	DrawDebugCylinder(GetWorld(), Center - CrowdDebugDrawing::Offset, Center, CrowdAgent->params.collisionQueryRange, 32, CrowdDebugDrawing::CollisionRange);

	for (int32 Idx = 0; Idx < CrowdAgent->boundary.getSegmentCount(); Idx++)
	{
		const float* s = CrowdAgent->boundary.getSegment(Idx);
		FColor Color = (dtTriArea2D(CrowdAgent->npos, s, s + 3) < 0.0f) ? CrowdDebugDrawing::CollisionSeg1 : CrowdDebugDrawing::CollisionSeg0;
		FVector Pt0 = Recast2UnrealPoint(s);
		FVector Pt1 = Recast2UnrealPoint(s + 3);

		DrawDebugLine(GetWorld(), Pt0 + CrowdDebugDrawing::Offset, Pt1 + CrowdDebugDrawing::Offset, Color, false, -1.0f, SDPG_World, 3.5f);
	}
}

void UCrowdManager::DrawDebugPath(const struct dtCrowdAgent* CrowdAgent) const
{
	ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(MyNavData);
	if (NavMesh == NULL)
	{
		return;
	}

	NavMesh->BeginBatchQuery();
	
	const dtPolyRef* Path = CrowdAgent->corridor.getPath();
	TArray<FVector> Verts;

	for (int32 Idx = 0; Idx < CrowdAgent->corridor.getPathCount(); Idx++)
	{
		Verts.Reset();
		NavMesh->GetPolyVerts(Path[Idx], Verts);

		uint16 PolyFlags = 0;
		uint16 AreaFlags = 0;
		NavMesh->GetPolyFlags(Path[Idx], PolyFlags, AreaFlags);
		const FColor PolyColor = AreaFlags != 1 ? CrowdDebugDrawing::Path : CrowdDebugDrawing::PathSpecial;

		for (int32 VertIdx = 0; VertIdx < Verts.Num(); VertIdx++)
		{
			const FVector Pt0 = Verts[VertIdx];
			const FVector Pt1 = Verts[(VertIdx + 1) % Verts.Num()];

			DrawDebugLine(GetWorld(), Pt0 + CrowdDebugDrawing::Offset * 0.5f, Pt1 + CrowdDebugDrawing::Offset * 0.5f, PolyColor, false
				, /*LifeTime*/-1.f, /*DepthPriority*/0
				, /*Thickness*/CrowdDebugDrawing::LineThickness);
		}
	}

	NavMesh->FinishBatchQuery();
}

void UCrowdManager::DrawDebugVelocityObstacles(const struct dtCrowdAgent* CrowdAgent) const
{
	FVector Center = Recast2UnrealPoint(CrowdAgent->npos) + CrowdDebugDrawing::Offset;
	DrawDebugCylinder(GetWorld(), Center - CrowdDebugDrawing::Offset, Center, CrowdAgent->params.maxSpeed, 32, CrowdDebugDrawing::AvoidanceRange);

	float BestSampleScore = -1.0f;
	FVector BestSampleLocation = FVector::ZeroVector;

	for (int32 Idx = 0; Idx < DetourAvoidanceDebug->getSampleCount(); Idx++)
	{
		const float* p = DetourAvoidanceDebug->getSampleVelocity(Idx);
		const float sr = DetourAvoidanceDebug->getSampleSize(Idx);
		const float pen = DetourAvoidanceDebug->getSamplePenalty(Idx);
		const float pen2 = DetourAvoidanceDebug->getSamplePreferredSidePenalty(Idx);

		FVector SamplePos = Center + Recast2UnrealPoint(p);

		if (BestSampleScore <= -1.0f || pen < BestSampleScore)
		{
			BestSampleScore = pen;
			BestSampleLocation = SamplePos;
		}

		float SamplePenalty = pen * 0.75f + pen2 * 0.25f;
		FColor SampleColor = FColor::MakeRedToGreenColorFromScalar(1.0f - SamplePenalty);
			
		FPlane Plane(0, 0, 1, SamplePos.Z);
		DrawDebugSolidPlane(GetWorld(), Plane, SamplePos, sr, SampleColor);
	}

	if (BestSampleScore >= 0.0f)
	{
		DrawDebugLine(GetWorld(), BestSampleLocation + FVector(0, 0, 100), BestSampleLocation + FVector(0, 0, -100), FColor::Green);
	}
}

void UCrowdManager::DrawDebugPathOptimization(const struct dtCrowdAgent* CrowdAgent) const
{
	FVector Pt0 = Recast2UnrealPoint(DetourAgentDebug->optStart) + CrowdDebugDrawing::Offset * 1.25f;
	FVector Pt1 = Recast2UnrealPoint(DetourAgentDebug->optEnd) + CrowdDebugDrawing::Offset * 1.25f;

	DrawDebugLine(GetWorld(), Pt0, Pt1, CrowdDebugDrawing::PathOpt, false, -1.0f, SDPG_World, 2.5f);
}

void UCrowdManager::DrawDebugNeighbors(const struct dtCrowdAgent* CrowdAgent) const
{
	FVector Center = Recast2UnrealPoint(CrowdAgent->npos) + CrowdDebugDrawing::Offset;
	DrawDebugCylinder(GetWorld(), Center - CrowdDebugDrawing::Offset, Center, CrowdAgent->params.collisionQueryRange, 32, CrowdDebugDrawing::CollisionRange);

	for (int32 Idx = 0; Idx < CrowdAgent->nneis; Idx++)
	{
		const dtCrowdAgent* nei = DetourCrowd->getAgent(CrowdAgent->neis[Idx].idx);
		if (nei)
		{
			FVector Pt0 = Recast2UnrealPoint(nei->npos) + CrowdDebugDrawing::Offset;
			DrawDebugLine(GetWorld(), Center, Pt0, CrowdDebugDrawing::Neighbor);
		}
	}
}

#endif // WITH_RECAST

#if WITH_EDITOR

void UCrowdManager::DebugTick() const
{
#if WITH_RECAST
	if (DetourCrowd == NULL || DetourAgentDebug == NULL)
	{
		return;
	}

	for (auto It = ActiveAgents.CreateConstIterator(); It; ++It)
	{
		const FCrowdAgentData& AgentData = It.Value();
		UpdateSelectedDebug(It.Key(), AgentData.AgentIndex);
	}

	// on screen debugging
	const struct dtCrowdAgent* SelectedAgent = DetourAgentDebug->idx >= 0 ? DetourCrowd->getAgent(DetourAgentDebug->idx) : NULL;
	if (SelectedAgent && CrowdDebugDrawing::bDebugSelectedActors)
	{
		if (CrowdDebugDrawing::bDrawDebugCorners)
		{
			DrawDebugCorners(SelectedAgent);
		}

		if (CrowdDebugDrawing::bDrawDebugCollisionSegments)
		{
			DrawDebugCollisionSegments(SelectedAgent);
		}

		if (CrowdDebugDrawing::bDrawDebugPath)
		{
			DrawDebugPath(SelectedAgent);
		}

		if (CrowdDebugDrawing::bDrawDebugVelocityObstacles)
		{
			DrawDebugVelocityObstacles(SelectedAgent);
		}

		if (CrowdDebugDrawing::bDrawDebugPathOptimization)
		{
			DrawDebugPathOptimization(SelectedAgent);
		}

		if (CrowdDebugDrawing::bDrawDebugNeighbors)
		{
			DrawDebugNeighbors(SelectedAgent);
		}
	}

	// vislog debugging
	if (CrowdDebugDrawing::bDebugVisLog)
	{
		for (auto It = ActiveAgents.CreateConstIterator(); It; ++It)
		{
			const ICrowdAgentInterface* IAgent = It.Key();
			const UObject* AgentOb = IAgent ? IAgent->GetUObjectInterfaceCrowdAgentInterface() : NULL;
			const AActor* LogOwner = AgentOb ? Cast<const AActor>(AgentOb->GetOuter()) : NULL;

			const FCrowdAgentData& AgentData = It.Value();
			const struct dtCrowdAgent* CrowdAgent = DetourCrowd->getAgent(AgentData.AgentIndex);

			if (CrowdAgent && LogOwner)
			{
				FVector P0 = Recast2UnrealPoint(CrowdAgent->npos);
				for (int32 Idx = 0; Idx < CrowdAgent->ncorners; Idx++)
				{
					FVector P1 = Recast2UnrealPoint(&CrowdAgent->cornerVerts[Idx * 3]);
					UE_VLOG_SEGMENT(LogOwner, LogEngineCrowdFollowing, Log, P0 + CrowdDebugDrawing::Offset, P1 + CrowdDebugDrawing::Offset, CrowdDebugDrawing::Corner, TEXT(""));
					P0 = P1;
				}

				if (CrowdAgent->ncorners && (CrowdAgent->cornerFlags[CrowdAgent->ncorners - 1] & DT_STRAIGHTPATH_OFFMESH_CONNECTION))
				{
					FVector P0 = Recast2UnrealPoint(&CrowdAgent->cornerVerts[(CrowdAgent->ncorners - 1) * 3]);
					UE_VLOG_SEGMENT(LogOwner, LogEngineCrowdFollowing, Log, P0, P0 + CrowdDebugDrawing::Offset * 2.0f, CrowdDebugDrawing::CornerLink, TEXT(""));
				}
			}
		}
	}
#endif	// WITH_RECAST
}

#endif // WITH_EDITOR

void UCrowdManager::UpdateNavData()
{
	if (MyNavData == NULL)
	{
		UNavigationSystem* NavSys = Cast<UNavigationSystem>(GetOuter());
		if (NavSys)
		{
			for (int32 Idx = 0; Idx < NavSys->NavDataSet.Num(); Idx++)
			{
				ARecastNavMesh* RecastNavData = Cast<ARecastNavMesh>(NavSys->NavDataSet[Idx]);
				if (RecastNavData && RecastNavData->IsSupportingDefaultAgent())
				{
					MyNavData = RecastNavData;
					RecastNavData->OnNavMeshUpdate.AddUObject(this, &UCrowdManager::OnNavMeshUpdate);
					OnNavMeshUpdate();

					break;
				}
			}
		}
	}
}

void UCrowdManager::OnNavMeshUpdate()
{
#if WITH_RECAST
	DestroyCrowdManager();
	CreateCrowdManager();
#endif // WITH_RECAST
}

void UCrowdManager::UpdateAvoidanceConfig()
{
#if WITH_RECAST
	if (DetourCrowd == NULL)
	{
		return;
	}

	for (int32 Idx = 0; Idx < AvoidanceConfig.Num(); Idx++)
	{
		const FCrowdAvoidanceConfig& ConfigInfo = AvoidanceConfig[Idx];
		
		dtObstacleAvoidanceParams params;
		params.velBias = ConfigInfo.VelocityBias;
		params.weightDesVel = ConfigInfo.DesiredVelocityWeight;
		params.weightCurVel = ConfigInfo.CurrentVelocityWeight;
		params.weightSide = ConfigInfo.SideBiasWeight;
		params.weightToi = ConfigInfo.ImpactTimeWeight;
		params.horizTime = ConfigInfo.ImpactTimeRange;
		params.adaptiveDivs = ConfigInfo.AdaptiveDivisions;
		params.adaptiveRings = ConfigInfo.AdaptiveRings;
		params.adaptiveDepth = ConfigInfo.AdaptiveDepth;

		DetourCrowd->setObstacleAvoidanceParams(Idx, &params);
	}
#endif // WITH_RECAST
}

void UCrowdManager::PostProximityUpdate()
{
	// empty in base class
}

class UWorld* UCrowdManager::GetWorld() const
{
	UNavigationSystem* NavSys = Cast<UNavigationSystem>(GetOuter());
	return NavSys ? NavSys->GetWorld() : NULL;
}

UCrowdManager* UCrowdManager::GetCurrent(class UObject* WorldContextObject)
{
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(WorldContextObject);
	return NavSys ? NavSys->GetCrowdManager() : NULL;
}

UCrowdManager* UCrowdManager::GetCurrent(class UWorld* World)
{
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(World);
	return NavSys ? NavSys->GetCrowdManager() : NULL;
}
