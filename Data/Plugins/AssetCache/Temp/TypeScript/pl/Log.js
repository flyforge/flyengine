/*SOURCE-HASH:3FD6550B9D0FD1ED*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
/**
 * Text logging functionality.
 */
var Log;
(function (Log) {
    /**
     * Logs the given text as an error.
     */
    function Error(text) { __CPP_Log_Error(text); }
    Log.Error = Error;
    /**
     * Logs the given text as an important warning.
     */
    function SeriousWarning(text) { __CPP_Log_SeriousWarning(text); }
    Log.SeriousWarning = SeriousWarning;
    /**
     * Logs the given text as a warning.
     */
    function Warning(text) { __CPP_Log_Warning(text); }
    Log.Warning = Warning;
    /**
     * Logs the given text as a success message.
     */
    function Success(text) { __CPP_Log_Success(text); }
    Log.Success = Success;
    /**
     * Logs the given text as an info message.
     */
    function Info(text) { __CPP_Log_Info(text); }
    Log.Info = Info;
    /**
     * Logs the given text as a message for developers. Will not be visible in Release builds.
     */
    function Dev(text) { __CPP_Log_Dev(text); }
    Log.Dev = Dev;
    /**
     * Logs the given text as a (verbose) debug message for developers. Will not be visible in Release builds.
     */
    function Debug(text) { __CPP_Log_Debug(text); }
    Log.Debug = Debug;
})(Log = exports.Log || (exports.Log = {}));
;
