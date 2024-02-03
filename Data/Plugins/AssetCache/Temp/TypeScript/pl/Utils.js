/*SOURCE-HASH:04B86D6543263E28*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var __GameObject = require("./GameObject");
exports.GameObject = __GameObject.GameObject;
var __Component = require("./Component");
exports.Component = __Component.Component;
exports.TypescriptComponent = __Component.TypescriptComponent;
/**
 * Common utility functions.
 */
var Utils;
(function (Utils) {
    /**
     * Returns whether f1 and f2 are equal within a given epsilon.
     */
    function IsNumberEqual(f1, f2, epsilon) {
        return f1 >= f2 - epsilon && f1 <= f2 + epsilon;
    }
    Utils.IsNumberEqual = IsNumberEqual;
    /**
     * Checks whether f1 is within epsilon close to zero.
     */
    function IsNumberZero(f1, epsilon) {
        return f1 >= -epsilon && f1 <= epsilon;
    }
    Utils.IsNumberZero = IsNumberZero;
    /**
     * Computes the hash value for a given string.
     */
    function StringToHash(text) {
        return __CPP_Utils_StringToHash(text);
    }
    Utils.StringToHash = StringToHash;
    /**
     * Returns value clamped to the range [low; high]
     */
    function Clamp(value, low, high) {
        return Math.min(Math.max(value, low), high);
    }
    Utils.Clamp = Clamp;
    /**
     * Returns value clamped to the range [0; 1]
     */
    function Saturate(value) {
        return Math.min(Math.max(value, 0.0), 1.0);
    }
    Utils.Saturate = Saturate;
    /**
     * Returns the linear interpolation between f0 and f1.
     * @param lerpFactor Factor between 0 and 1 that specifies how much to interpolate.
     */
    function LerpNumbers(f0, f1, lerpFactor) {
        return f0 + (f1 - f0) * lerpFactor;
    }
    Utils.LerpNumbers = LerpNumbers;
    /**
     * Returns the root-node inside a prefab hierarchy, which is typically the node one wants to send messages to to interact with the prefab.
     *
     * When a prefab is put into the world, there is a node that represents the prefab, which may have a name or global key to find it by,
     * say it is called 'prefab-proxy'. This node typically holds a single plPrefabReferenceComponent. After prefab instantiation is complete,
     * the content of the prefab asset is attached as children below 'prefab-proxy'. Many prefabs have a single top-level node, but they are allowed
     * to have multiple. To interact with a prefab, it is common for the prefab to have a script at that top-level node. So you either send messages
     * to that, or you query for a specific script component type there.
     * This utility function makes it easier to get to that specific node. If the given 'prefabNode' has exactly one child node (the most common case),
     * it returns that. If it has multiple child nodes, it returns the one with the name 'root'. If 'prefabNode' is null, has no children, or no child with
     * the name 'root' it returns null.
     *
     * @param prefabNode The object to search for the prefab root node. May be null.
     */
    function FindPrefabRootNode(prefabNode) {
        if (prefabNode == null)
            return null;
        return __CPP_Utils_FindPrefabRootNode(prefabNode);
    }
    Utils.FindPrefabRootNode = FindPrefabRootNode;
    /**
     * Similar to pl.Utils.FindPrefabRootNode() but additionally searches for a specific script component.
     *
     * @param prefabNode The GameObject representing the prefab proxy node under which the prefab was instantiated.
     * @param scriptComponentTypeName The name of the script component class to search for.
     *
     * @returns If either 'prefabNode' is null, or the script class name is unknown, or the prefab has no top-level node with
     * the requested script component attached, null is returned. Otherwise, the script component object.
     */
    function FindPrefabRootScript(prefabNode, scriptComponentTypeName) {
        if (prefabNode == null)
            return null;
        return __CPP_Utils_FindPrefabRootScript(prefabNode, scriptComponentTypeName);
    }
    Utils.FindPrefabRootScript = FindPrefabRootScript;
})(Utils = exports.Utils || (exports.Utils = {}));
