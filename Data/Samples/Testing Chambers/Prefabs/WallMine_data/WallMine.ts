import pl = require("../../TypeScript/pl")

export class WallMine extends pl.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    private distance = 0;

    constructor() {
        super()
    }

    OnSimulationStarted(): void {
        this.SetTickInterval(pl.Time.Milliseconds(40));
    }

    Tick(): void {

        let owner = this.GetOwner();
        let pos = owner.GetGlobalPosition();
        let dir = owner.GetGlobalDirForwards();

        let shapeId = -1;
        let staticactor = owner.TryGetComponentOfBaseType(pl.JoltStaticActorComponent);
        if (staticactor != null) {
            shapeId = staticactor.GetObjectFilterID();
        }

        let res = pl.Physics.Raycast(pos, dir, 10, 0, pl.Physics.ShapeType.Static | pl.Physics.ShapeType.AllInteractive , shapeId);

        if (res == null) {
            return;
        }

        if (res.distance < this.distance - 0.05) {
            // allow some slack

            this.Explode();
        }
        else if (res.distance > this.distance) {

            let glowLine = owner.FindChildByName("GlowLine", false);

            if (glowLine != null) {
                glowLine.SetLocalScaling(new pl.Vec3(res.distance, 1, 1));
                glowLine.SetLocalPosition(new pl.Vec3(res.distance * 0.5, 0, 0));
            }

            this.distance = res.distance;
        }
    }

    Explode(): void {

        let owner = this.GetOwner();
        let exp = owner.FindChildByName("Explosion");

        if (exp != null) {

            let spawnExpl = exp.TryGetComponentOfBaseType(pl.SpawnComponent);

            if (spawnExpl != null) {
                spawnExpl.TriggerManualSpawn(true, pl.Vec3.ZeroVector());
            }
        }

        pl.World.DeleteObjectDelayed(this.GetOwner());
    }

    // to use message handlers you must implement exactly this function
    static RegisterMessageHandlers() {
        // you can only call "RegisterMessageHandler" from within this function
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgDamage, "OnMsgDamage");
    }

    OnMsgDamage(msg: pl.MsgDamage): void {
        // explode on any damage
        this.Explode();
    }
}

