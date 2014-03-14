// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "DetailCustomizations.h"
#include "StaticMeshComponentDetails.h"
#include "LightComponentDetails.h"
#include "PointLightComponentDetails.h"
#include "DirectionalLightComponentDetails.h"
#include "SceneComponentDetails.h"
#include "PrimitiveComponentDetails.h"
#include "StaticMeshActorDetails.h"
#include "SkinnedMeshComponentDetails.h"
#include "SkeletalMeshComponentDetails.h"
#include "MeshComponentDetails.h"
#include "MatineeActorDetails.h"
#include "ReflectionCaptureDetails.h"
#include "SkyLightComponentDetails.h"
#include "BrushDetails.h"
#include "EmitterDetails.h"
#include "ActorDetails.h"
#include "SkeletalControlNodeDetails.h"
#include "AnimMontageSegmentDetails.h"
#include "AnimSequenceDetails.h"
#include "AnimStateNodeDetails.h"
#include "AnimTransitionNodeDetails.h"
#include "AmbientSoundDetails.h"
#include "ModuleManager.h"
#include "AnimGraphDefinitions.h"
#include "SoundDefinitions.h"
#include "TransitionPoseEvaluatorNodeDetails.h"
#include "LandscapeSplineDetails.h"
#include "MathStructCustomizations.h"
#include "MathStructProxyCustomizations.h"
#include "StringAssetReferenceCustomization.h"
#include "StringClassReferenceCustomization.h"
#include "AttenuationSettingsCustomizations.h"
#include "WorldSettingsDetails.h"
#include "DialogueStructsCustomizations.h"
#include "DataTableCustomization.h"
#include "DataTableCategoryCustomization.h"
#include "CurveTableCustomization.h"
#include "DialogueWaveDetails.h"
#include "BodyInstanceCustomization.h"
#include "BodySetupDetails.h"
#include "Runtime/Engine/Classes/PhysicsEngine/BodySetup.h"
#include "SlateBrushCustomization.h"
#include "SlateSoundCustomization.h"
#include "MarginCustomization.h"
#include "SceneComponentDetails.h"
#include "PhysicsConstraintComponentDetails.h"
#include "Runtime/Engine/Classes/PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Runtime/Engine/Classes/PhysicsEngine/PhysicsConstraintComponent.h"
#include "GuidStructCustomization.h"
#include "ParticleModuleDetails.h"
#include "CameraDetails.h"
#include "BlackboardEntryDetails.h"
#include "EnvQueryParamInstanceCustomization.h"
#include "SkeletonNotifyDetails.h"
#include "SlateColorCustomization.h"
#include "CurveStructCustomization.h"
#include "NavLinkStructCustomization.h"
#include "DirectoryPathStructCustomization.h"
#include "FilePathStructCustomization.h"
#include "DeviceProfileDetails.h"
#include "KeyStructCustomization.h"
#include "EditorStyleSettingsDetails.h"
#include "InputSettingsDetails.h"
#include "InputStructCustomization.h"
#include "CollisionProfileDetails.h"
#include "PhysicsSettingsDetails.h"
#include "GeneralProjectSettingsDetails.h"

IMPLEMENT_MODULE( FDetailCustomizationsModule, DetailCustomizations );


void FDetailCustomizationsModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	
	RegisterStructPropertyLayout( "StringAssetReference", FOnGetStructCustomizationInstance::CreateStatic( &FStringAssetReferenceCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "StringClassReference", FOnGetStructCustomizationInstance::CreateStatic( &FStringClassReferenceCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "DataTableRowHandle", FOnGetStructCustomizationInstance::CreateStatic( &FDataTableCustomizationLayout::MakeInstance ) );
	RegisterStructPropertyLayout( "DataTableCategoryHandle", FOnGetStructCustomizationInstance::CreateStatic( &FDataTableCategoryCustomizationLayout::MakeInstance ) );
	RegisterStructPropertyLayout( "CurveTableRowHandle", FOnGetStructCustomizationInstance::CreateStatic( &FCurveTableCustomizationLayout::MakeInstance ) );
	RegisterStructPropertyLayout( NAME_Vector, FOnGetStructCustomizationInstance::CreateStatic( &FMathStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( NAME_Vector4, FOnGetStructCustomizationInstance::CreateStatic( &FMathStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( NAME_Vector2D, FOnGetStructCustomizationInstance::CreateStatic( &FMathStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( NAME_IntPoint, FOnGetStructCustomizationInstance::CreateStatic( &FMathStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( NAME_Rotator, FOnGetStructCustomizationInstance::CreateStatic( &FRotatorStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( NAME_LinearColor, FOnGetStructCustomizationInstance::CreateStatic( &FColorStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( NAME_Color, FOnGetStructCustomizationInstance::CreateStatic( &FColorStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( NAME_Matrix, FOnGetStructCustomizationInstance::CreateStatic( &FMatrixStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( NAME_Transform, FOnGetStructCustomizationInstance::CreateStatic( &FTransformStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "SlateColor", FOnGetStructCustomizationInstance::CreateStatic( &FSlateColorCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "AttenuationSettings", FOnGetStructCustomizationInstance::CreateStatic( &FAttenuationSettingsCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "DialogueContext", FOnGetStructCustomizationInstance::CreateStatic( &FDialogueContextStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "DialogueWaveParameter", FOnGetStructCustomizationInstance::CreateStatic( &FDialogueWaveParameterStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "BodyInstance", FOnGetStructCustomizationInstance::CreateStatic( &FBodyInstanceCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "SlateBrush", FOnGetStructCustomizationInstance::CreateStatic( &FSlateBrushStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "SlateSound", FOnGetStructCustomizationInstance::CreateStatic( &FSlateSoundStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "Guid", FOnGetStructCustomizationInstance::CreateStatic( &FGuidStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "Key", FOnGetStructCustomizationInstance::CreateStatic(&FKeyStructCustomization::MakeInstance));

	RegisterStructPropertyLayout( "BlackboardEntry", FOnGetStructCustomizationInstance::CreateStatic( &FBlackboardEntryDetails::MakeInstance ) );
	RegisterStructPropertyLayout( "RuntimeFloatCurve", FOnGetStructCustomizationInstance::CreateStatic( &FCurveStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "EnvNamedValue", FOnGetStructCustomizationInstance::CreateStatic( &FEnvQueryParamInstanceCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "NavigationLink", FOnGetStructCustomizationInstance::CreateStatic( &FNavLinkStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "NavigationSegmentLink", FOnGetStructCustomizationInstance::CreateStatic( &FNavLinkStructCustomization::MakeInstance ) );

	RegisterStructPropertyLayout( "Margin", FOnGetStructCustomizationInstance::CreateStatic( &FMarginStructCustomization::MakeInstance ) );

	RegisterStructPropertyLayout( "DirectoryPath", FOnGetStructCustomizationInstance::CreateStatic( &FDirectoryPathStructCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "FilePath", FOnGetStructCustomizationInstance::CreateStatic( &FFilePathStructCustomization::MakeInstance ) );

	RegisterStructPropertyLayout( "InputAxisConfigEntry", FOnGetStructCustomizationInstance::CreateStatic( &FInputAxisConfigCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "InputActionKeyMapping", FOnGetStructCustomizationInstance::CreateStatic( &FInputActionMappingCustomization::MakeInstance ) );
	RegisterStructPropertyLayout( "InputAxisKeyMapping", FOnGetStructCustomizationInstance::CreateStatic( &FInputAxisMappingCustomization::MakeInstance ) );


	// Note: By default properties are displayed in script defined order (i.e the order in the header).  These layout detail classes are called in the order seen here which will display properties
	// in the order they are customized.  This is only relevant for inheritance where both a child and a parent have properties that are customized.
	// In the order below, Actor will get a chance to display details first, followed by USceneComponent.

	RegisterCustomPropertyLayout( AActor::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FActorDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( USceneComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FSceneComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UPrimitiveComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FPrimitiveComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UStaticMeshComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FStaticMeshComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( USkeletalMeshComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FSkeletalMeshComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( USkinnedMeshComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FSkinnedMeshComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( ULightComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FLightComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UPointLightComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FPointLightComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UDirectionalLightComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FDirectionalLightComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( AStaticMeshActor::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FStaticMeshActorDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UMeshComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FMeshComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( AMatineeActor::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FMatineeActorDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( AReflectionCapture::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FReflectionCaptureDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( ASkyLight::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FSkyLightComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( ABrush::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FBrushDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( AEmitter::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FEmitterDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( AAmbientSound::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FAmbientSoundDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( AWorldSettings::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FWorldSettingsDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UGeneralProjectSettings::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FGeneralProjectSettingsDetails::MakeInstance));

	//@TODO: A2REMOVAL: Rename FSkeletalControlNodeDetails to something more generic
	RegisterCustomPropertyLayout( UK2Node_StructMemberGet::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FSkeletalControlNodeDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UK2Node_StructMemberSet::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FSkeletalControlNodeDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( UAnimSequence::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FAnimSequenceDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( UEditorAnimSegment::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FAnimMontageSegmentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UEditorAnimCompositeSegment::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FAnimMontageSegmentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UEditorSkeletonNotifyObj::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FSkeletonNotifyDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UAnimStateNode::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FAnimStateNodeDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UAnimStateTransitionNode::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FAnimTransitionNodeDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UAnimGraphNode_TransitionPoseEvaluator::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FTransitionPoseEvaluatorNodeDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( ULandscapeSplineControlPoint::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FLandscapeSplineDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( ULandscapeSplineSegment::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FLandscapeSplineDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( UDialogueWave::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FDialogueWaveDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UBodySetup::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FBodySetupDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UPhysicsConstraintTemplate::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FPhysicsConstraintComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UPhysicsConstraintComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FPhysicsConstraintComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UCollisionProfile::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FCollisionProfileDetails::MakeInstance));
	RegisterCustomPropertyLayout( UPhysicsSettings::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FPhysicsSettingsDetails::MakeInstance));

	RegisterCustomPropertyLayout( UParticleModuleColorOverLife::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleColorOverLifeDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UParticleModuleRequired::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleRequiredDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UParticleModuleSubUV::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleSubUVDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UParticleModuleAccelerationDrag::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleAccelerationDragDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UParticleModuleAcceleration::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleAccelerationDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UParticleModuleAccelerationDragScaleOverLife::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleAccelerationDragScaleOverLifeDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UParticleModuleCollisionGPU::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleCollisionGPUDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UParticleModuleOrbit::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleOrbitDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UParticleModuleSizeMultiplyLife::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleSizeMultiplyLifeDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UParticleModuleSizeScale::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleSizeScaleDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UParticleModuleVectorFieldScale::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleVectorFieldScaleDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( UParticleModuleVectorFieldScaleOverLife::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleVectorFieldScaleOverLifeDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( UCameraComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FCameraDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( UDeviceProfile::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FDeviceProfileDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( UEditorStyleSettings::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FEditorStyleSettingsDetails::MakeInstance ) );

	RegisterCustomPropertyLayout(UInputSettings::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FInputSettingsDetails::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();
}

void FDetailCustomizationsModule::ShutdownModule()
{
	if( FModuleManager::Get().IsModuleLoaded( "PropertyEditor" ) )
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		// Unregister all classes
		for( auto It = RegisteredClasses.CreateConstIterator(); It; ++It )
		{
			if( It->IsValid() )
			{
				PropertyModule.UnregisterCustomPropertyLayout( It->Get() );
			}
		}

		// Unregister all structs
		for(auto It = RegisteredStructs.CreateConstIterator(); It; ++It)
		{
			if(It->IsValid())
			{
				PropertyModule.UnregisterStructPropertyLayout(*It);
			}
		}
	
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FDetailCustomizationsModule::RegisterCustomPropertyLayout( UClass* Class, FOnGetDetailCustomizationInstance DetailLayoutDelegate )
{
	check( Class );

	// Add the class to the list of classes we should unregister later
	RegisteredClasses.Add( Class );

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomPropertyLayout( Class, DetailLayoutDelegate );
}

void FDetailCustomizationsModule::RegisterStructPropertyLayout(FName StructTypeName, FOnGetStructCustomizationInstance StructLayoutDelegate)
{
	check(StructTypeName != NAME_None);

	RegisteredStructs.Add(StructTypeName);

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterStructPropertyLayout(StructTypeName, StructLayoutDelegate);
}