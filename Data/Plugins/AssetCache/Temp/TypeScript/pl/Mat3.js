/*SOURCE-HASH:42A27C56712D6255*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var __Utils = require("./Utils");
exports.Utils = __Utils.Utils;
var __Vec3 = require("./Vec3");
exports.Vec3 = __Vec3.Vec3;
/**
 * A 3x3 matrix that can represent rotations and scaling, but no translation.
 */
var Mat3 = /** @class */ (function () {
    /**
     * By default the constructor will initialize the matrix to identity.
     */
    function Mat3(c1r1, c2r1, c3r1, c1r2, c2r2, c3r2, c1r3, c2r3, c3r3) {
        if (c1r1 === void 0) { c1r1 = 1; }
        if (c2r1 === void 0) { c2r1 = 0; }
        if (c3r1 === void 0) { c3r1 = 0; }
        if (c1r2 === void 0) { c1r2 = 0; }
        if (c2r2 === void 0) { c2r2 = 1; }
        if (c3r2 === void 0) { c3r2 = 0; }
        if (c1r3 === void 0) { c1r3 = 0; }
        if (c2r3 === void 0) { c2r3 = 0; }
        if (c3r3 === void 0) { c3r3 = 1; }
        this.m_ElementsCM = [
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
        ];
        this.SetElements(c1r1, c2r1, c3r1, c1r2, c2r2, c3r2, c1r3, c2r3, c3r3);
    }
    /**
     * Returns a duplicate of this matrix.
     */
    Mat3.prototype.Clone = function () {
        var c = new Mat3();
        c.SetMat3(this);
        return c;
    };
    /**
     * Copies the values of m into this.
     */
    Mat3.prototype.SetMat3 = function (m) {
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] = m.m_ElementsCM[i];
        }
    };
    /**
     * Returns the value from the requested row and column.
     */
    Mat3.prototype.GetElement = function (column, row) {
        return this.m_ElementsCM[column * 3 + row];
    };
    /**
     * Overwrites the value in the given row and column.
     */
    Mat3.prototype.SetElement = function (column, row, value) {
        this.m_ElementsCM[column * 3 + row] = value;
    };
    /**
     * Sets all values of this matrix.
     */
    Mat3.prototype.SetElements = function (c1r1, c2r1, c3r1, c1r2, c2r2, c3r2, c1r3, c2r3, c3r3) {
        this.m_ElementsCM[0] = c1r1;
        this.m_ElementsCM[3] = c2r1;
        this.m_ElementsCM[6] = c3r1;
        this.m_ElementsCM[1] = c1r2;
        this.m_ElementsCM[4] = c2r2;
        this.m_ElementsCM[7] = c3r2;
        this.m_ElementsCM[2] = c1r3;
        this.m_ElementsCM[5] = c2r3;
        this.m_ElementsCM[8] = c3r3;
    };
    /**
     * Sets all values to zero.
     */
    Mat3.prototype.SetZero = function () {
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] = 0;
        }
    };
    /**
     * Sets the matrix to be the identity matrix.
     */
    Mat3.prototype.SetIdentity = function () {
        this.m_ElementsCM[0] = 1;
        this.m_ElementsCM[3] = 0;
        this.m_ElementsCM[6] = 0;
        this.m_ElementsCM[1] = 0;
        this.m_ElementsCM[4] = 1;
        this.m_ElementsCM[7] = 0;
        this.m_ElementsCM[2] = 0;
        this.m_ElementsCM[5] = 0;
        this.m_ElementsCM[8] = 1;
    };
    /**
     * Sets this matrix to be a scaling matrix
     * @param scale How much the matrix scales along x, y and z.
     */
    Mat3.prototype.SetScalingMatrix = function (scale) {
        this.SetElements(scale.x, 0, 0, 0, scale.y, 0, 0, 0, scale.z);
    };
    /**
     * Sets the matrix to rotate objects around the X axis.
     *
     * @param radians The angle of rotation in radians.
     */
    Mat3.prototype.SetRotationMatrixX = function (radians) {
        var fSin = Math.sin(radians);
        var fCos = Math.cos(radians);
        this.SetElements(1, 0, 0, 0, fCos, -fSin, 0, fSin, fCos);
    };
    /**
     * Sets the matrix to rotate objects around the Y axis.
     *
     * @param radians The angle of rotation in radians.
     */
    Mat3.prototype.SetRotationMatrixY = function (radians) {
        var fSin = Math.sin(radians);
        var fCos = Math.cos(radians);
        this.SetElements(fCos, 0, fSin, 0, 1, 0, -fSin, 0, fCos);
    };
    /**
     * Sets the matrix to rotate objects around the Z axis.
     *
     * @param radians The angle of rotation in radians.
     */
    Mat3.prototype.SetRotationMatrixZ = function (radians) {
        var fSin = Math.sin(radians);
        var fCos = Math.cos(radians);
        this.SetElements(fCos, -fSin, 0, fSin, fCos, 0, 0, 0, 1);
    };
    /**
     * Sets the matrix to rotate objects around an arbitrary axis.
     *
     * @param axis The normalized axis around which to rotate.
     * @param radians The angle of rotation in radians.
     */
    Mat3.prototype.SetRotationMatrix = function (axis, radians) {
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
        //Column 2
        this.m_ElementsCM[3] = oneCos_xy - zSin;
        this.m_ElementsCM[4] = cos + (oneMinusCos * (axis.y * axis.y));
        this.m_ElementsCM[5] = oneCos_yz + xSin;
        //Column 3
        this.m_ElementsCM[6] = oneCos_xz + ySin;
        this.m_ElementsCM[7] = oneCos_yz - xSin;
        this.m_ElementsCM[8] = cos + (oneMinusCos * (axis.z * axis.z));
    };
    /**
     * Returns an all-zero matrix.
     */
    Mat3.ZeroMatrix = function () {
        var m = new Mat3();
        m.SetZero();
        return m;
    };
    /**
     * Returns an identity matrix.
     */
    Mat3.IdentityMatrix = function () {
        var m = new Mat3();
        return m;
    };
    /**
     * Flips all values along the diagonal.
     */
    Mat3.prototype.Transpose = function () {
        var tmp;
        tmp = this.GetElement(0, 1);
        this.SetElement(0, 1, this.GetElement(1, 0));
        this.SetElement(1, 0, tmp);
        tmp = this.GetElement(0, 2);
        this.SetElement(0, 2, this.GetElement(2, 0));
        this.SetElement(2, 0, tmp);
        tmp = this.GetElement(1, 2);
        this.SetElement(1, 2, this.GetElement(2, 1));
        this.SetElement(2, 1, tmp);
    };
    /**
     * Returns a transposed clone of this matrix.
     */
    Mat3.prototype.GetTranspose = function () {
        var m = this.Clone();
        m.Transpose();
        return m;
    };
    /**
     * Sets the values in the given row.
     */
    Mat3.prototype.SetRow = function (row, c1, c2, c3) {
        this.m_ElementsCM[row] = c1;
        this.m_ElementsCM[3 + row] = c2;
        this.m_ElementsCM[6 + row] = c3;
    };
    /**
     * Sets the values in the given column.
     */
    Mat3.prototype.SetColumn = function (column, r1, r2, r3) {
        var off = column * 3;
        this.m_ElementsCM[off + 0] = r1;
        this.m_ElementsCM[off + 1] = r2;
        this.m_ElementsCM[off + 2] = r3;
    };
    /**
     * Sets the values on the diagonal.
     */
    Mat3.prototype.SetDiagonal = function (d1, d2, d3) {
        this.m_ElementsCM[0] = d1;
        this.m_ElementsCM[4] = d2;
        this.m_ElementsCM[8] = d3;
    };
    /**
     * Inverts this matrix, if possible.
     *
     * @param epsilon The epsilon to determine whether this matrix can be inverted at all.
     * @returns true if the matrix could be inverted, false if inversion failed. In case of failure, this is unchanged.
     */
    Mat3.prototype.Invert = function (epsilon) {
        if (epsilon === void 0) { epsilon = 0.00001; }
        var inv = this.GetInverse(epsilon);
        if (inv == null)
            return false;
        this.SetMat3(inv);
        return true;
    };
    /**
     * Returns an inverted clone of this or null if inversion failed.
     *
     * @param epsilon The epsilon to determine whether this matrix can be inverted at all.
     */
    Mat3.prototype.GetInverse = function (epsilon) {
        if (epsilon === void 0) { epsilon = 0.00001; }
        var Inverse = new Mat3();
        var fDet = this.GetElement(0, 0) * (this.GetElement(2, 2) * this.GetElement(1, 1) - this.GetElement(1, 2) * this.GetElement(2, 1)) -
            this.GetElement(0, 1) * (this.GetElement(2, 2) * this.GetElement(1, 0) - this.GetElement(1, 2) * this.GetElement(2, 0)) +
            this.GetElement(0, 2) * (this.GetElement(2, 1) * this.GetElement(1, 0) - this.GetElement(1, 1) * this.GetElement(2, 0));
        if (exports.Utils.IsNumberZero(fDet, epsilon))
            return null;
        var fOneDivDet = 1 / fDet;
        Inverse.SetElement(0, 0, (this.GetElement(2, 2) * this.GetElement(1, 1) - this.GetElement(1, 2) * this.GetElement(2, 1)));
        Inverse.SetElement(0, 1, -(this.GetElement(2, 2) * this.GetElement(0, 1) - this.GetElement(0, 2) * this.GetElement(2, 1)));
        Inverse.SetElement(0, 2, (this.GetElement(1, 2) * this.GetElement(0, 1) - this.GetElement(0, 2) * this.GetElement(1, 1)));
        Inverse.SetElement(1, 0, -(this.GetElement(2, 2) * this.GetElement(1, 0) - this.GetElement(1, 2) * this.GetElement(2, 0)));
        Inverse.SetElement(1, 1, (this.GetElement(2, 2) * this.GetElement(0, 0) - this.GetElement(0, 2) * this.GetElement(2, 0)));
        Inverse.SetElement(1, 2, -(this.GetElement(1, 2) * this.GetElement(0, 0) - this.GetElement(0, 2) * this.GetElement(1, 0)));
        Inverse.SetElement(2, 0, (this.GetElement(2, 1) * this.GetElement(1, 0) - this.GetElement(1, 1) * this.GetElement(2, 0)));
        Inverse.SetElement(2, 1, -(this.GetElement(2, 1) * this.GetElement(0, 0) - this.GetElement(0, 1) * this.GetElement(2, 0)));
        Inverse.SetElement(2, 2, (this.GetElement(1, 1) * this.GetElement(0, 0) - this.GetElement(0, 1) * this.GetElement(1, 0)));
        Inverse.MulNumber(fOneDivDet);
        return Inverse;
    };
    /**
     * Checks whether this and rhs have equal values within a certain epsilon.
     */
    Mat3.prototype.IsEqual = function (rhs, epsilon) {
        for (var i = 0; i < 9; ++i) {
            if (!exports.Utils.IsNumberEqual(this.m_ElementsCM[i], rhs.m_ElementsCM[i], epsilon)) {
                return false;
            }
        }
        return true;
    };
    /**
     * Checks whether this has all zero values within a certain epsilon.
     */
    Mat3.prototype.IsZero = function (epsilon) {
        if (epsilon === void 0) { epsilon = 0.0001; }
        for (var i = 0; i < 9; ++i) {
            if (!exports.Utils.IsNumberZero(this.m_ElementsCM[i], epsilon)) {
                return false;
            }
        }
        return true;
    };
    /**
     * Checks whether this is an identity matrix within a certain epsilon.
     */
    Mat3.prototype.IsIdentity = function (epsilon) {
        if (epsilon === void 0) { epsilon = 0.0001; }
        if (!exports.Utils.IsNumberEqual(this.m_ElementsCM[0], 1, epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[3], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[6], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[1], epsilon))
            return false;
        if (!exports.Utils.IsNumberEqual(this.m_ElementsCM[4], 1, epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[7], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[2], epsilon))
            return false;
        if (!exports.Utils.IsNumberZero(this.m_ElementsCM[5], epsilon))
            return false;
        if (!exports.Utils.IsNumberEqual(this.m_ElementsCM[8], 1, epsilon))
            return false;
        return true;
    };
    /**
     * Checks whether this and rhs are completely identical without any epsilon comparison.
     */
    Mat3.prototype.IsIdentical = function (rhs) {
        for (var i = 0; i < 9; ++i) {
            if (this.m_ElementsCM[i] != rhs.m_ElementsCM[i]) {
                return false;
            }
        }
        return true;
    };
    /**
     * Tries to extract the scaling factors for x, y and z within this matrix.
     */
    Mat3.prototype.GetScalingFactors = function () {
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
    Mat3.prototype.SetScalingFactors = function (x, y, z, epsilon) {
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
     * Modifies the incoming vector by multiplying this from the left.
     *   dir = this * dir
     */
    Mat3.prototype.TransformDirection = function (dir) {
        var x = this.GetElement(0, 0) * dir.x + this.GetElement(1, 0) * dir.y + this.GetElement(2, 0) * dir.z;
        var y = this.GetElement(0, 1) * dir.x + this.GetElement(1, 1) * dir.y + this.GetElement(2, 1) * dir.z;
        var z = this.GetElement(0, 2) * dir.x + this.GetElement(1, 2) * dir.y + this.GetElement(2, 2) * dir.z;
        dir.Set(x, y, z);
    };
    /**
     * Multiplies all values in this matrix with 'factor'.
     */
    Mat3.prototype.MulNumber = function (factor) {
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] *= factor;
        }
    };
    /**
     * Divides all values in this matrix by 'factor'.
     */
    Mat3.prototype.DivNumber = function (factor) {
        var mul = 1 / factor;
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] *= mul;
        }
    };
    /**
     * Adds the components of rhs into the components of this.
     */
    Mat3.prototype.AddMat3 = function (rhs) {
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] += rhs.m_ElementsCM[i];
        }
    };
    /**
     * Subtracts the components of rhs from the components of this.
     */
    Mat3.prototype.SubMat3 = function (rhs) {
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] -= rhs.m_ElementsCM[i];
        }
    };
    /**
     * Sets this matrix to be the product of lhs and rhs.
     *
     *   this = lhs * rhs
     */
    Mat3.prototype.SetMulMat3 = function (lhs, rhs) {
        for (var i = 0; i < 3; ++i) {
            this.SetElement(0, i, lhs.GetElement(0, i) * rhs.GetElement(0, 0) + lhs.GetElement(1, i) * rhs.GetElement(0, 1) + lhs.GetElement(2, i) * rhs.GetElement(0, 2));
            this.SetElement(1, i, lhs.GetElement(0, i) * rhs.GetElement(1, 0) + lhs.GetElement(1, i) * rhs.GetElement(1, 1) + lhs.GetElement(2, i) * rhs.GetElement(1, 2));
            this.SetElement(2, i, lhs.GetElement(0, i) * rhs.GetElement(2, 0) + lhs.GetElement(1, i) * rhs.GetElement(2, 1) + lhs.GetElement(2, i) * rhs.GetElement(2, 2));
        }
    };
    /**
     * Returns the values in 'row' as a 3-element array.
     */
    Mat3.prototype.GetRow = function (row) {
        return [this.m_ElementsCM[row], this.m_ElementsCM[row + 3], this.m_ElementsCM[row + 6]];
    };
    /**
     * Returns the values in 'column' as a 3-element array.
     */
    Mat3.prototype.GetColumn = function (column) {
        return [this.m_ElementsCM[column * 3], this.m_ElementsCM[column * 3 + 1], this.m_ElementsCM[column * 3 + 2]];
    };
    /**
     * Returns the values in from the diagonal as a 3-element array.
     */
    Mat3.prototype.GetDiagonal = function () {
        return [this.m_ElementsCM[0], this.m_ElementsCM[4], this.m_ElementsCM[8]];
    };
    /**
     * Sets all elements in this by copying them from an array.
     *
     * @param array The array with the 9 values to copy.
     * @param isColumnMajor Whether the data in the array is column-major or row-major.
     */
    Mat3.prototype.SetFromArray = function (array, isColumnMajor) {
        if (isColumnMajor) {
            for (var i = 0; i < 9; ++i) {
                this.m_ElementsCM[i] = array[i];
            }
        }
        else {
            this.m_ElementsCM[0] = array[0];
            this.m_ElementsCM[1] = array[3];
            this.m_ElementsCM[2] = array[6];
            this.m_ElementsCM[3] = array[1];
            this.m_ElementsCM[4] = array[4];
            this.m_ElementsCM[5] = array[7];
            this.m_ElementsCM[6] = array[2];
            this.m_ElementsCM[7] = array[5];
            this.m_ElementsCM[8] = array[8];
        }
    };
    /**
     * Returns the values of the matrix as an array.
     *
     * @param asColumnMajor Whether the array should contain the values in column-major or row-major order.
     */
    Mat3.prototype.GetAsArray = function (asColumnMajor) {
        if (asColumnMajor) {
            var array = [
                this.m_ElementsCM[0], this.m_ElementsCM[1], this.m_ElementsCM[2],
                this.m_ElementsCM[3], this.m_ElementsCM[4], this.m_ElementsCM[5],
                this.m_ElementsCM[6], this.m_ElementsCM[7], this.m_ElementsCM[8]
            ];
            return array;
        }
        else {
            var array = [
                this.m_ElementsCM[0], this.m_ElementsCM[3], this.m_ElementsCM[6],
                this.m_ElementsCM[1], this.m_ElementsCM[4], this.m_ElementsCM[7],
                this.m_ElementsCM[2], this.m_ElementsCM[5], this.m_ElementsCM[8]
            ];
            return array;
        }
    };
    return Mat3;
}());
exports.Mat3 = Mat3;
