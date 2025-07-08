#include <iostream>
#include <fstream>
#include <vtkMath.h>
#include <math.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkXMLPolyDataReader.h>



vtkSmartPointer<vtkPolyData> readXMLPolyData(std::string filename) {
  vtkSmartPointer<vtkXMLPolyDataReader> meshReader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  meshReader->SetFileName(filename.c_str());
  meshReader->Update();
  vtkSmartPointer<vtkPolyData> mesh = meshReader->GetOutput();
  return mesh;
}



int main(int argc, char * argv[]) {

  if(argc < 3) {
    std::cout << "Usage:  meshFileName outputTextFileName labelArrayName" << std::endl;
    return -1;
  }

  std::string meshFileName = argv[1];
  std::string outputTextFileName = argv[2];
  std::string labelArrayName = argv[3];

  vtkSmartPointer<vtkPolyData> mesh = readXMLPolyData(meshFileName);
  std::ofstream textFile(outputTextFileName.c_str());  
  vtkSmartPointer <vtkFloatArray> meshArray = vtkFloatArray::SafeDownCast(mesh->GetPointData()-> GetArray(labelArrayName.c_str()));
  unsigned int nPts = mesh->GetPoints()->GetNumberOfPoints();

  for(unsigned int n = 0; n < nPts; n++) {
    textFile << meshArray->GetValue(n) << "\n";
  }
  textFile.close();
  
  return 0;
}
