/*SOURCE-HASH:43D2516F6621AA5D*/
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
var guns = require("Prefabs/Guns/Gun");
var Pistol = /** @class */ (function (_super) {
    __extends(Pistol, _super);
    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */
    function Pistol() {
        var _this = _super.call(this) || this;
        _this.nextAmmoPlus1Time = 0;
        _this.singleShotPerTrigger = true;
        return _this;
    }
    Pistol.RegisterMessageHandlers = function () {
        guns.Gun.RegisterMessageHandlers();
        //pl.TypescriptComponent.RegisterMessageHandler(pl.MsgSetColor, "OnMsgSetColor");
    };
    Pistol.prototype.OnSimulationStarted = function () {
        _super.prototype.OnSimulationStarted.call(this);
        this.SetTickInterval(0);
    };
    Pistol.prototype.Tick = function () {
        var now = pl.Time.GetGameTime();
        if (this.nextAmmoPlus1Time < now) {
            this.ammoInClip = pl.Utils.Clamp(this.ammoInClip + 1, 0, this.GetAmmoClipSize());
            this.nextAmmoPlus1Time = now + pl.Time.Seconds(0.2);
        }
    };
    Pistol.prototype.GetAmmoType = function () {
        return _ge.Consumable.Ammo_None;
    };
    Pistol.prototype.GetAmmoClipSize = function () {
        return 8;
    };
    Pistol.prototype.Fire = function () {
        var spawn = this.GetOwner().FindChildByName("Spawn").TryGetComponentOfBaseType(pl.SpawnComponent);
        if (spawn.CanTriggerManualSpawn() == false)
            return;
        this.nextAmmoPlus1Time = pl.Time.GetGameTime() + pl.Time.Seconds(0.5);
        this.ammoInClip -= 1;
        spawn.TriggerManualSpawn(false, pl.Vec3.ZeroVector());
        this.PlayShootSound();
    };
    Pistol.prototype.RenderCrosshair = function () {
        // render nothing, have a laser pointer already
    };
    return Pistol;
}(guns.Gun));
exports.Pistol = Pistol;
