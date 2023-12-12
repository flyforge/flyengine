#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/World/GameObject.h>
#include <EditorPluginScene/Dialogs/DeltaTransformDlg.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Math/Random.h>

plQtDeltaTransformDlg::Mode plQtDeltaTransformDlg::s_Mode = plQtDeltaTransformDlg::Mode::Translate;
plQtDeltaTransformDlg::Space plQtDeltaTransformDlg::s_Space = plQtDeltaTransformDlg::Space::World;
plVec3 plQtDeltaTransformDlg::s_vTranslate(0.0f);
plVec3 plQtDeltaTransformDlg::s_vTranslateDeviation(0.0f);

plVec3 plQtDeltaTransformDlg::s_vScale(1.0f);
plVec3 plQtDeltaTransformDlg::s_vScaleDeviation(1.0f);

float plQtDeltaTransformDlg::s_fUniformScale = 1.0f;
float plQtDeltaTransformDlg::s_fUniformScaleDeviation = 1.0f;

plVec3 plQtDeltaTransformDlg::s_vRotate(0.0f);
plVec3 plQtDeltaTransformDlg::s_vRotateRandom(180.0f);
plVec3 plQtDeltaTransformDlg::s_vRotateDeviation(180.0f);

float plQtDeltaTransformDlg::s_fNaturalDeviationZ = 10.0f;

plQtDeltaTransformDlg::plQtDeltaTransformDlg(QWidget* parent, plSceneDocument* pSceneDoc)
  : QDialog(parent)
{
  m_pSceneDocument = pSceneDoc;

  setupUi(this);

  {
    plQtScopedBlockSignals _1(ComboMode);

    ComboMode->clear();
    ComboMode->addItem("Translate");
    ComboMode->addItem("Translate (deviation)");
    ComboMode->addItem("Rotate X");
    ComboMode->addItem("Rotate X (random)");
    ComboMode->addItem("Rotate X (deviation)");
    ComboMode->addItem("Rotate Y");
    ComboMode->addItem("Rotate Y (random)");
    ComboMode->addItem("Rotate Y (deviation)");
    ComboMode->addItem("Rotate Z");
    ComboMode->addItem("Rotate Z (random)");
    ComboMode->addItem("Rotate Z (deviation)");
    ComboMode->addItem("Scale");
    ComboMode->addItem("Scale (deviation)");
    ComboMode->addItem("Uniform Scale");
    ComboMode->addItem("Uniform Scale (deviation)");
    ComboMode->addItem("Natural Deviation Z");
  }

  UpdateUI();
  ComboMode->setCurrentIndex(s_Mode);
  ComboSpace->setCurrentIndex(s_Space);

  ButtonUndo->setEnabled(m_uiActionsApplied > 0 && m_pSceneDocument->GetCommandHistory()->CanUndo());
}

void plQtDeltaTransformDlg::on_ComboMode_currentIndexChanged(int index)
{
  QueryUI();

  s_Mode = (Mode)index;

  UpdateUI();
}

void plQtDeltaTransformDlg::on_ComboSpace_currentIndexChanged(int index)
{
  s_Space = (Space)index;
}

