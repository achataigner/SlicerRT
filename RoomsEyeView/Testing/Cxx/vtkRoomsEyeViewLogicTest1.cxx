/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Room's eye view includes
#include "vtkMRMLRoomsEyeViewNode.h"
#include "vtkSlicerRoomsEyeViewModuleLogic.h"

// Beams includes
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkSlicerIECTransformLogic.h"
#include "vtkSlicerBeamsModuleLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyData.h>


//----------------------------------------------------------------------------
/// Get all linear transforms from the scene that are not identity, and are not beam transform nodes
bool GetLinearTransformNodes(
  vtkMRMLScene* mrmlScene, std::vector<vtkMRMLLinearTransformNode*> &transformNodes,
  bool includeIdentity=true, bool includeBeamTransforms=true );
/// Determine whether a transform node is a beam transform
bool IsBeamTransformNode(vtkMRMLTransformNode* node);

int GetNumberOfNonIdentityIECTransforms(vtkMRMLScene* mrmlScene);
void PrintLinearTransformNodeMatrices( vtkMRMLScene* mrmlScene,
  bool includeIdentity=true, bool includeBeamTransforms=true );

bool IsTransformMatrixEqualTo(vtkMRMLScene* mrmlScene, const char* transformNodeName, double baselineElements[16]);
bool AreEqualWithTolerance(double a, double b);
bool IsEqual(vtkMatrix4x4* lhs, vtkMatrix4x4* rhs);

