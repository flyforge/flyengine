import pl = require("TypeScript/pl")
import PLASMA_TEST = require("./TestFramework")

export class TestLifetime extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
    }

    step: number = 0;
    obj1: pl.GameObject = null;
    comp1: pl.MeshComponent = null;
    comp2: pl.MeshComponent = null;

    ExecuteTests(): boolean {

        if (this.step == 0) {

            let d = new pl.GameObjectDesc;
            d.Name = "Jonny";
            d.LocalPosition = new pl.Vec3(1, 2, 3);
            d.Dynamic = true;
            d.Parent = this.GetOwner();

            this.obj1 = pl.World.CreateObject(d);

            PLASMA_TEST.BOOL(this.obj1.GetName() == d.Name);
            PLASMA_TEST.BOOL(this.obj1.GetParent() == this.GetOwner());
            PLASMA_TEST.BOOL(this.obj1.GetLocalPosition().IsEqual(d.LocalPosition));
            PLASMA_TEST.BOOL(this.obj1.GetLocalRotation().IsEqualRotation(pl.Quat.IdentityQuaternion()));
            PLASMA_TEST.BOOL(this.obj1.GetLocalScaling().IsEqual(pl.Vec3.OneVector()));
            PLASMA_TEST.FLOAT(this.obj1.GetLocalUniformScaling(), 1.0, 0.0001);

            this.comp1 = pl.World.CreateComponent(this.obj1, pl.MeshComponent);
            this.comp1.Mesh = "{ 6d619c33-6611-432b-a924-27b1b9bfd8db }"; // Box
            this.comp1.Color = pl.Color.BlueViolet();

            this.comp2 = pl.World.CreateComponent(this.obj1, pl.MeshComponent);
            this.comp2.Mesh = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }"; // Sphere
            this.comp2.Color = pl.Color.PaleVioletRed();

            return true;
        }

        if (this.step == 1) {
            PLASMA_TEST.BOOL(this.obj1 != null);
            PLASMA_TEST.BOOL(this.obj1.IsValid());

            PLASMA_TEST.BOOL(this.comp1.IsValid());
            PLASMA_TEST.BOOL(this.comp2.IsValid());

            this.obj1.SetLocalUniformScaling(2.0);
            
            pl.World.DeleteComponent(this.comp2);
            
            PLASMA_TEST.BOOL(!this.comp2.IsValid());

            return true;
        }

        if (this.step == 2) {
            PLASMA_TEST.BOOL(this.obj1 != null);
            PLASMA_TEST.BOOL(this.obj1.IsValid());

            PLASMA_TEST.BOOL(this.comp1.IsValid());
            PLASMA_TEST.BOOL(!this.comp2.IsValid());

            pl.World.DeleteObjectDelayed(this.obj1);

            // still valid this frame
            PLASMA_TEST.BOOL(this.obj1.IsValid());
            PLASMA_TEST.BOOL(this.comp1.IsValid());

            return true;
        }

        if (this.step == 3) {
            PLASMA_TEST.BOOL(this.obj1 != null);
            PLASMA_TEST.BOOL(!this.obj1.IsValid());
            PLASMA_TEST.BOOL(!this.comp1.IsValid());
            PLASMA_TEST.BOOL(!this.comp2.IsValid());
        }

        return false;
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "TestLifetime") {

            if (this.ExecuteTests()) {
                msg.Message = "repeat";
            }
            else {
                msg.Message = "done";
            }

            this.step += 1;
        }
    }

}

