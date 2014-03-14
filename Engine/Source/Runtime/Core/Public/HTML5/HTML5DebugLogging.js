var LibraryDebugLogging = {
  // Returns the given mangled C++ function name demangled to a readable form, or an empty string if the given string could not be demangled.
  // E.g. "_Z3foov" -> "foo()".
  emscripten_demangle: function(functionname) {
    if (typeof ___cxa_demangle === 'undefined') {
      Runtime.warnOnce('emscripten_demangle is not available in the current build. Please add the file $EMSCRIPTEN/system/lib/libcxxabi/src/cxa_demangle.cpp to your build to show demangled symbol names.');
      return '';
    }
    // The application entry point has a special name, so treat it manually.
    if (functionname == 'Object._main' || functionname == '_main') {
      return 'main()';
    }
    // If the compiled symbol starts with two underscores, there's one extra, which throws off cxa_demangle, so remove the first underscore.
    if (functionname.indexOf("__") == 0) {
      functionname = functionname.slice(1);
    }
    var sp = STACKTOP;
    var stat = allocate([0, 0, 0, 0], 'i32', ALLOC_STACK);
    var mangledname = allocate(512, 'i32*', ALLOC_STACK);
    writeStringToMemory(functionname, mangledname, false);
    var demangledname = allocate(512, 'i32*', ALLOC_STACK);
    ___cxa_demangle(mangledname, demangledname, 512, stat);
    var str = Pointer_stringify(demangledname);
    STACKTOP = sp;
    return str;
  },
  
  emscripten_get_callstack_js__deps: ['emscripten_demangle'],
  emscripten_get_callstack_js: function(flags) {
    var err = new Error();
    if (!err.stack) {
      Runtime.warnOnce('emscripten_get_callstack_js is not supported on this browser!');
      return '';
    }
    var callstack = new Error().stack.toString();

    // Find the symbols in the callstack that corresponds to the functions that report callstack information, and remove everyhing up to these from the output.
    var iThisFunc = callstack.lastIndexOf('_emscripten_log');
    var iThisFunc2 = callstack.lastIndexOf('_emscripten_get_callstack');
    var iNextLine = callstack.indexOf('\n', Math.max(iThisFunc, iThisFunc2))+1;
    callstack = callstack.slice(iNextLine);

    // If user requested to see the original source stack, but no source map information is available, just fall back to showing the JS stack.
    if (flags & 8/*EM_LOG_C_STACK*/ && typeof emscripten_source_map === 'undefined') {
      Runtime.warnOnce('Source map information is not available, emscripten_log with EM_LOG_C_STACK will be ignored.');
      flags ^= 8/*EM_LOG_C_STACK*/;
      flags |= 16/*EM_LOG_JS_STACK*/;
    }

    // Process all lines:
    lines = callstack.split('\n');
    callstack = '';
    var firefoxRe = new RegExp('\\s*(.*?)@(.*):(.*)'); // Extract components of form '       Object._main@http://server.com:4324'
    var chromeRe = new RegExp('\\s*at (.*?) \\\((.*):(.*):(.*)\\\)'); // Extract components of form '    at Object._main (http://server.com/file.html:4324:12)'
    
    for(l in lines) {
      var line = lines[l];

      var jsSymbolName = '';
      var file = '';
      var lineno = 0;
      var column = 0;

      var parts = chromeRe.exec(line);
      if (parts && parts.length == 5) {
        jsSymbolName = parts[1];
        file = parts[2];
        lineno = parts[3];
        column = parts[4];
      } else {
        parts = firefoxRe.exec(line);
        if (parts && parts.length == 4) {
          jsSymbolName = parts[1];
          file = parts[2];
          lineno = parts[3];
          column = 0; // Firefox doesn't carry column information.
        } else {
          // Was not able to extract this line for demangling/sourcemapping purposes. Output it as-is.
          callstack += line + '\n';
          continue;
        }
      }

      // Try to demangle the symbol, but fall back to showing the original JS symbol name if not available.
      var cSymbolName = (flags & 32/*EM_LOG_DEMANGLE*/) ? _emscripten_demangle(jsSymbolName) : jsSymbolName;
      if (!cSymbolName) {
        cSymbolName = jsSymbolName;
      }

      var haveSourceMap = false;

      if (flags & 8/*EM_LOG_C_STACK*/) {
        var orig = emscripten_source_map.originalPositionFor({line: lineno, column: column});
        haveSourceMap = (orig && orig.source);
        if (haveSourceMap) {
          if (flags & 64/*EM_LOG_NO_PATHS*/) {
            orig.source = orig.source.substring(orig.source.replace(/\\/g, "/").lastIndexOf('/')+1);
          }
          callstack += '    at ' + cSymbolName + ' (' + orig.source + ':' + orig.line + ':' + orig.column + ')\n';
        }
      }
      if ((flags & 16/*EM_LOG_JS_STACK*/) || !haveSourceMap) {
        if (flags & 64/*EM_LOG_NO_PATHS*/) {
          file = file.substring(file.replace(/\\/g, "/").lastIndexOf('/')+1);
        }
        callstack += (haveSourceMap ? ('     = '+jsSymbolName) : ('    at '+cSymbolName)) + ' (' + file + ':' + lineno + ':' + column + ')\n';
      }
    }
    // Trim extra whitespace at the end of the output.
    callstack = callstack.replace(/\s+$/, '');
    return callstack;
  },

  emscripten_get_callstack__deps: ['emscripten_get_callstack_js'],
  emscripten_get_callstack: function(flags, str, maxbytes) {
    var callstack = _emscripten_get_callstack_js(flags);
    // User can query the required amount of bytes to hold the callstack.
    if (!str || maxbytes <= 0) {
      return callstack.length+1;
    }
    // Truncate output to avoid writing past bounds.
    if (callstack.length > maxbytes-1) {
      callstack.slice(0, maxbytes-1);
    }
    // Output callstack string as C string to HEAP.
    writeStringToMemory(callstack, str, false);

    // Return number of bytes written.
    return callstack.length+1;
  },

  emscripten_log_js__deps: ['emscripten_get_callstack_js'],
  emscripten_log_js: function(flags, str) {
    if (flags & 24/*EM_LOG_C_STACK | EM_LOG_JS_STACK*/) {
      str = str.replace(/\s+$/, ''); // Ensure the message and the callstack are joined cleanly with exactly one newline.
      str += (str.length > 0 ? '\n' : '') + _emscripten_get_callstack_js(flags);
    }

    if (flags & 1 /*EM_LOG_CONSOLE*/) {
      if (flags & 4 /*EM_LOG_ERROR*/) {
        console.error(str);
      } else if (flags & 2 /*EM_LOG_WARN*/) {
        console.warn(str);
      } else {
        console.log(str);
      }
    } else if (flags & 6 /*EM_LOG_ERROR|EM_LOG_WARN*/) {
      Module.printErr(str);
    } else {
      Module.print(str);
    }
  },

  emscripten_log__deps: ['_formatString', 'emscripten_log_js'],
  emscripten_log: function(flags, varargs) {
    // Extract the (optionally-existing) printf format specifier field from varargs.
    var format = {{{ makeGetValue('varargs', '0', 'i32', undefined, undefined, true) }}};
    varargs += Math.max(Runtime.getNativeFieldSize('i32'), Runtime.getAlignSize('i32', null, true));
    var str = '';
    if (format) {
      var result = __formatString(format, varargs);
      for(var i = 0 ; i < result.length; ++i) {
        str += String.fromCharCode(result[i]);
      }
    }
    _emscripten_log_js(flags, str);
  }
};

mergeInto(LibraryManager.library, LibraryDebugLogging);
