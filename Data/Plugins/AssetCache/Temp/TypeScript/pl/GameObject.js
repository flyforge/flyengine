/*SOURCE-HASH:04AB0D15F157D788*/
"use strict";
var __spreadArrays = (this && this.__spreadArrays) || function () {
    for (var s = 0, i = 0, il = arguments.length; i < il; i++) s += arguments[i].length;
    for (var r = Array(s), k = 0, i = 0; i < il; i++)
        for (var a = arguments[i], j = 0, jl = a.length; j < jl; j++, k++)
            r[k] = a[j];
    return r;
};
Object.defineProperty(exports, "__esModule", { value: true });
var __Vec3 = require("./Vec3");
exports.Vec3 = __Vec3.Vec3;
var __Quat = require("./Quat");
exports.Quat = __Quat.Quat;
var __Angle = require("./Angle");
exports.Angle = __Angle.Angle;
var __Time = require("./Time");
exports.Time = __Time.Time;
var __Message = require("./Message");
exports.Message = __Message.Message;
exports.EventMessage = __Message.EventMessage;
var __Component = require("./Component");
exports.Component = __Component.Component;
exports.TypescriptComponent = __Component.TypescriptComponent;
/**
 * Represents a C++ plGameObject on the TypeScript side.
 *
 * This class acts like a weak pointer to the C++ instance. In between game updates, the C++ side object may get deleted.
 * Therefore it is vital to call 'IsValid()' on a GameObject before doing anything else with it. If IsValid returns false,
 * the C++ side plGameObject has been deleted and the TypeScript instance cannot be used anymore either.
 *
 * Be aware that functions that return GameObjects will typically return null objects, in case of failure. They will not return
 * 'invalid' GameObject instances.
 */
