#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <cmath>
#include <algorithm>
#include <limits>
#include <fstream>
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkBinaryContourImageFilter.h"
#include "itkMultiplyImageFilter.h"
#include "itkContinuousIndex.h"
#include "itkImageIteratorWithIndex.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkBinaryFillholeImageFilter.h"
#include "itkImageFileWriter.h"
#include "itkBinaryBallStructuringElement.h"
#include "itkBinaryDilateImageFilter.h"
#include "itkSubtractImageFilter.h"
#include "itkAddImageFilter.h"
#include "itkImageToVTKImageFilter.h"
#include "vtkImageData.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkLine.h"
#include "vtkPolyLine.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkMarchingCubes.h"
#include "vtkCleanPolyData.h"
#include "vtkDelaunay2D.h"
#include "vtkSmoothPolyDataFilter.h"
#include "vtkFloatArray.h"
#include "vtkDataArray.h"
#include "vtkPolyDataNormals.h" 
#include "vtkXMLPolyDataWriter.h"
#include "vtkMarchingCubes.h"
#include "vtkNIFTIImageWriter.h"
#include "itkMaskImageFilter.h"
#include "vtkDelaunay2D.h"
#include "vtkSphereSource.h"
#include "itkBinaryDilateImageFilter.h"
#include "itkBinaryBallStructuringElement.h"
#include "itkChangeInformationImageFilter.h"




// Initialize
const unsigned int nDims = 3;
typedef itk::Image<double,nDims> DoubleImageType;
typedef itk::VectorImage<double,nDims> VectorImageType;
typedef itk::Image<int,nDims> BinaryImageType;

typedef itk::ImageFileReader<DoubleImageType> DoubleImageFileReaderType;
typedef itk::ImageFileReader<VectorImageType> VectorImageFileReaderType;
typedef itk::ImageFileReader<BinaryImageType> BinaryImageFileReaderType;

typedef itk::ImageRegionIteratorWithIndex<BinaryImageType> ImageIteratorType;

typedef itk::LinearInterpolateImageFunction<DoubleImageType,double> LinearInterpolateDoubleImageFunctionType;
typedef itk::LinearInterpolateImageFunction<VectorImageType,double> LinearInterpolateVectorImageFunctionType;
typedef itk::LinearInterpolateImageFunction<BinaryImageType,double> LinearInterpolateIntImageFunctionType;

typedef itk::SubtractImageFilter<BinaryImageType,BinaryImageType,BinaryImageType> SubtractImageFilterType;
typedef itk::AddImageFilter<BinaryImageType,BinaryImageType,BinaryImageType> AddImageFilterType;
typedef itk::MultiplyImageFilter<BinaryImageType,BinaryImageType> MultiplyImageFilterType;
typedef itk::BinaryThresholdImageFilter<BinaryImageType,BinaryImageType> BinaryThresholdImageFilterType;
typedef itk::BinaryFillholeImageFilter<BinaryImageType> BinaryFillholeImageFilterType;

typedef itk::BinaryBallStructuringElement<BinaryImageType::PixelType,nDims> BinaryBallStructuringElementType;
typedef itk::BinaryDilateImageFilter<BinaryImageType,BinaryImageType,BinaryBallStructuringElementType> BinaryDilateImageFilterType;

typedef itk::ChangeInformationImageFilter<DoubleImageType> ChangeInformationDoubleImageFilterType;
typedef itk::ChangeInformationImageFilter<BinaryImageType> ChangeInformationImageFilterType;
typedef itk::ChangeInformationImageFilter<VectorImageType> ChangeInformationVectorImageFilterType;

typedef itk::ImageFileWriter<VectorImageType> VectorImageFileWriterType;


// Function:  read vector image and set origin to [0,0,0]
VectorImageType::Pointer readVectorImage(std::string filename){
  VectorImageFileReaderType::Pointer reader = VectorImageFileReaderType::New();
  reader->SetFileName(filename.c_str());
  reader->Update();

  return reader->GetOutput();
}



// Function:  read float image and set origin to [0,0,0]
DoubleImageType::Pointer readDoubleImage(std::string filename){
  DoubleImageFileReaderType::Pointer reader = DoubleImageFileReaderType::New();
  reader->SetFileName(filename.c_str());
  reader->Update();

  return reader->GetOutput();
}



