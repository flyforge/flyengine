/*SOURCE-HASH:F828FC3CFE9024CC*/
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
var AmmoPouch = /** @class */ (function () {
    function AmmoPouch() {
        this.ammo = [];
        this.ammo[_ge.Consumable.Ammo_Pistol] = 20;
        this.ammo[_ge.Consumable.Ammo_MachineGun] = 50;
        this.ammo[_ge.Consumable.Ammo_Plasma] = 50;
        this.ammo[_ge.Consumable.Ammo_Shotgun] = 10;
        this.ammo[_ge.Consumable.Ammo_Rocket] = 5;
    }
    return AmmoPouch;
}());
exports.AmmoPouch = AmmoPouch;
var GunInteraction;
(function (GunInteraction) {
    GunInteraction[GunInteraction["Fire"] = 0] = "Fire";
    GunInteraction[GunInteraction["Reload"] = 1] = "Reload";
})(GunInteraction = exports.GunInteraction || (exports.GunInteraction = {}));
var MsgGunInteraction = /** @class */ (function (_super) {
    __extends(MsgGunInteraction, _super);
    function MsgGunInteraction() {
        var _this = _super.call(this) || this;
        _this.ammoPouch = null;
        _this.TypeNameHash = 4176216766;
        return _this;
    }
    MsgGunInteraction.GetTypeNameHash = function () { return 4176216766; };
    return MsgGunInteraction;
}(pl.Message));
exports.MsgGunInteraction = MsgGunInteraction;
var Gun = /** @class */ (function (_super) {
    __extends(Gun, _super);
    function Gun() {
        var _this = _super.call(this) || this;
        /* BEGIN AUTO-GENERATED: VARIABLES */
        /* END AUTO-GENERATED: VARIABLES */
        _this.singleShotPerTrigger = false;
        _this.requireSingleShotReset = false;
        _this.shootSoundComponent = null;
        _this.muzzleFlashComponent = null;
        _this.ammoInClip = 0;
        return _this;
    }
    Gun.RegisterMessageHandlers = function () {
        pl.TypescriptComponent.RegisterMessageHandler(MsgGunInteraction, "OnMsgGunInteraction");
    };
    Gun.prototype.OnActivated = function () {
        this.DeselectGun();
    };
    Gun.prototype.OnSimulationStarted = function () {
        var owner = this.GetOwner();
        var node = owner.FindChildByName("ShootSound", true);
        if (node != null) {
            this.shootSoundComponent = node.TryGetComponentOfBaseType(pl.FmodEventComponent);
        }
        node = owner.FindChildByName("Muzzleflash", true);
        if (node != null) {
            this.muzzleFlashComponent = node.TryGetComponentOfBaseType(pl.ParticleComponent);
        }
    };
    Gun.prototype.OnMsgGunInteraction = function (msg) {
        if (msg.interaction == GunInteraction.Fire) {
            if (msg.keyState == pl.TriggerState.Deactivated) {
                this.requireSingleShotReset = false;
                return;
            }
            if (this.ammoInClip == 0) {
                // empty gun sound etc.
                this.Reload(msg.ammoPouch);
                return;
            }
            if (this.singleShotPerTrigger) {
                if (msg.keyState == pl.TriggerState.Activated) {
                    if (!this.requireSingleShotReset) {
                        this.requireSingleShotReset = true;
                        this.Fire();
                    }
                }
            }
            else {
                this.Fire();
            }
        }
        else if (msg.interaction == GunInteraction.Reload) {
            if (this.GetAmmoInClip() >= this.GetAmmoClipSize())
                return;
            this.Reload(msg.ammoPouch);
        }
    };
    Gun.prototype.Reload = function (ammoPouch) {
        var type = this.GetAmmoType();
        if (type == _ge.Consumable.Ammo_None)
            return;
        var needed = this.GetAmmoClipSize() - this.ammoInClip;
        var take = Math.min(needed, ammoPouch.ammo[type]);
        ammoPouch.ammo[type] -= take;
        this.ammoInClip += take;
    };
    Gun.prototype.PlayShootSound = function () {
        if (this.shootSoundComponent != null && this.shootSoundComponent.IsValid()) {
            this.shootSoundComponent.StartOneShot();
        }
        if (this.muzzleFlashComponent != null && this.muzzleFlashComponent.IsValid()) {
            this.muzzleFlashComponent.StartEffect();
        }
    };
    Gun.prototype.GetAmmoInClip = function () {
        return this.ammoInClip;
    };
    Gun.prototype.SelectGun = function () {
        var graphics = this.GetOwner().FindChildByName("Graphics", true);
        if (graphics == null)
            return;
        graphics.SetActiveFlag(true);
    };
    Gun.prototype.DeselectGun = function () {
        var graphics = this.GetOwner().FindChildByName("Graphics", true);
        if (graphics == null)
            return;
        graphics.SetActiveFlag(false);
    };
    Gun.prototype.RenderCrosshair = function () {
        var resolution = pl.Debug.GetResolution();
        var screenCenter = resolution;
        screenCenter.MulNumber(0.5);
        var lines = [];
        lines[0] = new pl.Debug.Line();
        lines[0].startX = screenCenter.x;
        lines[0].startY = screenCenter.y - 10;
        lines[0].endX = screenCenter.x;
        lines[0].endY = screenCenter.y + 10;
        lines[1] = new pl.Debug.Line();
        lines[1].startX = screenCenter.x - 10;
        lines[1].startY = screenCenter.y;
        lines[1].endX = screenCenter.x + 10;
        lines[1].endY = screenCenter.y;
        pl.Debug.Draw2DLines(lines);
    };
    return Gun;
}(pl.TickedTypescriptComponent));
exports.Gun = Gun;
