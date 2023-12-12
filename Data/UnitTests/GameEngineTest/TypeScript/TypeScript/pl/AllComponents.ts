
import __GameObject = require("TypeScript/pl/GameObject")
export import GameObject = __GameObject.GameObject;

import __Component = require("TypeScript/pl/Component")
export import Component = __Component.Component;

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

declare function __CPP_ComponentProperty_get(component: Component, id: number);
declare function __CPP_ComponentProperty_set(component: Component, id: number, value);
declare function __CPP_ComponentFunction_Call(component: Component, id: number, ...args);

export class AgentSteeringComponent extends Component
{
  public static GetTypeNameHash(): number { return 1760109148; }
  SetTargetPosition(position: Vec3): void { __CPP_ComponentFunction_Call(this, 207091595, position); }
  GetTargetPosition(): Vec3 { return __CPP_ComponentFunction_Call(this, 1192685398); }
  ClearTargetPosition(): void { __CPP_ComponentFunction_Call(this, 1185079771); }
}

export class RenderComponent extends Component
{
  public static GetTypeNameHash(): number { return 4102819859; }
}

export class AlwaysVisibleComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1430258478; }
}

export class SettingsComponent extends Component
{
  public static GetTypeNameHash(): number { return 952101475; }
}

export class AmbientLightComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 950297262; }
  get TopColor(): Color { return __CPP_ComponentProperty_get(this, 2024661606); }
  set TopColor(value: Color) { __CPP_ComponentProperty_set(this, 2024661606, value); }
  get BottomColor(): Color { return __CPP_ComponentProperty_get(this, 615468062); }
  set BottomColor(value: Color) { __CPP_ComponentProperty_set(this, 615468062, value); }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 4234105127); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 4234105127, value); }
}

export class MeshComponentBase extends RenderComponent
{
  public static GetTypeNameHash(): number { return 45766529; }
}

export class AnimatedMeshComponent extends MeshComponentBase
{
  public static GetTypeNameHash(): number { return 2035939403; }
  get Mesh(): string { return __CPP_ComponentProperty_get(this, 305497497); }
  set Mesh(value: string) { __CPP_ComponentProperty_set(this, 305497497, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2211164393); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2211164393, value); }
}

export class AnimationControllerComponent extends Component
{
  public static GetTypeNameHash(): number { return 2388391069; }
  get AnimController(): string { return __CPP_ComponentProperty_get(this, 2069666162); }
  set AnimController(value: string) { __CPP_ComponentProperty_set(this, 2069666162, value); }
  get RootMotionMode(): Enum.RootMotionMode { return __CPP_ComponentProperty_get(this, 797060997); }
  set RootMotionMode(value: Enum.RootMotionMode) { __CPP_ComponentProperty_set(this, 797060997, value); }
}

export class AreaDamageComponent extends Component
{
  public static GetTypeNameHash(): number { return 3469517624; }
  ApplyAreaDamage(): void { __CPP_ComponentFunction_Call(this, 1349203366); }
  get OnCreation(): boolean { return __CPP_ComponentProperty_get(this, 2841899360); }
  set OnCreation(value: boolean) { __CPP_ComponentProperty_set(this, 2841899360, value); }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 1171824972); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 1171824972, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 2446230746); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 2446230746, value); }
  get Damage(): number { return __CPP_ComponentProperty_get(this, 1094984346); }
  set Damage(value: number) { __CPP_ComponentProperty_set(this, 1094984346, value); }
  get Impulse(): number { return __CPP_ComponentProperty_get(this, 3118094972); }
  set Impulse(value: number) { __CPP_ComponentProperty_set(this, 3118094972, value); }
}

export class BakedProbesComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 3057819814; }
  get ShowDebugOverlay(): boolean { return __CPP_ComponentProperty_get(this, 1952385133); }
  set ShowDebugOverlay(value: boolean) { __CPP_ComponentProperty_set(this, 1952385133, value); }
  get ShowDebugProbes(): boolean { return __CPP_ComponentProperty_get(this, 3008089000); }
  set ShowDebugProbes(value: boolean) { __CPP_ComponentProperty_set(this, 3008089000, value); }
  get UseTestPosition(): boolean { return __CPP_ComponentProperty_get(this, 2703028463); }
  set UseTestPosition(value: boolean) { __CPP_ComponentProperty_set(this, 2703028463, value); }
  get TestPosition(): Vec3 { return __CPP_ComponentProperty_get(this, 1871860840); }
  set TestPosition(value: Vec3) { __CPP_ComponentProperty_set(this, 1871860840, value); }
}

export class BakedProbesVolumeComponent extends Component
{
  public static GetTypeNameHash(): number { return 2327212763; }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 1847089880); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 1847089880, value); }
}

export class BeamComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2143923328; }
  get TargetObject(): string { return __CPP_ComponentProperty_get(this, 1381946854); }
  set TargetObject(value: string) { __CPP_ComponentProperty_set(this, 1381946854, value); }
  get Material(): string { return __CPP_ComponentProperty_get(this, 3104469233); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 3104469233, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 3635095235); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 3635095235, value); }
  get Width(): number { return __CPP_ComponentProperty_get(this, 1738078974); }
  set Width(value: number) { __CPP_ComponentProperty_set(this, 1738078974, value); }
  get UVUnitsPerWorldUnit(): number { return __CPP_ComponentProperty_get(this, 222396887); }
  set UVUnitsPerWorldUnit(value: number) { __CPP_ComponentProperty_set(this, 222396887, value); }
}

export class BlackboardComponent extends Component
{
  public static GetTypeNameHash(): number { return 1131420421; }
  SetEntryValue(Name: string, Value: any): void { __CPP_ComponentFunction_Call(this, 1773908207, Name, Value); }
  GetEntryValue(Name: string): any { return __CPP_ComponentFunction_Call(this, 1449224516, Name); }
  get BlackboardName(): string { return __CPP_ComponentProperty_get(this, 544846061); }
  set BlackboardName(value: string) { __CPP_ComponentProperty_set(this, 544846061, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 406526337); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 406526337, value); }
  get SendEntryChangedMessage(): boolean { return __CPP_ComponentProperty_get(this, 3078711978); }
  set SendEntryChangedMessage(value: boolean) { __CPP_ComponentProperty_set(this, 3078711978, value); }
}

export class ReflectionProbeComponentBase extends Component
{
  public static GetTypeNameHash(): number { return 3519739115; }
  get ReflectionProbeMode(): Enum.ReflectionProbeMode { return __CPP_ComponentProperty_get(this, 2737642732); }
  set ReflectionProbeMode(value: Enum.ReflectionProbeMode) { __CPP_ComponentProperty_set(this, 2737642732, value); }
  get NearPlane(): number { return __CPP_ComponentProperty_get(this, 1536154975); }
  set NearPlane(value: number) { __CPP_ComponentProperty_set(this, 1536154975, value); }
  get FarPlane(): number { return __CPP_ComponentProperty_get(this, 81302529); }
  set FarPlane(value: number) { __CPP_ComponentProperty_set(this, 81302529, value); }
  get CaptureOffset(): Vec3 { return __CPP_ComponentProperty_get(this, 3183784557); }
  set CaptureOffset(value: Vec3) { __CPP_ComponentProperty_set(this, 3183784557, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 203876419); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 203876419, value); }
  get ShowMipMaps(): boolean { return __CPP_ComponentProperty_get(this, 2751375322); }
  set ShowMipMaps(value: boolean) { __CPP_ComponentProperty_set(this, 2751375322, value); }
}

