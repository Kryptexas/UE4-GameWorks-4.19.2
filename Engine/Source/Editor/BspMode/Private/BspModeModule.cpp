// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BspModePrivatePCH.h"
#include "BspModeModule.h"
#include "BspMode.h"
#include "SBspPalette.h"
#include "BspModeActions.h"

#define LOCTEXT_NAMESPACE "BspMode"

void FBspModeModule::StartupModule()
{
	FBspModeStyle::Initialize();

	FBspModeCommands::Register();

	BspMode = FBspMode::Create();
	GEditorModeTools().RegisterMode( BspMode.ToSharedRef() );

	RegisterBspBuilderType(UCubeBuilder::StaticClass(), LOCTEXT("CubeBuilderName", "Box"), LOCTEXT("CubeBuilderToolTip", "Make a box brush"), FBspModeStyle::Get().GetBrush(TEXT("BspMode.BoxBrush")));
	RegisterBspBuilderType(UConeBuilder::StaticClass(), LOCTEXT("ConeBuilderName", "Cone"), LOCTEXT("ConeBuilderToolTip", "Make a cone brush"), FBspModeStyle::Get().GetBrush(TEXT("BspMode.ConeBrush")));
	RegisterBspBuilderType(UCylinderBuilder::StaticClass(), LOCTEXT("CylinderBuilderName", "Cylinder"), LOCTEXT("CylinderBuilderToolTip", "Make a cylinder brush"), FBspModeStyle::Get().GetBrush(TEXT("BspMode.CylinderBrush")));
	RegisterBspBuilderType(UCurvedStairBuilder::StaticClass(), LOCTEXT("CurvedStairBuilderName", "Curved Stair"), LOCTEXT("CurvedStairBuilderToolTip", "Make a curved stair brush"), FBspModeStyle::Get().GetBrush(TEXT("BspMode.CurvedStairBrush")));
	RegisterBspBuilderType(ULinearStairBuilder::StaticClass(), LOCTEXT("LinearStairBuilderName", "Linear Stair"), LOCTEXT("LinearStairBuilderToolTip", "Make a linear stair brush"), FBspModeStyle::Get().GetBrush(TEXT("BspMode.LinearStairBrush")));
	RegisterBspBuilderType(USpiralStairBuilder::StaticClass(), LOCTEXT("SpiralStairBuilderName", "Spiral Stair"), LOCTEXT("SpiralStairBuilderToolTip", "Make a spiral stair brush"), FBspModeStyle::Get().GetBrush(TEXT("BspMode.SpiralStairBrush")));
	RegisterBspBuilderType(UTetrahedronBuilder::StaticClass(), LOCTEXT("SphereBuilderName", "Sphere"), LOCTEXT("SphereBuilderToolTip", "Make a sphere brush"), FBspModeStyle::Get().GetBrush(TEXT("BspMode.SphereBrush")));
}


void FBspModeModule::ShutdownModule()
{
	GEditorModeTools().UnregisterMode( BspMode.ToSharedRef() );
	BspMode = NULL;

	FBspModeCommands::Unregister();

	BspBuilderTypes.Empty();
}


TSharedRef< SWidget > FBspModeModule::CreateBspModeWidget() const
{
	return SNew(SBspPalette);
}


TSharedRef< FEdMode > FBspModeModule::GetBspMode() const
{
	return BspMode.ToSharedRef();
}


void FBspModeModule::RegisterBspBuilderType( class UClass* InBuilderClass, const FText& InBuilderName, const FText& InBuilderTooltip, const FSlateBrush* InBuilderIcon )
{
	check(InBuilderClass->IsChildOf(UBrushBuilder::StaticClass()));
	BspBuilderTypes.Add(MakeShareable(new FBspBuilderType(InBuilderClass, InBuilderName, InBuilderTooltip, InBuilderIcon)));
}


void FBspModeModule::UnregisterBspBuilderType( class UClass* InBuilderClass )
{
	BspBuilderTypes.RemoveAll( 
		[InBuilderClass] ( const TSharedPtr<FBspBuilderType>& RemovalCandidate ) -> bool
		{
			return (RemovalCandidate->BuilderClass == InBuilderClass);
		}
	);
}


const TArray< TSharedPtr<FBspBuilderType> >& FBspModeModule::GetBspBuilderTypes()
{
	return BspBuilderTypes;
}


TSharedPtr<FBspBuilderType> FBspModeModule::FindBspBuilderType(UClass* InBuilderClass) const
{
	const TSharedPtr<FBspBuilderType>* FoundBuilder = BspBuilderTypes.FindByPredicate(
		[InBuilderClass] ( const TSharedPtr<FBspBuilderType>& FindCandidate ) -> bool
		{
			return (FindCandidate->BuilderClass == InBuilderClass);
		}
	);

	return FoundBuilder != nullptr ? *FoundBuilder : TSharedPtr<FBspBuilderType>();
}

IMPLEMENT_MODULE( FBspModeModule, BspMode );

#undef LOCTEXT_NAMESPACE
