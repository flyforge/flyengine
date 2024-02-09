/*SOURCE-HASH:A829C8CD19C451AC*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var __Vec3 = require("./Vec3");
exports.Vec3 = __Vec3.Vec3;
var __Transform = require("./Transform");
exports.Transform = __Transform.Transform;
var __GameObject = require("./GameObject");
exports.GameObject = __GameObject.GameObject;
/**
 * Functions in this module are typically implemented by a physics engine and operate on the physics representation of the world,
 * which may be more simplified than the graphical representation.
 */
var Physics;
(function (Physics) {
    /**
     * Static physics shapes never move. In exchange, they cost much less performance.
     * Dynamic physics shapes are simulated according to rigid body dynamics and thus fall down, collide and interact with other dynamic shapes.
     * Query shapes do not participate in the simulation, but may represent objects that can be detected via raycasts and overlap queries.
     */
    var ShapeType;
    (function (ShapeType) {
        ShapeType[ShapeType["Static"] = 1] = "Static";
        ShapeType[ShapeType["Dynamic"] = 2] = "Dynamic";
        ShapeType[ShapeType["Query"] = 4] = "Query";
        ShapeType[ShapeType["Trigger"] = 8] = "Trigger";
        ShapeType[ShapeType["Character"] = 16] = "Character";
        ShapeType[ShapeType["Ragdoll"] = 32] = "Ragdoll";
        ShapeType[ShapeType["Rope"] = 64] = "Rope";
        ShapeType[ShapeType["AllInteractive"] = 118] = "AllInteractive";
    })(ShapeType = Physics.ShapeType || (Physics.ShapeType = {}));
    var HitResult = /** @class */ (function () {
        function HitResult() {
        }
        return HitResult;
    }());
    Physics.HitResult = HitResult;
    ;
    /**
     * Casts a ray in the physics world. Returns a HitResult for the closest object that was hit, or null if no object was hit.
     *
     * @param start The start position of the ray in global space.
     * @param dir The direction into which to cast the ray. Does not need to be normalized.
     * @param distance The length of the ray. Objects farther away than this cannot be hit.
     * @param collisionLayer The index of the collision layer to use, thus describing which objects can be hit by the raycast at all.
     * @param shapeTypes Wether to raycast against static or dynamic shapes, or both.
     * @param ignoreShapeId A single shape ID can be given to be ignored. This can be used, for instance, to filter out the own character controller capsule.
     * @returns A HitResult object or null.
     */
    function Raycast(start, dir, distance, collisionLayer, shapeTypes, ignoreShapeId) {
        if (shapeTypes === void 0) { shapeTypes = ShapeType.Static | ShapeType.Dynamic; }
        if (ignoreShapeId === void 0) { ignoreShapeId = -1; }
        return __CPP_Physics_Raycast(start, dir, distance, collisionLayer, shapeTypes, ignoreShapeId);
    }
    Physics.Raycast = Raycast;
    /**
     * Sweeps a sphere from a position along a direction for a maximum distance and reports the first shape that was hit along the way.
     * @returns A HitResult object or null.
     */
    function SweepTestSphere(sphereRadius, start, dir, distance, collisionLayer, shapeTypes, ignoreShapeId) {
        if (shapeTypes === void 0) { shapeTypes = ShapeType.Static | ShapeType.Dynamic; }
        if (ignoreShapeId === void 0) { ignoreShapeId = -1; }
        return __CPP_Physics_SweepTestSphere(sphereRadius, start, dir, distance, collisionLayer, shapeTypes, ignoreShapeId);
    }
    Physics.SweepTestSphere = SweepTestSphere;
    /**
     * Sweeps a box from a position along a direction for a maximum distance and reports the first shape that was hit along the way.
     * @returns A HitResult object or null.
     */
    function SweepTestBox(boxExtends, start, dir, distance, collisionLayer, shapeTypes, ignoreShapeId) {
        if (shapeTypes === void 0) { shapeTypes = ShapeType.Static | ShapeType.Dynamic; }
        if (ignoreShapeId === void 0) { ignoreShapeId = -1; }
        return __CPP_Physics_SweepTestBox(boxExtends, start, dir, distance, collisionLayer, shapeTypes, ignoreShapeId);
    }
    Physics.SweepTestBox = SweepTestBox;
    /**
     * Sweeps a capsule from a position along a direction for a maximum distance and reports the first shape that was hit along the way.
     * @returns A HitResult object or null.
     */
    function SweepTestCapsule(capsuleRadius, capsuleHeight, start, dir, distance, collisionLayer, shapeTypes, ignoreShapeId) {
        if (shapeTypes === void 0) { shapeTypes = ShapeType.Static | ShapeType.Dynamic; }
        if (ignoreShapeId === void 0) { ignoreShapeId = -1; }
        return __CPP_Physics_SweepTestCapsule(capsuleRadius, capsuleHeight, start, dir, distance, collisionLayer, shapeTypes, ignoreShapeId);
    }
    Physics.SweepTestCapsule = SweepTestCapsule;
    /**
     * Checks whether any shape overlaps with the given sphere.
     */
    function OverlapTestSphere(sphereRadius, position, collisionLayer, shapeTypes, ignoreShapeId) {
        if (shapeTypes === void 0) { shapeTypes = ShapeType.Static | ShapeType.Dynamic; }
        if (ignoreShapeId === void 0) { ignoreShapeId = -1; }
        return __CPP_Physics_OverlapTestSphere(sphereRadius, position, collisionLayer, shapeTypes, ignoreShapeId);
    }
    Physics.OverlapTestSphere = OverlapTestSphere;
    /**
     * Checks whether any shape overlaps with the given capsule.
     */
    function OverlapTestCapsule(capsuleRadius, capsuleHeight, transform, collisionLayer, shapeTypes, ignoreShapeId) {
        if (shapeTypes === void 0) { shapeTypes = ShapeType.Static | ShapeType.Dynamic; }
        if (ignoreShapeId === void 0) { ignoreShapeId = -1; }
        return __CPP_Physics_OverlapTestCapsule(capsuleRadius, capsuleHeight, transform, collisionLayer, shapeTypes, ignoreShapeId);
    }
    Physics.OverlapTestCapsule = OverlapTestCapsule;
    /**
     * Returns the currently set gravity.
     */
    function GetGravity() {
        return __CPP_Physics_GetGravity();
    }
    Physics.GetGravity = GetGravity;
    /**
     * Reports all dynamic shapes found within the given sphere, using a callback function.
     */
    function QueryShapesInSphere(radius, position, collisionLayer, shapeTypes, callback, ignoreShapeId) {
        if (shapeTypes === void 0) { shapeTypes = ShapeType.AllInteractive; }
        if (ignoreShapeId === void 0) { ignoreShapeId = -1; }
        __CPP_Physics_QueryShapesInSphere(radius, position, collisionLayer, shapeTypes, ignoreShapeId, callback);
    }
    Physics.QueryShapesInSphere = QueryShapesInSphere;
})(Physics = exports.Physics || (exports.Physics = {}));
