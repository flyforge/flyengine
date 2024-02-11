/*SOURCE-HASH:219B869B7DAB9860*/
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
var UnlockWeapon = /** @class */ (function (_super) {
    __extends(UnlockWeapon, _super);
    /* END AUTO-GENERATED: VARIABLES */
    function UnlockWeapon() {
        var _this = _super.call(this) || this;
        /* BEGIN AUTO-GENERATED: VARIABLES */
        _this.WeaponType = 0;
        return _this;
    }
    UnlockWeapon.RegisterMessageHandlers = function () {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgTriggerTriggered, "OnMsgTriggerTriggered");
    };
    UnlockWeapon.prototype.OnMsgTriggerTriggered = function (msg) {
        if (msg.TriggerState == pl.TriggerState.Activated && msg.Message == "Pickup") {
            // TODO: need GO handles in messages to identify who entered the trigger
            var player = pl.World.TryGetObjectWithGlobalKey("Player");
            if (player == null)
                return;
            var hm = new _gm.MsgUnlockWeapon();
            hm.WeaponType = this.WeaponType;
            player.SendMessage(hm, true);
            if (hm.return_consumed == false)
                return;
            var sound = this.GetOwner().TryGetComponentOfBaseType(pl.FmodEventComponent);
            if (sound != null)
                sound.StartOneShot();
            var del = new pl.MsgDeleteGameObject();
            this.GetOwner().PostMessage(del, 0.1);
        }
    };
    return UnlockWeapon;
}(pl.TypescriptComponent));
exports.UnlockWeapon = UnlockWeapon;
