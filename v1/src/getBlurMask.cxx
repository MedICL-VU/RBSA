#include <iostream>
#include <string>
#include <fstream>
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



// Initialize
const unsigned int nDims = 3;
typedef itk::Image<double,nDims> DistanceImageType;
typedef itk::VectorImage<double,nDims> VectorImageType;
typedef itk::Image<int,nDims> BinaryImageType;

typedef itk::ImageFileReader<DistanceImageType> DistanceImageFileReaderType;
typedef itk::ImageFileReader<VectorImageType> VectorImageFileReaderType;
typedef itk::ImageFileReader<BinaryImageType> BinaryImageFileReaderType;

typedef itk::ImageRegionIteratorWithIndex<BinaryImageType> ImageIteratorType;

typedef itk::LinearInterpolateImageFunction<DistanceImageType,double> LinearInterpolateFloatImageFunctionType;
typedef itk::LinearInterpolateImageFunction<VectorImageType,double> LinearInterpolateVectorImageFunctionType;
typedef itk::LinearInterpolateImageFunction<BinaryImageType,double> LinearInterpolateIntImageFunctionType;

typedef itk::SubtractImageFilter<BinaryImageType,BinaryImageType,BinaryImageType> SubtractImageFilterType;
typedef itk::AddImageFilter<BinaryImageType,BinaryImageType,BinaryImageType> AddImageFilterType;
typedef itk::MultiplyImageFilter<BinaryImageType,BinaryImageType> MultiplyImageFilterType;
typedef itk::BinaryThresholdImageFilter<BinaryImageType,BinaryImageType> BinaryThresholdImageFilterType;

typedef itk::ImageFileWriter<BinaryImageType> BinaryImageFileWriterType;



void writeBinaryImage(BinaryImageType::Pointer image, std::string filename){

  BinaryImageFileWriterType::Pointer imageWriter = BinaryImageFileWriterType::New();
  imageWriter->SetInput(image);
  imageWriter->SetFileName(filename);
  imageWriter->Update();
}



// Function:  print array
void printArray(double array[nDims]){
  std::cout << "[" << std::flush;
  for(unsigned int d = 0; d < nDims; d++){
    std::cout << " " << array[d] << std::flush;
  }
  std::cout << " ]" << std::flush;
}


// Function:  print array (int)
void printArrayItkIndex(BinaryImageType::IndexType array){
  std::cout << "[" << std::flush;
  for(unsigned int d = 0; d < nDims; d++){
    std::cout << " " << array[d] << std::flush;
  }
  std::cout << " ]" << std::flush;
}


// Function:  make a polydata sphere at a specified point
void makeSphere(double center[nDims], std::string filename) {
  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetCenter(center);
  sphere->SetRadius(0.1);

  vtkSmartPointer<vtkXMLPolyDataWriter> sphereWriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  sphereWriter->SetInputConnection(sphere->GetOutputPort());
  std::string path = "/home/larsonke/Code/Synthetic-Atrophy/Data/Spheres/" + filename + ".vtp";
  sphereWriter->SetFileName(path.c_str() );
  sphereWriter->Write();
}



