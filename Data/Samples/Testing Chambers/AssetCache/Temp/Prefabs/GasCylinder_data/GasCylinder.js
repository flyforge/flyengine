/*SOURCE-HASH:D238993CF3A7E7BF*/
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
var GasCylinder = /** @class */ (function (_super) {
    __extends(GasCylinder, _super);
    function GasCylinder() {
        var _this = _super.call(this) || this;
        _this.capHealth = 5;
        _this.bodyHealth = 50;
        return _this;
    }
    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */
    GasCylinder.RegisterMessageHandlers = function () {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgDamage, "OnMsgDamage");
    };
    GasCylinder.prototype.OnSimulationStarted = function () {
        this.SetTickInterval(pl.Time.Milliseconds(100));
    };
    GasCylinder.prototype.Tick = function () {
        if (this.capHealth <= 0) {
            var owner = this.GetOwner();
            var cap = owner.FindChildByName("Cap");
            var forceMsg = new pl.MsgPhysicsAddForce();
            forceMsg.GlobalPosition = cap.GetGlobalPosition();
            forceMsg.Force = cap.GetGlobalDirUp();
            var randomDir = pl.Vec3.CreateRandomDirection();
            randomDir.MulNumber(0.6);
            forceMsg.Force.AddVec3(randomDir);
            forceMsg.Force.MulNumber(-400);
            owner.SendMessage(forceMsg);
        }
    };
    GasCylinder.prototype.OnMsgDamage = function (msg) {
        //pl.Log.Info("Damaged: " + msg.HitObjectName + " - " + msg.Damage)
        this.bodyHealth -= msg.Damage;
        if (this.bodyHealth <= 0) {
            this.Explode();
            return;
        }
        if (msg.HitObjectName == "Cap") {
            if (this.capHealth > 0) {
                this.capHealth -= msg.Damage;
                if (this.capHealth <= 0) {
                    this.SetTickInterval(pl.Time.Milliseconds(0));
                    var leakObj = this.GetOwner().FindChildByName("LeakEffect");
                    if (leakObj != null) {
                        var leakFX = leakObj.TryGetComponentOfBaseType(pl.ParticleComponent);
                        if (leakFX != null) {
                            leakFX.StartEffect();
                        }
                        var leakSound = leakObj.TryGetComponentOfBaseType(pl.FmodEventComponent);
                        if (leakSound != null) {
                            leakSound.Restart();
                        }
                    }
                    // trigger code path below
                    msg.HitObjectName = "Tick";
                }
            }
        }
        if (msg.HitObjectName == "Tick") {
            var tickDmg = new pl.MsgDamage();
            tickDmg.Damage = 1;
            tickDmg.HitObjectName = "Tick";
            this.PostMessage(tickDmg, pl.Time.Milliseconds(100));
        }
    };
    GasCylinder.prototype.Explode = function () {
        var owner = this.GetOwner();
        var exp = owner.FindChildByName("Explosion");
        if (exp != null) {
            var spawnExpl = exp.TryGetComponentOfBaseType(pl.SpawnComponent);
            if (spawnExpl != null) {
                spawnExpl.TriggerManualSpawn(false, pl.Vec3.ZeroVector());
            }
        }
        pl.World.DeleteObjectDelayed(this.GetOwner());
    };
    return GasCylinder;
}(pl.TickedTypescriptComponent));
exports.GasCylinder = GasCylinder;
