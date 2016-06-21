// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysXSupport.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/PhysicsAsset.h"

UPhysicsConstraintTemplate::UPhysicsConstraintTemplate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	LinearXMotion_DEPRECATED = LCM_Locked;
	LinearYMotion_DEPRECATED = LCM_Locked;
	LinearZMotion_DEPRECATED = LCM_Locked;

	Pos1_DEPRECATED = FVector(0.0f, 0.0f, 0.0f);
	PriAxis1_DEPRECATED = FVector(1.0f, 0.0f, 0.0f);
	SecAxis1_DEPRECATED = FVector(0.0f, 1.0f, 0.0f);

	Pos2_DEPRECATED = FVector(0.0f, 0.0f, 0.0f);
	PriAxis2_DEPRECATED = FVector(1.0f, 0.0f, 0.0f);
	SecAxis2_DEPRECATED = FVector(0.0f, 1.0f, 0.0f);

	LinearBreakThreshold_DEPRECATED = 300.0f;
	AngularBreakThreshold_DEPRECATED = 500.0f;

	ProjectionLinearTolerance_DEPRECATED = 0.5; // Linear projection when error > 5.0 unreal units
	ProjectionAngularTolerance_DEPRECATED = 10.f; // Angular projection when error > 10 degrees
#endif
}

void UPhysicsConstraintTemplate::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// If old content, copy properties out of setup into instance
	if(Ar.UE4Ver() < VER_UE4_ALL_PROPS_TO_CONSTRAINTINSTANCE)
	{
		CopySetupPropsToInstance(&DefaultInstance);
	}

	if(Ar.IsLoading() && !Ar.IsTransacting())
	{
		UpdateConstraintProfileMap();
#if WITH_EDITORONLY_DATA

		if(!ConstraintProfileNameMap.FindRef(DefaultProfileName))	//If no profile found make sure to copy what's in default
		{
			NoProfileInstance = DefaultInstance.ProfileInstance;
		}

		UpdateInstanceFromProfile();
#endif
	}
}

#if WITH_EDITORONLY_DATA
void UPhysicsConstraintTemplate::UpdateInstanceFromProfile()
{
	if(UPhysicsConstraintProfile* ProfileInstance = ConstraintProfileNameMap.FindRef(DefaultProfileName))
	{
		DefaultInstance.ProfileInstance = ProfileInstance->ProfileProperties;
	}
	else
	{
		//no profile found so update the no profile cache
		DefaultInstance.ProfileInstance = NoProfileInstance;
	}
}
#endif

#if WITH_EDITOR
void UPhysicsConstraintTemplate::UpdateProfileBeingEdited()
{
	if (UPhysicsConstraintProfile* Profile = ConstraintProfileNameMap.FindRef(DefaultProfileName))
	{
		//Make sure any changes we make are passed back to the profile we are currently editing
		Profile->ProfileProperties = DefaultInstance.ProfileInstance;
	}
	else
	{
		//No profile found so make sure to update the no profile cache
		NoProfileInstance = DefaultInstance.ProfileInstance;
	}
}
#endif

void UPhysicsConstraintTemplate::SanitizeProfileArray()
{
	for(FPhysicsConstraintProfileHandle& ProfileHandle : ProfileHandles)
	{
		if(ProfileHandle.Profile == nullptr)
		{
			ProfileHandle.Profile = NewObject<UPhysicsConstraintProfile>(this);
			checkSlow(ProfileHandle.Profile);	//checkSlow to make static analysis happy
			ProfileHandle.Profile->ProfileProperties = DefaultInstance.ProfileInstance;	//new objects use whatever is the current default
		}
	}
}

void UPhysicsConstraintTemplate::UpdateConstraintProfileMap()
{
	SanitizeProfileArray();

	ConstraintProfileNameMap.Empty(ProfileHandles.Num());
	for(const FPhysicsConstraintProfileHandle& ProfileHandle : ProfileHandles)
	{
		if(ProfileHandle.Profile)
		{
			ConstraintProfileNameMap.Add(ProfileHandle.ProfileName, ProfileHandle.Profile);
		}
	}
}

