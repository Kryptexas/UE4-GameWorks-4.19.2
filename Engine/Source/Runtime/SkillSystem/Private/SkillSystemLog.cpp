// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "SkillSystemModulePrivatePCH.h"

DEFINE_LOG_CATEGORY(LogSkillSystem);

SkillSystemLogScope::~SkillSystemLogScope()
{
	SkillSystemLog::PopScope();
}

void SkillSystemLogScope::Init()
{
	PrintedInThisScope = false;
	SkillSystemLog::PushScope(this);
}

// ----------------------------------------------

SkillSystemLog * GetInstance()
{
	static SkillSystemLog Instance;
	return &Instance;
}

FString SkillSystemLog::Log(ELogVerbosity::Type Verbosity, FString Log)
{
	SkillSystemLog * Instance = GetInstance();

#if !NO_LOGGING
	if (!LogSkillSystem.IsSuppressed(Verbosity))
	{
		for (int32 idx=0; idx < Instance->ScopeStack.Num(); ++idx)
		{
			SkillSystemLogScope *Scope = Instance->ScopeStack[idx];
			if (!Scope->PrintedInThisScope && !Scope->ScopeName.IsEmpty())
			{
				if (Instance->NeedNewLine)
				{
					UE_LOG(LogSkillSystem, Log, TEXT(""));
					Instance->NeedNewLine = false;
				}

				Scope->PrintedInThisScope = true;
				int32 ident = (2 * idx);
				FString IndentStrX = FString::Printf(TEXT("%*s"), ident, TEXT(""));
				UE_LOG(LogSkillSystem, Log, TEXT("%s<%s>"), *IndentStrX, *Scope->ScopeName);
			}
		}
	}
#endif

	FString IndentStr = FString::Printf(TEXT("%*s"), Instance->Indent, TEXT(""));
	
	return IndentStr + Log;
}

void SkillSystemLog::PushScope(SkillSystemLogScope * Scope)
{
	SkillSystemLog * Instance = GetInstance();

	Instance->Indent += 2;
	Instance->ScopeStack.Push(Scope);
}

void SkillSystemLog::PopScope()
{
	SkillSystemLog * Instance = GetInstance();

	Instance->NeedNewLine = true;
	Instance->Indent -= 2;
	check(Instance->Indent >= 0);

	if (Instance->ScopeStack.Top()->PrintedInThisScope)
	{
		if (!Instance->ScopeStack.Top()->ScopeName.IsEmpty())
		{
			FString IndentStrX = FString::Printf(TEXT("%*s"), Instance->Indent, TEXT(""));
			UE_LOG(LogSkillSystem, Log, TEXT("%s</%s>"), *IndentStrX, *Instance->ScopeStack.Top()->ScopeName);
		}
	}
	
	Instance->ScopeStack.Pop();
}