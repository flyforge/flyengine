/*SOURCE-HASH:4449EDE8F0C99243*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var __Utils = require("./Utils");
exports.Utils = __Utils.Utils;
var __Mat3 = require("./Mat3");
exports.Mat3 = __Mat3.Mat3;
var __Vec3 = require("./Vec3");
exports.Vec3 = __Vec3.Vec3;
/**
 * A 4x4 matrix.
 */
var Mat4 = /** @class */ (function () {
    /**
     * By default the constructor will initialize the matrix to identity.
     */
    function Mat4(c1r1, c2r1, c3r1, c4r1, c1r2, c2r2, c3r2, c4r2, c1r3, c2r3, c3r3, c4r3, c1r4, c2r4, c3r4, c4r4) {
        if (c1r1 === void 0) { c1r1 = 1; }
        if (c2r1 === void 0) { c2r1 = 0; }
        if (c3r1 === void 0) { c3r1 = 0; }
        if (c4r1 === void 0) { c4r1 = 0; }
        if (c1r2 === void 0) { c1r2 = 0; }
        if (c2r2 === void 0) { c2r2 = 1; }
        if (c3r2 === void 0) { c3r2 = 0; }
        if (c4r2 === void 0) { c4r2 = 0; }
        if (c1r3 === void 0) { c1r3 = 0; }
        if (c2r3 === void 0) { c2r3 = 0; }
        if (c3r3 === void 0) { c3r3 = 1; }
        if (c4r3 === void 0) { c4r3 = 0; }
        if (c1r4 === void 0) { c1r4 = 0; }
        if (c2r4 === void 0) { c2r4 = 0; }
        if (c3r4 === void 0) { c3r4 = 0; }
        if (c4r4 === void 0) { c4r4 = 1; }
        this.m_ElementsCM = [
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        ];
        this.SetElements(c1r1, c2r1, c3r1, c4r1, c1r2, c2r2, c3r2, c4r2, c1r3, c2r3, c3r3, c4r3, c1r4, c2r4, c3r4, c4r4);
    }
    /**
     * Returns a duplicate of this matrix.
     */
    Mat4.prototype.Clone = function () {
        var c = new Mat4();
        c.SetMat4(this);
        return c;
    };
    /**
     * Returns a duplicate of this matrices' 3x3 sub-matrix.
     */
    Mat4.prototype.CloneAsMat3 = function () {
        return new exports.Mat3(this.m_ElementsCM[0], this.m_ElementsCM[4], this.m_ElementsCM[8], this.m_ElementsCM[1], this.m_ElementsCM[5], this.m_ElementsCM[9], this.m_ElementsCM[2], this.m_ElementsCM[6], this.m_ElementsCM[10]);
    };
    /**
     * Copies the values of m into this.
     */
    Mat4.prototype.SetMat4 = function (m) {
        for (var i = 0; i < 16; ++i) {
            this.m_ElementsCM[i] = m.m_ElementsCM[i];
        }
    };
    /**
     * Returns the value from the requested row and column.
     */
    Mat4.prototype.GetElement = function (column, row) {
        return this.m_ElementsCM[column * 4 + row];
    };
    /**
     * Overwrites the value in the given row and column.
     */
    Mat4.prototype.SetElement = function (column, row, value) {
        this.m_ElementsCM[column * 4 + row] = value;
    };
    /**
     * Sets all values of this matrix.
     */
    Mat4.prototype.SetElements = function (c1r1, c2r1, c3r1, c4r1, c1r2, c2r2, c3r2, c4r2, c1r3, c2r3, c3r3, c4r3, c1r4, c2r4, c3r4, c4r4) {
        this.m_ElementsCM[0] = c1r1;
        this.m_ElementsCM[4] = c2r1;
        this.m_ElementsCM[8] = c3r1;
        this.m_ElementsCM[12] = c4r1;
        this.m_ElementsCM[1] = c1r2;
        this.m_ElementsCM[5] = c2r2;
        this.m_ElementsCM[9] = c3r2;
        this.m_ElementsCM[13] = c4r2;
        this.m_ElementsCM[2] = c1r3;
        this.m_ElementsCM[6] = c2r3;
        this.m_ElementsCM[10] = c3r3;
        this.m_ElementsCM[14] = c4r3;
        this.m_ElementsCM[3] = c1r4;
        this.m_ElementsCM[7] = c2r4;
        this.m_ElementsCM[11] = c3r4;
        this.m_ElementsCM[15] = c4r4;
    };
    /**
     * Sets all values to zero.
     */
    Mat4.prototype.SetZero = function () {
        for (var i = 0; i < 16; ++i) {
            this.m_ElementsCM[i] = 0;
        }
    };
    /**
     * Sets the matrix to be the identity matrix.
     */
    Mat4.prototype.SetIdentity = function () {
        this.m_ElementsCM[0] = 1;
        this.m_ElementsCM[4] = 0;
        this.m_ElementsCM[8] = 0;
        this.m_ElementsCM[12] = 0;
        this.m_ElementsCM[1] = 0;
        this.m_ElementsCM[5] = 1;
        this.m_ElementsCM[9] = 0;
        this.m_ElementsCM[13] = 0;
        this.m_ElementsCM[2] = 0;
        this.m_ElementsCM[6] = 0;
        this.m_ElementsCM[10] = 1;
        this.m_ElementsCM[14] = 0;
        this.m_ElementsCM[3] = 0;
        this.m_ElementsCM[7] = 0;
        this.m_ElementsCM[11] = 0;
        this.m_ElementsCM[15] = 1;
    };
    /**
     * Sets this matrix to represent a translation.
     * @param translation How much the matrix translates along x, y and z.
     */
    Mat4.prototype.SetTranslationMatrix = function (translation) {
        this.SetElements(1, 0, 0, translation.x, 0, 1, 0, translation.y, 0, 0, 1, translation.z, 0, 0, 0, 1);
    };
    /**
     * Sets this matrix to be a scaling matrix
     * @param scale How much the matrix scales along x, y and z.
     */
    Mat4.prototype.SetScalingMatrix = function (scale) {
        this.SetElements(scale.x, 0, 0, 0, 0, scale.y, 0, 0, 0, 0, scale.z, 0, 0, 0, 0, 1);
    };
    /**
     * Sets the matrix to rotate objects around the X axis.
     *
     * @param radians The angle of rotation in radians.
     */
    Mat4.prototype.SetRotationMatrixX = function (radians) {
        var fSin = Math.sin(radians);
        var fCos = Math.cos(radians);
        this.SetElements(1, 0, 0, 0, 0, fCos, -fSin, 0, 0, fSin, fCos, 0, 0, 0, 0, 1);
    };
    /**
     * Sets the matrix to rotate objects around the Y axis.
     *
     * @param radians The angle of rotation in radians.
     */
    Mat4.prototype.SetRotationMatrixY = function (radians) {
        var fSin = Math.sin(radians);
        var fCos = Math.cos(radians);
        this.SetElements(fCos, 0, fSin, 0, 0, 1, 0, 0, -fSin, 0, fCos, 0, 0, 0, 0, 1);
    };
    /**
     * Sets the matrix to rotate objects around the Z axis.
     *
     * @param radians The angle of rotation in radians.
     */
    Mat4.prototype.SetRotationMatrixZ = function (radians) {
        var fSin = Math.sin(radians);
        var fCos = Math.cos(radians);
        this.SetElements(fCos, -fSin, 0, 0, fSin, fCos, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    };
    /**
     * Sets the matrix to rotate objects around an arbitrary axis.
     *
     * @param axis The normalized axis around which to rotate.
     * @param radians The angle of rotation in radians.
     */
    Mat4.prototype.SetRotationMatrix = function (axis, radians) {
        var cos = Math.cos(radians);
        var sin = Math.sin(radians);
        var oneMinusCos = 1 - cos;
        var xy = axis.x * axis.y;
        var xz = axis.x * axis.z;
        var yz = axis.y * axis.z;
        var xSin = axis.x * sin;
        var ySin = axis.y * sin;
        var zSin = axis.z * sin;
        var oneCos_xy = oneMinusCos * xy;
        var oneCos_xz = oneMinusCos * xz;
        var oneCos_yz = oneMinusCos * yz;
        //Column 1
        this.m_ElementsCM[0] = cos + (oneMinusCos * (axis.x * axis.x));
        this.m_ElementsCM[1] = oneCos_xy + zSin;
        this.m_ElementsCM[2] = oneCos_xz - ySin;
        this.m_ElementsCM[3] = 0;
        //Column 2
        this.m_ElementsCM[4] = oneCos_xy - zSin;
        this.m_ElementsCM[5] = cos + (oneMinusCos * (axis.y * axis.y));
        this.m_ElementsCM[6] = oneCos_yz + xSin;
        this.m_ElementsCM[7] = 0;
        //Column 3
        this.m_ElementsCM[8] = oneCos_xz + ySin;
        this.m_ElementsCM[9] = oneCos_yz - xSin;
        this.m_ElementsCM[10] = cos + (oneMinusCos * (axis.z * axis.z));
        this.m_ElementsCM[11] = 0;
        //Column 4
        this.m_ElementsCM[12] = 0;
        this.m_ElementsCM[13] = 0;
        this.m_ElementsCM[14] = 0;
        this.m_ElementsCM[15] = 1;
    };
    /**
     * Returns an all-zero matrix.
     */
    Mat4.ZeroMatrix = function () {
        var m = new Mat4();
        m.SetZero();
        return m;
    };
    /**
     * Returns an identity matrix.
     */
    Mat4.IdentityMatrix = function () {
        var m = new Mat4();
        return m;
    };
    /**
     * Flips all values along the diagonal.
     */
    Mat4.prototype.Transpose = function () {
        var tmp;
        tmp = this.GetElement(0, 1);
        this.SetElement(0, 1, this.GetElement(1, 0));
        this.SetElement(1, 0, tmp);
        tmp = this.GetElement(0, 2);
        this.SetElement(0, 2, this.GetElement(2, 0));
        this.SetElement(2, 0, tmp);
        tmp = this.GetElement(0, 3);
        this.SetElement(0, 3, this.GetElement(3, 0));
        this.SetElement(3, 0, tmp);
        tmp = this.GetElement(1, 2);
        this.SetElement(1, 2, this.GetElement(2, 1));
        this.SetElement(2, 1, tmp);
        tmp = this.GetElement(1, 3);
        this.SetElement(1, 3, this.GetElement(3, 1));
        this.SetElement(3, 1, tmp);
        tmp = this.GetElement(2, 3);
        this.SetElement(2, 3, this.GetElement(3, 2));
        this.SetElement(3, 2, tmp);
    };
    /**
     * Returns a transposed clone of this matrix.
     */
    Mat4.prototype.GetTranspose = function () {
        var m = this.Clone();
        m.Transpose();
        return m;
    };
    /**
     * Sets the values in the given row.
     */
    Mat4.prototype.SetRow = function (row, c1, c2, c3, c4) {
        this.m_ElementsCM[row] = c1;
        this.m_ElementsCM[4 + row] = c2;
        this.m_ElementsCM[8 + row] = c3;
        this.m_ElementsCM[12 + row] = c4;
    };
    /**
     * Sets the values in the given column.
     */
    Mat4.prototype.SetColumn = function (column, r1, r2, r3, r4) {
        var off = column * 4;
        this.m_ElementsCM[off + 0] = r1;
        this.m_ElementsCM[off + 1] = r2;
        this.m_ElementsCM[off + 2] = r3;
        this.m_ElementsCM[off + 3] = r4;
    };
    /**
     * Sets the values on the diagonal.
     */
    Mat4.prototype.SetDiagonal = function (d1, d2, d3, d4) {
        this.m_ElementsCM[0] = d1;
        this.m_ElementsCM[5] = d2;
        this.m_ElementsCM[10] = d3;
        this.m_ElementsCM[15] = d4;
    };
    /**
     * Inverts this matrix, if possible.
     *
     * @param epsilon The epsilon to determine whether this matrix can be inverted at all.
     * @returns true if the matrix could be inverted, false if inversion failed. In case of failure, this is unchanged.
     */
    Mat4.prototype.Invert = function (epsilon) {
        if (epsilon === void 0) { epsilon = 0.00001; }
        var inv = this.GetInverse(epsilon);
        if (inv == null)
            return false;
        this.SetMat4(inv);
        return true;
    };
    /**
     * Returns an inverted clone of this or null if inversion failed.
     *
     * @param epsilon The epsilon to determine whether this matrix can be inverted at all.
     */
    Mat4.prototype.GetInverse = function (epsilon) {
        if (epsilon === void 0) { epsilon = 0.00001; }
        var Inverse = new Mat4();
        var fDet = this.GetDeterminantOf4x4Matrix();
        if (exports.Utils.IsNumberEqual(fDet, 0, epsilon))
            return null;
        var fOneDivDet = 1.0 / fDet;
        for (var i = 0; i < 4; ++i) {
            Inverse.SetElement(i, 0, this.GetDeterminantOf3x3SubMatrix(i, 0) * fOneDivDet);
            fOneDivDet = -fOneDivDet;
            Inverse.SetElement(i, 1, this.GetDeterminantOf3x3SubMatrix(i, 1) * fOneDivDet);
            fOneDivDet = -fOneDivDet;
            Inverse.SetElement(i, 2, this.GetDeterminantOf3x3SubMatrix(i, 2) * fOneDivDet);
            fOneDivDet = -fOneDivDet;
            Inverse.SetElement(i, 3, this.GetDeterminantOf3x3SubMatrix(i, 3) * fOneDivDet);
        }
        return Inverse;
    };
    /**
     * Checks whether this and rhs have equal values within a certain epsilon.
     */
    Mat4.prototype.IsEqual = function (rhs, epsilon) {
        if (epsilon === void 0) { epsilon = 0.0001; }
        for (var i = 0; i < 16; ++i) {
            if (!exports.Utils.IsNumberEqual(this.m_ElementsCM[i], rhs.m_ElementsCM[i], epsilon)) {
                return false;
            }
        }
        return true;
    };
    /**
     * Checks whether this has all zero values within a certain epsilon.
     */
    Mat4.prototype.IsZero = function (epsilon) {
        if (epsilon === void 0) { epsilon = 0.0001; }
        for (var i = 0; i < 16; ++i) {
            if (!exports.Utils.IsNumberZero(this.m_ElementsCM[i], epsilon)) {
                return false;
            }
        }
        return true;
    };
    /**
     * Checks whether this is an identity matrix within a certain epsilon.
     */
    Mat4.prototype.IsIdentity = function (epsilon) {
        if (epsilon === void 0) { epsilon = 0.0001; }
        if (!exports.Utils.IsNumberEqual(this.m_ElementsCM[0], 1, epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[4], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[8], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[12], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[1], epsilon))
            return false;
        if (!exports.Utils.IsNumberEqual(this.m_ElementsCM[5], 1, epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[9], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[13], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[2], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[6], epsilon))
            return false;
        if (!exports.Utils.IsNumberEqual(this.m_ElementsCM[10], 1, epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[14], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[3], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[7], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[11], epsilon))
            return false;
        if (!exports.Utils.IsNumberEqual(this.m_ElementsCM[15], 1, epsilon))
            return false;
        return true;
    };
    /**
     * Checks whether this and rhs are completely identical without any epsilon comparison.
     */
    Mat4.prototype.IsIdentical = function (rhs) {
        for (var i = 0; i < 16; ++i) {
            if (this.m_ElementsCM[i] != rhs.m_ElementsCM[i]) {
                return false;
            }
        }
        return true;
    };
    /**
     * Returns the translation part of this matrix.
     */
    Mat4.prototype.GetTranslationVector = function () {
        return new exports.Vec3(this.m_ElementsCM[12], this.m_ElementsCM[13], this.m_ElementsCM[14]);
    };
    /**
     * Modifies only the translation part of this matrix.
     *
     * @param translation How much the matrix should translate objects.
     */
    Mat4.prototype.SetTranslationVector = function (translation) {
        this.m_ElementsCM[12] = translation.x;
        this.m_ElementsCM[13] = translation.y;
        this.m_ElementsCM[14] = translation.z;
    };
    /**
     * Tries to extract the scaling factors for x, y and z within this matrix.
     */
    Mat4.prototype.GetScalingFactors = function () {
        var tmp = new exports.Vec3();
        tmp.Set(this.GetElement(0, 0), this.GetElement(0, 1), this.GetElement(0, 2));
        var x = tmp.GetLength();
        tmp.Set(this.GetElement(1, 0), this.GetElement(1, 1), this.GetElement(1, 2));
        var y = tmp.GetLength();
        tmp.Set(this.GetElement(2, 0), this.GetElement(2, 1), this.GetElement(2, 2));
        var z = tmp.GetLength();
        tmp.Set(x, y, z);
        return tmp;
    };
    /**
     * Tries to rescale this matrix such that it applies the given scaling.
     *
     * @param epsilon The epsilon to detect whether rescaling the matrix is possible.
     * @returns True if successful, false if the desired scaling could not be baked into the matrix.
     */
    Mat4.prototype.SetScalingFactors = function (x, y, z, epsilon) {
        if (epsilon === void 0) { epsilon = 0.0001; }
        var tx = new exports.Vec3(this.GetElement(0, 0), this.GetElement(0, 1), this.GetElement(0, 2));
        var ty = new exports.Vec3(this.GetElement(1, 0), this.GetElement(1, 1), this.GetElement(1, 2));
        var tz = new exports.Vec3(this.GetElement(2, 0), this.GetElement(2, 1), this.GetElement(2, 2));
        if (tx.SetLength(x, epsilon) == false)
            return false;
        if (ty.SetLength(y, epsilon) == false)
            return false;
        if (tz.SetLength(z, epsilon) == false)
            return false;
        this.SetElement(0, 0, tx.x);
        this.SetElement(0, 1, tx.y);
        this.SetElement(0, 2, tx.z);
        this.SetElement(1, 0, ty.x);
        this.SetElement(1, 1, ty.y);
        this.SetElement(1, 2, ty.z);
        this.SetElement(2, 0, tz.x);
        this.SetElement(2, 1, tz.y);
        this.SetElement(2, 2, tz.z);
        return true;
    };
    /**
     * Modifies the incoming vector by multiplying this from the left. Treats 'pos' like a position vector with an implied w-component of 1.
     *   pos.xyz = this * pos.xyz1
     *
     * @param pos A 'position' vector with an implied w-component of 1. Thus the translation part of the matrix will affect the resulting vector.
     */
    Mat4.prototype.TransformPosition = function (pos) {
        var x = this.GetElement(0, 0) * pos.x + this.GetElement(1, 0) * pos.y + this.GetElement(2, 0) * pos.z + this.GetElement(3, 0);
        var y = this.GetElement(0, 1) * pos.x + this.GetElement(1, 1) * pos.y + this.GetElement(2, 1) * pos.z + this.GetElement(3, 1);
        var z = this.GetElement(0, 2) * pos.x + this.GetElement(1, 2) * pos.y + this.GetElement(2, 2) * pos.z + this.GetElement(3, 2);
        pos.Set(x, y, z);
    };
    /**
     * Modifies the incoming vector by multiplying this from the left. Treats 'dir' like a direction vector with an implied w-component of 0.
     *   dir.xyz = dir * pos.xyz0
     *
     * @param dir A 'direction' vector with an implied w-component of 0. Thus the translation part of the matrix willNOT  affect the resulting vector.
     */
    Mat4.prototype.TransformDirection = function (dir) {
        var x = this.GetElement(0, 0) * dir.x + this.GetElement(1, 0) * dir.y + this.GetElement(2, 0) * dir.z;
        var y = this.GetElement(0, 1) * dir.x + this.GetElement(1, 1) * dir.y + this.GetElement(2, 1) * dir.z;
        var z = this.GetElement(0, 2) * dir.x + this.GetElement(1, 2) * dir.y + this.GetElement(2, 2) * dir.z;
        dir.Set(x, y, z);
    };
    /**
     * Multiplies all values in this matrix with 'factor'.
     */
    Mat4.prototype.MulNumber = function (factor) {
        for (var i = 0; i < 16; ++i) {
            this.m_ElementsCM[i] *= factor;
        }
    };
    /**
     * Divides all values in this matrix by 'factor'.
     */
    Mat4.prototype.DivNumber = function (factor) {
        var mul = 1 / factor;
        for (var i = 0; i < 16; ++i) {
            this.m_ElementsCM[i] *= mul;
        }
    };
    /**
     * Adds the components of rhs into the components of this.
     */
    Mat4.prototype.AddMat4 = function (rhs) {
        for (var i = 0; i < 16; ++i) {
            this.m_ElementsCM[i] += rhs.m_ElementsCM[i];
        }
    };
    /**
     * Subtracts the components of rhs from the components of this.
     */
    Mat4.prototype.SubMat4 = function (rhs) {
        for (var i = 0; i < 16; ++i) {
            this.m_ElementsCM[i] -= rhs.m_ElementsCM[i];
        }
    };
    /**
     * Sets this matrix to be the product of lhs and rhs.
     *
     *   this = lhs * rhs
     */
    Mat4.prototype.SetMulMat4 = function (lhs, rhs) {
        for (var i = 0; i < 4; ++i) {
            this.SetElement(0, i, lhs.GetElement(0, i) * rhs.GetElement(0, 0) + lhs.GetElement(1, i) * rhs.GetElement(0, 1) + lhs.GetElement(2, i) * rhs.GetElement(0, 2) + lhs.GetElement(3, i) * rhs.GetElement(0, 3));
            this.SetElement(1, i, lhs.GetElement(0, i) * rhs.GetElement(1, 0) + lhs.GetElement(1, i) * rhs.GetElement(1, 1) + lhs.GetElement(2, i) * rhs.GetElement(1, 2) + lhs.GetElement(3, i) * rhs.GetElement(1, 3));
            this.SetElement(2, i, lhs.GetElement(0, i) * rhs.GetElement(2, 0) + lhs.GetElement(1, i) * rhs.GetElement(2, 1) + lhs.GetElement(2, i) * rhs.GetElement(2, 2) + lhs.GetElement(3, i) * rhs.GetElement(2, 3));
            this.SetElement(3, i, lhs.GetElement(0, i) * rhs.GetElement(3, 0) + lhs.GetElement(1, i) * rhs.GetElement(3, 1) + lhs.GetElement(2, i) * rhs.GetElement(3, 2) + lhs.GetElement(3, i) * rhs.GetElement(3, 3));
        }
    };
    /**
     * Returns the values in 'row' as a 4-element array.
     */
    Mat4.prototype.GetRow = function (row) {
        return [this.m_ElementsCM[row], this.m_ElementsCM[row + 4], this.m_ElementsCM[row + 8], this.m_ElementsCM[row + 12]];
    };
    /**
     * Returns the values in 'column' as a 4-element array.
     */
    Mat4.prototype.GetColumn = function (column) {
        return [this.m_ElementsCM[column * 4], this.m_ElementsCM[column * 4 + 1], this.m_ElementsCM[column * 4 + 2], this.m_ElementsCM[column * 4 + 3]];
    };
    /**
     * Returns the values in from the diagonal as a 4-element array.
     */
    Mat4.prototype.GetDiagonal = function () {
        return [this.m_ElementsCM[0], this.m_ElementsCM[5], this.m_ElementsCM[10], this.m_ElementsCM[15]];
    };
    /**
     * Sets all elements in this by copying them from an array.
     *
     * @param array The array with the 16 values to copy.
     * @param isColumnMajor Whether the data in the array is column-major or row-major.
     */
    Mat4.prototype.SetFromArray = function (array, isColumnMajor) {
        if (isColumnMajor) {
            for (var i = 0; i < 16; ++i) {
                this.m_ElementsCM[i] = array[i];
            }
        }
        else {
            this.m_ElementsCM[0] = array[0];
            this.m_ElementsCM[1] = array[4];
            this.m_ElementsCM[2] = array[8];
            this.m_ElementsCM[3] = array[12];
            this.m_ElementsCM[4] = array[1];
            this.m_ElementsCM[5] = array[5];
            this.m_ElementsCM[6] = array[9];
            this.m_ElementsCM[7] = array[13];
            this.m_ElementsCM[8] = array[2];
            this.m_ElementsCM[9] = array[6];
            this.m_ElementsCM[10] = array[10];
            this.m_ElementsCM[11] = array[14];
            this.m_ElementsCM[12] = array[3];
            this.m_ElementsCM[13] = array[7];
            this.m_ElementsCM[14] = array[11];
            this.m_ElementsCM[15] = array[15];
        }
    };
    /**
     * Returns the values of the matrix as an array.
     *
     * @param asColumnMajor Whether the array should contain the values in column-major or row-major order.
     */
    Mat4.prototype.GetAsArray = function (asColumnMajor) {
        if (asColumnMajor) {
            var array = [
                this.m_ElementsCM[0], this.m_ElementsCM[1], this.m_ElementsCM[2], this.m_ElementsCM[3],
                this.m_ElementsCM[4], this.m_ElementsCM[5], this.m_ElementsCM[6], this.m_ElementsCM[7],
                this.m_ElementsCM[8], this.m_ElementsCM[9], this.m_ElementsCM[10], this.m_ElementsCM[11],
                this.m_ElementsCM[12], this.m_ElementsCM[13], this.m_ElementsCM[14], this.m_ElementsCM[15]
            ];
            return array;
        }
        else {
            var array = [
                this.m_ElementsCM[0], this.m_ElementsCM[4], this.m_ElementsCM[8], this.m_ElementsCM[12],
                this.m_ElementsCM[1], this.m_ElementsCM[5], this.m_ElementsCM[9], this.m_ElementsCM[13],
                this.m_ElementsCM[2], this.m_ElementsCM[6], this.m_ElementsCM[10], this.m_ElementsCM[14],
                this.m_ElementsCM[3], this.m_ElementsCM[7], this.m_ElementsCM[11], this.m_ElementsCM[15]
            ];
            return array;
        }
    };
    /**
     * Combines a Mat3 for rotation/scaling and a Vec3 for translation into this Mat4.
     * All values in this are overwritten, including the last row.
     *
     * @param rotation The 3x3 matrix to copy into this matrix.
     * @param translation The vector to copy into the last column of this matrix.
     */
    Mat4.prototype.SetTransformationMatrix = function (rotation, translation) {
        this.SetRotationalPart(rotation);
        this.SetTranslationVector(translation);
        this.SetRow(3, 0, 0, 0, 1);
    };
    /**
     * Returns a clone of the 3x3 sub-matrix. Same as 'CloneAsMat3()'.
     */
    Mat4.prototype.GetRotationalPart = function () {
        return this.CloneAsMat3();
    };
    /**
     * Overwrites only the 3x3 sub-matrix with the given values.
     */
    Mat4.prototype.SetRotationalPart = function (mat3) {
        this.m_ElementsCM[0] = mat3.m_ElementsCM[0];
        this.m_ElementsCM[1] = mat3.m_ElementsCM[1];
        this.m_ElementsCM[2] = mat3.m_ElementsCM[2];
        this.m_ElementsCM[4] = mat3.m_ElementsCM[3];
        this.m_ElementsCM[5] = mat3.m_ElementsCM[4];
        this.m_ElementsCM[6] = mat3.m_ElementsCM[5];
        this.m_ElementsCM[8] = mat3.m_ElementsCM[6];
        this.m_ElementsCM[9] = mat3.m_ElementsCM[7];
        this.m_ElementsCM[10] = mat3.m_ElementsCM[8];
    };
    Mat4.prototype.GetDeterminantOf3x3SubMatrix = function (i, j) {
        var si0 = 0 + ((i <= 0) ? 1 : 0);
        var si1 = 1 + ((i <= 1) ? 1 : 0);
        var si2 = 2 + ((i <= 2) ? 1 : 0);
        var sj0 = 0 + ((j <= 0) ? 1 : 0);
        var sj1 = 1 + ((j <= 1) ? 1 : 0);
        var sj2 = 2 + ((j <= 2) ? 1 : 0);
        var fDet2 = ((this.GetElement(sj0, si0) * this.GetElement(sj1, si1) * this.GetElement(sj2, si2) +
            this.GetElement(sj1, si0) * this.GetElement(sj2, si1) * this.GetElement(sj0, si2) +
            this.GetElement(sj2, si0) * this.GetElement(sj0, si1) * this.GetElement(sj1, si2)) -
            (this.GetElement(sj0, si2) * this.GetElement(sj1, si1) * this.GetElement(sj2, si0) +
                this.GetElement(sj1, si2) * this.GetElement(sj2, si1) * this.GetElement(sj0, si0) +
                this.GetElement(sj2, si2) * this.GetElement(sj0, si1) * this.GetElement(sj1, si0)));
        return fDet2;
    };
    Mat4.prototype.GetDeterminantOf4x4Matrix = function () {
        var det = 0.0;
        det += this.GetElement(0, 0) * this.GetDeterminantOf3x3SubMatrix(0, 0);
        det += -this.GetElement(1, 0) * this.GetDeterminantOf3x3SubMatrix(0, 1);
        det += this.GetElement(2, 0) * this.GetDeterminantOf3x3SubMatrix(0, 2);
        det += -this.GetElement(3, 0) * this.GetDeterminantOf3x3SubMatrix(0, 3);
        return det;
    };
    return Mat4;
}());
exports.Mat4 = Mat4;
