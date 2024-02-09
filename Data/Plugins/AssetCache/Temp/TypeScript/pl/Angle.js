/*SOURCE-HASH:10CD85503485C61E*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
/**
 * Helper functions for working with angles.
 */
var Angle = /** @class */ (function () {
    function Angle() {
    }
    /**
     * Converts an angle from degree to radians.
     *
     * @param degree The angle in degree.
     * @returns The angle in radians.
     */
    Angle.DegreeToRadian = function (degree) {
        return degree * Math.PI / 180.0;
    };
    /**
     * Converts an angle from radians to degree.
     *
     * @param radians The angle in radians.
     * @returns The angle in degree.
     */
    Angle.RadianToDegree = function (radians) {
        return radians * 180.0 / Math.PI;
    };
    /**
     * Computes the angle between two angles
     *
     * @param radianA The first angle in radians.
     * @param radianB The second angle in radians.
     */
    Angle.AngleBetween = function (radianA, radianB) {
        return Math.PI - Math.abs(Math.abs(radianA - radianB) - Math.PI);
    };
    /**
     * Checks whether two angles are approximately equal. Multiples of 2 Pi are not considered to be equal.
     *
     * @param radianLhs The first angle in radians.
     * @param radianRhs The second angle in radians.
     * @param epsilon Epsilon in radians in which the two angles are considered equal.
     */
    Angle.IsEqualSimple = function (radianLhs, radianRhs, epsilon) {
        var diff = Angle.AngleBetween(radianLhs, radianRhs);
        return (diff >= -epsilon) && (diff <= epsilon);
    };
    return Angle;
}());
exports.Angle = Angle;
