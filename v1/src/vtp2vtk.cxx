#include <iostream>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyDataWriter.h>

int main(int argc, char * argv[]){

  vtkSmartPointer<vtkXMLPolyDataReader> meshreader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  meshreader->SetFileName(argv[1]);
  meshreader->Update();
  
  vtkSmartPointer<vtkPolyDataWriter> meshwriter = vtkSmartPointer<vtkPolyDataWriter>::New();
  meshwriter->SetFileName(argv[2]);
  meshwriter->SetInputData(meshreader->GetOutput());
  meshwriter->Update();

  return 0;
}