export class BoxReflectionProbeComponent extends ReflectionProbeComponentBase
{
  public static GetTypeNameHash(): number { return 38280947; }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 2713695784); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 2713695784, value); }
  get InfluenceScale(): Vec3 { return __CPP_ComponentProperty_get(this, 98219906); }
  set InfluenceScale(value: Vec3) { __CPP_ComponentProperty_set(this, 98219906, value); }
  get InfluenceShift(): Vec3 { return __CPP_ComponentProperty_get(this, 2699654273); }
  set InfluenceShift(value: Vec3) { __CPP_ComponentProperty_set(this, 2699654273, value); }
  get PositiveFalloff(): Vec3 { return __CPP_ComponentProperty_get(this, 1299096840); }
  set PositiveFalloff(value: Vec3) { __CPP_ComponentProperty_set(this, 1299096840, value); }
  get NegativeFalloff(): Vec3 { return __CPP_ComponentProperty_get(this, 4019536499); }
  set NegativeFalloff(value: Vec3) { __CPP_ComponentProperty_set(this, 4019536499, value); }
  get BoxProjection(): boolean { return __CPP_ComponentProperty_get(this, 2185400274); }
  set BoxProjection(value: boolean) { __CPP_ComponentProperty_set(this, 2185400274, value); }
}

export class CameraComponent extends Component
{
  public static GetTypeNameHash(): number { return 4046503783; }
  get EditorShortcut(): number { return __CPP_ComponentProperty_get(this, 3960258057); }
  set EditorShortcut(value: number) { __CPP_ComponentProperty_set(this, 3960258057, value); }
  get UsageHint(): Enum.CameraUsageHint { return __CPP_ComponentProperty_get(this, 3788405542); }
  set UsageHint(value: Enum.CameraUsageHint) { __CPP_ComponentProperty_set(this, 3788405542, value); }
  get Mode(): Enum.CameraMode { return __CPP_ComponentProperty_get(this, 1920516257); }
  set Mode(value: Enum.CameraMode) { __CPP_ComponentProperty_set(this, 1920516257, value); }
  get RenderTarget(): string { return __CPP_ComponentProperty_get(this, 1507099785); }
  set RenderTarget(value: string) { __CPP_ComponentProperty_set(this, 1507099785, value); }
  get RenderTargetOffset(): Vec2 { return __CPP_ComponentProperty_get(this, 993322431); }
  set RenderTargetOffset(value: Vec2) { __CPP_ComponentProperty_set(this, 993322431, value); }
  get RenderTargetSize(): Vec2 { return __CPP_ComponentProperty_get(this, 318402718); }
  set RenderTargetSize(value: Vec2) { __CPP_ComponentProperty_set(this, 318402718, value); }
  get NearPlane(): number { return __CPP_ComponentProperty_get(this, 3278053398); }
  set NearPlane(value: number) { __CPP_ComponentProperty_set(this, 3278053398, value); }
  get FarPlane(): number { return __CPP_ComponentProperty_get(this, 2314827136); }
  set FarPlane(value: number) { __CPP_ComponentProperty_set(this, 2314827136, value); }
  get FOV(): number { return __CPP_ComponentProperty_get(this, 914870516); }
  set FOV(value: number) { __CPP_ComponentProperty_set(this, 914870516, value); }
  get Dimensions(): number { return __CPP_ComponentProperty_get(this, 3637093599); }
  set Dimensions(value: number) { __CPP_ComponentProperty_set(this, 3637093599, value); }
  get CameraRenderPipeline(): string { return __CPP_ComponentProperty_get(this, 2312764063); }
  set CameraRenderPipeline(value: string) { __CPP_ComponentProperty_set(this, 2312764063, value); }
  get Aperture(): number { return __CPP_ComponentProperty_get(this, 388500517); }
  set Aperture(value: number) { __CPP_ComponentProperty_set(this, 388500517, value); }
  get ShutterTime(): number { return __CPP_ComponentProperty_get(this, 4038354758); }
  set ShutterTime(value: number) { __CPP_ComponentProperty_set(this, 4038354758, value); }
  get ISO(): number { return __CPP_ComponentProperty_get(this, 3157925242); }
  set ISO(value: number) { __CPP_ComponentProperty_set(this, 3157925242, value); }
  get ExposureCompensation(): number { return __CPP_ComponentProperty_get(this, 3116794361); }
  set ExposureCompensation(value: number) { __CPP_ComponentProperty_set(this, 3116794361, value); }
  get ShowStats(): boolean { return __CPP_ComponentProperty_get(this, 563278316); }
  set ShowStats(value: boolean) { __CPP_ComponentProperty_set(this, 563278316, value); }
}

export class CharacterControllerComponent extends Component
{
  public static GetTypeNameHash(): number { return 3763005338; }
  RawMove(moveDeltaGlobal: Vec3): void { __CPP_ComponentFunction_Call(this, 1101430293, moveDeltaGlobal); }
  TeleportCharacter(globalFootPosition: Vec3): void { __CPP_ComponentFunction_Call(this, 3818653292, globalFootPosition); }
  IsDestinationUnobstructed(globalFootPosition: Vec3, characterHeight: number): boolean { return __CPP_ComponentFunction_Call(this, 969129472, globalFootPosition, characterHeight); }
  IsTouchingGround(): boolean { return __CPP_ComponentFunction_Call(this, 3488774158); }
  IsCrouching(): boolean { return __CPP_ComponentFunction_Call(this, 2133672375); }
}

export class ClothSheetComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3013623916; }
  get Size(): Vec2 { return __CPP_ComponentProperty_get(this, 1518201195); }
  set Size(value: Vec2) { __CPP_ComponentProperty_set(this, 1518201195, value); }
  get Slack(): Vec2 { return __CPP_ComponentProperty_get(this, 353855628); }
  set Slack(value: Vec2) { __CPP_ComponentProperty_set(this, 353855628, value); }
  get Segments(): Vec2 { return __CPP_ComponentProperty_get(this, 3952598611); }
  set Segments(value: Vec2) { __CPP_ComponentProperty_set(this, 3952598611, value); }
  get Damping(): number { return __CPP_ComponentProperty_get(this, 715596432); }
  set Damping(value: number) { __CPP_ComponentProperty_set(this, 715596432, value); }
  get WindInfluence(): number { return __CPP_ComponentProperty_get(this, 1666716890); }
  set WindInfluence(value: number) { __CPP_ComponentProperty_set(this, 1666716890, value); }
  get Flags(): Flags.ClothSheetFlags { return __CPP_ComponentProperty_get(this, 3217398433); }
  set Flags(value: Flags.ClothSheetFlags) { __CPP_ComponentProperty_set(this, 3217398433, value); }
  get Material(): string { return __CPP_ComponentProperty_get(this, 33830499); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 33830499, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 1675282813); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 1675282813, value); }
}

export class CollectionComponent extends Component
{
  public static GetTypeNameHash(): number { return 3548913799; }
  get Collection(): string { return __CPP_ComponentProperty_get(this, 2676299555); }
  set Collection(value: string) { __CPP_ComponentProperty_set(this, 2676299555, value); }
}

export class ColorAnimationComponent extends Component
{
  public static GetTypeNameHash(): number { return 1426095993; }
  get Gradient(): string { return __CPP_ComponentProperty_get(this, 2209803606); }
  set Gradient(value: string) { __CPP_ComponentProperty_set(this, 2209803606, value); }
  get Duration(): number { return __CPP_ComponentProperty_get(this, 2947376301); }
  set Duration(value: number) { __CPP_ComponentProperty_set(this, 2947376301, value); }
  get SetColorMode(): Enum.SetColorMode { return __CPP_ComponentProperty_get(this, 698882720); }
  set SetColorMode(value: Enum.SetColorMode) { __CPP_ComponentProperty_set(this, 698882720, value); }
  get AnimationMode(): Enum.PropertyAnimMode { return __CPP_ComponentProperty_get(this, 2911480779); }
  set AnimationMode(value: Enum.PropertyAnimMode) { __CPP_ComponentProperty_set(this, 2911480779, value); }
  get RandomStartOffset(): boolean { return __CPP_ComponentProperty_get(this, 185138321); }
  set RandomStartOffset(value: boolean) { __CPP_ComponentProperty_set(this, 185138321, value); }
  get ApplyToChildren(): boolean { return __CPP_ComponentProperty_get(this, 3762785755); }
  set ApplyToChildren(value: boolean) { __CPP_ComponentProperty_set(this, 3762785755, value); }
}

