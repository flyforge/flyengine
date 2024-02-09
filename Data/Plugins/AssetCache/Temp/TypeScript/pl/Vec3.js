/*SOURCE-HASH:3085A8B49BE633F2*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var __Utils = require("./Utils");
exports.Utils = __Utils.Utils;
var __Angle = require("./Angle");
exports.Angle = __Angle.Angle;
var __Vec2 = require("./Vec2");
exports.Vec2 = __Vec2.Vec2;
/**
 * A three-component vector type.
 */
var Vec3 = /** @class */ (function () {
    // TODO:
    // GetAngleBetween
    // CreateRandomDeviationX/Y/Z/Normal
    /**
     * Default initializes the vector to all zero.
     */
    function Vec3(x, y, z) {
        if (x === void 0) { x = 0; }
        if (y === void 0) { y = 0; }
        if (z === void 0) { z = 0; }
        this.x = x;
        this.y = y;
        this.z = z;
    }
    /**
     * Returns a duplicate of this vector.
     */
    Vec3.prototype.Clone = function () {
        return new Vec3(this.x, this.y, this.z);
    };
    /**
     * Returns a duplicate Vec2 with the z component removed.
     */
    Vec3.prototype.CloneAsVec2 = function () {
        return new exports.Vec2(this.x, this.y);
    };
    /**
     * Returns a vector with all components set to zero.
     */
    Vec3.ZeroVector = function () {
        return new Vec3(0, 0, 0);
    };
    /**
     * Returns a vector with all components set to one.
     */
    Vec3.OneVector = function () {
        return new Vec3(1, 1, 1);
    };
    /**
     * Returns the vector (1, 0, 0)
     */
    Vec3.UnitAxisX = function () {
        return new Vec3(1, 0, 0);
    };
    /**
     * Returns the vector (0, 1, 0)
     */
    Vec3.UnitAxisY = function () {
        return new Vec3(0, 1, 0);
    };
    /**
     * Returns the vector (0, 0, 1)
     */
    Vec3.UnitAxisZ = function () {
        return new Vec3(0, 0, 1);
    };
    /**
     * Sets all components to the given values.
     */
    Vec3.prototype.Set = function (x, y, z) {
        this.x = x;
        this.y = y;
        this.z = z;
    };
    /**
     * Copies all component values from rhs.
     */
    Vec3.prototype.SetVec3 = function (rhs) {
        this.x = rhs.x;
        this.y = rhs.y;
        this.z = rhs.z;
    };
    /**
     * Sets all components to the value 'val'.
     */
    Vec3.prototype.SetAll = function (val) {
        this.x = val;
        this.y = val;
        this.z = val;
    };
    /**
     * Sets all components to zero.
     */
    Vec3.prototype.SetZero = function () {
        this.x = 0;
        this.y = 0;
        this.z = 0;
    };
    /**
     * Returns the squared length of the vector.
     */
    Vec3.prototype.GetLengthSquared = function () {
        return this.x * this.x + this.y * this.y + this.z * this.z;
    };
    /**
     * Returns the length of the vector.
     */
    Vec3.prototype.GetLength = function () {
        return Math.sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
    };
    /**
     * Computes and returns the length of the vector, and normalizes the vector.
     * This is more efficient than calling GetLength() followed by Normalize().
     */
    Vec3.prototype.GetLengthAndNormalize = function () {
        var length = this.GetLength();
        var invLength = 1.0 / length;
        this.x *= invLength;
        this.y *= invLength;
        this.z *= invLength;
        return length;
    };
    /**
     * Normalizes the vector. Afterwards its length will be one.
     * This only works on non-zero vectors. Calling Normalize() on a zero-vector is an error.
     */
    Vec3.prototype.Normalize = function () {
        var invLength = 1.0 / this.GetLength();
        this.x *= invLength;
        this.y *= invLength;
        this.z *= invLength;
    };
    /**
     * Returns a normalized duplicate of this vector.
     * Calling this on a zero-vector is an error.
     */
    Vec3.prototype.GetNormalized = function () {
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
    Vec3.prototype.NormalizeIfNotZero = function (fallback, epsilon) {
        if (fallback === void 0) { fallback = new Vec3(1, 0, 0); }
        if (epsilon === void 0) { epsilon = 0.000001; }
        var length = this.GetLength();
        if (length >= -epsilon && length <= epsilon) {
            this.SetVec3(fallback);
            return false;
        }
        this.DivNumber(length);
        return true;
    };
    /**
     * Checks whether all components of this are close to zero.
     */
    Vec3.prototype.IsZero = function (epsilon) {
        if (epsilon === void 0) { epsilon = 0; }
        if (epsilon != 0) {
            return this.x >= -epsilon && this.x <= epsilon &&
                this.y >= -epsilon && this.y <= epsilon &&
                this.z >= -epsilon && this.z <= epsilon;
        }
        else {
            return this.x == 0 && this.y == 0 && this.z == 0;
        }
    };
    /**
     * Checks whether this is normalized within some epsilon.
     */
    Vec3.prototype.IsNormalized = function (epsilon) {
        if (epsilon === void 0) { epsilon = 0.001; }
        var length = this.GetLength();
        return (length >= 1.0 - epsilon) && (length <= 1.0 + epsilon);
    };
    /**
     * Returns a negated duplicate of this.
     */
    Vec3.prototype.GetNegated = function () {
        return new Vec3(-this.x, -this.y, -this.z);
    };
    /**
     * Negates all components of this.
     */
    Vec3.prototype.Negate = function () {
        this.x = -this.x;
        this.y = -this.y;
        this.z = -this.z;
    };
    /**
     * Adds rhs component-wise to this.
     */
    Vec3.prototype.AddVec3 = function (rhs) {
        this.x += rhs.x;
        this.y += rhs.y;
        this.z += rhs.z;
    };
    /**
     * Subtracts rhs component-wise from this.
     */
    Vec3.prototype.SubVec3 = function (rhs) {
        this.x -= rhs.x;
        this.y -= rhs.y;
        this.z -= rhs.z;
    };
    /**
     * Multiplies rhs component-wise into this.
     */
    Vec3.prototype.MulVec3 = function (rhs) {
        this.x *= rhs.x;
        this.y *= rhs.y;
        this.z *= rhs.z;
    };
    /**
     * Divides each component of this by rhs.
     */
    Vec3.prototype.DivVec3 = function (rhs) {
        this.x /= rhs.x;
        this.y /= rhs.y;
        this.z /= rhs.z;
    };
    /**
     * Multiplies all components of this by 'val'.
     */
    Vec3.prototype.MulNumber = function (val) {
        this.x *= val;
        this.y *= val;
        this.z *= val;
    };
    /**
     * Divides all components of this by 'val'.
     */
    Vec3.prototype.DivNumber = function (val) {
        var invVal = 1.0 / val;
        this.x *= invVal;
        this.y *= invVal;
        this.z *= invVal;
    };
    /**
     * Checks whether this and rhs are exactly identical.
     */
    Vec3.prototype.IsIdentical = function (rhs) {
        return this.x == rhs.x && this.y == rhs.y && this.z == rhs.z;
    };
    /**
     * Checks whether this and rhs are approximately equal within a given epsilon.
     */
    Vec3.prototype.IsEqual = function (rhs, epsilon) {
        if (epsilon === void 0) { epsilon = 0.0001; }
        return (this.x >= rhs.x - epsilon && this.x <= rhs.x + epsilon) &&
            (this.y >= rhs.y - epsilon && this.y <= rhs.y + epsilon) &&
            (this.z >= rhs.z - epsilon && this.z <= rhs.z + epsilon);
    };
    /**
     * Returns the dot-product between this and rhs.
     */
    Vec3.prototype.Dot = function (rhs) {
        return this.x * rhs.x + this.y * rhs.y + this.z * rhs.z;
    };
    /**
     * Returns the cross-product between this and rhs.
     */
    Vec3.prototype.CrossRH = function (rhs) {
        return new Vec3(this.y * rhs.z - this.z * rhs.y, this.z * rhs.x - this.x * rhs.z, this.x * rhs.y - this.y * rhs.x);
    };
    /**
     * Sets this to be the cross product between lhs and rhs.
     */
    Vec3.prototype.SetCrossRH = function (lhs, rhs) {
        this.x = lhs.y * rhs.z - lhs.z * rhs.y;
        this.y = lhs.z * rhs.x - lhs.x * rhs.z;
        this.z = lhs.x * rhs.y - lhs.y * rhs.x;
    };
    /**
     * Returns a vector consisting of the minimum of the respective components of this and rhs.
     */
    Vec3.prototype.GetCompMin = function (rhs) {
        return new Vec3(Math.min(this.x, rhs.x), Math.min(this.y, rhs.y), Math.min(this.z, rhs.z));
    };
    /**
     * Returns a vector consisting of the maximum of the respective components of this and rhs.
     */
    Vec3.prototype.GetCompMax = function (rhs) {
        return new Vec3(Math.max(this.x, rhs.x), Math.max(this.y, rhs.y), Math.max(this.z, rhs.z));
    };
    /**
     * Returns a vector where each component is set to this component's value, clamped to the respective low and high value.
     */
    Vec3.prototype.GetCompClamp = function (low, high) {
        var _x = Math.max(low.x, Math.min(high.x, this.x));
        var _y = Math.max(low.y, Math.min(high.y, this.y));
        var _z = Math.max(low.z, Math.min(high.z, this.z));
        return new Vec3(_x, _y, _z);
    };
    /**
     * Returns a vector with each component being the product of this and rhs.
     */
    Vec3.prototype.GetCompMul = function (rhs) {
        return new Vec3(this.x * rhs.x, this.y * rhs.y, this.z * rhs.z);
    };
    /**
     * Returns a vector with each component being the division of this and rhs.
     */
    Vec3.prototype.GetCompDiv = function (rhs) {
        return new Vec3(this.x / rhs.x, this.y / rhs.y, this.z / rhs.z);
    };
    /**
     * Returns a vector with each component set to the absolute value of this vector's respective component.
     */
    Vec3.prototype.GetAbs = function () {
        return new Vec3(Math.abs(this.x), Math.abs(this.y), Math.abs(this.z));
    };
    /**
     * Sets this vector's components to the absolute value of lhs's respective components.
     */
    Vec3.prototype.SetAbs = function (lhs) {
        this.x = Math.abs(lhs.x);
        this.y = Math.abs(lhs.y);
        this.z = Math.abs(lhs.z);
    };
    /**
     * Sets this vector to be the normal of the plane formed by the given three points in space.
     * The points are expected to be in counter-clockwise order to define the 'front' of a triangle.
     * If the points are given in clockwise order, the normal will point in the opposite direction.
     * The points must form a proper triangle, if they are degenerate, the calculation may fail.
     *
     * @returns true when the normal could be calculated successfully.
     */
    Vec3.prototype.CalculateNormal = function (v1, v2, v3) {
        var tmp1 = new Vec3();
        tmp1.SetSub(v3, v2);
        var tmp2 = new Vec3();
        tmp2.SetSub(v1, v2);
        this.SetCrossRH(tmp1, tmp2);
        return this.NormalizeIfNotZero();
    };
    /**
     * Adjusts this vector such that it is orthogonal to the given normal.
     * The operation may change the length of this vector.
     */
    Vec3.prototype.MakeOrthogonalTo = function (normal) {
        var ortho = normal.CrossRH(this);
        this.SetCrossRH(ortho, normal);
    };
    /**
     * Returns an arbitrary vector that is orthogonal to this vector.
     */
    Vec3.prototype.GetOrthogonalVector = function () {
        if (Math.abs(this.y) < 0.999) {
            return this.CrossRH(new Vec3(0, 1, 0));
        }
        return this.CrossRH(new Vec3(1, 0, 0));
    };
    /**
     * Returns a vector that is this vector reflected along the given normal.
     */
    Vec3.prototype.GetReflectedVector = function (normal) {
        var res = this.Clone();
        var tmp = normal.Clone();
        tmp.MulNumber(this.Dot(normal) * 2.0);
        res.SubVec3(tmp);
        return res;
    };
    /**
     * Sets this vector to be the addition of lhs and rhs.
     */
    Vec3.prototype.SetAdd = function (lhs, rhs) {
        this.x = lhs.x + rhs.x;
        this.y = lhs.y + rhs.y;
        this.z = lhs.z + rhs.z;
    };
    /**
     * Sets this vector to be the subtraction of lhs and rhs.
     */
    Vec3.prototype.SetSub = function (lhs, rhs) {
        this.x = lhs.x - rhs.x;
        this.y = lhs.y - rhs.y;
        this.z = lhs.z - rhs.z;
    };
    /**
     * Sets this vector to be the product of lhs and rhs.
     */
    Vec3.prototype.SetMul = function (lhs, rhs) {
        this.x = lhs.x * rhs;
        this.y = lhs.y * rhs;
        this.z = lhs.z * rhs;
    };
    /**
     * Sets this vector to be the division of lhs and rhs.
     */
    Vec3.prototype.SetDiv = function (lhs, rhs) {
        var invRhs = 1.0 / rhs;
        this.x = lhs.x * invRhs;
        this.y = lhs.y * invRhs;
        this.z = lhs.z * invRhs;
    };
    /**
     * Attempts to modify this vector such that it has the desired length.
     * Requires that this vector is not zero.
     *
     * @returns true if the vector's length could be changed as desired.
     */
    Vec3.prototype.SetLength = function (length, epsilon) {
        if (!this.NormalizeIfNotZero(Vec3.ZeroVector(), epsilon))
            return false;
        this.MulNumber(length);
        return true;
    };
    /**
     * Sets this vector to the linear interpolation between lhs and rhs.
     * @param lerpFactor Factor between 0 and 1 that specifies how much to interpolate.
     */
    Vec3.prototype.SetLerp = function (lhs, rhs, lerpFactor) {
        this.SetSub(rhs, lhs);
        this.MulNumber(lerpFactor);
        this.AddVec3(lhs);
    };
    /**
     * Returns a random point inside a sphere of radius 1 around the origin.
     */
    Vec3.CreateRandomPointInSphere = function () {
        var px, py, pz;
        var len = 0.0;
        do {
            px = Math.random() * 2.0 - 1.0;
            py = Math.random() * 2.0 - 1.0;
            pz = Math.random() * 2.0 - 1.0;
            len = (px * px) + (py * py) + (pz * pz);
        } while (len > 1.0 || len <= 0.000001); // prevent the exact center
        return new Vec3(px, py, pz);
    };
    /**
     * Returns a random direction vector.
     */
    Vec3.CreateRandomDirection = function () {
        var res = Vec3.CreateRandomPointInSphere();
        res.Normalize();
        return res;
    };
    return Vec3;
}());
exports.Vec3 = Vec3;
