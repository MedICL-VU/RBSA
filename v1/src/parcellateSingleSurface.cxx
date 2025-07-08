#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkPointData.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkFloatArray.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkIdTypeArray.h>
#include <vtkIdList.h>
#include <vtkTriangleFilter.h>
#include <vtkCellArray.h>
#include <vtkSelectEnclosedPoints.h>
#include <vtkClipPolyData.h>
#include <vtkPolyDataConnectivityFilter.h>
#include <vtkFillHolesFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkCleanPolyData.h>
#include <vtkFeatureEdges.h>
#include <vtkStripper.h>
#include <vtkAppendPolyData.h>
#include <vtkCenterOfMass.h>
#include <vtkDelaunay3D.h>
#include <vtkDataSetSurfaceFilter.h>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkChangeInformationImageFilter.h>



const unsigned int nDims = 3;

typedef itk::Image<float,nDims> FloatImageType;
typedef itk::ImageFileReader<FloatImageType> FloatImageFileReaderType;
typedef itk::ImageFileWriter<FloatImageType> FloatImageFileWriterType;



FloatImageType::Pointer readFloatImage(std::string filename) {
  FloatImageFileReaderType::Pointer imageReader = FloatImageFileReaderType::New();
  imageReader->SetFileName(filename.c_str());
  imageReader->Update();

  FloatImageType::Pointer image = imageReader->GetOutput();
  return image;
}



vtkSmartPointer<vtkPolyData> readXMLPolyData(std::string filename) {
  vtkSmartPointer<vtkXMLPolyDataReader> meshreader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  meshreader->SetFileName(filename.c_str());
  meshreader->Update();
  vtkSmartPointer<vtkPolyData> mesh = meshreader->GetOutput();
  return mesh;
}



void writeXMLPolyData(vtkSmartPointer<vtkPolyData> mesh, std::string filename) {
  vtkSmartPointer<vtkXMLPolyDataWriter> meshWriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  meshWriter->SetInputData(mesh);
  meshWriter->SetFileName(filename.c_str());
  meshWriter->Write();
}



vtkSmartPointer<vtkFloatArray> AssignLabels(vtkSmartPointer<vtkPolyData> mesh, FloatImageType::Pointer image, std::string labelArrayName) {

  unsigned int nPts = mesh->GetNumberOfPoints();
  vtkSmartPointer<vtkFloatArray> labels = vtkSmartPointer<vtkFloatArray>::New() ;
  labels->SetNumberOfComponents(1);
  labels->SetNumberOfValues(nPts);
  labels->SetName(labelArrayName.c_str());

  for(unsigned int p = 0; p < nPts; p++) {
    double vtkPnt[3];
    mesh->GetPoint(p,vtkPnt);

    itk::ContinuousIndex<float,3> current;
    FloatImageType::IndexType currentIndex;
    current[0] = -vtkPnt[0];
    current[1] = -vtkPnt[1];
    current[2] = vtkPnt[2];

    bool inside = image->TransformPhysicalPointToIndex(current,currentIndex);
    if(inside) {
      FloatImageType::PixelType pixel = image->GetPixel(currentIndex);
      labels->SetValue(p,pixel);
    } else {
      std::cout << "not inside. huh. " << std::endl;
    }
  }

  return labels;
}



int main(int argc, char *argv[]) {

  if(argc < 3) {
    std::cout << "Usage: " << std::endl;
    std::cout << "inputMeshFileName inputLabelMapFileName outputMeshFileName labelArrayName" << std::endl;
    return -1;
  }

  std::string inputMeshFileName = argv[1];
  std::string labelFileName = argv[2];
  std::string outputMeshFileName = argv[3];
  std::string labelArrayName = argv[4];

  vtkSmartPointer<vtkPolyData> mesh = readXMLPolyData(inputMeshFileName);
  FloatImageType::Pointer image = readFloatImage(labelFileName);

  vtkSmartPointer<vtkFloatArray> labels = vtkSmartPointer<vtkFloatArray>::New();
  labels = AssignLabels(mesh,image,labelArrayName);
  mesh->GetPointData()->AddArray(labels);

  writeXMLPolyData(mesh,outputMeshFileName);

  return 0;
}
