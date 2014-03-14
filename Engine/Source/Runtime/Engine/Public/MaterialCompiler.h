// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MaterialCompiler.h: Material compiler interface.
=============================================================================*/

#pragma once

#include "Engine.h"
#include "MaterialShared.h"

/** 
 * The interface used to translate material expressions into executable code. 
 * Note: Most member functions should be pure virtual to force a FProxyMaterialCompiler override!
 */
class FMaterialCompiler
{
public:

	virtual void SetMaterialProperty(EMaterialProperty InProperty, EShaderFrequency InShaderFrequency) = 0;
	virtual int32 Error(const TCHAR* Text) = 0;
	int32 Errorf(const TCHAR* Format,...);

	virtual int32 CallExpression(FMaterialExpressionKey ExpressionKey,FMaterialCompiler* InCompiler) = 0;

	virtual EMaterialValueType GetType(int32 Code) = 0;

	virtual EMaterialQualityLevel::Type GetQualityLevel() = 0;

	virtual ERHIFeatureLevel::Type GetFeatureLevel() = 0;

	virtual float GetRefractionDepthBiasValue() = 0;

	/** 
	 * Casts the passed in code to DestType, or generates a compile error if the cast is not valid. 
	 * This will truncate a type (float4 -> float3) but not add components (float2 -> float3), however a float1 can be cast to any float type by replication. 
	 */
	virtual int32 ValidCast(int32 Code,EMaterialValueType DestType) = 0;
	virtual int32 ForceCast(int32 Code,EMaterialValueType DestType,bool bExactMatch=false,bool bReplicateValue=false) = 0;

	/** Pushes a function onto the compiler's function stack, which indicates that compilation is entering a function. */
	virtual void PushFunction(const FMaterialFunctionCompileState& FunctionState) = 0;

	/** Pops a function from the compiler's function stack, which indicates that compilation is leaving a function. */
	virtual FMaterialFunctionCompileState PopFunction() = 0;

	virtual int32 AccessCollectionParameter(UMaterialParameterCollection* ParameterCollection, int32 ParameterIndex, int32 ComponentIndex) = 0;
	virtual int32 VectorParameter(FName ParameterName,const FLinearColor& DefaultValue) = 0;
	virtual int32 ScalarParameter(FName ParameterName,float DefaultValue) = 0;

	virtual int32 Constant(float X) = 0;
	virtual int32 Constant2(float X,float Y) = 0;
	virtual int32 Constant3(float X,float Y,float Z) = 0;
	virtual int32 Constant4(float X,float Y,float Z,float W) = 0;

	virtual int32 GameTime() = 0;
	virtual int32 RealTime() = 0;
	virtual int32 PeriodicHint(int32 PeriodicCode) { return PeriodicCode; }

	virtual int32 Sine(int32 X) = 0;
	virtual int32 Cosine(int32 X) = 0;

	virtual int32 Floor(int32 X) = 0;
	virtual int32 Ceil(int32 X) = 0;
	virtual int32 Frac(int32 X) = 0;
	virtual int32 Fmod(int32 A, int32 B) = 0;
	virtual int32 Abs(int32 X) = 0;

	virtual int32 ReflectionVector() = 0;
	virtual int32 ReflectionAboutCustomWorldNormal(int32 CustomWorldNormal, int32 bNormalizeCustomWorldNormal) = 0;
	virtual int32 CameraVector() = 0;
	virtual int32 CameraWorldPosition() = 0;
	virtual int32 LightVector() = 0;

	virtual int32 ScreenPosition() = 0;
	virtual int32 ViewSize() = 0;
	virtual int32 SceneTexelSize() = 0;
	virtual int32 WorldPosition(EWorldPositionIncludedOffsets WorldPositionIncludedOffsets) = 0;
	virtual int32 ObjectWorldPosition() = 0;
	virtual int32 ObjectRadius() = 0;
	virtual int32 ObjectBounds() = 0;	
	virtual int32 DistanceCullFade() = 0;
	virtual int32 ActorWorldPosition() = 0;
	virtual int32 ParticleMacroUV() = 0;
	virtual int32 ParticleSubUV(int32 TextureIndex, EMaterialSamplerType SamplerType, bool bBlend) = 0;
	virtual int32 ParticleColor() = 0;
	virtual int32 ParticlePosition() = 0;
	virtual int32 ParticleRadius() = 0;
	virtual int32 SphericalParticleOpacity(int32 Density) = 0;
	virtual int32 ParticleRelativeTime() = 0;
	virtual int32 ParticleMotionBlurFade() = 0;
	virtual int32 ParticleDirection() = 0;
	virtual int32 ParticleSpeed() = 0;
	virtual int32 ParticleSize() = 0;