export class CustomMeshComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 469752896; }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 28444812); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 28444812, value); }
  get Material(): string { return __CPP_ComponentProperty_get(this, 955545296); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 955545296, value); }
}

export class DebugTextComponent extends Component
{
  public static GetTypeNameHash(): number { return 3018591528; }
  get Text(): string { return __CPP_ComponentProperty_get(this, 4157926666); }
  set Text(value: string) { __CPP_ComponentProperty_set(this, 4157926666, value); }
  get Value0(): number { return __CPP_ComponentProperty_get(this, 1323847655); }
  set Value0(value: number) { __CPP_ComponentProperty_set(this, 1323847655, value); }
  get Value1(): number { return __CPP_ComponentProperty_get(this, 3942076706); }
  set Value1(value: number) { __CPP_ComponentProperty_set(this, 3942076706, value); }
  get Value2(): number { return __CPP_ComponentProperty_get(this, 192083144); }
  set Value2(value: number) { __CPP_ComponentProperty_set(this, 192083144, value); }
  get Value3(): number { return __CPP_ComponentProperty_get(this, 1945586141); }
  set Value3(value: number) { __CPP_ComponentProperty_set(this, 1945586141, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 1094018476); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 1094018476, value); }
}

export class DecalComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 275487952; }
  get ProjectionAxis(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 1548951079); }
  set ProjectionAxis(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 1548951079, value); }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 3390006638); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 3390006638, value); }
  get SizeVariance(): number { return __CPP_ComponentProperty_get(this, 1660552640); }
  set SizeVariance(value: number) { __CPP_ComponentProperty_set(this, 1660552640, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 694547174); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 694547174, value); }
  get EmissiveColor(): Color { return __CPP_ComponentProperty_get(this, 1513606000); }
  set EmissiveColor(value: Color) { __CPP_ComponentProperty_set(this, 1513606000, value); }
  get SortOrder(): number { return __CPP_ComponentProperty_get(this, 383719798); }
  set SortOrder(value: number) { __CPP_ComponentProperty_set(this, 383719798, value); }
  get WrapAround(): boolean { return __CPP_ComponentProperty_get(this, 3186321325); }
  set WrapAround(value: boolean) { __CPP_ComponentProperty_set(this, 3186321325, value); }
  get MapNormalToGeometry(): boolean { return __CPP_ComponentProperty_get(this, 2713945030); }
  set MapNormalToGeometry(value: boolean) { __CPP_ComponentProperty_set(this, 2713945030, value); }
  get InnerFadeAngle(): number { return __CPP_ComponentProperty_get(this, 2551098307); }
  set InnerFadeAngle(value: number) { __CPP_ComponentProperty_set(this, 2551098307, value); }
  get OuterFadeAngle(): number { return __CPP_ComponentProperty_get(this, 4173405358); }
  set OuterFadeAngle(value: number) { __CPP_ComponentProperty_set(this, 4173405358, value); }
  get FadeOutDuration(): number { return __CPP_ComponentProperty_get(this, 1160090686); }
  set FadeOutDuration(value: number) { __CPP_ComponentProperty_set(this, 1160090686, value); }
  get OnFinishedAction(): Enum.OnComponentFinishedAction { return __CPP_ComponentProperty_get(this, 1376001981); }
  set OnFinishedAction(value: Enum.OnComponentFinishedAction) { __CPP_ComponentProperty_set(this, 1376001981, value); }
  get ApplyToDynamic(): string { return __CPP_ComponentProperty_get(this, 3873730861); }
  set ApplyToDynamic(value: string) { __CPP_ComponentProperty_set(this, 3873730861, value); }
}

export class DeviceTrackingComponent extends Component
{
  public static GetTypeNameHash(): number { return 2620100168; }
  get DeviceType(): Enum.XRDeviceType { return __CPP_ComponentProperty_get(this, 214273525); }
  set DeviceType(value: Enum.XRDeviceType) { __CPP_ComponentProperty_set(this, 214273525, value); }
  get PoseLocation(): Enum.XRPoseLocation { return __CPP_ComponentProperty_get(this, 3731095080); }
  set PoseLocation(value: Enum.XRPoseLocation) { __CPP_ComponentProperty_set(this, 3731095080, value); }
  get TransformSpace(): Enum.XRTransformSpace { return __CPP_ComponentProperty_get(this, 882080304); }
  set TransformSpace(value: Enum.XRTransformSpace) { __CPP_ComponentProperty_set(this, 882080304, value); }
  get Rotation(): boolean { return __CPP_ComponentProperty_get(this, 3591711394); }
  set Rotation(value: boolean) { __CPP_ComponentProperty_set(this, 3591711394, value); }
  get Scale(): boolean { return __CPP_ComponentProperty_get(this, 948171275); }
  set Scale(value: boolean) { __CPP_ComponentProperty_set(this, 948171275, value); }
}

export class LightComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3454614024; }
  get LightColor(): Color { return __CPP_ComponentProperty_get(this, 1251321172); }
  set LightColor(value: Color) { __CPP_ComponentProperty_set(this, 1251321172, value); }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 2197920774); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 2197920774, value); }
  get CastShadows(): boolean { return __CPP_ComponentProperty_get(this, 501488984); }
  set CastShadows(value: boolean) { __CPP_ComponentProperty_set(this, 501488984, value); }
  get PenumbraSize(): number { return __CPP_ComponentProperty_get(this, 4173820343); }
  set PenumbraSize(value: number) { __CPP_ComponentProperty_set(this, 4173820343, value); }
  get SlopeBias(): number { return __CPP_ComponentProperty_get(this, 2087382592); }
  set SlopeBias(value: number) { __CPP_ComponentProperty_set(this, 2087382592, value); }
  get ConstantBias(): number { return __CPP_ComponentProperty_get(this, 4183093458); }
  set ConstantBias(value: number) { __CPP_ComponentProperty_set(this, 4183093458, value); }
}

export class DirectionalLightComponent extends LightComponent
{
  public static GetTypeNameHash(): number { return 4266332463; }
  get NumCascades(): number { return __CPP_ComponentProperty_get(this, 414683411); }
  set NumCascades(value: number) { __CPP_ComponentProperty_set(this, 414683411, value); }
  get MinShadowRange(): number { return __CPP_ComponentProperty_get(this, 4077881475); }
  set MinShadowRange(value: number) { __CPP_ComponentProperty_set(this, 4077881475, value); }
  get FadeOutStart(): number { return __CPP_ComponentProperty_get(this, 2063982046); }
  set FadeOutStart(value: number) { __CPP_ComponentProperty_set(this, 2063982046, value); }
  get SplitModeWeight(): number { return __CPP_ComponentProperty_get(this, 3297285592); }
  set SplitModeWeight(value: number) { __CPP_ComponentProperty_set(this, 3297285592, value); }
  get NearPlaneOffset(): number { return __CPP_ComponentProperty_get(this, 3635477819); }
  set NearPlaneOffset(value: number) { __CPP_ComponentProperty_set(this, 3635477819, value); }
}

export class EventMessageHandlerComponent extends Component
{
  public static GetTypeNameHash(): number { return 43077958; }
  get HandleGlobalEvents(): boolean { return __CPP_ComponentProperty_get(this, 2364604685); }
  set HandleGlobalEvents(value: boolean) { __CPP_ComponentProperty_set(this, 2364604685, value); }
  get PassThroughUnhandledEvents(): boolean { return __CPP_ComponentProperty_get(this, 1956477909); }
  set PassThroughUnhandledEvents(value: boolean) { __CPP_ComponentProperty_set(this, 1956477909, value); }
}

