// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DateTime.h: Declares the FDateTime class.
=============================================================================*/

#pragma once


class UObject;
class FOutputDevice;


namespace EDayOfWeek
{
	/**
	 * Enumerates the days of the week in 7-day calendars.
	 */
	enum Type
	{
		Monday = 0,
		Tuesday,
		Wednesday,
		Thursday,
		Friday,
		Saturday,
		Sunday
	};
};


namespace EMonthOfYear
{
	/**
	 * Enumerates the months of the year in 12-month calendars.
	 */
	enum Type
	{
		January = 1,
		February,
		March,
		April,
		May,
		June,
		July,
		August,
		September,
		October,
		November,
		December
	};
};


/**
 * Implements a date and time.
 *
 * Values of this type represent dates and times between Midnight 00:00:00, January 1, 0001 and
 * Midnight 23:59:59.9999999, December 31, 9999 in the Gregorian calendar. Internally, the time
 * values are stored in ticks of 0.1 microseconds (= 100 nanoseconds) since January 1, 0001.
 *
 * To retrieve the current local date and time, use the FDateTime.Now() method. To retrieve the
 * current UTC time, use the FDateTime.UtcNow() method instead.
 *
 * This class also provides methods to convert dates and times from and to string representations,
 * calculate the number of days in a given month and year, check for leap years and determine the
 * time of day, day of week and month of year of a given date and time.
 *
 * The companion class FTimespan is provided for enabling date and time based arithmetic, such as
 * calculating the difference between two dates or adding a certain amount of time to a given date.
 *
 * Ranges of dates and times can be represented by the FDateRange class.
 *
 * @see FDateRange
 * @see FTimespan
 */
class FDateTime
{
public:

	/**
	 * Holds the components of a Gregorian date.
	 */
	struct FDate
	{
		/**
		 * Holds the date's day.
		 */
		int32 Day;

		/**
		 * Holds the date's month.
		 */
		int32 Month;

		/**
		 * Holds the date's year.
		 */
		int32 Year;
	};


	/**
	 * Holds the components of a time of day.
	 */
	struct FTime
	{
		/**
		 * Holds the time of day's hour.
		 */
		int32 Hour;

		/**
		 * Holds the time of day's minute.
		 */
		int32 Minute;

		/**
		 * Holds the time of day's second.
		 */
		int32 Second;

		/**
		 * Holds the time of day's millisecond.
		 */
		int32 Millisecond;
	};


public:

	/**
	 * Default constructor.
	 */
	FDateTime( ) { }

	/**
	 * Creates and initializes a new instance with the specified number of ticks.
	 *
	 * @param Ticks - The ticks representing the date and time.
	 */
	FDateTime( int64 InTicks )
		: Ticks(InTicks)
	{ }

	/**
	 * Creates and initializes a new instance with the specified year, month, day, hour, minute, second and millisecond.
	 *
	 * @param Year - The year.
	 * @param Month - The month.
	 * @param Day - The day.
	 * @param Hour - The hour (optional).
	 * @param Minute - The minute (optional).
	 * @param Second - The second (optional).
	 * @param Millisecond - The millisecond (optional).
	 */
	CORE_API FDateTime( int32 Year, int32 Month, int32 Day, int32 Hour = 0, int32 Minute = 0, int32 Second = 0, int32 Millisecond = 0 );


public:

	/**
	 * Returns result of adding the given time span to this date.
	 *
	 * @return A date whose value is the sum of this date and the given time span.
	 *
	 * @see FTimespan
	 */
	FDateTime operator+( const FTimespan& Other ) const
	{
		return FDateTime(Ticks + Other.GetTicks());
	}

	/**
	 * Adds the given time span to this date.
	 *
	 * @return This date.
	 *
	 * @see FTimespan
	 */
	FDateTime& operator+=( const FTimespan& Other )
	{
		Ticks += Other.GetTicks();

		return *this;
	}

	/**
	 * Returns time span between this date and the given date.
	 *
	 * @return A time span whose value is the difference of this date and the given date.
	 *
	 * @see FTimespan
	 */
	FTimespan operator-( const FDateTime& Other ) const
	{
		return FTimespan(Ticks - Other.Ticks);
	}

	/**
	 * Returns result of subtracting the given time span from this date.
	 *
	 * @return A date whose value is the difference of this date and the given time span.
	 *
	 * @see FTimespan
	 */
	FDateTime operator-( const FTimespan& Other ) const
	{
		return FDateTime(Ticks - Other.GetTicks());
	}

	/**
	 * Subtracts the given time span from this date.
	 *
	 * @return This date.
	 *
	 * @see FTimespan
	 */
	FDateTime& operator-=( const FTimespan& Other )
	{
		Ticks -= Other.GetTicks();

		return *this;
	}