// Function:  read binary image and set origin to [0,0,0]
BinaryImageType::Pointer readBinaryImage(std::string filename){
  BinaryImageFileReaderType::Pointer reader = BinaryImageFileReaderType::New();
  reader->SetFileName(filename.c_str());
  reader->Update();
  
  return reader->GetOutput();
}



// Function: convert VTK point to ITK point (templated with binary image)
itk::ContinuousIndex<double,nDims> vtk2itk_BinaryImage(double vtkPt[nDims], BinaryImageType *refImage) {
  itk::ContinuousIndex<double,nDims> itkPt;
  for(unsigned int d = 0; d < nDims; d++) {
    itkPt[d] = (vtkPt[d] - refImage->GetOrigin()[d])/(refImage->GetSpacing()[d] * refImage->GetDirection()[d][d]);
  }
  itkPt[0] = -itkPt[0];
  itkPt[1] = -itkPt[1];

  return itkPt;
}



// Function: convert ITK point to VTK point (templated with binary image)
void itk2vtk_BinaryImage(itk::ContinuousIndex<double,nDims> itkPt, BinaryImageType *refImage, double vtkPt[nDims]) {
  for(unsigned int d = 0; d < nDims; d++) {
    vtkPt[d] = itkPt[d] * (refImage->GetSpacing()[d] * refImage->GetDirection()[d][d]) + refImage->GetOrigin()[d];
  }
  vtkPt[0] = -vtkPt[0];
  vtkPt[1] = -vtkPt[1];
}



// Function: convert VTK point to ITK point (templated with vector image)
itk::ContinuousIndex<double,nDims> vtk2itk_VectorImage(double vtkPt[nDims], VectorImageType *refImage) {
  itk::ContinuousIndex<double,nDims> itkPt;
  for(unsigned int d = 0; d < nDims; d++) {
    itkPt[d] = (vtkPt[d] - refImage->GetOrigin()[d])/(refImage->GetSpacing()[d] * refImage->GetDirection()[d][d]);
  }
  itkPt[0] = -itkPt[0];
  itkPt[1] = -itkPt[1];

  return itkPt;
}



// Function: convert ITK point to VTK point (templated with vector image)
void itk2vtk_VectorImage(itk::ContinuousIndex<double,nDims> itkPt, VectorImageType *refImage, double vtkPt[nDims]) {
  for(unsigned int d = 0; d < nDims; d++) {
    vtkPt[d] = itkPt[d] * (refImage->GetSpacing()[d] * refImage->GetDirection()[d][d]) + refImage->GetOrigin()[d];
  }
  vtkPt[0] = -vtkPt[0];
  vtkPt[1] = -vtkPt[1];
}



