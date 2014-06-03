// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//----------------------------------------------------------------------//
// Engine headers
//----------------------------------------------------------------------//

#include "Core.h"
#include "CoreUObject.h"
#include "InputCore.h"
#include "EngineDefines.h"
#include "EngineSettings.h"
#include "EngineStats.h"
#include "EngineLogs.h"
#include "EngineGlobals.h"

#include "Engine/EngineBaseTypes.h"
#include "Engine/EngineTypes.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerController.h"
#include "AI/Navigation/RecastNavMesh.h"
#include "GameFramework/Pawn.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "TimerManager.h"					// Game play timers

//----------------------------------------------------------------------//
// AIModule headers
//----------------------------------------------------------------------//
#include "AIModule.h"

#include "AITypes.h"
#include "AISystem.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "AIController.h"
#include "AI/Navigation/NavigationSystem.h"

#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAINavigation, Warning, All);
DECLARE_LOG_CATEGORY_EXTERN(LogBehaviorTree, Display, All);
DECLARE_LOG_CATEGORY_EXTERN(LogCrowdFollowing, Warning, All);