export class FakeRopeComponent extends Component
{
  public static GetTypeNameHash(): number { return 32723491; }
  get Anchor(): string { return __CPP_ComponentProperty_get(this, 567704478); }
  set Anchor(value: string) { __CPP_ComponentProperty_set(this, 567704478, value); }
  get AttachToOrigin(): boolean { return __CPP_ComponentProperty_get(this, 325918694); }
  set AttachToOrigin(value: boolean) { __CPP_ComponentProperty_set(this, 325918694, value); }
  get AttachToAnchor(): boolean { return __CPP_ComponentProperty_get(this, 368438004); }
  set AttachToAnchor(value: boolean) { __CPP_ComponentProperty_set(this, 368438004, value); }
  get Pieces(): number { return __CPP_ComponentProperty_get(this, 2131253943); }
  set Pieces(value: number) { __CPP_ComponentProperty_set(this, 2131253943, value); }
  get Slack(): number { return __CPP_ComponentProperty_get(this, 1876105123); }
  set Slack(value: number) { __CPP_ComponentProperty_set(this, 1876105123, value); }
  get Damping(): number { return __CPP_ComponentProperty_get(this, 2507459163); }
  set Damping(value: number) { __CPP_ComponentProperty_set(this, 2507459163, value); }
  get WindInfluence(): number { return __CPP_ComponentProperty_get(this, 4191580362); }
  set WindInfluence(value: number) { __CPP_ComponentProperty_set(this, 4191580362, value); }
}

export class FogComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 1145128005; }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 1141075868); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 1141075868, value); }
  get Density(): number { return __CPP_ComponentProperty_get(this, 1526358461); }
  set Density(value: number) { __CPP_ComponentProperty_set(this, 1526358461, value); }
  get HeightFalloff(): number { return __CPP_ComponentProperty_get(this, 158453629); }
  set HeightFalloff(value: number) { __CPP_ComponentProperty_set(this, 158453629, value); }
  get ModulateWithSkyColor(): boolean { return __CPP_ComponentProperty_get(this, 3134075842); }
  set ModulateWithSkyColor(value: boolean) { __CPP_ComponentProperty_set(this, 3134075842, value); }
  get SkyDistance(): number { return __CPP_ComponentProperty_get(this, 3727326431); }
  set SkyDistance(value: number) { __CPP_ComponentProperty_set(this, 3727326431, value); }
}

export class MeshComponent extends MeshComponentBase
{
  public static GetTypeNameHash(): number { return 782315957; }
  get Mesh(): string { return __CPP_ComponentProperty_get(this, 1319310849); }
  set Mesh(value: string) { __CPP_ComponentProperty_set(this, 1319310849, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 1299346143); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 1299346143, value); }
}

export class GizmoComponent extends MeshComponent
{
  public static GetTypeNameHash(): number { return 50896064; }
}

export class GrabbableItemComponent extends Component
{
  public static GetTypeNameHash(): number { return 2122584786; }
}

export class GreyBoxComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3727394343; }
  get Shape(): Enum.GreyBoxShape { return __CPP_ComponentProperty_get(this, 3153073440); }
  set Shape(value: Enum.GreyBoxShape) { __CPP_ComponentProperty_set(this, 3153073440, value); }
  get Material(): string { return __CPP_ComponentProperty_get(this, 3423075555); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 3423075555, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2308891450); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2308891450, value); }
  get SizeNegX(): number { return __CPP_ComponentProperty_get(this, 1742140195); }
  set SizeNegX(value: number) { __CPP_ComponentProperty_set(this, 1742140195, value); }
  get SizePosX(): number { return __CPP_ComponentProperty_get(this, 4168795639); }
  set SizePosX(value: number) { __CPP_ComponentProperty_set(this, 4168795639, value); }
  get SizeNegY(): number { return __CPP_ComponentProperty_get(this, 875522100); }
  set SizeNegY(value: number) { __CPP_ComponentProperty_set(this, 875522100, value); }
  get SizePosY(): number { return __CPP_ComponentProperty_get(this, 741400747); }
  set SizePosY(value: number) { __CPP_ComponentProperty_set(this, 741400747, value); }
  get SizeNegZ(): number { return __CPP_ComponentProperty_get(this, 316763251); }
  set SizeNegZ(value: number) { __CPP_ComponentProperty_set(this, 316763251, value); }
  get SizePosZ(): number { return __CPP_ComponentProperty_get(this, 3457328243); }
  set SizePosZ(value: number) { __CPP_ComponentProperty_set(this, 3457328243, value); }
  get Detail(): number { return __CPP_ComponentProperty_get(this, 1291239827); }
  set Detail(value: number) { __CPP_ComponentProperty_set(this, 1291239827, value); }
  get Curvature(): number { return __CPP_ComponentProperty_get(this, 2327837840); }
  set Curvature(value: number) { __CPP_ComponentProperty_set(this, 2327837840, value); }
  get Thickness(): number { return __CPP_ComponentProperty_get(this, 3219114317); }
  set Thickness(value: number) { __CPP_ComponentProperty_set(this, 3219114317, value); }
  get SlopedTop(): boolean { return __CPP_ComponentProperty_get(this, 202665484); }
  set SlopedTop(value: boolean) { __CPP_ComponentProperty_set(this, 202665484, value); }
  get SlopedBottom(): boolean { return __CPP_ComponentProperty_get(this, 1054112644); }
  set SlopedBottom(value: boolean) { __CPP_ComponentProperty_set(this, 1054112644, value); }
  get GenerateCollision(): boolean { return __CPP_ComponentProperty_get(this, 2314781908); }
  set GenerateCollision(value: boolean) { __CPP_ComponentProperty_set(this, 2314781908, value); }
  get IncludeInNavmesh(): boolean { return __CPP_ComponentProperty_get(this, 3722132795); }
  set IncludeInNavmesh(value: boolean) { __CPP_ComponentProperty_set(this, 3722132795, value); }
}

export class HeadBoneComponent extends Component
{
  public static GetTypeNameHash(): number { return 3471497300; }
  SetVerticalRotation(Radians: number): void { __CPP_ComponentFunction_Call(this, 3114238805, Radians); }
  ChangeVerticalRotation(Radians: number): void { __CPP_ComponentFunction_Call(this, 2666870655, Radians); }
  get VerticalRotation(): number { return __CPP_ComponentProperty_get(this, 2504573224); }
  set VerticalRotation(value: number) { __CPP_ComponentProperty_set(this, 2504573224, value); }
}

export class HeightfieldComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1133312175; }
  get HeightfieldImage(): string { return __CPP_ComponentProperty_get(this, 457095988); }
  set HeightfieldImage(value: string) { __CPP_ComponentProperty_set(this, 457095988, value); }
  get Material(): string { return __CPP_ComponentProperty_get(this, 2692094696); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 2692094696, value); }
  get HalfExtents(): Vec2 { return __CPP_ComponentProperty_get(this, 767042533); }
  set HalfExtents(value: Vec2) { __CPP_ComponentProperty_set(this, 767042533, value); }
  get Height(): number { return __CPP_ComponentProperty_get(this, 2420329095); }
  set Height(value: number) { __CPP_ComponentProperty_set(this, 2420329095, value); }
  get Tesselation(): Vec2 { return __CPP_ComponentProperty_get(this, 3364409714); }
  set Tesselation(value: Vec2) { __CPP_ComponentProperty_set(this, 3364409714, value); }
  get TexCoordOffset(): Vec2 { return __CPP_ComponentProperty_get(this, 245041212); }
  set TexCoordOffset(value: Vec2) { __CPP_ComponentProperty_set(this, 245041212, value); }
  get TexCoordScale(): Vec2 { return __CPP_ComponentProperty_get(this, 1289808008); }
  set TexCoordScale(value: Vec2) { __CPP_ComponentProperty_set(this, 1289808008, value); }
  get GenerateCollision(): boolean { return __CPP_ComponentProperty_get(this, 277520990); }
  set GenerateCollision(value: boolean) { __CPP_ComponentProperty_set(this, 277520990, value); }
  get ColMeshTesselation(): Vec2 { return __CPP_ComponentProperty_get(this, 3057787631); }
  set ColMeshTesselation(value: Vec2) { __CPP_ComponentProperty_set(this, 3057787631, value); }
  get IncludeInNavmesh(): boolean { return __CPP_ComponentProperty_get(this, 2161031829); }
  set IncludeInNavmesh(value: boolean) { __CPP_ComponentProperty_set(this, 2161031829, value); }
}

