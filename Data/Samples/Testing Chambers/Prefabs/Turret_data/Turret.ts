import pl = require("TypeScript/pl")

export class Turret extends pl.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    Health: number = 50;
    /* END AUTO-GENERATED: VARIABLES */

    target: pl.GameObject = null;
    gunSpawn: pl.SpawnComponent = null;
    gunSound: pl.FmodEventComponent = null;

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgDamage, "OnMsgDamage");
    }

    OnMsgDamage(msg: pl.MsgDamage) {

        if (this.Health <= 0)
            return;

        this.Health -= msg.Damage;

        if (this.Health > 0)
            return;

        let expObj = this.GetOwner().FindChildByName("Explosion", true);
        if (expObj == null)
            return;

        let expComp = expObj.TryGetComponentOfBaseType(pl.SpawnComponent);
        if (expComp == null)
            return;

        expComp.TriggerManualSpawn(true, pl.Vec3.ZeroVector());
    }

    OnSimulationStarted(): void {
        this.SetTickInterval(pl.Time.Milliseconds(50));

        let gun = this.GetOwner().FindChildByName("Gun", true);

        this.gunSpawn = gun.TryGetComponentOfBaseType(pl.SpawnComponent);
        this.gunSound = gun.TryGetComponentOfBaseType(pl.FmodEventComponent);
    }


    FoundObjectCallback = (go: pl.GameObject): boolean => {

        this.target = go;

        return false;
    }

    Tick(): void {

        if (this.Health <= 0)
            return;

        if (this.gunSpawn == null || !this.gunSpawn.IsValid())
            return;

        let owner = this.GetOwner();

        this.target = null;
        pl.World.FindObjectsInSphere("Player", owner.GetGlobalPosition(), 15, this.FoundObjectCallback);

        if (this.target == null)
            return;

        let dirToTarget = new pl.Vec3();
        dirToTarget.SetSub(this.target.GetGlobalPosition(), owner.GetGlobalPosition());

        let distance = dirToTarget.GetLength();

        let vis = pl.Physics.Raycast(owner.GetGlobalPosition(), dirToTarget, distance, 7, pl.Physics.ShapeType.Static);
        if (vis != null)
            return;

        let targetRotation = new pl.Quat();
        targetRotation.SetShortestRotation(pl.Vec3.UnitAxisX(), dirToTarget);

        let newRotation = new pl.Quat();
        newRotation.SetSlerp(owner.GetGlobalRotation(), targetRotation, 0.1);

        owner.SetGlobalRotation(newRotation);

        dirToTarget.Normalize();


        if (dirToTarget.Dot(owner.GetGlobalDirForwards()) > Math.cos(pl.Angle.DegreeToRadian(15))) {

            this.gunSpawn.ScheduleSpawn();
            this.gunSound.StartOneShot();
        }
    }
}

