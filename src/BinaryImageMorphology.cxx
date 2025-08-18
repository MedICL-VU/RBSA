#include "BinaryImageMorphology.h"


UCharImageType::Pointer DilateImage
(UCharImageType::Pointer image, BinaryBallStructuringElementType kernel,
 UCharImageType::PixelType value)
{
  auto filter = BinaryDilateImageFilterType::New();
  filter->SetInput(image);
  filter->SetKernel(kernel);
  filter->SetDilateValue(value);
  filter->Update();

  return filter->GetOutput();
}


UCharImageType::Pointer ErodeImage
(UCharImageType::Pointer image, BinaryBallStructuringElementType kernel,
 UCharImageType::PixelType value)
{
  auto filter = BinaryErodeImageFilterType::New();
  filter->SetInput(image);
  filter->SetKernel(kernel);
  filter->SetErodeValue(value);
  filter->Update();

  return filter->GetOutput();
}


UCharImageType::Pointer DilateErodeCorrection(UCharImageType::Pointer image)
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

  

UCharImageType::Pointer LocalizedAtrophy
(UCharImageType::Pointer targetLabelImage, UCharImageType::Pointer referenceAdjacentLabelImage,
 unsigned int nIters)
{
  // Set up kernel
  BinaryBallStructuringElementType kernel;
  kernel.SetRadius(kernelRadius);
  kernel.CreateStructuringElement();

  // Do 1 voxel erosions for each iteration
  unsigned int iter = 0;
  UCharImageType::Pointer dilateMask = DilateImage(referenceAdjacentLabelImage, kernel);
  
  while(iter < nIters) {
    UCharImageType::Pointer erodeMask = ErodeImage(targetLabelImage, kernel);
    
    targetLabelImage = SubtractImages<UCharImageType>(targetLabelImage, erodeMask);
    targetLabelImage = MultiplyImages<UCharImageType>(targetLabelImage, dilateMask);
    targetLabelImage = AddImages<UCharImageType>(targetLabelImage, erodeMask);
    targetLabelImage = BinaryThresholdImage<UCharImageType>(targetLabelImage, 1, 1, 0, 1);

    iter++;
  }

  return targetLabelImage;
}


UCharImageType::Pointer ReplaceLabelInImage
(UCharImageType::Pointer inputImage, UCharImageType::Pointer origLabelMask,
 UCharImageType::Pointer atrophyLabelMask)
{
  /* 1. Get reverse label mask
     2. Multiply mask w/ input to remove origLabelMask from inputImage
     3. Add atrophyLabelMask to product  */
  
  UCharImageType::Pointer outputImage =
    BinaryThresholdImage<UCharImageType>(origLabelMask, 1, 1, 1, 0);
  outputImage = MultiplyImages<UCharImageType>(outputImage, inputImage);
  outputImage = AddImages<UCharImageType>(outputImage, atrophyLabelMask);

  return outputImage;
}
