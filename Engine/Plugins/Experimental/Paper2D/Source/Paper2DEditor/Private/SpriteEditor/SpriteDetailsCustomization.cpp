// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "SpriteDetailsCustomization.h"

#include "PhysicsEngine/BodySetup.h"

//////////////////////////////////////////////////////////////////////////
// FSpriteDetailsCustomization

TSharedRef<IDetailCustomization> FSpriteDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FSpriteDetailsCustomization);
}

void FSpriteDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Make sure sprite properties are near the top
	IDetailCategoryBuilder& SpriteCategory = DetailLayout.EditCategory("Sprite", TEXT(""), ECategoryPriority::TypeSpecific);

	TSharedRef<IPropertyHandle> RenderGeometry = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, RenderGeometry));

	IDetailCategoryBuilder& RenderingCategory = DetailLayout.EditCategory("Rendering");
	IDetailPropertyRow& RenderGeometryProperty = RenderingCategory.AddProperty(RenderGeometry);

	// Build the collision category
	IDetailCategoryBuilder& CollisionCategory = DetailLayout.EditCategory("Collision");
	
	TSharedPtr<IPropertyHandle> SpriteCollisionDomainProperty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, SpriteCollisionDomain));
	TAttribute<EVisibility> ParticipatesInPhysics = TAttribute<EVisibility>::Create( TAttribute<EVisibility>::FGetter::CreateSP( this, &FSpriteDetailsCustomization::AnyPhysicsMode, SpriteCollisionDomainProperty) ) ;
	TAttribute<EVisibility> ParticipatesInPhysics3D = TAttribute<EVisibility>::Create( TAttribute<EVisibility>::FGetter::CreateSP( this, &FSpriteDetailsCustomization::PhysicsModeMatches, SpriteCollisionDomainProperty, ESpriteCollisionMode::Use3DPhysics) ) ;

	CollisionCategory.AddProperty(SpriteCollisionDomainProperty);
	CollisionCategory.AddProperty( DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, CollisionGeometry)) )
		.Visibility(ParticipatesInPhysics);
	CollisionCategory.AddProperty( DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, CollisionThickness)) )
		.Visibility(ParticipatesInPhysics3D);

	// Hide some useless body setup properties
// 	CollisionCategory.AddProperty( DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, BodySetup3D)) )
// 		.Visibility(ParticipatesInPhysics3D);
// 	TSharedRef<IPropertyHandle> BodySetup3D = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, BodySetup3D));
// 	DetailLayout.HideProperty(BodySetup3D->GetChildHandle(GET_MEMBER_NAME_CHECKED(UBodySetup, DefaultInstance)));
// 	DetailLayout.HideProperty(BodySetup3D->GetChildHandle(GET_MEMBER_NAME_CHECKED(UBodySetup, BoneName)));
// 	DetailLayout.HideProperty(BodySetup3D->GetChildHandle(GET_MEMBER_NAME_CHECKED(UBodySetup, PhysicsType)));
// 	DetailLayout.HideProperty(BodySetup3D->GetChildHandle(GET_MEMBER_NAME_CHECKED(UBodySetup, bConsiderForBounds)));

	// Show other normal properties in the sprite category so that desired ordering doesn't get messed up
	SpriteCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, SourceUV));
	SpriteCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, SourceDimension));
	SpriteCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, SourceTexture));
	SpriteCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, DefaultMaterial));
	SpriteCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, Sockets));
	SpriteCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, PixelsPerUnrealUnit));

	// Show/hide the experimental atlas group support based on whether or not it is enabled
	TSharedPtr<IPropertyHandle> AtlasGroupProperty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, AtlasGroup));
	TAttribute<EVisibility> AtlasGroupPropertyVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FSpriteDetailsCustomization::GetAtlasGroupVisibility, AtlasGroupProperty));
	SpriteCategory.AddProperty(AtlasGroupProperty, EPropertyLocation::Advanced).Visibility(AtlasGroupPropertyVisibility);

	// Show/hide the custom pivot point based on the pivot mode
	TSharedPtr<IPropertyHandle> PivotModeProperty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, PivotMode));
	TSharedPtr<IPropertyHandle> CustomPivotPointProperty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UPaperSprite, CustomPivotPoint));
	TAttribute<EVisibility> CustomPivotPointVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FSpriteDetailsCustomization::GetCustomPivotVisibility, PivotModeProperty));
	SpriteCategory.AddProperty(PivotModeProperty);
	SpriteCategory.AddProperty(CustomPivotPointProperty).Visibility(CustomPivotPointVisibility);
}

EVisibility FSpriteDetailsCustomization::PhysicsModeMatches(TSharedPtr<IPropertyHandle> Property, ESpriteCollisionMode::Type DesiredMode) const
{
	if (Property.IsValid())
	{
		uint8 ValueAsByte;
		FPropertyAccess::Result Result = Property->GetValue(/*out*/ ValueAsByte);

		if (Result == FPropertyAccess::Success)
		{
			return (((ESpriteCollisionMode::Type)ValueAsByte) == DesiredMode) ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	// If there are multiple values, show all properties
	return EVisibility::Visible;
}

EVisibility FSpriteDetailsCustomization::AnyPhysicsMode(TSharedPtr<IPropertyHandle> Property) const
{
	if (Property.IsValid())
	{
		uint8 ValueAsByte;
		FPropertyAccess::Result Result = Property->GetValue(/*out*/ ValueAsByte);

		if (Result == FPropertyAccess::Success)
		{
			return (((ESpriteCollisionMode::Type)ValueAsByte) != ESpriteCollisionMode::None) ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	// If there are multiple values, show all properties
	return EVisibility::Visible;
}

EVisibility FSpriteDetailsCustomization::GetAtlasGroupVisibility(TSharedPtr<IPropertyHandle> Property) const
{
	return GetDefault<UPaperRuntimeSettings>()->bEnableSpriteAtlasGroups ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FSpriteDetailsCustomization::GetCustomPivotVisibility(TSharedPtr<IPropertyHandle> Property) const
{
	if (Property.IsValid())
	{
		uint8 ValueAsByte;
		FPropertyAccess::Result Result = Property->GetValue(/*out*/ ValueAsByte);

		if (Result == FPropertyAccess::Success)
		{
			return (((ESpritePivotMode::Type)ValueAsByte) == ESpritePivotMode::Custom) ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	// If there are multiple values, show all properties
	return EVisibility::Visible;
}
