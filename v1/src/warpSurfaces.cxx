#include <iostream>
#include <fstream>
#include <vtkMath.h>
#include <math.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkXMLPolyDataReader.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkChangeInformationImageFilter.h>
#include <vtkXMLPolyDataWriter.h>



const int nDims = 3;
typedef itk::Image<int,nDims> IntImageType;
typedef itk::ImageFileReader<IntImageType> IntImageFileReaderType;
typedef itk::ChangeInformationImageFilter<IntImageType> ChangeInformationIntImageFilterType;
typedef itk::Vector<double,nDims> VectorType;
typedef itk::Image<VectorType,nDims> VectorImageType;
typedef itk::ImageFileReader<VectorImageType> VectorImageFileReaderType;
typedef itk::ChangeInformationImageFilter<VectorImageType> ChangeInformationVectorImageFilterType;
typedef itk::LinearInterpolateImageFunction<VectorImageType> LinearInterpolateVectorImageFunctionType;



itk::ContinuousIndex<double,nDims> vtk2itk_VectorImage(double vtkPt[nDims], VectorImageType *refImage) {
  itk::ContinuousIndex<double,nDims> itkPt;
  for(unsigned int d = 0; d < nDims; d++) {
    itkPt[d] = (vtkPt[d] - refImage->GetOrigin()[d])/(refImage->GetSpacing()[d] * refImage->GetDirection()[d][d]);
  }
  itkPt[0] = -itkPt[0];
  itkPt[1] = -itkPt[1];
  return itkPt;
}


vtkSmartPointer<vtkPolyData> ReadPolyData(std::string filename) {
  vtkSmartPointer<vtkXMLPolyDataReader> polyDataReader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  polyDataReader->SetFileName(filename.c_str());
  polyDataReader->Update();

  return polyDataReader->GetOutput();
}


void WritePolyData(vtkSmartPointer<vtkPolyData> polyData, std::string filename) {
  vtkSmartPointer<vtkXMLPolyDataWriter> polyDataWriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  polyDataWriter->SetInputData(polyData);
  polyDataWriter->SetFileName(filename.c_str());
  polyDataWriter->Update();
}



VectorImageType::Pointer ReadVectorImage(std::string filename, int centerImage) {
  VectorImageFileReaderType::Pointer vectorImageReader = VectorImageFileReaderType::New();
  vectorImageReader->SetFileName(filename.c_str());
  vectorImageReader->Update();

  VectorImageType::Pointer vectorImage = VectorImageType::New();
  if(centerImage==1) {
    ChangeInformationVectorImageFilterType::Pointer filter = ChangeInformationVectorImageFilterType::New();
    filter->SetInput(vectorImageReader->GetOutput());
    filter->CenterImageOn();
    filter->Update();
    vectorImage = filter->GetOutput();
  } else {
    vectorImage = vectorImageReader->GetOutput();
  }
  
  return vectorImage;
}



IntImageType::Pointer ReadIntImage(std::string filename, int centerImage) {
  IntImageFileReaderType::Pointer intImageReader = IntImageFileReaderType::New();
  intImageReader->SetFileName(filename.c_str());
  intImageReader->Update();
  
  IntImageType::Pointer intImage = IntImageType::New();
  if(centerImage==1) {
    ChangeInformationIntImageFilterType::Pointer filter = ChangeInformationIntImageFilterType::New();
    filter->SetInput(intImageReader->GetOutput());
    filter->CenterImageOn();
    filter->Update();
    intImage = filter->GetOutput();
  } else {
    intImage = intImageReader->GetOutput();
  }

  return intImage;
}


  
int main(int argc, char* argv[]) {

  // Verify command line arguments
  if( argc < 2 ) {
      std::cerr << "Usage: " << std::endl;
      std::cerr << argv[0] << " inputMesh.vtp inputDisplacementField.nii.gz fieldMask.nii.gz outputWarpedMesh.vtp centerImage(1 for FS surfaces, 0 for LOGB) checkMask(0 or 1)" << std::endl;
      return -1;
    }
  

  // Read in data
  std::string inputMeshFileName = argv[1];
  std::string dispFieldFileName = argv[2];
  std::string dispFieldMaskFileName = argv[3];
  std::string outputMeshFileName = argv[4];
  int centerImage = atoi(argv[5]);
  int checkMask = atoi(argv[6]);

  vtkSmartPointer<vtkPolyData> mesh = ReadPolyData(inputMeshFileName);
  int nPts = mesh->GetPoints()->GetNumberOfPoints();

  VectorImageType::Pointer dispField = ReadVectorImage(dispFieldFileName,centerImage);
  IntImageType::Pointer dispFieldMask = ReadIntImage(dispFieldMaskFileName,centerImage);
  

  // Warp
  unsigned int count = 0;
  double x0_vtk[nDims], x1_vtk[nDims];
  itk::ContinuousIndex<double,nDims> x0_itk;
  itk::ContinuousIndex<double,nDims> x0_itk_otherSpace;
  VectorType disp;
  bool inside;
  IntImageType::IndexType maskIndex;
  IntImageType::PixelType maskVal;

  vtkSmartPointer<vtkFloatArray> warpLabels = vtkSmartPointer<vtkFloatArray>::New();
  warpLabels->SetNumberOfComponents(1);
  warpLabels->SetNumberOfValues(nPts);
  warpLabels->SetName("Is-Warped");

  LinearInterpolateVectorImageFunctionType::Pointer interpolator = LinearInterpolateVectorImageFunctionType::New();
  interpolator->SetInputImage(dispField);

  
  for(unsigned int p = 0; p < nPts; p++) {
    mesh->GetPoint(p,x0_vtk);
    x0_itk = vtk2itk_VectorImage(x0_vtk,dispField);
    disp = interpolator->EvaluateAtContinuousIndex(x0_itk);
    /*
    x1_vtk[0] = x0_vtk[0] - disp[0];
    x1_vtk[1] = x0_vtk[1] - disp[1];
    x1_vtk[2] = x0_vtk[2] + disp[2];
    mesh->GetPoints()->SetPoint(p,x1_vtk);
    */
    x0_itk_otherSpace[0] = -x0_itk[0];
    x0_itk_otherSpace[1] = -x0_itk[1];
    x0_itk_otherSpace[2] = x0_itk[2];
    inside = dispField->TransformPhysicalPointToIndex(x0_itk_otherSpace,maskIndex);
    maskVal = dispFieldMask->GetPixel(maskIndex);
    if(checkMask) {
      if(maskVal == 1) {
	//std::cout << maskVal << std::endl;
	disp = interpolator->EvaluateAtContinuousIndex(x0_itk);
	x1_vtk[0] = x0_vtk[0] - disp[0];
	x1_vtk[1] = x0_vtk[1] - disp[1];
	x1_vtk[2] = x0_vtk[2] + disp[2];
	mesh->GetPoints()->SetPoint(p,x1_vtk);
	warpLabels->SetValue(p,1);
      } else {
	warpLabels->SetValue(p,0);
      }
    }
    else {
      disp = interpolator->EvaluateAtContinuousIndex(x0_itk);
      x1_vtk[0] = x0_vtk[0] - disp[0];
      x1_vtk[1] = x0_vtk[1] - disp[1];
      x1_vtk[2] = x0_vtk[2] + disp[2];
      mesh->GetPoints()->SetPoint(p,x1_vtk);
      warpLabels->SetValue(p,1);
    }
  }


  // Output warped mesh
  mesh->GetPointData()->AddArray(warpLabels);
  mesh->BuildLinks();
  WritePolyData(mesh,outputMeshFileName);
  
  return 0;
}
