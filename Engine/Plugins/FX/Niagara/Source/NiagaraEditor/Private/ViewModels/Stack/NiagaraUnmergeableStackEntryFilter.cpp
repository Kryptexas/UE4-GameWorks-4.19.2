// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraUnmergeableStackEntryFilter.h"
#include "ViewModels/NiagaraSystemViewModel.h"
#include "ViewModels/Stack/NiagaraStackEntry.h"
#include "ViewModels/Stack/NiagaraStackEmitterSpawnScriptItemGroup.h"
#include "ViewModels/Stack/NiagaraStackSpacer.h"
#include "ViewModels/Stack/NiagaraStackAddScriptModuleItem.h"
#include "ViewModels/Stack/NiagaraStackEventHandlerGroup.h"
#include "ViewModels/Stack/NiagaraStackEventScriptItemGroup.h"
#include "ViewModels/Stack/NiagaraStackRenderItemGroup.h"

UNiagaraStackEntry::FOnFilterChild FNiagaraUnmergeableStackEntryFilter::CreateFilter()
{
	return UNiagaraStackEntry::FOnFilterChild::CreateLambda([](const UNiagaraStackEntry& StackEntry)
	{
		return true;
	});
}