void plQtDeltaTransformDlg::on_ButtonApply_clicked()
{
  plStringBuilder sAction;

  // early out when nothing is to do
  switch (s_Mode)
  {
    case Mode::Translate:
      if (s_vTranslate == plVec3(0.0f))
        return;

      sAction.Format("Translate: {0} | {1} | {2}", plArgF(s_vTranslate.x, 2), plArgF(s_vTranslate.y, 2), plArgF(s_vTranslate.z, 2));
      break;

    case Mode::TranslateDeviation:
      if (s_vTranslateDeviation == plVec3(0.0f))
        return;

      sAction.Format("Translate (deviation): {0} | {1} | {2}", plArgF(s_vTranslateDeviation.x, 2), plArgF(s_vTranslateDeviation.y, 2),
        plArgF(s_vTranslateDeviation.z, 2));
      break;

    case Mode::RotateX:
      if (s_vRotate.x == 0.0f)
        return;

      sAction.Format("Rotate X: {0}", plArgF(s_vRotate.x, 1));
      break;

    case Mode::RotateXRandom:
      if (s_vRotateRandom.x == 0.0f)
        return;

      sAction.Format("Rotate X (random): {0}", plArgF(s_vRotateRandom.x, 1));
      break;

    case Mode::RotateXDeviation:
      if (s_vRotateDeviation.x == 0.0f)
        return;

      sAction.Format("Rotate X (deviation): {0}", plArgF(s_vRotateDeviation.x, 1));
      break;

    case Mode::RotateY:
      if (s_vRotate.y == 0.0f)
        return;

      sAction.Format("Rotate Y: {0}", plArgF(s_vRotate.y, 1));
      break;

    case Mode::RotateYRandom:
      if (s_vRotateRandom.y == 0.0f)
        return;

      sAction.Format("Rotate Y (random): {0}", plArgF(s_vRotateRandom.y, 1));
      break;

    case Mode::RotateYDeviation:
      if (s_vRotateDeviation.y == 0.0f)
        return;

      sAction.Format("Rotate Y (deviation): {0}", plArgF(s_vRotateDeviation.y, 1));
      break;

    case Mode::RotateZ:
      if (s_vRotate.z == 0.0f)
        return;

      sAction.Format("Rotate Z: {0}", plArgF(s_vRotate.z, 1));
      break;

    case Mode::RotateZRandom:
      if (s_vRotateRandom.z == 0.0f)
        return;

      sAction.Format("Rotate Z (random): {0}", plArgF(s_vRotateRandom.z, 1));
      break;

    case Mode::RotateZDeviation:
      if (s_vRotateDeviation.z == 0.0f)
        return;

      sAction.Format("Rotate Z (deviation): {0}", plArgF(s_vRotateDeviation.z, 1));
      break;

    case Mode::Scale:
      if (s_vScale == plVec3(1.0f))
        return;

      sAction.Format("Scale: {0} | {1} | {2}", plArgF(s_vScale.x, 2), plArgF(s_vScale.y, 2), plArgF(s_vScale.z, 2));
      break;

    case Mode::ScaleDeviation:
      if (s_vScaleDeviation == plVec3(1.0f))
        return;

      sAction.Format(
        "Scale (deviation): {0} | {1} | {2}", plArgF(s_vScaleDeviation.x, 2), plArgF(s_vScaleDeviation.y, 2), plArgF(s_vScaleDeviation.z, 2));
      break;

    case Mode::UniformScale:
      if (s_fUniformScale == 1.0f)
        return;

      sAction.Format("Scale: {0}", plArgF(s_fUniformScale, 2));
      break;

    case Mode::UniformScaleDeviation:
      if (s_fUniformScaleDeviation == 1.0f)
        return;

      sAction.Format("Scale (deviation): {0}", plArgF(s_fUniformScaleDeviation, 2));
      break;

    case Mode::NaturalDeviationZ:
      sAction.Format("Natural Deviation Z: {0}", plArgF(s_fNaturalDeviationZ, 1));
      break;
  }


  auto history = m_pSceneDocument->GetCommandHistory();
  auto selman = m_pSceneDocument->GetSelectionManager();
  const auto& selection = selman->GetTopLevelSelection();

  if (selection.IsEmpty())
    return;

  Space space = (Space)ComboSpace->currentIndex();

  if (s_Mode == Mode::NaturalDeviationZ)
    space = Space::LocalEach;

  plTransform tReference = m_pSceneDocument->GetGlobalTransform(selection.PeekBack());

  if (space == Space::World)
  {
    tReference = m_pSceneDocument->GetGlobalTransform(selection.PeekBack());
    tReference.m_qRotation.SetIdentity();
  }

  history->StartTransaction(sAction.GetData());

  plRandom rng;
  rng.InitializeFromCurrentTime();

  for (const plDocumentObject* pObject : selection)
  {
    if (!pObject->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
      continue;

    plVec3 vTranslate = s_vTranslate;
    plVec3 vRotate = s_vRotate;
    plVec3 vScale = s_vScale;
    float fUniformScale = s_fUniformScale;


    switch (s_Mode)
    {
      case plQtDeltaTransformDlg::TranslateDeviation:
      {
        plVec3 vAbsTranslate = s_vTranslateDeviation.Abs();
        vTranslate.x = rng.DoubleVarianceAroundZero(vAbsTranslate.x);
        vTranslate.y = rng.DoubleVarianceAroundZero(vAbsTranslate.y);
        vTranslate.z = rng.DoubleVarianceAroundZero(vAbsTranslate.z);
        break;
      }

      case plQtDeltaTransformDlg::RotateXRandom:
        vRotate.x = rng.DoubleMinMax(-s_vRotateRandom.Abs().x, +s_vRotateRandom.Abs().x);
        break;

      case plQtDeltaTransformDlg::RotateXDeviation:
        vRotate.x = rng.DoubleVarianceAroundZero(s_vRotateDeviation.Abs().x);
        break;

      case plQtDeltaTransformDlg::RotateYRandom:
        vRotate.y = rng.DoubleMinMax(-s_vRotateRandom.Abs().y, +s_vRotateRandom.Abs().y);
        break;

      case plQtDeltaTransformDlg::RotateYDeviation:
        vRotate.y = rng.DoubleVarianceAroundZero(s_vRotateDeviation.Abs().y);
        break;

      case plQtDeltaTransformDlg::RotateZRandom:
        vRotate.z = rng.DoubleMinMax(-s_vRotateRandom.Abs().z, +s_vRotateRandom.Abs().z);
        break;

      case plQtDeltaTransformDlg::RotateZDeviation:
        vRotate.z = rng.DoubleVarianceAroundZero(s_vRotateDeviation.Abs().z);
        break;

      case plQtDeltaTransformDlg::ScaleDeviation:
      {
        const plVec3 vScaleMin = s_vScaleDeviation.CompMin(plVec3(1.0f).CompDiv(s_vScaleDeviation));
        const plVec3 vScaleMax = s_vScaleDeviation.CompMax(plVec3(1.0f).CompDiv(s_vScaleDeviation));
        vScale.x = rng.DoubleVariance((vScaleMax.x - vScaleMin.x) * 0.5, 1.0) + vScaleMin.x;
        vScale.y = rng.DoubleVariance((vScaleMax.y - vScaleMin.y) * 0.5, 1.0) + vScaleMin.y;
        vScale.z = rng.DoubleVariance((vScaleMax.z - vScaleMin.z) * 0.5, 1.0) + vScaleMin.z;
        break;
      }

      case plQtDeltaTransformDlg::UniformScaleDeviation:
      {
        const float fUniScaleMin = plMath::Min(1.0f / s_fUniformScaleDeviation, s_fUniformScaleDeviation);
        const float fUniScaleMax = plMath::Max(1.0f / s_fUniformScaleDeviation, s_fUniformScaleDeviation);
        fUniformScale = rng.DoubleVariance((fUniScaleMax - fUniScaleMin) * 0.5, 1.0) + fUniScaleMin;
        break;
      }

      default:
        break;
    }

    if (space == Space::LocalEach)
    {
      tReference = m_pSceneDocument->GetGlobalTransform(pObject);
    }

    plTransform trans = m_pSceneDocument->GetGlobalTransform(pObject);
    plTransform localTrans = tReference.GetInverse() * trans;
    plQuat qRot;

    switch (s_Mode)
    {
      case Mode::Translate:
      case Mode::TranslateDeviation:
        trans.m_vPosition += tReference.m_qRotation * vTranslate;
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Translation);
        break;

      case Mode::RotateX:
      case Mode::RotateXRandom:
      case Mode::RotateXDeviation:
        qRot.SetFromAxisAndAngle(plVec3(1, 0, 0), plAngle::Degree(vRotate.x));
        localTrans.m_qRotation = qRot * localTrans.m_qRotation;
        localTrans.m_vPosition = qRot * localTrans.m_vPosition;
        trans = tReference * localTrans;
        trans.m_qRotation.Normalize();
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Translation | TransformationChanges::Rotation);
        break;

      case Mode::RotateY:
      case Mode::RotateYRandom:
      case Mode::RotateYDeviation:
        qRot.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(vRotate.y));
        localTrans.m_qRotation = qRot * localTrans.m_qRotation;
        localTrans.m_vPosition = qRot * localTrans.m_vPosition;
        trans = tReference * localTrans;
        trans.m_qRotation.Normalize();
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Translation | TransformationChanges::Rotation);
        break;

      case Mode::RotateZ:
      case Mode::RotateZRandom:
      case Mode::RotateZDeviation:
        qRot.SetFromAxisAndAngle(plVec3(0, 0, 1), plAngle::Degree(vRotate.z));
        localTrans.m_qRotation = qRot * localTrans.m_qRotation;
        localTrans.m_vPosition = qRot * localTrans.m_vPosition;
        trans = tReference * localTrans;
        trans.m_qRotation.Normalize();
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Translation | TransformationChanges::Rotation);
        break;

      case Mode::Scale:
      case Mode::ScaleDeviation:
        trans.m_vScale = trans.m_vScale.CompMul(vScale);
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Scale);
        break;

      case Mode::UniformScale:
      case Mode::UniformScaleDeviation:
        trans.m_vScale *= fUniformScale;

        if (trans.m_vScale.x == trans.m_vScale.y && trans.m_vScale.x == trans.m_vScale.z)
          m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::UniformScale);
        else
          m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Scale);

        break;

      case Mode::NaturalDeviationZ:
      {
        const plAngle randomRotationZ = plAngle::Degree(rng.DoubleInRange(0, 360));

        plQuat qDeviation;
        qDeviation.SetIdentity();

        if (s_fNaturalDeviationZ > 0.0f)
        {
          const plVec3 vDeviationAxis = plVec3::CreateRandomDeviationZ(rng, plAngle::Degree(s_fNaturalDeviationZ));
          qDeviation.SetShortestRotation(plVec3(0, 0, 1), vDeviationAxis);
        }

        qRot.SetFromAxisAndAngle(plVec3(0, 0, 1), randomRotationZ);
        localTrans.m_qRotation = qDeviation * qRot * localTrans.m_qRotation;
        localTrans.m_vPosition = qDeviation * qRot * localTrans.m_vPosition;
        trans = tReference * localTrans;
        trans.m_qRotation.Normalize();
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Translation | TransformationChanges::Rotation);

        break;
      }

      default:
        break;
    }
  }

  history->FinishTransaction();

  ++m_uiActionsApplied;
  ButtonUndo->setEnabled(m_uiActionsApplied > 0 && m_pSceneDocument->GetCommandHistory()->CanUndo());
}