#if WITH_EDITOR
void UPhysicsConstraintTemplate::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if(UProperty* Property = PropertyChangedEvent.Property)
	{
		const FName PropertyName = Property->GetFName();
		FName ProfileHandlesName = GET_MEMBER_NAME_CHECKED(UPhysicsConstraintTemplate, ProfileHandles);
		FName ProfileHandleInnerName = GET_MEMBER_NAME_CHECKED(FPhysicsConstraintProfileHandle, ProfileName);	//The profile name (inside the array element)
		if(PropertyName == ProfileHandlesName || PropertyName == ProfileHandleInnerName)
		{
			UpdateConstraintProfileMap();
		}
		
		FName DefaultProfileNamePropertyName = GET_MEMBER_NAME_CHECKED(UPhysicsConstraintTemplate, DefaultProfileName);
		if (PropertyName == DefaultProfileNamePropertyName || PropertyName == ProfileHandlesName || PropertyName == ProfileHandleInnerName)
		{
			UpdateInstanceFromProfile();
		}
		
		UpdateProfileBeingEdited();
	}
}
#endif

void UPhysicsConstraintTemplate::CopySetupPropsToInstance(FConstraintInstance* Instance)
{
#if WITH_EDITORONLY_DATA
	Instance->JointName			= JointName_DEPRECATED;
	Instance->ConstraintBone1	= ConstraintBone1_DEPRECATED;
	Instance->ConstraintBone2	= ConstraintBone2_DEPRECATED;

	Instance->Pos1				= Pos1_DEPRECATED;
	Instance->PriAxis1			= PriAxis1_DEPRECATED;
	Instance->SecAxis1			= SecAxis1_DEPRECATED;
	Instance->Pos2				= Pos2_DEPRECATED;
	Instance->PriAxis2			= PriAxis2_DEPRECATED;
	Instance->SecAxis2			= SecAxis2_DEPRECATED;

	Instance->ProfileInstance.bEnableProjection				= bEnableProjection_DEPRECATED;
	Instance->ProfileInstance.ProjectionLinearTolerance		= ProjectionLinearTolerance_DEPRECATED;
	Instance->ProfileInstance.ProjectionAngularTolerance		= ProjectionAngularTolerance_DEPRECATED;
	Instance->ProfileInstance.LinearLimit.XMotion				= LinearXMotion_DEPRECATED;
	Instance->ProfileInstance.LinearLimit.YMotion				= LinearYMotion_DEPRECATED;
	Instance->ProfileInstance.LinearLimit.ZMotion				= LinearZMotion_DEPRECATED;
	Instance->ProfileInstance.LinearLimit.Limit				= LinearLimitSize_DEPRECATED;
	Instance->ProfileInstance.LinearLimit.bSoftConstraint		= bLinearLimitSoft_DEPRECATED;
	Instance->ProfileInstance.LinearLimit.Stiffness			= LinearLimitStiffness_DEPRECATED;
	Instance->ProfileInstance.LinearLimit.Damping				= LinearLimitDamping_DEPRECATED;
	Instance->ProfileInstance.bLinearBreakable				= bLinearBreakable_DEPRECATED;
	Instance->ProfileInstance.LinearBreakThreshold			= LinearBreakThreshold_DEPRECATED;
	Instance->ProfileInstance.ConeLimit.Swing1Motion			= AngularSwing1Motion_DEPRECATED;
	Instance->ProfileInstance.ConeLimit.Swing2Motion			= AngularSwing2Motion_DEPRECATED;
	Instance->ProfileInstance.TwistLimit.TwistMotion			= AngularTwistMotion_DEPRECATED;
	Instance->ProfileInstance.ConeLimit.bSoftConstraint		= bSwingLimitSoft_DEPRECATED;
	Instance->ProfileInstance.TwistLimit.bSoftConstraint		= bTwistLimitSoft_DEPRECATED;
	Instance->ProfileInstance.ConeLimit.Swing1LimitDegrees	= Swing1LimitAngle_DEPRECATED;
	Instance->ProfileInstance.ConeLimit.Swing2LimitDegrees	= Swing2LimitAngle_DEPRECATED;
	Instance->ProfileInstance.TwistLimit.TwistLimitDegrees	= TwistLimitAngle_DEPRECATED;
	Instance->ProfileInstance.ConeLimit.Stiffness				= SwingLimitStiffness_DEPRECATED;
	Instance->ProfileInstance.ConeLimit.Damping				= SwingLimitDamping_DEPRECATED;
	Instance->ProfileInstance.TwistLimit.Stiffness			= TwistLimitStiffness_DEPRECATED;
	Instance->ProfileInstance.TwistLimit.Damping				= TwistLimitDamping_DEPRECATED;
	Instance->ProfileInstance.bAngularBreakable				= bAngularBreakable_DEPRECATED;
	Instance->ProfileInstance.AngularBreakThreshold			= AngularBreakThreshold_DEPRECATED;
#endif
}