#include "BinaryImageMorphology.h"


//----------------------------------------------------------------------------------------------------

TPointer<UCharImageType> DilateImage(TPointer<UCharImageType> image,
				     BinaryBallStructuringElementType kernel,
				     TPixel<UCharImageType> value)
{
  auto filter = BinaryDilateImageFilterType::New();
  filter->SetInput(image);
  filter->SetKernel(kernel);
  filter->SetDilateValue(value);
  filter->Update();

  TPointer<UCharImageType> output = filter->GetOutput();
  //output->DisconnectPipeline();
  return output;
}


TPointer<UCharImageType> ErodeImage(TPointer<UCharImageType> image,
				    BinaryBallStructuringElementType kernel,
				    TPixel<UCharImageType> value)
{
  auto filter = BinaryErodeImageFilterType::New();
  filter->SetInput(image);
  filter->SetKernel(kernel);
  filter->SetErodeValue(value);
  filter->Update();

  TPointer<UCharImageType> output = filter->GetOutput();
  //output->DisconnectPipeline();
  return output;
}


TPointer<UCharImageType> DilateErodeCorrection(TPointer<UCharImageType> image)
{
  // Set up kernel
  BinaryBallStructuringElementType kernel;
  kernel.SetRadius(kernelRadius);
  kernel.CreateStructuringElement();

  // Dilate and erode
  image = DilateImage(image, kernel);
  image = ErodeImage(image, kernel);
  
  return image;
}

  

TPointer<UCharImageType> LocalizedAtrophy(TPointer<UCharImageType> targetLabelImage,
					  TPointer<UCharImageType> referenceAdjacentLabelImage,
					  unsigned int nIters)
{
  // Set up kernel
  BinaryBallStructuringElementType kernel;
  kernel.SetRadius(kernelRadius);
  kernel.CreateStructuringElement();

  // Initialize masks
  TPointer<UCharImageType> dilateMask = DilateImage(referenceAdjacentLabelImage, kernel);
  TPointer<UCharImageType> output = DuplicateImage<UCharImageType>(targetLabelImage);
  
  // Do 1 voxel erosions for each iteration
  unsigned int iter = 0;
  
  for(unsigned int iter = 0; iter < nIters; iter++) {
    TPointer<UCharImageType> erodeMask = ErodeImage(output, kernel);
    SubtractImagesInPlace<UCharImageType>(output, erodeMask);
    MultiplyImagesInPlace<UCharImageType>(output, dilateMask);
    AddImagesInPlace<UCharImageType>(output, erodeMask);
    BinaryThresholdImageInPlace(output, 1, 1);
  }

  return output;
}


TPointer<UCharImageType> ReplaceLabelInImage(TPointer<UCharImageType> image,
					     TPointer<UCharImageType> oldLabel,
					     TPointer<UCharImageType> newLabel)
{
  TPointer<UCharImageType> output = BinaryThresholdImage<UCharImageType>(oldLabel, 1, 1, 1, 0);
  MultiplyImagesInPlace<UCharImageType>(output, image);
  AddImagesInPlace<UCharImageType>(output, newLabel);

  return output;
}
