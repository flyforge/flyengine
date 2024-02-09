/*SOURCE-HASH:3614033EA679232D*/
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
var PushButton = /** @class */ (function (_super) {
    __extends(PushButton, _super);
    function PushButton() {
        var _this = _super.call(this) || this;
        /* BEGIN AUTO-GENERATED: VARIABLES */
        _this.ButtonName = "";
        /* END AUTO-GENERATED: VARIABLES */
        _this.slider = null;
        return _this;
    }
    PushButton.RegisterMessageHandlers = function () {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
    };
    PushButton.prototype.OnSimulationStarted = function () {
        var owner = this.GetOwner();
        var button = owner.FindChildByName("Button");
        this.slider = button.TryGetComponentOfBaseType(pl.TransformComponent);
    };
    PushButton.prototype.OnMsgGenericEvent = function (msg) {
        if (msg.Message == "Use") {
            if (this.slider == null || this.slider.Running)
                return;
            this.slider.SetDirectionForwards(true);
            this.slider.Running = true;
            var newMsg = new pl.MsgGenericEvent();
            newMsg.Message = this.ButtonName;
            this.BroadcastEvent(newMsg);
        }
    };
    return PushButton;
}(pl.TypescriptComponent));
exports.PushButton = PushButton;