export class InputComponent extends Component
{
  public static GetTypeNameHash(): number { return 1856772619; }
  GetCurrentInputState(InputAction: string, OnlyKeyPressed: boolean): number { return __CPP_ComponentFunction_Call(this, 1902634236, InputAction, OnlyKeyPressed); }
  get InputSet(): string { return __CPP_ComponentProperty_get(this, 1047775561); }
  set InputSet(value: string) { __CPP_ComponentProperty_set(this, 1047775561, value); }
  get Granularity(): Enum.InputMessageGranularity { return __CPP_ComponentProperty_get(this, 3880605719); }
  set Granularity(value: Enum.InputMessageGranularity) { __CPP_ComponentProperty_set(this, 3880605719, value); }
  get ForwardToBlackboard(): boolean { return __CPP_ComponentProperty_get(this, 2806723576); }
  set ForwardToBlackboard(value: boolean) { __CPP_ComponentProperty_set(this, 2806723576, value); }
}

export class InstancedMeshComponent extends MeshComponentBase
{
  public static GetTypeNameHash(): number { return 2637158932; }
  get Mesh(): string { return __CPP_ComponentProperty_get(this, 3269109557); }
  set Mesh(value: string) { __CPP_ComponentProperty_set(this, 3269109557, value); }
  get MainColor(): Color { return __CPP_ComponentProperty_get(this, 2930754); }
  set MainColor(value: Color) { __CPP_ComponentProperty_set(this, 2930754, value); }
}

export class JointAttachmentComponent extends Component
{
  public static GetTypeNameHash(): number { return 2522374239; }
  get JointName(): string { return __CPP_ComponentProperty_get(this, 2043889763); }
  set JointName(value: string) { __CPP_ComponentProperty_set(this, 2043889763, value); }
  get PositionOffset(): Vec3 { return __CPP_ComponentProperty_get(this, 3000018772); }
  set PositionOffset(value: Vec3) { __CPP_ComponentProperty_set(this, 3000018772, value); }
  get RotationOffset(): Quat { return __CPP_ComponentProperty_get(this, 284736989); }
  set RotationOffset(value: Quat) { __CPP_ComponentProperty_set(this, 284736989, value); }
}

export class JointOverrideComponent extends Component
{
  public static GetTypeNameHash(): number { return 2447168174; }
  get JointName(): string { return __CPP_ComponentProperty_get(this, 3852620157); }
  set JointName(value: string) { __CPP_ComponentProperty_set(this, 3852620157, value); }
  get OverridePosition(): boolean { return __CPP_ComponentProperty_get(this, 3412208747); }
  set OverridePosition(value: boolean) { __CPP_ComponentProperty_set(this, 3412208747, value); }
  get OverrideRotation(): boolean { return __CPP_ComponentProperty_get(this, 910358873); }
  set OverrideRotation(value: boolean) { __CPP_ComponentProperty_set(this, 910358873, value); }
  get OverrideScale(): boolean { return __CPP_ComponentProperty_get(this, 3891164273); }
  set OverrideScale(value: boolean) { __CPP_ComponentProperty_set(this, 3891164273, value); }
}

export class LineToComponent extends Component
{
  public static GetTypeNameHash(): number { return 1090183880; }
  get Target(): string { return __CPP_ComponentProperty_get(this, 2863038841); }
  set Target(value: string) { __CPP_ComponentProperty_set(this, 2863038841, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 3311204308); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 3311204308, value); }
}

export class MarkerComponent extends Component
{
  public static GetTypeNameHash(): number { return 2650534863; }
  get Marker(): string { return __CPP_ComponentProperty_get(this, 2079907588); }
  set Marker(value: string) { __CPP_ComponentProperty_set(this, 2079907588, value); }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 1115465707); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 1115465707, value); }
}

export class MoveToComponent extends Component
{
  public static GetTypeNameHash(): number { return 1975281948; }
  SetTargetPosition(position: Vec3): void { __CPP_ComponentFunction_Call(this, 1804816777, position); }
  get Running(): boolean { return __CPP_ComponentProperty_get(this, 1083423131); }
  set Running(value: boolean) { __CPP_ComponentProperty_set(this, 1083423131, value); }
  get TranslationSpeed(): number { return __CPP_ComponentProperty_get(this, 2322761256); }
  set TranslationSpeed(value: number) { __CPP_ComponentProperty_set(this, 2322761256, value); }
  get TranslationAcceleration(): number { return __CPP_ComponentProperty_get(this, 2363550603); }
  set TranslationAcceleration(value: number) { __CPP_ComponentProperty_set(this, 2363550603, value); }
  get TranslationDeceleration(): number { return __CPP_ComponentProperty_get(this, 1020377139); }
  set TranslationDeceleration(value: number) { __CPP_ComponentProperty_set(this, 1020377139, value); }
}

export class NpcComponent extends Component
{
  public static GetTypeNameHash(): number { return 2265739091; }
}

export class ayerStartPointComponent extends Component
{
  public static GetTypeNameHash(): number { return 259027510; }
  get PlayerPrefab(): string { return __CPP_ComponentProperty_get(this, 2472515884); }
  set PlayerPrefab(value: string) { __CPP_ComponentProperty_set(this, 2472515884, value); }
}

export class PointLightComponent extends LightComponent
{
  public static GetTypeNameHash(): number { return 95677977; }
  get Range(): number { return __CPP_ComponentProperty_get(this, 1256012399); }
  set Range(value: number) { __CPP_ComponentProperty_set(this, 1256012399, value); }
}

export class PrefabReferenceComponent extends Component
{
  public static GetTypeNameHash(): number { return 2176728124; }
  get Prefab(): string { return __CPP_ComponentProperty_get(this, 400275382); }
  set Prefab(value: string) { __CPP_ComponentProperty_set(this, 400275382, value); }
}

export class ProjectileComponent extends Component
{
  public static GetTypeNameHash(): number { return 2567892475; }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 966556212); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 966556212, value); }
  get GravityMultiplier(): number { return __CPP_ComponentProperty_get(this, 16679846); }
  set GravityMultiplier(value: number) { __CPP_ComponentProperty_set(this, 16679846, value); }
  get MaxLifetime(): number { return __CPP_ComponentProperty_get(this, 1366530349); }
  set MaxLifetime(value: number) { __CPP_ComponentProperty_set(this, 1366530349, value); }
  get OnTimeoutSpawn(): string { return __CPP_ComponentProperty_get(this, 1859887062); }
  set OnTimeoutSpawn(value: string) { __CPP_ComponentProperty_set(this, 1859887062, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 1242909342); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 1242909342, value); }
  get FallbackSurface(): string { return __CPP_ComponentProperty_get(this, 2895138433); }
  set FallbackSurface(value: string) { __CPP_ComponentProperty_set(this, 2895138433, value); }
}

export class PropertyAnimComponent extends Component
{
  public static GetTypeNameHash(): number { return 4131681850; }
  PlayAnimationRange(RangeLow: number, RangeHigh: number): void { __CPP_ComponentFunction_Call(this, 913237778, RangeLow, RangeHigh); }
  get Animation(): string { return __CPP_ComponentProperty_get(this, 3578172382); }
  set Animation(value: string) { __CPP_ComponentProperty_set(this, 3578172382, value); }
  get Playing(): boolean { return __CPP_ComponentProperty_get(this, 3751661210); }
  set Playing(value: boolean) { __CPP_ComponentProperty_set(this, 3751661210, value); }
  get Mode(): Enum.PropertyAnimMode { return __CPP_ComponentProperty_get(this, 3340823466); }
  set Mode(value: Enum.PropertyAnimMode) { __CPP_ComponentProperty_set(this, 3340823466, value); }
  get RandomOffset(): number { return __CPP_ComponentProperty_get(this, 2450635448); }
  set RandomOffset(value: number) { __CPP_ComponentProperty_set(this, 2450635448, value); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 1101350246); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 1101350246, value); }
  get RangeLow(): number { return __CPP_ComponentProperty_get(this, 1462270118); }
  set RangeLow(value: number) { __CPP_ComponentProperty_set(this, 1462270118, value); }
  get RangeHigh(): number { return __CPP_ComponentProperty_get(this, 1440054882); }
  set RangeHigh(value: number) { __CPP_ComponentProperty_set(this, 1440054882, value); }
}

