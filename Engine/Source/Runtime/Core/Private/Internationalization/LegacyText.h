// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

template<typename T1, typename T2>
FText FText::AsNumberTemplate(T1 Val, const FNumberFormattingOptions* const Options, const TSharedPtr<FCulture>& TargetCulture)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	const TSharedRef<FCulture> Culture = TargetCulture.IsValid() ? TargetCulture.ToSharedRef() : FInternationalization::GetCurrentCulture();
	return FText::CreateNumericalText( Culture->NumberFormattingRule.AsNumber(Val) );
}

template<typename T1, typename T2>
FText FText::AsCurrencyTemplate(T1 Val, const FNumberFormattingOptions* const Options, const TSharedPtr<FCulture>& TargetCulture)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	const TSharedRef<FCulture> Culture = TargetCulture.IsValid() ? TargetCulture.ToSharedRef() : FInternationalization::GetCurrentCulture();
	return FText::CreateNumericalText( Culture->NumberFormattingRule.AsCurrency(Val) );
}

template<typename T1, typename T2>
FText FText::AsPercentTemplate(T1 Val, const FNumberFormattingOptions* const Options, const TSharedPtr<FCulture>& TargetCulture)
{
	checkf(FInternationalization::IsInitialized() == true, TEXT("FInternationalization is not initialized. An FText formatting method was likely used in static object initialization - this is not supported."));
	const TSharedRef<FCulture> Culture = TargetCulture.IsValid() ? TargetCulture.ToSharedRef() : FInternationalization::GetCurrentCulture();
	return FText::CreateNumericalText( Culture->NumberFormattingRule.AsPercent(Val) );
}