	virtual int32 If(int32 A,int32 B,int32 AGreaterThanB,int32 AEqualsB,int32 ALessThanB,int32 Threshold) = 0;

	virtual int32 TextureCoordinate(uint32 CoordinateIndex, bool UnMirrorU, bool UnMirrorV) = 0;
	virtual int32 TextureSample(int32 Texture,int32 Coordinate,enum EMaterialSamplerType SamplerType,int32 MipValueIndex=INDEX_NONE,ETextureMipValueMode MipValueMode=TMVM_None) = 0;

	virtual int32 Texture(UTexture* Texture) = 0;
	virtual int32 TextureParameter(FName ParameterName,UTexture* DefaultTexture) = 0;

	virtual	int32 PixelDepth()=0;
	virtual int32 SceneDepth(int32 Offset, int32 UV, bool bUseOffset) = 0;
	virtual int32 SceneColor(int32 Offset, int32 UV, bool bUseOffset) = 0;
	// @param SceneTextureId of type ESceneTextureId e.g. PPI_SubsurfaceColor
	virtual int32 SceneTextureLookup(int32 UV, uint32 SceneTextureId) = 0;
	// @param SceneTextureId of type ESceneTextureId e.g. PPI_SubsurfaceColor
	virtual int32 SceneTextureSize(uint32 SceneTextureId, bool bInvert) = 0;
	// @param SceneTextureId of type ESceneTextureId e.g. PPI_SubsurfaceColor
	virtual int32 SceneTextureMax(uint32 InSceneTextureId) = 0;
	// @param SceneTextureId of type ESceneTextureId e.g. PPI_SubsurfaceColor
	virtual int32 SceneTextureMin(uint32 InSceneTextureId) = 0;

	virtual int32 StaticBool(bool Value) = 0;
	virtual int32 StaticBoolParameter(FName ParameterName,bool bDefaultValue) = 0;
	virtual int32 StaticComponentMask(int32 Vector,FName ParameterName,bool bDefaultR,bool bDefaultG,bool bDefaultB,bool bDefaultA) = 0;
	virtual bool GetStaticBoolValue(int32 BoolIndex, bool& bSucceeded) = 0;
	virtual int32 StaticTerrainLayerWeight(FName ParameterName,int32 Default) = 0;

	virtual int32 VertexColor() = 0;

	virtual int32 Add(int32 A,int32 B) = 0;
	virtual int32 Sub(int32 A,int32 B) = 0;
	virtual int32 Mul(int32 A,int32 B) = 0;
	virtual int32 Div(int32 A,int32 B) = 0;
	virtual int32 Dot(int32 A,int32 B) = 0;
	virtual int32 Cross(int32 A,int32 B) = 0;

	virtual int32 Power(int32 Base,int32 Exponent) = 0;
	virtual int32 SquareRoot(int32 X) = 0;
	virtual int32 Length(int32 X) = 0;

	virtual int32 Lerp(int32 X,int32 Y,int32 A) = 0;
	virtual int32 Min(int32 A,int32 B) = 0;
	virtual int32 Max(int32 A,int32 B) = 0;
	virtual int32 Clamp(int32 X,int32 A,int32 B) = 0;

	virtual int32 ComponentMask(int32 Vector,bool R,bool G,bool B,bool A) = 0;
	virtual int32 AppendVector(int32 A,int32 B) = 0;
	virtual int32 TransformVector(uint8 SourceCoordType,uint8 DestCoordType,int32 A) = 0;
	virtual int32 TransformPosition(uint8 SourceCoordType,uint8 DestCoordType,int32 A) = 0;

	virtual int32 DynamicParameter() = 0;
	virtual int32 LightmapUVs() = 0;

	virtual int32 LightmassReplace(int32 Realtime, int32 Lightmass) = 0;
	virtual int32 GIReplace(int32 Direct, int32 StaticIndirect, int32 DynamicIndirect) = 0;

