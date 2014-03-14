// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "AutomationTest.h"

// Disable optimization for DateTimeFormattingRulesTest as it compiles very slowly in development builds
PRAGMA_DISABLE_OPTIMIZATION

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDateTimeFormattingRulesTest, "Core.Misc.DateTime Formatting Rules", EAutomationTestFlags::ATF_SmokeTest)

namespace
{
	void Test(FDateTimeFormattingRulesTest* const ExplicitThis, const TCHAR* const Desc, const FText& A, const FText& B)
	{
		if( !A.EqualTo(B) )
		{
			ExplicitThis->AddError(FString::Printf(TEXT("%s - A=%s B=%s"),Desc,*A.ToString(),*B.ToString()));
		}
	}
}

bool FDateTimeFormattingRulesTest::RunTest (const FString& Parameters)
{
	const FString OriginalCulture = FInternationalization::GetCurrentCulture()->GetName();

	//////////////////////////////////////////////////////////////////////////

#if UE_ENABLE_ICU
	const FDateTime UnixEpoch = FDateTime::FromUnixTimestamp(0);
	const FDateTime UnixBillennium = FDateTime::FromUnixTimestamp(1000000000);
	const FDateTime UnixOnes = FDateTime::FromUnixTimestamp(1111111111);
	const FDateTime UnixDecimalSequence = FDateTime::FromUnixTimestamp(1234567890);
	const FDateTime::FDate TestDate = { 13, 6, 1990 };
	const FDateTime::FTime TestTime = { 12, 34, 56, 789 };
	const FDateTime TestDateTime( TestDate.Year, TestDate.Month, TestDate.Day, TestTime.Hour, TestTime.Minute, TestTime.Second, TestTime.Millisecond );

	FInternationalization::SetCurrentCulture("en-US");

	// Unix Time Values via Date Time
	Test( this, TEXT("Testing Unix Epoch"), FText::AsDateTime(UnixEpoch, EDateTimeStyle::Short, EDateTimeStyle::Short, "GMT"), FText::FromString( TEXT("1/1/70, 12:00 AM") ) );
	Test( this, TEXT("Testing Unix Epoch"), FText::AsDateTime(UnixEpoch, EDateTimeStyle::Medium, EDateTimeStyle::Medium, "GMT"), FText::FromString( TEXT("Jan 1, 1970, 12:00:00 AM") ) );
	Test( this, TEXT("Testing Unix Epoch"), FText::AsDateTime(UnixEpoch, EDateTimeStyle::Long, EDateTimeStyle::Long, "GMT"), FText::FromString( TEXT("January 1, 1970 at 12:00:00 AM GMT") ) );
	Test( this, TEXT("Testing Unix Epoch"), FText::AsDateTime(UnixEpoch, EDateTimeStyle::Full, EDateTimeStyle::Full, "GMT"), FText::FromString( TEXT("Thursday, January 1, 1970 at 12:00:00 AM GMT") ) );

	Test( this, TEXT("Testing Unix Billennium"), FText::AsDateTime(UnixBillennium, EDateTimeStyle::Short, EDateTimeStyle::Short, "GMT"), FText::FromString( TEXT("9/9/01, 1:46 AM") ) );
	Test( this, TEXT("Testing Unix Billennium"), FText::AsDateTime(UnixBillennium, EDateTimeStyle::Medium, EDateTimeStyle::Medium, "GMT"), FText::FromString( TEXT("Sep 9, 2001, 1:46:40 AM") ) );
	Test( this, TEXT("Testing Unix Billennium"), FText::AsDateTime(UnixBillennium, EDateTimeStyle::Long, EDateTimeStyle::Long, "GMT"), FText::FromString( TEXT("September 9, 2001 at 1:46:40 AM GMT") ) );
	Test( this, TEXT("Testing Unix Billennium"), FText::AsDateTime(UnixBillennium, EDateTimeStyle::Full, EDateTimeStyle::Full, "GMT"), FText::FromString( TEXT("Sunday, September 9, 2001 at 1:46:40 AM GMT") ) );

	Test( this, TEXT("Testing Unix Ones"), FText::AsDateTime(UnixOnes, EDateTimeStyle::Short, EDateTimeStyle::Short, "GMT"), FText::FromString( TEXT("3/18/05, 1:58 AM") ) );
	Test( this, TEXT("Testing Unix Ones"), FText::AsDateTime(UnixOnes, EDateTimeStyle::Medium, EDateTimeStyle::Medium, "GMT"), FText::FromString( TEXT("Mar 18, 2005, 1:58:31 AM") ) );
	Test( this, TEXT("Testing Unix Ones"), FText::AsDateTime(UnixOnes, EDateTimeStyle::Long, EDateTimeStyle::Long, "GMT"), FText::FromString( TEXT("March 18, 2005 at 1:58:31 AM GMT") ) );
	Test( this, TEXT("Testing Unix Ones"), FText::AsDateTime(UnixOnes, EDateTimeStyle::Full, EDateTimeStyle::Full, "GMT"), FText::FromString( TEXT("Friday, March 18, 2005 at 1:58:31 AM GMT") ) );

	Test( this, TEXT("Testing Unix Decimal Sequence"), FText::AsDateTime(UnixDecimalSequence, EDateTimeStyle::Short, EDateTimeStyle::Short, "GMT"), FText::FromString( TEXT("2/13/09, 11:31 PM") ) );
	Test( this, TEXT("Testing Unix Decimal Sequence"), FText::AsDateTime(UnixDecimalSequence, EDateTimeStyle::Medium, EDateTimeStyle::Medium, "GMT"), FText::FromString( TEXT("Feb 13, 2009, 11:31:30 PM") ) );
	Test( this, TEXT("Testing Unix Decimal Sequence"), FText::AsDateTime(UnixDecimalSequence, EDateTimeStyle::Long, EDateTimeStyle::Long, "GMT"), FText::FromString( TEXT("February 13, 2009 at 11:31:30 PM GMT") ) );
	Test( this, TEXT("Testing Unix Decimal Sequence"), FText::AsDateTime(UnixDecimalSequence, EDateTimeStyle::Full, EDateTimeStyle::Full, "GMT"), FText::FromString( TEXT("Friday, February 13, 2009 at 11:31:30 PM GMT") ) );

	// Date
	Test( this, TEXT("Testing Date"), FText::AsDate(TestDate, EDateTimeStyle::Short), FText::FromString( TEXT("6/13/90") ) );
	Test( this, TEXT("Testing Date"), FText::AsDate(TestDate, EDateTimeStyle::Medium), FText::FromString( TEXT("Jun 13, 1990") ) );
	Test( this, TEXT("Testing Date"), FText::AsDate(TestDate, EDateTimeStyle::Long), FText::FromString( TEXT("June 13, 1990") ) );
	Test( this, TEXT("Testing Date"), FText::AsDate(TestDate, EDateTimeStyle::Full), FText::FromString( TEXT("Wednesday, June 13, 1990") ) );

	// Time
	Test( this, TEXT("Testing Time"), FText::AsTime(TestTime, EDateTimeStyle::Short, "GMT"), FText::FromString( TEXT("12:34 PM") ) );
	Test( this, TEXT("Testing Time"), FText::AsTime(TestTime, EDateTimeStyle::Medium, "GMT"), FText::FromString( TEXT("12:34:56 PM") ) );
	Test( this, TEXT("Testing Time"), FText::AsTime(TestTime, EDateTimeStyle::Long, "GMT"), FText::FromString( TEXT("12:34:56 PM GMT") ) );
	Test( this, TEXT("Testing Time"), FText::AsTime(TestTime, EDateTimeStyle::Full, "GMT"), FText::FromString( TEXT("12:34:56 PM GMT") ) );

	// Date Time
	Test( this, TEXT("Testing Date-Time"), FText::AsDateTime(TestDateTime, EDateTimeStyle::Short,EDateTimeStyle::Short, "GMT"), FText::FromString( TEXT("6/13/90, 12:34 PM") ) );
	Test( this, TEXT("Testing Date-Time"), FText::AsDateTime(TestDateTime, EDateTimeStyle::Medium, EDateTimeStyle::Medium, "GMT"), FText::FromString( TEXT("Jun 13, 1990, 12:34:56 PM") ) );
	Test( this, TEXT("Testing Date-Time"), FText::AsDateTime(TestDateTime, EDateTimeStyle::Long, EDateTimeStyle::Long, "GMT"), FText::FromString( TEXT("June 13, 1990 at 12:34:56 PM GMT") ) );
	Test( this, TEXT("Testing Date-Time"), FText::AsDateTime(TestDateTime, EDateTimeStyle::Full, EDateTimeStyle::Full, "GMT"), FText::FromString( TEXT("Wednesday, June 13, 1990 at 12:34:56 PM GMT") ) );

	FInternationalization::SetCurrentCulture("ja-JP");

	// Unix Time Values via Date Time
	Test( this, TEXT("Testing Unix Epoch"), FText::AsDateTime(UnixEpoch, EDateTimeStyle::Short, EDateTimeStyle::Short, "GMT"), FText::FromString( TEXT("1970/01/01 0:00") ) );
	Test( this, TEXT("Testing Unix Epoch"), FText::AsDateTime(UnixEpoch, EDateTimeStyle::Medium, EDateTimeStyle::Medium, "GMT"), FText::FromString( TEXT("1970/01/01 0:00:00") ) );
	Test( this, TEXT("Testing Unix Epoch"), FText::AsDateTime(UnixEpoch, EDateTimeStyle::Long, EDateTimeStyle::Long, "GMT"), FText::FromString( TEXT("1970\x5E74")TEXT("1\x6708")TEXT("1\x65E5")TEXT(" 0:00:00 GMT") ) );
	Test( this, TEXT("Testing Unix Epoch"), FText::AsDateTime(UnixEpoch, EDateTimeStyle::Full, EDateTimeStyle::Full, "GMT"), FText::FromString( TEXT("1970\x5E74")TEXT("1\x6708")TEXT("1\x65E5\x6728\x66DC\x65E5")TEXT(" ")TEXT("0\x6642")TEXT("00\x5206")TEXT("00\x79D2")TEXT(" GMT") ) );

	Test( this, TEXT("Testing Unix Billennium"), FText::AsDateTime(UnixBillennium, EDateTimeStyle::Short, EDateTimeStyle::Short, "GMT"), FText::FromString( TEXT("2001/09/09 1:46") ) );
	Test( this, TEXT("Testing Unix Billennium"), FText::AsDateTime(UnixBillennium, EDateTimeStyle::Medium, EDateTimeStyle::Medium, "GMT"), FText::FromString( TEXT("2001/09/09 1:46:40") ) );
	Test( this, TEXT("Testing Unix Billennium"), FText::AsDateTime(UnixBillennium, EDateTimeStyle::Long, EDateTimeStyle::Long, "GMT"), FText::FromString( TEXT("2001\x5E74")TEXT("9\x6708")TEXT("9\x65E5")TEXT(" 1:46:40 GMT") ) );
	Test( this, TEXT("Testing Unix Billennium"), FText::AsDateTime(UnixBillennium, EDateTimeStyle::Full, EDateTimeStyle::Full, "GMT"), FText::FromString( TEXT("2001\x5E74")TEXT("9\x6708")TEXT("9\x65E5\x65E5\x66DC\x65E5")TEXT(" ")TEXT("1\x6642")TEXT("46\x5206")TEXT("40\x79D2")TEXT(" GMT") ) );

	Test( this, TEXT("Testing Unix Ones"), FText::AsDateTime(UnixOnes, EDateTimeStyle::Short, EDateTimeStyle::Short, "GMT"), FText::FromString( TEXT("2005/03/18 1:58") ) );
	Test( this, TEXT("Testing Unix Ones"), FText::AsDateTime(UnixOnes, EDateTimeStyle::Medium, EDateTimeStyle::Medium, "GMT"), FText::FromString( TEXT("2005/03/18 1:58:31") ) );
	Test( this, TEXT("Testing Unix Ones"), FText::AsDateTime(UnixOnes, EDateTimeStyle::Long, EDateTimeStyle::Long, "GMT"), FText::FromString( TEXT("2005\x5E74")TEXT("3\x6708")TEXT("18\x65E5")TEXT(" 1:58:31 GMT") ) );
	Test( this, TEXT("Testing Unix Ones"), FText::AsDateTime(UnixOnes, EDateTimeStyle::Full, EDateTimeStyle::Full, "GMT"), FText::FromString( TEXT("2005\x5E74")TEXT("3\x6708")TEXT("18\x65E5\x91D1\x66DC\x65E5")TEXT(" ")TEXT("1\x6642")TEXT("58\x5206")TEXT("31\x79D2")TEXT(" GMT") ) );

	Test( this, TEXT("Testing Unix Decimal Sequence"), FText::AsDateTime(UnixDecimalSequence, EDateTimeStyle::Short, EDateTimeStyle::Short, "GMT"), FText::FromString( TEXT("2009/02/13 23:31") ) );
	Test( this, TEXT("Testing Unix Decimal Sequence"), FText::AsDateTime(UnixDecimalSequence, EDateTimeStyle::Medium, EDateTimeStyle::Medium, "GMT"), FText::FromString( TEXT("2009/02/13 23:31:30") ) );
	Test( this, TEXT("Testing Unix Decimal Sequence"), FText::AsDateTime(UnixDecimalSequence, EDateTimeStyle::Long, EDateTimeStyle::Long, "GMT"), FText::FromString( TEXT("2009\x5E74")TEXT("2\x6708")TEXT("13\x65E5 23:31:30 GMT") ) );
	Test( this, TEXT("Testing Unix Decimal Sequence"), FText::AsDateTime(UnixDecimalSequence, EDateTimeStyle::Full, EDateTimeStyle::Full, "GMT"), FText::FromString( TEXT("2009\x5E74")TEXT("2\x6708")TEXT("13\x65E5\x91D1\x66DC\x65E5")TEXT(" ")TEXT("23\x6642")TEXT("31\x5206")TEXT("30\x79D2")TEXT(" GMT") ) );

	// Date
	Test( this, TEXT("Testing Date"), FText::AsDate(TestDate, EDateTimeStyle::Short), FText::FromString( TEXT("1990/06/13") ) );
	Test( this, TEXT("Testing Date"), FText::AsDate(TestDate, EDateTimeStyle::Medium), FText::FromString( TEXT("1990/06/13") ) );
	Test( this, TEXT("Testing Date"), FText::AsDate(TestDate, EDateTimeStyle::Long), FText::FromString( TEXT("1990\x5E74")TEXT("6\x6708")TEXT("13\x65E5") ) );
	Test( this, TEXT("Testing Date"), FText::AsDate(TestDate, EDateTimeStyle::Full), FText::FromString( TEXT("1990\x5E74")TEXT("6\x6708")TEXT("13\x65E5\x6C34\x66DC\x65E5") ) );

	// Time
	Test( this, TEXT("Testing Time"), FText::AsTime(TestTime, EDateTimeStyle::Short, "GMT"), FText::FromString( TEXT("12:34") ) );
	Test( this, TEXT("Testing Time"), FText::AsTime(TestTime, EDateTimeStyle::Medium, "GMT"), FText::FromString( TEXT("12:34:56") ) );
	Test( this, TEXT("Testing Time"), FText::AsTime(TestTime, EDateTimeStyle::Long, "GMT"), FText::FromString( TEXT("12:34:56 GMT") ) );
	Test( this, TEXT("Testing Time"), FText::AsTime(TestTime, EDateTimeStyle::Full, "GMT"), FText::FromString( TEXT("12\x6642")TEXT("34\x5206")TEXT("56\x79D2")TEXT(" GMT") ) );

	// Date Time
	Test( this, TEXT("Testing Date-Time"), FText::AsDateTime(TestDateTime, EDateTimeStyle::Short,EDateTimeStyle::Short, "GMT"), FText::FromString( TEXT("1990/06/13 12:34") ) );
	Test( this, TEXT("Testing Date-Time"), FText::AsDateTime(TestDateTime, EDateTimeStyle::Medium, EDateTimeStyle::Medium, "GMT"), FText::FromString( TEXT("1990/06/13 12:34:56") ) );
	Test( this, TEXT("Testing Date-Time"), FText::AsDateTime(TestDateTime, EDateTimeStyle::Long, EDateTimeStyle::Long, "GMT"), FText::FromString( TEXT("1990\x5E74")TEXT("6\x6708")TEXT("13\x65E5")TEXT(" 12:34:56 GMT") ) );
	Test( this, TEXT("Testing Date-Time"), FText::AsDateTime(TestDateTime, EDateTimeStyle::Full, EDateTimeStyle::Full, "GMT"), FText::FromString( TEXT("1990\x5E74")TEXT("6\x6708")TEXT("13\x65E5\x6C34\x66DC\x65E5")TEXT(" ")TEXT("12\x6642")TEXT("34\x5206")TEXT("56\x79D2 GMT") ) );
#else
	AddWarning("ICU is disabled thus locale-aware date/time formatting is disabled.");
#endif
	//////////////////////////////////////////////////////////////////////////

	FInternationalization::SetCurrentCulture(OriginalCulture);

	return true;
}

PRAGMA_ENABLE_OPTIMIZATION