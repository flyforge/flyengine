/*SOURCE-HASH:E1136D5DF670E7F2*/
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
var MsgSwitchMonitor = /** @class */ (function (_super) {
    __extends(MsgSwitchMonitor, _super);
    function MsgSwitchMonitor() {
        var _this = _super.call(this) || this;
        _this.TypeNameHash = 914363845;
        return _this;
    }
    MsgSwitchMonitor.GetTypeNameHash = function () { return 914363845; };
    return MsgSwitchMonitor;
}(pl.Message));
exports.MsgSwitchMonitor = MsgSwitchMonitor;
var Monitor = /** @class */ (function (_super) {
    __extends(Monitor, _super);
    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */
    function Monitor() {
        return _super.call(this) || this;
    }
    Monitor.RegisterMessageHandlers = function () {
        pl.TypescriptComponent.RegisterMessageHandler(MsgSwitchMonitor, "OnMsgSwitchMonitor");
    };
    Monitor.prototype.OnMsgSwitchMonitor = function (msg) {
        var owner = this.GetOwner();
        var display = owner.FindChildByName("Display");
        var mat = new pl.MsgSetMeshMaterial();
        mat.MaterialSlot = 0;
        mat.Material = msg.screenMaterial;
        display.SendMessage(mat);
        var activator = display.TryGetComponentOfBaseType(pl.RenderTargetActivatorComponent);
        activator.RenderTarget = msg.renderTarget;
    };
    return Monitor;
}(pl.TypescriptComponent));
exports.Monitor = Monitor;