	virtual int32 ObjectOrientation() = 0;
	virtual int32 RotateAboutAxis(int32 NormalizedRotationAxisAndAngleIndex, int32 PositionOnAxisIndex, int32 PositionIndex) = 0;
	virtual int32 TwoSidedSign() = 0;
	virtual int32 VertexNormal() = 0;
	virtual int32 PixelNormalWS() = 0;

	virtual int32 CustomExpression( class UMaterialExpressionCustom* Custom, TArray<int32>& CompiledInputs ) = 0;

	virtual int32 DDX(int32 X) = 0;
	virtual int32 DDY(int32 X) = 0;

	virtual int32 PerInstanceRandom() = 0;
	virtual int32 PerInstanceFadeAmount() = 0;
	virtual int32 AntialiasedTextureMask(int32 Tex, int32 UV, float Threshold, uint8 Channel) = 0;
	virtual int32 Noise(int32 Position, float Scale, int32 Quality, uint8 NoiseFunction, bool bTurbulence, int32 Levels, float OutputMin, float OutputMax, float LevelScale, int32 FilterWidth) = 0;
	virtual int32 BlackBody( int32 Temp ) = 0;
	virtual int32 DepthOfFieldFunction(int32 Depth, int32 FunctionValueIndex) = 0;
	virtual int32 AtmosphericFogColor(int32 WorldPosition) = 0;
	virtual int32 SpeedTree(ESpeedTreeGeometryType GeometryType, ESpeedTreeWindType WindType, ESpeedTreeLODType LODType, float BillboardThreshold) = 0;
	virtual int32 TextureCoordinateOffset() = 0;
	virtual int32 EyeAdaptation() = 0;
};

/** 
 * A proxy for the material compiler interface which by default passes all function calls unmodified. 
 * Note: Any functions of FMaterialCompiler that change the internal compiler state must be routed!
 */
class FProxyMaterialCompiler : public FMaterialCompiler
{
public:

	// Constructor.
	FProxyMaterialCompiler(FMaterialCompiler* InCompiler):
		Compiler(InCompiler)
	{}

	// Simple pass through all other material operations unmodified.

	virtual void SetMaterialProperty(EMaterialProperty InProperty, EShaderFrequency InShaderFrequency) OVERRIDE { Compiler->SetMaterialProperty(InProperty, InShaderFrequency); }
	virtual int32 Error(const TCHAR* Text) OVERRIDE { return Compiler->Error(Text); }

	virtual int32 CallExpression(FMaterialExpressionKey ExpressionKey,FMaterialCompiler* InCompiler) OVERRIDE { return Compiler->CallExpression(ExpressionKey,InCompiler); }

	virtual void PushFunction(const FMaterialFunctionCompileState& FunctionState) OVERRIDE { Compiler->PushFunction(FunctionState); }
	virtual FMaterialFunctionCompileState PopFunction() OVERRIDE { return Compiler->PopFunction(); }

	virtual EMaterialValueType GetType(int32 Code) OVERRIDE { return Compiler->GetType(Code); }
	virtual EMaterialQualityLevel::Type GetQualityLevel() OVERRIDE { return Compiler->GetQualityLevel(); }
	virtual ERHIFeatureLevel::Type GetFeatureLevel() OVERRIDE { return Compiler->GetFeatureLevel(); }
	virtual float GetRefractionDepthBiasValue() OVERRIDE { return Compiler->GetRefractionDepthBiasValue(); }
	virtual int32 ValidCast(int32 Code,EMaterialValueType DestType) OVERRIDE { return Compiler->ValidCast(Code, DestType); }
	virtual int32 ForceCast(int32 Code,EMaterialValueType DestType,bool bExactMatch=false,bool bReplicateValue=false) OVERRIDE
	{ return Compiler->ForceCast(Code,DestType,bExactMatch,bReplicateValue); }

	virtual int32 AccessCollectionParameter(UMaterialParameterCollection* ParameterCollection, int32 ParameterIndex, int32 ComponentIndex) OVERRIDE { return Compiler->AccessCollectionParameter(ParameterCollection, ParameterIndex, ComponentIndex); }
	virtual int32 VectorParameter(FName ParameterName,const FLinearColor& DefaultValue) OVERRIDE { return Compiler->VectorParameter(ParameterName,DefaultValue); }
	virtual int32 ScalarParameter(FName ParameterName,float DefaultValue) OVERRIDE { return Compiler->ScalarParameter(ParameterName,DefaultValue); }

