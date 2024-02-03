import pl = require("TypeScript/pl")

export class BreakOnDamage extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    Health: number = 10;
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgDamage, "OnMsgDamage");
    }

    OnMsgDamage(msg: pl.MsgDamage): void {
        
        if (this.Health > 0) {

            this.Health -= msg.Damage;

            if (this.Health <= 0) {

                let spawnNode = this.GetOwner().FindChildByName("OnBreakSpawn");
                if (spawnNode != null) {

                    let spawnComp = spawnNode.TryGetComponentOfBaseType(pl.SpawnComponent);

                    if (spawnComp != null) {

                        let offset = pl.Vec3.CreateRandomPointInSphere();
                        offset.MulNumber(0.3);
                        spawnComp.TriggerManualSpawn(true, offset);
                    }
                }

                pl.World.DeleteObjectDelayed(this.GetOwner());
            }
        }
    }
}

