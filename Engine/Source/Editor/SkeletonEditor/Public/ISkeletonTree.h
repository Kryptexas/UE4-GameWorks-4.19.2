// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

// Called when a bone is selected
DECLARE_MULTICAST_DELEGATE_OneParam(FOnObjectSelectedMulticast, UObject* /* InObject */);

// Called when an object is selected
typedef FOnObjectSelectedMulticast::FDelegate FOnObjectSelected;

/** Init params for a skeleton tree widget */
struct FSkeletonTreeArgs
{
	FSkeletonTreeArgs(FSimpleMulticastDelegate& InOnPostUndo)
		: OnPostUndo(InOnPostUndo)
	{}

	/** Multicast delegate the tree will register with for undo messages */
	FSimpleMulticastDelegate& OnPostUndo;

	/** Delegate called by the tree when a socket is selected */
	FOnObjectSelected OnObjectSelected;

	/** Optional preview scene that we can pair with */
	TSharedPtr<class IPersonaPreviewScene> PreviewScene;
};

/** Enum which tells us what type of bones we should be showing */
enum class EBoneFilter : uint8
{
	All,
	Mesh,
	LOD,
	Weighted, /** only showing weighted bones of current LOD */
	None,
	Count
};

/** Enum which tells us what type of sockets we should be showing */
enum class ESocketFilter : uint8
{
	Active,
	Mesh,
	Skeleton,
	All,
	None,
	Count
};

/** Interface used to deal with skeleton editing UI */
class ISkeletonTree : public SCompoundWidget
{
public:
	/** Get editable skeleton that this widget is editing */
	virtual TSharedRef<class IEditableSkeleton> GetEditableSkeleton() const = 0;

	/** Get preview scene that this widget is editing */
	virtual TSharedPtr<class IPersonaPreviewScene> GetPreviewScene() const = 0;

	/** Set the skeletal mesh we optionally work with */
	virtual void SetSkeletalMesh(class USkeletalMesh* NewSkeletalMesh) = 0;

	/** Set the selected socket */
	virtual void SetSelectedSocket(const struct FSelectedSocketInfo& InSocketInfo) = 0;

	/** Set the selected bone */
	virtual void SetSelectedBone(const FName& InBoneName) = 0;

	/** Deselect everything that is currently selected */
	virtual void DeselectAll() = 0;

	/** Duplicate the socket and select it */
	virtual void DuplicateAndSelectSocket(const FSelectedSocketInfo& SocketInfoToDuplicate, const FName& NewParentBoneName = FName()) = 0;

	/** Registers a delegate to be called when the selected object is changed */
	virtual void RegisterOnObjectSelected(const FOnObjectSelected& Delegate) = 0;

	/** Unregisters a delegate to be called when the selected object is changed */
	virtual void UnregisterOnObjectSelected(SWidget* Widget) = 0;
};