/*SOURCE-HASH:6BB9F2CD3198BD9C*/
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
var Turret = /** @class */ (function (_super) {
    __extends(Turret, _super);
    function Turret() {
        var _this = _super.call(this) || this;
        /* BEGIN AUTO-GENERATED: VARIABLES */
        _this.Health = 50;
        /* END AUTO-GENERATED: VARIABLES */
        _this.target = null;
        _this.gunSpawn = null;
        _this.gunSound = null;
        _this.FoundObjectCallback = function (go) {
            _this.target = go;
            return false;
        };
        return _this;
    }
    Turret.RegisterMessageHandlers = function () {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgDamage, "OnMsgDamage");
    };
    Turret.prototype.OnMsgDamage = function (msg) {
        if (this.Health <= 0)
            return;
        this.Health -= msg.Damage;
        if (this.Health > 0)
            return;
        var expObj = this.GetOwner().FindChildByName("Explosion", true);
        if (expObj == null)
            return;
        var expComp = expObj.TryGetComponentOfBaseType(pl.SpawnComponent);
        if (expComp == null)
            return;
        expComp.TriggerManualSpawn(true, pl.Vec3.ZeroVector());
    };
    Turret.prototype.OnSimulationStarted = function () {
        this.SetTickInterval(pl.Time.Milliseconds(50));
        var gun = this.GetOwner().FindChildByName("Gun", true);
        this.gunSpawn = gun.TryGetComponentOfBaseType(pl.SpawnComponent);
        this.gunSound = gun.TryGetComponentOfBaseType(pl.FmodEventComponent);
    };
    Turret.prototype.Tick = function () {
        if (this.Health <= 0)
            return;
        if (this.gunSpawn == null || !this.gunSpawn.IsValid())
            return;
        var owner = this.GetOwner();
        this.target = null;
        pl.World.FindObjectsInSphere("Player", owner.GetGlobalPosition(), 15, this.FoundObjectCallback);
        if (this.target == null)
            return;
        var dirToTarget = new pl.Vec3();
        dirToTarget.SetSub(this.target.GetGlobalPosition(), owner.GetGlobalPosition());
        var distance = dirToTarget.GetLength();
        var vis = pl.Physics.Raycast(owner.GetGlobalPosition(), dirToTarget, distance, 7, pl.Physics.ShapeType.Static);
        if (vis != null)
            return;
        var targetRotation = new pl.Quat();
        targetRotation.SetShortestRotation(pl.Vec3.UnitAxisX(), dirToTarget);
        var newRotation = new pl.Quat();
        newRotation.SetSlerp(owner.GetGlobalRotation(), targetRotation, 0.1);
        owner.SetGlobalRotation(newRotation);
        dirToTarget.Normalize();
        if (dirToTarget.Dot(owner.GetGlobalDirForwards()) > Math.cos(pl.Angle.DegreeToRadian(15))) {
            this.gunSpawn.ScheduleSpawn();
            this.gunSound.StartOneShot();
        }
    };
    return Turret;
}(pl.TickedTypescriptComponent));
exports.Turret = Turret;