export class RaycastComponent extends Component
{
  public static GetTypeNameHash(): number { return 711580813; }
  get MaxDistance(): number { return __CPP_ComponentProperty_get(this, 4148093919); }
  set MaxDistance(value: number) { __CPP_ComponentProperty_set(this, 4148093919, value); }
  get DisableTargetObjectOnNoHit(): boolean { return __CPP_ComponentProperty_get(this, 460273873); }
  set DisableTargetObjectOnNoHit(value: boolean) { __CPP_ComponentProperty_set(this, 460273873, value); }
  get RaycastEndObject(): string { return __CPP_ComponentProperty_get(this, 4153887862); }
  set RaycastEndObject(value: string) { __CPP_ComponentProperty_set(this, 4153887862, value); }
  get ForceTargetParentless(): boolean { return __CPP_ComponentProperty_get(this, 1261434851); }
  set ForceTargetParentless(value: boolean) { __CPP_ComponentProperty_set(this, 1261434851, value); }
  get ShapeTypesToHit(): Flags.PhysicsShapeType { return __CPP_ComponentProperty_get(this, 966868869); }
  set ShapeTypesToHit(value: Flags.PhysicsShapeType) { __CPP_ComponentProperty_set(this, 966868869, value); }
  get CollisionLayerEndPoint(): number { return __CPP_ComponentProperty_get(this, 3825519321); }
  set CollisionLayerEndPoint(value: number) { __CPP_ComponentProperty_set(this, 3825519321, value); }
  get CollisionLayerTrigger(): number { return __CPP_ComponentProperty_get(this, 2759641840); }
  set CollisionLayerTrigger(value: number) { __CPP_ComponentProperty_set(this, 2759641840, value); }
  get TriggerMessage(): string { return __CPP_ComponentProperty_get(this, 2517701215); }
  set TriggerMessage(value: string) { __CPP_ComponentProperty_set(this, 2517701215, value); }
}

export class RenderTargetActivatorComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 53148992; }
  get RenderTarget(): string { return __CPP_ComponentProperty_get(this, 2748523744); }
  set RenderTarget(value: string) { __CPP_ComponentProperty_set(this, 2748523744, value); }
}

export class RopeRenderComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3961735526; }
  get Material(): string { return __CPP_ComponentProperty_get(this, 1535502768); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 1535502768, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2389526470); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2389526470, value); }
  get Thickness(): number { return __CPP_ComponentProperty_get(this, 3712344599); }
  set Thickness(value: number) { __CPP_ComponentProperty_set(this, 3712344599, value); }
  get Detail(): number { return __CPP_ComponentProperty_get(this, 3877842974); }
  set Detail(value: number) { __CPP_ComponentProperty_set(this, 3877842974, value); }
  get Subdivide(): boolean { return __CPP_ComponentProperty_get(this, 771757885); }
  set Subdivide(value: boolean) { __CPP_ComponentProperty_set(this, 771757885, value); }
  get UScale(): number { return __CPP_ComponentProperty_get(this, 1194398193); }
  set UScale(value: number) { __CPP_ComponentProperty_set(this, 1194398193, value); }
}

export class TransformComponent extends Component
{
  public static GetTypeNameHash(): number { return 3118620549; }
  SetDirectionForwards(Forwards: boolean): void { __CPP_ComponentFunction_Call(this, 2618354834, Forwards); }
  IsDirectionForwards(): boolean { return __CPP_ComponentFunction_Call(this, 2620541444); }
  ToggleDirection(): void { __CPP_ComponentFunction_Call(this, 3729584804); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 144847305); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 144847305, value); }
  get Running(): boolean { return __CPP_ComponentProperty_get(this, 4011333514); }
  set Running(value: boolean) { __CPP_ComponentProperty_set(this, 4011333514, value); }
  get ReverseAtEnd(): boolean { return __CPP_ComponentProperty_get(this, 3058134379); }
  set ReverseAtEnd(value: boolean) { __CPP_ComponentProperty_set(this, 3058134379, value); }
  get ReverseAtStart(): boolean { return __CPP_ComponentProperty_get(this, 563633913); }
  set ReverseAtStart(value: boolean) { __CPP_ComponentProperty_set(this, 563633913, value); }
}

export class RotorComponent extends TransformComponent
{
  public static GetTypeNameHash(): number { return 289916853; }
  get Axis(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 1255108281); }
  set Axis(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 1255108281, value); }
  get AxisDeviation(): number { return __CPP_ComponentProperty_get(this, 1401324510); }
  set AxisDeviation(value: number) { __CPP_ComponentProperty_set(this, 1401324510, value); }
  get DegreesToRotate(): number { return __CPP_ComponentProperty_get(this, 4112424060); }
  set DegreesToRotate(value: number) { __CPP_ComponentProperty_set(this, 4112424060, value); }
  get Acceleration(): number { return __CPP_ComponentProperty_get(this, 2539851035); }
  set Acceleration(value: number) { __CPP_ComponentProperty_set(this, 2539851035, value); }
  get Deceleration(): number { return __CPP_ComponentProperty_get(this, 790447056); }
  set Deceleration(value: number) { __CPP_ComponentProperty_set(this, 790447056, value); }
}

export class SensorComponent extends Component
{
  public static GetTypeNameHash(): number { return 4058510066; }
  get UpdateRate(): Enum.UpdateRate { return __CPP_ComponentProperty_get(this, 2523312954); }
  set UpdateRate(value: Enum.UpdateRate) { __CPP_ComponentProperty_set(this, 2523312954, value); }
  get SpatialCategory(): string { return __CPP_ComponentProperty_get(this, 1355954376); }
  set SpatialCategory(value: string) { __CPP_ComponentProperty_set(this, 1355954376, value); }
  get TestVisibility(): boolean { return __CPP_ComponentProperty_get(this, 3801312974); }
  set TestVisibility(value: boolean) { __CPP_ComponentProperty_set(this, 3801312974, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 2871471389); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 2871471389, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 30297912); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 30297912, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2707646046); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2707646046, value); }
}

export class SensorConeComponent extends SensorComponent
{
  public static GetTypeNameHash(): number { return 618802408; }
  get NearDistance(): number { return __CPP_ComponentProperty_get(this, 255583002); }
  set NearDistance(value: number) { __CPP_ComponentProperty_set(this, 255583002, value); }
  get FarDistance(): number { return __CPP_ComponentProperty_get(this, 85992778); }
  set FarDistance(value: number) { __CPP_ComponentProperty_set(this, 85992778, value); }
  get Angle(): number { return __CPP_ComponentProperty_get(this, 2027165417); }
  set Angle(value: number) { __CPP_ComponentProperty_set(this, 2027165417, value); }
}

export class SensorCylinderComponent extends SensorComponent
{
  public static GetTypeNameHash(): number { return 4125144479; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 1179764110); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 1179764110, value); }
  get Height(): number { return __CPP_ComponentProperty_get(this, 3749186721); }
  set Height(value: number) { __CPP_ComponentProperty_set(this, 3749186721, value); }
}

export class SensorSphereComponent extends SensorComponent
{
  public static GetTypeNameHash(): number { return 3742944810; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 1068241895); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 1068241895, value); }
}

export class ShapeIconComponent extends Component
{
  public static GetTypeNameHash(): number { return 2531012202; }
}

