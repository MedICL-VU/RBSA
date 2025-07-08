#include <iostream>
#include <string>
#include <math.h>
#include <cmath>

#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellLocator.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkFloatArray.h"
#include "vtkDataArray.h"
#include "vtkPolyDataNormals.h"
#include "vtkDistancePolyDataFilter.h"
#include "vtkTriangleFilter.h"



// Initialize
const unsigned int nDims = 3;


// Function:  read polydata
vtkSmartPointer<vtkPolyData> ReadPolyData(std::string filename) {
  vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  reader->SetFileName(filename.c_str());
  reader->Update();

  // Make sure it's triangulated
  vtkSmartPointer<vtkTriangleFilter> triangleFilter = vtkSmartPointer<vtkTriangleFilter>::New();
  triangleFilter->SetInputData(reader->GetOutput());
  triangleFilter->Update();
  
  return triangleFilter->GetOutput();
}


// Function:  write polydata
void WritePolyData(vtkSmartPointer<vtkPolyData> mesh, std::string filename) {
  vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  writer->SetInputData(mesh);
  writer->SetFileName(filename.c_str());
  writer->Write();
}



// Main
int main(int argc, char * argv[]) {

  if( argc < 3 ) {
    std::cerr << "Usage: " << std::endl;
    std::cerr << "startSurface endSurface OutputSurface" << std::endl;
    return -1;
  }

  
  // Read in ze data
  std::string inputPolyDataFileName_start = argv[1];
  std::string inputPolyDataFileName_end = argv[2];
  std::string outputPolyDataFileName = argv[3];
  
  vtkSmartPointer<vtkPolyData> startMesh = ReadPolyData(inputPolyDataFileName_start);
  vtkSmartPointer<vtkPolyData> endMesh = ReadPolyData(inputPolyDataFileName_end);
  unsigned int nPts = startMesh->GetPoints()->GetNumberOfPoints();

  
  
  // Calculate signed distance
  vtkSmartPointer<vtkDistancePolyDataFilter> distanceFilter = vtkSmartPointer<vtkDistancePolyDataFilter>::New();
  distanceFilter->SetInputData(0,startMesh);
  distanceFilter->SetInputData(1,endMesh);
  distanceFilter->SignedDistanceOn();
  distanceFilter->Update();


  // Add as a float array
  vtkSmartPointer<vtkDoubleArray> buffer = dynamic_cast<vtkDoubleArray*>(distanceFilter->GetOutput()->GetPointData()->GetArray("Distance"));
  vtkSmartPointer<vtkFloatArray> surfaceDistance = vtkSmartPointer<vtkFloatArray>::New();
  surfaceDistance->SetNumberOfComponents(1);
  surfaceDistance->SetNumberOfTuples(nPts);
  surfaceDistance->SetName("Surface-Distance");

  for(unsigned int p = 0; p < nPts; p++) {
    double dist = buffer->GetValue(p);
    surfaceDistance->SetValue(p,static_cast<float>(dist));
  }

  vtkSmartPointer<vtkPolyData> outputMesh = vtkSmartPointer<vtkPolyData>::New();
  outputMesh->DeepCopy(startMesh);
  outputMesh->GetPointData()->AddArray(surfaceDistance);
  outputMesh->BuildLinks();
  WritePolyData(outputMesh,outputPolyDataFileName);     


  return 0;
}
