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
#include "WheeledVehicleMovementComponent4WDetails.h"
#include "MatineeActorDetails.h"
#include "ReflectionCaptureDetails.h"
#include "SkyLightComponentDetails.h"
#include "BrushDetails.h"
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
#include "RangeStructCustomization.h"
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
#include "VehicleTransmissionDataCustomization.h"
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
#include "InternationalizationSettingsModelDetails.h"
#include "InputSettingsDetails.h"
#include "InputStructCustomization.h"
#include "CollisionProfileDetails.h"
#include "PhysicsSettingsDetails.h"
#include "GeneralProjectSettingsDetails.h"
#include "WindowsTargetSettingsDetails.h"
#include "MoviePlayerSettingsDetails.h"
#include "SourceCodeAccessSettingsDetails.h"
#include "ParticleSystemComponentDetails.h"


IMPLEMENT_MODULE( FDetailCustomizationsModule, DetailCustomizations );


void FDetailCustomizationsModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	
	RegisterPropertyTypeCustomizations();

	RegisterObjectCustomizations();

	PropertyModule.NotifyCustomizationModuleChanged();
}


void FDetailCustomizationsModule::ShutdownModule()
{
	if( FModuleManager::Get().IsModuleLoaded( "PropertyEditor" ) )
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		// Unregister all classes customized by name
		for( auto It = RegisteredClassNames.CreateConstIterator(); It; ++It )
		{
			if( It->IsValid() )
			{
				PropertyModule.UnregisterCustomPropertyLayout( *It );
			}
		}

		// Unregister all structs
		for(auto It = RegisteredPropertyTypes.CreateConstIterator(); It; ++It)
		{
			if(It->IsValid())
			{
				PropertyModule.UnregisterCustomPropertyTypeLayout(*It);
			}
		}
	
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}


