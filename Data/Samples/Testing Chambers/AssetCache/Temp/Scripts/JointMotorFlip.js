/*SOURCE-HASH:01B688E99049C664*/
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
var JointMotorFlip = /** @class */ (function (_super) {
    __extends(JointMotorFlip, _super);
    /* END AUTO-GENERATED: VARIABLES */
    function JointMotorFlip() {
        var _this = _super.call(this) || this;
        /* BEGIN AUTO-GENERATED: VARIABLES */
        _this.Seconds = 10;
        return _this;
    }
    JointMotorFlip.RegisterMessageHandlers = function () {
        // you can only call "RegisterMessageHandler" from within this function
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgComponentInternalTrigger, "OnMsgComponentInternalTrigger");
    };
    JointMotorFlip.prototype.OnSimulationStarted = function () {
        var msg = new pl.MsgComponentInternalTrigger();
        msg.Message = "FlipMotor";
        this.PostMessage(msg, this.Seconds);
    };
    JointMotorFlip.prototype.OnMsgComponentInternalTrigger = function (msg) {
        var joint = this.GetOwner().TryGetComponentOfBaseType(pl.JoltHingeConstraintComponent);
        if (joint != null) {
            joint.DriveTargetValue = -joint.DriveTargetValue;
        }
        this.PostMessage(msg, this.Seconds);
    };
    return JointMotorFlip;
}(pl.TypescriptComponent));
exports.JointMotorFlip = JointMotorFlip;
