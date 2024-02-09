/*SOURCE-HASH:19FAE7144F92C7ED*/
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
var _ge = require("Scripting/GameEnums");
var gun = require("Prefabs/Guns/Gun");
var Shotgun = /** @class */ (function (_super) {
    __extends(Shotgun, _super);
    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */
    function Shotgun() {
        var _this = _super.call(this) || this;
        _this.singleShotPerTrigger = true;
        return _this;
    }
    Shotgun.RegisterMessageHandlers = function () {
        gun.Gun.RegisterMessageHandlers();
        //pl.TypescriptComponent.RegisterMessageHandler(pl.MsgSetColor, "OnMsgSetColor");
    };
    Shotgun.prototype.Tick = function () { };
    Shotgun.prototype.GetAmmoType = function () {
        return _ge.Consumable.Ammo_Shotgun;
    };
    Shotgun.prototype.GetAmmoClipSize = function () {
        return 8;
    };
    Shotgun.prototype.Fire = function () {
        var spawn = this.GetOwner().FindChildByName("Spawn").TryGetComponentOfBaseType(pl.SpawnComponent);
        if (spawn.CanTriggerManualSpawn() == false)
            return;
        this.ammoInClip -= 1;
        for (var i = 0; i < 16; ++i) {
            spawn.TriggerManualSpawn(true, new pl.Vec3(pl.Random.DoubleMinMax(-0.05, 0.05), 0, 0));
        }
        this.PlayShootSound();
    };
    return Shotgun;
}(gun.Gun));
exports.Shotgun = Shotgun;
