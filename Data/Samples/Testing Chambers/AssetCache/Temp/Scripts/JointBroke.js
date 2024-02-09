/*SOURCE-HASH:6D33F6752C9A4021*/
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
var JointBroke = /** @class */ (function (_super) {
    __extends(JointBroke, _super);
    /* END AUTO-GENERATED: VARIABLES */
    function JointBroke() {
        var _this = _super.call(this) || this;
        /* BEGIN AUTO-GENERATED: VARIABLES */
        _this.OnBreakMsg = "Joint Broke !";
        return _this;
    }
    JointBroke.RegisterMessageHandlers = function () {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgPhysicsJointBroke, "OnMsgPhysicsJointBroke");
    };
    JointBroke.prototype.OnMsgPhysicsJointBroke = function (msg) {
        pl.Log.Info(this.OnBreakMsg);
    };
    return JointBroke;
}(pl.TypescriptComponent));
exports.JointBroke = JointBroke;
