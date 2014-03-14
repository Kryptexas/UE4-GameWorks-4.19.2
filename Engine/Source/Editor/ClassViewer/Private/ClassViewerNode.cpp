// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ClassViewerPrivatePCH.h"

#include "ClassViewerNode.h"
#include "ClassViewerFilter.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"

FClassViewerNode::FClassViewerNode( FString _ClassName, bool _bIsPlaceable)
{
	ClassName = MakeShareable(new FString(_ClassName));
	bPassesFilter = false;
	bIsClassPlaceable = _bIsPlaceable;
	bIsBPNormalType = false;

	Class = NULL;
	Blueprint = NULL;
}

FClassViewerNode::FClassViewerNode( const FClassViewerNode& InCopyObject)
{
	ClassName = InCopyObject.ClassName;
	bPassesFilter = InCopyObject.bPassesFilter;
	bIsClassPlaceable = InCopyObject.bIsClassPlaceable;

	Class = InCopyObject.Class;
	Blueprint = InCopyObject.Blueprint;
	
	UnloadedBlueprintData = InCopyObject.UnloadedBlueprintData;

	GeneratedClassPackage = InCopyObject.GeneratedClassPackage;
	GeneratedClassname = InCopyObject.GeneratedClassname;
	ParentClassname = InCopyObject.ParentClassname;
	ClassName = InCopyObject.ClassName;
	AssetName = InCopyObject.AssetName;
	bIsBPNormalType = InCopyObject.bIsBPNormalType;

	// We do not want to copy the child list, do not add it. It should be the only item missing.
}

/**
 * Adds the specified child to the node.
 *
 * @param	Child							The child to be added to this node for the tree.
 */
void FClassViewerNode::AddChild( TSharedPtr<FClassViewerNode> Child )
{
	check(Child.IsValid());
	ChildrenList.Add(Child);
}

void FClassViewerNode::AddUniqueChild(TSharedPtr<FClassViewerNode> NewChild)
{
	check(NewChild.IsValid());
	const UClass* NewChildClass = NewChild->Class.Get();
	if(NULL != NewChildClass)
	{
		for(int ChildIndex = 0; ChildIndex < ChildrenList.Num(); ++ChildIndex)
		{
			TSharedPtr<FClassViewerNode> OldChild = ChildrenList[ChildIndex];
			if(OldChild.IsValid() && OldChild->Class == NewChildClass)
			{
				const bool bNewChildHasMoreInfo = NewChild->UnloadedBlueprintData.IsValid();
				const bool bOldChildHasMoreInfo = OldChild->UnloadedBlueprintData.IsValid();
				if(bNewChildHasMoreInfo && !bOldChildHasMoreInfo)
				{
					// make sure, that new child has all needed children
					for(int OldChildIndex = 0; OldChildIndex < OldChild->ChildrenList.Num(); ++OldChildIndex)
					{
						NewChild->AddUniqueChild( OldChild->ChildrenList[OldChildIndex] );
					}

					// replace child
					ChildrenList[ChildIndex] = NewChild;
				}
				return;
			}
		}
	}

	AddChild(NewChild);
}

bool FClassViewerNode::IsRestricted() const
{
	return PropertyHandle.IsValid() && PropertyHandle->IsRestricted(*ClassName);
}