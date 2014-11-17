// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IScriptPlugin.h"
#include "CoreUObject.h"
#include "ModuleManager.h"
#include "Engine.h"
#include "ScriptComponent.h"
#include "ScriptAsset.h"
#include "ScriptTestActor.h"

#if WITH_LUA
#include "LuaIntegration.h"
#endif // WITH_LUA

DECLARE_LOG_CATEGORY_EXTERN(LogScriptPlugin, Log, All);

