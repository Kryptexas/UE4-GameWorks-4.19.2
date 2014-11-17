// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#if WITH_LUA

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

class FLuaContext : public FScriptContextBase
{
	bool bHasTick;
	bool bHasDestroy;
	bool bHasBeginPlay;
	lua_State* LuaState;

public:

	FLuaContext();

	// FScriptContextBase interface
	virtual bool Initialize(UScriptComponent* Owner) OVERRIDE;
	virtual void BeginPlay() OVERRIDE;
	virtual void Tick(float DeltaTime) OVERRIDE;
	virtual void Destroy() OVERRIDE;
	virtual bool CanTick() OVERRIDE;
};

struct FLuaUtils
{
	static void RegisterLibrary(lua_State* LuaState, const luaL_Reg Lib[], const ANSICHAR* LibName);
	static bool CallFunction(lua_State* LuaState, const ANSICHAR* FunctionName);
	static bool DoesFunctionExist(lua_State* LuaState, const char* FunctionName);
};

struct FLuaVector
{
	static FVector Get(lua_State *LuaState, int ParamIndex);
	static void Return(lua_State *LuaState, const FVector& Vec);
};

struct FLuaVector4
{
	static FVector4 Get(lua_State *LuaState, int ParamIndex);
	static void Return(lua_State *LuaState, const FVector4& Vec);
};

struct FLuaQuat
{
	static FQuat Get(lua_State *LuaState, int ParamIndex);
	static void Return(lua_State *LuaState, const FQuat& Vec);
};

struct FLuaTransform
{
	static FTransform Get(lua_State *LuaState, int ParamIndex);
	static void Return(lua_State *LuaState, const FTransform& Transform);
};

#endif // WITH_LUA