// Function:  create polydata from points at each voxel in an image
vtkSmartPointer<vtkPolyData> convertMaskToPolyData(BinaryImageType *mask, BinaryImageType *edge) {

  // Convert ITK image to VTK image
  typedef itk::ImageToVTKImageFilter<BinaryImageType> ImageToVTKImageFilterType;
  ImageToVTKImageFilterType::Pointer ITKtoVTK = ImageToVTKImageFilterType::New();
  ITKtoVTK->SetInput(mask);
  ITKtoVTK->Update();
  vtkSmartPointer<vtkImageData> vtkMask = ITKtoVTK->GetOutput();

  double origin[nDims];
  origin[0] = -mask->GetOrigin()[0];
  origin[1] = -mask->GetOrigin()[1];
  origin[2] = mask->GetOrigin()[2];
  vtkMask->SetOrigin(origin);

  // Convert mask to mesh
  vtkSmartPointer<vtkMarchingCubes> marchingCube = vtkSmartPointer<vtkMarchingCubes>::New();
  marchingCube->SetInputData(vtkMask);
  marchingCube->ComputeNormalsOn();
  marchingCube->SetValue(0,0.5);
  marchingCube->Update();

  vtkSmartPointer<vtkSmoothPolyDataFilter> smoother = vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
  smoother->SetInputConnection(marchingCube->GetOutputPort());
  smoother->SetNumberOfIterations(15);
  smoother->SetRelaxationFactor(0.1);
  smoother->FeatureEdgeSmoothingOff();
  smoother->BoundarySmoothingOn();
  smoother->Update();

  vtkSmartPointer<vtkPolyDataNormals> normalGenerator = vtkSmartPointer<vtkPolyDataNormals>::New();
  normalGenerator->SetInputConnection(smoother->GetOutputPort());
  normalGenerator->ComputePointNormalsOn();
  normalGenerator->ComputeCellNormalsOn();
  normalGenerator->Update();
  
  // Label points on the outside or inside of the brain
  vtkSmartPointer<vtkPolyData> mesh = normalGenerator->GetOutput();
  int nPts = mesh->GetNumberOfPoints();

  vtkSmartPointer<vtkFloatArray> edgeLabels = vtkSmartPointer<vtkFloatArray>::New() ;
  edgeLabels->SetNumberOfComponents(1);
  edgeLabels->SetNumberOfValues(nPts);
  edgeLabels->SetName("Edge");

  // Create the dilated edge mask
  typedef itk::BinaryBallStructuringElement<BinaryImageType::PixelType,nDims> BinaryBallStructuringElementType;
  BinaryBallStructuringElementType kernel;
  kernel.SetRadius(1);
  kernel.CreateStructuringElement();

  typedef itk::BinaryDilateImageFilter<BinaryImageType,BinaryImageType,BinaryBallStructuringElementType> BinaryDilateImageFilterType;
  BinaryDilateImageFilterType::Pointer dilater = BinaryDilateImageFilterType::New();
  dilater->SetInput(edge);
  dilater->SetKernel(kernel);
  dilater->SetDilateValue(1);
  dilater->Update();

  SubtractImageFilterType::Pointer subtractImageFilter = SubtractImageFilterType::New();
  subtractImageFilter->SetInput1(dilater->GetOutput());
  subtractImageFilter->SetInput2(mask);
  subtractImageFilter->Update();

  BinaryThresholdImageFilterType::Pointer dilateThresholder = BinaryThresholdImageFilterType::New();
  dilateThresholder->SetInput(subtractImageFilter->GetOutput());
  dilateThresholder->SetLowerThreshold(1);
  dilateThresholder->SetUpperThreshold(1);
  dilateThresholder->SetInsideValue(1);
  dilateThresholder->SetOutsideValue(0);
  dilateThresholder->Update();
  
  AddImageFilterType::Pointer addImageFilter = AddImageFilterType::New();
  addImageFilter->SetInput1(dilateThresholder->GetOutput());
  addImageFilter->SetInput2(edge);
  addImageFilter->Update();

  BinaryImageType::Pointer dilatedEdge = addImageFilter->GetOutput();
  LinearInterpolateIntImageFunctionType::Pointer edgeInterpolator = LinearInterpolateIntImageFunctionType::New();
  edgeInterpolator->SetInputImage(dilatedEdge);

  BinaryImageType::IndexType x0index;
  itk::ContinuousIndex<double,nDims> x0;
  double x0vtk[nDims];
  for(unsigned int p = 0; p < nPts; p++) {
    mesh->GetPoint(p,x0vtk);
    
    x0[0] = -x0vtk[0]; // this is because itkImage-->vtkImage switches the dimensions (WHY)
    x0[1] = -x0vtk[1];
    x0[2] = x0vtk[2];
    
    dilatedEdge->TransformPhysicalPointToIndex(x0,x0index) ;
    BinaryImageType::PixelType pixel = dilatedEdge->GetPixel(x0index);
    edgeLabels->SetValue(p,pixel);
  }
  mesh->GetPointData()->AddArray(edgeLabels);

  /*
  vtkSmartPointer<vtkXMLPolyDataWriter> testMeshWriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  testMeshWriter->SetInputData(mesh);
  testMeshWriter->SetFileName("/home/larsonke/Code/Synthetic-Atrophy/Data/testMesh.vtp");
  testMeshWriter->Write();
  */

  return mesh;
}



