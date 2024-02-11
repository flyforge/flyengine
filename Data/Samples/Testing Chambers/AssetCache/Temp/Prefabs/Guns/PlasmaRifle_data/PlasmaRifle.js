/*SOURCE-HASH:32CCF209174E8679*/
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
var PlasmaRifle = /** @class */ (function (_super) {
    __extends(PlasmaRifle, _super);
    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */
    function PlasmaRifle() {
        var _this = _super.call(this) || this;
        _this.singleShotPerTrigger = false;
        return _this;
    }
    PlasmaRifle.RegisterMessageHandlers = function () {
        gun.Gun.RegisterMessageHandlers();
        //pl.TypescriptComponent.RegisterMessageHandler(pl.MsgSetColor, "OnMsgSetColor");
    };
    PlasmaRifle.prototype.Tick = function () { };
    PlasmaRifle.prototype.GetAmmoClipSize = function () {
        return 30;
    };
    PlasmaRifle.prototype.GetAmmoType = function () {
        return _ge.Consumable.Ammo_Plasma;
    };
    PlasmaRifle.prototype.Fire = function () {
        var spawn = this.GetOwner().FindChildByName("Spawn").TryGetComponentOfBaseType(pl.SpawnComponent);
        if (spawn.CanTriggerManualSpawn() == false)
            return;
        this.ammoInClip -= 1;
        spawn.TriggerManualSpawn(true, pl.Vec3.ZeroVector());
        this.PlayShootSound();
    };
    return PlasmaRifle;
}(gun.Gun));
exports.PlasmaRifle = PlasmaRifle;
