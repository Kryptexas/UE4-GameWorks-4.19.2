// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "HierarchicalLODOutlinerPrivatePCH.h"
#include "HierarchicalLODOutlinerModule.h"
#include "HLODOutliner.h"

void FHierarchicalLODOutlinerModule::StartupModule()
{

}

void FHierarchicalLODOutlinerModule::ShutdownModule()
{

}

TSharedRef<SWidget> FHierarchicalLODOutlinerModule::CreateHLODOutlinerWidget()
{
	TSharedRef<HLODOutliner::SHLODOutliner> HLODWindow = SNew(HLODOutliner::SHLODOutliner);
	return HLODWindow;
}

IMPLEMENT_MODULE(FHierarchicalLODOutlinerModule, HLODPluginModule);