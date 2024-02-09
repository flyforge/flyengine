/*SOURCE-HASH:D2881309E9F4000F*/
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
var BallMineState;
(function (BallMineState) {
    BallMineState[BallMineState["Idle"] = 0] = "Idle";
    BallMineState[BallMineState["Alert"] = 1] = "Alert";
    BallMineState[BallMineState["Approaching"] = 2] = "Approaching";
    BallMineState[BallMineState["Attacking"] = 3] = "Attacking";
})(BallMineState || (BallMineState = {}));
;
var BallMine = /** @class */ (function (_super) {
    __extends(BallMine, _super);
    function BallMine() {
        var _this = _super.call(this) || this;
        /* BEGIN AUTO-GENERATED: VARIABLES */
        _this.AlertDistance = 15;
        _this.ApproachDistance = 10;
        _this.AttackDistance = 1.5;
        _this.RollForce = 100;
        _this.Health = 20;
        _this._state = BallMineState.Idle;
        _this.QueryForNPC = function (go) {
            // just accept the first object that was found
            _this._player = go;
            return false;
        };
        return _this;
    }
    BallMine.prototype.OnSimulationStarted = function () {
        this.SetTickInterval(pl.Time.Seconds(0.5));
        this._player = pl.World.TryGetObjectWithGlobalKey("Player");
    };
    BallMine.prototype.Tick = function () {
        var oldState = this._state;
        var owner = this.GetOwner();
        if (this._player == null || !this._player.IsValid()) {
            this._player = null;
            pl.World.FindObjectsInSphere("Player", owner.GetGlobalPosition(), this.AlertDistance, this.QueryForNPC);
        }
        if (this._player != null && this._player.IsValid()) {
            var playerPos = this._player.GetGlobalPosition();
            var ownPos = this.GetOwner().GetGlobalPosition();
            var diffPos = new pl.Vec3();
            diffPos.SetSub(playerPos, ownPos);
            var distToPlayer = diffPos.GetLength();
            //pl.Log.Dev("Distance to Player: " + distToPlayer);
            if (distToPlayer <= this.ApproachDistance) {
                this._state = BallMineState.Approaching;
                var actor = this.GetOwner().TryGetComponentOfBaseType(pl.JoltDynamicActorComponent);
                if (actor != null) {
                    diffPos.Normalize();
                    diffPos.MulNumber(this.RollForce);
                    actor.AddLinearForce(diffPos);
                }
                //pl.Log.Dev("Attack: " + distToPlayer);
            }
            else if (distToPlayer <= this.AlertDistance) {
                this._state = BallMineState.Alert;
                //pl.Log.Dev("Alert: " + distToPlayer);
            }
            else {
                this._state = BallMineState.Idle;
            }
            if (distToPlayer <= this.AttackDistance) {
                this._state = BallMineState.Attacking;
            }
        }
        else {
            this._state = BallMineState.Idle;
        }
        if (oldState != this._state) {
            switch (this._state) {
                case BallMineState.Idle:
                    {
                        var matMsg = new pl.MsgSetMeshMaterial();
                        matMsg.Material = "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }";
                        this.GetOwner().SendMessageRecursive(matMsg);
                        this.SetTickInterval(pl.Time.Milliseconds(500));
                        return;
                    }
                case BallMineState.Alert:
                    {
                        var matMsg = new pl.MsgSetMeshMaterial();
                        matMsg.Material = "{ 6ae73fcf-e09c-1c3f-54a8-8a80498519fb }";
                        this.GetOwner().SendMessageRecursive(matMsg);
                        this.SetTickInterval(pl.Time.Milliseconds(100));
                        return;
                    }
                case BallMineState.Approaching:
                    {
                        var matMsg = new pl.MsgSetMeshMaterial();
                        matMsg.Material = "{ 49324140-a093-4a75-9c6c-efde65a39fc4 }";
                        this.GetOwner().SendMessageRecursive(matMsg);
                        this.SetTickInterval(pl.Time.Milliseconds(50));
                        return;
                    }
                case BallMineState.Attacking:
                    {
                        this.Explode();
                        this.SetTickInterval(pl.Time.Milliseconds(50));
                        return;
                    }
            }
        }
    };
    BallMine.prototype.Explode = function () {
        var spawnExpl = this.GetOwner().TryGetComponentOfBaseType(pl.SpawnComponent);
        if (spawnExpl != null) {
            spawnExpl.TriggerManualSpawn(true, pl.Vec3.ZeroVector());
        }
        pl.World.DeleteObjectDelayed(this.GetOwner());
    };
    // to use message handlers you must implement exactly this function
    BallMine.RegisterMessageHandlers = function () {
        // you can only call "RegisterMessageHandler" from within this function
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgDamage, "OnMsgDamage");
    };
    // example message handler
    BallMine.prototype.OnMsgDamage = function (msg) {
        if (this.Health > 0) {
            this.Health -= msg.Damage;
            if (this.Health <= 0) {
                this.Explode();
            }
        }
    };
    return BallMine;
}(pl.TickedTypescriptComponent));
exports.BallMine = BallMine;
