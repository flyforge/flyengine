/*SOURCE-HASH:EC041BF3C50B2891*/
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
var PhysicsLevelScript = /** @class */ (function (_super) {
    __extends(PhysicsLevelScript, _super);
    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */
    function PhysicsLevelScript() {
        return _super.call(this) || this;
    }
    PhysicsLevelScript.RegisterMessageHandlers = function () {
        // you can only call "RegisterMessageHandler" from within this function
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgTriggerTriggered, "OnMsgTriggerTriggered");
    };
    PhysicsLevelScript.prototype.OnSimulationStarted = function () {
    };
    PhysicsLevelScript.prototype.OnMsgTriggerTriggered = function (msg) {
        if (msg.Message == "ActivatePaddleWheel") {
            if (msg.TriggerState == pl.TriggerState.Activated) {
                var spawn1 = pl.World.TryGetObjectWithGlobalKey("PaddleWheelSpawn1");
                if (spawn1 != null) {
                    spawn1.SetActiveFlag(true);
                }
            }
            else if (msg.TriggerState == pl.TriggerState.Deactivated) {
                var spawn1 = pl.World.TryGetObjectWithGlobalKey("PaddleWheelSpawn1");
                if (spawn1 != null) {
                    spawn1.SetActiveFlag(false);
                }
            }
        }
        if (msg.Message == "ActivateSwing") {
            if (msg.TriggerState == pl.TriggerState.Activated) {
                var spawn1 = pl.World.TryGetObjectWithGlobalKey("SwingSpawn1");
                if (spawn1 != null) {
                    spawn1.SetActiveFlag(true);
                }
            }
            else if (msg.TriggerState == pl.TriggerState.Deactivated) {
                var spawn1 = pl.World.TryGetObjectWithGlobalKey("SwingSpawn1");
                if (spawn1 != null) {
                    spawn1.SetActiveFlag(false);
                }
            }
        }
    };
    return PhysicsLevelScript;
}(pl.TypescriptComponent));
exports.PhysicsLevelScript = PhysicsLevelScript;