// Function:  interpolate along a line in a polydata from a gradient vector value to {0,0,0}
std::vector<VectorImageType::PixelType> blurField(vtkPoints *pointCloud, std::vector<int> pointIds, VectorImageType::Pointer fieldImage) {
  unsigned int nPts = static_cast<int>(pointIds.size());
  //std::cout << " --> " << nPts << std::endl;
  std::vector<VectorImageType::PixelType> interpField;

  // Set first value
  LinearInterpolateVectorImageFunctionType::Pointer fieldInterpolator = LinearInterpolateVectorImageFunctionType::New();
  fieldInterpolator->SetInputImage(fieldImage);

  double x0vtk[nDims];
  pointCloud->GetPoint(pointIds.at(0),x0vtk);

  itk::ContinuousIndex<double,nDims> x0 = vtk2itk_VectorImage(x0vtk,fieldImage);
  interpField.push_back(fieldInterpolator->EvaluateAtContinuousIndex(x0));

  // Interpolate along line for each component of field vector from max to 0
  VectorImageType::PixelType interpPoint;
  interpPoint.SetSize(nDims);
  if(nPts > 1) {
    for(unsigned int p = 1; p < nPts; p++) {
      for(unsigned int d = 0; d < nDims; d++) {
	interpPoint[d] = interpField.at(0)[d] - ((p * interpField.at(0)[d])/(nPts-1));
      }
      interpField.push_back(interpPoint);
    }
  }
  

  return interpField;
}



// Function:  get n smallest values in an array
std::vector<std::vector<int> > nSmallest(std::vector<std::vector<double> > vect, unsigned int n) {
  unsigned int nLines = static_cast<int>(vect.size());
  std::vector<std::vector<int> > nSmallestInds(n);
  double inf = std::numeric_limits<double>::infinity();
  
  int i = 0;
  int minLineInd = 0;
  int minPtInd = 0;
  std::vector<int> minLine_minPt(2);

  while(i < n) {
    for(unsigned int p = 0; p < nLines; p++) {
      unsigned int nPtsOnLine = static_cast<int>(vect.at(p).size());
      for(unsigned int q = 0; q < nPtsOnLine; q++) {
	if(vect.at(p).at(q) < vect.at(minLineInd).at(minPtInd)) {
	  minLineInd = p;
	  minPtInd = q;
	}
      }
    }
    minLine_minPt.at(0) = minLineInd;
    minLine_minPt.at(1) = minPtInd;
    //std::cout << "Line: " << minLineInd << ", point: " << minPtInd << " --> " << vect.at(minLineInd).at(minPtInd) << std::endl;
    nSmallestInds.at(i) = minLine_minPt;

    //vect.at(minLineInd).at(minPtInd) = inf; // this is if just point
    // This is if you want to eliminate a whole line
    for(unsigned int q = 0; q < static_cast<int>(vect.at(minLineInd).size()); q++) {
      vect.at(minLineInd).at(q) = inf;
    }
    i++;
  }  
  return nSmallestInds;
}



// Function:  convert distances to weights normalized to their sum (closest distance = largest weight)
std::vector<double> reverseNormalize(std::vector<double> vect) {
  unsigned int nVals = static_cast<int>(vect.size());
  std::vector<double> weights;
  
  double sum1 = 0;
  for(unsigned int i = 0; i < nVals; i++){
    sum1 += vect.at(i);
  }
  double sum2 = 0;
  for(unsigned int i = 0; i < nVals; i++){
    sum2 += sum1 / vect.at(i);
  }
  for(unsigned int i = 0; i < nVals; i++){
    weights.push_back(sum1 / (vect.at(i) * sum2));
  }
  return weights;
}


