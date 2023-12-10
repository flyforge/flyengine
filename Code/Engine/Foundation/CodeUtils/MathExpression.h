#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Strings/String.h>

class plLogInterface;

/// \brief A wrapper around plExpression infrastructure to evaluate simple math expressions
class PLASMA_FOUNDATION_DLL plMathExpression
{
public:
  /// \brief Creates a new invalid math expression.
  ///
  /// Need to call Reset before you can do anything with it.
  plMathExpression();

  /// \brief Initializes using a given expression.
  ///
  /// If anything goes wrong it is logged and the math expression is in an invalid state.
  /// \param log
  ///   If null, default log interface will be used.
  explicit plMathExpression(plStringView sExpressionString); // [tested]

  /// \brief Reinitializes using the given expression.
  ///
  /// An empty string or nullptr are considered to be 'invalid' expressions.
  void Reset(plStringView sExpressionString);

  /// Whether the expression is valid and can be evaluated.
  bool IsValid() const { return m_bIsValid; }

  /// Returns the original expression string that this MathExpression can evaluate.
  plStringView GetExpressionString() const { return m_sOriginalExpression; }

  struct Input
  {
    plHashedString m_sName;
    float m_fValue;
  };

  /// \brief Evaluates parsed expression with the given inputs.
  ///
  /// Only way this function can fail is if the expression was not valid.
  /// \see IsValid
  float Evaluate(plArrayPtr<Input> inputs = plArrayPtr<Input>()); // [tested]

private:
  plHashedString m_sOriginalExpression;
  bool m_bIsValid = false;

  plExpressionByteCode m_ByteCode;
  plExpressionVM m_VM;
};