	virtual int32 Constant(float X) OVERRIDE { return Compiler->Constant(X); }
	virtual int32 Constant2(float X,float Y) OVERRIDE { return Compiler->Constant2(X,Y); }
	virtual int32 Constant3(float X,float Y,float Z) OVERRIDE { return Compiler->Constant3(X,Y,Z); }
	virtual int32 Constant4(float X,float Y,float Z,float W) OVERRIDE { return Compiler->Constant4(X,Y,Z,W); }

	virtual int32 GameTime() OVERRIDE { return Compiler->GameTime(); }
	virtual int32 RealTime() OVERRIDE { return Compiler->RealTime(); }

	virtual int32 PeriodicHint(int32 PeriodicCode) OVERRIDE { return Compiler->PeriodicHint(PeriodicCode); }

	virtual int32 Sine(int32 X) OVERRIDE { return Compiler->Sine(X); }
	virtual int32 Cosine(int32 X) OVERRIDE { return Compiler->Cosine(X); }

	virtual int32 Floor(int32 X) OVERRIDE { return Compiler->Floor(X); }
	virtual int32 Ceil(int32 X) OVERRIDE { return Compiler->Ceil(X); }
	virtual int32 Frac(int32 X) OVERRIDE { return Compiler->Frac(X); }
	virtual int32 Fmod(int32 A, int32 B) OVERRIDE { return Compiler->Fmod(A,B); }
	virtual int32 Abs(int32 X) OVERRIDE { return Compiler->Abs(X); }

	virtual int32 ReflectionVector() OVERRIDE { return Compiler->ReflectionVector(); }
	virtual int32 CameraVector() OVERRIDE { return Compiler->CameraVector(); }
	virtual int32 CameraWorldPosition() OVERRIDE { return Compiler->CameraWorldPosition(); }
	virtual int32 LightVector() OVERRIDE { return Compiler->LightVector(); }

	virtual int32 ScreenPosition() OVERRIDE { return Compiler->ScreenPosition(); }
	virtual int32 ViewSize() OVERRIDE { return Compiler->ViewSize(); }
	virtual int32 SceneTexelSize() OVERRIDE { return Compiler->SceneTexelSize(); }
	virtual int32 WorldPosition(EWorldPositionIncludedOffsets WorldPositionIncludedOffsets) OVERRIDE { return Compiler->WorldPosition(WorldPositionIncludedOffsets); }
	virtual int32 ObjectWorldPosition() OVERRIDE { return Compiler->ObjectWorldPosition(); }
	virtual int32 ObjectRadius() OVERRIDE { return Compiler->ObjectRadius(); }
	virtual int32 ObjectBounds() OVERRIDE { return Compiler->ObjectBounds(); }
	virtual int32 DistanceCullFade() OVERRIDE { return Compiler->DistanceCullFade(); }
	virtual int32 ActorWorldPosition() OVERRIDE { return Compiler->ActorWorldPosition(); }
	virtual int32 ParticleMacroUV() OVERRIDE { return Compiler->ParticleMacroUV(); }
	virtual int32 ParticleSubUV(int32 TextureIndex, EMaterialSamplerType SamplerType, bool bBlend) OVERRIDE { return Compiler->ParticleSubUV(TextureIndex, SamplerType, bBlend); }
	virtual int32 ParticleColor() OVERRIDE { return Compiler->ParticleColor(); }
	virtual int32 ParticlePosition() OVERRIDE { return Compiler->ParticlePosition(); }
	virtual int32 ParticleRadius() OVERRIDE { return Compiler->ParticleRadius(); }
	virtual int32 SphericalParticleOpacity(int32 Density) OVERRIDE { return Compiler->SphericalParticleOpacity(Density); }

	virtual int32 If(int32 A,int32 B,int32 AGreaterThanB,int32 AEqualsB,int32 ALessThanB,int32 Threshold) OVERRIDE { return Compiler->If(A,B,AGreaterThanB,AEqualsB,ALessThanB,Threshold); }

	virtual int32 TextureSample(int32 InTexture,int32 Coordinate,EMaterialSamplerType SamplerType,int32 MipValueIndex=INDEX_NONE,ETextureMipValueMode MipValueMode=TMVM_None) OVERRIDE { return Compiler->TextureSample(InTexture,Coordinate,SamplerType,MipValueIndex,MipValueMode); }
	virtual int32 TextureCoordinate(uint32 CoordinateIndex, bool UnMirrorU, bool UnMirrorV) OVERRIDE { return Compiler->TextureCoordinate(CoordinateIndex, UnMirrorU, UnMirrorV); }

