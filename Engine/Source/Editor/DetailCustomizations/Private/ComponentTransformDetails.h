// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetSelection.h"

/**
 * Manages the Transform section of a details view                    
 */
class FComponentTransformDetails : public TSharedFromThis<FComponentTransformDetails>, public IDetailCustomNodeBuilder
{
public:
	FComponentTransformDetails( const TArray< TWeakObjectPtr<UObject> >& InSelectedObjects, const FSelectedActorInfo& InSelectedActorInfo, FNotifyHook* NotifyHook );

	/**
	 * Caches the representation of the actor transform for the user input boxes                   
	 */
	void CacheTransform();

	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) OVERRIDE {}
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) OVERRIDE;
	virtual bool RequiresTick() const OVERRIDE { return true; }
	virtual FName GetName() const OVERRIDE { return NAME_None; }
	virtual bool InitiallyCollapsed() const OVERRIDE { return false; }
	virtual void Tick( float DeltaTime ) OVERRIDE;
	virtual void SetOnRebuildChildren( FSimpleDelegate OnRebuildChildren ) OVERRIDE{}

private:
	/** @return Whether the transform details panel should be enabled (editable) or not (read-only / greyed out) */
	bool GetIsEnabled() const;

	/**
	 * Sets the selected object(s) translation (relative)
	 *
	 * @param NewValue The new translation value
	 */
	void OnSetLocation( float NewValue, ETextCommit::Type CommitInfo, int32 Axis );

	/**
	 * Sets the selected object(s) rotation (relative)
	 *
	 * @param NewValue		The new rotation value for the axis
	 * @param bCommittted	true if the value was committed, false is the value comes from the slider (int32 due to delegate restrictions)
	 * @param Axis			The axis of rotation which changed.  0 = Roll, 1 = Pitch, 2 = Yaw
	 */
	void OnSetRotation( float NewValue, bool bCommitted, int32 Axis );

	/**
	 * Sets the selected object(s) rotation (relative)
	 *
	 * @param CommitInfo	Whether or not this was committed from pressing enter or loosing focus
	 * @param Axis			The axis of rotation which changed.  0 = Roll, 1 = Pitch, 2 = Yaw
	 * @param NewValue		The new rotation value for the axis
	 */
	void OnRotationCommitted( float NewValue, ETextCommit::Type CommitInfo, int32 Axis );

	/**
	 * Called when the one of the axis sliders for object rotation begins to change for the first time 
	 */
	void OnBeginRotatonSlider();

	/**
	 * Called when the one of the axis sliders for object rotation is released
	 */
	void OnEndRotationSlider(float NewValue);

	/**
	 * Sets the selected object(s) scale (relative)
	 *
	 * @param NewValue The new scale value
	 */
	void OnSetScale( float NewValue, ETextCommit::Type CommitInfo, int32 Axis );

	/**
	 * Sets the selected object(s) scale or mirrors them (relative)
	 * Helper function for OnSetScale and On*ScaleMirrored.
	 *
	 * @param NewValue					The new scale value - unused if mirroring
	 * @param Axis						The axis we are manipulating
	 * @param bMirror					Whether to mirror the axis
	 * @param TransactionSessionName	the name of the undo/redo transaction
	 */
	void ScaleObject( float NewValue, int32 Axis, bool bMirror, const FText& TransactionSessionName );

	/** @return Icon to use in the preserve scale ratio check box */
	const FSlateBrush* GetPreserveScaleRatioImage() const;

	/** @return The state of the preserve scale ratio checkbox */
	ESlateCheckBoxState::Type IsPreserveScaleRatioChecked() const;

	/**
	 * Called when the preserve scale ratio checkbox is toggled
	 */
	void OnPreserveScaleRatioToggled( ESlateCheckBoxState::Type NewState );

	/**
	 * @return the Label to display for translation                   
	 */
	FString GetLocationLabel() const;

	/**
	 * @return the Label to display for rotation                   
	 */
	FString GetRotationLabel() const;

	/**
	 * @return the Label to display for scale                   
	 */
	FString GetScaleLabel() const;

	/**
	 * Called when the location label is clicked (toggles between absolute and relative)                   
	 */
	void OnLocationLabelClicked( );

	/**
	 * Called when the rotation label is clicked (toggles between absolute and relative)                   
	 */
	void OnRotationLabelClicked( );

	/**
	 * Called when the scale label is clicked (toggles between absolute and relative)                   
	 */
	void OnScaleLabelClicked( );

	/** Called when the "Reset to Default" button for the location has been clicked */
	FReply OnLocationResetClicked();

	/** Called when the "Reset to Default" button for the rotation has been clicked */
	FReply OnRotationResetClicked();

	/** Called when the "Reset to Default" button for the scale has been clicked */
	FReply OnScaleResetClicked();

	/** Extend the context menu for the X component */
	void ExtendXScaleContextMenu( FMenuBuilder& MenuBuilder );
	/** Extend the context menu for the Y component */
	void ExtendYScaleContextMenu( FMenuBuilder& MenuBuilder );
	/** Extend the context menu for the Z component */
	void ExtendZScaleContextMenu( FMenuBuilder& MenuBuilder );

	/** Called when the X axis scale is mirrored */
	void OnXScaleMirrored();
	/** Called when the Y axis scale is mirrored */
	void OnYScaleMirrored();
	/** Called when the Z axis scale is mirrored */
	void OnZScaleMirrored();

	/** @return The X component of location */
	TOptional<float> GetLocationX() const { return CachedLocation.X; }
	/** @return The Y component of location */
	TOptional<float> GetLocationY() const { return CachedLocation.Y; }
	/** @return The Z component of location */
	TOptional<float> GetLocationZ() const { return CachedLocation.Z; }
	/** @return The visibility of the "Reset to Default" button for the location component */
	EVisibility GetLocationResetVisibility() const { return CachedLocation.X.GetValue() == 0.f && CachedLocation.Y.GetValue() == 0.f && CachedLocation.Z.GetValue() == 0.f ? EVisibility::Hidden : EVisibility::Visible; }

	/** @return The X component of rotation */
	TOptional<float> GetRotationX() const { return CachedRotation.X; }
	/** @return The Y component of rotation */
	TOptional<float> GetRotationY() const { return CachedRotation.Y; }
	/** @return The Z component of rotation */
	TOptional<float> GetRotationZ() const { return CachedRotation.Z; }
	/** @return The visibility of the "Reset to Default" button for the rotation component */
	EVisibility GetRotationResetVisibility() const { return CachedRotation.X.GetValue() == 0.f && CachedRotation.Y.GetValue() == 0.f && CachedRotation.Z.GetValue() == 0.f ? EVisibility::Hidden : EVisibility::Visible; }

	/** @return The X component of scale */
	TOptional<float> GetScaleX() const { return CachedScale.X; }
	/** @return The Y component of scale */
	TOptional<float> GetScaleY() const { return CachedScale.Y; }
	/** @return The Z component of scale */
	TOptional<float> GetScaleZ() const { return CachedScale.Z; }
	/** @return The visibility of the "Reset to Default" button for the scale component */
	EVisibility GetScaleResetVisibility() const { return CachedScale.X.GetValue() == 1.f && CachedScale.Y.GetValue() == 1.f && CachedScale.Z.GetValue() == 1.f ? EVisibility::Hidden : EVisibility::Visible; }

