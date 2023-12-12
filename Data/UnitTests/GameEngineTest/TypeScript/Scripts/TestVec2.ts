import pl = require("TypeScript/pl")
import PLASMA_TEST = require("./TestFramework")

export class TestVec2 extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        // constructor
        let d = new pl.Vec2();
        PLASMA_TEST.FLOAT(d.x, 0, 0.001);
        PLASMA_TEST.FLOAT(d.y, 0, 0.001);

        // ZeroVector
        PLASMA_TEST.VEC2(new pl.Vec2(), pl.Vec2.ZeroVector(), 0.0001);

        let v = new pl.Vec2(1, 2);
        PLASMA_TEST.FLOAT(v.x, 1, 0.001);
        PLASMA_TEST.FLOAT(v.y, 2, 0.001);

        // Clone
        PLASMA_TEST.VEC2(v.Clone(), v, 0.001);

        // TODO: CloneAsVec3
        //PLASMA_TEST.VEC3(v.CloneAsVec3(3), new pl.Vec3(1, 2, 3), 0.001);

        // OneVector
        PLASMA_TEST.VEC2(new pl.Vec2(1, 1), pl.Vec2.OneVector(), 0.0001);

        // UnitAxisX
        PLASMA_TEST.VEC2(new pl.Vec2(1, 0), pl.Vec2.UnitAxisX(), 0.0001);

        // UnitAxisY
        PLASMA_TEST.VEC2(new pl.Vec2(0, 1), pl.Vec2.UnitAxisY(), 0.0001);

        // Set
        v.Set(4, 5);
        PLASMA_TEST.FLOAT(v.x, 4, 0.001);
        PLASMA_TEST.FLOAT(v.y, 5, 0.001);

        // SetVec2
        let v2 = new pl.Vec2();
        v2.SetVec2(v);
        PLASMA_TEST.VEC2(v, v2, 0.0001);

        // SetAll
        v2.SetAll(7);
        PLASMA_TEST.FLOAT(v2.x, 7, 0.001);
        PLASMA_TEST.FLOAT(v2.y, 7, 0.001);

        // SetZero
        v2.SetZero();
        PLASMA_TEST.FLOAT(v2.x, 0, 0.001);
        PLASMA_TEST.FLOAT(v2.y, 0, 0.001);

        // GetLengthSquared
        PLASMA_TEST.FLOAT(v2.GetLengthSquared(), 0, 0.001);
        v2.SetAll(1);

        PLASMA_TEST.FLOAT(v2.GetLengthSquared(), 2, 0.001);

        // GetLength
        PLASMA_TEST.FLOAT(v2.GetLength(), Math.sqrt(2), 0.001);

        // GetLengthAndNormalize
        let l = v2.GetLengthAndNormalize();
        PLASMA_TEST.FLOAT(l, Math.sqrt(2), 0.001);
        PLASMA_TEST.FLOAT(v2.GetLength(), 1, 0.001);

        // IsNormalized
        PLASMA_TEST.BOOL(!v.IsNormalized());

        // Normalize
        v.Normalize();

        PLASMA_TEST.FLOAT(v.GetLength(), 1, 0.001);
        PLASMA_TEST.BOOL(v.IsNormalized());


        // GetNormalized
        v.Set(3, 0);
        PLASMA_TEST.VEC2(v.GetNormalized(), pl.Vec2.UnitAxisX(), 0.0001);

        // NormalizeIfNotZero
        PLASMA_TEST.BOOL(v.NormalizeIfNotZero(pl.Vec2.UnitAxisY(), 0.001));
        PLASMA_TEST.VEC2(v, pl.Vec2.UnitAxisX(), 0.0001);

        // IsZero
        PLASMA_TEST.BOOL(!v.IsZero());

        // SetZero
        v.SetZero();
        PLASMA_TEST.BOOL(v.IsZero());

        PLASMA_TEST.BOOL(!v.NormalizeIfNotZero(pl.Vec2.UnitAxisY(), 0.001));
        PLASMA_TEST.VEC2(v, pl.Vec2.UnitAxisY(), 0.0001);

        // GetNegated
        v.Set(1, 2);
        PLASMA_TEST.VEC2(v.GetNegated(), new pl.Vec2(-1, -2), 0.0001);
        PLASMA_TEST.VEC2(v, new pl.Vec2(1, 2), 0.0001);

        // Negate
        v.Negate();
        PLASMA_TEST.VEC2(v, new pl.Vec2(-1, -2), 0.0001);

        // AddVec2
        v.Set(2, 3);
        v2.Set(5, 6);
        v.AddVec2(v2);
        PLASMA_TEST.VEC2(v, new pl.Vec2(7, 9), 0.0001);

        // SubVec2
        v.SubVec2(v2);
        PLASMA_TEST.VEC2(v, new pl.Vec2(2, 3), 0.0001);

        // MulVec2
        v.MulVec2(v);
        PLASMA_TEST.VEC2(v, new pl.Vec2(4, 9), 0.0001);

        // DivVec2
        v.DivVec2(new pl.Vec2(2, 3));
        PLASMA_TEST.VEC2(v, new pl.Vec2(2, 3), 0.0001);

        // MulNumber
        v.MulNumber(2);
        PLASMA_TEST.VEC2(v, new pl.Vec2(4, 6), 0.0001);

        // DivNumber
        v.DivNumber(2);
        PLASMA_TEST.VEC2(v, new pl.Vec2(2, 3), 0.0001);

        // IsIdentical
        PLASMA_TEST.BOOL(v.IsIdentical(v));
        PLASMA_TEST.BOOL(!v.IsIdentical(v2));

        // IsEqual
        PLASMA_TEST.BOOL(v.IsEqual(new pl.Vec2(2, 3), 0.0001));
        PLASMA_TEST.BOOL(!v.IsEqual(new pl.Vec2(2, 3.5), 0.0001));

        // Dot
        v.Set(2, 3);
        v2.Set(3, 4);
        PLASMA_TEST.FLOAT(v.Dot(v2), 18, 0.001);

        // GetCompMin
        v.Set(2, 4);
        v2.Set(1, 5);
        PLASMA_TEST.VEC2(v.GetCompMin(v2), new pl.Vec2(1, 4), 0.001);

        // GetCompMax
        v.Set(2, 4);
        v2.Set(1, 5);
        PLASMA_TEST.VEC2(v.GetCompMax(v2), new pl.Vec2(2, 5), 0.001);

        // GetCompClamp
        PLASMA_TEST.VEC2(v.GetCompClamp(new pl.Vec2(3, 4), new pl.Vec2(4, 5)), new pl.Vec2(3, 4), 0.001);

        // GetCompMul
        PLASMA_TEST.VEC2(v.GetCompMul(new pl.Vec2(2, 3)), new pl.Vec2(4, 12), 0.001);

        // GetCompDiv
        PLASMA_TEST.VEC2(v.GetCompDiv(new pl.Vec2(2, 4)), pl.Vec2.OneVector(), 0.001);

        // GetAbs
        v.Set(-1, -2);
        PLASMA_TEST.VEC2(v.GetAbs(), new pl.Vec2(1, 2), 0.001);

        // SetAbs
        v2.SetAbs(v);
        PLASMA_TEST.VEC2(v2, new pl.Vec2(1, 2), 0.001);

        // GetReflectedVector
        v.Set(1, 1);
        v2 = v.GetReflectedVector(new pl.Vec2(0, -1));
        PLASMA_TEST.VEC2(v2, new pl.Vec2(1, -1), 0.0001);

        // SetAdd
        v.SetAdd(new pl.Vec2(1, 2), new pl.Vec2(4, 5));
        PLASMA_TEST.VEC2(v, new pl.Vec2(5, 7), 0.0001);

        // SetSub
        v.SetSub(new pl.Vec2(4, 5), new pl.Vec2(1, 2));
        PLASMA_TEST.VEC2(v, new pl.Vec2(3, 3), 0.0001);

        // SetMul
        v.SetMul(new pl.Vec2(1, 2), 2);
        PLASMA_TEST.VEC2(v, new pl.Vec2(2, 4), 0.0001);

        // SetDiv
        v.SetDiv(new pl.Vec2(2, 4), 2);
        PLASMA_TEST.VEC2(v, new pl.Vec2(1, 2), 0.0001);

        // CreateRandomPointInCircle
        {
            let avg = new pl.Vec2();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = pl.Vec2.CreateRandomPointInCircle();

                PLASMA_TEST.BOOL(v.GetLength() <= 1.0);
                PLASMA_TEST.BOOL(!v.IsZero());

                avg.AddVec2(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            PLASMA_TEST.BOOL(avg.IsZero(0.1));
        }

        // CreateRandomDirection
        {
            let avg = new pl.Vec2();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = pl.Vec2.CreateRandomDirection();

                PLASMA_TEST.BOOL(v.IsNormalized());

                avg.AddVec2(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            PLASMA_TEST.BOOL(avg.IsZero(0.1));
        }
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "TestVec2") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

