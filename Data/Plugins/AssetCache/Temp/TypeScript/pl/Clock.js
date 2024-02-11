/*SOURCE-HASH:19FF4A913DB3004B*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
/**
 * Functions to work with the game clock.
 */
var Clock;
(function (Clock) {
    /**
     * Changes the speed at which the clock's time advances. Default is 1.0.
     *
     * @param speed The speed of time. 1.0 for real time, smaller values for 'slow motion', higher values for 'fast forward'.
     */
    function SetClockSpeed(speed) {
        __CPP_Clock_SetSpeed(speed);
    }
    Clock.SetClockSpeed = SetClockSpeed;
    /**
     * Returns the current speed of the clock. See SetClockSpeed().
     */
    function GetClockSpeed() {
        return __CPP_Clock_GetSpeed();
    }
    Clock.GetClockSpeed = GetClockSpeed;
    /**
     * Returns the elapsed time in seconds since the last game step.
     */
    function GetTimeDiff() {
        return __CPP_Clock_GetTimeDiff();
    }
    Clock.GetTimeDiff = GetTimeDiff;
    /**
     * Returns the accumulated time in seconds since the game simulation started.
     */
    function GetAccumulatedTime() {
        return __CPP_Clock_GetAccumulatedTime();
    }
    Clock.GetAccumulatedTime = GetAccumulatedTime;
})(Clock = exports.Clock || (exports.Clock = {}));
