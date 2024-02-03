/*SOURCE-HASH:A71D302B8BABE633*/
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
var monitor = require("Features/RenderToTexture/Monitor_data/Monitor");
var Corridor = /** @class */ (function (_super) {
    __extends(Corridor, _super);
    function Corridor() {
        var _this = _super.call(this) || this;
        /* BEGIN AUTO-GENERATED: VARIABLES */
        /* END AUTO-GENERATED: VARIABLES */
        _this.monitor1State = 0;
        return _this;
    }
    Corridor.RegisterMessageHandlers = function () {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
    };
    Corridor.prototype.OnSimulationStarted = function () {
        this.SetTickInterval(pl.Time.Milliseconds(1000));
    };
    Corridor.prototype.OnMsgGenericEvent = function (msg) {
        if (msg.Message == "SecretDoorButton") {
            var door = pl.World.TryGetObjectWithGlobalKey("SecretDoor");
            if (door != null) {
                var slider = door.TryGetComponentOfBaseType(pl.SliderComponent);
                if (slider != null && !slider.Running) {
                    // slider direction toggles automatically, just need to set the running state again
                    slider.Running = true;
                }
            }
        }
        if (msg.Message == "MoveA" || msg.Message == "MoveB") {
            var obj = pl.World.TryGetObjectWithGlobalKey("Obj");
            var move = obj.TryGetComponentOfBaseType(pl.MoveToComponent);
            move.Running = true;
            if (msg.Message == "MoveA") {
                move.SetTargetPosition(new pl.Vec3(10, -1, 1.5));
            }
            else {
                move.SetTargetPosition(new pl.Vec3(10, 3, 1.5));
            }
        }
        if (msg.Message == "SwitchMonitor1") {
            ++this.monitor1State;
            if (this.monitor1State > 2)
                this.monitor1State = 0;
            var msg_1 = new monitor.MsgSwitchMonitor();
            switch (this.monitor1State) {
                case 0:
                    msg_1.screenMaterial = "{ 6c56721b-d71a-4795-88ac-39cae26c39f1 }";
                    msg_1.renderTarget = "{ 2fe9db45-6e52-4e17-8e27-5744f9e8ada6 }";
                    break;
                case 1:
                    msg_1.screenMaterial = "{ eb4cb027-44b2-4f69-8f88-3d5594f0fa9d }";
                    msg_1.renderTarget = "{ 852fa58a-7bea-4486-832b-3a2b2792fea3 }";
                    break;
                case 2:
                    msg_1.screenMaterial = "{ eb842e16-7314-4f8a-8479-0f92e43ca708 }";
                    msg_1.renderTarget = "{ 673e8ea0-b70e-4e47-a72b-037d67024a71 }";
                    break;
            }
            var mon = pl.Utils.FindPrefabRootScript(pl.World.TryGetObjectWithGlobalKey("Monitor1"), "Monitor");
            if (mon != null) {
                // can call the function directly
                mon.OnMsgSwitchMonitor(msg_1);
                //monitor.SendMessage(msg);
            }
        }
    };
    Corridor.prototype.Tick = function () {
    };
    return Corridor;
}(pl.TickedTypescriptComponent));
exports.Corridor = Corridor;
