// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

CORE_API DECLARE_LOG_CATEGORY_EXTERN(LogJson, Log, All);

/**
 * Json (JavaScript Object Notation) is a lightweight data-interchange format.
 * Information on how it works can be found here: http://www.json.org/.
 * This code was written from scratch with only the Json spec as a guide.
 *
 * In order to use Json effectively, you need to be familiar with the Object/Value
 * hierarchy, and you should use the FJsonObject class and FJsonValue subclasses.
 */

/**
 * Represents all the types a Json Value can be.
 */
namespace EJson
{
	enum Type
	{
		None,
		Null,
		String,
		Number,
		Boolean,
		Array,
		Object,
	};
}

namespace EJsonToken
{
	enum Type
	{
		None,
		Comma,
		CurlyOpen,
		CurlyClose,
		SquareOpen,
		SquareClose,
		Colon,
		String,
		Number,
		True,
		False,
		Null,
	};
}

namespace EJsonNotation
{
	enum Type
	{
		ObjectStart,
		ObjectEnd,
		ArrayStart,
		ArrayEnd,
		Boolean,
		String,
		Number,
		Null,

		Error,
	};
}


#include "JsonPrintPolicies.h"
#include "JsonReader.h"
#include "JsonWriter.h"
#include "JsonDocumentObjectModel.h"
#include "JsonSerializer.h"