	/**
	 * Compares this date with the given date for equality.
	 *
	 * @param other - The date to compare with.
	 *
	 * @return true if the dates are equal, false otherwise.
	 */
	bool operator==( const FDateTime& Other ) const
	{
		return (Ticks == Other.Ticks);
	}

	/**
	 * Compares this date with the given date for inequality.
	 *
	 * @param other - The date to compare with.
	 *
	 * @return true if the dates are not equal, false otherwise.
	 */
	bool operator!=( const FDateTime& Other ) const
	{
		return (Ticks != Other.Ticks);
	}

	/**
	 * Checks whether this date is greater than the given date.
	 *
	 * @param other - The date to compare with.
	 *
	 * @return true if this date is greater, false otherwise.
	 */
	bool operator>( const FDateTime& Other ) const
	{
		return (Ticks > Other.Ticks);
	}

	/**
	 * Checks whether this date is greater than or equal to the date span.
	 *
	 * @param other - The date to compare with.
	 *
	 * @return true if this date is greater or equal, false otherwise.
	 */
	bool operator>=( const FDateTime& Other ) const
	{
		return (Ticks >= Other.Ticks);
	}

	/**
	 * Checks whether this date is less than the given date.
	 *
	 * @param other - The date to compare with.
	 *
	 * @return true if this date is less, false otherwise.
	 */
	bool operator<( const FDateTime& Other ) const
	{
		return (Ticks < Other.Ticks);
	}

	/**
	 * Checks whether this date is less than or equal to the given date.
	 *
	 * @param other - The date to compare with.
	 *
	 * @return true if this date is less or equal, false otherwise.
	 */
	bool operator<=( const FDateTime& Other ) const
	{
		return (Ticks <= Other.Ticks);
	}


public:

	/**
	 * Gets the date part of this date.
	 *
	 * The time part is truncated and becomes 00:00:00.000.
	 *
	 * @return A FDateTime object containing the date.
	 *
	 * @see FTimespan
	 */
	FDateTime GetDate( )
	{
		return FDateTime(Ticks - (Ticks % ETimespan::TicksPerDay));
	}

	/**
	 * Gets this date's day part (1 to 31).
	 *
	 * @return Day of the month.
	 */
	int32 GetDay( ) const
	{
		return ToDate().Day;
	}

	/**
	 * Calculates this date's day of the week (Sunday - Saturday).
	 *
	 * @return The week day.
	 */
	CORE_API EDayOfWeek::Type GetDayOfWeek( ) const;

	/**
	 * Gets this date's day of the year.
	 *
	 * @return The day of year.
	 */
	CORE_API int32 GetDayOfYear( ) const;

	/**
	 * Gets this date's hour part in 24-hour clock format (0 to 23).
	 *
	 * @return The hour.
	 */
	int32 GetHour( ) const
	{
		return (int32)((Ticks / ETimespan::TicksPerHour) % 24);
	}

	/**
	 * Gets this date's hour part in 12-hour clock format (1 to 12).
	 *
	 * @return The hour in AM/PM format.
	 */
	CORE_API int32 GetHour12( ) const;

	/**
	 * Returns the Julian Day for this date.
	 *
	 * The Julian Day is the number of days since the inception of the Julian calendar at noon on
	 * Monday, January 1, 4713 B.C.E. The minimum Julian Day that can be represented in FDateTime is
	 * 1721425.5, which corresponds to Monday, January 1, 0001 in the Gregorian calendar.
	 *
	 * @return Julian Day.
	 *
	 * @see GetModifiedJulianDay
	 */
	double GetJulianDay( ) const
	{
		return (double)(1721425.5 + Ticks / ETimespan::TicksPerDay);
	}

	/**
	 * Returns the Modified Julian day.
	 *
	 * The Modified Julian Day is calculated by subtracting 2400000.5, which corresponds to midnight UTC on
	 * November 17, 1858 in the Gregorian calendar.
	 *
	 * @return Modified Julian Day
	 *
	 * @see GetJulianDay
	 */
	double GetModifiedJulianDay( ) const
	{
		return (GetJulianDay() - 2400000.5);
	}

	/**
	 * Gets this date's millisecond part (0 to 999).
	 *
	 * @return The millisecond.
	 */
	int32 GetMillisecond( ) const
	{
		return (int32)((Ticks / ETimespan::TicksPerMillisecond) % 1000);
	}

	/**
	 * Gets this date's minute part (0 to 59).
	 *
	 * @return The minute.
	 */
	int32 GetMinute( ) const
	{
		return (int32)((Ticks / ETimespan::TicksPerMinute) % 60);
	}

