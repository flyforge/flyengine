#pragma once

#include "RapidXML/rapidxml.hpp"

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

using namespace rapidxml;

class plStreamReader;

enum class CE_Visibility : plUInt8
{
  None,
  Public,
  Protected,
  Private,
};

enum class CodeElementType : plUInt8
{
  None,
  Namespace,
  Struct,
  Typedef,
  Class,
  Function,
  Variable,
  OperatorFunction,
  Unimplemented,
  Union,
  CvQualifiedType,
  Enumeration,
  FundamentalType,
  PointerType,
  Constructor,
  OperatorMethod,
  Destructor,
  Field,
  ElaboratedType,
  ReferenceType,
  Method,
  ArrayType,
  Converter,
  FunctionType,
  Comment,
  File,
};

struct CodeElement
{
  PLASMA_DECLARE_POD_TYPE();

  CodeElementType m_Type = CodeElementType::None;
  plUInt32 m_uiIndex = 0;
};

struct CE_File
{
  plString m_sPath;
};

struct CE_Comment
{
  PLASMA_DECLARE_POD_TYPE();
};

struct CE_Typedef
{
  plString m_sName;
  plString m_sTypeID;
  plString m_sContextID;
};

struct CE_Structure
{
  enum class Type
  {
    None,
    Struct,
    Class,
    Union
  };

  Type m_Type = Type::None;
  plString m_sName;
  plString m_sContextID;
  // bool m_bAstract = false; TODO ?
  // TODO: bases
  plHybridArray<plString, 16> m_Members;
};

struct CE_Argument
{
  plString m_sName;
  plString m_sTypeID;
  plString m_sDefault;
};

struct CE_Function
{
  plString m_sName;
  plString m_sReturnTypeID;
  plString m_sContextID;

  plHybridArray<CE_Argument, 4> m_Arguments;
};

struct CE_Variable
{
  plString m_sName;
  plString m_sTypeID;
  plString m_sContextID;
  CE_Visibility m_Visibility;
  bool m_bStatic = false;
  // bool m_bExtern ?
  // TODO: init value ?
};

struct CE_ArrayType
{
  plString m_sTypeID;
  plUInt32 m_uiSize = plInvalidIndex;
};

struct CE_FundamentalType
{
  plString m_sName;
};

struct CE_PointerOrReferenceType
{
  plString m_sTypeID;
};

struct CE_CvQualifiedType
{
  plString m_sTypeID;
  bool m_bConst = false;
  bool m_bVolatile = false;
};

struct CE_Field
{
  plString m_sName;
  plString m_sTypeID;
  CE_Visibility m_Visibility;
  plString m_sContextID;
};

struct CE_ElaboratedType
{
  plString m_sTypeID;
};

struct CE_Method
{
  plString m_sName;
  plString m_sReturnTypeID;
  bool m_bConst = false;
  bool m_bStatic = false;
  bool m_bVirtual = false;
  bool m_bOverride = false;
  bool m_bArtificial = false;
  CE_Visibility m_Visibility;
  plString m_sContextID;

  plHybridArray<CE_Argument, 4> m_Arguments;
};

struct CE_Constructor
{
  bool m_bArtifical = false;
  CE_Visibility m_Visibility;
  plString m_sContextID;

  plHybridArray<CE_Argument, 4> m_Arguments;
};

struct CE_Destructor
{
  plString m_sName;
  plString m_sContextID;
  bool m_bArtifical = false;
  bool m_bVirtual = false; // TODO
  CE_Visibility m_Visibility;
};

struct CE_Namespace
{
  plString m_sName;
  plString m_sContextID;
  // TODO: members
};

struct CE_EnumValue
{
  plString m_sName;
  plString m_sValue;
};

struct CE_Enumeration
{
  plString m_sName;
  plString m_sTypeID;
  plString m_sContextID;
  CE_Visibility m_Visibility;

  plHybridArray<CE_EnumValue, 4> m_Values;
};

class CppStructure
{
public:
  plResult ParseXML(plStreamReader& stream);

  plResult SynthesizeTemplate(const char* szTargetType, const char* szTemplateType1, const char* szTemplateType2);