	virtual int32 Texture(UTexture* InTexture) OVERRIDE { return Compiler->Texture(InTexture); }
	virtual int32 TextureParameter(FName ParameterName,UTexture* DefaultValue) OVERRIDE { return Compiler->TextureParameter(ParameterName,DefaultValue); }

	virtual	int32 PixelDepth() OVERRIDE { return Compiler->PixelDepth();	}
	virtual int32 SceneDepth(int32 Offset, int32 UV, bool bUseOffset) OVERRIDE { return Compiler->SceneDepth(Offset, UV, bUseOffset); }
	virtual int32 SceneColor(int32 Offset, int32 UV, bool bUseOffset) OVERRIDE { return Compiler->SceneColor(Offset, UV, bUseOffset); }
	virtual int32 SceneTextureLookup(int32 UV, uint32 InSceneTextureId) OVERRIDE { return Compiler->SceneTextureLookup(UV, InSceneTextureId); }
	virtual int32 SceneTextureSize(uint32 InSceneTextureId, bool bInvert) OVERRIDE { return Compiler->SceneTextureSize(InSceneTextureId, bInvert); }
	virtual int32 SceneTextureMax(uint32 InSceneTextureId) OVERRIDE { return Compiler->SceneTextureMax(InSceneTextureId); }
	virtual int32 SceneTextureMin(uint32 InSceneTextureId) OVERRIDE { return Compiler->SceneTextureMin(InSceneTextureId); }

	virtual int32 StaticBool(bool Value) OVERRIDE { return Compiler->StaticBool(Value); }
	virtual int32 StaticBoolParameter(FName ParameterName,bool bDefaultValue) OVERRIDE { return Compiler->StaticBoolParameter(ParameterName,bDefaultValue); }
	virtual int32 StaticComponentMask(int32 Vector,FName ParameterName,bool bDefaultR,bool bDefaultG,bool bDefaultB,bool bDefaultA) OVERRIDE { return Compiler->StaticComponentMask(Vector,ParameterName,bDefaultR,bDefaultG,bDefaultB,bDefaultA); }
	virtual bool GetStaticBoolValue(int32 BoolIndex, bool& bSucceeded) OVERRIDE { return Compiler->GetStaticBoolValue(BoolIndex, bSucceeded); }
	virtual int32 StaticTerrainLayerWeight(FName ParameterName,int32 Default) OVERRIDE { return Compiler->StaticTerrainLayerWeight(ParameterName,Default); }

	virtual int32 VertexColor() OVERRIDE { return Compiler->VertexColor(); }

	virtual int32 Add(int32 A,int32 B) OVERRIDE { return Compiler->Add(A,B); }
	virtual int32 Sub(int32 A,int32 B) OVERRIDE { return Compiler->Sub(A,B); }
	virtual int32 Mul(int32 A,int32 B) OVERRIDE { return Compiler->Mul(A,B); }
	virtual int32 Div(int32 A,int32 B) OVERRIDE { return Compiler->Div(A,B); }
	virtual int32 Dot(int32 A,int32 B) OVERRIDE { return Compiler->Dot(A,B); }
	virtual int32 Cross(int32 A,int32 B) OVERRIDE { return Compiler->Cross(A,B); }

	virtual int32 Power(int32 Base,int32 Exponent) OVERRIDE { return Compiler->Power(Base,Exponent); }
	virtual int32 SquareRoot(int32 X) OVERRIDE { return Compiler->SquareRoot(X); }
	virtual int32 Length(int32 X) OVERRIDE { return Compiler->Length(X); }

	virtual int32 Lerp(int32 X,int32 Y,int32 A) OVERRIDE { return Compiler->Lerp(X,Y,A); }
	virtual int32 Min(int32 A,int32 B) OVERRIDE { return Compiler->Min(A,B); }
	virtual int32 Max(int32 A,int32 B) OVERRIDE { return Compiler->Max(A,B); }
	virtual int32 Clamp(int32 X,int32 A,int32 B) OVERRIDE { return Compiler->Clamp(X,A,B); }