	/**
	 * Gets this date's the month part (1 to 12).
	 *
	 * @return The month.
	 */
	 int32 GetMonth( ) const
	 {
		 return ToDate().Month;
	 }

	 /**
	  * Gets the date's month of the year (January to December).
	  *
	  * @return Month of year.
	  */
	 EMonthOfYear::Type GetMonthOfYear( ) const
	 {
		 return static_cast<EMonthOfYear::Type>(GetMonth());
	 }

	/**
	 * Gets this date's second part.
	 *
	 * @return The second.
	 */
	int32 GetSecond( ) const
	{
		return (int32)((Ticks / ETimespan::TicksPerSecond) % 60);
	}

	/**
	 * Gets this date's representation as number of ticks.
	 *
	 * @return Number of ticks since midnight, January 1, 0001.
	 */
	int64 GetTicks( ) const
	{
		return Ticks;
	}

	/**
	 * Gets the time elapsed since midnight of this date.
	 *
	 * @param Time of day since midnight.
	 */
	FTimespan GetTimeOfDay( ) const
	{
		return FTimespan(Ticks % ETimespan::TicksPerDay);
	}

	/**
	 * Gets this date's year part.
	 *
	 * @return The year.
	 */
	int32 GetYear( ) const
	{
		return ToDate().Year;
	}

	/**
	 * Gets whether this date's time is in the afternoon.
	 *
	 * @param true if it is in the afternoon, false otherwise.
	 */
	bool IsAfternoon( ) const
	{
		return (GetHour() >= 12);
	}

	/**
	 * Gets whether this date's time is in the morning.
	 *
	 * @param true if it is in the morning, false otherwise.
	 */
	bool IsMorning( ) const
	{
		return (GetHour() < 12);
	}

	/**
	 * Returns this date as a Gregorian date structure.
	 */
	CORE_API FDate ToDate( ) const;

	/**
	 * Returns the string representation of this date using a default format.
	 *
	 * The returned string has the following format:
	 *		yyyy.mm.dd-hh.mm.ss
	 *
	 * @return String representation.
	 */
	CORE_API FString ToString( ) const;

	/**
	 * Returns the ISO-8601 string representation of the FDateTime.
	 * The resulting string assumes that the FDateTime is in UTC since this class
	 * doesn't contain timezone offset information.
	 * 
	 * @return String representation.
	 */
	CORE_API FString ToIso8601() const;

	/**
	 * Returns the string representation of this date.
	 *
	 * @param Format - The format of the returned string.
	 *
	 * @return String representation.
	 */
	CORE_API FString ToString( const TCHAR* Format ) const;

	/**
	 * Returns this date's time part as a time of day structure.
	 *
	 * @return Time of day.
	 */
	CORE_API FTime ToTime( ) const;

	/**
	 * Returns this date as the number of seconds since the Unix Epoch (January 1st of 1970).
	 *
	 * @return Time of day.
	 */
	CORE_API int32 ToUnixTimestamp( ) const
	{
		return static_cast<int32>((Ticks - FDateTime(1970, 1, 1).Ticks) / ETimespan::TicksPerSecond);
	}


public:

	/**
	 * Gets the number of days in the year and month.
	 *
	 * @param Year - The year.
	 * @param Month - The month.
	 *
	 * @return The number of days
	 */
	static CORE_API int32 DaysInMonth( int32 Year, int32 Month );

	/**
	 * Gets the number of days in the given year.
	 *
	 * @param Year - The year.
	 *
	 * @return The number of days.
	 */
	static CORE_API int32 DaysInYear( int32 Year );

	/**
	 * Returns the proleptic Gregorian date for the given Julian Day.
	 *
	 * @param JulianDay - The Julian Day.
	 *
	 * @return Gregorian date and time.
	 */
	static FDateTime FromJulianDay( double JulianDay )
	{
		return FDateTime((int64)((JulianDay - 1721425.5) * ETimespan::TicksPerDay));
	}

	/**
	 * Returns the date from Unix time (seconds from midnight 1970-01-01)
	 *
	 * @param UnixTime - Unix time (seconds from midnight 1970-01-01)
	 *
	 * @return Gregorian date and time.
	 *
	 * @see ToUnixTimestamp
	 */
	static FDateTime FromUnixTimestamp( int32 UnixTime )
	{
		return FDateTime(1970, 1, 1) + FTimespan(static_cast<int64>(UnixTime) * ETimespan::TicksPerSecond);
	}

