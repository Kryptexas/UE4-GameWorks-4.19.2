// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once



#if WITH_EDITOR
#include "UnrealEd.h"
#endif

#include "Core.h"
//#include "CoreUObject.h"
//#include "Engine/EngineBaseTypes.h"
//#include "Engine/EngineTypes.h"
//// @todo consider making this non-shippable
//#include "DisplayDebugHelpers.h"
//#include "InputCoreTypes.h"
//#include "GameFramework/Actor.h"
#include "Engine.h"

#include "AIModule.h"

#include "AITypes.h"
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