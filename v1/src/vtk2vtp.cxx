#include <iostream>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>

int main(int argc, char * argv[]){

  vtkSmartPointer<vtkPolyDataReader> meshreader = vtkSmartPointer<vtkPolyDataReader>::New();
  meshreader->SetFileName(argv[1]);
  meshreader->Update();
  
  vtkSmartPointer<vtkXMLPolyDataWriter> meshwriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  meshwriter->SetFileName(argv[2]);
  meshwriter->SetInputData(meshreader->GetOutput());
  meshwriter->Update();

  return 0;
}