	/**
	 * Parses a stringified DateTime in ISO-8601 format
	 * 
	 * @param DateTimeString The string to be parsed
	 * @param OutDateTime FDateTime object (in UTC) corresponding to the input string (which may have been in any timezone).
	 *
	 * @return success/fail
	 */
	static CORE_API bool FromIso8601( const TCHAR* DateTimeString, FDateTime* OutDateTime );

	/**
	 * Checks whether the given year is a leap year.
	 *
	 * A leap year is a year containing one additional day in order to keep the calendar synchronized
	 * with the astronomical year. All years divisible by 4, but not divisible by 100 - except if they
	 * are also divisible by 400 - are leap years.
	 *
	 * @param Year - The year to check.
	 *
	 * @return true if the year is a leap year, false otherwise.
	 */
	static CORE_API bool IsLeapYear( int32 Year );

	/**
	 * Checks whether the date and time components are valid.
	 *
	 * @return true if the components are valid, false otherwise.
	 */
	static CORE_API bool IsValid( int32 Year, int32 Month, int32 Day, int32 Hour, int32 Minute, int32 Second, int32 Millisecond );

	/**
	 * Returns the maximum date value.
	 *
	 * The maximum date value is December 31, 9999, 23:59:59.9999999.
	 *
	 * @see MinValue
	 */
	static FDateTime MaxValue( )
	{
		return FDateTime(3652059 * ETimespan::TicksPerDay - 1);
	}

	/**
	 * Returns the minimum date value.
	 *
	 * The minimum date value is January 1, 0001, 00:00:00.0.
	 *
	 * @see MaxValue
	 */
	static FDateTime MinValue( )
	{
		return FDateTime(0);
	}

	/**
	 * Gets the local date and time on this computer.
	 *
	 * @return Current date and time.
	 */
	static CORE_API FDateTime Now( );

	/**
	 * Converts a string to a date and time.
	 *
	 * Currently, the string must be in the format written by either FDateTime.ToString() or
	 * FTimeStamp.TimestampToFString(). Other formats are not supported at this time.
	 *
	 * @param DateTimeString - The string to convert.
	 * @param OutDateTime - Will contain the parsed date and time.
	 *
	 * @return true if the string was converted successfully, false otherwise.
	 */
	static CORE_API bool Parse( const FString& DateTimeString, FDateTime& OutDateTime );

	/**
	 * Gets the local date on this computer.
	 *
	 * The time component is set to 00:00:00
	 *
	 * @return Current date.
	 */
	static FDateTime Today( )
	{
		return Now().GetDate();
	}

	/**
	 * Gets the UTC date and time on this computer.
	 *
	 * @return Current date and time.
	 */
	static CORE_API FDateTime UtcNow( );


public:

	/**
	 * Serializes the given date and time from or into the specified archive.
	 *
	 * @param Ar - The archive to serialize from or into.
	 * @param DateTime - The date and time value to serialize.
	 *
	 * @return The archive.
	 *
	 * @todo gmp: Figure out better include order in Core.h so this can be inlined.
	 */
	friend CORE_API FArchive& operator<<( FArchive& Ar, FDateTime& DateTime );

	/**
	 * Gets the hash for the specified date and time.
	 *
	 * @param DateTime - The date and time to get the hash for.
	 *
	 * @return Hash value.
	 */
	friend uint32 GetTypeHash( const FDateTime& DateTime )
	{
		return GetTypeHash(DateTime.Ticks);
	}

	/**
	 * Exports the DateTime value to a string.
	 *
	 * @param ValueStr - Will hold the string value.
	 * @param DefaultValue - The default value.
	 * @param Parent - Not used.
	 * @param PortFlags - Not used.
	 * @param ExportRootScope - Not used.
	 *
	 * @return true on success, false otherwise.
	 */
	CORE_API bool ExportTextItem( FString& ValueStr, FDateTime const& DefaultValue, UObject* Parent, int32 PortFlags, class UObject* ExportRootScope ) const;

	/**
	 * Imports the DateTime value from a text buffer.
	 *
	 * @param Buffer - The text buffer to import from.
	 * @param PortFlags - Not used.
	 * @param Parent - Not used.
	 * @param ErrorText - The output device for error logging.
	 *
	 * @return true on success, false otherwise.
	 */
	CORE_API bool ImportTextItem( const TCHAR*& Buffer, int32 PortFlags, class UObject* Parent, FOutputDevice* ErrorText );


protected:

	/**
	 * Holds the days per month in a non-leap year.
	 */
	static const int32 DaysPerMonth[];

	/**
	 * Holds the cumulative days per month in a non-leap year.
	 */
	static const int32 DaysToMonth[];


public:

	// Holds the ticks in 100 nanoseconds resolution since January 1, 0001 A.D.
	int64 Ticks;
};