export class SimpleAnimationComponent extends Component
{
  public static GetTypeNameHash(): number { return 650557677; }
  get AnimationClip(): string { return __CPP_ComponentProperty_get(this, 1794405718); }
  set AnimationClip(value: string) { __CPP_ComponentProperty_set(this, 1794405718, value); }
  get AnimationMode(): Enum.PropertyAnimMode { return __CPP_ComponentProperty_get(this, 2593438599); }
  set AnimationMode(value: Enum.PropertyAnimMode) { __CPP_ComponentProperty_set(this, 2593438599, value); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 3978870185); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 3978870185, value); }
  get RootMotionMode(): Enum.RootMotionMode { return __CPP_ComponentProperty_get(this, 2046126458); }
  set RootMotionMode(value: Enum.RootMotionMode) { __CPP_ComponentProperty_set(this, 2046126458, value); }
}

export class SimpleWindComponent extends Component
{
  public static GetTypeNameHash(): number { return 2485539505; }
  get MinWindStrength(): Enum.WindStrength { return __CPP_ComponentProperty_get(this, 4210957034); }
  set MinWindStrength(value: Enum.WindStrength) { __CPP_ComponentProperty_set(this, 4210957034, value); }
  get MaxWindStrength(): Enum.WindStrength { return __CPP_ComponentProperty_get(this, 1454391844); }
  set MaxWindStrength(value: Enum.WindStrength) { __CPP_ComponentProperty_set(this, 1454391844, value); }
  get MaxDeviation(): number { return __CPP_ComponentProperty_get(this, 1211731916); }
  set MaxDeviation(value: number) { __CPP_ComponentProperty_set(this, 1211731916, value); }
}

export class SkeletonComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3948584101; }
  get Skeleton(): string { return __CPP_ComponentProperty_get(this, 1630336020); }
  set Skeleton(value: string) { __CPP_ComponentProperty_set(this, 1630336020, value); }
  get VisualizeSkeleton(): boolean { return __CPP_ComponentProperty_get(this, 3747577168); }
  set VisualizeSkeleton(value: boolean) { __CPP_ComponentProperty_set(this, 3747577168, value); }
  get VisualizeColliders(): boolean { return __CPP_ComponentProperty_get(this, 1095768855); }
  set VisualizeColliders(value: boolean) { __CPP_ComponentProperty_set(this, 1095768855, value); }
  get VisualizeJoints(): boolean { return __CPP_ComponentProperty_get(this, 3742085399); }
  set VisualizeJoints(value: boolean) { __CPP_ComponentProperty_set(this, 3742085399, value); }
  get VisualizeSwingLimits(): boolean { return __CPP_ComponentProperty_get(this, 2244654782); }
  set VisualizeSwingLimits(value: boolean) { __CPP_ComponentProperty_set(this, 2244654782, value); }
  get VisualizeTwistLimits(): boolean { return __CPP_ComponentProperty_get(this, 2077809534); }
  set VisualizeTwistLimits(value: boolean) { __CPP_ComponentProperty_set(this, 2077809534, value); }
  get BonesToHighlight(): string { return __CPP_ComponentProperty_get(this, 2302522730); }
  set BonesToHighlight(value: string) { __CPP_ComponentProperty_set(this, 2302522730, value); }
}

export class SkeletonPoseComponent extends Component
{
  public static GetTypeNameHash(): number { return 3152514708; }
  get Skeleton(): string { return __CPP_ComponentProperty_get(this, 2753693555); }
  set Skeleton(value: string) { __CPP_ComponentProperty_set(this, 2753693555, value); }
  get Mode(): Enum.SkeletonPoseMode { return __CPP_ComponentProperty_get(this, 1027491165); }
  set Mode(value: Enum.SkeletonPoseMode) { __CPP_ComponentProperty_set(this, 1027491165, value); }
  get EditBones(): number { return __CPP_ComponentProperty_get(this, 523131054); }
  set EditBones(value: number) { __CPP_ComponentProperty_set(this, 523131054, value); }
}

export class SkyBoxComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2634996648; }
  get CubeMap(): string { return __CPP_ComponentProperty_get(this, 3047930784); }
  set CubeMap(value: string) { __CPP_ComponentProperty_set(this, 3047930784, value); }
  get ExposureBias(): number { return __CPP_ComponentProperty_get(this, 343442963); }
  set ExposureBias(value: number) { __CPP_ComponentProperty_set(this, 343442963, value); }
  get InverseTonemap(): boolean { return __CPP_ComponentProperty_get(this, 2867938877); }
  set InverseTonemap(value: boolean) { __CPP_ComponentProperty_set(this, 2867938877, value); }
  get UseFog(): boolean { return __CPP_ComponentProperty_get(this, 1669626952); }
  set UseFog(value: boolean) { __CPP_ComponentProperty_set(this, 1669626952, value); }
  get VirtualDistance(): number { return __CPP_ComponentProperty_get(this, 4069625675); }
  set VirtualDistance(value: number) { __CPP_ComponentProperty_set(this, 4069625675, value); }
}

export class SkyLightComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 4218232286; }
  get ReflectionProbeMode(): Enum.ReflectionProbeMode { return __CPP_ComponentProperty_get(this, 993469209); }
  set ReflectionProbeMode(value: Enum.ReflectionProbeMode) { __CPP_ComponentProperty_set(this, 993469209, value); }
  get CubeMap(): string { return __CPP_ComponentProperty_get(this, 4108510940); }
  set CubeMap(value: string) { __CPP_ComponentProperty_set(this, 4108510940, value); }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 27742328); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 27742328, value); }
  get Saturation(): number { return __CPP_ComponentProperty_get(this, 135871345); }
  set Saturation(value: number) { __CPP_ComponentProperty_set(this, 135871345, value); }
  get NearPlane(): number { return __CPP_ComponentProperty_get(this, 3313454553); }
  set NearPlane(value: number) { __CPP_ComponentProperty_set(this, 3313454553, value); }
  get FarPlane(): number { return __CPP_ComponentProperty_get(this, 2768994575); }
  set FarPlane(value: number) { __CPP_ComponentProperty_set(this, 2768994575, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 1288426292); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 1288426292, value); }
  get ShowMipMaps(): boolean { return __CPP_ComponentProperty_get(this, 3754700636); }
  set ShowMipMaps(value: boolean) { __CPP_ComponentProperty_set(this, 3754700636, value); }
}

export class SliderComponent extends TransformComponent
{
  public static GetTypeNameHash(): number { return 2602944833; }
  get Axis(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 4131715947); }
  set Axis(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 4131715947, value); }
  get Distance(): number { return __CPP_ComponentProperty_get(this, 3369545050); }
  set Distance(value: number) { __CPP_ComponentProperty_set(this, 3369545050, value); }
  get Acceleration(): number { return __CPP_ComponentProperty_get(this, 1362758253); }
  set Acceleration(value: number) { __CPP_ComponentProperty_set(this, 1362758253, value); }
  get Deceleration(): number { return __CPP_ComponentProperty_get(this, 3473289455); }
  set Deceleration(value: number) { __CPP_ComponentProperty_set(this, 3473289455, value); }
  get RandomStart(): number { return __CPP_ComponentProperty_get(this, 3480257636); }
  set RandomStart(value: number) { __CPP_ComponentProperty_set(this, 3480257636, value); }
}

export class SpatialAnchorComponent extends Component
{
  public static GetTypeNameHash(): number { return 1728828119; }
}

