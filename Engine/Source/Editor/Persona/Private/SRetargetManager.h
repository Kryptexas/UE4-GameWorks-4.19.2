// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// SRetargetManager

class SRetargetManager : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SRetargetManager )
	{}

	SLATE_END_ARGS()

	/**
	* Slate construction function
	*
	* @param InArgs - Arguments passed from Slate
	*
	*/
	void Construct(const FArguments& InArgs, const TSharedRef<IEditableSkeleton>& InEditableSkeleton, const TSharedRef<IPersonaPreviewScene>& InPreviewScene, FSimpleMulticastDelegate& InOnPostUndo);

private:
	FReply OnViewRetargetBasePose();
	FReply OnResetRetargetBasePose();
	FReply OnSaveRetargetBasePose();

	/** The editable skeleton */
	TWeakPtr<class IEditableSkeleton> EditableSkeletonPtr;

	/** The preview scene  */
	TWeakPtr<class IPersonaPreviewScene> PreviewScenePtr;

	/** Delegate for undo/redo transaction **/
	void PostUndo();
	FText GetToggleRetargetBasePose() const;	
};