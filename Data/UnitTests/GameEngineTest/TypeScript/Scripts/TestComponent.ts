import pl = require("TypeScript/pl")
import PLASMA_TEST = require("./TestFramework")

export class TestComponent extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        let owner = this.GetOwner();

        let mesh = owner.TryGetComponentOfBaseType(pl.MeshComponent);
        let text = owner.TryGetComponentOfBaseType(pl.DebugTextComponent);

        // IsValid
        {
            PLASMA_TEST.BOOL(mesh != null && mesh.IsValid());
            PLASMA_TEST.BOOL(text != null && text.IsValid());
        }

        // GetOWner
        {
            PLASMA_TEST.BOOL(mesh.GetOwner() == owner);
            PLASMA_TEST.BOOL(text.GetOwner() == owner);
        }

        // Active Flag / Active State
        {
            PLASMA_TEST.BOOL(mesh.IsActive());
            PLASMA_TEST.BOOL(mesh.IsActiveAndInitialized());
            PLASMA_TEST.BOOL(mesh.IsActiveAndSimulating());
            
            PLASMA_TEST.BOOL(!text.GetActiveFlag());
            PLASMA_TEST.BOOL(!text.IsActive());
            PLASMA_TEST.BOOL(!text.IsActiveAndInitialized());
            
            text.SetActiveFlag(true);
            PLASMA_TEST.BOOL(text.GetActiveFlag());
            PLASMA_TEST.BOOL(text.IsActive());
            PLASMA_TEST.BOOL(text.IsActiveAndInitialized());
            
            mesh.SetActiveFlag(false);
            PLASMA_TEST.BOOL(!mesh.GetActiveFlag());
            PLASMA_TEST.BOOL(!mesh.IsActive());
            PLASMA_TEST.BOOL(!mesh.IsActiveAndInitialized());
            PLASMA_TEST.BOOL(!mesh.IsActiveAndSimulating());
        }

        // GetUniqueID
        {
            // plInvalidIndex
            PLASMA_TEST.INT(mesh.GetUniqueID(), 4294967295);
            PLASMA_TEST.INT(text.GetUniqueID(), 4294967295);
        }

        // TryGetScriptComponent
        {
            let sc = this.GetOwner().TryGetScriptComponent("TestComponent");

            PLASMA_TEST.BOOL(sc == this);
        }

        // interact with C++ components
        {
            let c = pl.World.CreateComponent(this.GetOwner(), pl.MoveToComponent);

            // execute function
            c.SetTargetPosition(new pl.Vec3(1, 2, 3));

            // get/set properties
            c.TranslationSpeed = 23;
            PLASMA_TEST.FLOAT(c.TranslationSpeed, 23);
            
            c.TranslationSpeed = 17;
            PLASMA_TEST.FLOAT(c.TranslationSpeed, 17);
        }
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "TestComponent") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

