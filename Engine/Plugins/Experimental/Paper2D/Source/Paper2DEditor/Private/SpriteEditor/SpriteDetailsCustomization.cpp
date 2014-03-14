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
	DetailLayout.EditCategory("Sprite", TEXT(""), ECategoryPriority::TypeSpecific);

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