void plQtDeltaTransformDlg::on_ButtonUndo_clicked()
{
  auto history = m_pSceneDocument->GetCommandHistory();

  if (history->CanUndo())
  {
    --m_uiActionsApplied;
    history->Undo().IgnoreResult();
  }

  ButtonUndo->setEnabled(m_uiActionsApplied > 0 && m_pSceneDocument->GetCommandHistory()->CanUndo());
}

void plQtDeltaTransformDlg::QueryUI()
{
  switch (s_Mode)
  {
    case Mode::Translate:
      s_vTranslate.x = (float)Value1->value();
      s_vTranslate.y = (float)Value2->value();
      s_vTranslate.z = (float)Value3->value();
      break;

    case Mode::RotateX:
      s_vRotate.x = (float)Value1->value();
      break;

    case Mode::RotateY:
      s_vRotate.y = (float)Value1->value();
      break;

    case Mode::RotateZ:
      s_vRotate.z = (float)Value1->value();
      break;

    case Mode::Scale:
      s_vScale.x = (float)Value1->value();
      s_vScale.y = (float)Value2->value();
      s_vScale.z = (float)Value3->value();
      break;

    case Mode::UniformScale:
      s_fUniformScale = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::TranslateDeviation:
      s_vTranslateDeviation.x = (float)Value1->value();
      s_vTranslateDeviation.y = (float)Value2->value();
      s_vTranslateDeviation.z = (float)Value3->value();
      break;

    case plQtDeltaTransformDlg::RotateXRandom:
      s_vRotateRandom.x = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::RotateXDeviation:
      s_vRotateDeviation.x = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::RotateYRandom:
      s_vRotateRandom.y = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::RotateYDeviation:
      s_vRotateDeviation.y = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::RotateZRandom:
      s_vRotateRandom.z = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::RotateZDeviation:
      s_vRotateDeviation.y = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::ScaleDeviation:
      s_vScaleDeviation.x = (float)Value1->value();
      s_vScaleDeviation.y = (float)Value2->value();
      s_vScaleDeviation.z = (float)Value3->value();
      break;

    case plQtDeltaTransformDlg::UniformScaleDeviation:
      s_fUniformScaleDeviation = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::NaturalDeviationZ:
      s_fNaturalDeviationZ = (float)Value1->value();
      break;
  }
}