	virtual int32 ComponentMask(int32 Vector,bool R,bool G,bool B,bool A) OVERRIDE { return Compiler->ComponentMask(Vector,R,G,B,A); }
	virtual int32 AppendVector(int32 A,int32 B) OVERRIDE { return Compiler->AppendVector(A,B); }
	virtual int32 TransformVector(uint8 SourceCoordType,uint8 DestCoordType,int32 A) OVERRIDE { return Compiler->TransformVector(SourceCoordType,DestCoordType,A); }
	virtual int32 TransformPosition(uint8 SourceCoordType,uint8 DestCoordType,int32 A) OVERRIDE { return Compiler->TransformPosition(SourceCoordType,DestCoordType,A); }

	virtual int32 DynamicParameter() OVERRIDE { return Compiler->DynamicParameter(); }
	virtual int32 LightmapUVs() OVERRIDE { return Compiler->LightmapUVs(); }

	virtual int32 LightmassReplace(int32 Realtime, int32 Lightmass) OVERRIDE { return Realtime; }
	virtual int32 GIReplace(int32 Direct, int32 StaticIndirect, int32 DynamicIndirect) OVERRIDE { return Compiler->GIReplace(Direct, StaticIndirect, DynamicIndirect); }
	virtual int32 ObjectOrientation() OVERRIDE { return Compiler->ObjectOrientation(); }
	virtual int32 RotateAboutAxis(int32 NormalizedRotationAxisAndAngleIndex, int32 PositionOnAxisIndex, int32 PositionIndex) OVERRIDE
	{
		return Compiler->RotateAboutAxis(NormalizedRotationAxisAndAngleIndex, PositionOnAxisIndex, PositionIndex);
	}
	virtual int32 TwoSidedSign() OVERRIDE { return Compiler->TwoSidedSign(); }
	virtual int32 VertexNormal() OVERRIDE { return Compiler->VertexNormal(); }
	virtual int32 PixelNormalWS() OVERRIDE { return Compiler->PixelNormalWS(); }

	virtual int32 CustomExpression( class UMaterialExpressionCustom* Custom, TArray<int32>& CompiledInputs ) OVERRIDE { return Compiler->CustomExpression(Custom,CompiledInputs); }
	virtual int32 DDX(int32 X) OVERRIDE { return Compiler->DDX(X); }
	virtual int32 DDY(int32 X) OVERRIDE { return Compiler->DDY(X); }

	virtual int32 AntialiasedTextureMask(int32 Tex, int32 UV, float Threshold, uint8 Channel) OVERRIDE
	{
		return Compiler->AntialiasedTextureMask(Tex, UV, Threshold, Channel);
	}
	virtual int32 Noise(int32 Position, float Scale, int32 Quality, uint8 NoiseFunction, bool bTurbulence, int32 Levels, float OutputMin, float OutputMax, float LevelScale, int32 FilterWidth) OVERRIDE
	{
		return Compiler->Noise(Position, Scale, Quality, NoiseFunction, bTurbulence, Levels, OutputMin, OutputMax, LevelScale, FilterWidth);
	}
	virtual int32 BlackBody( int32 Temp ) OVERRIDE { return Compiler->BlackBody(Temp); }
	virtual int32 PerInstanceRandom() OVERRIDE { return Compiler->PerInstanceRandom(); }
	virtual int32 PerInstanceFadeAmount() OVERRIDE { return Compiler->PerInstanceFadeAmount(); }
	virtual int32 DepthOfFieldFunction(int32 Depth, int32 FunctionValueIndex) OVERRIDE
	{
		return Compiler->DepthOfFieldFunction(Depth, FunctionValueIndex);
	}

	virtual int32 SpeedTree(ESpeedTreeGeometryType GeometryType, ESpeedTreeWindType WindType, ESpeedTreeLODType LODType, float BillboardThreshold) OVERRIDE { return Compiler->SpeedTree(GeometryType, WindType, LODType, BillboardThreshold); }
	
	virtual int32 AtmosphericFogColor(int32 WorldPosition) OVERRIDE
	{
		return Compiler->AtmosphericFogColor(WorldPosition);
	}

	virtual int32 TextureCoordinateOffset() OVERRIDE
	{
		return Compiler->TextureCoordinateOffset();
	}

	virtual int32 EyeAdaptation() OVERRIDE
	{
		return Compiler->EyeAdaptation();
	}
protected:
		
	FMaterialCompiler* Compiler;
};
