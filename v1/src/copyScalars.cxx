#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkFloatArray.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkIdTypeArray.h>
#include <vtkIdList.h>


int main(int argc, char* argv[]){

  // Read data
  vtkSmartPointer<vtkXMLPolyDataReader> meshReader1 = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  meshReader1->SetFileName(argv[1]);
  meshReader1->Update();
  vtkSmartPointer<vtkPolyData> mesh1 = meshReader1->GetOutput();

  vtkSmartPointer<vtkXMLPolyDataReader> meshReader2 = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  meshReader2->SetFileName(argv[2]);
  meshReader2->Update();
  vtkSmartPointer<vtkPolyData> mesh2 = meshReader2->GetOutput();

  // Get labels
  vtkSmartPointer<vtkFloatArray> labels = vtkFloatArray::SafeDownCast(mesh2->GetPointData()->GetArray(argv[4]));
  mesh1->GetPointData()->AddArray(labels);  

  // Output new mesh
  vtkSmartPointer<vtkXMLPolyDataWriter> meshWriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  meshWriter->SetFileName(argv[3]);
  meshWriter->SetInputData(mesh1);
  meshWriter->Update();

  return 0;
}