void plQtDeltaTransformDlg::UpdateUI()
{
  ComboSpace->setVisible(true);
  Value1->setVisible(true);
  Value2->setVisible(true);
  Value3->setVisible(true);
  Label1->setVisible(true);
  Label2->setVisible(true);
  Label3->setVisible(true);
  Label1->setText("X:");
  Label2->setText("Y:");
  Label3->setText("Z:");

  switch (s_Mode)
  {
    case Mode::Translate:
      Value1->setValue(s_vTranslate.x);
      Value2->setValue(s_vTranslate.y);
      Value3->setValue(s_vTranslate.z);
      Value1->setSingleStep(1.0f);
      Value2->setSingleStep(1.0f);
      Value3->setSingleStep(1.0f);
      break;

    case Mode::TranslateDeviation:
      Value1->setValue(s_vTranslateDeviation.x);
      Value2->setValue(s_vTranslateDeviation.y);
      Value3->setValue(s_vTranslateDeviation.z);
      Value1->setSingleStep(1.0f);
      Value2->setSingleStep(1.0f);
      Value3->setSingleStep(1.0f);
      break;

    case Mode::RotateX:
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Value1->setValue(s_vRotate.x);
      Value1->setSingleStep(5.0f);
      break;

    case Mode::RotateXRandom:
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Value1->setValue(s_vRotateRandom.x);
      Value1->setSingleStep(5.0f);
      break;

    case Mode::RotateXDeviation:
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Value1->setValue(s_vRotateDeviation.x);
      Value1->setSingleStep(5.0f);
      break;

    case Mode::RotateY:
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Label1->setText("Y:");
      Value1->setValue(s_vRotate.y);
      Value1->setSingleStep(5.0f);
      break;

    case Mode::RotateYRandom:
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Value1->setValue(s_vRotateRandom.y);
      Value1->setSingleStep(5.0f);
      break;

    case Mode::RotateYDeviation:
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Value1->setValue(s_vRotateDeviation.y);
      Value1->setSingleStep(5.0f);
      break;

    case Mode::RotateZ:
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Label1->setText("Z:");
      Value1->setValue(s_vRotate.z);
      Value1->setSingleStep(5.0f);
      break;

    case Mode::RotateZRandom:
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Value1->setValue(s_vRotateRandom.z);
      Value1->setSingleStep(5.0f);
      break;

    case Mode::RotateZDeviation:
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Value1->setValue(s_vRotateDeviation.z);
      Value1->setSingleStep(5.0f);
      break;

    case Mode::Scale:
      ComboSpace->setVisible(false);
      Value1->setValue(s_vScale.x);
      Value2->setValue(s_vScale.y);
      Value3->setValue(s_vScale.z);
      Value1->setSingleStep(1.0f);
      break;

    case Mode::ScaleDeviation:
      ComboSpace->setVisible(false);
      Value1->setValue(s_vScaleDeviation.x);
      Value2->setValue(s_vScaleDeviation.y);
      Value3->setValue(s_vScaleDeviation.z);
      Value1->setSingleStep(1.0f);
      break;

    case Mode::UniformScale:
      ComboSpace->setVisible(false);
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Label1->setText("XYZ:");
      Value1->setValue(s_fUniformScale);
      Value1->setSingleStep(1.0f);
      break;

    case Mode::UniformScaleDeviation:
      ComboSpace->setVisible(false);
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Label1->setText("XYZ:");
      Value1->setValue(s_fUniformScaleDeviation);
      Value1->setSingleStep(1.0f);
      break;

    case Mode::NaturalDeviationZ:
      ComboSpace->setVisible(false);
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Label1->setText("Max Tilt:");
      Value1->setValue(s_fNaturalDeviationZ);
      Value1->setSingleStep(1.0f);
      break;
  }
}

