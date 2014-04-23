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
	RegisterStructPropertyLayout( "FloatRange", FOnGetStructCustomizationInstance::CreateStatic( &FRangeStructCustomization<float>::MakeInstance ) );
	RegisterStructPropertyLayout( "Int32Range", FOnGetStructCustomizationInstance::CreateStatic( &FRangeStructCustomization<int32>::MakeInstance ) );

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

	RegisterCustomPropertyLayout( "Actor", FOnGetDetailCustomizationInstance::CreateStatic( &FActorDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "SceneComponent", FOnGetDetailCustomizationInstance::CreateStatic( &FSceneComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "PrimitiveComponent", FOnGetDetailCustomizationInstance::CreateStatic( &FPrimitiveComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "StaticMeshComponent", FOnGetDetailCustomizationInstance::CreateStatic( &FStaticMeshComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "SkeletalMeshComponent", FOnGetDetailCustomizationInstance::CreateStatic( &FSkeletalMeshComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "SkinnedMeshComponent", FOnGetDetailCustomizationInstance::CreateStatic( &FSkinnedMeshComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "LightComponent", FOnGetDetailCustomizationInstance::CreateStatic( &FLightComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "PointLightComponent", FOnGetDetailCustomizationInstance::CreateStatic( &FPointLightComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "DirectionalLightComponent", FOnGetDetailCustomizationInstance::CreateStatic( &FDirectionalLightComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "StaticMeshActor", FOnGetDetailCustomizationInstance::CreateStatic( &FStaticMeshActorDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "MeshComponent", FOnGetDetailCustomizationInstance::CreateStatic( &FMeshComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "MatineeActor", FOnGetDetailCustomizationInstance::CreateStatic( &FMatineeActorDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "ReflectionCapture", FOnGetDetailCustomizationInstance::CreateStatic( &FReflectionCaptureDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "SkyLight", FOnGetDetailCustomizationInstance::CreateStatic( &FSkyLightComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "Brush", FOnGetDetailCustomizationInstance::CreateStatic( &FBrushDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "Emitter", FOnGetDetailCustomizationInstance::CreateStatic( &FEmitterDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "AmbientSound", FOnGetDetailCustomizationInstance::CreateStatic( &FAmbientSoundDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "WorldSettings", FOnGetDetailCustomizationInstance::CreateStatic( &FWorldSettingsDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "GeneralProjectSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FGeneralProjectSettingsDetails::MakeInstance));

	//@TODO: A2REMOVAL: Rename FSkeletalControlNodeDetails to something more generic
	RegisterCustomPropertyLayout( "K2Node_StructMemberGet", FOnGetDetailCustomizationInstance::CreateStatic( &FSkeletalControlNodeDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "K2Node_StructMemberSet", FOnGetDetailCustomizationInstance::CreateStatic( &FSkeletalControlNodeDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( "AnimSequence", FOnGetDetailCustomizationInstance::CreateStatic( &FAnimSequenceDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( "EditorAnimSegment", FOnGetDetailCustomizationInstance::CreateStatic( &FAnimMontageSegmentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "EditorAnimCompositeSegment", FOnGetDetailCustomizationInstance::CreateStatic( &FAnimMontageSegmentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "EditorSkeletonNotifyObj", FOnGetDetailCustomizationInstance::CreateStatic( &FSkeletonNotifyDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "AnimStateNode", FOnGetDetailCustomizationInstance::CreateStatic( &FAnimStateNodeDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "AnimStateTransitionNode", FOnGetDetailCustomizationInstance::CreateStatic( &FAnimTransitionNodeDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "AnimGraphNode_TransitionPoseEvaluator", FOnGetDetailCustomizationInstance::CreateStatic( &FTransitionPoseEvaluatorNodeDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( "LandscapeSplineControlPoint", FOnGetDetailCustomizationInstance::CreateStatic( &FLandscapeSplineDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "LandscapeSplineSegment", FOnGetDetailCustomizationInstance::CreateStatic( &FLandscapeSplineDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( "DialogueWave", FOnGetDetailCustomizationInstance::CreateStatic( &FDialogueWaveDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "BodySetup", FOnGetDetailCustomizationInstance::CreateStatic( &FBodySetupDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "PhysicsConstraintTemplate", FOnGetDetailCustomizationInstance::CreateStatic( &FPhysicsConstraintComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "PhysicsConstraintComponent", FOnGetDetailCustomizationInstance::CreateStatic( &FPhysicsConstraintComponentDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "CollisionProfile", FOnGetDetailCustomizationInstance::CreateStatic(&FCollisionProfileDetails::MakeInstance));
	RegisterCustomPropertyLayout( "PhysicsSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FPhysicsSettingsDetails::MakeInstance));

	RegisterCustomPropertyLayout( "ParticleModuleColorOverLife", FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleColorOverLifeDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "ParticleModuleRequired", FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleRequiredDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "ParticleModuleSubUV", FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleSubUVDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "ParticleModuleAccelerationDrag", FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleAccelerationDragDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "ParticleModuleAcceleration", FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleAccelerationDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "ParticleModuleAccelerationDragScaleOverLife", FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleAccelerationDragScaleOverLifeDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "ParticleModuleCollisionGPU", FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleCollisionGPUDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "ParticleModuleOrbit", FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleOrbitDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "ParticleModuleSizeMultiplyLife", FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleSizeMultiplyLifeDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "ParticleModuleSizeScale", FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleSizeScaleDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "ParticleModuleVectorFieldScale", FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleVectorFieldScaleDetails::MakeInstance ) );
	RegisterCustomPropertyLayout( "ParticleModuleVectorFieldScaleOverLife", FOnGetDetailCustomizationInstance::CreateStatic( &FParticleModuleVectorFieldScaleOverLifeDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( "CameraComponent", FOnGetDetailCustomizationInstance::CreateStatic( &FCameraDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( "DeviceProfile", FOnGetDetailCustomizationInstance::CreateStatic( &FDeviceProfileDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( "InternationalizationSettingsModel", FOnGetDetailCustomizationInstance::CreateStatic( &FInternationalizationSettingsModelDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( "InputSettings", FOnGetDetailCustomizationInstance::CreateStatic( &FInputSettingsDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( "WindowsTargetSettings", FOnGetDetailCustomizationInstance::CreateStatic( &FWindowsTargetSettingsDetails::MakeInstance ) );

	RegisterCustomPropertyLayout( "MoviePlayerSettings", FOnGetDetailCustomizationInstance::CreateStatic( &FMoviePlayerSettingsDetails::MakeInstance ) );

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

void FDetailCustomizationsModule::RegisterCustomPropertyLayout(FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate )
{
	check( ClassName != NAME_None );

	// Add the class to the list of classes we should unregister later
	RegisteredClassNames.Add( ClassName );

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomPropertyLayout( ClassName, DetailLayoutDelegate );
}

void FDetailCustomizationsModule::RegisterStructPropertyLayout(FName StructTypeName, FOnGetStructCustomizationInstance StructLayoutDelegate)
{
	check(StructTypeName != NAME_None);

	RegisteredStructs.Add(StructTypeName);

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterStructPropertyLayout(StructTypeName, StructLayoutDelegate);
}