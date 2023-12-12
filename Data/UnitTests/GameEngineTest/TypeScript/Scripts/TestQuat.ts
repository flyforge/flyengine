import pl = require("TypeScript/pl")
import PLASMA_TEST = require("./TestFramework")

export class TestQuat extends pl.TypescriptComponent {

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
            let q = new pl.Quat();
            PLASMA_TEST.FLOAT(q.x, 0, 0.001);
            PLASMA_TEST.FLOAT(q.y, 0, 0.001);
            PLASMA_TEST.FLOAT(q.z, 0, 0.001);
            PLASMA_TEST.FLOAT(q.w, 1, 0.001);
        }

        // Clone / Normalize
        {
            let q = new pl.Quat(1, 2, 3, 4);
            PLASMA_TEST.BOOL(q.IsIdentical(new pl.Quat(1, 2, 3, 4)));

            q.Normalize();
            PLASMA_TEST.QUAT(q.Clone(), q, 0.001);
        }

        // SetIdentity
        {
            let q = new pl.Quat(1, 2, 3, 4);
            q.SetIdentity();
            PLASMA_TEST.QUAT(q, pl.Quat.IdentityQuaternion(), 0.001);
        }


        // IdentityQuaternion
        {
            let q = pl.Quat.IdentityQuaternion();

            PLASMA_TEST.FLOAT(q.x, 0, 0.001);
            PLASMA_TEST.FLOAT(q.y, 0, 0.001);
            PLASMA_TEST.FLOAT(q.z, 0, 0.001);
            PLASMA_TEST.FLOAT(q.w, 1, 0.001);
        }

        // SetFromAxisAndAngle / RotateVec3
        {
            {
                let q = new pl.Quat();
                q.SetFromAxisAndAngle(new pl.Vec3(1, 0, 0), pl.Angle.DegreeToRadian(90));

                let v = new pl.Vec3(0, 1, 0);
                q.RotateVec3(v);
                PLASMA_TEST.VEC3(v, new pl.Vec3(0, 0, 1), 0.0001);

                let v2 = new pl.Vec3(0, 1, 0);
                q.InvRotateVec3(v2);
                PLASMA_TEST.VEC3(v2, new pl.Vec3(0, 0, -1), 0.0001);
            }

            {
                let q = new pl.Quat();
                q.SetFromAxisAndAngle(new pl.Vec3(0, 1, 0), pl.Angle.DegreeToRadian(90));

                let v = new pl.Vec3(1, 0, 0);
                q.RotateVec3(v);
                PLASMA_TEST.VEC3(v, new pl.Vec3(0, 0, -1), 0.0001);

                let v2 = new pl.Vec3(1, 0, 0);
                q.InvRotateVec3(v2);
                PLASMA_TEST.VEC3(v2, new pl.Vec3(0, 0, 1), 0.0001);
            }

            {
                let q = new pl.Quat();
                q.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(90));

                let v = new pl.Vec3(0, 1, 0);
                q.RotateVec3(v);
                PLASMA_TEST.VEC3(v, new pl.Vec3(-1, 0, 0), 0.0001);

                let v2 = new pl.Vec3(0, 1, 0);
                q.InvRotateVec3(v2);
                PLASMA_TEST.VEC3(v2, new pl.Vec3(1, 0, 0), 0.0001);
            }
        }

        // SetQuat
        {
            let q = new pl.Quat(1, 2, 3, 4);
            let q2 = new pl.Quat();
            q2.SetQuat(q);

            PLASMA_TEST.FLOAT(q2.x, 1, 0.001);
            PLASMA_TEST.FLOAT(q2.y, 2, 0.001);
            PLASMA_TEST.FLOAT(q2.z, 3, 0.001);
            PLASMA_TEST.FLOAT(q2.w, 4, 0.001);
        }

        // SetFromMat3
        {
            let m = new pl.Mat3();
            m.SetRotationMatrixZ(pl.Angle.DegreeToRadian(-90));

            let q1 = new pl.Quat();
            let q2 = new pl.Quat();
            let q3 = new pl.Quat();

            q1.SetFromMat3(m);
            q2.SetFromAxisAndAngle(new pl.Vec3(0, 0, -1), pl.Angle.DegreeToRadian(90));
            q3.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(-90));

            PLASMA_TEST.BOOL(q1.IsEqualRotation(q2, 0.001));
            PLASMA_TEST.BOOL(q1.IsEqualRotation(q3, 0.001));
        }

        // SetSlerp
        {
            let q1 = new pl.Quat();
            let q2 = new pl.Quat();
            let q3 = new pl.Quat();
            let qr = new pl.Quat();

            q1.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(45));
            q2.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(0));
            q3.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(90));

            qr.SetSlerp(q2, q3, 0.5);

            PLASMA_TEST.QUAT(q1, qr, 0.0001);
        }

        // GetRotationAxisAndAngle
        {
            let q1 = new pl.Quat();
            let q2 = new pl.Quat();
            let q3 = new pl.Quat();

            q1.SetShortestRotation(new pl.Vec3(0, 1, 0), new pl.Vec3(1, 0, 0));
            q2.SetFromAxisAndAngle(new pl.Vec3(0, 0, -1), pl.Angle.DegreeToRadian(90));
            q3.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(-90));

            let res = q1.GetRotationAxisAndAngle();
            PLASMA_TEST.VEC3(res.axis, new pl.Vec3(0, 0, -1), 0.001);
            PLASMA_TEST.FLOAT(pl.Angle.RadianToDegree(res.angleInRadian), 90, 0.001);

            res = q2.GetRotationAxisAndAngle();
            PLASMA_TEST.VEC3(res.axis, new pl.Vec3(0, 0, -1), 0.001);
            PLASMA_TEST.FLOAT(pl.Angle.RadianToDegree(res.angleInRadian), 90, 0.001);

            res = q3.GetRotationAxisAndAngle();
            PLASMA_TEST.VEC3(res.axis, new pl.Vec3(0, 0, -1), 0.001);
            PLASMA_TEST.FLOAT(pl.Angle.RadianToDegree(res.angleInRadian), 90, 0.001);

            res = pl.Quat.IdentityQuaternion().GetRotationAxisAndAngle();
            PLASMA_TEST.VEC3(res.axis, new pl.Vec3(1, 0, 0), 0.001);
            PLASMA_TEST.FLOAT(pl.Angle.RadianToDegree(res.angleInRadian), 0, 0.001);

            let otherIdentity = new pl.Quat(0, 0, 0, -1);
            res = otherIdentity.GetRotationAxisAndAngle();
            PLASMA_TEST.VEC3(res.axis, new pl.Vec3(1, 0, 0), 0.001);
            PLASMA_TEST.FLOAT(pl.Angle.RadianToDegree(res.angleInRadian), 360, 0.001);
        }

        // GetAsMat3
        {
            let q = new pl.Quat();
            q.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(90));

            let mr = new pl.Mat3();
            mr.SetRotationMatrixZ(pl.Angle.DegreeToRadian(90));

            let m = q.GetAsMat3();

            PLASMA_TEST.BOOL(mr.IsEqual(m, 0.001));
        }

        // GetAsMat4
        {
            let q = new pl.Quat();
            q.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(90));

            let mr = new pl.Mat4();
            mr.SetRotationMatrixZ(pl.Angle.DegreeToRadian(90));

            let m = q.GetAsMat4();

            PLASMA_TEST.BOOL(mr.IsEqual(m, 0.001));
        }

        // SetShortestRotation / IsEqualRotation
        {
            let q1 = new pl.Quat();
            let q2 = new pl.Quat();
            let q3 = new pl.Quat();

            q1.SetShortestRotation(new pl.Vec3(0, 1, 0), new pl.Vec3(1, 0, 0));
            q2.SetFromAxisAndAngle(new pl.Vec3(0, 0, -1), pl.Angle.DegreeToRadian(90));
            q3.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(-90));

            PLASMA_TEST.BOOL(q1.IsEqualRotation(q2, 0.001));
            PLASMA_TEST.BOOL(q1.IsEqualRotation(q3, 0.001));

            PLASMA_TEST.BOOL(pl.Quat.IdentityQuaternion().IsEqualRotation(pl.Quat.IdentityQuaternion(), 0.001));
            PLASMA_TEST.BOOL(pl.Quat.IdentityQuaternion().IsEqualRotation(new pl.Quat(0, 0, 0, -1), 0.001));
        }

        // SetConcatenatedRotations / ConcatenateRotations
        {
            let q1 = new pl.Quat();
            let q2 = new pl.Quat();
            let q3 = new pl.Quat();
            let qr = new pl.Quat();

            q1.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(60));
            q2.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(30));
            q3.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(90));

            qr.SetConcatenatedRotations(q1, q2);

            PLASMA_TEST.BOOL(qr.IsEqualRotation(q3, 0.0001));

            let qr2 = q1.Clone();
            qr2.ConcatenateRotations(q2);

            PLASMA_TEST.QUAT(qr, qr2, 0.001);
        }

        // IsIdentical
        {
            let q1 = new pl.Quat();
            let q2 = new pl.Quat();

            q1.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(60));
            q2.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(30));
            PLASMA_TEST.BOOL(!q1.IsIdentical(q2));

            q2.SetFromAxisAndAngle(new pl.Vec3(1, 0, 0), pl.Angle.DegreeToRadian(60));
            PLASMA_TEST.BOOL(!q1.IsIdentical(q2));

            q2.SetFromAxisAndAngle(new pl.Vec3(0, 0, 1), pl.Angle.DegreeToRadian(60));
            PLASMA_TEST.BOOL(q1.IsIdentical(q2));
        }

        // Negate / GetNegated
        {
            let q = new pl.Quat();
            q.SetFromAxisAndAngle(new pl.Vec3(1, 0, 0), pl.Angle.DegreeToRadian(90));

            let v = new pl.Vec3(0, 1, 0);
            q.RotateVec3(v);
            PLASMA_TEST.VEC3(v, new pl.Vec3(0, 0, 1), 0.0001);

            let n1 = q.GetNegated();
            let n2 = q.Clone();
            n2.Negate();

            PLASMA_TEST.QUAT(n1, n2, 0.001);

            let v2 = new pl.Vec3(0, 1, 0);
            n1.RotateVec3(v2);
            PLASMA_TEST.VEC3(v2, new pl.Vec3(0, 0, -1), 0.0001);
        }

        // SetFromEulerAngles / GetAsEulerAngles
        {
            let q = new pl.Quat();
            q.SetFromEulerAngles(pl.Angle.DegreeToRadian(90), 0, 0);

            let euler = q.GetAsEulerAngles();
            PLASMA_TEST.FLOAT(euler.roll, pl.Angle.DegreeToRadian(90), 0.001);
            PLASMA_TEST.FLOAT(euler.pitch, pl.Angle.DegreeToRadian(0), 0.001);
            PLASMA_TEST.FLOAT(euler.yaw, pl.Angle.DegreeToRadian(0), 0.001);

            q.SetFromEulerAngles(0, pl.Angle.DegreeToRadian(90), 0);
            euler = q.GetAsEulerAngles();
            PLASMA_TEST.FLOAT(euler.pitch, pl.Angle.DegreeToRadian(90), 0.001);

            // due to compilation differences, this the result for this computation can be very different (but equivalent)
            PLASMA_TEST.BOOL((pl.Utils.IsNumberEqual(euler.roll, pl.Angle.DegreeToRadian(180), 0.001) && 
                          pl.Utils.IsNumberEqual(euler.yaw, pl.Angle.DegreeToRadian(180), 0.001)) ||
                          (pl.Utils.IsNumberEqual(euler.roll, pl.Angle.DegreeToRadian(0), 0.001) && 
                          pl.Utils.IsNumberEqual(euler.yaw, pl.Angle.DegreeToRadian(0), 0.001)));


            q.SetFromEulerAngles(0, 0, pl.Angle.DegreeToRadian(90));
            euler = q.GetAsEulerAngles();
            PLASMA_TEST.FLOAT(euler.roll, pl.Angle.DegreeToRadian(0), 0.001);
            PLASMA_TEST.FLOAT(euler.pitch, pl.Angle.DegreeToRadian(0), 0.001);
            PLASMA_TEST.FLOAT(euler.yaw, pl.Angle.DegreeToRadian(90), 0.001);
        }
    }


    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "TestQuat") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