var GameObject = /** @class */ (function () {
    function GameObject() {
    }
    // TODO:
    // GetComponents
    // GetChildIterator
    /**
     * If the GameObject is not null, it may still be 'dead' on the C++ side. This function checks whether that is the case.
     *
     * If the object is valid, all other functions can be called, otherwise it is an error to do anything with the GameObject.
     * GameObjects will always stay valid throughout a single game update (end of frame), so it is not necessary to call this
     * more than once per frame.
     */
    GameObject.prototype.IsValid = function () {
        return __CPP_GameObject_IsValid(this);
    };
    /**
     * Changes the name of the GameObject.
     */
    GameObject.prototype.SetName = function (name) {
        __CPP_GameObject_SetName(this, name);
    };
    /**
     * Returns the name of the GameObject.
     */
    GameObject.prototype.GetName = function () {
        return __CPP_GameObject_GetName(this);
    };
    /**
     * Sets the active flag for a game object, which affects the object's final 'active state'.
     * When a GameObject does not have the active flag, it and all its children get deactivated, as well as all attached components.
     */
    GameObject.prototype.SetActiveFlag = function (enabled) {
        __CPP_GameObject_SetActiveFlag(this, enabled);
    };
    /**
     * Returns whether the GameObject is has the active flag set.
     */
    GameObject.prototype.GetActiveFlag = function () {
        return __CPP_GameObject_GetActiveFlag(this);
    };
    /**
     * Returns whether the GameObject is active (it and all its parents have the active flag).
     */
    GameObject.prototype.IsActive = function () {
        return __CPP_GameObject_IsActive(this);
    };
    /**
     * Sets the position of the object relative to its parent object.
     * If the object has no parent, this is the same as the global position.
     * The global transform is updated at the end of the frame, so this change is not reflected in the global transform
     * until the next frame.
     */
    GameObject.prototype.SetLocalPosition = function (pos) {
        __CPP_GameObject_SetLocalPosition(this, pos);
    };
    /**
     * Returns the position relative to the parent object.
     * If the object has no parent, this is the same as the global position.
     */
    GameObject.prototype.GetLocalPosition = function () {
        return __CPP_GameObject_GetLocalPosition(this);
    };
    /**
     * Sets the rotation of the object relative to its parent object.
     * If the object has no parent, this is the same as the global rotation.
     * The global transform is updated at the end of the frame, so this change is not reflected in the global transform
     * until the next frame.
     */
    GameObject.prototype.SetLocalRotation = function (rot) {
        __CPP_GameObject_SetLocalRotation(this, rot);
    };
    /**
     * Returns the rotation relative to the parent object.
     * If the object has no parent, this is the same as the global rotation.
     */
    GameObject.prototype.GetLocalRotation = function () {
        return __CPP_GameObject_GetLocalRotation(this);
    };
    /**
     * Sets the scaling of the object relative to its parent object.
     * If the object has no parent, this is the same as the global scale.
     * The global transform is updated at the end of the frame, so this change is not reflected in the global transform
     * until the next frame.
     */
    GameObject.prototype.SetLocalScaling = function (scaling) {
        __CPP_GameObject_SetLocalScaling(this, scaling);
    };
    /**
     * Returns the scaling relative to the parent object.
     * If the object has no parent, this is the same as the global scaling.
     */
    GameObject.prototype.GetLocalScaling = function () {
        return __CPP_GameObject_GetLocalScaling(this);
    };
    /**
     * Sets the uniform scaling of the object relative to its parent object.
     * If the object has no parent, this is the same as the global uniform scale.
     * The global transform is updated at the end of the frame, so this change is not reflected in the global transform
     * until the next frame.
     */
    GameObject.prototype.SetLocalUniformScaling = function (scaling) {
        __CPP_GameObject_SetLocalUniformScaling(this, scaling);
    };
    /**
     * Returns the uniform scaling relative to the parent object.
     * If the object has no parent, this is the same as the global uniform scaling.
     */
    GameObject.prototype.GetLocalUniformScaling = function () {
        return __CPP_GameObject_GetLocalUniformScaling(this);
    };
    /**
     * Sets the object's global position.
     * Internally this will set the local position such that the desired global position is reached.
     */
    GameObject.prototype.SetGlobalPosition = function (pos) {
        __CPP_GameObject_SetGlobalPosition(this, pos);
    };
    /**
     * Returns the current global position as computed from the local transforms.
     */
    GameObject.prototype.GetGlobalPosition = function () {
        return __CPP_GameObject_GetGlobalPosition(this);
    };
    /**
     * Sets the object's global rotation.
     * Internally this will set the local rotation such that the desired global rotation is reached.
     */
    GameObject.prototype.SetGlobalRotation = function (rot) {
        __CPP_GameObject_SetGlobalRotation(this, rot);
    };
    /**
     * Returns the current global rotation as computed from the local transforms.
     */
    GameObject.prototype.GetGlobalRotation = function () {
        return __CPP_GameObject_GetGlobalRotation(this);
    };
    /**
     * Sets the object's scaling position.
     * Internally this will set the local scaling such that the desired global scaling is reached.
     */
    GameObject.prototype.SetGlobalScaling = function (scaling) {
        __CPP_GameObject_SetGlobalScaling(this, scaling);
    };
    /**
     * Returns the current global scaling as computed from the local transforms.
     * Note that there is no global uniform scaling as the local uniform scaling and non-uniform scaling are
     * combined into the global scaling.
     */
    GameObject.prototype.GetGlobalScaling = function () {
        return __CPP_GameObject_GetGlobalScaling(this);
    };
    /**
     * Returns the vector representing the logical 'forward' direction of the GameObject in global space.
     */
    GameObject.prototype.GetGlobalDirForwards = function () {
        return __CPP_GameObject_GetGlobalDirForwards(this);
    };
    /**
     * Returns the vector representing the logical 'right' direction of the GameObject in global space.
     */
    GameObject.prototype.GetGlobalDirRight = function () {
        return __CPP_GameObject_GetGlobalDirRight(this);
    };
    /**
     * Returns the vector representing the logical 'up' direction of the GameObject in global space.
     */
    GameObject.prototype.GetGlobalDirUp = function () {
        return __CPP_GameObject_GetGlobalDirUp(this);
    };
    /**
     * Returns the linear velocity of the object over the last two world updates.
     */
    GameObject.prototype.GetLinearVelocity = function () {
        return __CPP_GameObject_GetLinearVelocity(this);
    };
    /**
     * Sets the team ID of this GameObject.
     *
     * The team ID can be used to identify to which team or player an object belongs to.
     * The team ID is inherited to other GameObjects, e.g. when spawning new objects.
     * Thus for instance a projectile inherits the team ID of the weapon or player that spawned it.
     *
     * @param id The team ID must be in range [0; 65535] (uint16).
     */
    GameObject.prototype.SetTeamID = function (id) {
        __CPP_GameObject_SetTeamID(this, id);
    };
    /**
     * Returns the object's team ID.
     */
    GameObject.prototype.GetTeamID = function () {
        return __CPP_GameObject_GetTeamID(this);
    };
    /**
     * Searches for a child GameObject with the given name.
     *
     * @param name The expected exact name of the child object.
     * @param recursive If false, only direct children are inspected. Otherwise recursively all children are inspected.
     */
    GameObject.prototype.FindChildByName = function (name, recursive) {
        if (recursive === void 0) { recursive = true; }
        return __CPP_GameObject_FindChildByName(this, name, recursive);
    };
    /**
     * Searches for a child using a path. Every path segment represents a child with a given name.
     *
     * Paths are separated with single slashes: /
     * When an empty path is given, 'this' is returned.
     * When on any part of the path the next child cannot be found, null is returned.
     * This function expects an exact path to the destination. It does not search the full hierarchy for
     * the next child, as SearchChildByNameSequence() does.
     */
    GameObject.prototype.FindChildByPath = function (path) {
        return __CPP_GameObject_FindChildByPath(this, path);
    };
    /**
     * Searches for a child similar to FindChildByName() but allows to search for multiple names in a sequence.
     *
     * The names in the sequence are separated with slashes.
     * For example, calling this with "a/b" will first search the entire hierarchy below this object for a child
     * named "a". If that is found, the search continues from there for a child called "b".
     */
    GameObject.prototype.SearchForChildByNameSequence = function (objectSequence) {
        return __CPP_GameObject_SearchForChildByNameSequence(this, objectSequence, 0);
    };
    /**
     * Searches for a child similar to FindChildByName() but allows to search for multiple names in a sequence.
     *
     * The names in the sequence are separated with slashes.
     * For example, calling this with "a/b" will first search the entire hierarchy below this object for a child
     * named "a". If that is found, the search continues from there for a child called "b".
     * If such a child is found it is verified that the object contains a component of 'typeClass'.
     * If it doesn't the search continues (including back-tracking).
     */
    GameObject.prototype.SearchForChildWithComponentByNameSequence = function (objectSequence, typeClass) {
        return __CPP_GameObject_SearchForChildByNameSequence(this, objectSequence, typeClass.GetTypeNameHash());
    };
    /**
     * Tries to find a component of type 'typeName' in the object's components list and returns the first match.
     *
     * This function only works for C++ components, not for script components (derived from TypescriptComponent).
     * Use TryGetScriptComponent() for such use cases instead.
     */
    GameObject.prototype.TryGetComponentOfBaseTypeName = function (typeName) {
        return __CPP_GameObject_TryGetComponentOfBaseTypeName(this, typeName);
    };
    /**
     * Tries to find a component of type 'typeClass' in the object's components list and returns the first match.
     *
     * This function only works for C++ components, not for script components (derived from TypescriptComponent).
     * Use TryGetScriptComponent() for such use cases instead.
     */
    GameObject.prototype.TryGetComponentOfBaseType = function (typeClass) {
        return __CPP_GameObject_TryGetComponentOfBaseTypeNameHash(this, typeClass.GetTypeNameHash());
    };
    /**
     * Similar to TryGetComponentOfBaseType() but tries to find a component type written in TypeScript.
     *
     * See also pl.Utils.FindPrefabRootScript() for the common use case to finding the script of a prefab instance.
     *
     * @param typeName The name of the TypeScript component to find.
     */
    GameObject.prototype.TryGetScriptComponent = function (typeName) {
        return __CPP_GameObject_TryGetScriptComponent(this, typeName);
    };
    /**
     * Sends a message to all the components on this GameObject (but not its children).
     *
     * The message is delivered immediately.
     *
     * @param expectResultData If set to true, the calling code assumes that the message receiver(s) may write result data
     *   back into the message and thus the caller is interested in reading that data afterwards. If set to false
     *   (the default) the state of the message is not synchronized back into the TypeScript message after the message
     *   has been delivered and thus any data written into the message by the receiver, is lost.
     */
    GameObject.prototype.SendMessage = function (msg, expectResultData) {
        if (expectResultData === void 0) { expectResultData = false; }
        __CPP_GameObject_SendMessage(this, msg.TypeNameHash, msg, false, expectResultData);
    };
    /**
     * Sends a message to all the components on this GameObject (including all its children).
     *
     * The message is delivered immediately.
     *
     * @param expectResultData If set to true, the calling code assumes that the message receiver(s) may write result data
     *   back into the message and thus the caller is interested in reading that data afterwards. If set to false
     *   (the default) the state of the message is not synchronized back into the TypeScript message after the message
     *   has been delivered and thus any data written into the message by the receiver, is lost.
     */
    GameObject.prototype.SendMessageRecursive = function (msg, expectResultData) {
        if (expectResultData === void 0) { expectResultData = false; }
        __CPP_GameObject_SendMessage(this, msg.TypeNameHash, msg, true, expectResultData);
    };
    /**
     * Queues a message to be sent to all the components on this GameObject (but not its children).
     *
     * The message is delivered after the timeout.
     * If the timeout is zero, the message is delivered in the next frame.
     */
    GameObject.prototype.PostMessage = function (msg, delay) {
        if (delay === void 0) { delay = exports.Time.Zero(); }
        __CPP_GameObject_PostMessage(this, msg.TypeNameHash, msg, false, delay);
    };
    /**
     * Queues a message to be sent to all the components on this GameObject (including all its children).
     *
     * The message is delivered after the timeout.
     * If the timeout is zero, the message is delivered in the next frame.
     */
    GameObject.prototype.PostMessageRecursive = function (msg, delay) {
        if (delay === void 0) { delay = exports.Time.Zero(); }
        __CPP_GameObject_PostMessage(this, msg.TypeNameHash, msg, true, delay);
    };
    /**
     * Sends an *event message* up the object hierarchy to the closest event handler (typically another script).
     *
     * The message is delivered immediately.
     *
     * @param expectResultData If set to true, the calling code assumes that the message receiver(s) may write result data
     *   back into the message and thus the caller is interested in reading that data afterwards. If set to false
     *   (the default) the state of the message is not synchronized back into the TypeScript message after the message
     *   has been delivered and thus any data written into the message by the receiver, is lost.
     */
    GameObject.prototype.SendEventMessage = function (msg, sender, expectMsgResult) {
        if (expectMsgResult === void 0) { expectMsgResult = false; }
        __CPP_GameObject_SendEventMessage(this, msg.TypeNameHash, msg, sender, expectMsgResult);
    };
    /**
     * Queues an *event message* to be sent up the object hierarchy to the closest event handler (typically another script).
     *
     * The message is delivered after the timeout.
     * If the timeout is zero, the message is delivered in the next frame.
     */
    GameObject.prototype.PostEventMessage = function (msg, sender, delay) {
        if (delay === void 0) { delay = exports.Time.Zero(); }
        __CPP_GameObject_PostEventMessage(this, msg.TypeNameHash, msg, sender, delay);
    };
    /**
     * Replaces all the tags on this GameObject with the given set of tags.
     */
    GameObject.prototype.SetTags = function () {
        var tags = [];
        for (var _i = 0; _i < arguments.length; _i++) {
            tags[_i] = arguments[_i];
        }
        __CPP_GameObject_SetTags.apply(void 0, __spreadArrays([this], tags));
    };
    /**
     * Adds all the given tags to this GameObject.
     */
    GameObject.prototype.AddTags = function () {
        var tags = [];
        for (var _i = 0; _i < arguments.length; _i++) {
            tags[_i] = arguments[_i];
        }
        __CPP_GameObject_AddTags.apply(void 0, __spreadArrays([this], tags));
    };
    /**
     * Removes all the given tags from this GameObject.
     * Ignores tags that were not set on this GameObject to begin with.
     */
    GameObject.prototype.RemoveTags = function () {
        var tags = [];
        for (var _i = 0; _i < arguments.length; _i++) {
            tags[_i] = arguments[_i];
        }
        __CPP_GameObject_RemoveTags.apply(void 0, __spreadArrays([this], tags));
    };
    /**
     * Checks whether this object has at least on of the given tags.
     */
    GameObject.prototype.HasAnyTags = function () {
        var tags = [];
        for (var _i = 0; _i < arguments.length; _i++) {
            tags[_i] = arguments[_i];
        }
        return __CPP_GameObject_HasAnyTags.apply(void 0, __spreadArrays([this], tags));
    };
    /**
     * Checks whether this object has all the given tags.
     */
    GameObject.prototype.HasAllTags = function () {
        var tags = [];
        for (var _i = 0; _i < arguments.length; _i++) {
            tags[_i] = arguments[_i];
        }
        return __CPP_GameObject_HasAllTags.apply(void 0, __spreadArrays([this], tags));
    };
    /**
     * Changes the global key of the GameObject.
     */
    GameObject.prototype.SetGlobalKey = function (name) {
        __CPP_GameObject_SetGlobalKey(this, name);
    };
    /**
     * Returns the global key of the GameObject.
     */
    GameObject.prototype.GetGlobalKey = function () {
        return __CPP_GameObject_GetGlobalKey(this);
    };
    /**
     * Returns the parent game object or null if this object has no parent.
     */
    GameObject.prototype.GetParent = function () {
        return __CPP_GameObject_GetParent(this);
    };
    /**
     * Attaches this object to another game object as its child.
     *
     * @param parent The object to attach this object to.
     * @param preserveGlobalTransform If true, the global transform of this is preserved and the local transform is adjusted as needed.
     *  If false, the local transform is preserved and the global transform is computed accordingly.
     */
    GameObject.prototype.SetParent = function (parent, preserveGlobalTransform) {
        if (preserveGlobalTransform === void 0) { preserveGlobalTransform = true; }
        __CPP_GameObject_SetParent(this, parent, preserveGlobalTransform);
    };
    /**
     * Attaches the given object to this object as a child.
     *
     * @param child The object to attach to this object.
     * @param preserveGlobalTransform If true, the global transform of the child is preserved and the local transform is adjusted as needed.
     *  If false, the local transform is preserved and the global transform is computed accordingly.
     */
    GameObject.prototype.AddChild = function (child, preserveGlobalTransform) {
        if (preserveGlobalTransform === void 0) { preserveGlobalTransform = true; }
        __CPP_GameObject_AddChild(this, child, preserveGlobalTransform);
    };
    /**
     * Detaches the given child object from this object.
     * This is similar to child.SetParent(null), but only detaches the child, if it is indeed attached to this object.
     *
     * @param child The object to detach from this object.
     * @param preserveGlobalTransform If true, the global transform of the child is preserved and the local transform is adjusted as needed.
     *  If false, the local transform is preserved and the global transform is computed accordingly.
     */
    GameObject.prototype.DetachChild = function (child, preserveGlobalTransform) {
        if (preserveGlobalTransform === void 0) { preserveGlobalTransform = true; }
        __CPP_GameObject_DetachChild(this, child, preserveGlobalTransform);
    };
    /**
     * Returns the number of objects attached to this as children.
     */
    GameObject.prototype.GetChildCount = function () {
        return __CPP_GameObject_GetChildCount(this);
    };
    /**
     * Returns all the child objects in an array.
     */
    GameObject.prototype.GetChildren = function () {
        return __CPP_GameObject_GetChildren(this);
    };
    return GameObject;
}());
exports.GameObject = GameObject;