// Function:  interpolate a point in the field image based on the linear field interpolation lines (see previous function)
VectorImageType::PixelType interpolateFieldPoint(VectorImageType::IndexType x0, vtkPolyData *cloud, std::vector<std::vector<int> > pointIds, std::vector<std::vector<VectorImageType::PixelType> > fieldValues, VectorImageType::Pointer fieldImage) {
  unsigned int nLines = static_cast<int>(fieldValues.size());
  std::vector<std::vector<double> > distance;
  itk::ContinuousIndex<double,nDims> x1;

  // Convert to vtk point
  double x0vtk[nDims];
  itk2vtk_VectorImage(x0,fieldImage,x0vtk);

  
  // Associate each field value with a distance
  for(unsigned int n = 0; n < nLines; n++) {
    unsigned int nPtsOnLine = static_cast<int>(fieldValues.at(n).size());
    std::vector<double> distanceOnLine;
    for(unsigned int p = 0; p < nPtsOnLine; p++) {
      int id = pointIds.at(n).at(p);
      double x1vtk[nDims];
      cloud->GetPoints()->GetPoint(id,x1vtk);

      double sumSqr = 0;
      for(unsigned int d = 0; d < nDims; d++) {
	sumSqr += pow(x1vtk[d] - x0vtk[d],2);
      }
      distanceOnLine.push_back(pow(sumSqr,0.5));
    }
    distance.push_back(distanceOnLine);
  }

  
  // Find which lines are the nearest for interpolation
  unsigned int nVals = 4;
  std::vector<std::vector<int> > minInds = nSmallest(distance,nVals);
  std::vector<double> minDists;
  for(unsigned int n = 0; n < nVals; n++) {
    minDists.push_back(distance.at(minInds.at(n).at(0)).at(minInds.at(n).at(1)));
    
    double x1vtk[nDims];
    cloud->GetPoint(pointIds.at(minInds.at(n).at(0)).at(minInds.at(n).at(1)),x1vtk);
  }

  
  // Calculate weights and apply to field values
  std::vector<double> weights = reverseNormalize(minDists);
  
  VectorImageType::PixelType fieldInterpValue;
  fieldInterpValue.SetSize(nDims);

  
  for(unsigned int d = 0; d < nDims; d++) {
    fieldInterpValue[d] = 0;
    for(unsigned int n = 0; n < nVals; n++) {
      fieldInterpValue[d] += weights.at(n) * fieldValues.at(minInds.at(n).at(0)).at(minInds.at(n).at(1))[d];
    }
  }

  return fieldInterpValue;
}




