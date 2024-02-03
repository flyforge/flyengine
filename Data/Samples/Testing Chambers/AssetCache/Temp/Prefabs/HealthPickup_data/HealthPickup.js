/*SOURCE-HASH:6A4FDB11BC19A705*/
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
var HealthPickup = /** @class */ (function (_super) {
    __extends(HealthPickup, _super);
    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */
    function HealthPickup() {
        var _this = _super.call(this) || this;
        _this.pfxTop = null;
        _this.pfxPickup = null;
        return _this;
    }
    HealthPickup.prototype.OnSimulationStarted = function () {
        this.pfxTop = this.GetOwner().FindChildByName("Particle", true).TryGetComponentOfBaseType(pl.ParticleComponent);
        this.pfxPickup = this.GetOwner().TryGetComponentOfBaseType(pl.ParticleComponent);
    };
    HealthPickup.RegisterMessageHandlers = function () {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgTriggerTriggered, "OnMsgTriggerTriggered");
    };
    HealthPickup.prototype.OnMsgGenericEvent = function (msg) {
        if (msg.Message == "Animation Cue 1") {
            this.pfxTop.StartEffect();
        }
    };
    HealthPickup.prototype.OnMsgTriggerTriggered = function (msg) {
        if (msg.TriggerState == pl.TriggerState.Activated && msg.Message == "Pickup") {
            this.pfxPickup.StartEffect();
            var del = new pl.MsgDeleteGameObject();
            this.GetOwner().PostMessage(del, 0.1);
        }
    };
    return HealthPickup;
}(pl.TypescriptComponent));
exports.HealthPickup = HealthPickup;
