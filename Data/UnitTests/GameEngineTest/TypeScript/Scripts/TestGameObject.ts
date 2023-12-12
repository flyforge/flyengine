import pl = require("TypeScript/pl")
import PLASMA_TEST = require("./TestFramework")

export class TestGameObject extends pl.TypescriptComponent {

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
        let child1 = owner.FindChildByName("Child1");
        let child2 = owner.FindChildByPath("Child2");


        // IsValid
        {
            PLASMA_TEST.BOOL(owner.IsValid());
            PLASMA_TEST.BOOL(child1.IsValid());
            PLASMA_TEST.BOOL(child2.IsValid());
        }

        // GetName / SetName
        {
            PLASMA_TEST.BOOL(owner.GetName() == "GameObject");
            owner.SetName("TestGameObject");
            PLASMA_TEST.BOOL(owner.GetName() == "TestGameObject");
        }

        // Active Flag / Active State
        {
            PLASMA_TEST.BOOL(child1.GetActiveFlag());
            PLASMA_TEST.BOOL(child1.IsActive());
            PLASMA_TEST.BOOL(child2.GetActiveFlag());
            PLASMA_TEST.BOOL(child2.IsActive());

            child2.SetActiveFlag(false);

            PLASMA_TEST.BOOL(child1.GetActiveFlag());
            PLASMA_TEST.BOOL(child1.IsActive());
            PLASMA_TEST.BOOL(!child2.GetActiveFlag());
            PLASMA_TEST.BOOL(!child2.IsActive());

            child2.SetActiveFlag(true);

            PLASMA_TEST.BOOL(child1.GetActiveFlag());
            PLASMA_TEST.BOOL(child1.IsActive());
            PLASMA_TEST.BOOL(child2.GetActiveFlag());
            PLASMA_TEST.BOOL(child2.IsActive());
        }

        // Local Position
        {
            PLASMA_TEST.VEC3(child1.GetLocalPosition(), new pl.Vec3(1, 2, 3));
            PLASMA_TEST.VEC3(child2.GetLocalPosition(), new pl.Vec3(4, 5, 6));

            child1.SetLocalPosition(new pl.Vec3(11, 22, 33));
            PLASMA_TEST.VEC3(child1.GetLocalPosition(), new pl.Vec3(11, 22, 33));
        }

        // Local Rotation
        {
            PLASMA_TEST.QUAT(child1.GetLocalRotation(), pl.Quat.IdentityQuaternion());

            let nr = new pl.Quat();
            nr.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(45));

