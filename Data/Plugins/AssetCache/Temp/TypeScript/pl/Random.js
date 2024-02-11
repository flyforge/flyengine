/*SOURCE-HASH:7C27B358C8C679C7*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
/**
 * Functions for generating random numbers.
 */
var Random;
(function (Random) {
    /**
     * Returns a random boolean value.
     */
    function Bool() {
        return __CPP_RNG_Bool();
    }
    Random.Bool = Bool;
    /**
     * Returns a number in the range [0; 1).
     */
    function DoubleZeroToOneExclusive() {
        return __CPP_RNG_DoubleZeroToOneExclusive();
    }
    Random.DoubleZeroToOneExclusive = DoubleZeroToOneExclusive;
    /**
     * Returns a number in the range [0; 1].
     */
    function DoubleZeroToOneInclusive() {
        return __CPP_RNG_DoubleZeroToOneInclusive();
    }
    Random.DoubleZeroToOneInclusive = DoubleZeroToOneInclusive;
    /**
     * Returns a positive integer number.
     */
    function UIntInRange(range) {
        return __CPP_RNG_UIntInRange(range);
    }
    Random.UIntInRange = UIntInRange;
    /**
     * Returns a double value between [-fAbsMaxValue; +fAbsMaxValue] with a Gaussian distribution.
     */
    function DoubleVarianceAroundZero(fAbsMaxValue) {
        return __CPP_RNG_DoubleVarianceAroundZero(fAbsMaxValue);
    }
    Random.DoubleVarianceAroundZero = DoubleVarianceAroundZero;
    /**
     * Returns an int32 value in range [minValue ; minValue + range - 1]
     *
     * A range of 0 is invalid and will assert! It also has no mathematical meaning. A range of 1 already means "between 0 and 1 EXCLUDING 1".
     * So always use a range of at least 1.
     */
    function IntInRange(minValue, range) {
        return __CPP_RNG_IntInRange(minValue, range);
    }
    Random.IntInRange = IntInRange;
    /**
     * Returns an int32 value in range [minValue ; maxValue]
     */
    function IntMinMax(minValue, maxValue) {
        return __CPP_RNG_IntMinMax(minValue, maxValue);
    }
    Random.IntMinMax = IntMinMax;
    /**
     * Returns a number in range [minValue ; minValue + range)
     */
    function DoubleInRange(minValue, range) {
        return __CPP_RNG_DoubleInRange(minValue, range);
    }
    Random.DoubleInRange = DoubleInRange;
    /**
     * Returns a number in range [minValue ; maxValue]
     */
    function DoubleMinMax(minValue, maxValue) {
        return __CPP_RNG_DoubleMinMax(minValue, maxValue);
    }
    Random.DoubleMinMax = DoubleMinMax;
    /**
     * Returns a number around 'value'' with a given variance (0 - 1 range)
     */
    function DoubleVariance(value, variance) {
        return __CPP_RNG_DoubleVariance(value, variance);
    }
    Random.DoubleVariance = DoubleVariance;
})(Random = exports.Random || (exports.Random = {}));
