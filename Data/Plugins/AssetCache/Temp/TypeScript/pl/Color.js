/*SOURCE-HASH:D4DE43379BBF6C2F*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var __Utils = require("./Utils");
exports.Utils = __Utils.Utils;
/**
 * Represents an HDR RGBA color value in linear space.
 */
var Color = /** @class */ (function () {
    /**
     * Constructs a custom color or default initializes it to black (alpha = 1)
     *
     * @param r Red in [0; 1] linear range.
     * @param g Green in [0; 1] linear range.
     * @param b Blue in [0; 1] linear range.
     * @param a Alpha in [0; 1] range.
     */
    function Color(r, g, b, a) {
        if (r === void 0) { r = 0; }
        if (g === void 0) { g = 0; }
        if (b === void 0) { b = 0; }
        if (a === void 0) { a = 1; }
        this.r = r;
        this.g = g;
        this.b = b;
        this.a = a;
    }
    /**
     * Returns a duplicate of this object.
     */
    Color.prototype.Clone = function () {
        return new Color(this.r, this.g, this.b, this.a);
    };
    Color.prototype.SetColor = function (rhs) {
        this.r = rhs.r;
        this.g = rhs.g;
        this.b = rhs.b;
        this.a = rhs.a;
    };
    /**
     * Returns an all-zero color object.
     */
    Color.ZeroColor = function () {
        return new Color(0, 0, 0, 0);
    };
    /**
     * Converts a color value from Gamma space to Linear space.
     *
     * @param gamma A color value (red, green or blue) in Gamma space.
     * @returns The converted value in Linear space.
     */
    Color.GammaToLinear = function (gamma) {
        return (gamma <= 0.04045) ? (gamma / 12.92) : (Math.pow((gamma + 0.055) / 1.055, 2.4));
    };
    /**
     * Converts a color value from Linear space to Gamma space.
     *
     * @param linear A color value (red, green or blue) in Linear space.
     * @returns The converted value in Gamma space.
     */
    Color.LinearToGamma = function (linear) {
        return (linear <= 0.0031308) ? (12.92 * linear) : (1.055 * Math.pow(linear, 1.0 / 2.4) - 0.055);
    };
    /**
     * Converts a color value from [0; 255] range to [0; 1] range.
     *
     * @param byte A color value in [0; 255] range.
     * @returns The color value in [0; 1] range.
     */
    Color.ColorByteToFloat = function (byte) {
        return byte / 255.0;
    };
    /**
     * Converts a color value from [0; 1] range to [0; 255] range.
     *
     * @param float A color value in [0; 1] range.
     * @returns The color value in [0; 255] range.
     */
    Color.ColorFloatToByte = function (float) {
        return Math.floor(exports.Utils.Saturate(float) * 255.0 + 0.5);
    };
    /**
     * Sets the RGB part with values in Linear space, [0; 1] range.
     * Does not modify alpha.
     */
    Color.prototype.SetLinearRGB = function (r, g, b) {
        this.r = r;
        this.g = g;
        this.b = b;
    };
    /**
     * Sets RGB and Alpha values in Linear space, [0; 1] range.
     */
    Color.prototype.SetLinearRGBA = function (r, g, b, a) {
        if (a === void 0) { a = 1.0; }
        this.r = r;
        this.g = g;
        this.b = b;
        this.a = a;
    };
    /**
     * Sets the RGB part with values in Gamma space, [0; 255] range.
     * Converts the given values to Linear space, [0; 1] range.
     * Does not modify alpha.
     */
    Color.prototype.SetGammaByteRGB = function (byteR, byteG, byteB) {
        this.r = Color.GammaToLinear(Color.ColorByteToFloat(byteR));
        this.g = Color.GammaToLinear(Color.ColorByteToFloat(byteG));
        this.b = Color.GammaToLinear(Color.ColorByteToFloat(byteB));
    };
    /**
     * Sets RGB and Alpha with values in Gamma space, [0; 255] range.
     * Converts the given values to Linear space, [0; 1] range.
     */
    Color.prototype.SetGammaByteRGBA = function (byteR, byteG, byteB, byteA) {
        if (byteA === void 0) { byteA = 255; }
        this.r = Color.GammaToLinear(Color.ColorByteToFloat(byteR));
        this.g = Color.GammaToLinear(Color.ColorByteToFloat(byteG));
        this.b = Color.GammaToLinear(Color.ColorByteToFloat(byteB));
        this.a = Color.ColorByteToFloat(byteA);
    };
    /**
     * Returns RGBA converted to GAmma space and in [0; 255] range.
     */
    Color.prototype.GetAsGammaByteRGBA = function () {
        var r = Color.ColorFloatToByte(Color.LinearToGamma(this.r));
        var g = Color.ColorFloatToByte(Color.LinearToGamma(this.g));
        var b = Color.ColorFloatToByte(Color.LinearToGamma(this.b));
        var a = Color.ColorFloatToByte(this.a);
        return { byteR: r, byteG: g, byteB: b, byteA: a };
    };
    /**
     * Sets all values to zero.
     */
    Color.prototype.SetZero = function () {
        this.r = 0;
        this.g = 0;
        this.b = 0;
        this.a = 0;
    };
    /**
     * Scales the RGB values with the given factor.
     * Does not modify Alpha.
     */
    Color.prototype.ScaleRGB = function (factor) {
        this.r *= factor;
        this.g *= factor;
        this.b *= factor;
    };
    /**
     * Adds rhs to this.
     */
    Color.prototype.AddColor = function (rhs) {
        this.r += rhs.r;
        this.g += rhs.g;
        this.b += rhs.b;
        this.a += rhs.a;
    };
    /**
     * Subtracts rhs from this.
     */
    Color.prototype.SubColor = function (rhs) {
        this.r -= rhs.r;
        this.g -= rhs.g;
        this.b -= rhs.b;
        this.a -= rhs.a;
    };
    /**
     * Multiplies rhs into this.
     */
    Color.prototype.MulColor = function (rhs) {
        this.r *= rhs.r;
        this.g *= rhs.g;
        this.b *= rhs.b;
        this.a *= rhs.a;
    };
    /**
     * Multiplies this with factor.
     */
    Color.prototype.MulNumber = function (factor) {
        this.r *= factor;
        this.g *= factor;
        this.b *= factor;
        this.a *= factor;
    };
    /**
     * Divides this by factor.
     */
    Color.prototype.DivNumber = function (factor) {
        var oneDiv = 1.0 / factor;
        this.r *= oneDiv;
        this.g *= oneDiv;
        this.b *= oneDiv;
        this.a *= oneDiv;
    };
    /**
     * Checks whether this and rhs are completely identical in RGB. Ignores Alpha.
     */
    Color.prototype.IsIdenticalRGB = function (rhs) {
        return this.r == rhs.r && this.g == rhs.g && this.b == rhs.b;
    };
    /**
     * Checks whether this and rhs are completely identical in RGB and Alpha.
     */
    Color.prototype.IsIdenticalRGBA = function (rhs) {
        return this.r == rhs.r && this.g == rhs.g && this.b == rhs.b && this.a == rhs.a;
    };
    /**
     * Checks whether this and rhs are approximately equal in RGB. Ignores Alpha.
     * @param epsilon In Linear space, [0; 1] range.
     */
    Color.prototype.IsEqualRGB = function (rhs, epsilon) {
        if (epsilon === void 0) { epsilon = 0.01; }
        return exports.Utils.IsNumberEqual(this.r, rhs.r, epsilon) && exports.Utils.IsNumberEqual(this.g, rhs.g, epsilon) && exports.Utils.IsNumberEqual(this.b, rhs.b, epsilon);
    };
    /**
     * Checks whether this and rhs are approximately equal in RGB and Alpha.
     * @param epsilon In Linear space, [0; 1] range.
     */
    Color.prototype.IsEqualRGBA = function (rhs, epsilon) {
        if (epsilon === void 0) { epsilon = 0.01; }
        return exports.Utils.IsNumberEqual(this.r, rhs.r, epsilon) && exports.Utils.IsNumberEqual(this.g, rhs.g, epsilon) && exports.Utils.IsNumberEqual(this.b, rhs.b, epsilon) && exports.Utils.IsNumberEqual(this.a, rhs.a, epsilon);
    };
    /**
     * Returns a duplicate of this, but with a replaced alpha value.
     */
    Color.prototype.WithAlpha = function (alpha) {
        return new Color(this.r, this.g, this.b, alpha);
    };
    /**
     * Sets this to lhs + rhs.
     */
    Color.prototype.SetAdd = function (lhs, rhs) {
        this.r = lhs.r + rhs.r;
        this.g = lhs.g + rhs.g;
        this.b = lhs.b + rhs.b;
        this.a = lhs.a + rhs.a;
    };
    /**
     * Sets this to lhs - rhs.
     */
    Color.prototype.SetSub = function (lhs, rhs) {
        this.r = lhs.r - rhs.r;
        this.g = lhs.g - rhs.g;
        this.b = lhs.b - rhs.b;
        this.a = lhs.a - rhs.a;
    };
    /**
     * Sets this to lhs * rhs.
     */
    Color.prototype.SetMul = function (lhs, rhs) {
        this.r = lhs.r * rhs.r;
        this.g = lhs.g * rhs.g;
        this.b = lhs.b * rhs.b;
        this.a = lhs.a * rhs.a;
    };
    /**
     * Sets this to lhs * rhs.
     */
    Color.prototype.SetMulNumber = function (lhs, rhs) {
        this.r = lhs.r * rhs;
        this.g = lhs.g * rhs;
        this.b = lhs.b * rhs;
        this.a = lhs.a * rhs;
    };
    /**
     * Sets this to lhs / rhs.
     */
    Color.prototype.SetDivNumber = function (lhs, rhs) {
        var invRhs = 1.0 / rhs;
        this.r = lhs.r * invRhs;
        this.g = lhs.g * invRhs;
        this.b = lhs.b * invRhs;
        this.a = lhs.a * invRhs;
    };
    /**
     * Calculates the average of the RGB channels.
     */
    Color.prototype.CalcAverageRGB = function () {
        return (this.r + this.g + this.b) / 3.0;
    };
    /**
     * Returns if the color is in the Range [0; 1] on all 4 channels.
     */
    Color.prototype.IsNormalized = function () {
        return (this.r <= 1.0 && this.g <= 1.0 && this.b <= 1.0 && this.a <= 1.0) &&
            (this.r >= 0.0 && this.g >= 0.0 && this.b >= 0.0 && this.a >= 0.0);
    };
    /**
     * Returns 1 for an LDR color (all ´RGB components < 1). Otherwise the value of the largest component. Ignores alpha.
     */
    Color.prototype.ComputeHdrMultiplier = function () {
        return Math.max(1.0, Math.max(this.r, Math.max(this.g, this.b)));
    };
    /**
     * If this is an HDR color, the largest component value is used to normalize RGB to LDR range. Alpha is unaffected.
     */
    Color.prototype.NormalizeToLdrRange = function () {
        this.ScaleRGB(1.0 / this.ComputeHdrMultiplier());
    };
    /**
     * Computes the perceived luminance. Assumes linear color space (http://en.wikipedia.org/wiki/Luminance_%28relative%29).
     */
    Color.prototype.GetLuminance = function () {
        return 0.2126 * this.r + 0.7152 * this.g + 0.0722 * this.b;
    };
    /**
     * Performs a simple (1.0 - color) inversion on all four channels.
     */
    Color.prototype.GetInvertedColor = function () {
        return new Color(1.0 - this.r, 1.0 - this.g, 1.0 - this.b, 1.0 - this.a);
    };
    /**
     * Returns the base-2 logarithm of ComputeHdrMultiplier().
     * 0 for LDR colors, +1, +2, etc. for HDR colors.
     */
    Color.prototype.ComputeHdrExposureValue = function () {
        return Math.log(this.ComputeHdrMultiplier()) / Math.log(2.0);
    };
    /**
     * Raises 2 to the power \a ev and multiplies RGB with that factor.
     */
    Color.prototype.ApplyHdrExposureValue = function (ev) {
        var factor = Math.pow(2, ev);
        this.r *= factor;
        this.g *= factor;
        this.b *= factor;
    };
    Color.prototype.SetAliceBlue = function () { this.SetGammaByteRGBA(0xF0, 0xF8, 0xFF); };
    Color.prototype.SetAntiqueWhite = function () { this.SetGammaByteRGBA(0xFA, 0xEB, 0xD7); };
    Color.prototype.SetAqua = function () { this.SetGammaByteRGBA(0x00, 0xFF, 0xFF); };
    Color.prototype.SetAquamarine = function () { this.SetGammaByteRGBA(0x7F, 0xFF, 0xD4); };
    Color.prototype.SetAzure = function () { this.SetGammaByteRGBA(0xF0, 0xFF, 0xFF); };
    Color.prototype.SetBeige = function () { this.SetGammaByteRGBA(0xF5, 0xF5, 0xDC); };
    Color.prototype.SetBisque = function () { this.SetGammaByteRGBA(0xFF, 0xE4, 0xC4); };
    Color.prototype.SetBlack = function () { this.SetGammaByteRGBA(0x00, 0x00, 0x00); };
    Color.prototype.SetBlanchedAlmond = function () { this.SetGammaByteRGBA(0xFF, 0xEB, 0xCD); };
    Color.prototype.SetBlue = function () { this.SetGammaByteRGBA(0x00, 0x00, 0xFF); };
    Color.prototype.SetBlueViolet = function () { this.SetGammaByteRGBA(0x8A, 0x2B, 0xE2); };
    Color.prototype.SetBrown = function () { this.SetGammaByteRGBA(0xA5, 0x2A, 0x2A); };
    Color.prototype.SetBurlyWood = function () { this.SetGammaByteRGBA(0xDE, 0xB8, 0x87); };
    Color.prototype.SetCadetBlue = function () { this.SetGammaByteRGBA(0x5F, 0x9E, 0xA0); };
    Color.prototype.SetChartreuse = function () { this.SetGammaByteRGBA(0x7F, 0xFF, 0x00); };
    Color.prototype.SetChocolate = function () { this.SetGammaByteRGBA(0xD2, 0x69, 0x1E); };
    Color.prototype.SetCoral = function () { this.SetGammaByteRGBA(0xFF, 0x7F, 0x50); };
    Color.prototype.SetCornflowerBlue = function () { this.SetGammaByteRGBA(0x64, 0x95, 0xED); }; // The Original!
    Color.prototype.SetCornsilk = function () { this.SetGammaByteRGBA(0xFF, 0xF8, 0xDC); };
    Color.prototype.SetCrimson = function () { this.SetGammaByteRGBA(0xDC, 0x14, 0x3C); };
    Color.prototype.SetCyan = function () { this.SetGammaByteRGBA(0x00, 0xFF, 0xFF); };
    Color.prototype.SetDarkBlue = function () { this.SetGammaByteRGBA(0x00, 0x00, 0x8B); };
    Color.prototype.SetDarkCyan = function () { this.SetGammaByteRGBA(0x00, 0x8B, 0x8B); };
    Color.prototype.SetDarkGoldenRod = function () { this.SetGammaByteRGBA(0xB8, 0x86, 0x0B); };
    Color.prototype.SetDarkGray = function () { this.SetGammaByteRGBA(0xA9, 0xA9, 0xA9); };
    Color.prototype.SetDarkGrey = function () { this.SetGammaByteRGBA(0xA9, 0xA9, 0xA9); };
    Color.prototype.SetDarkGreen = function () { this.SetGammaByteRGBA(0x00, 0x64, 0x00); };
    Color.prototype.SetDarkKhaki = function () { this.SetGammaByteRGBA(0xBD, 0xB7, 0x6B); };
    Color.prototype.SetDarkMagenta = function () { this.SetGammaByteRGBA(0x8B, 0x00, 0x8B); };
    Color.prototype.SetDarkOliveGreen = function () { this.SetGammaByteRGBA(0x55, 0x6B, 0x2F); };
    Color.prototype.SetDarkOrange = function () { this.SetGammaByteRGBA(0xFF, 0x8C, 0x00); };
    Color.prototype.SetDarkOrchid = function () { this.SetGammaByteRGBA(0x99, 0x32, 0xCC); };
    Color.prototype.SetDarkRed = function () { this.SetGammaByteRGBA(0x8B, 0x00, 0x00); };
    Color.prototype.SetDarkSalmon = function () { this.SetGammaByteRGBA(0xE9, 0x96, 0x7A); };
    Color.prototype.SetDarkSeaGreen = function () { this.SetGammaByteRGBA(0x8F, 0xBC, 0x8F); };
    Color.prototype.SetDarkSlateBlue = function () { this.SetGammaByteRGBA(0x48, 0x3D, 0x8B); };
    Color.prototype.SetDarkSlateGray = function () { this.SetGammaByteRGBA(0x2F, 0x4F, 0x4F); };
    Color.prototype.SetDarkSlateGrey = function () { this.SetGammaByteRGBA(0x2F, 0x4F, 0x4F); };
    Color.prototype.SetDarkTurquoise = function () { this.SetGammaByteRGBA(0x00, 0xCE, 0xD1); };
    Color.prototype.SetDarkViolet = function () { this.SetGammaByteRGBA(0x94, 0x00, 0xD3); };
    Color.prototype.SetDeepPink = function () { this.SetGammaByteRGBA(0xFF, 0x14, 0x93); };
    Color.prototype.SetDeepSkyBlue = function () { this.SetGammaByteRGBA(0x00, 0xBF, 0xFF); };
    Color.prototype.SetDimGray = function () { this.SetGammaByteRGBA(0x69, 0x69, 0x69); };
    Color.prototype.SetDimGrey = function () { this.SetGammaByteRGBA(0x69, 0x69, 0x69); };
    Color.prototype.SetDodgerBlue = function () { this.SetGammaByteRGBA(0x1E, 0x90, 0xFF); };
    Color.prototype.SetFireBrick = function () { this.SetGammaByteRGBA(0xB2, 0x22, 0x22); };
    Color.prototype.SetFloralWhite = function () { this.SetGammaByteRGBA(0xFF, 0xFA, 0xF0); };
    Color.prototype.SetForestGreen = function () { this.SetGammaByteRGBA(0x22, 0x8B, 0x22); };
    Color.prototype.SetFuchsia = function () { this.SetGammaByteRGBA(0xFF, 0x00, 0xFF); };
    Color.prototype.SetGainsboro = function () { this.SetGammaByteRGBA(0xDC, 0xDC, 0xDC); };
    Color.prototype.SetGhostWhite = function () { this.SetGammaByteRGBA(0xF8, 0xF8, 0xFF); };
    Color.prototype.SetGold = function () { this.SetGammaByteRGBA(0xFF, 0xD7, 0x00); };
    Color.prototype.SetGoldenRod = function () { this.SetGammaByteRGBA(0xDA, 0xA5, 0x20); };
    Color.prototype.SetGray = function () { this.SetGammaByteRGBA(0x80, 0x80, 0x80); };
    Color.prototype.SetGrey = function () { this.SetGammaByteRGBA(0x80, 0x80, 0x80); };
    Color.prototype.SetGreen = function () { this.SetGammaByteRGBA(0x00, 0x80, 0x00); };
    Color.prototype.SetGreenYellow = function () { this.SetGammaByteRGBA(0xAD, 0xFF, 0x2F); };
    Color.prototype.SetHoneyDew = function () { this.SetGammaByteRGBA(0xF0, 0xFF, 0xF0); };
    Color.prototype.SetHotPink = function () { this.SetGammaByteRGBA(0xFF, 0x69, 0xB4); };
    Color.prototype.SetIndianRed = function () { this.SetGammaByteRGBA(0xCD, 0x5C, 0x5C); };
    Color.prototype.SetIndigo = function () { this.SetGammaByteRGBA(0x4B, 0x00, 0x82); };
    Color.prototype.SetIvory = function () { this.SetGammaByteRGBA(0xFF, 0xFF, 0xF0); };
    Color.prototype.SetKhaki = function () { this.SetGammaByteRGBA(0xF0, 0xE6, 0x8C); };
    Color.prototype.SetLavender = function () { this.SetGammaByteRGBA(0xE6, 0xE6, 0xFA); };
    Color.prototype.SetLavenderBlush = function () { this.SetGammaByteRGBA(0xFF, 0xF0, 0xF5); };
    Color.prototype.SetLawnGreen = function () { this.SetGammaByteRGBA(0x7C, 0xFC, 0x00); };
    Color.prototype.SetLemonChiffon = function () { this.SetGammaByteRGBA(0xFF, 0xFA, 0xCD); };
    Color.prototype.SetLightBlue = function () { this.SetGammaByteRGBA(0xAD, 0xD8, 0xE6); };
    Color.prototype.SetLightCoral = function () { this.SetGammaByteRGBA(0xF0, 0x80, 0x80); };
    Color.prototype.SetLightCyan = function () { this.SetGammaByteRGBA(0xE0, 0xFF, 0xFF); };
    Color.prototype.SetLightGoldenRodYellow = function () { this.SetGammaByteRGBA(0xFA, 0xFA, 0xD2); };
    Color.prototype.SetLightGray = function () { this.SetGammaByteRGBA(0xD3, 0xD3, 0xD3); };
    Color.prototype.SetLightGrey = function () { this.SetGammaByteRGBA(0xD3, 0xD3, 0xD3); };
    Color.prototype.SetLightGreen = function () { this.SetGammaByteRGBA(0x90, 0xEE, 0x90); };
    Color.prototype.SetLightPink = function () { this.SetGammaByteRGBA(0xFF, 0xB6, 0xC1); };
    Color.prototype.SetLightSalmon = function () { this.SetGammaByteRGBA(0xFF, 0xA0, 0x7A); };
    Color.prototype.SetLightSeaGreen = function () { this.SetGammaByteRGBA(0x20, 0xB2, 0xAA); };
    Color.prototype.SetLightSkyBlue = function () { this.SetGammaByteRGBA(0x87, 0xCE, 0xFA); };
    Color.prototype.SetLightSlateGray = function () { this.SetGammaByteRGBA(0x77, 0x88, 0x99); };
    Color.prototype.SetLightSlateGrey = function () { this.SetGammaByteRGBA(0x77, 0x88, 0x99); };
    Color.prototype.SetLightSteelBlue = function () { this.SetGammaByteRGBA(0xB0, 0xC4, 0xDE); };
    Color.prototype.SetLightYellow = function () { this.SetGammaByteRGBA(0xFF, 0xFF, 0xE0); };
    Color.prototype.SetLime = function () { this.SetGammaByteRGBA(0x00, 0xFF, 0x00); };
    Color.prototype.SetLimeGreen = function () { this.SetGammaByteRGBA(0x32, 0xCD, 0x32); };
    Color.prototype.SetLinen = function () { this.SetGammaByteRGBA(0xFA, 0xF0, 0xE6); };
    Color.prototype.SetMagenta = function () { this.SetGammaByteRGBA(0xFF, 0x00, 0xFF); };
    Color.prototype.SetMaroon = function () { this.SetGammaByteRGBA(0x80, 0x00, 0x00); };
    Color.prototype.SetMediumAquaMarine = function () { this.SetGammaByteRGBA(0x66, 0xCD, 0xAA); };
    Color.prototype.SetMediumBlue = function () { this.SetGammaByteRGBA(0x00, 0x00, 0xCD); };
    Color.prototype.SetMediumOrchid = function () { this.SetGammaByteRGBA(0xBA, 0x55, 0xD3); };
    Color.prototype.SetMediumPurple = function () { this.SetGammaByteRGBA(0x93, 0x70, 0xDB); };
    Color.prototype.SetMediumSeaGreen = function () { this.SetGammaByteRGBA(0x3C, 0xB3, 0x71); };
    Color.prototype.SetMediumSlateBlue = function () { this.SetGammaByteRGBA(0x7B, 0x68, 0xEE); };
    Color.prototype.SetMediumSpringGreen = function () { this.SetGammaByteRGBA(0x00, 0xFA, 0x9A); };
    Color.prototype.SetMediumTurquoise = function () { this.SetGammaByteRGBA(0x48, 0xD1, 0xCC); };
    Color.prototype.SetMediumVioletRed = function () { this.SetGammaByteRGBA(0xC7, 0x15, 0x85); };
    Color.prototype.SetMidnightBlue = function () { this.SetGammaByteRGBA(0x19, 0x19, 0x70); };
    Color.prototype.SetMintCream = function () { this.SetGammaByteRGBA(0xF5, 0xFF, 0xFA); };
    Color.prototype.SetMistyRose = function () { this.SetGammaByteRGBA(0xFF, 0xE4, 0xE1); };
    Color.prototype.SetMoccasin = function () { this.SetGammaByteRGBA(0xFF, 0xE4, 0xB5); };
    Color.prototype.SetNavajoWhite = function () { this.SetGammaByteRGBA(0xFF, 0xDE, 0xAD); };
    Color.prototype.SetNavy = function () { this.SetGammaByteRGBA(0x00, 0x00, 0x80); };
    Color.prototype.SetOldLace = function () { this.SetGammaByteRGBA(0xFD, 0xF5, 0xE6); };
    Color.prototype.SetOlive = function () { this.SetGammaByteRGBA(0x80, 0x80, 0x00); };
    Color.prototype.SetOliveDrab = function () { this.SetGammaByteRGBA(0x6B, 0x8E, 0x23); };
    Color.prototype.SetOrange = function () { this.SetGammaByteRGBA(0xFF, 0xA5, 0x00); };
    Color.prototype.SetOrangeRed = function () { this.SetGammaByteRGBA(0xFF, 0x45, 0x00); };
    Color.prototype.SetOrchid = function () { this.SetGammaByteRGBA(0xDA, 0x70, 0xD6); };
    Color.prototype.SetPaleGoldenRod = function () { this.SetGammaByteRGBA(0xEE, 0xE8, 0xAA); };
    Color.prototype.SetPaleGreen = function () { this.SetGammaByteRGBA(0x98, 0xFB, 0x98); };
    Color.prototype.SetPaleTurquoise = function () { this.SetGammaByteRGBA(0xAF, 0xEE, 0xEE); };
    Color.prototype.SetPaleVioletRed = function () { this.SetGammaByteRGBA(0xDB, 0x70, 0x93); };
    Color.prototype.SetPapayaWhip = function () { this.SetGammaByteRGBA(0xFF, 0xEF, 0xD5); };
    Color.prototype.SetPeachPuff = function () { this.SetGammaByteRGBA(0xFF, 0xDA, 0xB9); };
    Color.prototype.SetPeru = function () { this.SetGammaByteRGBA(0xCD, 0x85, 0x3F); };
    Color.prototype.SetPink = function () { this.SetGammaByteRGBA(0xFF, 0xC0, 0xCB); };
    Color.prototype.SetPlum = function () { this.SetGammaByteRGBA(0xDD, 0xA0, 0xDD); };
    Color.prototype.SetPowderBlue = function () { this.SetGammaByteRGBA(0xB0, 0xE0, 0xE6); };
    Color.prototype.SetPurple = function () { this.SetGammaByteRGBA(0x80, 0x00, 0x80); };
    Color.prototype.SetRebeccaPurple = function () { this.SetGammaByteRGBA(0x66, 0x33, 0x99); };
    Color.prototype.SetRed = function () { this.SetGammaByteRGBA(0xFF, 0x00, 0x00); };
    Color.prototype.SetRosyBrown = function () { this.SetGammaByteRGBA(0xBC, 0x8F, 0x8F); };
    Color.prototype.SetRoyalBlue = function () { this.SetGammaByteRGBA(0x41, 0x69, 0xE1); };
    Color.prototype.SetSaddleBrown = function () { this.SetGammaByteRGBA(0x8B, 0x45, 0x13); };
    Color.prototype.SetSalmon = function () { this.SetGammaByteRGBA(0xFA, 0x80, 0x72); };
    Color.prototype.SetSandyBrown = function () { this.SetGammaByteRGBA(0xF4, 0xA4, 0x60); };
    Color.prototype.SetSeaGreen = function () { this.SetGammaByteRGBA(0x2E, 0x8B, 0x57); };
    Color.prototype.SetSeaShell = function () { this.SetGammaByteRGBA(0xFF, 0xF5, 0xEE); };
    Color.prototype.SetSienna = function () { this.SetGammaByteRGBA(0xA0, 0x52, 0x2D); };
    Color.prototype.SetSilver = function () { this.SetGammaByteRGBA(0xC0, 0xC0, 0xC0); };
    Color.prototype.SetSkyBlue = function () { this.SetGammaByteRGBA(0x87, 0xCE, 0xEB); };
    Color.prototype.SetSlateBlue = function () { this.SetGammaByteRGBA(0x6A, 0x5A, 0xCD); };
    Color.prototype.SetSlateGray = function () { this.SetGammaByteRGBA(0x70, 0x80, 0x90); };
    Color.prototype.SetSlateGrey = function () { this.SetGammaByteRGBA(0x70, 0x80, 0x90); };
    Color.prototype.SetSnow = function () { this.SetGammaByteRGBA(0xFF, 0xFA, 0xFA); };
    Color.prototype.SetSpringGreen = function () { this.SetGammaByteRGBA(0x00, 0xFF, 0x7F); };
    Color.prototype.SetSteelBlue = function () { this.SetGammaByteRGBA(0x46, 0x82, 0xB4); };
    Color.prototype.SetTan = function () { this.SetGammaByteRGBA(0xD2, 0xB4, 0x8C); };
    Color.prototype.SetTeal = function () { this.SetGammaByteRGBA(0x00, 0x80, 0x80); };
    Color.prototype.SetThistle = function () { this.SetGammaByteRGBA(0xD8, 0xBF, 0xD8); };
    Color.prototype.SetTomato = function () { this.SetGammaByteRGBA(0xFF, 0x63, 0x47); };
    Color.prototype.SetTurquoise = function () { this.SetGammaByteRGBA(0x40, 0xE0, 0xD0); };
    Color.prototype.SetViolet = function () { this.SetGammaByteRGBA(0xEE, 0x82, 0xEE); };
    Color.prototype.SetWheat = function () { this.SetGammaByteRGBA(0xF5, 0xDE, 0xB3); };
    Color.prototype.SetWhite = function () { this.SetGammaByteRGBA(0xFF, 0xFF, 0xFF); };
    Color.prototype.SetWhiteSmoke = function () { this.SetGammaByteRGBA(0xF5, 0xF5, 0xF5); };
    Color.prototype.SetYellow = function () { this.SetGammaByteRGBA(0xFF, 0xFF, 0x00); };
    Color.prototype.SetYellowGreen = function () { this.SetGammaByteRGBA(0x9A, 0xCD, 0x32); };
    Color.AliceBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0xF0, 0xF8, 0xFF); return c; };
    Color.AntiqueWhite = function () { var c = new Color(); c.SetGammaByteRGBA(0xFA, 0xEB, 0xD7); return c; };
    Color.Aqua = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0xFF, 0xFF); return c; };
    Color.Aquamarine = function () { var c = new Color(); c.SetGammaByteRGBA(0x7F, 0xFF, 0xD4); return c; };
    Color.Azure = function () { var c = new Color(); c.SetGammaByteRGBA(0xF0, 0xFF, 0xFF); return c; };
    Color.Beige = function () { var c = new Color(); c.SetGammaByteRGBA(0xF5, 0xF5, 0xDC); return c; };
    Color.Bisque = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xE4, 0xC4); return c; };
    Color.Black = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0x00, 0x00); return c; };
    Color.BlanchedAlmond = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xEB, 0xCD); return c; };
    Color.Blue = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0x00, 0xFF); return c; };
    Color.BlueViolet = function () { var c = new Color(); c.SetGammaByteRGBA(0x8A, 0x2B, 0xE2); return c; };
    Color.Brown = function () { var c = new Color(); c.SetGammaByteRGBA(0xA5, 0x2A, 0x2A); return c; };
    Color.BurlyWood = function () { var c = new Color(); c.SetGammaByteRGBA(0xDE, 0xB8, 0x87); return c; };
    Color.CadetBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x5F, 0x9E, 0xA0); return c; };
    Color.Chartreuse = function () { var c = new Color(); c.SetGammaByteRGBA(0x7F, 0xFF, 0x00); return c; };
    Color.Chocolate = function () { var c = new Color(); c.SetGammaByteRGBA(0xD2, 0x69, 0x1E); return c; };
    Color.Coral = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0x7F, 0x50); return c; };
    Color.CornflowerBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x64, 0x95, 0xED); return c; }; // The Original!
    Color.Cornsilk = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xF8, 0xDC); return c; };
    Color.Crimson = function () { var c = new Color(); c.SetGammaByteRGBA(0xDC, 0x14, 0x3C); return c; };
    Color.Cyan = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0xFF, 0xFF); return c; };
    Color.DarkBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0x00, 0x8B); return c; };
    Color.DarkCyan = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0x8B, 0x8B); return c; };
    Color.DarkGoldenRod = function () { var c = new Color(); c.SetGammaByteRGBA(0xB8, 0x86, 0x0B); return c; };
    Color.DarkGray = function () { var c = new Color(); c.SetGammaByteRGBA(0xA9, 0xA9, 0xA9); return c; };
    Color.DarkGrey = function () { var c = new Color(); c.SetGammaByteRGBA(0xA9, 0xA9, 0xA9); return c; };
    Color.DarkGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0x64, 0x00); return c; };
    Color.DarkKhaki = function () { var c = new Color(); c.SetGammaByteRGBA(0xBD, 0xB7, 0x6B); return c; };
    Color.DarkMagenta = function () { var c = new Color(); c.SetGammaByteRGBA(0x8B, 0x00, 0x8B); return c; };
    Color.DarkOliveGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x55, 0x6B, 0x2F); return c; };
    Color.DarkOrange = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0x8C, 0x00); return c; };
    Color.DarkOrchid = function () { var c = new Color(); c.SetGammaByteRGBA(0x99, 0x32, 0xCC); return c; };
    Color.DarkRed = function () { var c = new Color(); c.SetGammaByteRGBA(0x8B, 0x00, 0x00); return c; };
    Color.DarkSalmon = function () { var c = new Color(); c.SetGammaByteRGBA(0xE9, 0x96, 0x7A); return c; };
    Color.DarkSeaGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x8F, 0xBC, 0x8F); return c; };
    Color.DarkSlateBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x48, 0x3D, 0x8B); return c; };
    Color.DarkSlateGray = function () { var c = new Color(); c.SetGammaByteRGBA(0x2F, 0x4F, 0x4F); return c; };
    Color.DarkSlateGrey = function () { var c = new Color(); c.SetGammaByteRGBA(0x2F, 0x4F, 0x4F); return c; };
    Color.DarkTurquoise = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0xCE, 0xD1); return c; };
    Color.DarkViolet = function () { var c = new Color(); c.SetGammaByteRGBA(0x94, 0x00, 0xD3); return c; };
    Color.DeepPink = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0x14, 0x93); return c; };
    Color.DeepSkyBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0xBF, 0xFF); return c; };
    Color.DimGray = function () { var c = new Color(); c.SetGammaByteRGBA(0x69, 0x69, 0x69); return c; };
    Color.DimGrey = function () { var c = new Color(); c.SetGammaByteRGBA(0x69, 0x69, 0x69); return c; };
    Color.DodgerBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x1E, 0x90, 0xFF); return c; };
    Color.FireBrick = function () { var c = new Color(); c.SetGammaByteRGBA(0xB2, 0x22, 0x22); return c; };
    Color.FloralWhite = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFA, 0xF0); return c; };
    Color.ForestGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x22, 0x8B, 0x22); return c; };
    Color.Fuchsia = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0x00, 0xFF); return c; };
    Color.Gainsboro = function () { var c = new Color(); c.SetGammaByteRGBA(0xDC, 0xDC, 0xDC); return c; };
    Color.GhostWhite = function () { var c = new Color(); c.SetGammaByteRGBA(0xF8, 0xF8, 0xFF); return c; };
    Color.Gold = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xD7, 0x00); return c; };
    Color.GoldenRod = function () { var c = new Color(); c.SetGammaByteRGBA(0xDA, 0xA5, 0x20); return c; };
    Color.Gray = function () { var c = new Color(); c.SetGammaByteRGBA(0x80, 0x80, 0x80); return c; };
    Color.Grey = function () { var c = new Color(); c.SetGammaByteRGBA(0x80, 0x80, 0x80); return c; };
    Color.Green = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0x80, 0x00); return c; };
    Color.GreenYellow = function () { var c = new Color(); c.SetGammaByteRGBA(0xAD, 0xFF, 0x2F); return c; };
    Color.HoneyDew = function () { var c = new Color(); c.SetGammaByteRGBA(0xF0, 0xFF, 0xF0); return c; };
    Color.HotPink = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0x69, 0xB4); return c; };
    Color.IndianRed = function () { var c = new Color(); c.SetGammaByteRGBA(0xCD, 0x5C, 0x5C); return c; };
    Color.Indigo = function () { var c = new Color(); c.SetGammaByteRGBA(0x4B, 0x00, 0x82); return c; };
    Color.Ivory = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFF, 0xF0); return c; };
    Color.Khaki = function () { var c = new Color(); c.SetGammaByteRGBA(0xF0, 0xE6, 0x8C); return c; };
    Color.Lavender = function () { var c = new Color(); c.SetGammaByteRGBA(0xE6, 0xE6, 0xFA); return c; };
    Color.LavenderBlush = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xF0, 0xF5); return c; };
    Color.LawnGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x7C, 0xFC, 0x00); return c; };
    Color.LemonChiffon = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFA, 0xCD); return c; };
    Color.LightBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0xAD, 0xD8, 0xE6); return c; };
    Color.LightCoral = function () { var c = new Color(); c.SetGammaByteRGBA(0xF0, 0x80, 0x80); return c; };
    Color.LightCyan = function () { var c = new Color(); c.SetGammaByteRGBA(0xE0, 0xFF, 0xFF); return c; };
    Color.LightGoldenRodYellow = function () { var c = new Color(); c.SetGammaByteRGBA(0xFA, 0xFA, 0xD2); return c; };
    Color.LightGray = function () { var c = new Color(); c.SetGammaByteRGBA(0xD3, 0xD3, 0xD3); return c; };
    Color.LightGrey = function () { var c = new Color(); c.SetGammaByteRGBA(0xD3, 0xD3, 0xD3); return c; };
    Color.LightGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x90, 0xEE, 0x90); return c; };
    Color.LightPink = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xB6, 0xC1); return c; };
    Color.LightSalmon = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xA0, 0x7A); return c; };
    Color.LightSeaGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x20, 0xB2, 0xAA); return c; };
    Color.LightSkyBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x87, 0xCE, 0xFA); return c; };
    Color.LightSlateGray = function () { var c = new Color(); c.SetGammaByteRGBA(0x77, 0x88, 0x99); return c; };
    Color.LightSlateGrey = function () { var c = new Color(); c.SetGammaByteRGBA(0x77, 0x88, 0x99); return c; };
    Color.LightSteelBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0xB0, 0xC4, 0xDE); return c; };
    Color.LightYellow = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFF, 0xE0); return c; };
    Color.Lime = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0xFF, 0x00); return c; };
    Color.LimeGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x32, 0xCD, 0x32); return c; };
    Color.Linen = function () { var c = new Color(); c.SetGammaByteRGBA(0xFA, 0xF0, 0xE6); return c; };
    Color.Magenta = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0x00, 0xFF); return c; };
    Color.Maroon = function () { var c = new Color(); c.SetGammaByteRGBA(0x80, 0x00, 0x00); return c; };
    Color.MediumAquaMarine = function () { var c = new Color(); c.SetGammaByteRGBA(0x66, 0xCD, 0xAA); return c; };
    Color.MediumBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0x00, 0xCD); return c; };
    Color.MediumOrchid = function () { var c = new Color(); c.SetGammaByteRGBA(0xBA, 0x55, 0xD3); return c; };
    Color.MediumPurple = function () { var c = new Color(); c.SetGammaByteRGBA(0x93, 0x70, 0xDB); return c; };
    Color.MediumSeaGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x3C, 0xB3, 0x71); return c; };
    Color.MediumSlateBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x7B, 0x68, 0xEE); return c; };
    Color.MediumSpringGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0xFA, 0x9A); return c; };
    Color.MediumTurquoise = function () { var c = new Color(); c.SetGammaByteRGBA(0x48, 0xD1, 0xCC); return c; };
    Color.MediumVioletRed = function () { var c = new Color(); c.SetGammaByteRGBA(0xC7, 0x15, 0x85); return c; };
    Color.MidnightBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x19, 0x19, 0x70); return c; };
    Color.MintCream = function () { var c = new Color(); c.SetGammaByteRGBA(0xF5, 0xFF, 0xFA); return c; };
    Color.MistyRose = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xE4, 0xE1); return c; };
    Color.Moccasin = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xE4, 0xB5); return c; };
    Color.NavajoWhite = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xDE, 0xAD); return c; };
    Color.Navy = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0x00, 0x80); return c; };
    Color.OldLace = function () { var c = new Color(); c.SetGammaByteRGBA(0xFD, 0xF5, 0xE6); return c; };
    Color.Olive = function () { var c = new Color(); c.SetGammaByteRGBA(0x80, 0x80, 0x00); return c; };
    Color.OliveDrab = function () { var c = new Color(); c.SetGammaByteRGBA(0x6B, 0x8E, 0x23); return c; };
    Color.Orange = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xA5, 0x00); return c; };
    Color.OrangeRed = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0x45, 0x00); return c; };
    Color.Orchid = function () { var c = new Color(); c.SetGammaByteRGBA(0xDA, 0x70, 0xD6); return c; };
    Color.PaleGoldenRod = function () { var c = new Color(); c.SetGammaByteRGBA(0xEE, 0xE8, 0xAA); return c; };
    Color.PaleGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x98, 0xFB, 0x98); return c; };
    Color.PaleTurquoise = function () { var c = new Color(); c.SetGammaByteRGBA(0xAF, 0xEE, 0xEE); return c; };
    Color.PaleVioletRed = function () { var c = new Color(); c.SetGammaByteRGBA(0xDB, 0x70, 0x93); return c; };
    Color.PapayaWhip = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xEF, 0xD5); return c; };
    Color.PeachPuff = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xDA, 0xB9); return c; };
    Color.Peru = function () { var c = new Color(); c.SetGammaByteRGBA(0xCD, 0x85, 0x3F); return c; };
    Color.Pink = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xC0, 0xCB); return c; };
    Color.Plum = function () { var c = new Color(); c.SetGammaByteRGBA(0xDD, 0xA0, 0xDD); return c; };
    Color.PowderBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0xB0, 0xE0, 0xE6); return c; };
    Color.Purple = function () { var c = new Color(); c.SetGammaByteRGBA(0x80, 0x00, 0x80); return c; };
    Color.RebeccaPurple = function () { var c = new Color(); c.SetGammaByteRGBA(0x66, 0x33, 0x99); return c; };
    Color.Red = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0x00, 0x00); return c; };
    Color.RosyBrown = function () { var c = new Color(); c.SetGammaByteRGBA(0xBC, 0x8F, 0x8F); return c; };
    Color.RoyalBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x41, 0x69, 0xE1); return c; };
    Color.SaddleBrown = function () { var c = new Color(); c.SetGammaByteRGBA(0x8B, 0x45, 0x13); return c; };
    Color.Salmon = function () { var c = new Color(); c.SetGammaByteRGBA(0xFA, 0x80, 0x72); return c; };
    Color.SandyBrown = function () { var c = new Color(); c.SetGammaByteRGBA(0xF4, 0xA4, 0x60); return c; };
    Color.SeaGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x2E, 0x8B, 0x57); return c; };
    Color.SeaShell = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xF5, 0xEE); return c; };
    Color.Sienna = function () { var c = new Color(); c.SetGammaByteRGBA(0xA0, 0x52, 0x2D); return c; };
    Color.Silver = function () { var c = new Color(); c.SetGammaByteRGBA(0xC0, 0xC0, 0xC0); return c; };
    Color.SkyBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x87, 0xCE, 0xEB); return c; };
    Color.SlateBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x6A, 0x5A, 0xCD); return c; };
    Color.SlateGray = function () { var c = new Color(); c.SetGammaByteRGBA(0x70, 0x80, 0x90); return c; };
    Color.SlateGrey = function () { var c = new Color(); c.SetGammaByteRGBA(0x70, 0x80, 0x90); return c; };
    Color.Snow = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFA, 0xFA); return c; };
    Color.SpringGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0xFF, 0x7F); return c; };
    Color.SteelBlue = function () { var c = new Color(); c.SetGammaByteRGBA(0x46, 0x82, 0xB4); return c; };
    Color.Tan = function () { var c = new Color(); c.SetGammaByteRGBA(0xD2, 0xB4, 0x8C); return c; };
    Color.Teal = function () { var c = new Color(); c.SetGammaByteRGBA(0x00, 0x80, 0x80); return c; };
    Color.Thistle = function () { var c = new Color(); c.SetGammaByteRGBA(0xD8, 0xBF, 0xD8); return c; };
    Color.Tomato = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0x63, 0x47); return c; };
    Color.Turquoise = function () { var c = new Color(); c.SetGammaByteRGBA(0x40, 0xE0, 0xD0); return c; };
    Color.Violet = function () { var c = new Color(); c.SetGammaByteRGBA(0xEE, 0x82, 0xEE); return c; };
    Color.Wheat = function () { var c = new Color(); c.SetGammaByteRGBA(0xF5, 0xDE, 0xB3); return c; };
    Color.White = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFF, 0xFF); return c; };
    Color.WhiteSmoke = function () { var c = new Color(); c.SetGammaByteRGBA(0xF5, 0xF5, 0xF5); return c; };
    Color.Yellow = function () { var c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFF, 0x00); return c; };
    Color.YellowGreen = function () { var c = new Color(); c.SetGammaByteRGBA(0x9A, 0xCD, 0x32); return c; };
    return Color;
}());
exports.Color = Color;
