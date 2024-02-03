/*SOURCE-HASH:964B9DFB3F840017*/
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
var pl = require("../../TypeScript/pl");
var WallMine = /** @class */ (function (_super) {
    __extends(WallMine, _super);
    function WallMine() {
        var _this = _super.call(this) || this;
        /* BEGIN AUTO-GENERATED: VARIABLES */
        /* END AUTO-GENERATED: VARIABLES */
        _this.distance = 0;
        return _this;
    }
    WallMine.prototype.OnSimulationStarted = function () {
        this.SetTickInterval(pl.Time.Milliseconds(40));
    };
    WallMine.prototype.Tick = function () {
        var owner = this.GetOwner();
        var pos = owner.GetGlobalPosition();
        var dir = owner.GetGlobalDirForwards();
        var shapeId = -1;
        var staticactor = owner.TryGetComponentOfBaseType(pl.JoltStaticActorComponent);
        if (staticactor != null) {
            shapeId = staticactor.GetObjectFilterID();
        }
        var res = pl.Physics.Raycast(pos, dir, 10, 0, pl.Physics.ShapeType.Static | pl.Physics.ShapeType.AllInteractive, shapeId);
        if (res == null) {
            return;
        }
        if (res.distance < this.distance - 0.05) {
            // allow some slack
            this.Explode();
        }
        else if (res.distance > this.distance) {
            var glowLine = owner.FindChildByName("GlowLine", false);
            if (glowLine != null) {
                glowLine.SetLocalScaling(new pl.Vec3(res.distance, 1, 1));
                glowLine.SetLocalPosition(new pl.Vec3(res.distance * 0.5, 0, 0));
            }
            this.distance = res.distance;
        }
    };
    WallMine.prototype.Explode = function () {
        var owner = this.GetOwner();
        var exp = owner.FindChildByName("Explosion");
        if (exp != null) {
            var spawnExpl = exp.TryGetComponentOfBaseType(pl.SpawnComponent);
            if (spawnExpl != null) {
                spawnExpl.TriggerManualSpawn(true, pl.Vec3.ZeroVector());
            }
        }
        pl.World.DeleteObjectDelayed(this.GetOwner());
    };
    // to use message handlers you must implement exactly this function
    WallMine.RegisterMessageHandlers = function () {
        // you can only call "RegisterMessageHandler" from within this function
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgDamage, "OnMsgDamage");
    };
    WallMine.prototype.OnMsgDamage = function (msg) {
        // explode on any damage
        this.Explode();
    };
    return WallMine;
}(pl.TickedTypescriptComponent));
exports.WallMine = WallMine;
