// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#ifndef __SAnimMontageScrubPanel_h__
#define __SAnimMontageScrubPanel_h__
#include "SAnimationScrubPanel.h"

class SAnimMontageScrubPanel : public SAnimationScrubPanel
{
public:

	SLATE_BEGIN_ARGS(SAnimMontageScrubPanel)
		: _Persona()
		, _LockedSequence()
		, _MontageEditor()
	{}
		SLATE_ARGUMENT( TWeakPtr<class SMontageEditor>, MontageEditor)
		SLATE_ARGUMENT(TWeakPtr<FPersona>, Persona )
		/** If you'd like to lock to one asset for this scrub control, give this**/
		SLATE_ARGUMENT(UAnimSequenceBase*, LockedSequence)
		/** View Input range **/
		SLATE_ATTRIBUTE( float, ViewInputMin )
		SLATE_ATTRIBUTE( float, ViewInputMax )
		SLATE_EVENT( FOnSetInputViewRange, OnSetInputViewRange )
		/** Called when an anim sequence is cropped before/after a selected frame */
		SLATE_EVENT( FOnCropAnimSequence, OnCropAnimSequence )
		/** Called to zero out selected frame's translation from origin */
		SLATE_EVENT( FSimpleDelegate, OnReZeroAnimSequence )
		SLATE_ARGUMENT( bool, bAllowZoom )
	SLATE_END_ARGS()

	/**
	 * Construct the widget
	 * 
	 * @param InArgs   A declaration from which to construct the widget
	 */
	void Construct( const FArguments& InArgs );
protected:
	// notifiers 
	virtual FReply OnClick_Backward_End() OVERRIDE;
	virtual FReply OnClick_Forward_End() OVERRIDE;

	virtual FReply OnClick_Backward_Step() OVERRIDE;
	virtual FReply OnClick_Forward_Step() OVERRIDE;

	virtual FReply OnClick_Forward() OVERRIDE;
	virtual FReply OnClick_Backward() OVERRIDE;

	virtual FReply OnClick_ToggleLoop() OVERRIDE;

	virtual void OnValueChanged(float NewValue) OVERRIDE;

private:
	TWeakPtr<SMontageEditor> MontageEditor;
};
#endif // __SAnimMontageScrubPanel_h__
