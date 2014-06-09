// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"
#include "SUserWidgetTest.h"


namespace Implementation
{
	
class SUserWidgetExample : public ::SUserWidgetExample
{
public:

	void Construct( const FArguments& InArgs )
	{
		SUserWidget::Construct( SUserWidget::FArguments()
		[
			SNew(STextBlock)
			.Text( FText::Format( NSLOCTEXT("SlateTestSuite","UserWidgetExampleTitle"," Implemented in the .cpp : {0}"), InArgs._Title ) )
		]);
	}


	virtual void DoStuff() override
	{

	}
};

}


TSharedRef<SUserWidgetExample> SUserWidgetExample::New()
{
	return MakeShareable(new Implementation::SUserWidgetExample()); 
}
