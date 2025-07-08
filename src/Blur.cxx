#include "Blur.h"
#include "utils.h"

UCharImageType::Pointer BinaryContourImage(UCharImageType::Pointer image)
{
  auto filter = BinaryContourImageFilterType::New();
  filter->SetInput(image);
  filter->SetForegroundValue(0);
  filter->SetBackgroundValue(1);
  filter->Update();

  return filter->GetOutput();
}


float GetMaximumImageValue(FloatImageType::Pointer image)
{
  auto filter = MinimumMaximumImageCalculatorType::New();
  filter->SetImage(image);
  filter->Compute();

  return filter->GetMaximum();
}


vtkSmartPointer<vtkPolyData> BinaryITKImageToVTKMesh
(UCharImageType::Pointer itkTargetImage, UCharImageType::Pointer edgeMap)
{
  // Convert itkTargetImage to vtkTargetImage (same info except direction matrix is identity)
  const auto& region = itkTargetImage->GetLargestPossibleRegion();
  const auto& size = region.GetSize();
  const auto& spacing = itkTargetImage->GetSpacing();
  const auto& origin = itkTargetImage->GetOrigin();
  const auto& direction = itkTargetImage->GetDirection();

  auto vtkTargetImage = vtkSmartPointer<vtkImageData>::New();
  vtkTargetImage->SetDimensions(size[0], size[1], size[2]);
  vtkTargetImage->SetSpacing(spacing[0], spacing[1], spacing[2]);
  vtkTargetImage->SetOrigin(origin[0], origin[1], origin[2]);
  vtkTargetImage->SetExtent(0, size[0] - 1, 0, size[1] - 0, 0, size[2] - 1);
  vtkTargetImage->AllocateScalars(VTK_UNSIGNED_CHAR, 0);
  
  for(unsigned int k = 0; k < size[2]; k++) {
    for(unsigned int j = 0; j < size[1]; j++) {
      for(unsigned int i = 0; i < size[0]; i++) {
	const UCharImageType::IndexType& idx = {i, j, k};
	const UCharImageType::PixelType& value = itkTargetImage->GetPixel(idx);
	unsigned char* voxel =
	  static_cast<unsigned char*>(vtkTargetImage->GetScalarPointer(i, j, k));
	*voxel = value;
      }
    }
  }
  vtkTargetImage->Modified();


  // Generate mesh from vtkImage and w/ itkTargetImage direction
  vtkNew<vtkMarchingCubes> marchingCubesFilter;
  marchingCubesFilter->SetInputData(vtkTargetImage);
  marchingCubesFilter->ComputeScalarsOff();
  marchingCubesFilter->ComputeNormalsOn();
  marchingCubesFilter->ComputeGradientsOn();
  marchingCubesFilter->SetValue(0, 0.5);

  vtkNew<vtkWindowedSincPolyDataFilter> smoothingFilter;
  smoothingFilter->SetInputConnection(marchingCubesFilter->GetOutputPort());
  smoothingFilter->BoundarySmoothingOff();
  smoothingFilter->SetPassBand(0.1);
  smoothingFilter->SetFeatureAngle(60.0);

  vtkNew<vtkDecimatePro> decimateFilter;
  decimateFilter->SetInputConnection(smoothingFilter->GetOutputPort());
  decimateFilter->SetTargetReduction(0.5);
  decimateFilter->PreserveTopologyOn();
  decimateFilter->Update();
  
  vtkSmartPointer<vtkPolyData> mesh = decimateFilter->GetOutput();

  for(unsigned int p = 0; p < mesh->GetNumberOfPoints(); p++) {
    double vtkPoint[nDims], itkPoint[nDims], vtkPointNew[nDims];
    mesh->GetPoint(p, vtkPoint);

    for(unsigned int d = 0; d < nDims; d++) {
      vtkPoint[d] -= origin[d];
    }

    for(unsigned int i = 0; i < nDims; i++) {
      itkPoint[i] = 0;
      for(unsigned int j = 0; j < nDims; j++) {
	itkPoint[i] += direction[i][j] * vtkPoint[j];
      }
    }

    for(unsigned int d = 0; d < nDims; d++) {
      vtkPointNew[d] = itkPoint[d] + origin[d];
    }
    mesh->GetPoints()->SetPoint(p, vtkPointNew);
  }
  mesh->BuildLinks();
  
  // Determine which vertices of mesh lie on the CSF boundary
  BinaryBallStructuringElementType kernel;
  kernel.SetRadius(kernelRadius);
  kernel.CreateStructuringElement();

  UCharImageType::Pointer mask1 = DilateImage(edgeMap, kernel);
  mask1 = MultiplyImages<UCharImageType, UCharImageType, UCharImageType>(mask1, itkTargetImage);
  UCharImageType::Pointer mask2 = DilateImage(itkTargetImage, kernel);
  mask2 = MultiplyImages<UCharImageType, UCharImageType, UCharImageType>(mask2, edgeMap);
  UCharImageType::Pointer labelEdgeMask = AddImages<UCharImageType, UCharImageType, UCharImageType>
    (mask1, mask2);

  WriteImage<UCharImageType>
    (labelEdgeMask, "/space/azura/1/users/kl021/Code/RBSA/testing/labelEdgeMask.nii.gz");
  
  // Add labels to mesh denoted if edge
  vtkNew<vtkFloatArray> isEdge;
  isEdge->SetNumberOfComponents(1);
  isEdge->SetNumberOfValues(mesh->GetNumberOfPoints());
  isEdge->SetName("Is edge");
  isEdge->Fill(0.0);
  
  itk::ContinuousIndex<double,nDims> x0;
  UCharImageType::IndexType idx;
  
  for(unsigned int p = 0; p < mesh->GetNumberOfPoints(); p++) {
    // Get coords as itkContinuousIndex
    double p0[nDims];
    mesh->GetPoint(p, p0);
    for(unsigned int d = 0; d < nDims; d++) {
      x0[d] = p0[d];
    }

    // Get value from mask image
    labelEdgeMask->TransformPhysicalPointToIndex(x0, idx);
    if(labelEdgeMask->GetLargestPossibleRegion().IsInside(idx)) {
      const UCharImageType::PixelType& x = labelEdgeMask->GetPixel(idx);
      isEdge->SetValue(p, x);
    }
    else {
      std::cerr << "huh, index is not inside\n";
    }
  }
  mesh->GetPointData()->AddArray(isEdge);
  mesh->BuildLinks();

  /*
  vtkNew<vtkXMLPolyDataWriter> meshWriter;
  meshWriter->SetInputData(mesh);
  meshWriter->SetFileName("/space/azura/1/users/kl021/Code/RBSA/testing/mesh.vtp");
  meshWriter->Update();
  */
  return mesh;
}
			       