void FDetailCustomizationsModule::RegisterPropertyTypeCustomizations()
{
	// Structs
	RegisterCustomPropertyTypeLayout("StringAssetReference", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FStringAssetReferenceCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("StringClassReference", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FStringClassReferenceCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("DataTableRowHandle", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDataTableCustomizationLayout::MakeInstance));
	RegisterCustomPropertyTypeLayout("DataTableCategoryHandle", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDataTableCategoryCustomizationLayout::MakeInstance));
	RegisterCustomPropertyTypeLayout("CurveTableRowHandle", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCurveTableCustomizationLayout::MakeInstance));
	RegisterCustomPropertyTypeLayout(NAME_Vector, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMathStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout(NAME_Vector4, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMathStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout(NAME_Vector2D, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMathStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout(NAME_IntPoint, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMathStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout(NAME_Rotator, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FRotatorStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout(NAME_LinearColor, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FColorStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout(NAME_Color, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FColorStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout(NAME_Matrix, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMatrixStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout(NAME_Transform, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTransformStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("SlateColor", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSlateColorCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("AttenuationSettings", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FAttenuationSettingsCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("DialogueContext", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDialogueContextStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("DialogueWaveParameter", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDialogueWaveParameterStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("BodyInstance", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FBodyInstanceCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("VehicleTransmissionData", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FVehicleTransmissionDataCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("SlateBrush", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSlateBrushStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("SlateSound", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSlateSoundStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("Guid", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGuidStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("Key", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FKeyStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("FloatRange", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FRangeStructCustomization<float>::MakeInstance));
	RegisterCustomPropertyTypeLayout("Int32Range", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FRangeStructCustomization<int32>::MakeInstance));

	RegisterCustomPropertyTypeLayout("BlackboardEntry", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FBlackboardEntryDetails::MakeInstance));
	RegisterCustomPropertyTypeLayout("RuntimeFloatCurve", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCurveStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("EnvNamedValue", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FEnvQueryParamInstanceCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("NavigationLink", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FNavLinkStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("NavigationSegmentLink", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FNavLinkStructCustomization::MakeInstance));

	RegisterCustomPropertyTypeLayout("Margin", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMarginStructCustomization::MakeInstance));

	RegisterCustomPropertyTypeLayout("DirectoryPath", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDirectoryPathStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("FilePath", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFilePathStructCustomization::MakeInstance));

	RegisterCustomPropertyTypeLayout("InputAxisConfigEntry", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FInputAxisConfigCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("InputActionKeyMapping", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FInputActionMappingCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("InputAxisKeyMapping", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FInputAxisMappingCustomization::MakeInstance));
}

void FDetailCustomizationsModule::RegisterObjectCustomizations()
{
	// Note: By default properties are displayed in script defined order (i.e the order in the header).  These layout detail classes are called in the order seen here which will display properties
	// in the order they are customized.  This is only relevant for inheritance where both a child and a parent have properties that are customized.
	// In the order below, Actor will get a chance to display details first, followed by USceneComponent.

	RegisterCustomPropertyLayout("Actor", FOnGetDetailCustomizationInstance::CreateStatic(&FActorDetails::MakeInstance));
	RegisterCustomPropertyLayout("SceneComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FSceneComponentDetails::MakeInstance));
	RegisterCustomPropertyLayout("PrimitiveComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FPrimitiveComponentDetails::MakeInstance));
	RegisterCustomPropertyLayout("StaticMeshComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FStaticMeshComponentDetails::MakeInstance));
	RegisterCustomPropertyLayout("SkeletalMeshComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FSkeletalMeshComponentDetails::MakeInstance));
	RegisterCustomPropertyLayout("SkinnedMeshComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FSkinnedMeshComponentDetails::MakeInstance));
	RegisterCustomPropertyLayout("WheeledVehicleMovementComponent4W", FOnGetDetailCustomizationInstance::CreateStatic(&FWheeledVehicleMovementComponent4WDetails::MakeInstance));
	RegisterCustomPropertyLayout("LightComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FLightComponentDetails::MakeInstance));
	RegisterCustomPropertyLayout("PointLightComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FPointLightComponentDetails::MakeInstance));
	RegisterCustomPropertyLayout("DirectionalLightComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FDirectionalLightComponentDetails::MakeInstance));
	RegisterCustomPropertyLayout("StaticMeshActor", FOnGetDetailCustomizationInstance::CreateStatic(&FStaticMeshActorDetails::MakeInstance));
	RegisterCustomPropertyLayout("MeshComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FMeshComponentDetails::MakeInstance));
	RegisterCustomPropertyLayout("MatineeActor", FOnGetDetailCustomizationInstance::CreateStatic(&FMatineeActorDetails::MakeInstance));
	RegisterCustomPropertyLayout("ReflectionCapture", FOnGetDetailCustomizationInstance::CreateStatic(&FReflectionCaptureDetails::MakeInstance));
	RegisterCustomPropertyLayout("SkyLight", FOnGetDetailCustomizationInstance::CreateStatic(&FSkyLightComponentDetails::MakeInstance));
	RegisterCustomPropertyLayout("Brush", FOnGetDetailCustomizationInstance::CreateStatic(&FBrushDetails::MakeInstance));
	RegisterCustomPropertyLayout("AmbientSound", FOnGetDetailCustomizationInstance::CreateStatic(&FAmbientSoundDetails::MakeInstance));
	RegisterCustomPropertyLayout("WorldSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FWorldSettingsDetails::MakeInstance));
	RegisterCustomPropertyLayout("GeneralProjectSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FGeneralProjectSettingsDetails::MakeInstance));

	//@TODO: A2REMOVAL: Rename FSkeletalControlNodeDetails to something more generic
	RegisterCustomPropertyLayout("K2Node_StructMemberGet", FOnGetDetailCustomizationInstance::CreateStatic(&FSkeletalControlNodeDetails::MakeInstance));
	RegisterCustomPropertyLayout("K2Node_StructMemberSet", FOnGetDetailCustomizationInstance::CreateStatic(&FSkeletalControlNodeDetails::MakeInstance));

	RegisterCustomPropertyLayout("AnimSequence", FOnGetDetailCustomizationInstance::CreateStatic(&FAnimSequenceDetails::MakeInstance));

	RegisterCustomPropertyLayout("EditorAnimSegment", FOnGetDetailCustomizationInstance::CreateStatic(&FAnimMontageSegmentDetails::MakeInstance));
	RegisterCustomPropertyLayout("EditorAnimCompositeSegment", FOnGetDetailCustomizationInstance::CreateStatic(&FAnimMontageSegmentDetails::MakeInstance));
	RegisterCustomPropertyLayout("EditorSkeletonNotifyObj", FOnGetDetailCustomizationInstance::CreateStatic(&FSkeletonNotifyDetails::MakeInstance));
	RegisterCustomPropertyLayout("AnimStateNode", FOnGetDetailCustomizationInstance::CreateStatic(&FAnimStateNodeDetails::MakeInstance));
	RegisterCustomPropertyLayout("AnimStateTransitionNode", FOnGetDetailCustomizationInstance::CreateStatic(&FAnimTransitionNodeDetails::MakeInstance));
	RegisterCustomPropertyLayout("AnimGraphNode_TransitionPoseEvaluator", FOnGetDetailCustomizationInstance::CreateStatic(&FTransitionPoseEvaluatorNodeDetails::MakeInstance));

	RegisterCustomPropertyLayout("LandscapeSplineControlPoint", FOnGetDetailCustomizationInstance::CreateStatic(&FLandscapeSplineDetails::MakeInstance));
	RegisterCustomPropertyLayout("LandscapeSplineSegment", FOnGetDetailCustomizationInstance::CreateStatic(&FLandscapeSplineDetails::MakeInstance));

	RegisterCustomPropertyLayout("DialogueWave", FOnGetDetailCustomizationInstance::CreateStatic(&FDialogueWaveDetails::MakeInstance));
	RegisterCustomPropertyLayout("BodySetup", FOnGetDetailCustomizationInstance::CreateStatic(&FBodySetupDetails::MakeInstance));
	RegisterCustomPropertyLayout("PhysicsConstraintTemplate", FOnGetDetailCustomizationInstance::CreateStatic(&FPhysicsConstraintComponentDetails::MakeInstance));
	RegisterCustomPropertyLayout("PhysicsConstraintComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FPhysicsConstraintComponentDetails::MakeInstance));
	RegisterCustomPropertyLayout("CollisionProfile", FOnGetDetailCustomizationInstance::CreateStatic(&FCollisionProfileDetails::MakeInstance));
	RegisterCustomPropertyLayout("PhysicsSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FPhysicsSettingsDetails::MakeInstance));

	RegisterCustomPropertyLayout("ParticleModuleRequired", FOnGetDetailCustomizationInstance::CreateStatic(&FParticleModuleRequiredDetails::MakeInstance));
	RegisterCustomPropertyLayout("ParticleModuleSubUV", FOnGetDetailCustomizationInstance::CreateStatic(&FParticleModuleSubUVDetails::MakeInstance));
	RegisterCustomPropertyLayout("ParticleModuleAccelerationDrag", FOnGetDetailCustomizationInstance::CreateStatic(&FParticleModuleAccelerationDragDetails::MakeInstance));
	RegisterCustomPropertyLayout("ParticleModuleAcceleration", FOnGetDetailCustomizationInstance::CreateStatic(&FParticleModuleAccelerationDetails::MakeInstance));
	RegisterCustomPropertyLayout("ParticleModuleAccelerationDragScaleOverLife", FOnGetDetailCustomizationInstance::CreateStatic(&FParticleModuleAccelerationDragScaleOverLifeDetails::MakeInstance));
	RegisterCustomPropertyLayout("ParticleModuleCollisionGPU", FOnGetDetailCustomizationInstance::CreateStatic(&FParticleModuleCollisionGPUDetails::MakeInstance));
	RegisterCustomPropertyLayout("ParticleModuleOrbit", FOnGetDetailCustomizationInstance::CreateStatic(&FParticleModuleOrbitDetails::MakeInstance));
	RegisterCustomPropertyLayout("ParticleModuleSizeMultiplyLife", FOnGetDetailCustomizationInstance::CreateStatic(&FParticleModuleSizeMultiplyLifeDetails::MakeInstance));
	RegisterCustomPropertyLayout("ParticleModuleSizeScale", FOnGetDetailCustomizationInstance::CreateStatic(&FParticleModuleSizeScaleDetails::MakeInstance));
	RegisterCustomPropertyLayout("ParticleModuleVectorFieldScale", FOnGetDetailCustomizationInstance::CreateStatic(&FParticleModuleVectorFieldScaleDetails::MakeInstance));
	RegisterCustomPropertyLayout("ParticleModuleVectorFieldScaleOverLife", FOnGetDetailCustomizationInstance::CreateStatic(&FParticleModuleVectorFieldScaleOverLifeDetails::MakeInstance));

	RegisterCustomPropertyLayout("CameraComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FCameraDetails::MakeInstance));
	RegisterCustomPropertyLayout("DeviceProfile", FOnGetDetailCustomizationInstance::CreateStatic(&FDeviceProfileDetails::MakeInstance));
	RegisterCustomPropertyLayout("InternationalizationSettingsModel", FOnGetDetailCustomizationInstance::CreateStatic(&FInternationalizationSettingsModelDetails::MakeInstance));
	RegisterCustomPropertyLayout("InputSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FInputSettingsDetails::MakeInstance));
	RegisterCustomPropertyLayout("WindowsTargetSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FWindowsTargetSettingsDetails::MakeInstance));
	RegisterCustomPropertyLayout("MoviePlayerSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FMoviePlayerSettingsDetails::MakeInstance));

	RegisterCustomPropertyLayout("SourceCodeAccessSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FSourceCodeAccessSettingsDetails::MakeInstance));
	RegisterCustomPropertyLayout("ParticleSystemComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FParticleSystemComponentDetails::MakeInstance));
}

void FDetailCustomizationsModule::RegisterCustomPropertyLayout(FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate )
{
	check( ClassName != NAME_None );

	static FName PropertyEditor("PropertyEditor");

	// Add the class to the list of classes we should unregister later
	RegisteredClassNames.Add( ClassName );

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);

	PropertyModule.RegisterCustomPropertyLayout( ClassName, DetailLayoutDelegate );
}


void FDetailCustomizationsModule::RegisterCustomPropertyTypeLayout(FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate)
{
	check(PropertyTypeName != NAME_None);

	static FName PropertyEditor("PropertyEditor");

	RegisteredPropertyTypes.Add(PropertyTypeName);

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);

	PropertyModule.RegisterCustomPropertyTypeLayout(PropertyTypeName, PropertyTypeLayoutDelegate);
}