void plQtDeltaTransformDlg::on_Value1_valueChanged(double value)
{
  switch (s_Mode)
  {
    case Mode::Translate:
      s_vTranslate.x = (float)Value1->value();
      break;

    case Mode::RotateX:
      s_vRotate.x = (float)Value1->value();
      break;

    case Mode::RotateY:
      s_vRotate.y = (float)Value1->value();
      break;

    case Mode::RotateZ:
      s_vRotate.z = (float)Value1->value();
      break;

    case Mode::Scale:
      s_vScale.x = (float)Value1->value();
      break;

    case Mode::UniformScale:
      s_fUniformScale = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::TranslateDeviation:
      s_vTranslateDeviation.x = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::RotateXRandom:
      s_vRotateRandom.x = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::RotateXDeviation:
      s_vRotateDeviation.x = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::RotateYRandom:
      s_vRotateRandom.y = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::RotateYDeviation:
      s_vRotateDeviation.y = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::RotateZRandom:
      s_vRotateRandom.z = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::RotateZDeviation:
      s_vRotateDeviation.z = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::ScaleDeviation:
      s_vScaleDeviation.x = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::UniformScaleDeviation:
      s_fUniformScaleDeviation = (float)Value1->value();
      break;

    case plQtDeltaTransformDlg::NaturalDeviationZ:
      s_fNaturalDeviationZ = (float)Value1->value();
      break;

    default:
      break;
  }
}

void plQtDeltaTransformDlg::on_Value2_valueChanged(double value)
{
  switch (s_Mode)
  {
    case Mode::Translate:
      s_vTranslate.y = (float)Value2->value();
      break;

    case Mode::Scale:
      s_vScale.y = (float)Value2->value();
      break;

    case Mode::TranslateDeviation:
      s_vTranslateDeviation.y = (float)Value2->value();
      break;

    case Mode::ScaleDeviation:
      s_vScaleDeviation.y = (float)Value2->value();
      break;

    default:
      break;
  }
}

void plQtDeltaTransformDlg::on_Value3_valueChanged(double value)
{
  switch (s_Mode)
  {
    case Mode::Translate:
      s_vTranslate.z = (float)Value3->value();
      break;

    case Mode::Scale:
      s_vScale.z = (float)Value3->value();
      break;

    case Mode::TranslateDeviation:
      s_vTranslateDeviation.z = (float)Value3->value();
      break;

    case Mode::ScaleDeviation:
      s_vScaleDeviation.z = (float)Value3->value();
      break;

    default:
      break;
  }
}
