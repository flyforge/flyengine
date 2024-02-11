/*SOURCE-HASH:BA29252FDF51BA69*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var __Utils = require("./Utils");
exports.Utils = __Utils.Utils;
var __Vec3 = require("./Vec3");
exports.Vec3 = __Vec3.Vec3;
/**
 * A two-component vector type.
 */
var Vec2 = /** @class */ (function () {
    // TODO:
    // GetAngleBetween
    /**
     * Default initializes the vector to all zero.
     */
    function Vec2(x, y) {
        if (x === void 0) { x = 0; }
        if (y === void 0) { y = 0; }
        this.x = x;
        this.y = y;
    }
    /**
     * Returns a duplicate of this vector.
     */
    Vec2.prototype.Clone = function () {
        return new Vec2(this.x, this.y);
    };
    /**
     * Returns a duplicate of this as a Vec3, with the provided value as z.
     */
    Vec2.prototype.CloneAsVec3 = function (z) {
        if (z === void 0) { z = 0; }
        return new exports.Vec3(this.x, this.y, z);
    };
    /**
     * Returns a vector with all components set to zero.
     */
    Vec2.ZeroVector = function () {
        return new Vec2(0, 0);
    };
    /**
     * Returns a vector with all components set to one.
     */
    Vec2.OneVector = function () {
        return new Vec2(1, 1);
    };
    /**
     * Returns the vector (1, 0)
     */
    Vec2.UnitAxisX = function () {
        return new Vec2(1, 0);
    };
    /**
     * Returns the vector (0, 1)
     */
    Vec2.UnitAxisY = function () {
        return new Vec2(0, 1);
    };
    /**
     * Sets all components to the given values.
     */
    Vec2.prototype.Set = function (x, y) {
        this.x = x;
        this.y = y;
    };
    /**
     * Copies all component values from rhs.
     */
    Vec2.prototype.SetVec2 = function (rhs) {
        this.x = rhs.x;
        this.y = rhs.y;
    };
    /**
     * Sets all components to the value 'val'.
     */
    Vec2.prototype.SetAll = function (val) {
        this.x = val;
        this.y = val;
    };
    /**
     * Sets all components to zero.
     */
    Vec2.prototype.SetZero = function () {
        this.x = 0;
        this.y = 0;
    };
    /**
     * Returns the squared length of the vector.
     */
    Vec2.prototype.GetLengthSquared = function () {
        return this.x * this.x + this.y * this.y;
    };
    /**
     * Returns the length of the vector.
     */
    Vec2.prototype.GetLength = function () {
        return Math.sqrt(this.x * this.x + this.y * this.y);
    };
    /**
     * Computes and returns the length of the vector, and normalizes the vector.
     * This is more efficient than calling GetLength() followed by Normalize().
     */
    Vec2.prototype.GetLengthAndNormalize = function () {
        var length = this.GetLength();
        var invLength = 1.0 / length;
        this.x *= invLength;
        this.y *= invLength;
        return length;
    };
    /**
     * Normalizes the vector. Afterwards its length will be one.
     * This only works on non-zero vectors. Calling Normalize() on a zero-vector is an error.
     */
    Vec2.prototype.Normalize = function () {
        var invLength = 1.0 / this.GetLength();
        this.x *= invLength;
        this.y *= invLength;
    };
    /**
     * Returns a normalized duplicate of this vector.
     * Calling this on a zero-vector is an error.
     */
    Vec2.prototype.GetNormalized = function () {
        var norm = this.Clone();
        norm.Normalize();
        return norm;
    };
    /**
     * Normalizes the vector as long as it is not a zero-vector (within the given epsilon).
     * If it is determined to be too close to zero, it is set to 'fallback'.
     *
     * @param fallback The value to use in case 'this' is too close to zero.
     * @param epsilon The epsilon within this vector is considered to be a zero-vector.
     * @returns true if the vector was normalized regularly, false if the vector was close to zero and 'fallback' was used instead.
     */
    Vec2.prototype.NormalizeIfNotZero = function (fallback, epsilon) {
        if (fallback === void 0) { fallback = new Vec2(1, 0); }
        if (epsilon === void 0) { epsilon = 0.000001; }
        var length = this.GetLength();
        if (length >= -epsilon && length <= epsilon) {
            this.SetVec2(fallback);
            return false;
        }
        this.DivNumber(length);
        return true;
    };
    /**
     * Checks whether all components of this are close to zero.
     */
    Vec2.prototype.IsZero = function (epsilon) {
        if (epsilon === void 0) { epsilon = 0; }
        if (epsilon != 0) {
            return this.x >= -epsilon && this.x <= epsilon &&
                this.y >= -epsilon && this.y <= epsilon;
        }
        else {
            return this.x == 0 && this.y == 0;
        }
    };
    /**
     * Checks whether this is normalized within some epsilon.
     */
    Vec2.prototype.IsNormalized = function (epsilon) {
        if (epsilon === void 0) { epsilon = 0.001; }
        var length = this.GetLength();
        return (length >= 1.0 - epsilon) && (length <= 1.0 + epsilon);
    };
    /**
     * Returns a negated duplicate of this.
     */
    Vec2.prototype.GetNegated = function () {
        return new Vec2(-this.x, -this.y);
    };
    /**
     * Negates all components of this.
     */
    Vec2.prototype.Negate = function () {
        this.x = -this.x;
        this.y = -this.y;
    };
    /**
     * Adds rhs component-wise to this.
     */
    Vec2.prototype.AddVec2 = function (rhs) {
        this.x += rhs.x;
        this.y += rhs.y;
    };
    /**
     * Subtracts rhs component-wise from this.
     */
    Vec2.prototype.SubVec2 = function (rhs) {
        this.x -= rhs.x;
        this.y -= rhs.y;
    };
    /**
     * Multiplies rhs component-wise into this.
     */
    Vec2.prototype.MulVec2 = function (rhs) {
        this.x *= rhs.x;
        this.y *= rhs.y;
    };
    /**
     * Divides each component of this by rhs.
     */
    Vec2.prototype.DivVec2 = function (rhs) {
        this.x /= rhs.x;
        this.y /= rhs.y;
    };
    /**
     * Multiplies all components of this by 'val'.
     */
    Vec2.prototype.MulNumber = function (val) {
        this.x *= val;
        this.y *= val;
    };
    /**
     * Divides all components of this by 'val'.
     */
    Vec2.prototype.DivNumber = function (val) {
        var invVal = 1.0 / val;
        this.x *= invVal;
        this.y *= invVal;
    };
    /**
     * Checks whether this and rhs are exactly identical.
     */
    Vec2.prototype.IsIdentical = function (rhs) {
        return this.x == rhs.x && this.y == rhs.y;
    };
    /**
     * Checks whether this and rhs are approximately equal within a given epsilon.
     */
    Vec2.prototype.IsEqual = function (rhs, epsilon) {
        return (this.x >= rhs.x - epsilon && this.x <= rhs.x + epsilon) &&
            (this.y >= rhs.y - epsilon && this.y <= rhs.y + epsilon);
    };
    /**
     * Returns the dot-product between this and rhs.
     */
    Vec2.prototype.Dot = function (rhs) {
        return this.x * rhs.x + this.y * rhs.y;
    };
    /**
     * Returns a vector consisting of the minimum of the respective components of this and rhs.
     */
    Vec2.prototype.GetCompMin = function (rhs) {
        return new Vec2(Math.min(this.x, rhs.x), Math.min(this.y, rhs.y));
    };
    /**
     * Returns a vector consisting of the maximum of the respective components of this and rhs.
     */
    Vec2.prototype.GetCompMax = function (rhs) {
        return new Vec2(Math.max(this.x, rhs.x), Math.max(this.y, rhs.y));
    };
    /**
     * Returns a vector where each component is set to this component's value, clamped to the respective low and high value.
     */
    Vec2.prototype.GetCompClamp = function (low, high) {
        var _x = Math.max(low.x, Math.min(high.x, this.x));
        var _y = Math.max(low.y, Math.min(high.y, this.y));
        return new Vec2(_x, _y);
    };
    /**
     * Returns a vector with each component being the product of this and rhs.
     */
    Vec2.prototype.GetCompMul = function (rhs) {
        return new Vec2(this.x * rhs.x, this.y * rhs.y);
    };
    /**
     * Returns a vector with each component being the division of this and rhs.
     */
    Vec2.prototype.GetCompDiv = function (rhs) {
        return new Vec2(this.x / rhs.x, this.y / rhs.y);
    };
    /**
     * Returns a vector with each component set to the absolute value of this vector's respective component.
     */
    Vec2.prototype.GetAbs = function () {
        return new Vec2(Math.abs(this.x), Math.abs(this.y));
    };
    /**
     * Sets this vector's components to the absolute value of lhs's respective components.
     */
    Vec2.prototype.SetAbs = function (lhs) {
        this.x = Math.abs(lhs.x);
        this.y = Math.abs(lhs.y);
    };
    /**
     * Returns a vector that is this vector reflected along the given normal.
     */
    Vec2.prototype.GetReflectedVector = function (normal) {
        var res = this.Clone();
        var tmp = normal.Clone();
        tmp.MulNumber(this.Dot(normal) * 2.0);
        res.SubVec2(tmp);
        return res;
    };
    /**
     * Sets this vector to be the addition of lhs and rhs.
     */
    Vec2.prototype.SetAdd = function (lhs, rhs) {
        this.x = lhs.x + rhs.x;
        this.y = lhs.y + rhs.y;
    };
    /**
     * Sets this vector to be the subtraction of lhs and rhs.
     */
    Vec2.prototype.SetSub = function (lhs, rhs) {
        this.x = lhs.x - rhs.x;
        this.y = lhs.y - rhs.y;
    };
    /**
     * Sets this vector to be the product of lhs and rhs.
     */
    Vec2.prototype.SetMul = function (lhs, rhs) {
        this.x = lhs.x * rhs;
        this.y = lhs.y * rhs;
    };
    /**
     * Sets this vector to be the division of lhs and rhs.
     */
    Vec2.prototype.SetDiv = function (lhs, rhs) {
        var invRhs = 1.0 / rhs;
        this.x = lhs.x * invRhs;
        this.y = lhs.y * invRhs;
    };
    /**
     * Returns a random point inside a circle of radius 1 around the origin.
     */
    Vec2.CreateRandomPointInCircle = function () {
        var px, py;
        var len = 0.0;
        do {
            px = Math.random() * 2.0 - 1.0;
            py = Math.random() * 2.0 - 1.0;
            len = (px * px) + (py * py);
        } while (len > 1.0 || len <= 0.000001); // prevent the exact center
        return new Vec2(px, py);
    };
    /**
     * Returns a random direction vector.
     */
    Vec2.CreateRandomDirection = function () {
        var res = Vec2.CreateRandomPointInCircle();
        res.Normalize();
        return res;
    };
    return Vec2;
}());
exports.Vec2 = Vec2;