//----------------------------------------------------------------------------
int vtkRoomsEyeViewLogicTest1(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // Create and set up logic
  vtkSmartPointer<vtkSlicerRoomsEyeViewModuleLogic> revLogic = vtkSmartPointer<vtkSlicerRoomsEyeViewModuleLogic>::New();
  revLogic->SetMRMLScene(mrmlScene);
  revLogic->BuildRoomsEyeViewTransformHierarchy();

  // Create mock linac component models
  vtkSmartPointer<vtkMRMLModelNode> collimatorModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  collimatorModelNode->SetName(vtkSlicerRoomsEyeViewModuleLogic::COLLIMATOR_MODEL_NAME);
  mrmlScene->AddNode(collimatorModelNode);
  vtkSmartPointer<vtkPolyData> collimatorPolyData = vtkSmartPointer<vtkPolyData>::New();
  collimatorModelNode->SetAndObservePolyData(collimatorPolyData);
  vtkSmartPointer<vtkMRMLModelNode> gantryModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  gantryModelNode->SetName(vtkSlicerRoomsEyeViewModuleLogic::GANTRY_MODEL_NAME);
  mrmlScene->AddNode(gantryModelNode);
  vtkSmartPointer<vtkPolyData> gantryPolyData = vtkSmartPointer<vtkPolyData>::New();
  gantryModelNode->SetAndObservePolyData(gantryPolyData);
  vtkSmartPointer<vtkMRMLModelNode> imagingPanelLeftModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  imagingPanelLeftModelNode->SetName(vtkSlicerRoomsEyeViewModuleLogic::IMAGINGPANELLEFT_MODEL_NAME);
  mrmlScene->AddNode(imagingPanelLeftModelNode);
  vtkSmartPointer<vtkPolyData> imagingPanelLeftPolyData = vtkSmartPointer<vtkPolyData>::New();
  imagingPanelLeftModelNode->SetAndObservePolyData(imagingPanelLeftPolyData);
  vtkSmartPointer<vtkMRMLModelNode> imagingPanelRightModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  imagingPanelRightModelNode->SetName(vtkSlicerRoomsEyeViewModuleLogic::IMAGINGPANELRIGHT_MODEL_NAME);
  mrmlScene->AddNode(imagingPanelRightModelNode);
  vtkSmartPointer<vtkPolyData> imagingPanelRightPolyData = vtkSmartPointer<vtkPolyData>::New();
  imagingPanelRightModelNode->SetAndObservePolyData(imagingPanelRightPolyData);
  vtkSmartPointer<vtkMRMLModelNode> linacBodyModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  linacBodyModelNode->SetName(vtkSlicerRoomsEyeViewModuleLogic::LINACBODY_MODEL_NAME);
  mrmlScene->AddNode(linacBodyModelNode);
  vtkSmartPointer<vtkPolyData> linacBodyPolyData = vtkSmartPointer<vtkPolyData>::New();
  linacBodyModelNode->SetAndObservePolyData(linacBodyPolyData);
  vtkSmartPointer<vtkMRMLModelNode> patientSupportModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  patientSupportModelNode->SetName(vtkSlicerRoomsEyeViewModuleLogic::PATIENTSUPPORT_MODEL_NAME);
  mrmlScene->AddNode(patientSupportModelNode);
  vtkSmartPointer<vtkPolyData> patientSupportPolyData = vtkSmartPointer<vtkPolyData>::New();
  patientSupportModelNode->SetAndObservePolyData(patientSupportPolyData);
  vtkSmartPointer<vtkMRMLModelNode> tableTopModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  tableTopModelNode->SetName(vtkSlicerRoomsEyeViewModuleLogic::TABLETOP_MODEL_NAME);
  mrmlScene->AddNode(tableTopModelNode);
  vtkSmartPointer<vtkPolyData> tableTopPolyData = vtkSmartPointer<vtkPolyData>::New();
  tableTopModelNode->SetAndObservePolyData(tableTopPolyData);

  // Create REV parameter node
  vtkSmartPointer<vtkMRMLRoomsEyeViewNode> paramNode = vtkSmartPointer<vtkMRMLRoomsEyeViewNode>::New();
  mrmlScene->AddNode(paramNode);

  int expectedNumberOfLinearTransformNodes = 20;
  int numberOfLinearTransformNodes = mrmlScene->GetNumberOfNodesByClass("vtkMRMLLinearTransformNode");
  if (numberOfLinearTransformNodes != expectedNumberOfLinearTransformNodes)
  {
    std::cerr << __LINE__ << ": Number of linear transform nodes: " << numberOfLinearTransformNodes << " does not match expected value: " << expectedNumberOfLinearTransformNodes << std::endl;
    return EXIT_FAILURE;
  }

  // Print transform nodes in hierarchy
  std::cout << "Transform hierarchy successfully created (" << numberOfLinearTransformNodes << " transforms):" << std::endl;
  std::vector<vtkMRMLLinearTransformNode*> transformNodes;
  GetLinearTransformNodes(mrmlScene, transformNodes);
  std::vector<vtkMRMLLinearTransformNode*>::iterator trIt;
  for (trIt=transformNodes.begin(); trIt!=transformNodes.end(); ++trIt)
  {
    vtkMRMLLinearTransformNode* node = (*trIt);
    std::cout << "  " << node->GetName() << std::endl;
    vtkMRMLTransformNode* parentTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(node)->GetParentTransformNode();
    std::cout << "    Parent: " << (parentTransformNode?parentTransformNode->GetName():"(none)") << std::endl;
  }
  std::cout << std::endl;

  // Create plan and beam node so that the IEC logic can create the FixedReference to RAS transform for the isocenter test cases
  vtkSmartPointer<vtkMRMLRTBeamNode> beamNode = vtkSmartPointer<vtkMRMLRTBeamNode>::New();
  mrmlScene->AddNode(beamNode);
  vtkSmartPointer<vtkMRMLRTPlanNode> planNode = vtkSmartPointer<vtkMRMLRTPlanNode>::New();
  mrmlScene->AddNode(planNode);
  // Create beams logic that is responsible for handling beam added event
  vtkSmartPointer<vtkSlicerBeamsModuleLogic> beamsLogic = vtkSmartPointer<vtkSlicerBeamsModuleLogic>::New();
  beamsLogic->SetMRMLScene(mrmlScene);
  planNode->AddBeam(beamNode);
  // Temporary IEC logic creates the transform
  vtkSmartPointer<vtkSlicerIECTransformLogic> iecLogic = vtkSmartPointer<vtkSlicerIECTransformLogic>::New();

  //
  // Test effect of parameter changes on the transform hierarchy

  // Isocenter position, origin
  int numOfNonIdentityTransforms = 0;
  int expectedNumOfNonIdentityTransforms = 0;
  if ((numOfNonIdentityTransforms = GetNumberOfNonIdentityIECTransforms(mrmlScene)) != expectedNumOfNonIdentityTransforms)
    {
    std::cerr << __LINE__ << ": Number of non-identity linear transforms: " << numOfNonIdentityTransforms << " does not match expected value: " << expectedNumOfNonIdentityTransforms << std::endl;
    return EXIT_FAILURE;
    }

  iecLogic->UpdateTransformForBeam(beamNode);
  double expectedFixedReferenceToRasTransform_Origin_MatrixElements[16] =
    {  1, 0, 0, 0,   0, 0, 1, 0,   0, -1, 0, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      vtkSlicerIECTransformLogic::FIXEDREFERENCE_TO_RAS_TRANSFORM_NODE_NAME, expectedFixedReferenceToRasTransform_Origin_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": FixedReferenceToRasTransform transform does not match baseline for origin" << std::endl;
    return EXIT_FAILURE;
    }

  // Isocenter position, translated
  double isocenter[3] = {1000.0, 200.0, 0.0};
  planNode->SetIsocenterPosition(isocenter);
  iecLogic->UpdateTransformForBeam(beamNode);

  double expectedFixedReferenceToRasTransform_Translated_MatrixElements[16] =
    {  1, 0, 0, 1000,   0, 0, 1, 200,   0, -1, 0, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      vtkSlicerIECTransformLogic::FIXEDREFERENCE_TO_RAS_TRANSFORM_NODE_NAME, expectedFixedReferenceToRasTransform_Translated_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": FixedReferenceToRasTransform transform does not match baseline for translated isocenter" << std::endl;
    return EXIT_FAILURE;
    }

  // Gantry angle, 1 degree
  paramNode->SetGantryRotationAngle(1.0);
  revLogic->UpdateGantryToFixedReferenceTransform(paramNode);
  expectedNumOfNonIdentityTransforms = 2;
  if ((numOfNonIdentityTransforms = GetNumberOfNonIdentityIECTransforms(mrmlScene)) != expectedNumOfNonIdentityTransforms)
    {
    std::cerr << __LINE__ << ": Number of non-identity linear transforms: " << numOfNonIdentityTransforms << " does not match expected value: " << expectedNumOfNonIdentityTransforms << std::endl;
    return EXIT_FAILURE;
    }

  double expectedGantryToFixedReference_1_MatrixElements[16] =
    {  0.999848, 0, -0.0174524, 0,   0, 1, 0, 0,   0.0174524, 0, 0.999848, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      vtkSlicerIECTransformLogic::GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME, expectedGantryToFixedReference_1_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": GantryToFixedReferenceTransform does not match baseline for 1 degree angle" << std::endl;
    return EXIT_FAILURE;
    }

  // Gantry angle, 90 degrees
  paramNode->SetGantryRotationAngle(90.0);
  revLogic->UpdateGantryToFixedReferenceTransform(paramNode);
  double expectedGantryToFixedReference_90_MatrixElements[16] =
    {  0, 0, -1, 0,   0, 1, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      vtkSlicerIECTransformLogic::GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME, expectedGantryToFixedReference_90_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": GantryToFixedReferenceTransform does not match baseline for 90 degrees angle" << std::endl;
    return EXIT_FAILURE;
    }

  // Collimator angle, 1 degree
  //TODO: Different transform after consolidation (and for many other transforms as well)
  paramNode->SetCollimatorRotationAngle(1.0);
  revLogic->UpdateCollimatorToFixedReferenceIsocenterTransform(paramNode); //TODO:
  revLogic->UpdateFixedReferenceIsocenterToCollimatorRotatedTransform(paramNode); //TODO:
  revLogic->UpdateCollimatorToGantryTransform(paramNode);
  expectedNumOfNonIdentityTransforms = 3;
  if ((numOfNonIdentityTransforms = GetNumberOfNonIdentityIECTransforms(mrmlScene)) != expectedNumOfNonIdentityTransforms)
    {
    std::cerr << __LINE__ << ": Number of non-identity linear transforms: " << numOfNonIdentityTransforms << " does not match expected value: " << expectedNumOfNonIdentityTransforms << std::endl;
    return EXIT_FAILURE;
    }

  double expectedFixedReferenceIsocenterToCollimatorRotatedTransform_1_MatrixElements[16] =
    {  0.999848, -0.0174524, 0, 0,   0.0174524, 0.999848, 0, 0,   0, 0, 1, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      vtkSlicerIECTransformLogic::FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME, expectedFixedReferenceIsocenterToCollimatorRotatedTransform_1_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": FixedReferenceIsocenterToCollimatorRotatedTransform does not match baseline" << std::endl;
    return EXIT_FAILURE;
    }

  // Collimator angle, 90 degrees
  paramNode->SetCollimatorRotationAngle(90.0);
  revLogic->UpdateCollimatorToFixedReferenceIsocenterTransform(paramNode); //TODO:
  revLogic->UpdateFixedReferenceIsocenterToCollimatorRotatedTransform(paramNode); //TODO:
  revLogic->UpdateCollimatorToGantryTransform(paramNode);
  double expectedFixedReferenceIsocenterToCollimatorRotatedTransform_90_MatrixElements[16] =
    {  0, -1, 0, 0,   1, 0, 0, 0,   0, 0, 1, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      vtkSlicerIECTransformLogic::FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME, expectedFixedReferenceIsocenterToCollimatorRotatedTransform_90_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": FixedReferenceIsocenterToCollimatorRotatedTransform does not match baseline" << std::endl;
    return EXIT_FAILURE;
    }

  //TODO: Test code to print all non-identity transforms (useful to add more test cases)
  //std::cout << "ZZZ after collimator angle 90:" << std::endl;
  //PrintLinearTransformNodeMatrices(mrmlScene, false, true);

  std::cout << "Room's eye view logic test passed" << std::endl;
  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int GetNumberOfNonIdentityIECTransforms(vtkMRMLScene* mrmlScene)
{
  std::vector<vtkMRMLLinearTransformNode*> nonIdentityIECTransforms;
  if (!GetLinearTransformNodes(mrmlScene, nonIdentityIECTransforms, false, false))
  {
    return -1;
  }

  return nonIdentityIECTransforms.size();
}

//----------------------------------------------------------------------------
void PrintLinearTransformNodeMatrices(vtkMRMLScene* mrmlScene,
  bool includeIdentity/*=true*/, bool includeBeamTransforms/*=true*/ )
{
  std::vector<vtkMRMLLinearTransformNode*> transformNodes;
  GetLinearTransformNodes(mrmlScene, transformNodes, includeIdentity, includeBeamTransforms);
  if (!transformNodes.size())
  {
    std::cout << "There are no non-identity IEC transform in the scene" << std::endl;
    return;
  }
  std::cout << "Printing " << transformNodes.size() << " linear transform nodes in the scene (include identity transforms:"
    << (includeIdentity?"true":"false") << ", include beam transforms:" << (includeBeamTransforms?"true":"false") << std::endl;

  // Print linear transform node matrices for nodes that fulfill the conditions
  int numberOfNonIdentityTransforms = 0;
  std::vector<vtkMRMLLinearTransformNode*>::iterator trIt;
  vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
  for (trIt=transformNodes.begin(); trIt!=transformNodes.end(); ++trIt)
  {
    vtkMRMLLinearTransformNode* transformNode = (*trIt);
    if (!transformNode)
    {
      continue;
    }

    std::cout << "- " << transformNode->GetName() << std::endl << "    {";
    transformNode->GetMatrixTransformToParent(matrix);
    for (int i = 0; i < 4; i++)
    {
      std::cout << "  ";
      for (int j = 0; j < 4; j++)
      {
        std::cout << matrix->GetElement(i,j) << ((i==3&&j==3)?"  ":", ");
      }
    }
    std::cout << "}" << std::endl;
  }
}

//----------------------------------------------------------------------------
bool IsBeamTransformNode(vtkMRMLTransformNode* node)
{
  if (!node)
  {
    return false;
  }

  std::string nodeName(node->GetName());
  std::string postfix = nodeName.substr(nodeName.size() - strlen(vtkMRMLRTBeamNode::BEAM_TRANSFORM_NODE_NAME_POSTFIX), strlen(vtkMRMLRTBeamNode::BEAM_TRANSFORM_NODE_NAME_POSTFIX));
  return !postfix.compare(vtkMRMLRTBeamNode::BEAM_TRANSFORM_NODE_NAME_POSTFIX);
}

//----------------------------------------------------------------------------
bool GetLinearTransformNodes(
  vtkMRMLScene* mrmlScene, std::vector<vtkMRMLLinearTransformNode*> &transformNodes,
  bool includeIdentity/*=true*/, bool includeBeamTransforms/*=true*/ )
{
  if (!mrmlScene)
  {
    return false;
  }

  transformNodes.clear();

  // Create identity matrix for comparison
  vtkSmartPointer<vtkMatrix4x4> identityMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  identityMatrix->Identity();

  // Collect transform nodes that fulfill the conditions
  mrmlScene->InitTraversal();
  vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
  vtkMRMLNode* node = mrmlScene->GetNextNodeByClass("vtkMRMLLinearTransformNode");
  while (node)
  {
    vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(node);
    transformNode->GetMatrixTransformToParent(matrix);
    if ( (includeIdentity || !IsEqual(matrix, identityMatrix))
      && (includeBeamTransforms || !IsBeamTransformNode(transformNode)) )
    {
      transformNodes.push_back(transformNode);
    }

    node = mrmlScene->GetNextNodeByClass("vtkMRMLLinearTransformNode");
  }

  return true;
}

//---------------------------------------------------------------------------
bool IsTransformMatrixEqualTo(vtkMRMLScene* mrmlScene, const char* transformNodeName, double baselineElements[16])
{
  if (!mrmlScene || !transformNodeName)
  {
    return false;
  }

  // Get transform node and verify that it's linear
  vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    mrmlScene->GetFirstNodeByName(transformNodeName) );
  if (!transformNode)
  {
    std::cerr << __LINE__ << ": Failed to get transform node by name: " << transformNodeName << std::endl;
    return false;
  }
  vtkSmartPointer<vtkTransform> linearTransform = vtkSmartPointer<vtkTransform>::New();
  if (!vtkMRMLTransformNode::IsGeneralTransformLinear(transformNode->GetTransformToParent(), linearTransform))
  {
    std::cerr << __LINE__ << ": Non-linear transform found in transform node: " << transformNodeName << std::endl;
    return false;
  }

  // Create matrix from elements
  vtkSmartPointer<vtkMatrix4x4> baselineMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      baselineMatrix->SetElement(i,j, baselineElements[4*i+j]);
    }
  }

  return IsEqual(linearTransform->GetMatrix(), baselineMatrix);
}

