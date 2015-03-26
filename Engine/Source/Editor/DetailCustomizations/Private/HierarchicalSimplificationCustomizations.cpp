// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "HierarchicalSimplificationCustomizations.h"
#include "Developer/MeshUtilities/Public/MeshUtilities.h"

TSharedRef<IPropertyTypeCustomization> FHierarchicalSimplificationCustomizations::MakeInstance() 
{
	return MakeShareable( new FHierarchicalSimplificationCustomizations );
}

void FHierarchicalSimplificationCustomizations::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

void FHierarchicalSimplificationCustomizations::CustomizeChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren( NumChildren );

	TMap<FName, TSharedPtr< IPropertyHandle > > PropertyHandles;

	for( uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex )
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle( ChildIndex ).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		PropertyHandles.Add(PropertyName, ChildHandle);
	}

	TSharedPtr< IPropertyHandle > SimplifyMeshPropertyHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FHierarchicalSimplification, bSimplifyMesh));

	IDetailPropertyRow& SimplifyMeshRow = ChildBuilder.AddChildProperty(SimplifyMeshPropertyHandle.ToSharedRef());
	SimplifyMeshRow.Visibility(TAttribute<EVisibility>(this, &FHierarchicalSimplificationCustomizations::IsSimplifyMeshVisible));

	for( auto Iter(PropertyHandles.CreateConstIterator()); Iter; ++Iter  )
	{
		if (Iter.Value() != SimplifyMeshPropertyHandle)
		{
			ChildBuilder.AddChildProperty(Iter.Value().ToSharedRef());
		}
	}
}

EVisibility FHierarchicalSimplificationCustomizations::IsSimplifyMeshVisible() const
{
	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	if (MeshUtilities.GetMeshMergingInterface() != nullptr)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}

