// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

typedef TArray<TSharedRef<const class FPropertyRestriction>> TRestrictionList;

class FParticleModuleDetailsBase : public IDetailCustomization
{
protected:
	void RestrictPropertiesOnGPUEmitter( IDetailLayoutBuilder& DetailBuilder, TArray<FString>& PropertyNames, TRestrictionList& Restrictions );

	TSharedRef<const FPropertyRestriction> GetDistributionsForGPURestriction();
};

class FParticleModuleRequiredDetails: public FParticleModuleDetailsBase
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
};

class FParticleModuleColorOverLifeDetails : public FParticleModuleDetailsBase
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
};

class FParticleModuleSubUVDetails : public FParticleModuleDetailsBase
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
};

class FParticleModuleAccelerationDetails : public FParticleModuleDetailsBase
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
};

class FParticleModuleAccelerationDragDetails : public FParticleModuleDetailsBase
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
};

 class FParticleModuleAccelerationDragScaleOverLifeDetails : public FParticleModuleDetailsBase
 {
 public:
 	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
 	static TSharedRef<IDetailCustomization> MakeInstance();
 
 	/** IDetailCustomization interface */
 	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
 };

 class FParticleModuleCollisionGPUDetails : public FParticleModuleDetailsBase
 {
 public:
 	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
 	static TSharedRef<IDetailCustomization> MakeInstance();
 
 	/** IDetailCustomization interface */
 	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
 };

 class FParticleModuleOrbitDetails : public FParticleModuleDetailsBase
 {
 public:
	 /** Makes a new instance of this detail layout class for a specific detail view requesting it */
	 static TSharedRef<IDetailCustomization> MakeInstance();

	 /** IDetailCustomization interface */
	 virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
 };

 class FParticleModuleSizeMultiplyLifeDetails : public FParticleModuleDetailsBase
 {
 public:
	 /** Makes a new instance of this detail layout class for a specific detail view requesting it */
	 static TSharedRef<IDetailCustomization> MakeInstance();

	 /** IDetailCustomization interface */
	 virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
 };

 class FParticleModuleSizeScaleDetails : public FParticleModuleDetailsBase
 {
 public:
	 /** Makes a new instance of this detail layout class for a specific detail view requesting it */
	 static TSharedRef<IDetailCustomization> MakeInstance();

	 /** IDetailCustomization interface */
	 virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
 };

 class FParticleModuleVectorFieldScaleDetails : public FParticleModuleDetailsBase
 {
 public:
	 /** Makes a new instance of this detail layout class for a specific detail view requesting it */
	 static TSharedRef<IDetailCustomization> MakeInstance();

	 /** IDetailCustomization interface */
	 virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
 };

 class FParticleModuleVectorFieldScaleOverLifeDetails : public FParticleModuleDetailsBase
 {
 public:
	 /** Makes a new instance of this detail layout class for a specific detail view requesting it */
	 static TSharedRef<IDetailCustomization> MakeInstance();

	 /** IDetailCustomization interface */
	 virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
 };
 