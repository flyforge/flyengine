/*SOURCE-HASH:417C7AF44892366D*/
"use strict";
var __spreadArrays = (this && this.__spreadArrays) || function () {
    for (var s = 0, i = 0, il = arguments.length; i < il; i++) s += arguments[i].length;
    for (var r = Array(s), k = 0, i = 0; i < il; i++)
        for (var a = arguments[i], j = 0, jl = a.length; j < jl; j++, k++)
            r[k] = a[j];
    return r;
};
Object.defineProperty(exports, "__esModule", { value: true });
var __Vec2 = require("./Vec2");
exports.Vec2 = __Vec2.Vec2;
var __Vec3 = require("./Vec3");
exports.Vec3 = __Vec3.Vec3;
var __Quat = require("./Quat");
exports.Quat = __Quat.Quat;
var __Color = require("./Color");
exports.Color = __Color.Color;
var __Transform = require("./Transform");
exports.Transform = __Transform.Transform;
/**
 * Debug visualization functionality.
 */
var Debug;
(function (Debug) {
    var HorizontalAlignment;
    (function (HorizontalAlignment) {
        HorizontalAlignment[HorizontalAlignment["Left"] = 0] = "Left";
        HorizontalAlignment[HorizontalAlignment["Center"] = 1] = "Center";
        HorizontalAlignment[HorizontalAlignment["Right"] = 2] = "Right";
    })(HorizontalAlignment = Debug.HorizontalAlignment || (Debug.HorizontalAlignment = {}));
    var ScreenPlacement;
    (function (ScreenPlacement) {
        ScreenPlacement[ScreenPlacement["TopLeft"] = 0] = "TopLeft";
        ScreenPlacement[ScreenPlacement["TopCenter"] = 1] = "TopCenter";
        ScreenPlacement[ScreenPlacement["TopRight"] = 2] = "TopRight";
        ScreenPlacement[ScreenPlacement["BottomLeft"] = 3] = "BottomLeft";
        ScreenPlacement[ScreenPlacement["BottomCenter"] = 4] = "BottomCenter";
        ScreenPlacement[ScreenPlacement["BottomRight"] = 5] = "BottomRight";
    })(ScreenPlacement = Debug.ScreenPlacement || (Debug.ScreenPlacement = {}));
    // TODO:
    // DrawLineBoxCorners
    // DrawLineCapsuleZ
    // DrawLineFrustum
    function GetResolution() {
        return __CPP_Debug_GetResolution();
    }
    Debug.GetResolution = GetResolution;
    /**
     * Renders a cross of three lines at the given position.
     *
     * @param pos Position in world-space where to render the cross.
     * @param size Length of the cross lines.
     * @param color Color of the cross lines.
     * @param transform Optional transformation (rotation, scale, translation) of the cross.
     */
    function DrawCross(pos, size, color, transform) {
        if (transform === void 0) { transform = null; }
        __CPP_Debug_DrawCross(pos, size, color, transform);
    }
    Debug.DrawCross = DrawCross;
    /**
     * Represents a line in 3D.
     */
    var Line = /** @class */ (function () {
        function Line() {
            this.startX = 0;
            this.startY = 0;
            this.startZ = 0;
            this.endX = 0;
            this.endY = 0;
            this.endZ = 0;
        }
        return Line;
    }());
    Debug.Line = Line;
    /**
     * Draws a set of lines with one color in 3D world space.
     */
    function DrawLines(lines, color) {
        if (color === void 0) { color = null; }
        __CPP_Debug_DrawLines(lines, color);
    }
    Debug.DrawLines = DrawLines;
    /**
     * Draws a set of lines with one color in 2D screen space. Depth (z coordinate) is used for sorting but not for perspective.
     */
    function Draw2DLines(lines, color) {
        if (color === void 0) { color = null; }
        __CPP_Debug_Draw2DLines(lines, color);
    }
    Debug.Draw2DLines = Draw2DLines;
    /**
     * Draws an axis-aligned box out of lines.
     * If the box should not be axis-aligned, the rotation must be set through the transform parameter.
     *
     * @param min The minimum corner of the AABB.
     * @param max The maximum corner of the AABB.
     * @param color The color of the lines.
     * @param transform Optional transformation (rotation, scale, translation) of the bbox.
     */
    function DrawLineBox(min, max, color, transform) {
        if (color === void 0) { color = null; }
        if (transform === void 0) { transform = null; }
        __CPP_Debug_DrawLineBox(min, max, color, transform);
    }
    Debug.DrawLineBox = DrawLineBox;
    /**
     * Draws an axis-aligned solid box.
     * If the box should not be axis-aligned, the rotation must be set through the transform parameter.
     *
     * @param min The minimum corner of the AABB.
     * @param max The maximum corner of the AABB.
     * @param color The color of the faces.
     * @param transform Optional transformation (rotation, scale, translation) of the bbox.
     */
    function DrawSolidBox(min, max, color, transform) {
        if (color === void 0) { color = null; }
        if (transform === void 0) { transform = null; }
        __CPP_Debug_DrawSolidBox(min, max, color, transform);
    }
    Debug.DrawSolidBox = DrawSolidBox;
    /**
     * Draws a sphere out of lines.
     *
     * @param center The world-space position of the sphere.
     * @param radius The radius of the sphere.
     * @param color The color of the lines.
     * @param transform An optional transform. Mostly for convenience to just pass in an object's transform,
     *                  but also makes it possible to squash the sphere with non-uniform scaling.
     */
    function DrawLineSphere(center, radius, color, transform) {
        if (color === void 0) { color = null; }
        if (transform === void 0) { transform = null; }
        __CPP_Debug_DrawLineSphere(center, radius, color, transform);
    }
    Debug.DrawLineSphere = DrawLineSphere;
    /**
     * Draws text at a pixel position on screen.
     *
     * The string may contain newlines (\n) for multi-line output.
     * If horizontal alignment is right, the entire text block is aligned according to the longest line.
     *
     * Data can be output as a table, by separating columns with tabs (\n). For example:\n
     * "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|"
     *
     * @param pos The screen-space position where to render the text.
     * @param sizeInPixel The size of the text in pixels.
     */
    function Draw2DText(text, pos, color, sizeInPixel, alignHorz) {
        if (color === void 0) { color = null; }
        if (sizeInPixel === void 0) { sizeInPixel = 16; }
        if (alignHorz === void 0) { alignHorz = HorizontalAlignment.Left; }
        __CPP_Debug_Draw2DText(text, pos, color, sizeInPixel, alignHorz);
    }
    Debug.Draw2DText = Draw2DText;
    /**
     * Draws text at a 3D position, always facing the camera.
     *
     * @param pos The world-space position where to render the text.
     * @param sizeInPixel The size of the text in pixels. The text will always be the same size and facing the camera.
     */
    function Draw3DText(text, pos, color, sizeInPixel) {
        if (color === void 0) { color = null; }
        if (sizeInPixel === void 0) { sizeInPixel = 16; }
        __CPP_Debug_Draw3DText(text, pos, color, sizeInPixel);
    }
    Debug.Draw3DText = Draw3DText;
    /**
     * Draws text in one of the screen corners.
     * Makes sure text in the same corner does not overlap.
     * Has the same formatting options as Draw2DText().
     *
     * @param corner In which area of the screen to position the text.
     */
    function DrawInfoText(corner, text, color) {
        if (color === void 0) { color = null; }
        __CPP_Debug_DrawInfoText(corner, text, color);
    }
    Debug.DrawInfoText = DrawInfoText;
    /**
     * Reads the boolean CVar of the given name and returns its value.
     * If no CVar with this name exists or it uses a different type, 'undefined' is returned.
     */
    function ReadCVar_Boolean(name) {
        return __CPP_Debug_ReadCVarBool(name);
    }
    Debug.ReadCVar_Boolean = ReadCVar_Boolean;
    /**
     * Reads the boolean CVar of the given name and returns its value.
     * If no CVar with this name exists or it uses a different type, 'undefined' is returned.
     */
    function ReadCVar_Int(name) {
        return __CPP_Debug_ReadCVarInt(name);
    }
    Debug.ReadCVar_Int = ReadCVar_Int;
    /**
     * Reads the boolean CVar of the given name and returns its value.
     * If no CVar with this name exists or it uses a different type, 'undefined' is returned.
     */
    function ReadCVar_Float(name) {
        return __CPP_Debug_ReadCVarFloat(name);
    }
    Debug.ReadCVar_Float = ReadCVar_Float;
    /**
     * Reads the boolean CVar of the given name and returns its value.
     * If no CVar with this name exists or it uses a different type, 'undefined' is returned.
     */
    function ReadCVar_String(name) {
        return __CPP_Debug_ReadCVarString(name);
    }
    Debug.ReadCVar_String = ReadCVar_String;
    /**
     * Stores the given value in the CVar with the provided name.
     * Throws an error if no such CVar exists or the existing one is not of the expected type.
     */
    function WriteCVar_Boolean(name, value) {
        return __CPP_Debug_WriteCVarBool(name, value);
    }
    Debug.WriteCVar_Boolean = WriteCVar_Boolean;
    /**
     * Stores the given value in the CVar with the provided name.
     * Throws an error if no such CVar exists or the existing one is not of the expected type.
     */
    function WriteCVar_Int(name, value) {
        return __CPP_Debug_WriteCVarInt(name, value);
    }
    Debug.WriteCVar_Int = WriteCVar_Int;
    /**
     * Stores the given value in the CVar with the provided name.
     * Throws an error if no such CVar exists or the existing one is not of the expected type.
     */
    function WriteCVar_Float(name, value) {
        return __CPP_Debug_WriteCVarFloat(name, value);
    }
    Debug.WriteCVar_Float = WriteCVar_Float;
    /**
     * Stores the given value in the CVar with the provided name.
     * Throws an error if no such CVar exists or the existing one is not of the expected type.
     */
    function WriteCVar_String(name, value) {
        return __CPP_Debug_WriteCVarString(name, value);
    }
    Debug.WriteCVar_String = WriteCVar_String;
    /**
     * Creates a new CVar with the given name, value and description.
     * If a CVar with this name was already created before, the call is ignored.
     * The CVar can be modified like any other CVar and thus allows external configuration of the script code.
     * When the world in which this script is executed is destroyed, the CVar will cease existing as well.
     */
    function RegisterCVar_Int(name, value, description) {
        __CPP_Debug_RegisterCVar(name, 0, value, description);
    }
    Debug.RegisterCVar_Int = RegisterCVar_Int;
    /**
     * See RegisterCVar_Int
     */
    function RegisterCVar_Float(name, value, description) {
        __CPP_Debug_RegisterCVar(name, 1, value, description);
    }
    Debug.RegisterCVar_Float = RegisterCVar_Float;
    /**
     * See RegisterCVar_Int
     */
    function RegisterCVar_Boolean(name, value, description) {
        __CPP_Debug_RegisterCVar(name, 2, value, description);
    }
    Debug.RegisterCVar_Boolean = RegisterCVar_Boolean;
    /**
     * See RegisterCVar_Int
     */
    function RegisterCVar_String(name, value, description) {
        __CPP_Debug_RegisterCVar(name, 3, value, description);
    }
    Debug.RegisterCVar_String = RegisterCVar_String;
    var ArgType;
    (function (ArgType) {
        ArgType[ArgType["Boolean"] = 2] = "Boolean";
        ArgType[ArgType["Number"] = 12] = "Number";
        ArgType[ArgType["String"] = 27] = "String";
    })(ArgType = Debug.ArgType || (Debug.ArgType = {}));
    /**
     * Registers a function as a console function.
     * The function can be registered multiple times with different 'func' arguments, to bind the call to multiple objects,
     * however, the list of argument types must be identical each time.
     *
     * @param owner The component that owns this function. If the component dies, the function will not be called anymore.
     * @param funcName The name under which to expose the function. E.g. "Print"
     * @param funcDesc A description of the function. Should ideally begin with the argument list to call it by. E.g.: "(text: string): Prints 'text' on screen."
     * @param func The typescript function to execute. Must accept the arguments as described by 'argTypes'. E.g. "function Print(text: string)".
     * @param argTypes Variadic list describing the type of each argument. E.g. "pl.Debug.ArgType.String, pl.Debug.ArgType.Number"
     */
    function RegisterConsoleFunc(owner, funcName, funcDesc, func) {
        var argTypes = [];
        for (var _i = 4; _i < arguments.length; _i++) {
            argTypes[_i - 4] = arguments[_i];
        }
        __CPP_Debug_RegisterCFunc.apply(void 0, __spreadArrays([owner, funcName, funcDesc, func], argTypes));
    }
    Debug.RegisterConsoleFunc = RegisterConsoleFunc;
})(Debug = exports.Debug || (exports.Debug = {}));