export class SpawnComponent extends Component
{
  public static GetTypeNameHash(): number { return 221071071; }
  CanTriggerManualSpawn(): boolean { return __CPP_ComponentFunction_Call(this, 1830303161); }
  TriggerManualSpawn(IgnoreSpawnDelay: boolean, LocalOffset: Vec3): boolean { return __CPP_ComponentFunction_Call(this, 1930294762, IgnoreSpawnDelay, LocalOffset); }
  ScheduleSpawn(): void { __CPP_ComponentFunction_Call(this, 3712154994); }
  get Prefab(): string { return __CPP_ComponentProperty_get(this, 1480097937); }
  set Prefab(value: string) { __CPP_ComponentProperty_set(this, 1480097937, value); }
  get AttachAsChild(): boolean { return __CPP_ComponentProperty_get(this, 3451874519); }
  set AttachAsChild(value: boolean) { __CPP_ComponentProperty_set(this, 3451874519, value); }
  get SpawnAtStart(): boolean { return __CPP_ComponentProperty_get(this, 594655493); }
  set SpawnAtStart(value: boolean) { __CPP_ComponentProperty_set(this, 594655493, value); }
  get SpawnContinuously(): boolean { return __CPP_ComponentProperty_get(this, 3965480866); }
  set SpawnContinuously(value: boolean) { __CPP_ComponentProperty_set(this, 3965480866, value); }
  get MinDelay(): number { return __CPP_ComponentProperty_get(this, 3512339470); }
  set MinDelay(value: number) { __CPP_ComponentProperty_set(this, 3512339470, value); }
  get DelayRange(): number { return __CPP_ComponentProperty_get(this, 664557015); }
  set DelayRange(value: number) { __CPP_ComponentProperty_set(this, 664557015, value); }
  get Deviation(): number { return __CPP_ComponentProperty_get(this, 123174910); }
  set Deviation(value: number) { __CPP_ComponentProperty_set(this, 123174910, value); }
}

export class SphereReflectionProbeComponent extends ReflectionProbeComponentBase
{
  public static GetTypeNameHash(): number { return 1794895364; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 1794042387); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 1794042387, value); }
  get Falloff(): number { return __CPP_ComponentProperty_get(this, 651824816); }
  set Falloff(value: number) { __CPP_ComponentProperty_set(this, 651824816, value); }
  get SphereProjection(): boolean { return __CPP_ComponentProperty_get(this, 2985709809); }
  set SphereProjection(value: boolean) { __CPP_ComponentProperty_set(this, 2985709809, value); }
}

export class SpotLightComponent extends LightComponent
{
  public static GetTypeNameHash(): number { return 2271193492; }
  get Range(): number { return __CPP_ComponentProperty_get(this, 2960173997); }
  set Range(value: number) { __CPP_ComponentProperty_set(this, 2960173997, value); }
  get InnerSpotAngle(): number { return __CPP_ComponentProperty_get(this, 3077254186); }
  set InnerSpotAngle(value: number) { __CPP_ComponentProperty_set(this, 3077254186, value); }
  get OuterSpotAngle(): number { return __CPP_ComponentProperty_get(this, 3525778862); }
  set OuterSpotAngle(value: number) { __CPP_ComponentProperty_set(this, 3525778862, value); }
}

export class SpriteComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 670221457; }
  get Texture(): string { return __CPP_ComponentProperty_get(this, 2351263135); }
  set Texture(value: string) { __CPP_ComponentProperty_set(this, 2351263135, value); }
  get BlendMode(): Enum.SpriteBlendMode { return __CPP_ComponentProperty_get(this, 632408123); }
  set BlendMode(value: Enum.SpriteBlendMode) { __CPP_ComponentProperty_set(this, 632408123, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2657268785); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2657268785, value); }
  get Size(): number { return __CPP_ComponentProperty_get(this, 1870777452); }
  set Size(value: number) { __CPP_ComponentProperty_set(this, 1870777452, value); }
  get MaxScreenSize(): number { return __CPP_ComponentProperty_get(this, 938106363); }
  set MaxScreenSize(value: number) { __CPP_ComponentProperty_set(this, 938106363, value); }
  get AspectRatio(): number { return __CPP_ComponentProperty_get(this, 1017601789); }
  set AspectRatio(value: number) { __CPP_ComponentProperty_set(this, 1017601789, value); }
}

export class StageSpaceComponent extends Component
{
  public static GetTypeNameHash(): number { return 105353684; }
  get StageSpace(): Enum.XRStageSpace { return __CPP_ComponentProperty_get(this, 3376897096); }
  set StageSpace(value: Enum.XRStageSpace) { __CPP_ComponentProperty_set(this, 3376897096, value); }
}

export class StateMachineComponent extends Component
{
  public static GetTypeNameHash(): number { return 253670360; }
  SetState(Name: string): boolean { return __CPP_ComponentFunction_Call(this, 2747214728, Name); }
  get Resource(): string { return __CPP_ComponentProperty_get(this, 1015317134); }
  set Resource(value: string) { __CPP_ComponentProperty_set(this, 1015317134, value); }
  get InitialState(): string { return __CPP_ComponentProperty_get(this, 3300285383); }
  set InitialState(value: string) { __CPP_ComponentProperty_set(this, 3300285383, value); }
}

export class TimedDeathComponent extends Component
{
  public static GetTypeNameHash(): number { return 1821283035; }
  get MinDelay(): number { return __CPP_ComponentProperty_get(this, 811928530); }
  set MinDelay(value: number) { __CPP_ComponentProperty_set(this, 811928530, value); }
  get DelayRange(): number { return __CPP_ComponentProperty_get(this, 2131838574); }
  set DelayRange(value: number) { __CPP_ComponentProperty_set(this, 2131838574, value); }
  get TimeoutPrefab(): string { return __CPP_ComponentProperty_get(this, 1125782390); }
  set TimeoutPrefab(value: string) { __CPP_ComponentProperty_set(this, 1125782390, value); }
}

export class VisualScriptComponent extends EventMessageHandlerComponent
{
  public static GetTypeNameHash(): number { return 1371360638; }
  get Script(): string { return __CPP_ComponentProperty_get(this, 831844885); }
  set Script(value: string) { __CPP_ComponentProperty_set(this, 831844885, value); }
}

export class VisualizeHandComponent extends Component
{
  public static GetTypeNameHash(): number { return 2581909238; }
}

export class WindVolumeComponent extends Component
{
  public static GetTypeNameHash(): number { return 713708816; }
  get Strength(): Enum.WindStrength { return __CPP_ComponentProperty_get(this, 2563576701); }
  set Strength(value: Enum.WindStrength) { __CPP_ComponentProperty_set(this, 2563576701, value); }
  get ReverseDirection(): boolean { return __CPP_ComponentProperty_get(this, 3810310416); }
  set ReverseDirection(value: boolean) { __CPP_ComponentProperty_set(this, 3810310416, value); }
  get BurstDuration(): number { return __CPP_ComponentProperty_get(this, 3968416112); }
  set BurstDuration(value: number) { __CPP_ComponentProperty_set(this, 3968416112, value); }
  get OnFinishedAction(): Enum.OnComponentFinishedAction { return __CPP_ComponentProperty_get(this, 3287069013); }
  set OnFinishedAction(value: Enum.OnComponentFinishedAction) { __CPP_ComponentProperty_set(this, 3287069013, value); }
}

export class WindVolumeConeComponent extends WindVolumeComponent
{
  public static GetTypeNameHash(): number { return 2146166055; }
  get Angle(): number { return __CPP_ComponentProperty_get(this, 3324541592); }
  set Angle(value: number) { __CPP_ComponentProperty_set(this, 3324541592, value); }
  get Length(): number { return __CPP_ComponentProperty_get(this, 2148893484); }
  set Length(value: number) { __CPP_ComponentProperty_set(this, 2148893484, value); }
}

export class WindVolumeCylinderComponent extends WindVolumeComponent
{
  public static GetTypeNameHash(): number { return 1059717772; }
  get Length(): number { return __CPP_ComponentProperty_get(this, 3925928231); }
  set Length(value: number) { __CPP_ComponentProperty_set(this, 3925928231, value); }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 1580418292); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 1580418292, value); }
  get Mode(): Enum.WindVolumeCylinderMode { return __CPP_ComponentProperty_get(this, 1944528185); }
  set Mode(value: Enum.WindVolumeCylinderMode) { __CPP_ComponentProperty_set(this, 1944528185, value); }
}

export class WindVolumeSphereComponent extends WindVolumeComponent
{
  public static GetTypeNameHash(): number { return 1938089959; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 1542772929); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 1542772929, value); }
}