            child1.SetLocalRotation(nr);
            PLASMA_TEST.QUAT(child1.GetLocalRotation(), nr);
        }

        // Local Scaling
        {
            PLASMA_TEST.VEC3(child2.GetLocalScaling(), new pl.Vec3(2, 3, 4));
            PLASMA_TEST.FLOAT(child2.GetLocalUniformScaling(), 5);

            child2.SetLocalScaling(new pl.Vec3(22, 33, 44));
            child2.SetLocalUniformScaling(55);

            PLASMA_TEST.VEC3(child2.GetLocalScaling(), new pl.Vec3(22, 33, 44));
            PLASMA_TEST.FLOAT(child2.GetLocalUniformScaling(), 55);
        }

        // Global Position
        {
            PLASMA_TEST.VEC3(child1.GetGlobalPosition(), new pl.Vec3(1, 2, 3));
            PLASMA_TEST.VEC3(child2.GetGlobalPosition(), new pl.Vec3(4, 5, 6));

            child1.SetGlobalPosition(new pl.Vec3(11, 22, 33));
            PLASMA_TEST.VEC3(child1.GetGlobalPosition(), new pl.Vec3(11, 22, 33));

        }

        // Global Rotation
        {
            PLASMA_TEST.QUAT(child1.GetGlobalRotation(), pl.Quat.IdentityQuaternion());

            let nr = new pl.Quat();
            nr.SetFromAxisAndAngle(new pl.Vec3(0, 1, 0), pl.Angle.DegreeToRadian(30));

            child1.SetGlobalRotation(nr);
            PLASMA_TEST.QUAT(child1.GetGlobalRotation(), nr);
        }

        // Global Scaling
        {
            PLASMA_TEST.VEC3(child2.GetGlobalScaling(), new pl.Vec3(2 * 5, 3 * 5, 4 * 5));

            child2.SetGlobalScaling(new pl.Vec3(1, 2, 3));

            PLASMA_TEST.VEC3(child2.GetGlobalScaling(), new pl.Vec3(1, 2, 3));
            PLASMA_TEST.FLOAT(child2.GetLocalUniformScaling(), 1);
        }

        // Global Dirs
        {
            child1.SetGlobalRotation(pl.Quat.IdentityQuaternion());

            PLASMA_TEST.VEC3(child1.GetGlobalDirForwards(), new pl.Vec3(1, 0, 0));
            PLASMA_TEST.VEC3(child1.GetGlobalDirRight(), new pl.Vec3(0, 1, 0));
            PLASMA_TEST.VEC3(child1.GetGlobalDirUp(), new pl.Vec3(0, 0, 1));

            let r = new pl.Quat();
            r.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(90));

            child1.SetGlobalRotation(r);

            PLASMA_TEST.VEC3(child1.GetGlobalDirForwards(), new pl.Vec3(0, 1, 0));
            PLASMA_TEST.VEC3(child1.GetGlobalDirRight(), new pl.Vec3(-1, 0, 0));
            PLASMA_TEST.VEC3(child1.GetGlobalDirUp(), new pl.Vec3(0, 0, 1));
        }

        // Velocity
        {
            PLASMA_TEST.VEC3(child1.GetVelocity(), pl.Vec3.ZeroVector());

            child1.SetVelocity(new pl.Vec3(1, 2, 3));
            PLASMA_TEST.VEC3(child1.GetVelocity(), new pl.Vec3(1, 2, 3));
        }

        // Team ID
        {
            PLASMA_TEST.FLOAT(child1.GetTeamID(), 0);
            child1.SetTeamID(11);
            PLASMA_TEST.FLOAT(child1.GetTeamID(), 11);
        }

        // FindChildByName
        {
            let c = owner.FindChildByName("Child1_Child1", false);
            PLASMA_TEST.BOOL(c == null);

            c = owner.FindChildByName("Child1_Child1", true);
            PLASMA_TEST.BOOL(c != null);
            PLASMA_TEST.BOOL(c.IsValid());
            PLASMA_TEST.BOOL(c.GetName() == "Child1_Child1");
        }

        // FindChildByName
        {
            let c = owner.FindChildByPath("Child2_Child1");
            PLASMA_TEST.BOOL(c == null);

            c = owner.FindChildByPath("Child2/Child2_Child1");
            PLASMA_TEST.BOOL(c != null);
            PLASMA_TEST.BOOL(c.IsValid());
            PLASMA_TEST.BOOL(c.GetName() == "Child2_Child1");
        }

        // SearchForChildByNameSequence
        {
            let c = owner.SearchForChildByNameSequence("Child1_Child1/A");
            PLASMA_TEST.BOOL(c != null && c.IsValid());
            PLASMA_TEST.FLOAT(c.GetLocalUniformScaling(), 2);

            c = owner.SearchForChildWithComponentByNameSequence("Child2/A", pl.PointLightComponent);
            PLASMA_TEST.BOOL(c != null && c.IsValid());
            PLASMA_TEST.FLOAT(c.GetLocalUniformScaling(), 3);
        }

        // TryGetComponentOfBaseType
        {
            // let sl = child1.TryGetComponentOfBaseType(pl.SpotLightComponent);
            // PLASMA_TEST.BOOL(sl != null && sl.IsValid());

            let pl = child1.TryGetComponentOfBaseTypeName<pl.SpotLightComponent>("plPointLightComponent");
            PLASMA_TEST.BOOL(pl != null && pl.IsValid());
        }

        // Tags
        {
            PLASMA_TEST.BOOL(owner.HasAllTags("AutoColMesh"));
            PLASMA_TEST.BOOL(owner.HasAllTags("CastShadow"));
            PLASMA_TEST.BOOL(owner.HasAllTags("AutoColMesh", "CastShadow"));
            PLASMA_TEST.BOOL(owner.HasAnyTags("AutoColMesh", "NOTAG"));
            PLASMA_TEST.BOOL(owner.HasAnyTags("CastShadow", "NOTAG"));
            PLASMA_TEST.BOOL(owner.HasAnyTags("AutoColMesh", "CastShadow"));

            owner.RemoveTags("CastShadow", "AutoColMesh");
            PLASMA_TEST.BOOL(!owner.HasAnyTags("AutoColMesh", "CastShadow"));

            owner.AddTags("CastShadow", "TAG1");
            PLASMA_TEST.BOOL(owner.HasAnyTags("AutoColMesh", "CastShadow"));
            PLASMA_TEST.BOOL(!owner.HasAllTags("AutoColMesh", "CastShadow"));

            owner.SetTags("TAG");
            PLASMA_TEST.BOOL(owner.HasAnyTags("TAG"));
            PLASMA_TEST.BOOL(!owner.HasAnyTags("AutoColMesh", "CastShadow", "TAG1"));
        }

        // Global Key
        {
            let obj = pl.World.TryGetObjectWithGlobalKey("Tests");
            PLASMA_TEST.BOOL(obj != null);
            PLASMA_TEST.BOOL(obj.GetName() == "All Tests");

            this.GetOwner().SetGlobalKey("TestGameObjects");
            PLASMA_TEST.BOOL(this.GetOwner().GetGlobalKey() == "TestGameObjects");
            let tgo = pl.World.TryGetObjectWithGlobalKey("TestGameObjects");
            PLASMA_TEST.BOOL(tgo == this.GetOwner());
            this.GetOwner().SetGlobalKey("");
            let tgo2 = pl.World.TryGetObjectWithGlobalKey("TestGameObjects");
            PLASMA_TEST.BOOL(tgo2 == null);
        }

        // GetChildren
        {
            PLASMA_TEST.INT(this.GetOwner().GetChildCount(), 3);

            let children = this.GetOwner().GetChildren();
            PLASMA_TEST.INT(this.GetOwner().GetChildCount(), children.length);

            this.GetOwner().DetachChild(children[0]);
            PLASMA_TEST.INT(this.GetOwner().GetChildCount(), 2);
            PLASMA_TEST.BOOL(children[0].GetParent() == null);

            children[0].SetParent(this.GetOwner());
            PLASMA_TEST.INT(this.GetOwner().GetChildCount(), 3);
            PLASMA_TEST.BOOL(children[0].GetParent() == this.GetOwner());

            this.GetOwner().DetachChild(children[2]);
            PLASMA_TEST.BOOL(children[2].GetParent() == null);
            this.GetOwner().AddChild(children[2]);
            PLASMA_TEST.BOOL(children[2].GetParent() == this.GetOwner());
        }
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "TestGameObject") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

