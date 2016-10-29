// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runtime/Core/Public/Core.h"
#include "Runtime/Core/Public/Features/IModularFeatures.h"
#include "Runtime/Core/Public/Modules/ModuleManager.h"
#include "Runtime/Json/Public/Json.h"
#include "Runtime/Messaging/Public/IMessageBus.h"
#include "Runtime/Messaging/Public/IMessageTracer.h"
#include "Runtime/Messaging/Public/IMessageTracerBreakpoint.h"
#include "Runtime/Serialization/Public/StructSerializer.h"
#include "Runtime/Serialization/Public/Backends/JsonStructSerializerBackend.h"
#include "Runtime/Slate/Public/SlateBasics.h"
#include "Runtime/Slate/Public/Widgets/Docking/SDockTab.h"
#include "Runtime/Slate/Public/Widgets/Input/SHyperlink.h"
#include "Runtime/Slate/Public/Widgets/Input/SSearchBox.h"
#include "Runtime/Slate/Public/Widgets/Layout/SExpandableArea.h"

#if WITH_EDITOR
	#include "Editor/PropertyEditor/Public/IDetailsView.h"
	#include "Editor/PropertyEditor/Public/IStructureDetailsView.h"
	#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
	#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#endif
