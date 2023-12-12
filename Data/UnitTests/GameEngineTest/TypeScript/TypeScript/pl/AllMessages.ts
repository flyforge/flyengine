// AUTO-GENERATED FILE

import __Message = require("TypeScript/pl/Message")
export import Message = __Message.Message;
export import EventMessage = __Message.EventMessage;

import __Vec2 = require("TypeScript/pl/Vec2")
export import Vec2 = __Vec2.Vec2;

import __Vec3 = require("TypeScript/pl/Vec3")
export import Vec3 = __Vec3.Vec3;

import __Mat3 = require("TypeScript/pl/Mat3")
export import Mat3 = __Mat3.Mat3;

import __Mat4 = require("TypeScript/pl/Mat4")
export import Mat4 = __Mat4.Mat4;

import __Quat = require("TypeScript/pl/Quat")
export import Quat = __Quat.Quat;

import __Transform = require("TypeScript/pl/Transform")
export import Transform = __Transform.Transform;

import __Color = require("TypeScript/pl/Color")
export import Color = __Color.Color;

import __Time = require("TypeScript/pl/Time")
export import Time = __Time.Time;

import __Angle = require("TypeScript/pl/Angle")
export import Angle = __Angle.Angle;

import Enum = require("./AllEnums")
import Flags = require("./AllFlags")


export class MsgAnimationPosePreparing extends Message
{
  public static GetTypeNameHash(): number { return 2853027862; }
  constructor() { super(); this.TypeNameHash = 2853027862; }
}

export class MsgAnimationPoseProposal extends Message
{
  public static GetTypeNameHash(): number { return 2569954979; }
  constructor() { super(); this.TypeNameHash = 2569954979; }
}

export class MsgAnimationPoseUpdated extends Message
{
  public static GetTypeNameHash(): number { return 4076004400; }
  constructor() { super(); this.TypeNameHash = 4076004400; }
}

export class MsgAnimationReachedEnd extends EventMessage
{
  public static GetTypeNameHash(): number { return 3692884115; }
  constructor() { super(); this.TypeNameHash = 3692884115; }
}

export class MsgApplyRootMotion extends Message
{
  public static GetTypeNameHash(): number { return 1298584120; }
  constructor() { super(); this.TypeNameHash = 1298584120; }
  Translation: Vec3 = new Vec3(0, 0, 0);
  RotationX: number = 0;
  RotationY: number = 0;
  RotationZ: number = 0;
}

export class MsgBlackboardEntryChanged extends EventMessage
{
  public static GetTypeNameHash(): number { return 743007790; }
  constructor() { super(); this.TypeNameHash = 743007790; }
  Name: string;
  OldValue: any;
  NewValue: any;
}

export class MsgBuildStaticMesh extends Message
{
  public static GetTypeNameHash(): number { return 1997394818; }
  constructor() { super(); this.TypeNameHash = 1997394818; }
}

export class MsgChildrenChanged extends Message
{
  public static GetTypeNameHash(): number { return 1935467571; }
  constructor() { super(); this.TypeNameHash = 1935467571; }
}

export class MsgCollision extends Message
{
  public static GetTypeNameHash(): number { return 2023198976; }
  constructor() { super(); this.TypeNameHash = 2023198976; }
}

export class MsgComponentInternalTrigger extends Message
{
  public static GetTypeNameHash(): number { return 463509774; }
  constructor() { super(); this.TypeNameHash = 463509774; }
  Message: string;
}

export class MsgComponentsChanged extends Message
{
  public static GetTypeNameHash(): number { return 3930081123; }
  constructor() { super(); this.TypeNameHash = 3930081123; }
}

export class MsgDamage extends EventMessage
{
  public static GetTypeNameHash(): number { return 516815926; }
  constructor() { super(); this.TypeNameHash = 516815926; }
  Damage: number = 0;
  HitObjectName: string;
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  ImpactDirection: Vec3 = new Vec3(0, 0, 0);
}

export class MsgDeleteGameObject extends Message
{
  public static GetTypeNameHash(): number { return 215792238; }
  constructor() { super(); this.TypeNameHash = 215792238; }
}

export class MsgExtractGeometry extends Message
{
  public static GetTypeNameHash(): number { return 1608611944; }
  constructor() { super(); this.TypeNameHash = 1608611944; }
}

export class MsgExtractRenderData extends Message
{
  public static GetTypeNameHash(): number { return 333415303; }
  constructor() { super(); this.TypeNameHash = 333415303; }
}

export class MsgGenericEvent extends EventMessage
{
  public static GetTypeNameHash(): number { return 140092074; }
  constructor() { super(); this.TypeNameHash = 140092074; }
  Message: string;
  Value: any = 0;
}

