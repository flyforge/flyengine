/*SOURCE-HASH:71A77B211214576F*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var __GameObject = require("./GameObject");
exports.GameObject = __GameObject.GameObject;
var __Component = require("./Component");
exports.Component = __Component.Component;
var __Vec3 = require("./Vec3");
exports.Vec3 = __Vec3.Vec3;
var __Quat = require("./Quat");
exports.Quat = __Quat.Quat;
var __Color = require("./Color");
exports.Color = __Color.Color;
var __Transform = require("./Transform");
exports.Transform = __Transform.Transform;
/**
 * Describes the properties of a to-be-created GameObject.
 */
var GameObjectDesc = /** @class */ (function () {
    function GameObjectDesc() {
        this.ActiveFlag = true; /** Whether the GO should start in an active state. */
        this.Dynamic = false; /** Whether the GO should be considered 'dynamic', ie. can be moved around. */
        this.TeamID = 0; /** The team index to give to this GO. */
        this.Parent = null; /** An optional parent object to attach the new GO to. */
        this.LocalPosition = null; /** The local position offset to the parent. Default is (0, 0, 0). */
        this.LocalRotation = null; /** The local rotation offset to the parent. Default is identity. */
        this.LocalScaling = null; /** The local scaling offset to the parent. Default is (1, 1, 1). */
        this.LocalUniformScaling = 1; /** The local uniform scaling offset to the parent. Default is 1. */
    }
    return GameObjectDesc;
}());
exports.GameObjectDesc = GameObjectDesc;
;
/**
 * Functions to modify or interact with the world / scenegraph.
 */
var World;
(function (World) {
    /**
     * Creates a new GameObject on the C++ side and returns a TypeScript GameObject that links to that.
     */
    function CreateObject(desc) {
        return __CPP_World_CreateObject(desc);
    }
    World.CreateObject = CreateObject;
    /**
     * Queues the given GO for deletion at the end of the frame.
     * This is the typical way to delete objects, they stay alive until the end of the frame, to guarantee that GOs never disappear in the middle of a frame.
     *
     * @param object The object to be deleted.
     */
    function DeleteObjectDelayed(object) {
        __CPP_World_DeleteObjectDelayed(object);
    }
    World.DeleteObjectDelayed = DeleteObjectDelayed;
    /**
     * Creates a new component of the given type and attaches it to the given GameObject.
     *
     * Example:
     *   let slider = pl.World.CreateComponent(someGameObject, pl.SliderComponent);
     *
     * @param owner The GameObject to attach the component to.
     * @param typeClass The component class type to instantiate.
     */
    function CreateComponent(owner, typeClass) {
        return __CPP_World_CreateComponent(owner, typeClass.GetTypeNameHash());
    }
    World.CreateComponent = CreateComponent;
    /**
     * Instructs the C++ side to delete the given component.
     */
    function DeleteComponent(component) {
        __CPP_World_DeleteComponent(component);
    }
    World.DeleteComponent = DeleteComponent;
    /**
     * Searches the world for a game object with the given 'global key'.
     * Global keys must be unique within a world, thus this lookup is fast. However, working with global keys
     * can be messy and ensuring a global key is never used twice can be difficult, therefore it is advised to
     * use this concept with care.
     */
    function TryGetObjectWithGlobalKey(globalKey) {
        return __CPP_World_TryGetObjectWithGlobalKey(globalKey);
    }
    World.TryGetObjectWithGlobalKey = TryGetObjectWithGlobalKey;
    /**
     * Searches for objects with a specified type category that overlap with the given sphere.
     *
     * @param type The object type category to search for. See plMarkerComponent for a way to attach a type category to an object.
     * @param center World-space center of the sphere.
     * @param radius Radius of the sphere.
     * @param callback A function that is used to report every overlapping GameObject.
     *                 To pass in a member function that has access to your 'this' object, declare your callback like this:
     *                 FoundObjectCallback = (go: pl.GameObject): boolean => { ... }
     *                 As long as the callack returns 'true', further results will be delivered. Return 'false' to cancel.
     */
    function FindObjectsInSphere(type, center, radius, callback) {
        __CPP_World_FindObjectsInSphere(type, center, radius, callback);
    }
    World.FindObjectsInSphere = FindObjectsInSphere;
    /**
     * Searches for objects with a specified type category that overlap with the given box.
     *
     * @param type The object type category to search for. See plMarkerComponent for a way to attach a type category to an object.
     * @param min The minimum vertex of the AABB.
     * @param max The maximum vertex of the AABB.
     * @param callback A function that is used to report every overlapping GameObject.
     *                 To pass in a member function that has access to your 'this' object, declare your callback like this:
     *                 FoundObjectCallback = (go: pl.GameObject): boolean => { ... }
     *                 As long as the callack returns 'true', further results will be delivered. Return 'false' to cancel.
     */
    function FindObjectsInBox(type, min, max, callback) {
        __CPP_World_FindObjectsInBox(type, min, max, callback);
    }
    World.FindObjectsInBox = FindObjectsInBox;
})(World = exports.World || (exports.World = {}));
;
