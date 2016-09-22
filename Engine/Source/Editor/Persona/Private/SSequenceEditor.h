// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "Persona.h"
#include "GraphEditor.h"
#include "SNodePanel.h"
#include "SAnimCurvePanel.h"
#include "SAnimEditorBase.h"

//////////////////////////////////////////////////////////////////////////
// SSequenceEditor

/** Overall animation sequence editing widget */
class SSequenceEditor : public SAnimEditorBase
{
public:
	SLATE_BEGIN_ARGS( SSequenceEditor )
		: _Sequence(NULL)
		{}

		SLATE_ARGUMENT(UAnimSequenceBase*, Sequence)
		SLATE_EVENT(FOnObjectsSelected, OnObjectsSelected)
		SLATE_EVENT(FSimpleDelegate, OnAnimNotifiesChanged)
		SLATE_EVENT(FOnInvokeTab, OnInvokeTab)

	SLATE_END_ARGS()

private:
	TSharedPtr<class SAnimNotifyPanel>	AnimNotifyPanel;
	TSharedPtr<class SAnimCurvePanel>	AnimCurvePanel;
	TSharedPtr<class SAnimTrackCurvePanel>	AnimTrackCurvePanel;
	TSharedPtr<class SAnimationScrubPanel> AnimScrubPanel;
	TWeakPtr<class IPersonaPreviewScene> PreviewScenePtr;
public:
	void Construct(const FArguments& InArgs, TSharedRef<class IPersonaPreviewScene> InPreviewScene, TSharedRef<class IEditableSkeleton> InEditableSkeleton, FSimpleMulticastDelegate& OnPostUndo, FSimpleMulticastDelegate& OnCurvesChanged);

	virtual UAnimationAsset* GetEditorObject() const override { return SequenceObj; }

private:
	/** Pointer to the animation sequence being edited */
	UAnimSequenceBase* SequenceObj;

	/** Post undo **/
	void PostUndo();
	void HandleCurvesChanged();
};
