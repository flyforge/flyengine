import pl = require("TypeScript/pl")

export class ShootingStar2 extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    ragdollFinished:boolean = false;

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgDamage, "OnMsgDamage");
    }

    OnMsgDamage(msg: pl.MsgDamage): void {

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
    }
}

