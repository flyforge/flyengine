/*SOURCE-HASH:BEE28A543A697C94*/
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
var _gm = require("Scripting/GameMessages");
var ConsumablePickup = /** @class */ (function (_super) {
    __extends(ConsumablePickup, _super);
    /* END AUTO-GENERATED: VARIABLES */
    function ConsumablePickup() {
        var _this = _super.call(this) || this;
        /* BEGIN AUTO-GENERATED: VARIABLES */
        _this.ConsumableType = 0;
        _this.Amount = 0;
        return _this;
    }
    ConsumablePickup.RegisterMessageHandlers = function () {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgTriggerTriggered, "OnMsgTriggerTriggered");
    };
    ConsumablePickup.prototype.OnMsgTriggerTriggered = function (msg) {
        if (msg.TriggerState == pl.TriggerState.Activated && msg.Message == "Pickup") {
            // TODO: need GO handles in messages to identify who entered the trigger
            var player = pl.World.TryGetObjectWithGlobalKey("Player");
            if (player == null)
                return;
            var hm = new _gm.MsgAddConsumable();
            hm.consumableType = this.ConsumableType;
            hm.amount = this.Amount;
            player.SendMessage(hm, true);
            if (hm.return_consumed == false)
                return;
            var sound = this.GetOwner().TryGetComponentOfBaseType(pl.FmodEventComponent);
            sound.StartOneShot();
            var del = new pl.MsgDeleteGameObject();
            this.GetOwner().PostMessage(del, 0.1);
        }
    };
    return ConsumablePickup;
}(pl.TypescriptComponent));
exports.ConsumablePickup = ConsumablePickup;
