// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


var UE_JavaScriptLibary =
{
	UE_SendAndRecievePayLoad: function (url, indata, insize, outdataptr, outsizeptr) {
		var _url = Pointer_stringify(url)

		var request = new XMLHttpRequest();
		if (insize && indata) {
			var postData = Module.HEAP8.subarray(indata, indata + insize);
			request.open('POST', _url, false);
			request.send(postData);
		} else {
			request.open('GET', _url, false);
			request.send();
		}

		console.log("Fetching " + _url +  request.lenght);

		if (request.status != 200) {
			console.log("Fetching " + _url + " failed: " + request.responseText);

			Module.HEAP32[outsizeptr >> 2] = 0;
			Module.HEAP32[outdataptr >> 2] = 0;

			return;
		}

		// we got the XHR result as a string.  We need to write this to Module.HEAP8[outdataptr]
		var replyString = request.response;
		var replyLength = replyString.length;

		var outdata = Module._malloc(replyLength);
		if (!outdata) {
			console.log("Failed to allocate " + replyLength + " bytes in heap for reply");

			Module.HEAP32[outsizeptr >> 2] = 0;
			Module.HEAP32[outdataptr >> 2] = 0;

			return;
		}

		// tears and crying.  Copy from the result-string into the heap.
		var replyDest = Module.HEAP8.subarray(outdata, outdata + replyLength);
		for (var i = 0; i < replyLength; ++i) {
			replyDest[i] = replyString.charCodeAt(i);
		}

		Module.HEAP32[outsizeptr >> 2] = replyLength;
		Module.HEAP32[outdataptr >> 2] = outdata;
	},


	UE_SaveGame: function (name, userIndex, indata, insize){
			// user index is not used.
			var _name = Pointer_stringify(name);
			var gamedata = Module.HEAP8.subarray(indata, indata + insize);
			// local storage only takes strings, we need to convert string to base64 before storing.
			var b64encoded = base64EncArr(gamedata);
			$.jStorage.set(_name, b64encoded);
			return true;
	},


	UE_LoadGame: function (name, userIndex, outdataptr, outsizeptr){
		var _name = Pointer_stringify(name);
		// local storage only takes strings, we need to convert string to base64 before storing.
		var b64encoded = $.jStorage.get(_name);
		if (typeof b64encoded == null)
			return false;
		var decodedArray = base64DecToArr(b64encoded);
		// copy back the decoded array.
		var outdata = Module._malloc(decodedArray.length);
		// view the allocated data as a HEAP8.
		var dest = Module.HEAP8.subarray(outdata, outdata + decodedArray.length);
		// copy back.
		for (var i = 0; i < decodedArray.length; ++i) {
			dest[i] = decodedArray[i];
		}
		Module.HEAP32[outsizeptr >> 2] = decodedArray.length;
		Module.HEAP32[outdataptr >> 2] = outdata;
		return true;
	},

	UE_DoesSaveGameExist: function (name, userIndex){
		var _name = Pointer_stringify(name);
		var b64encoded = $.jStorage.get(_name);
		if (b64encoded == null)
			return false;
		return true;
	}

};

mergeInto(LibraryManager.library, UE_JavaScriptLibary);