private:
	/** A vector where it may optionally be unset */
	struct FOptionalVector
	{
		/**
		 * Sets the value from an FVector                   
		 */
		void Set( const FVector& InVec )
		{
			X = InVec.X;
			Y = InVec.Y;
			Z = InVec.Z;
		}

		/**
		 * Sets the value from an FRotator                   
		 */
		void Set( const FRotator& InRot )
		{
			X = InRot.Roll;
			Y = InRot.Pitch;
			Z = InRot.Yaw;
		}

		/** @return Whether or not the value is set */
		bool IsSet() const
		{
			// The vector is set if all values are set
			return X.IsSet() && Y.IsSet() && Z.IsSet();
		}

		TOptional<float> X;
		TOptional<float> Y;
		TOptional<float> Z;
	};
	
	FSelectedActorInfo SelectedActorInfo;
	/** Copy of selected actor references in the details view */
	TArray< TWeakObjectPtr<UObject> > SelectedObjects;
	/** Cache translation value of the selected set */
	FOptionalVector CachedLocation;
	/** Cache rotation value of the selected set */
	FOptionalVector CachedRotation;
	/** Cache scale value of the selected set */
	FOptionalVector CachedScale;
	/** Notify hook to use */
	FNotifyHook* NotifyHook;
	/** Whether or not we are in absolute translation mode */
	bool bAbsoluteLocation;
	/** Whether or not we are in absolute rotation mode */
	bool bAbsoluteRotation;
	/** Whether or not we are in absolute scale mode */
	bool bAbsoluteScale;
	/** Whether or not to preserve scale ratios */
	bool bPreserveScaleRatio;
	/** Mapping from object to relative rotation values which are not affected by Quat->Rotator conversions during transform calculations */
	TMap< UObject*, FRotator > ObjectToRelativeRotationMap;
	/** Flag to indicate we are currently editing the rotation in the UI, so we should rely on the cached value in objectToRelativeRotationMap, not the value from the object */
	bool bEditingRotationInUI;
};
