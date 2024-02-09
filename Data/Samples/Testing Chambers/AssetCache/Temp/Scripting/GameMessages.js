/*SOURCE-HASH:1B3A2100333A5E59*/
"use strict";
var __extends = (this && this.__extends) || (function () {
    var extendStatics = function (d, b) {
        extendStatics = Object.setPrototypeOf ||
            ({ __proto__: [] } instanceof Array && function (d, b) { d.__proto__ = b; }) ||
            function (d, b) { for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p]; };
        return extendStatics(d, b);
    };
    return function (d, b) {
        extendStatics(d, b);
        function __() { this.constructor = d; }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
var pl = require("TypeScript/pl");
var MsgAddConsumable = /** @class */ (function (_super) {
    __extends(MsgAddConsumable, _super);
    function MsgAddConsumable() {
        var _this = _super.call(this) || this;
        _this.amount = 0;
        _this.return_consumed = true;
        _this.TypeNameHash = 130909488;
        return _this;
    }
    MsgAddConsumable.GetTypeNameHash = function () { return 130909488; };
    return MsgAddConsumable;
}(pl.Message));
exports.MsgAddConsumable = MsgAddConsumable;
var MsgUnlockWeapon = /** @class */ (function (_super) {
    __extends(MsgUnlockWeapon, _super);
    function MsgUnlockWeapon() {
        var _this = _super.call(this) || this;
        _this.return_consumed = true;
        _this.TypeNameHash = 1954625410;
        return _this;
    }
    MsgUnlockWeapon.GetTypeNameHash = function () { return 1954625410; };
    return MsgUnlockWeapon;
}(pl.Message));
exports.MsgUnlockWeapon = MsgUnlockWeapon;
