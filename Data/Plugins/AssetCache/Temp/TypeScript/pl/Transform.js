/*SOURCE-HASH:2BFF60DB75790729*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var __Utils = require("./Utils");
exports.Utils = __Utils.Utils;
var __Vec3 = require("./Vec3");
exports.Vec3 = __Vec3.Vec3;
var __Quat = require("./Quat");
exports.Quat = __Quat.Quat;
var __Mat4 = require("./Mat4");
exports.Mat4 = __Mat4.Mat4;
/**
 * A 'transform' represents a position/translation, rotation and scaling with dedicated values for each.
 */
var Transform = /** @class */ (function () {
    function Transform() {
        this.position = new exports.Vec3(); /** Identity translation (none) by default */
        this.rotation = new exports.Quat(); /** Identity rotation by default */
        this.scale = new exports.Vec3(1, 1, 1); /** Identity scaling (one) by default */
    }
    /**
     * Returns a duplicate of this transform.
     */
    Transform.prototype.Clone = function () {
        var clone = new Transform();
        clone.position = this.position.Clone();
        clone.rotation = this.rotation.Clone();
        clone.scale = this.scale.Clone();
        return clone;
    };
    /**
     * Copies the transform values from rhs.
     */
    Transform.prototype.SetTransform = function (rhs) {
        this.position = rhs.position.Clone();
        this.rotation = rhs.rotation.Clone();
        this.scale = rhs.scale.Clone();
    };
    /**
     * Sets this to be the identity transform.
     */
    Transform.prototype.SetIdentity = function () {
        this.position.SetZero();
        this.rotation.SetIdentity();
        this.scale.SetAll(1);
    };
    /**
     * Returns an identity transform.
     */
    Transform.IdentityTransform = function () {
        return new Transform();
    };
    /**
     * Checks whether this and rhs are identical.
     */
    Transform.prototype.IsIdentical = function (rhs) {
        return this.position.IsIdentical(rhs.position) && this.rotation.IsIdentical(rhs.rotation) && this.scale.IsIdentical(rhs.scale);
    };
    /**
     * Checks whether this and rhs are approximately equal.
     */
    Transform.prototype.IsEqual = function (rhs, epsilon) {
        if (epsilon === void 0) { epsilon = 0.0001; }
        return this.position.IsEqual(rhs.position, epsilon) && this.rotation.IsEqualRotation(rhs.rotation, epsilon) && this.scale.IsEqual(rhs.scale, epsilon);
    };
    /**
     * Inverts the transformation.
     */
    Transform.prototype.Invert = function () {
        this.rotation.Negate();
        this.scale.x = 1.0 / this.scale.x;
        this.scale.y = 1.0 / this.scale.y;
        this.scale.z = 1.0 / this.scale.z;
        this.position.Negate();
        this.position.MulVec3(this.scale);
        this.rotation.RotateVec3(this.position);
    };
    /**
     * Returns the inverse transformation.
     */
    Transform.prototype.GetInverse = function () {
        var invTransform = this.Clone();
        invTransform.Invert();
        return invTransform;
    };
    /**
     * Modifies 'pos' in place and treats it like a position vector, ie. with an implied w-component of 1.
     * Thus the translation part of the transform is applied.
     */
    Transform.prototype.TransformPosition = function (pos) {
        pos.MulVec3(this.scale);
        this.rotation.RotateVec3(pos);
        pos.AddVec3(this.position);
    };
    /**
     * Modifies 'dir' in place and treats it like a direction vector, ie. with an implied w-component of 0.
     * Thus the translation part of the transform is NOT applied, only rotation and scaling.
     */
    Transform.prototype.TransformDirection = function (dir) {
        dir.MulVec3(this.scale);
        this.rotation.RotateVec3(dir);
    };
    /**
     * Adds more translation to this transform.
     */
    Transform.prototype.Translate = function (movePos) {
        this.position.AddVec3(movePos);
    };
    /**
     * Sets this transform to be the local transformation needed to get from the parent's transform to the child's.
     */
    Transform.prototype.SetLocalTransform = function (globalTransformParent, globalTransformChild) {
        var invScale = new exports.Vec3(1, 1, 1);
        invScale.DivVec3(globalTransformParent.scale);
        this.position.SetSub(globalTransformChild.position, globalTransformParent.position);
        globalTransformParent.rotation.InvRotateVec3(this.position);
        this.position.MulVec3(invScale);
        this.rotation.SetQuat(globalTransformParent.rotation);
        this.rotation.Negate();
        this.rotation.ConcatenateRotations(globalTransformChild.rotation);
        this.scale.SetVec3(invScale);
        this.scale.MulVec3(globalTransformChild.scale);
    };
    /**
     * Sets this transform to the global transform, that is reached by applying the child's local transform to the parent's global one.
     */
    Transform.prototype.SetGlobalTransform = function (globalTransformParent, localTransformChild) {
        this.SetMulTransform(globalTransformParent, localTransformChild);
    };
    /**
     * Sets this transform to be the concatenated transformation of this and rhs.
     *   this = this * rhs
     */
    Transform.prototype.MulTransform = function (rhs) {
        var tmp1 = this.scale.Clone();
        tmp1.MulVec3(rhs.position);
        this.rotation.RotateVec3(tmp1);
        this.position.AddVec3(tmp1);
        this.rotation.ConcatenateRotations(rhs.rotation);
        this.scale.MulVec3(rhs.scale);
    };
    /**
     * Sets this transform to be the concatenated transformation of lhs and rhs.
     *   this = lhs * rhs
     */
    Transform.prototype.SetMulTransform = function (lhs, rhs) {
        this.SetTransform(lhs);
        this.MulTransform(rhs);
    };
    /**
     * Modifies this rotation to be the concatenation of this and rhs.
     *   this.rotation = this.rotation * rhs
     */
    Transform.prototype.ConcatenateRotations = function (rhs) {
        this.rotation.ConcatenateRotations(rhs);
    };
    /**
     * Modifies this rotation to be the reverse concatenation of this and rhs.
     *   this.rotation = lhs * this.rotation
     */
    Transform.prototype.ConcatenateRotationsReverse = function (lhs) {
        this.rotation.SetConcatenatedRotations(lhs, this.rotation.Clone());
    };
    /**
     * Returns the transformation as a matrix.
     */
    Transform.prototype.GetAsMat4 = function () {
        var result = this.rotation.GetAsMat4();
        result.m_ElementsCM[0] *= this.scale.x;
        result.m_ElementsCM[1] *= this.scale.x;
        result.m_ElementsCM[2] *= this.scale.x;
        result.m_ElementsCM[4] *= this.scale.y;
        result.m_ElementsCM[5] *= this.scale.y;
        result.m_ElementsCM[6] *= this.scale.y;
        result.m_ElementsCM[8] *= this.scale.z;
        result.m_ElementsCM[9] *= this.scale.z;
        result.m_ElementsCM[10] *= this.scale.z;
        result.m_ElementsCM[12] = this.position.x;
        result.m_ElementsCM[13] = this.position.y;
        result.m_ElementsCM[14] = this.position.z;
        return result;
    };
    /**
     * Attempts to extract position, scale and rotation from the matrix. Negative scaling and shearing will get lost in the process.
     */
    Transform.prototype.SetFromMat4 = function (mat) {
        var mRot = mat.GetRotationalPart();
        this.position.SetVec3(mat.GetTranslationVector());
        this.scale.SetVec3(mRot.GetScalingFactors());
        mRot.SetScalingFactors(1, 1, 1);
        this.rotation.SetFromMat3(mRot);
    };
    return Transform;
}());
exports.Transform = Transform;