export class MsgInputActionTriggered extends EventMessage
{
  public static GetTypeNameHash(): number { return 43781192; }
  constructor() { super(); this.TypeNameHash = 43781192; }
  InputAction: string;
  KeyPressValue: number = 0;
  TriggerState: Enum.TriggerState = 0;
}

export class MsgMoveCharacterController extends Message
{
  public static GetTypeNameHash(): number { return 710864978; }
  constructor() { super(); this.TypeNameHash = 710864978; }
  MoveForwards: number = 0;
  MoveBackwards: number = 0;
  StrafeLeft: number = 0;
  StrafeRight: number = 0;
  RotateLeft: number = 0;
  RotateRight: number = 0;
  Run: boolean = false;
  Jump: boolean = false;
  Crouch: boolean = false;
}

export class MsgOnlyApplyToObject extends Message
{
  public static GetTypeNameHash(): number { return 3486871595; }
  constructor() { super(); this.TypeNameHash = 3486871595; }
}

export class MsgPhysicsAddForce extends Message
{
  public static GetTypeNameHash(): number { return 669246455; }
  constructor() { super(); this.TypeNameHash = 669246455; }
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  Force: Vec3 = new Vec3(0, 0, 0);
}

export class MsgPhysicsAddImpulse extends Message
{
  public static GetTypeNameHash(): number { return 3030038864; }
  constructor() { super(); this.TypeNameHash = 3030038864; }
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  Impulse: Vec3 = new Vec3(0, 0, 0);
  ObjectFilterID: number = 0;
}

export class MsgPhysicsJointBroke extends EventMessage
{
  public static GetTypeNameHash(): number { return 3243743391; }
  constructor() { super(); this.TypeNameHash = 3243743391; }
}

export class MsgQueryAnimationSkeleton extends Message
{
  public static GetTypeNameHash(): number { return 3338051858; }
  constructor() { super(); this.TypeNameHash = 3338051858; }
}

export class MsgRetrieveBoneState extends Message
{
  public static GetTypeNameHash(): number { return 4162104982; }
  constructor() { super(); this.TypeNameHash = 4162104982; }
}

export class MsgRopePoseUpdated extends Message
{
  public static GetTypeNameHash(): number { return 1432429162; }
  constructor() { super(); this.TypeNameHash = 1432429162; }
}

export class MsgSensorDetectedObjectsChanged extends EventMessage
{
  public static GetTypeNameHash(): number { return 3585418991; }
  constructor() { super(); this.TypeNameHash = 3585418991; }
}

export class MsgSetColor extends Message
{
  public static GetTypeNameHash(): number { return 3403091999; }
  constructor() { super(); this.TypeNameHash = 3403091999; }
  Color: Color = new Color(1, 1, 1, 1);
  Mode: Enum.SetColorMode = 0;
}

export class MsgSetFloatParameter extends Message
{
  public static GetTypeNameHash(): number { return 3215095514; }
  constructor() { super(); this.TypeNameHash = 3215095514; }
  Name: string;
  Value: number = 0;
}

export class MsgSetMeshMaterial extends Message
{
  public static GetTypeNameHash(): number { return 2996999833; }
  constructor() { super(); this.TypeNameHash = 2996999833; }
  Material: string;
  MaterialSlot: number = 0;
}

export class MsgSetPlaying extends Message
{
  public static GetTypeNameHash(): number { return 3130379613; }
  constructor() { super(); this.TypeNameHash = 3130379613; }
  Play: boolean = true;
}

export class MsgStateMachineStateChanged extends EventMessage
{
  public static GetTypeNameHash(): number { return 2733108415; }
  constructor() { super(); this.TypeNameHash = 2733108415; }
  NewStateName: string;
}

export class MsgTransformChanged extends Message
{
  public static GetTypeNameHash(): number { return 3891532130; }
  constructor() { super(); this.TypeNameHash = 3891532130; }
}

export class MsgTriggerTriggered extends EventMessage
{
  public static GetTypeNameHash(): number { return 2234980389; }
  constructor() { super(); this.TypeNameHash = 2234980389; }
  Message: string;
  TriggerState: Enum.TriggerState = 0;
}

export class MsgTypeScriptMsgProxy extends Message
{
  public static GetTypeNameHash(): number { return 2272334386; }
  constructor() { super(); this.TypeNameHash = 2272334386; }
}

export class MsgUpdateLocalBounds extends Message
{
  public static GetTypeNameHash(): number { return 2210198742; }
  constructor() { super(); this.TypeNameHash = 2210198742; }
}