// Main
int main(int argc, char * argv[]) {

  if( argc < 6 ) {
    std::cerr << "Usage: " << std::endl;
    std::cerr << "distanceImageFileName roiImageFileName skullStripMaskFileName fieldImageFileName wmMaskFileName stepSize outputPointsFileName outputFieldFileName" << std::endl;
    return -1;
  }

  
  // Read in ze data
  std::string distanceImageFileName = argv[1];
  std::string roiImageFileName = argv[2];
  std::string skullStripMaskFileName = argv[3];
  std::string fieldImageFileName = argv[4];
  std::string wmMaskFileName = argv[5];
  const float stepSize = atof(argv[6]);
  std::string outputPointsFileName = argv[7];
  std::string outputFieldFileName = argv[8];
  
  DoubleImageType::Pointer distanceImage = readDoubleImage(distanceImageFileName);
  BinaryImageType::Pointer roiImage = readBinaryImage(roiImageFileName);
  BinaryImageType::Pointer skullStripMaskImage = readBinaryImage(skullStripMaskFileName);
  VectorImageType::Pointer fieldImage = readVectorImage(fieldImageFileName);
  BinaryImageType::Pointer wmMaskImage = readBinaryImage(wmMaskFileName);
  

  // Set up mask for initial gradient location (initialize where to iterate over)
  typedef itk::MinimumMaximumImageCalculator<DoubleImageType> MinimumMaximumImageCalculatorType;
  MinimumMaximumImageCalculatorType::Pointer minimumDistanceCalculator = MinimumMaximumImageCalculatorType::New();
  minimumDistanceCalculator->SetImage(distanceImage);
  minimumDistanceCalculator->ComputeMaximum();

  typedef itk::BinaryThresholdImageFilter<DoubleImageType,BinaryImageType> BinaryThresholdDistanceImageFilterType;
  BinaryThresholdDistanceImageFilterType::Pointer brainMaskFilter = BinaryThresholdDistanceImageFilterType::New();
  brainMaskFilter->SetInput(distanceImage);
  brainMaskFilter->SetOutsideValue(1);
  brainMaskFilter->SetInsideValue(0);
  brainMaskFilter->SetLowerThreshold(0.01);
  brainMaskFilter->SetUpperThreshold(minimumDistanceCalculator->GetMaximum());
  brainMaskFilter->Update();
  BinaryImageType::Pointer brainMaskImage = brainMaskFilter->GetOutput();

  typedef itk::BinaryContourImageFilter<BinaryImageType,BinaryImageType> BinaryContourImageFilterType;
  BinaryContourImageFilterType::Pointer binaryContourFilter = BinaryContourImageFilterType::New();
  binaryContourFilter->SetInput(brainMaskImage);
  binaryContourFilter->SetForegroundValue(1);
  binaryContourFilter->SetBackgroundValue(0);
  binaryContourFilter->Update();

  MultiplyImageFilterType::Pointer multiplyImageFilter = MultiplyImageFilterType::New();
  multiplyImageFilter->SetInput1(binaryContourFilter->GetOutput());
  multiplyImageFilter->SetInput2(roiImage);
  multiplyImageFilter->Update();
  BinaryImageType::Pointer outerMaskEdgeImage = multiplyImageFilter->GetOutput();
  BinaryImageType::Pointer outerMaskImage = multiplyImageFilter->GetOutput();

  
  // Get initial points and normals (search direction)
  vtkSmartPointer<vtkPolyData> edgeMaskMesh = convertMaskToPolyData(roiImage,outerMaskEdgeImage);  
  vtkSmartPointer<vtkFloatArray> isEdge = vtkFloatArray::SafeDownCast(edgeMaskMesh->GetPointData()->GetArray("Edge"));
  vtkSmartPointer<vtkDataArray> normals = edgeMaskMesh->GetPointData()->GetNormals();
  vtkSmartPointer<vtkPoints> startPoints = edgeMaskMesh->GetPoints();
  int nPts = startPoints->GetNumberOfPoints();

  // Initialize the points that you are going to have
  vtkSmartPointer<vtkPolyData> fieldCloud = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> cloudPoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> cloudLines = vtkSmartPointer<vtkCellArray>::New();
  std::vector<std::vector<int> > cloudIds;

  
  // Expand gradient mask to medial boundary around sulcus
  LinearInterpolateDoubleImageFunctionType::Pointer distanceInterpolator = LinearInterpolateDoubleImageFunctionType::New();
  distanceInterpolator->SetInputImage(distanceImage);
  LinearInterpolateIntImageFunctionType::Pointer skullStripInterpolator = LinearInterpolateIntImageFunctionType::New();
  skullStripInterpolator->SetInputImage(skullStripMaskImage);

  ImageIteratorType imageIterator(outerMaskEdgeImage,outerMaskEdgeImage->GetLargestPossibleRegion());
  imageIterator.GoToBegin();

  double lowerBound[nDims];
  lowerBound[0] = distanceImage->GetOrigin()[0];
  lowerBound[1] = distanceImage->GetOrigin()[1];
  lowerBound[2] = distanceImage->GetOrigin()[2];
  double upperBound[nDims];
  upperBound[0] = distanceImage->GetOrigin()[0] + distanceImage->GetLargestPossibleRegion().GetSize()[0]*distanceImage->GetSpacing()[0];
  upperBound[1] = distanceImage->GetOrigin()[1] + distanceImage->GetLargestPossibleRegion().GetSize()[1]*distanceImage->GetSpacing()[1];
  upperBound[2] = distanceImage->GetOrigin()[2] + distanceImage->GetLargestPossibleRegion().GetSize()[2]*distanceImage->GetSpacing()[2];

  //std::cout << lowerBound[0] << " " << lowerBound[1] << " " << lowerBound[2] << std::endl;
  //std::cout << upperBound[0] << " " << upperBound[1] << " " << upperBound[2] << std::endl;
  unsigned int boundaryCount;

  int pointId = 0;
  for(unsigned int p = 0; p < nPts; p++) {
    if(isEdge->GetValue(p) == 1) {
      std::vector<int> pointsInLine;
      boundaryCount = 0;
      
      // Initialize search line
      double x0vtk[nDims];
      edgeMaskMesh->GetPoint(p,x0vtk);
      cloudPoints->InsertNextPoint(x0vtk);
      pointsInLine.push_back(pointId);
      pointId++;
      
      bool insideSkull = true;
      bool atMedialBoundary = false;
      bool insideImage = true;

      // Travel along normal until conditions fail
      while(!atMedialBoundary && insideSkull) {
	double x1vtk[nDims];
	for(unsigned int d = 0; d < nDims; d++) {
	  x1vtk[d] = x0vtk[d] + stepSize*normals->GetComponent(p,d);
	  //std::cout << x0vtk[d] << " --> " << x1vtk[d] << " || " << std::flush;
	}
	//std::cout << std::endl;

	if(x1vtk[0] < lowerBound[0] || x0vtk[0] > upperBound[0] || x0vtk[1] < lowerBound[1] || x0vtk[1] > upperBound[1] || x0vtk[2] < lowerBound[2] || x0vtk[2] > upperBound[2]) {
          insideImage = false;
	  insideSkull = false;
	}

	if(insideImage) {
	  // Convert VTK to ITK continuous index
	  itk::ContinuousIndex<double,nDims> x0 = vtk2itk_BinaryImage(x0vtk,outerMaskEdgeImage);
	  itk::ContinuousIndex<double,nDims> x1 = vtk2itk_BinaryImage(x1vtk,outerMaskEdgeImage);

	  // Check conditions
	  DoubleImageType::PixelType Dx0 = distanceInterpolator->EvaluateAtContinuousIndex(x0);
	  DoubleImageType::PixelType Dx1 = distanceInterpolator->EvaluateAtContinuousIndex(x1);
	
	  if(Dx0 >= Dx1) {
	    atMedialBoundary = true;
	  }
	
	  if(skullStripInterpolator->EvaluateAtContinuousIndex(x1) < 1) {
	    insideSkull = false;
	  }
	
	  // Proceed?
	  if(!atMedialBoundary && insideSkull) {
	    cloudPoints->InsertNextPoint(x1vtk);
	    pointsInLine.push_back(pointId);
	  
	    vtkSmartPointer<vtkLine> cloudLine = vtkSmartPointer<vtkLine>::New();
	    cloudLine->GetPointIds()->SetId(0,pointId-1);
	    cloudLine->GetPointIds()->SetId(1,pointId);
	    cloudLines->InsertNextCell(cloudLine);
	    pointId++;

	    BinaryImageType::IndexType outerMaskIndex;
	    for(unsigned int d = 0; d < nDims; d++) {
	      outerMaskIndex[d] = (int)round(x1[d]);
	    }
	    outerMaskImage->SetPixel(outerMaskIndex,1);
	  }

	  for(unsigned int d = 0; d < nDims; d++) {
	    x0vtk[d] = x1vtk[d];
	  }
	}
	cloudIds.push_back(pointsInLine);
      }
    }
  }

  std::cout << "do we get here" << std::endl;
  
  fieldCloud->SetPoints(cloudPoints);
  fieldCloud->SetLines(cloudLines);
  fieldCloud->BuildLinks();


  /*
  vtkSmartPointer<vtkXMLPolyDataWriter> testPointsWriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  testPointsWriter->SetInputData(fieldCloud);
  testPointsWriter->SetFileName(outputPointsFileName.c_str());
  testPointsWriter->Write();
  */

  // Get final mask
  BinaryFillholeImageFilterType::Pointer fillHoleFilter = BinaryFillholeImageFilterType::New();
  fillHoleFilter->SetInput(outerMaskImage);
  fillHoleFilter->SetForegroundValue(1);
  fillHoleFilter->Update();
  
  BinaryBallStructuringElementType structuringElement;
  structuringElement.SetRadius(1);
  structuringElement.CreateStructuringElement();

  BinaryDilateImageFilterType::Pointer dilateFilter = BinaryDilateImageFilterType::New();
  dilateFilter->SetInput(fillHoleFilter->GetOutput());
  dilateFilter->SetKernel(structuringElement);
  dilateFilter->SetDilateValue(1);
  dilateFilter->Update();

  AddImageFilterType::Pointer addImageFilter = AddImageFilterType::New();
  addImageFilter->SetInput1(wmMaskImage);
  addImageFilter->SetInput2(roiImage);
  addImageFilter->Update();
  
  
  MultiplyImageFilterType::Pointer multiplyImageFilter2 = MultiplyImageFilterType::New();
  multiplyImageFilter2->SetInput1(fillHoleFilter->GetOutput());
  multiplyImageFilter2->SetInput2(addImageFilter->GetOutput());
  multiplyImageFilter2->Update();

  SubtractImageFilterType::Pointer subtractImageFilter = SubtractImageFilterType::New();
  subtractImageFilter->SetInput1(fillHoleFilter->GetOutput());
  subtractImageFilter->SetInput2(multiplyImageFilter2->GetOutput());
  subtractImageFilter->Update();


  MultiplyImageFilterType::Pointer multiplyImageFilter4 = MultiplyImageFilterType::New();
  multiplyImageFilter4->SetInput1(subtractImageFilter->GetOutput());
  multiplyImageFilter4->SetInput2(skullStripMaskImage);
  multiplyImageFilter4->Update();
  BinaryImageType::Pointer finalOuterMaskImage = multiplyImageFilter4->GetOutput();
 
  std::cout << " got the mask" << std::endl;
  
  // Now we will interpolate along gradient lines to "blur" the field
  unsigned int nLines = static_cast<int>(cloudIds.size());
  std::cout << nLines << std::endl;
  std::vector<std::vector<VectorImageType::PixelType> > fieldValues;

  for(unsigned int n = 0; n < nLines; n++) {
    //std::cout << n << std::flush;
    std::vector<int> lineIds;
    lineIds.resize(cloudIds.at(n).size());
    lineIds = cloudIds.at(n);
    fieldValues.push_back(blurField(fieldCloud->GetPoints(),lineIds,fieldImage));
  }
  std::cout << "got the values" << std::endl;
  
  // Create masked and "smoothed" displacement field
  typedef itk::MaskImageFilter<VectorImageType,BinaryImageType,VectorImageType> MaskImageFilterType;
  MaskImageFilterType::Pointer fieldMaskFilter = MaskImageFilterType::New();
  fieldMaskFilter->SetInput1(fieldImage);
  fieldMaskFilter->SetInput2(roiImage);
  fieldMaskFilter->Update();
  VectorImageType::Pointer smoothMaskedFieldImage = fieldMaskFilter->GetOutput();

  ImageIteratorType outerMaskIterator(finalOuterMaskImage,finalOuterMaskImage->GetLargestPossibleRegion());
  outerMaskIterator.GoToBegin();

  VectorImageType::SizeType smoothMaskedFieldImageSize = smoothMaskedFieldImage->GetLargestPossibleRegion().GetSize();
  int nVox = smoothMaskedFieldImageSize[0]*smoothMaskedFieldImageSize[1]*smoothMaskedFieldImageSize[2];
  int count = 0;
  int percentDone;
  bool printed = false;


  while(!outerMaskIterator.IsAtEnd()) {
    //std::cout << outerMaskIterator.GetIndex() << "-->" << outerMaskIterator.Get() << std::endl;
    count++;
    percentDone = 100*count/nVox;
    if(percentDone % 5 == 0 && !printed){
      std::cout << percentDone << "........" << std::flush;
      printed = true;
    } else if(!(percentDone % 5 == 0) && printed){
      printed = false;
    }
    
    if(outerMaskIterator.Get() == 1) {      
      VectorImageType::IndexType currentPt = outerMaskIterator.GetIndex();
      VectorImageType::PixelType currentVal = interpolateFieldPoint(currentPt,fieldCloud,cloudIds,fieldValues,fieldImage);
      smoothMaskedFieldImage->SetPixel(outerMaskIterator.GetIndex(),currentVal);
    }
    ++outerMaskIterator; 
  }
  std::cout << "field blurred" << std::endl;

  // Write out final field image
  VectorImageFileWriterType::Pointer fieldMaskBlurWriter = VectorImageFileWriterType::New();
  fieldMaskBlurWriter->SetInput(smoothMaskedFieldImage);
  fieldMaskBlurWriter->SetFileName(outputFieldFileName.c_str());
  fieldMaskBlurWriter->Update();
  std::cout << "done" << std::endl;
  
  
  return 0;
}
