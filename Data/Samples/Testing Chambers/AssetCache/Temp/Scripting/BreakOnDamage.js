/*SOURCE-HASH:42CB8B02D5695627*/
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
var BreakOnDamage = /** @class */ (function (_super) {
    __extends(BreakOnDamage, _super);
    /* END AUTO-GENERATED: VARIABLES */
    function BreakOnDamage() {
        var _this = _super.call(this) || this;
        /* BEGIN AUTO-GENERATED: VARIABLES */
        _this.Health = 10;
        return _this;
    }
    BreakOnDamage.RegisterMessageHandlers = function () {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgDamage, "OnMsgDamage");
    };
    BreakOnDamage.prototype.OnMsgDamage = function (msg) {
        if (this.Health > 0) {
            this.Health -= msg.Damage;
            if (this.Health <= 0) {
                var spawnNode = this.GetOwner().FindChildByName("OnBreakSpawn");
                if (spawnNode != null) {
                    var spawnComp = spawnNode.TryGetComponentOfBaseType(pl.SpawnComponent);
                    if (spawnComp != null) {
                        var offset = pl.Vec3.CreateRandomPointInSphere();
                        offset.MulNumber(0.3);
                        spawnComp.TriggerManualSpawn(true, offset);
                    }
                }
                pl.World.DeleteObjectDelayed(this.GetOwner());
            }
        }
    };
    return BreakOnDamage;
}(pl.TypescriptComponent));
exports.BreakOnDamage = BreakOnDamage;
