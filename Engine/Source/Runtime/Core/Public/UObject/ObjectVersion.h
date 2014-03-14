// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ObjectVersion.h: Unreal object version.
=============================================================================*/

#pragma once

// Prevents incorrect files from being loaded.

#define PACKAGE_FILE_TAG			0x9E2A83C1
#define PACKAGE_FILE_TAG_SWAPPED	0xC1832A9E

// Cast to ensure that the construct cannot be used in a #if without compiler error.
// This is useful as enum vales cannot be seen by the preprocessor.
#define PREPROCESSOR_ENUM_PROTECT(a) ((unsigned int)(a))

// this is the minimum UE3 version we can accept...no effective difference between this and 864
#define VER_MIN_ENGINE_UE3	852

// this is forever the UE3 engine version in UE4
#define VER_LAST_ENGINE_UE3	864

enum EUnrealEngineObjectUE4Version
{
	// Added array support to blueprints
	VER_UE4_ADD_PINTYPE_ARRAY = 108,
	// Remove redundant key from raw animation data
	VER_UE4_REMOVE_REDUNDANT_KEY,
	// Changing from WORDs to UINTs in the shader cache serialization, needs a new version
	VER_UE4_SUPPORT_LARGE_SHADERS,
	// Added material functions to FMaterialShaderMapId
	VER_UE4_FUNCTIONS_IN_SHADERMAPID,
	// Added asset registry tags to the package summary so the editor can learn more about the assets in the package without loading it
	VER_UE4_ASSET_REGISTRY_TAGS,
	// Removed DontSortCategories option to classes
	VER_UE4_DONTSORTCATEGORIES_REMOVED,
	// Added Tiled navmesh generation and redone navmesh serialization
	VER_UE4_TILED_NAVMESH,
	// Removed old pylon-based navigation mesh system
	VER_UE4_REMOVED_OLD_NAVMESH,
	// AnimNotify name change
	VER_UE4_ANIMNOTIFY_NAMECHANGE,
	// Removed/consolidated some properties used only in the header parser that should never be serialized
	VER_UE4_CONSOLIDATE_HEADER_PARSER_ONLY_PROPERTIES,
	// Made ComponentNameToDefaultObjectMap non-serialized
	VER_UE4_STOPPED_SERIALIZING_COMPONENTNAMETODEFAULTOBJECTMAP,
	// Reset ModifyFrequency on static lights
	VER_UE4_RESET_MODIFYFREQUENCY_STATICLIGHTS,
	// Add a GUID to SoundNodeWave
	VER_UE4_ADD_SOUNDNODEWAVE_GUID,
	// Add audio to DDC
	VER_UE4_ADD_SOUNDNODEWAVE_TO_DDC,
	// - Fix for Material Blend Mode override
	VER_UE4_MATERIAL_BLEND_OVERRIDE,
	// Ability to save cooked audio
	VER_UE4_ADD_COOKED_TO_SOUND_NODE_WAVE,
	// Update the derived data key for textures.
	VER_UE4_TEXTURE_DERIVED_DATA2,
	// Textures can now be cooked into packages
	VER_UE4_ADD_COOKED_TO_TEXTURE2D,
	// Ability to save cooked PhysX meshes
	VER_UE4_ADD_COOKED_TO_BODY_SETUP,
	// Blueprint saved before this may need Event Graph change to Local/Server Graph
	VER_UE4_ADD_KISMETNETWORKGRAPHS,
	// - Added material quality level switches
	VER_UE4_MATERIAL_QUALITY_LEVEL_SWITCH,
	// - Debugging material shader uniform expression sets.
	VER_DEBUG_MATERIALSHADER_UNIFORM_EXPRESSIONS,
	// Removed StripData
	VER_UE4_REMOVED_STRIP_DATA,
	// Setting RF_Transactional object flag on blueprint's SimpleConstructionScript
	VER_UE4_FLAG_SCS_TRANSACTIONAL,
	// - Fixing chunk bounding boxes in imported NxDestructibleAssets.
	VER_UE4_NX_DESTRUCTIBLE_ASSET_CHUNK_BOUNDS_FIX,
	// Add support for StaticMesh sockets
	VER_UE4_STATIC_MESH_SOCKETS,
	// - Removed extra skelmesh vert weights
	VER_UE4_REMOVE_EXTRA_SKELMESH_VERTEX_INFLUENCES,
	// - Change UCurve objects to use FRichCurve
	VER_UE4_UCURVE_USING_RICHCURVES,
	// Add support for inline shaders
	VER_UE4_INLINE_SHADERS,
	// Change additive types to include mesh rotation only to be baked
	VER_UE4_ADDITIVE_TYPE_CHANGE, 
	// Readd cooker versioning to package
	VER_UE4_READD_COOKER,
	// Serialize class properties
	VER_UE4_ADDED_SCRIPT_SERIALIZATION_FOR_BLUEPRINT_GENERATED_CLASSES,
	// Variable UBoolProperty size.
	VER_UE4_VARIABLE_BITFIELD_SIZE,
	// Fix skeletons which only list active bones in their required bones list.
	VER_UE4_FIX_REQUIRED_BONES,
	// Switched 'cooked package' version to simply be the package version itself.
	VER_UE4_COOKED_PACKAGE_VERSION_IS_PACKAGE_VERSION,
	// Refactor how texture source art is stored to better isolate editor-only data.
	VER_UE4_TEXTURE_SOURCE_ART_REFACTOR,
	// Add additional settings to static and skeletal mesh optimization struct (FStaticMeshOptimizationSettings and FSkeletalMeshOptimizationSettings)
	VER_UE4_ADDED_EXTRA_MESH_OPTIMIZATION_SETTINGS,
	// Add BodySetup to DestructibleMesh, use it to store the destructible physical material.
	VER_UE4_DESTRUCTIBLE_MESH_BODYSETUP_HOLDS_PHYSICAL_MATERIAL,
	// Remove USequence class and references
	VER_UE4_REMOVE_USEQUENCE,
	// Added by-ref parameters to blueprints
	VER_UE4_ADD_PINTYPE_BYREF,
	// Change to make public blueprint variables 'read only'
	VER_UE4_PUBLIC_BLUEPRINT_VARS_READONLY,
	// change HiddenGame, DrawInGame, DrawInEditor to bVisible, and bHiddenInGame
	VER_UE4_VISIBILITY_FLAG_CHANGES,
	// change Light/Fog/Blur bEnable to use bVisible
	VER_UE4_REMOVE_COMPONENT_ENABLED_FLAG,
	// change Particle/Audio/Thrust/RadialForce bEnable/bAutoPlay to use bAutoActivate
	VER_UE4_CONFORM_COMPONENT_ACTIVATE_FLAG,
	// make the 'mesh to import vertex map' in skelmesh always loaded so it can be used by vertex anim
	VER_UE4_ADD_SKELMESH_MESHTOIMPORTVERTEXMAP,
	// remove serialization for properties added with UE3 version 864 serialization
	VER_UE4_REMOVE_UE3_864_SERIALIZATION,
	// Spherical harmonic lightmaps
	VER_UE4_SH_LIGHTMAPS,
	// Removed per-shader DDC entries
	VER_UE4_REMOVED_PERSHADER_DDC,
	// Core split into Core and CoreUObject
	VER_UE4_CORE_SPLIT,
	// Removed some compile outputs being stored in FMaterial
	VER_UE4_REMOVED_FMATERIAL_COMPILE_OUTPUTS,
	// New physical material model
	VER_UE4_PHYSICAL_MATERIAL_MODEL,
	// Added a usage to FMaterialShaderMapId
	VER_UE4_ADDED_MATERIALSHADERMAP_USAGE,
	// Covert blueprint PropertyFlags from int32 to uint64
	VER_UE4_BLUEPRINT_PROPERTYFLAGS_SIZE_CHANGE,
	// Consolidate UpdateSkelWhenNotRendered and TickAnimationWhenNotRendered to enum
	VER_UE4_CONSOLIDATE_SKINNEDMESH_UPDATE_FLAGS,
	// Remove Internal Archetype
	VER_UE4_REMOVE_INTERNAL_ARCHETYPE,
	// Remove Internal Archetype
	VER_UE4_REMOVE_ARCHETYPE_INDEX_FROM_LINKER_TABLES,
	// Made change to UK2Node_Variable so that VariableSourceClass is NULL if bSelfContext is TRUE
	VER_UE4_VARK2NODE_NULL_VARSRCCLASS_ON_SELF,
	// Removed SpecularBoost
	VER_UE4_REMOVED_SPECULAR_BOOST,
	// Add CPF_BlueprintVisible flag
	VER_UE4_ADD_KISMETVISIBLE,
	// UDistribution* objects moved to PostInitProperties.
	VER_UE4_MOVE_DISTRIBUITONS_TO_POSTINITPROPS,
	// Add optimized shadow-only index buffers to static meshes.
	VER_UE4_SHADOW_ONLY_INDEX_BUFFERS,
	// Changed indirect lighting volume sample format
	VER_UE4_CHANGED_VOLUME_SAMPLE_FORMAT,
	/** Change bool bEnableCollision in BodyInstance to enum CollisionEnabled */
	VER_UE4_CHANGE_BENABLECOLLISION_TO_COLLISIONENABLED,
	// Changed irrelevant light guids
	VER_UE4_CHANGED_IRRELEVANT_LIGHT_GUIDS,
	/** Rename bDisableAllRigidBody to bCreatePhysicsState */
	VER_UE4_RENAME_DISABLEALLRIGIDBODIES,
	// Unified SoundNodeAttenuation settings with other attenuation settings
	VER_UE4_SOUND_NODE_ATTENUATION_SETTINGS_CHANGE,
	// Add a NodeGuid to EdGraphNode, upping version to generate for existing nodes 
	VER_UE4_ADD_EDGRAPHNODE_GUID,
	// Fix the outer of InterpData objects
	VER_UE4_FIX_INTERPDATA_OUTERS,
	// Natively serialize blueprint core classes
	VER_UE4_BLUEPRINT_NATIVE_SERIALIZATION,
	// Inherit SoundNode from EdGraphNOde
	VER_UE4_SOUND_NODE_INHERIT_FROM_ED_GRAPH_NODE,
	// Unify ambient sound actor classes in to single ambient actor class
	VER_UE4_UNIFY_AMBIENT_SOUND_ACTORS,
	// Lightmap compression
	VER_UE4_LIGHTMAP_COMPRESSION,
	// MorphTarget data type integration to curve
	VER_UE4_MORPHTARGET_CURVE_INTEGRATION,
	// Fix LevelScriptBlueprints being standalone
	VER_UE4_CLEAR_STANDALONE_FROM_LEVEL_SCRIPT_BLUEPRINTS,
	// Natively serialize blueprint core classes
	VER_UE4_NO_INTERFACE_PROPERTY,
	// Category field moved to metadata.
	VER_UE4_CATEGORY_MOVED_TO_METADATA,
	// We removed the ctor link flag, this just clears this flag on load for future use
	VER_UE4_REMOVE_CTOR_LINK,
	// Short to long package name associations removal.
	VER_UE4_REMOVE_SHORT_PACKAGE_NAME_ASSOCIATIONS,
	// Add bCreatedByConstructionScript flag to ActorComponent
	VER_UE4_ADD_CREATEDBYCONSTRUCTIONSCRIPT,
	// Fix loading of bogus NxDestructibleAssetAuthoring
	VER_UE4_NX_DESTRUCTIBLE_ASSET_AUTHORING_LOAD_FIX,
	// Added angular constraint options
	VER_UE4_ANGULAR_CONSTRAINT_OPTIONS,
	/** Changed material expression constants 3 and 4 to use a FLinearColor rather than separate floats to make it more artist friendly */
	VER_UE4_CHANGE_MATERIAL_EXPRESSION_CONSTANTS_TO_LINEARCOLOR,
	// Added built lighting flag to primitive component
	VER_UE4_PRIMITIVE_BUILT_LIGHTING_FLAG,
	// Added Counter for atmospheric fog
	VER_UE4_ATMOSPHERIC_FOG_CACHE_TEXTURE,
	// Ressurrected precomputed shadowmaps
	VER_UE4_PRECOMPUTED_SHADOW_MAPS,
	// Eliminated use of distribution for USoundNodeModulatorContinuous
	VER_UE4_MODULATOR_CONTINUOUS_NO_DISTRIBUTION,
	// Added a 4-byte magic number at the end of the package for file corruption validation
	VER_UE4_PACKAGE_MAGIC_POSTTAG,
	// Discard invalid irrelevant lights
	VER_UE4_TOSS_IRRELEVANT_LIGHTS,
	// Removed NetIndex
	VER_UE4_REMOVE_NET_INDEX,
	// Moved blueprint authoritative data from Skeleton CDO to the Generated CDO
	VER_UE4_BLUEPRINT_CDO_MIGRATION,
	// Bulkdata is stored at the end of package files and can be located at offsets > 2GB
	VER_UE4_BULKDATA_AT_LARGE_OFFSETS,
	// Explicitly track whether streaming texture data has been built
	VER_UE4_EXPLICIT_STREAMING_TEXTURE_BUILT,
	// Precomputed shadowmaps on bsp and landscape
	VER_UE4_PRECOMPUTED_SHADOW_MAPS_BSP,
	// Refactor of static mesh build pipeline.
	VER_UE4_STATIC_MESH_REFACTOR,
	// Remove cached static mesh streaming texture factors. They have been moved to derived data.
	VER_UE4_REMOVE_CACHED_STATIC_MESH_STREAMING_FACTORS,
	// Added Atmospheric fog Material support
	VER_UE4_ATMOSPHERIC_FOG_MATERIAL,
	// Fixup BSP brush type
	VER_UE4_FIX_BSP_BRUSH_TYPE,
	// Removed ClientDestroyedActorContent from UWorld
	VER_UE4_REMOVE_CLIENTDESTROYEDACTORCONTENT,
	// Added SoundCueGraph for new SoundCue editor
	VER_UE4_SOUND_CUE_GRAPH_EDITOR,
	// Strip TransLevelMoveBuffers out of Worlds
	VER_UE4_STRIP_TRANS_LEVEL_MOVE_BUFFER,
	// Deprecated PrimitiveComponent.bNoEncroachCheck
	VER_UE4_DEPRECATED_BNOENCROACHCHECK,
	// Light component bUseIESBrightness now defaults to false
	VER_UE4_LIGHTS_USE_IES_BRIGHTNESS_DEFAULT_CHANGED,
	// Material attributes multiplex
	VER_UE4_MATERIAL_ATTRIBUTES_MULTIPLEX,
	// Renamed & moved TSF_RGBA8/E8 to TSF_BGRA8/E8
	VER_UE4_TEXTURE_FORMAT_RGBA_SWIZZLE,
	// Package summary stores the offset to the beginning of the area where the bulkdata gets stored */
	VER_UE4_SUMMARY_HAS_BULKDATA_OFFSET,
	// The SimpleConstructionScript now marks the default root component as transactional, and bCreatedByConstructionScript true
	VER_UE4_DEFAULT_ROOT_COMP_TRANSACTIONAL,
	// Hashed material compile output stored in packages to detect mismatches
	VER_UE4_HASHED_MATERIAL_OUTPUT,
	// Removed restriction on blueprint-exposed variables from being read-only
	VER_UE4_BLUEPRINT_VARS_NOT_READ_ONLY,
	// Added manually serialized element to UStaticMesh (precalculated nav collision)
	VER_UE4_STATIC_MESH_STORE_NAV_COLLISION,
	// Changed property name for atmospheric fog
	VER_UE4_ATMOSPHERIC_FOG_DECAY_NAME_CHANGE,
	// Change many properties/functions from Translation to Location
	VER_UE4_SCENECOMP_TRANSLATION_TO_LOCATION,
	// Material attributes reordering
	VER_UE4_MATERIAL_ATTRIBUTES_REORDERING,
	// Collision Profile setting has been added, and all components that exists has to be properly upgraded
	VER_UE4_COLLISION_PROFILE_SETTING,
	// Making the blueprint's skeleton class transient
	VER_UE4_BLUEPRINT_SKEL_TEMPORARY_TRANSIENT,
	// Making the blueprint's skeleton class serialized again
	VER_UE4_BLUEPRINT_SKEL_SERIALIZED_AGAIN,
	// Blueprint now controls replication settings again
	VER_UE4_BLUEPRINT_SETS_REPLICATION,
	// Added level info used by World browser
	VER_UE4_WORLD_LEVEL_INFO,
	// Changed capsule height to capsule half-height (afterwards)
	VER_UE4_AFTER_CAPSULE_HALF_HEIGHT_CHANGE,
	// Added Namepace, GUID (Key) and Flags to FText
	VER_UE4_ADDED_NAMESPACE_AND_KEY_DATA_TO_FTEXT,
	// Attenuation shapes
	VER_UE4_ATTENUATION_SHAPES,
	// Use IES texture multiplier even when IES brightness is not being used
	VER_UE4_LIGHTCOMPONENT_USE_IES_TEXTURE_MULTIPLIER_ON_NON_IES_BRIGHTNESS,
	// Removed InputComponent as a blueprint addable component
	VER_UE4_REMOVE_INPUT_COMPONENTS_FROM_BLUEPRINTS,
	// Use an FMemberReference struct in UK2Node_Variable
	VER_UE4_VARK2NODE_USE_MEMBERREFSTRUCT,
	// Refactored material expression inputs for UMaterialExpressionSceneColor and UMaterialExpressionSceneDepth
	VER_UE4_REFACTOR_MATERIAL_EXPRESSION_SCENECOLOR_AND_SCENEDEPTH_INPUTS,
	// Spline meshes changed from Z forwards to configurable
	VER_UE4_SPLINE_MESH_ORIENTATION,
	// Added ReverbEffect asset type
	VER_UE4_REVERB_EFFECT_ASSET_TYPE,
	// changed max texcoords from 4 to 8
	VER_UE4_MAX_TEXCOORD_INCREASED,
	// static meshes changed to support SpeedTrees
	VER_UE4_SPEEDTREE_STATICMESH,
	// Landscape component reference between landscape component and collision component
	VER_UE4_LANDSCAPE_COMPONENT_LAZY_REFERENCES,
	// Refactored UK2Node_CallFunction to use FMemberReference
	VER_UE4_SWITCH_CALL_NODE_TO_USE_MEMBER_REFERENCE,
	// Added fixup step to remove skeleton class references from blueprint objects
	VER_UE4_ADDED_SKELETON_ARCHIVER_REMOVAL,
	// See above, take 2.
	VER_UE4_ADDED_SKELETON_ARCHIVER_REMOVAL_SECOND_TIME,
	// Making the skeleton class on blueprints transient
	VER_UE4_BLUEPRINT_SKEL_CLASS_TRANSIENT_AGAIN,
	// UClass knows if it's been cooked
	VER_UE4_ADD_COOKED_TO_UCLASS,
	// Deprecated static mesh thumbnail properties were removed
	VER_UE4_DEPRECATED_STATIC_MESH_THUMBNAIL_PROPERTIES_REMOVED,
	// Added collections in material shader map ids
	VER_UE4_COLLECTIONS_IN_SHADERMAPID,
	// Renamed some Movement Component properties, added PawnMovementComponent
	VER_UE4_REFACTOR_MOVEMENT_COMPONENT_HIERARCHY,
	// Swap UMaterialExpressionTerrainLayerSwitch::LayerUsed/LayerNotUsed the correct way round
	VER_UE4_FIX_TERRAIN_LAYER_SWITCH_ORDER,
	// Remove URB_ConstraintSetup
	VER_UE4_ALL_PROPS_TO_CONSTRAINTINSTANCE,
	// Low quality directional lightmaps
	VER_UE4_LOW_QUALITY_DIRECTIONAL_LIGHTMAPS,
	// Added NoiseEmitterComponent and removed related Pawn properties.
	VER_UE4_ADDED_NOISE_EMITTER_COMPONENT,
	// Add text component vertical alignment
	VER_UE4_ADD_TEXT_COMPONENT_VERTICAL_ALIGNMENT,
	// Added AssetImportData for FBX asset types, deprecating SourceFilePath and SourceFileTimestamp
	VER_UE4_ADDED_FBX_ASSET_IMPORT_DATA,
	// Remove LevelBodySetup from ULevel
	VER_UE4_REMOVE_LEVELBODYSETUP,
	// Refactor character crouching
	VER_UE4_REFACTOR_CHARACTER_CROUCH,	
	// Trimmed down material shader debug information.
	VER_UE4_SMALLER_DEBUG_MATERIALSHADER_UNIFORM_EXPRESSIONS,
	// APEX Clothing
	VER_UE4_APEX_CLOTH,
	// Change Collision Channel to save only modified ones than all of them
	// @note!!! once we pass this CL, we can rename FCollisionResponseContainer enum values
	// we should rename to match ECollisionChannel
	VER_UE4_SAVE_COLLISIONRESPONSE_PER_CHANNEL,
	// Added Landscape Spline editor meshes
	VER_UE4_ADDED_LANDSCAPE_SPLINE_EDITOR_MESH,
	// Fixup input expressions for reading from refraction material attributes.
	VER_UE4_CHANGED_MATERIAL_REFACTION_TYPE,
	// Refactor projectile movement, along with some other movement component work.
	VER_UE4_REFACTOR_PROJECTILE_MOVEMENT,
	// Remove PhysicalMaterialProperty and replace with user defined enum
	VER_UE4_REMOVE_PHYSICALMATERIALPROPERTY,
	// Removed all compile outputs from FMaterial
	VER_UE4_PURGED_FMATERIAL_COMPILE_OUTPUTS,
	// Ability to save cooked PhysX meshes to Landscape
	VER_UE4_ADD_COOKED_TO_LANDSCAPE,
	// Change how input component consumption works
	VER_UE4_CONSUME_INPUT_PER_BIND,
	// Added new Graph based SoundClass Editor
	VER_UE4_SOUND_CLASS_GRAPH_EDITOR,
	// Fixed terrain layer node guids which was causing artifacts
	VER_UE4_FIXUP_TERRAIN_LAYER_NODES,
	// Added clamp min/max swap check to catch older materials
	VER_UE4_RETROFIT_CLAMP_EXPRESSIONS_SWAP,
	// Remove static/movable/stationary light classes
	VER_UE4_REMOVE_LIGHT_MOBILITY_CLASSES,
	// Refactor the way physics blending works to allow partial blending
	VER_UE4_REFACTOR_PHYSICS_BLENDING,
	// WorldLevelInfo: Added reference to parent level and streaming distance
	VER_UE4_WORLD_LEVEL_INFO_UPDATED,
	// Fixed cooking of skeletal/static meshes due to bad serialization logic
	VER_UE4_STATIC_SKELETAL_MESH_SERIALIZATION_FIX,
	// Removal of InterpActor and PhysicsActor
	VER_UE4_REMOVE_STATICMESH_MOBILITY_CLASSES,
	// Refactor physics transforms
	VER_UE4_REFACTOR_PHYSICS_TRANSFORMS,
	// Remove zero triangle sections from static meshes and compact material indices.
	VER_UE4_REMOVE_ZERO_TRIANGLE_SECTIONS,
	// Add param for deceleration in character movement instead of using acceleration.
	VER_UE4_CHARACTER_MOVEMENT_DECELERATION,
	// Made ACameraActor use a UCameraComponent for parameter storage, etc...
	VER_UE4_CAMERA_ACTOR_USING_CAMERA_COMPONENT,
	// Deprecated some pitch/roll properties in CharacterMovementComponent
	VER_UE4_CHARACTER_MOVEMENT_DEPRECATE_PITCH_ROLL,
	// Rebuild texture streaming data on load for uncooked builds
	VER_UE4_REBUILD_TEXTURE_STREAMING_DATA_ON_LOAD,
	// Add support for 32 bit index buffers for static meshes.
	VER_UE4_SUPPORT_32BIT_STATIC_MESH_INDICES,
	// Added streaming install ChunkID to AssetData and UPackage
	VER_UE4_ADDED_CHUNKID_TO_ASSETDATA_AND_UPACKAGE,
	// Add flag to control whether Character blueprints receive default movement bindings.
	VER_UE4_CHARACTER_DEFAULT_MOVEMENT_BINDINGS,
	// APEX Clothing LOD Info
	VER_UE4_APEX_CLOTH_LOD,
	// Added atmospheric fog texture data to be general
	VER_UE4_ATMOSPHERIC_FOG_CACHE_DATA,
	// Arrays serialize their inner's tags
	VAR_UE4_ARRAY_PROPERTY_INNER_TAGS,
	// Skeletal mesh index data is kept in memory in game to support mesh merging.
	VER_UE4_KEEP_SKEL_MESH_INDEX_DATA,
	// Added compatibility for the body instance collision change
	VER_UE4_BODYSETUP_COLLISION_CONVERSION,
	// Reflection capture cooking
	VER_UE4_REFLECTION_CAPTURE_COOKING,
	// Removal of DynamicTriggerVolume, DynamicBlockingVolume, DynamicPhysicsVolume
	VER_UE4_REMOVE_DYNAMIC_VOLUME_CLASSES,
	// Store an additional flag in the BodySetup to indicate whether there is any cooked data to load
	VER_UE4_STORE_HASCOOKEDDATA_FOR_BODYSETUP,
	// Changed name of RefractionBias to RefractionDepthBias.
	VER_UE4_REFRACTION_BIAS_TO_REFRACTION_DEPTH_BIAS,
	// Removal of SkeletalPhysicsActor
	VER_UE4_REMOVE_SKELETALPHYSICSACTOR,
	// PlayerController rotation input refactor
	VER_UE4_PC_ROTATION_INPUT_REFACTOR,
	// Landscape Platform Data cooking
	VER_UE4_LANDSCAPE_PLATFORMDATA_COOKING,
	// Added call for linking classes in CreateExport to ensure memory is initialized properly
	VER_UE4_CREATEEXPORTS_CLASS_LINKING_FOR_BLUEPRINTS,
	// Remove native component nodes from the blueprint SimpleConstructionScript
	VER_UE4_REMOVE_NATIVE_COMPONENTS_FROM_BLUEPRINT_SCS,
	// Removal of Single Node Instance
	VER_UE4_REMOVE_SINGLENODEINSTANCE, 
	// Character movement braking changes
	VER_UE4_CHARACTER_BRAKING_REFACTOR,
	// Supported low quality lightmaps in volume samples
	VER_UE4_VOLUME_SAMPLE_LOW_QUALITY_SUPPORT,
	// Split bEnableTouchEvents out from bEnableClickEvents
	VER_UE4_SPLIT_TOUCH_AND_CLICK_ENABLES,
	// Health/Death refactor
	VER_UE4_HEALTH_DEATH_REFACTOR,
	// Moving USoundNodeEnveloper from UDistributionFloatConstantCurve to FRichCurve
	VER_UE4_SOUND_NODE_ENVELOPER_CURVE_CHANGE,
	// Moved SourceRadius to UPointLightComponent
	VER_UE4_POINT_LIGHT_SOURCE_RADIUS,
	// Scene capture actors based on camera actors.
	VER_UE4_SCENE_CAPTURE_CAMERA_CHANGE,
	// Moving SkeletalMesh shadow casting flag from LoD details to material
	VER_UE4_MOVE_SKELETALMESH_SHADOWCASTING,
	// Changing bytecode operators for creating arrays
	VER_UE4_CHANGE_SETARRAY_BYTECODE,
	// Material Instances overriding base material properties.
	VER_UE4_MATERIAL_INSTANCE_BASE_PROPERTY_OVERRIDES,
	// Combined top/bottom lightmap textures
	VER_UE4_COMBINED_LIGHTMAP_TEXTURES,
	// Forced material lightmass guids to be regenerated
	VER_UE4_BUMPED_MATERIAL_EXPORT_GUIDS,
	// Allow overriding of parent class input bindings
	VER_UE4_BLUEPRINT_INPUT_BINDING_OVERRIDES,
	// Fix up convex invalid transform
	VER_UE4_FIXUP_BODYSETUP_INVALID_CONVEX_TRANSFORM, 
	// Fix up scale of physics stiffness and damping value
	VER_UE4_FIXUP_STIFFNESS_AND_DAMPING_SCALE, 
	// Convert USkeleton and FBoneContrainer to using FReferenceSkeleton.
	VER_UE4_REFERENCE_SKELETON_REFACTOR,
	// Adding references to variable, function, and macro nodes to be able to update to renamed values
	VER_UE4_K2NODE_REFERENCEGUIDS,
	// Fix up the 0th bone's parent bone index.
	VER_UE4_FIXUP_ROOTBONE_PARENT,
	//Allow setting of TextRenderComponents size in world space.
	VER_UE4_TEXT_RENDER_COMPONENTS_WORLD_SPACE_SIZING,
	// Material Instances overriding base material properties #2.
	VER_UE4_MATERIAL_INSTANCE_BASE_PROPERTY_OVERRIDES_PHASE_2,
	// CLASS_Placeable becomes CLASS_NotPlaceable
	VER_UE4_CLASS_NOTPLACEABLE_ADDED,
	// Added LOD info list to a world tile description
	VER_UE4_WORLD_LEVEL_INFO_LOD_LIST,
	// CharacterMovement variable naming refactor
	VER_UE4_CHARACTER_MOVEMENT_VARIABLE_RENAMING_1,
	// FName properties containing sound names converted to FSlateSound properties
	VER_UE4_FSLATESOUND_CONVERSION,
	// Added ZOrder to a world tile description
	VER_UE4_WORLD_LEVEL_INFO_ZORDER,
	// Added flagging of localization gather requirement to packages
	VER_UE4_PACKAGE_REQUIRES_LOCALIZATION_GATHER_FLAGGING,
	// Preventing Blueprint Actor variables from having default values
	VER_UE4_BP_ACTOR_VARIABLE_DEFAULT_PREVENTING,
	// Preventing Blueprint Actor variables from having default values
	VER_UE4_TEST_ANIMCOMP_CHANGE,
	// Class as primary asset, name convention changed
	VER_UE4_EDITORONLY_BLUEPRINTS,
	// Custom serialization for FEdGraphPinType
	VER_UE4_EDGRAPHPINTYPE_SERIALIZATION,
	// Stop generating 'mirrored' cooked mesh for Brush and Model components
	VER_UE4_NO_MIRROR_BRUSH_MODEL_COLLISION,
	// Changed ChunkID to be an array of IDs.
	VER_UE4_CHANGED_CHUNKID_TO_BE_AN_ARRAY_OF_CHUNKIDS,
	// Worlds have been renamed from "TheWorld" to be named after the package containing them
	VER_UE4_WORLD_NAMED_AFTER_PACKAGE,
	// Added sky light component
	VER_UE4_SKY_LIGHT_COMPONENT,
	// Added Enable distance streaming flag to FWorldTileLayer
	VER_UE4_WORLD_LAYER_ENABLE_DISTANCE_STREAMING,
	// Remove visibility/zone information from UModel
	VER_UE4_REMOVE_ZONES_FROM_MODEL,
	// Fix base pose serialization 
	VER_UE4_FIX_ANIMATIONBASEPOSE_SERIALIZATION,
	// Support for up to 8 skinning influences per vertex on skeletal meshes (on non-gpu vertices)
	VER_UE4_SUPPORT_8_BONE_INFLUENCES_SKELETAL_MESHES,
	// Add explicit bOverrideGravity to world settings
	VER_UE4_ADD_OVERRIDE_GRAVITY_FLAG,
	// Support for up to 8 skinning influences per vertex on skeletal meshes (on gpu vertices)
	VER_UE4_SUPPORT_GPUSKINNING_8_BONE_INFLUENCES,
	// Supporting nonuniform scale animation
	VER_UE4_ANIM_SUPPORT_NONUNIFORM_SCALE_ANIMATION,
	// Engine version is stored as a FEngineVersion object rather than changelist number
	VER_UE4_ENGINE_VERSION_OBJECT,
	// World assets now have RF_Public
	VER_UE4_PUBLIC_WORLDS,
	// Skeleton Guid
	VER_UE4_SKELETON_GUID_SERIALIZATION,
	// Character movement WalkableFloor refactor
	VER_UE4_CHARACTER_MOVEMENT_WALKABLE_FLOOR_REFACTOR,
	// Lights default to inverse squared
	VER_UE4_INVERSE_SQUARED_LIGHTS_DEFAULT,
	// Disabled SCRIPT_LIMIT_BYTECODE_TO_64KB
	VER_UE4_DISABLED_SCRIPT_LIMIT_BYTECODE,
	// Made remote role private, exposed bReplicates
	VER_UE4_PRIVATE_REMOTE_ROLE,