//---------------------------------------------------------------------------
bool AreEqualWithTolerance(double a, double b)
{
  return fabs(a - b) < 0.0001;
};

//---------------------------------------------------------------------------
bool IsEqual(vtkMatrix4x4* lhs, vtkMatrix4x4* rhs)
{
  if (!lhs || !rhs)
    {
    return false;
    }
  return  AreEqualWithTolerance(lhs->GetElement(0,0), rhs->GetElement(0,0)) &&
          AreEqualWithTolerance(lhs->GetElement(0,1), rhs->GetElement(0,1)) &&
          AreEqualWithTolerance(lhs->GetElement(0,2), rhs->GetElement(0,2)) &&
          AreEqualWithTolerance(lhs->GetElement(0,3), rhs->GetElement(0,3)) &&
          AreEqualWithTolerance(lhs->GetElement(1,0), rhs->GetElement(1,0)) &&
          AreEqualWithTolerance(lhs->GetElement(1,1), rhs->GetElement(1,1)) &&
          AreEqualWithTolerance(lhs->GetElement(1,2), rhs->GetElement(1,2)) &&
          AreEqualWithTolerance(lhs->GetElement(1,3), rhs->GetElement(1,3)) &&
          AreEqualWithTolerance(lhs->GetElement(2,0), rhs->GetElement(2,0)) &&
          AreEqualWithTolerance(lhs->GetElement(2,1), rhs->GetElement(2,1)) &&
          AreEqualWithTolerance(lhs->GetElement(2,2), rhs->GetElement(2,2)) &&
          AreEqualWithTolerance(lhs->GetElement(2,3), rhs->GetElement(2,3)) &&
          AreEqualWithTolerance(lhs->GetElement(3,0), rhs->GetElement(3,0)) &&
          AreEqualWithTolerance(lhs->GetElement(3,1), rhs->GetElement(3,1)) &&
          AreEqualWithTolerance(lhs->GetElement(3,2), rhs->GetElement(3,2)) &&
          AreEqualWithTolerance(lhs->GetElement(3,3), rhs->GetElement(3,3));
}
