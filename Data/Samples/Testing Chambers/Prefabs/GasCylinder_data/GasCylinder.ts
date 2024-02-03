import pl = require("TypeScript/pl")

export class GasCylinder extends pl.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    static RegisterMessageHandlers() {

        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgDamage, "OnMsgDamage");
    }

    private capHealth = 5;
    private bodyHealth = 50;

    constructor() {
        super()
    }

    OnSimulationStarted(): void {
        this.SetTickInterval(pl.Time.Milliseconds(100));
    }

    Tick(): void {

        if (this.capHealth <= 0) {

            let owner = this.GetOwner();
            let cap = owner.FindChildByName("Cap");

            let forceMsg = new pl.MsgPhysicsAddForce();

            forceMsg.GlobalPosition = cap.GetGlobalPosition();
            forceMsg.Force = cap.GetGlobalDirUp();

            let randomDir = pl.Vec3.CreateRandomDirection();
            randomDir.MulNumber(0.6);

            forceMsg.Force.AddVec3(randomDir);
            forceMsg.Force.MulNumber(-400);

            owner.SendMessage(forceMsg);
        }
    }

    OnMsgDamage(msg: pl.MsgDamage) {

        //pl.Log.Info("Damaged: " + msg.HitObjectName + " - " + msg.Damage)

        this.bodyHealth -= msg.Damage;

        if (this.bodyHealth <= 0) {
            this.Explode();
            return;
        }

        if (msg.HitObjectName == "Cap") {

            if (this.capHealth > 0) {

                this.capHealth -= msg.Damage;

                if (this.capHealth <= 0) {

                    this.SetTickInterval(pl.Time.Milliseconds(0));

                    let leakObj = this.GetOwner().FindChildByName("LeakEffect");
                    if (leakObj != null) {

                        let leakFX = leakObj.TryGetComponentOfBaseType(pl.ParticleComponent);

                        if (leakFX != null) {
                            leakFX.StartEffect();
                        }

                        let leakSound = leakObj.TryGetComponentOfBaseType(pl.FmodEventComponent);

                        if (leakSound != null) {
                            leakSound.Restart();
                        }
                    }

                    // trigger code path below
                    msg.HitObjectName = "Tick";
                }
            }
        }

        if (msg.HitObjectName == "Tick") {

            let tickDmg = new pl.MsgDamage();
            tickDmg.Damage = 1;
            tickDmg.HitObjectName = "Tick";
            this.PostMessage(tickDmg, pl.Time.Milliseconds(100));

        }
    }

    Explode(): void {

        let owner = this.GetOwner();
        let exp = owner.FindChildByName("Explosion");

        if (exp != null) {

            let spawnExpl = exp.TryGetComponentOfBaseType(pl.SpawnComponent);

            if (spawnExpl != null) {
                spawnExpl.TriggerManualSpawn(false, pl.Vec3.ZeroVector());
            }
        }

        pl.World.DeleteObjectDelayed(this.GetOwner());
    }
}