// Function: convert VTK point to ITK point (templated with binary image)
itk::ContinuousIndex<double,nDims> vtk2itk_BinaryImage(double vtkPt[nDims], BinaryImageType *refImage) {
  itk::ContinuousIndex<double,nDims> itkPt;
  for(unsigned int d = 0; d < nDims; d++) {
    //itkPt[d] = (vtkPt[d] - refImage->GetOrigin()[d]);
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
    itkPt[d] = (vtkPt[d] - refImage->GetOrigin()[d])*(refImage->GetSpacing()[d] / refImage->GetDirection()[d][d]);
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

  return mesh;
}



// Main
int main(int argc, char * argv[]) {

  if( argc < 5 ) {
    std::cerr << "Usage: " << std::endl;
    std::cerr << "inputMask distanceImage skullStripMask outputMask textFilegradientStepSize" << std::endl;
  }

  // Read in ze data
  const float stepSize = atof(argv[5]);
  
  BinaryImageFileReaderType::Pointer roiReader = BinaryImageFileReaderType::New(); // original ROI (pre-erosion)
  roiReader->SetFileName(argv[1]);
  roiReader->Update();
  BinaryImageType::Pointer roiImage = roiReader->GetOutput();
  
  DistanceImageFileReaderType::Pointer distanceReader = DistanceImageFileReaderType::New(); // SDT of brain mask
  distanceReader->SetFileName(argv[2]);
  distanceReader->Update();
  DistanceImageType::Pointer distanceImage = distanceReader->GetOutput();

  BinaryImageFileReaderType::Pointer skullStripMaskReader = BinaryImageFileReaderType::New(); // inside brain
  skullStripMaskReader->SetFileName(argv[3]);
  skullStripMaskReader->Update();
  BinaryImageType::Pointer skullStripMaskImage = skullStripMaskReader->GetOutput();

  // Set up mask for initial gradient location (initialize where to iterate over)
  typedef itk::MinimumMaximumImageCalculator<DistanceImageType> MinimumMaximumImageCalculatorType;
  MinimumMaximumImageCalculatorType::Pointer minimumDistanceCalculator = MinimumMaximumImageCalculatorType::New();
  minimumDistanceCalculator->SetImage(distanceImage);
  minimumDistanceCalculator->ComputeMo();

  typedef itk::BinaryThresholdImageFilter<DistanceImageType,BinaryImageType> BinaryThresholdDistanceImageFilterType;
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
  BinaryImageType::Pointer outerMaskImage = multiplyImageFilter->GetOutput();


  // Get initial points and normals (search direction)
  vtkSmartPointer<vtkPolyData> edgeMaskMesh = convertMaskToPolyData(roiImage,outerMaskImage);  
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
  LinearInterpolateFloatImageFunctionType::Pointer distanceInterpolator = LinearInterpolateFloatImageFunctionType::New();
  distanceInterpolator->SetInputImage(distanceImage);
  LinearInterpolateIntImageFunctionType::Pointer skullStripInterpolator = LinearInterpolateIntImageFunctionType::New();
  skullStripInterpolator->SetInputImage(skullStripMaskImage);

  double lowerBound[nDims];
  lowerBound[0] = distanceImage->GetOrigin()[0];
  lowerBound[1] = distanceImage->GetOrigin()[1];
  lowerBound[2] = distanceImage->GetOrigin()[2];
  double upperBound[nDims];
  upperBound[0] = distanceImage->GetOrigin()[0] + distanceImage->GetLargestPossibleRegion().GetSize()[0];
  upperBound[1] = distanceImage->GetOrigin()[1] + distanceImage->GetLargestPossibleRegion().GetSize()[1];
  upperBound[2] = distanceImage->GetOrigin()[2] + distanceImage->GetLargestPossibleRegion().GetSize()[2];

  double x0vtk[nDims];
  double x1vtk[nDims];
  double x0vtk_shifted[nDims];
  double x1vtk_shifted[nDims];
  DistanceImageType::IndexType testIndex;

  int pointId = 0;
  for(unsigned int p = 0; p < nPts; p++) {
    if(isEdge->GetValue(p) == 1) {
      std::vector<int> pointsInLine;
      //std::cout << p << std::endl;
      // Initialize search line
      edgeMaskMesh->GetPoint(p,x0vtk);
      cloudPoints->InsertNextPoint(x0vtk);
      pointsInLine.push_back(pointId);
      pointId++;
      
      bool insideSkull = true;
      bool atMedialBoundary = false;
      bool insideImage = true;
      bool insideImage2;

      x0vtk_shifted[0] = x0vtk[0] + 2*outerMaskImage->GetOrigin()[0];
      x0vtk_shifted[1] = x0vtk[1] + 2*outerMaskImage->GetOrigin()[1];
      x0vtk_shifted[2] = x0vtk[2];
      
      // Travel along normal until conditions fail
      while(!atMedialBoundary && insideSkull && insideImage) {
        for(unsigned int d = 0; d < nDims; d++) {
          x1vtk_shifted[d] = x0vtk_shifted[d] + stepSize*normals->GetComponent(p,d);
	  x1vtk[d] = x0vtk[d] + stepSize*normals->GetComponent(p,d);
        }

	// Make sure the pixel is actually contained in the image
	if(x0vtk_shifted[0] < lowerBound[0] || x0vtk_shifted[0] > upperBound[0] \
	   || x0vtk_shifted[1] < lowerBound[1] || x0vtk_shifted[1] > upperBound[1] \
	   || x0vtk_shifted[2] < lowerBound[2] || x0vtk_shifted[2] > upperBound[2]) {
	  insideImage = false;
	}
	
	if(insideImage) {
	  // Convert VTK to ITK continuous index
	  itk::ContinuousIndex<double,nDims> x0 = vtk2itk_BinaryImage(x0vtk_shifted,outerMaskImage);
          itk::ContinuousIndex<double,nDims> x1 = vtk2itk_BinaryImage(x1vtk_shifted,outerMaskImage);

	  // Check conditions
	  DistanceImageType::PixelType Dx0 = distanceInterpolator->EvaluateAtContinuousIndex(x0);
          DistanceImageType::PixelType Dx1 = distanceInterpolator->EvaluateAtContinuousIndex(x1);
	  insideImage2 = distanceImage->TransformPhysicalPointToIndex(x1,testIndex);

	  if(Dx0 >= Dx1) {
            atMedialBoundary = true;
          }
          if(skullStripInterpolator->EvaluateAtContinuousIndex(x1) < 1) {
            insideSkull = false;
          }

	  // Proceed?
	  if(!atMedialBoundary && insideSkull ) {
	    
	    BinaryImageType::IndexType outerMaskIndex;
            for(unsigned int d = 0; d < nDims; d++) {
              outerMaskIndex[d] = (int)round(x1[d]);
            }
            outerMaskImage->SetPixel(outerMaskIndex,1);

	    cloudPoints->InsertNextPoint(x1vtk);
            pointsInLine.push_back(pointId);

            vtkSmartPointer<vtkLine> cloudLine = vtkSmartPointer<vtkLine>::New();
            cloudLine->GetPointIds()->SetId(0,pointId-1);
            cloudLine->GetPointIds()->SetId(1,pointId);
            cloudLines->InsertNextCell(cloudLine);
            pointId++;
          }

	  
	  for(unsigned int d = 0; d < nDims; d++) {
	    x0vtk_shifted[d] = x1vtk_shifted[d];
            x0vtk[d] = x1vtk[d];
	  }
        }
      }
      cloudIds.push_back(pointsInLine);
    }
  }

  fieldCloud->SetPoints(cloudPoints);
  fieldCloud->SetLines(cloudLines);
  fieldCloud->BuildLinks();


  // Get final mask
  typedef itk::BinaryFillholeImageFilter<BinaryImageType> BinaryFillholeImageFilterType;
  BinaryFillholeImageFilterType::Pointer fillHoleFilter = BinaryFillholeImageFilterType::New();
  fillHoleFilter->SetInput(outerMaskImage);
  fillHoleFilter->SetForegroundValue(1);
  fillHoleFilter->Update();


  MultiplyImageFilterType::Pointer findMaskOverlapFilter = MultiplyImageFilterType::New();
  findMaskOverlapFilter->SetInput1(fillHoleFilter->GetOutput());
  findMaskOverlapFilter->SetInput2(roiImage);
  findMaskOverlapFilter->Update();
  
  SubtractImageFilterType::Pointer subtractImageFilter = SubtractImageFilterType::New();
  subtractImageFilter->SetInput1(fillHoleFilter->GetOutput());
  subtractImageFilter->SetInput2(findMaskOverlapFilter->GetOutput());
  subtractImageFilter->Update();

  
  AddImageFilterType::Pointer addMasksFilter = AddImageFilterType::New();
  addMasksFilter->SetInput1(subtractImageFilter->GetOutput());
  addMasksFilter->SetInput2(roiImage);
  addMasksFilter->Update();
  BinaryImageType::Pointer finalOuterMaskImage = addMasksFilter->GetOutput();

  // Write out mask
  writeBinaryImage(finalOuterMaskImage,argv[4]);  

  return 0;
}
