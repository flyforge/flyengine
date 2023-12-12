import pl = require("TypeScript/pl")
import PLASMA_TEST = require("./TestFramework")

export class TestTransform extends pl.TypescriptComponent {

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
        {
            let t = new pl.Transform();

            PLASMA_TEST.VEC3(t.position, pl.Vec3.ZeroVector());
            PLASMA_TEST.QUAT(t.rotation, pl.Quat.IdentityQuaternion());
            PLASMA_TEST.VEC3(t.scale, pl.Vec3.OneVector());
        }

        // Clone
        {
            let t = new pl.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(pl.Vec3.UnitAxisZ(), pl.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            let c = t.Clone();
            PLASMA_TEST.BOOL(t != c);
            PLASMA_TEST.BOOL(t.position != c.position);
            PLASMA_TEST.BOOL(t.rotation != c.rotation);
            PLASMA_TEST.BOOL(t.scale != c.scale);

            PLASMA_TEST.VEC3(t.position, c.position);
            PLASMA_TEST.QUAT(t.rotation, c.rotation);
            PLASMA_TEST.VEC3(t.scale, c.scale);
        }

        // SetTransform
        {
            let t = new pl.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(pl.Vec3.UnitAxisZ(), pl.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            let c = new pl.Transform();
            c.SetTransform(t);

            PLASMA_TEST.BOOL(t != c);
            PLASMA_TEST.BOOL(t.position != c.position);
            PLASMA_TEST.BOOL(t.rotation != c.rotation);
            PLASMA_TEST.BOOL(t.scale != c.scale);

            PLASMA_TEST.VEC3(t.position, c.position);
            PLASMA_TEST.QUAT(t.rotation, c.rotation);
            PLASMA_TEST.VEC3(t.scale, c.scale);
        }

        // SetIdentity
        {
            let t = new pl.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(pl.Vec3.UnitAxisZ(), pl.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            t.SetIdentity();

            PLASMA_TEST.VEC3(t.position, pl.Vec3.ZeroVector());
            PLASMA_TEST.QUAT(t.rotation, pl.Quat.IdentityQuaternion());
            PLASMA_TEST.VEC3(t.scale, pl.Vec3.OneVector());
        }

        // IdentityTransform
        {
            let t = pl.Transform.IdentityTransform();

            PLASMA_TEST.VEC3(t.position, pl.Vec3.ZeroVector());
            PLASMA_TEST.QUAT(t.rotation, pl.Quat.IdentityQuaternion());
            PLASMA_TEST.VEC3(t.scale, pl.Vec3.OneVector());
        }

        // IsIdentical
        {
            let t = new pl.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(pl.Vec3.UnitAxisZ(), pl.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            let c = new pl.Transform();
            c.SetTransform(t);

            PLASMA_TEST.BOOL(t.IsIdentical(c));

            c.position.x += 0.0001;

            PLASMA_TEST.BOOL(!t.IsIdentical(c));
        }

        // IsEqual
        {
            let t = new pl.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(pl.Vec3.UnitAxisZ(), pl.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            let c = new pl.Transform();
            c.SetTransform(t);

            PLASMA_TEST.BOOL(t.IsEqual(c));

            c.position.x += 0.0001;

            PLASMA_TEST.BOOL(t.IsEqual(c, 0.001));
            PLASMA_TEST.BOOL(!t.IsEqual(c, 0.00001));
        }

        // Translate
        {
            let t = new pl.Transform();
            t.Translate(new pl.Vec3(1, 2, 3));

            PLASMA_TEST.VEC3(t.position, new pl.Vec3(1, 2, 3));
            PLASMA_TEST.QUAT(t.rotation, pl.Quat.IdentityQuaternion());
            PLASMA_TEST.VEC3(t.scale, pl.Vec3.OneVector());
        }

        // SetMulTransform / MulTransform
        {
            let tParent = new pl.Transform();
            tParent.position.Set(1, 2, 3);

            tParent.rotation.SetFromAxisAndAngle(new pl.Vec3(0, 1, 0), pl.Angle.DegreeToRadian(90));
            tParent.scale.SetAll(2);

            let tToChild = new pl.Transform();
            tToChild.position.Set(4, 5, 6);

            tToChild.rotation.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(90));
            tToChild.scale.SetAll(4);

            // this is exactly the same as SetGlobalTransform
            let tChild = new pl.Transform();
            tChild.SetMulTransform(tParent, tToChild);

            PLASMA_TEST.BOOL(tChild.position.IsEqual(new pl.Vec3(13, 12, -5), 0.0001));

            let q1 = new pl.Quat();
            q1.SetConcatenatedRotations(tParent.rotation, tToChild.rotation);
            PLASMA_TEST.BOOL(tChild.rotation.IsEqualRotation(q1, 0.0001));

            PLASMA_TEST.VEC3(tChild.scale, new pl.Vec3(8, 8, 8));

            tChild = tParent.Clone();
            tChild.MulTransform(tToChild);

            PLASMA_TEST.BOOL(tChild.position.IsEqual(new pl.Vec3(13, 12, -5), 0.0001));

            q1.SetConcatenatedRotations(tParent.rotation, tToChild.rotation);
            PLASMA_TEST.QUAT(tChild.rotation, q1);
            PLASMA_TEST.VEC3(tChild.scale, new pl.Vec3(8, 8, 8));

            let a = new pl.Vec3(7, 8, 9);
            let b = a.Clone();
            tToChild.TransformPosition(b);
            tParent.TransformPosition(b);

            let c = a.Clone();
            tChild.TransformPosition(c);

            PLASMA_TEST.VEC3(b, c);
        }

        // Invert / GetInverse
        {
            let tParent = new pl.Transform();
            tParent.position.Set(1, 2, 3);

            tParent.rotation.SetFromAxisAndAngle(new pl.Vec3(0, 1, 0), pl.Angle.DegreeToRadian(90));
            tParent.scale.SetAll(2);

            let tToChild = new pl.Transform();
            tParent.position.Set(4, 5, 6);

            tToChild.rotation.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(90));
            tToChild.scale.SetAll(4);

            let tChild = new pl.Transform();
            tChild.SetMulTransform(tParent, tToChild);

            // invert twice -> get back original
            let t2 = tToChild.Clone();
            t2.Invert();
            PLASMA_TEST.BOOL(!t2.IsEqual(tToChild, 0.0001));
            t2 = t2.GetInverse();
            PLASMA_TEST.BOOL(t2.IsEqual(tToChild, 0.0001));

            let tInvToChild = tToChild.GetInverse();

            let tParentFromChild = new pl.Transform();
            tParentFromChild.SetMulTransform(tChild, tInvToChild);

            PLASMA_TEST.BOOL(tParent.IsEqual(tParentFromChild, 0.0001));
        }

        // SetLocalTransform
        {
            let q = new pl.Quat();
            q.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(90));

            let tParent = new pl.Transform();
            tParent.position.Set(1, 2, 3);
            tParent.rotation.SetFromAxisAndAngle(new pl.Vec3(0, 1, 0), pl.Angle.DegreeToRadian(90));
            tParent.scale.SetAll(2);

            let tChild = new pl.Transform();
            tChild.position.Set(13, 12, -5);
            tChild.rotation.SetConcatenatedRotations(tParent.rotation, q);
            tChild.scale.SetAll(8);

            let tToChild = new pl.Transform();
            tToChild.SetLocalTransform(tParent, tChild);

            PLASMA_TEST.VEC3(tToChild.position, new pl.Vec3(4, 5, 6));
            PLASMA_TEST.QUAT(tToChild.rotation, q);
            PLASMA_TEST.VEC3(tToChild.scale, new pl.Vec3(4, 4, 4));
        }

        // SetGlobalTransform
        {
            let tParent = new pl.Transform();
            tParent.position.Set(1, 2, 3);
            tParent.rotation.SetFromAxisAndAngle(new pl.Vec3(0, 1, 0), pl.Angle.DegreeToRadian(90));
            tParent.scale.SetAll(2);

            let tToChild = new pl.Transform();
            tToChild.position.Set(4, 5, 6);
            tToChild.rotation.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(90));
            tToChild.scale.SetAll(4);

            let tChild = new pl.Transform();
            tChild.SetGlobalTransform(tParent, tToChild);

            PLASMA_TEST.VEC3(tChild.position, new pl.Vec3(13, 12, -5));

            let q = new pl.Quat();
            q.SetConcatenatedRotations(tParent.rotation, tToChild.rotation);
            PLASMA_TEST.QUAT(tChild.rotation, q);
            PLASMA_TEST.VEC3(tChild.scale, new pl.Vec3(8, 8, 8));
        }

        // TransformPosition / TransformDirection
        {
            let qRotX = new pl.Quat();
            let qRotY = new pl.Quat();

            qRotX.SetFromAxisAndAngle(new pl.Vec3(1, 0, 0), pl.Angle.DegreeToRadian(90));
            qRotY.SetFromAxisAndAngle(new pl.Vec3(0, 1, 0), pl.Angle.DegreeToRadian(90));

            let t = new pl.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetConcatenatedRotations(qRotY, qRotX);
            t.scale.Set(2, -2, 4);

            let v = new pl.Vec3(4, 5, 6);
            t.TransformPosition(v);
            PLASMA_TEST.VEC3(v, new pl.Vec3((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3));

            v.Set(4, 5, 6);
            t.TransformDirection(v);
            PLASMA_TEST.VEC3(v, new pl.Vec3((5 * -2), (-6 * 4), (-4 * 2)));
        }

        // ConcatenateRotations / ConcatenateRotationsReverse
        {
            let t = new pl.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(new pl.Vec3(0, 1, 0), pl.Angle.DegreeToRadian(90));
            t.scale.SetAll(2);

            let q = new pl.Quat();
            q.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(90));

            let t2 = t.Clone();
            let t4 = t.Clone();
            t2.ConcatenateRotations(q);
            t4.ConcatenateRotationsReverse(q);

            let t3 = t.Clone();
            t3.ConcatenateRotations(q);
            PLASMA_TEST.BOOL(t2.IsEqual(t3));
            PLASMA_TEST.BOOL(!t3.IsEqual(t4));

            let a = new pl.Vec3(7, 8, 9);
            let b = a.Clone();
            t2.TransformPosition(b);

            let c = a.Clone();
            q.RotateVec3(c);
            t.TransformPosition(c);

            PLASMA_TEST.VEC3(b, c);
        }

        // GetAsMat4
        {
            let t = new pl.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(new pl.Vec3(0, 1, 0), pl.Angle.DegreeToRadian(34));
            t.scale.Set(2, -1, 5);

            let m = t.GetAsMat4();

            // reference
            {
                let q = new pl.Quat();
                q.SetFromAxisAndAngle(new pl.Vec3(0, 1, 0), pl.Angle.DegreeToRadian(34));

                let referenceTransform = new pl.Transform();
                referenceTransform.position.Set(1, 2, 3);
                referenceTransform.rotation.SetQuat(q);
                referenceTransform.scale.Set(2, -1, 5);

                let refM = referenceTransform.GetAsMat4();

                PLASMA_TEST.BOOL(m.IsEqual(refM));
            }

            let p: pl.Vec3[] = [new pl.Vec3(- 4, 0, 0), new pl.Vec3(5, 0, 0), new pl.Vec3(0, -6, 0), new pl.Vec3(0, 7, 0),
            new pl.Vec3(0, 0, -8), new pl.Vec3(0, 0, 9), new pl.Vec3(1, -2, 3), new pl.Vec3(-4, 5, 7)];

            for (let i = 0; i < 8; ++i) {

                let pt = p[i].Clone();
                t.TransformPosition(pt);

                let pm = p[i].Clone();
                m.TransformPosition(pm);

                PLASMA_TEST.VEC3(pt, pm);
            }
        }

        // SetFromMat4
        {
            let mRot = new pl.Mat3();
            mRot.SetRotationMatrix((new pl.Vec3(1, 2, 3)).GetNormalized(), pl.Angle.DegreeToRadian(42));

            let mTrans = new pl.Mat4();
            mTrans.SetTransformationMatrix(mRot, new pl.Vec3(1, 2, 3));

            let t = new pl.Transform();
            t.SetFromMat4(mTrans);
            PLASMA_TEST.VEC3(t.position, new pl.Vec3(1, 2, 3), 0);
            PLASMA_TEST.BOOL(t.rotation.GetAsMat3().IsEqual(mRot, 0.001));
        }
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "TestTransform") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

