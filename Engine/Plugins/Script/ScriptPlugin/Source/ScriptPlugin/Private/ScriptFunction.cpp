// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved. 
#include "ScriptPluginPrivatePCH.h"
#include "ScriptFunction.h"
#include "ScriptBlueprintGeneratedClass.h"
#include "ScriptActor.h"

//////////////////////////////////////////////////////////////////////////

UScriptFunction::UScriptFunction(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP, NULL, FUNC_Public | FUNC_BlueprintCallable | FUNC_Native, 65535)
{
	UScriptBlueprintGeneratedClass* ScriptClass = Cast<UScriptBlueprintGeneratedClass>(GetOuter());
	if (ScriptClass)
	{
		ScriptClass->AddUniqueNativeFunction(GetFName(), (Native)&AScriptActor::InvokeScriptFunction);
		SetNativeFunc((Native)&AScriptActor::InvokeScriptFunction);
	}
}