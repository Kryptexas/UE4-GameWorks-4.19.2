// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "GameplayTagAssetInterface.h"
//#include "GameplayTagsManager.h"
//#include "GameplayTagContainer.h"
#include "SkillSystemLog.h"

DECLARE_STATS_GROUP(TEXT("SkillSystem"), STATGROUP_SkillSystem, STATCAT_Advanced);

DECLARE_CYCLE_STAT_EXTERN(TEXT("GameplayEffectsHasAllTagsTime"), STAT_GameplayEffectsHasAllTags, STATGROUP_SkillSystem, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("GameplayEffectsHasAnyTagTime"), STAT_GameplayEffectsHasAnyTag, STATGROUP_SkillSystem, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("GameplayEffectsTick"), STAT_GameplayEffectsTick, STATGROUP_SkillSystem, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("CanApplyAttributeModifiers"), STAT_GameplayEffectsCanApplyAttributeModifiers, STATGROUP_SkillSystem, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("GetActiveEffectsData"), STAT_GameplayEffectsGetActiveEffectsData, STATGROUP_SkillSystem, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("GetCooldownTimeRemaining"), STAT_GameplayAbilityGetCooldownTimeRemaining, STATGROUP_SkillSystem, );