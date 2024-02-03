/*SOURCE-HASH:E5F872A6FFD053DC*/
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
var ShootingStar2 = /** @class */ (function (_super) {
    __extends(ShootingStar2, _super);
    function ShootingStar2() {
        var _this = _super.call(this) || this;
        /* BEGIN AUTO-GENERATED: VARIABLES */
        /* END AUTO-GENERATED: VARIABLES */
        _this.ragdollFinished = false;
        return _this;
    }
    ShootingStar2.RegisterMessageHandlers = function () {
        // you can only call "RegisterMessageHandler" from within this function
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgDamage, "OnMsgDamage");
    };
    ShootingStar2.prototype.OnMsgDamage = function (msg) {
        if (!this.ragdollFinished) {
            var col = this.GetOwner().TryGetComponentOfBaseType(pl.JoltHitboxComponent);
            if (col != null) {
                // if present, deactivate the bone collider component, it isn't needed anymore
                col.SetActiveFlag(false);
            }
            var da = this.GetOwner().TryGetComponentOfBaseType(pl.JoltDynamicActorComponent);
            if (da != null) {
                // if present, deactivate the dynamic actor component, it isn't needed anymore
                da.SetActiveFlag(false);
            }
            var rdc = this.GetOwner().TryGetComponentOfBaseType(pl.JoltRagdollComponent);
            if (rdc != null) {
                if (rdc.IsActiveAndSimulating()) {
                    this.ragdollFinished = true;
                    return;
                }
                rdc.SetJointTypeOverride(msg.HitObjectName, pl.SkeletonJointType.None);
                rdc.StartMode = pl.JoltRagdollStartMode.WithCurrentMeshPose;
                rdc.SetActiveFlag(true);
                // we want the ragdoll to get a kick, so send an impulse message
                var imp = new pl.MsgPhysicsAddImpulse();
                imp.Impulse = msg.ImpactDirection.Clone();
                imp.Impulse.MulNumber(Math.min(msg.Damage, 5) * 10);
                imp.GlobalPosition = msg.GlobalPosition.Clone();
                rdc.SendMessage(imp);
            }
        }
    };
    return ShootingStar2;
}(pl.TypescriptComponent));
exports.ShootingStar2 = ShootingStar2;
