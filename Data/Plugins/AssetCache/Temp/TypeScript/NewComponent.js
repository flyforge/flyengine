/*SOURCE-HASH:F0D020AF7382E63B*/
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
var pl = require("<PATH-TO-pl-TS>");
//export class NewComponent extends pl.TypescriptComponent {
var NewComponent = /** @class */ (function (_super) {
    __extends(NewComponent, _super);
    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */
    function NewComponent() {
        return _super.call(this) || this;
    }
    NewComponent.RegisterMessageHandlers = function () {
        // you can only call "RegisterMessageHandler" from within this function
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgSetColor, "OnMsgSetColor");
    };
    // Initialize(): void { }
    // Deinitialize(): void { }
    // OnActivated(): void { }
    // OnDeactivated(): void { }
    NewComponent.prototype.OnSimulationStarted = function () {
        this.SetTickInterval(pl.Time.Milliseconds(100));
    };
    NewComponent.prototype.OnMsgSetColor = function (msg) {
        pl.Log.Info("MsgSetColor: " + msg.Color.r + ", " + msg.Color.g + ", " + msg.Color.b + ", " + msg.Color.a);
    };
    NewComponent.prototype.Tick = function () {
        // if a regular tick is not needed, remove this and derive directly from pl.TypescriptComponent
        pl.Log.Info("NewComponent.Tick()");
    };
    return NewComponent;
}(pl.TickedTypescriptComponent));
exports.NewComponent = NewComponent;
