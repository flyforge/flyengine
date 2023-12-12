import pl = require("TypeScript/pl")
import PLASMA_TEST = require("./TestFramework")
import prefab = require("./Prefab")

export class TestUtils extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        // IsNumberEqual
        {
            PLASMA_TEST.BOOL(pl.Utils.IsNumberEqual(13, 14, 0.9) == false);
            PLASMA_TEST.BOOL(pl.Utils.IsNumberEqual(13, 14, 1.01) == true);
        }

        // IsNumberZero
        {
            PLASMA_TEST.BOOL(pl.Utils.IsNumberZero(0.1, 0.09) == false);
            PLASMA_TEST.BOOL(pl.Utils.IsNumberZero(0.1, 0.11) == true);

            PLASMA_TEST.BOOL(pl.Utils.IsNumberZero(-0.1, 0.09) == false);
            PLASMA_TEST.BOOL(pl.Utils.IsNumberZero(-0.1, 0.11) == true);
        }

        // StringToHash
        {
            PLASMA_TEST.BOOL(pl.Utils.StringToHash("a") != pl.Utils.StringToHash("b"));
        }

        // Clamp
        {
            PLASMA_TEST.INT(pl.Utils.Clamp(13, 8, 11), 11);
            PLASMA_TEST.INT(pl.Utils.Clamp(6, 8, 11), 8);
            PLASMA_TEST.INT(pl.Utils.Clamp(9, 8, 11), 9);
        }

        // Saturate
        {
            PLASMA_TEST.FLOAT(pl.Utils.Saturate(-0.7), 0, 0.001);
            PLASMA_TEST.FLOAT(pl.Utils.Saturate(0.3), 0.3, 0.001);
            PLASMA_TEST.FLOAT(pl.Utils.Saturate(1.3), 1.0, 0.001);
        }

        // FindPrefabRootNode / FindPrefabRootScript / Exposed Script Parameters
        {
            let p1 = this.GetOwner().FindChildByName("Prefab1");
            let p2 = this.GetOwner().FindChildByName("Prefab2");

            PLASMA_TEST.BOOL(p1 != null);
            PLASMA_TEST.BOOL(p2 != null);

            {
                let p1r = pl.Utils.FindPrefabRootNode(p1);
                let p1s: prefab.Prefab = pl.Utils.FindPrefabRootScript(p1, "Prefab");

                PLASMA_TEST.BOOL(p1r != null);
                PLASMA_TEST.BOOL(p1r.GetName() == "root");

                PLASMA_TEST.BOOL(p1s != null);
                PLASMA_TEST.FLOAT(p1s.NumberVar, 11, 0.001);
                PLASMA_TEST.BOOL(p1s.BoolVar);
                PLASMA_TEST.BOOL(p1s.StringVar == "Hello");
                PLASMA_TEST.BOOL(p1s.Vec3Var.IsEqual(new pl.Vec3(1, 2, 3)));

                let c = new pl.Color();
                c.SetGammaByteRGBA(227, 106, 6, 255);
                PLASMA_TEST.BOOL(p1s.ColorVar.IsEqualRGBA(c));
            }

            {
                let p2r = pl.Utils.FindPrefabRootNode(p2);
                let p2s: prefab.Prefab = pl.Utils.FindPrefabRootScript(p2, "Prefab");

                PLASMA_TEST.BOOL(p2r != null);
                PLASMA_TEST.BOOL(p2r.GetName() == "root");

                PLASMA_TEST.BOOL(p2s != null);
                PLASMA_TEST.FLOAT(p2s.NumberVar, 2, 0.001);
                PLASMA_TEST.BOOL(p2s.BoolVar == false);
                PLASMA_TEST.BOOL(p2s.StringVar == "Bye");
                PLASMA_TEST.BOOL(p2s.Vec3Var.IsEqual(new pl.Vec3(4, 5, 6)));

                let c = new pl.Color();
                c.SetGammaByteRGBA(6, 164, 227, 255);
                PLASMA_TEST.BOOL(p2s.ColorVar.IsEqualRGBA(c));
            }
        }
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "TestUtils") {

            this.ExecuteTests();
            msg.Message = "done";
        }
    }

}

