/*SOURCE-HASH:5D26DF6D1D875F71*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
/**
 * Utility functions to work with time values.
 * Time is generally measured in seconds, this class provides functions to convert between different units.
 */
var Time = /** @class */ (function () {
    function Time() {
    }
    /**
     * Returns the current time, independent of game speed.
     * This should be used for UI elements and other things that always advance at real time speeds.
     */
    Time.GetRealTime = function () {
        return __CPP_Time_GetRealTime();
    };
    /**
     * Returns the current game time. This depends on the speed at which the game is simulated (slow motion etc.).
     * This should be used for most game mechanics that should speed up and slow down according to the game speed.
     */
    Time.GetGameTime = function () {
        return __CPP_Time_GetGameTime();
    };
    /**
     * Returns the amount of game time that has passed between this frame and the last frame.
     * This should be used for game mechanics that are updated every single frame and need to change according to
     * how much time has passed since the last frame.
     */
    Time.GetGameTimeDiff = function () {
        return __CPP_Time_GetGameTimeDiff();
    };
    /**
     * Converts nanoseconds to seconds
     */
    Time.Nanoseconds = function (fNanoseconds) {
        return fNanoseconds * 0.000000001;
    };
    /**
     * Converts microseconds to seconds
     */
    Time.Microseconds = function (fMicroseconds) {
        return fMicroseconds * 0.000001;
    };
    /**
     * Converts milliseconds to seconds
     */
    Time.Milliseconds = function (fMilliseconds) {
        return fMilliseconds * 0.001;
    };
    /**
     * Converts seconds to seconds
     */
    Time.Seconds = function (fSeconds) {
        return fSeconds;
    };
    /**
     * Converts minutes to seconds
     */
    Time.Minutes = function (fMinutes) {
        return fMinutes * 60;
    };
    /**
     * Converts hours to seconds
     */
    Time.Hours = function (fHours) {
        return fHours * 60 * 60;
    };
    /**
     * Returns a zero time value
     */
    Time.Zero = function () {
        return 0;
    };
    /**
     * Converts seconds to nanoseconds
     */
    Time.GetNanoseconds = function (Time) {
        return Time * 1000000000.0;
    };
    /**
     * Converts seconds to microseconds
     */
    Time.GetMicroseconds = function (Time) {
        return Time * 1000000.0;
    };
    /**
     * Converts seconds to milliseconds
     */
    Time.GetMilliseconds = function (Time) {
        return Time * 1000.0;
    };
    /**
     * Converts seconds to seconds
     */
    Time.GetSeconds = function (Time) {
        return Time;
    };
    /**
     * Converts seconds to minutes
     */
    Time.GetMinutes = function (Time) {
        return Time / 60.0;
    };
    /**
     * Converts seconds to hours
     */
    Time.GetHours = function (Time) {
        return Time / (60.0 * 60.0);
    };
    return Time;
}());
exports.Time = Time;