  plMap<plString, CodeElement> m_IdToElement;

  plDeque<CE_File> m_CesFile;
  plDeque<CE_Comment> m_CesComment;
  plDeque<CE_Typedef> m_CesTypedef;
  plDeque<CE_Structure> m_CesStructure;
  plDeque<CE_Function> m_CesFunction;
  plDeque<CE_Variable> m_CesVariable;
  plDeque<CE_FundamentalType> m_CesFundamentalType;
  plDeque<CE_ArrayType> m_CesArrayType;
  plDeque<CE_PointerOrReferenceType> m_CesPointerType;
  plDeque<CE_PointerOrReferenceType> m_CesReferenceType;
  plDeque<CE_CvQualifiedType> m_CesCvQualifiedType;
  plDeque<CE_Field> m_CesField;
  plDeque<CE_ElaboratedType> m_CesElaboratedType;
  plDeque<CE_Method> m_CesMethod;
  plDeque<CE_Constructor> m_CesConstructor;
  plDeque<CE_Destructor> m_CesDestructor;
  plDeque<CE_Function> m_CesOperatorFunction;
  plDeque<CE_Namespace> m_CesNamespace;
  plDeque<CE_Enumeration> m_CesEnumeration;
  plDeque<CE_Method> m_CesOperatorMethod;

private:
  plUInt32 ParseNamespace(xml_node<>* node);                          // [done]
  plUInt32 ParseTypedef(xml_node<>* node);                            // [done]
  plUInt32 ParseStructure(xml_node<>* node, CE_Structure::Type type); // [done]
  plUInt32 ParseFunction(xml_node<>* node);                           // [done]
  plUInt32 ParseVariable(xml_node<>* node);                           // [done]
  plUInt32 ParseOperatorFunction(xml_node<>* node);                   // [done]
  plUInt32 ParseUnimplemented(xml_node<>* node);                      // [done]
  plUInt32 ParseCvQualifiedType(xml_node<>* node);                    // [done]
  plUInt32 ParseEnumeration(xml_node<>* node);                        // [done]
  plUInt32 ParseFundamentalType(xml_node<>* node);                    // [done]
  plUInt32 ParsePointerType(xml_node<>* node);                        // [done]
  plUInt32 ParseConstructor(xml_node<>* node);                        // [done]
  plUInt32 ParseOperatorMethod(xml_node<>* node);                     // [done]
  plUInt32 ParseDestructor(xml_node<>* node);                         // [done]
  plUInt32 ParseField(xml_node<>* node);                              // [done]
  plUInt32 ParseElaboratedType(xml_node<>* node);                     // [done]
  plUInt32 ParseReferenceType(xml_node<>* node);                      // [done]
  plUInt32 ParseMethod(xml_node<>* node);                             // [done]
  plUInt32 ParseArrayType(xml_node<>* node);                          // [done]
  plUInt32 ParseConverter(xml_node<>* node);                          //
  plUInt32 ParseFunctionType(xml_node<>* node);                       //
  plUInt32 ParseComment(xml_node<>* node);                            // [done]
  plUInt32 ParseFile(xml_node<>* node);                               // [done]
  void ParseArguments(xml_node<>* first, plDynamicArray<CE_Argument>& arguments);
  void ParseEnumValues(xml_node<>* first, plDynamicArray<CE_EnumValue>& values);
  void ParseVisibility(xml_node<>* node, CE_Visibility& out);
  void ParseContext(xml_node<>* node, plString& out);

  plResult SynthesizeMember(CE_Structure& out, const plString& m1, const plString& m2);
  plResult SynthesizeMethod(CE_Method& out, const CE_Method& m1, const CE_Method& m2);
  plResult SynthesizeType(plString& type, const plString& m1, const plString& m2);
  plString RegisterSynthElement(CodeElementType type, plUInt32 index);
  void PreprocessStructureForSynthesis(CE_Structure& structure) const;

  plUInt32 m_uiTtCounter = 0;

  plStringBuilder m_sXmlContent;
  xml_document<> m_XmlDoc;
};
