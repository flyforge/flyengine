/*SOURCE-HASH:0ACAED439AB6DCA1*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var __Vec3 = require("./Vec3");
exports.Vec3 = __Vec3.Vec3;
var __Angle = require("./Angle");
exports.Angle = __Angle.Angle;
var __Mat3 = require("./Mat3");
exports.Mat3 = __Mat3.Mat3;
var __Mat4 = require("./Mat4");
exports.Mat4 = __Mat4.Mat4;
exports.Utils = __Vec3.Utils;
/**
 * A quaternion class to represent rotations.
 */
var Quat = /** @class */ (function () {
    /**
     * By default the constructor initializes the quaternion to identity. It is very rare to set the quaternion values to anything different manually.
     */
    function Quat(_x, _y, _z, _w) {
        if (_x === void 0) { _x = 0.0; }
        if (_y === void 0) { _y = 0.0; }
        if (_z === void 0) { _z = 0.0; }
        if (_w === void 0) { _w = 1.0; }
        this.x = _x;
        this.y = _y;
        this.z = _z;
        this.w = _w;
    }
    /**
     * Returns a duplicate of this quaternion.
     */
    Quat.prototype.Clone = function () {
        return new Quat(this.x, this.y, this.z, this.w);
    };
    /**
     * Returns a 3x3 rotation matrix that represents the same rotation as the quaternion.
     */
    Quat.prototype.GetAsMat3 = function () {
        var m = new exports.Mat3();
        var fTx = this.x + this.x;
        var fTy = this.y + this.y;
        var fTz = this.z + this.z;
        var fTwx = fTx * this.w;
        var fTwy = fTy * this.w;
        var fTwz = fTz * this.w;
        var fTxx = fTx * this.x;
        var fTxy = fTy * this.x;
        var fTxz = fTz * this.x;
        var fTyy = fTy * this.y;
        var fTyz = fTz * this.y;
        var fTzz = fTz * this.z;
        m.SetElement(0, 0, 1 - (fTyy + fTzz));
        m.SetElement(1, 0, fTxy - fTwz);
        m.SetElement(2, 0, fTxz + fTwy);
        m.SetElement(0, 1, fTxy + fTwz);
        m.SetElement(1, 1, 1 - (fTxx + fTzz));
        m.SetElement(2, 1, fTyz - fTwx);
        m.SetElement(0, 2, fTxz - fTwy);
        m.SetElement(1, 2, fTyz + fTwx);
        m.SetElement(2, 2, 1 - (fTxx + fTyy));
        return m;
    };
    /**
     * Returns a 4x4 rotation matrix that represents the same rotation as the quaternion.
     */
    Quat.prototype.GetAsMat4 = function () {
        var m = new exports.Mat4();
        var fTx = this.x + this.x;
        var fTy = this.y + this.y;
        var fTz = this.z + this.z;
        var fTwx = fTx * this.w;
        var fTwy = fTy * this.w;
        var fTwz = fTz * this.w;
        var fTxx = fTx * this.x;
        var fTxy = fTy * this.x;
        var fTxz = fTz * this.x;
        var fTyy = fTy * this.y;
        var fTyz = fTz * this.y;
        var fTzz = fTz * this.z;
        m.SetElement(0, 0, 1 - (fTyy + fTzz));
        m.SetElement(1, 0, fTxy - fTwz);
        m.SetElement(2, 0, fTxz + fTwy);
        m.SetElement(0, 1, fTxy + fTwz);
        m.SetElement(1, 1, 1 - (fTxx + fTzz));
        m.SetElement(2, 1, fTyz - fTwx);
        m.SetElement(0, 2, fTxz - fTwy);
        m.SetElement(1, 2, fTyz + fTwx);
        m.SetElement(2, 2, 1 - (fTxx + fTyy));
        return m;
    };
    /**
     * Extracts the rotation stored in the 3x3 matrix and initializes this quaternion with it.
     */
    Quat.prototype.SetFromMat3 = function (m) {
        var trace = m.GetElement(0, 0) + m.GetElement(1, 1) + m.GetElement(2, 2);
        var half = 0.5;
        var val = [0, 0, 0, 0];
        if (trace > 0) {
            var s = Math.sqrt(trace + 1);
            var t = half / s;
            val[0] = (m.GetElement(1, 2) - m.GetElement(2, 1)) * t;
            val[1] = (m.GetElement(2, 0) - m.GetElement(0, 2)) * t;
            val[2] = (m.GetElement(0, 1) - m.GetElement(1, 0)) * t;
            val[3] = half * s;
        }
        else {
            var next = [1, 2, 0];
            var i = 0;
            if (m.GetElement(1, 1) > m.GetElement(0, 0))
                i = 1;
            if (m.GetElement(2, 2) > m.GetElement(i, i))
                i = 2;
            var j = next[i];
            var k = next[j];
            var s = Math.sqrt(m.GetElement(i, i) - (m.GetElement(j, j) + m.GetElement(k, k)) + 1);
            var t = half / s;
            val[i] = half * s;
            val[3] = (m.GetElement(j, k) - m.GetElement(k, j)) * t;
            val[j] = (m.GetElement(i, j) + m.GetElement(j, i)) * t;
            val[k] = (m.GetElement(i, k) + m.GetElement(k, i)) * t;
        }
        this.x = val[0];
        this.y = val[1];
        this.z = val[2];
        this.w = val[3];
    };
    /**
     * Copies the values from rhs into this.
     */
    Quat.prototype.SetQuat = function (rhs) {
        this.x = rhs.x;
        this.y = rhs.y;
        this.z = rhs.z;
        this.w = rhs.w;
    };
    /**
     * Sets this to be identity, ie. a zero rotation quaternion.
     */
    Quat.prototype.SetIdentity = function () {
        this.x = 0;
        this.y = 0;
        this.z = 0;
        this.w = 1.0;
    };
    /**
     * Returns an identity quaternion.
     */
    Quat.IdentityQuaternion = function () {
        return new Quat();
    };
    /**
     * Normalizes the quaternion. All quaternions must be normalized to work correctly.
     */
    Quat.prototype.Normalize = function () {
        var n = this.x * this.x + this.y * this.y + this.z * this.z + this.w * this.w;
        n = 1.0 / Math.sqrt(n);
        this.x *= n;
        this.y *= n;
        this.z *= n;
        this.w *= n;
    };
    /**
     * Negates the quaternion. Afterwards it will rotate objects in the opposite direction.
     */
    Quat.prototype.Negate = function () {
        this.x = -this.x;
        this.y = -this.y;
        this.z = -this.z;
    };
    /**
     * Returns a negated quaternion, which rotates objects in the opposite direction.
     */
    Quat.prototype.GetNegated = function () {
        return new Quat(-this.x, -this.y, -this.z, this.w);
    };
    /**
     * Creates the quaternion from a normalized axis and an angle (in radians).
     */
    Quat.prototype.SetFromAxisAndAngle = function (rotationAxis, angleInRadians) {
        var halfAngle = angleInRadians * 0.5;
        var sinHalfAngle = Math.sin(halfAngle);
        this.x = rotationAxis.x * sinHalfAngle;
        this.y = rotationAxis.y * sinHalfAngle;
        this.z = rotationAxis.z * sinHalfAngle;
        this.w = Math.cos(halfAngle);
    };
    /**
     * Sets this quaternion to represent the shortest rotation that would rotate 'dirFrom' such that it ends up as 'dirTo'.
     *
     * @param dirFrom A normalized source direction.
     * @param dirTo A normalized target direction.
     */
    Quat.prototype.SetShortestRotation = function (dirFrom, dirTo) {
        var v0 = dirFrom.GetNormalized();
        var v1 = dirTo.GetNormalized();
        var fDot = v0.Dot(v1);
        // if both vectors are identical -> no rotation needed
        if (exports.Utils.IsNumberEqual(fDot, 1.0, 0.0001)) {
            this.SetIdentity();
            return;
        }
        else if (exports.Utils.IsNumberEqual(fDot, -1.0, 0.0001)) // if both vectors are opposing
         {
            // find an axis, that is not identical and not opposing, plVec3Template::Cross-product to find perpendicular vector, rotate around that
            if (Math.abs(v0.x) < 0.8)
                this.SetFromAxisAndAngle(v0.CrossRH(new exports.Vec3(1, 0, 0)).GetNormalized(), Math.PI);
            else
                this.SetFromAxisAndAngle(v0.CrossRH(new exports.Vec3(0, 1, 0)).GetNormalized(), Math.PI);
            return;
        }
        var c = v0.CrossRH(v1);
        var d = v0.Dot(v1);
        var s = Math.sqrt((1.0 + d) * 2.0);
        var invS = 1.0 / s;
        this.x = c.x * invS;
        this.y = c.y * invS;
        this.z = c.z * invS;
        this.w = s * 0.5;
        this.Normalize();
    };
    /**
     * Sets this quaternion to be the spherical linear interpolation of the other two.
     *
     * @param from The quaternion to interpolate from.
     * @param to   The quaternion to interpolate to.
     * @param lerpFactor The interpolation value in range [0; 1]. Ie with 0.5 this will be half-way between 'from' and 'to'.
     */
    Quat.prototype.SetSlerp = function (from, to, lerpFactor) {
        var qDelta = 0.009;
        var cosTheta = (from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w);
        var bFlipSign = false;
        if (cosTheta < 0.0) {
            bFlipSign = true;
            cosTheta = -cosTheta;
        }
        var t0, t1;
        if (cosTheta < qDelta) {
            var theta = Math.acos(cosTheta);
            // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
            var iSinTheta = 1.0 / Math.sqrt(1.0 - (cosTheta * cosTheta));
            var tTheta = lerpFactor * theta;
            var s0 = Math.sin(theta - tTheta);
            var s1 = Math.sin(tTheta);
            t0 = s0 * iSinTheta;
            t1 = s1 * iSinTheta;
        }
        else {
            // If q0 is nearly the same as q1 we just linearly interpolate
            t0 = 1.0 - lerpFactor;
            t1 = lerpFactor;
        }
        if (bFlipSign)
            t1 = -t1;
        this.x = t0 * from.x;
        this.y = t0 * from.y;
        this.z = t0 * from.z;
        this.w = t0 * from.w;
        this.x += t1 * to.x;
        this.y += t1 * to.y;
        this.z += t1 * to.z;
        this.w += t1 * to.w;
        this.Normalize();
    };
    /**
     * Returns two values, a Vec3 that represents the axis through which this quaternion rotates, and a number that is the angle of rotation in radians.
     */
    Quat.prototype.GetRotationAxisAndAngle = function () {
        var acos = Math.acos(this.w);
        var d = Math.sin(acos);
        var axis = new exports.Vec3();
        var angleInRadian;
        if (d < 0.00001) {
            axis.Set(1, 0, 0);
        }
        else {
            var invD = 1.0 / d;
            axis.x = this.x * invD;
            axis.y = this.y * invD;
            axis.z = this.z * invD;
        }
        angleInRadian = acos * 2.0;
        return { axis: axis, angleInRadian: angleInRadian };
    };
    /**
     * Checks whether this and 'other' are approximately the equal rotation.
     */
    Quat.prototype.IsEqualRotation = function (other, epsilon) {
        if (epsilon === void 0) { epsilon = 0.0001; }
        var res1 = this.GetRotationAxisAndAngle();
        var res2 = other.GetRotationAxisAndAngle();
        if (exports.Angle.IsEqualSimple(res1.angleInRadian, res2.angleInRadian, epsilon) && res1.axis.IsEqual(res2.axis, epsilon)) {
            return true;
        }
        if (exports.Angle.IsEqualSimple(res1.angleInRadian, -res2.angleInRadian, epsilon) && res1.axis.IsEqual(res2.axis.GetNegated(), epsilon)) {
            return true;
        }
        return false;
    };
    /**
     * Returns three values in Euler angles (in radians): yaw, pitch and roll
     */
    Quat.prototype.GetAsEulerAngles = function () {
        var yaw;
        var pitch;
        var roll;
        // roll (x-axis rotation)
        var sinR = 2.0 * (this.w * this.x + this.y * this.z);
        var cosR = 1.0 - 2.0 * (this.x * this.x + this.y * this.y);
        roll = Math.atan2(sinR, cosR);
        // pitch (y-axis rotation)
        var sinP = 2.0 * (this.w * this.y - this.z * this.x);
        if (Math.abs(sinP) >= 1.0)
            pitch = Math.abs(Math.PI * 0.5) * Math.sign(sinP);
        else
            pitch = Math.asin(sinP);
        // yaw (z-axis rotation)
        var sinY = 2.0 * (this.w * this.z + this.x * this.y);
        var cosY = 1.0 - 2.0 * (this.y * this.y + this.z * this.z);
        yaw = Math.atan2(sinY, cosY);
        return { yaw: yaw, pitch: pitch, roll: roll };
    };
    /**
     * Sets this from three Euler angles (in radians)
     */
    Quat.prototype.SetFromEulerAngles = function (radianX, radianY, radianZ) {
        var yaw = radianZ;
        var pitch = radianY;
        var roll = radianX;
        var cy = Math.cos(yaw * 0.5);
        var sy = Math.sin(yaw * 0.5);
        var cr = Math.cos(roll * 0.5);
        var sr = Math.sin(roll * 0.5);
        var cp = Math.cos(pitch * 0.5);
        var sp = Math.sin(pitch * 0.5);
        this.w = (cy * cr * cp + sy * sr * sp);
        this.x = (cy * sr * cp - sy * cr * sp);
        this.y = (cy * cr * sp + sy * sr * cp);
        this.z = (sy * cr * cp - cy * sr * sp);
    };
    /**
     * Applies the quaternion's rotation to 'vector' in place.
     */
    Quat.prototype.RotateVec3 = function (vector) {
        // t = cross(this, vector) * 2
        var tx = (this.y * vector.z - this.z * vector.y) * 2.0;
        var ty = (this.z * vector.x - this.x * vector.z) * 2.0;
        var tz = (this.x * vector.y - this.y * vector.x) * 2.0;
        // t2 = cross(this, t)
        var t2x = this.y * tz - this.z * ty;
        var t2y = this.z * tx - this.x * tz;
        var t2z = this.x * ty - this.y * tx;
        vector.x += (tx * this.w) + t2x;
        vector.y += (ty * this.w) + t2y;
        vector.z += (tz * this.w) + t2z;
    };
    /**
     * Applies the quaternion's inverse rotation to 'vector' in place.
     */
    Quat.prototype.InvRotateVec3 = function (vector) {
        // t = cross(this, vector) * 2
        var tx = (-this.y * vector.z + this.z * vector.y) * 2.0;
        var ty = (-this.z * vector.x + this.x * vector.z) * 2.0;
        var tz = (-this.x * vector.y + this.y * vector.x) * 2.0;
        // t2 = cross(this, t)
        var t2x = -this.y * tz + this.z * ty;
        var t2y = -this.z * tx + this.x * tz;
        var t2z = -this.x * ty + this.y * tx;
        vector.x += (tx * this.w) + t2x;
        vector.y += (ty * this.w) + t2y;
        vector.z += (tz * this.w) + t2z;
    };
    /**
    * Concatenates the rotations of 'this' and 'rhs' and writes the result into 'this':
    *   this = this * rhs
    */
    Quat.prototype.ConcatenateRotations = function (rhs) {
        var q = new Quat;
        q.w = this.w * rhs.w - (this.x * rhs.x + this.y * rhs.y + this.z * rhs.z);
        var t1x = rhs.x * this.w;
        var t1y = rhs.y * this.w;
        var t1z = rhs.z * this.w;
        var t2x = this.x * rhs.w;
        var t2y = this.y * rhs.w;
        var t2z = this.z * rhs.w;
        q.x = t1x + t2x;
        q.y = t1y + t2y;
        q.z = t1z + t2z;
        // q.v += Cross(this.v, q2.v)
        q.x += this.y * rhs.z - this.z * rhs.y;
        q.y += this.z * rhs.x - this.x * rhs.z;
        q.z += this.x * rhs.y - this.y * rhs.x;
        this.x = q.x;
        this.y = q.y;
        this.z = q.z;
        this.w = q.w;
    };
    /**
     * Sets 'this' with the concatenated rotation or lhs and rhs:
     *   this = lhs * rhs
     */
    Quat.prototype.SetConcatenatedRotations = function (lhs, rhs) {
        this.SetQuat(lhs);
        this.ConcatenateRotations(rhs);
    };
    /**
     * Checks whether this and rhs are fully identical.
     */
    Quat.prototype.IsIdentical = function (rhs) {
        return this.x == rhs.x &&
            this.y == rhs.y &&
            this.z == rhs.z &&
            this.w == rhs.w;
    };
    return Quat;
}());
exports.Quat = Quat;