	// -----<new versions can be added before this line>-------------------------------------------------
	// - this needs to be the last line (see note below)
	VER_UE4_AUTOMATIC_VERSION_PLUS_ONE,
	VER_UE4_AUTOMATIC_VERSION = VER_UE4_AUTOMATIC_VERSION_PLUS_ONE - 1
};

#include "PendingVersions.h"

enum EUnrealEngineObjectLicenseeUE4Version
{
	VER_LIC_NONE = 0,
	// - this needs to be the last line (see note below)
	VER_LIC_AUTOMATIC_VERSION_PLUS_ONE,
	VER_LIC_AUTOMATIC_VERSION = VER_LIC_AUTOMATIC_VERSION_PLUS_ONE - 1
};

#define VER_LATEST_ENGINE_UE4           PREPROCESSOR_ENUM_PROTECT(VER_UE4_AUTOMATIC_VERSION)
#define VER_UE4_OLDEST_LOADABLE_PACKAGE PREPROCESSOR_ENUM_PROTECT(VER_UE4_ADD_PINTYPE_ARRAY)
#define VER_UE4_DEPRECATED_PACKAGE      PREPROCESSOR_ENUM_PROTECT(VER_UE4_ADD_PINTYPE_ARRAY)
#define VER_LATEST_ENGINE_LICENSEEUE4   PREPROCESSOR_ENUM_PROTECT(VER_LIC_AUTOMATIC_VERSION)

// Minimum package version that contains legal bytecode
#define VER_MIN_SCRIPTVM_UE4									(VER_UE4_DISABLED_SCRIPT_LIMIT_BYTECODE)
#define VER_MIN_SCRIPTVM_LICENSEEUE4							(VER_LIC_NONE)

// Version access.

extern CORE_API int32			GEngineNetVersion;				// Version used for networking; the P4 changelist number.
extern CORE_API int32			GEngineMinNetVersion;			// Earliest engine build that is network compatible with this one.
extern CORE_API int32			GEngineNegotiationVersion;		// Base protocol version to negotiate in network play.

extern CORE_API int32			GPackageFileUE4Version;			// UE4 Version Number.
extern CORE_API int32			GPackageFileLicenseeUE4Version;	// Licensee Version Number.