FloatImageType::Pointer SignedDistanceTransform(UCharImageType::Pointer image) {
  auto filter = SignedMaurerDistanceMapImageFilterType::New();
  filter->SetInput(image);
  filter->Update();

  return filter->GetOutput();
}


UCharImageType::Pointer GetBlurMask
(UCharImageType::Pointer labelMask, UCharImageType::Pointer brainMask,
 UCharImageType::Pointer skullStripMask, float stepSize)
{
  // Extract edge of labelMask that is bordering CSF
  UCharImageType::Pointer brainMaskEdgeMap = BinaryContourImage(brainMask);
  brainMaskEdgeMap = BinaryThresholdImage<UCharImageType, UCharImageType>(brainMaskEdgeMap, 0, 0);

  WriteImage<UCharImageType>
    (brainMask, "/space/azura/1/users/kl021/Code/RBSA/testing/brainMask.nii.gz");
  WriteImage<UCharImageType>
    (brainMaskEdgeMap, "/space/azura/1/users/kl021/Code/RBSA/testing/brainMaskEdgeMap.nii.gz");
  
  // Convert edge map to vtkPolyData (to get a set of points and normals on edge)
  vtkSmartPointer<vtkPolyData> edge = BinaryITKImageToVTKMesh(labelMask, brainMaskEdgeMap);
  
  



  // Get distance map of brainMask
  //FloatImageType::Pointer distanceMap = SignedDistanceTransform(brainMask);

  
  
  
}
