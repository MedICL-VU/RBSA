#include "RBSA.h"

//--------------------------------------------------------------------------------------------------

RBSA::RBSA(){}


void RBSA::SetInputParcellation(TPointer<IntImageType> parc)
{
  // Parcellation
  this->m_parc = parc;

  // Brain mask
  this->m_brainMask = BinaryThresholdImage<IntImageType>(this->m_parc);
  
  // Composite outputs
  this->m_countImage = InitializeZeroFilledImage<IntImageType, IntImageType>(parc);
  this->m_warp = InitializeZeroFilledImage<IntImageType, VectorImageType>(parc);
}

void RBSA::SetSkullStripMask(TPointer<UCharImageType> mask)
{
  auto filter = BinaryFillHolesFilterType::New();
  filter->SetInput(mask);
  filter->SetForegroundValue(1);
  filter->Update();

  this->m_skullStripMask = filter->GetOutput();
  this->m_skullStripMask->DisconnectPipeline();
}



void RBSA::SetTargetLabels(const std::vector<TPixel<IntImageType>>& inputLabels)
{
  this->m_targetLabels.reserve(inputLabels.size());

  // Check if labels exist in this->m_parc
  for(const auto& label : inputLabels) {
    if(IsLabelInImage<IntImageType>(this->m_parc, label)) {
      this->m_targetLabels.emplace_back(label);
    }
    else {
      std::cerr << label << " does not exist within the input parcellation :(" << std::endl;
    }
  }
}


void RBSA::SetWMLabels(const std::vector<TPixel<IntImageType>>& inputLabels)
{
  std::vector<int> wmLabels;
  this->m_wmMask = InitializeZeroFilledImage<IntImageType, UCharImageType>(this->m_parc);
  
  // Check if labels exist in this->m_parc
  for(const auto& label : inputLabels) {
    if(IsLabelInImage<IntImageType>(this->m_parc, label)) {
      wmLabels.emplace_back(label);
    }
    else {
      std::cerr << label << " does not exist within the input parcellation :(" << std::endl;
    }
  }

  // Create mask with only WM labels
  for(const auto& label : wmLabels) {
    TPointer<UCharImageType> temp = BinaryThresholdImage<IntImageType>(this->m_parc, label, label);
    AddImagesInPlace<UCharImageType>(this->m_wmMask, temp);
  }
  
  this->m_wmMask->DisconnectPipeline();
}


void RBSA::GenerateTransformForLabel(unsigned int label)
{
  // Check if labels exist in this->m_parc
  if(!IsLabelInImage<IntImageType>(this->m_parc, label)) {
    std::cerr << label << " does not exist within the input parcellation :(" << std::endl;
    return;
  }

  // Create original label mask
  auto origLabelMask = BinaryThresholdImage<IntImageType>(this->m_parc, label, label);

  // Get the blur mask data
  BlurMaskGenerator Blur;
  Blur.SetStepSize(0.5);

  auto blurMask = Blur.Generate(origLabelMask, this->m_brainMask, this->m_skullStripMask);

  // Crop necessary masks around blurMask and upsample resolution
  HighResCropFromReferenceMask highResCropFilter;
  highResCropFilter.SetReferenceImage(blurMask);
  highResCropFilter.SetCropRegionBuffer(2);
  highResCropFilter.SetResamplingFactor(m_upsamplingFactor);
  highResCropFilter.FindCropRegion();

  auto blurMaskHighResCrop = highResCropFilter.Apply<UCharImageType>(blurMask);
  auto origLabelMaskHighResCrop = highResCropFilter.Apply<UCharImageType>(origLabelMask);
  auto origBrainMaskHighResCrop = highResCropFilter.Apply<UCharImageType>(m_brainMask);
  auto skullStripMaskHighResCrop = highResCropFilter.Apply<UCharImageType>(m_skullStripMask);
  auto wmMaskHighResCrop = highResCropFilter.Apply<UCharImageType>(m_wmMask);

  // Create cropped/upsampled masks for atrophied timepoint
  auto atrophyLabelMaskHighResCrop =
    LocalizedAtrophy(origLabelMaskHighResCrop, wmMaskHighResCrop, m_nErosionIters);

  auto atrophyBrainMaskHighResCrop =
    ReplaceLabelInImage(origBrainMaskHighResCrop, origLabelMaskHighResCrop,
			atrophyLabelMaskHighResCrop);

  // Register brain masks (OrigHighResCrop to AtrophyHighResCrop)
  auto warpHighResCrop = RegisterLabelMasks(origBrainMaskHighResCrop, atrophyBrainMaskHighResCrop);

  // Apply blur mask to displacement field
  auto warpMaskedHighResCrop =
    Blur.ApplyToWarp(warpHighResCrop, blurMaskHighResCrop, origLabelMaskHighResCrop);

  // Paste into composite output data
  AddImagesInPlace<IntImageType, UCharImageType>(this->m_countImage, blurMask);
  
  auto warpMasked = highResCropFilter.Revert<VectorImageType>(warpMaskedHighResCrop);
  AddImagesInPlace<VectorImageType>(this->m_warp, warpMasked);
}


void RBSA::GenerateAtrophyTransforms()
{
  // Generate label-specific transforms
  for(const auto& label : this->m_targetLabels) {
    std::cout << "Generating atrophy transform for label " << label << std::endl;
    this->GenerateTransformForLabel(label);
  }

  // Normalize output warps by this->m_countImage
  auto region = this->m_countImage->GetLargestPossibleRegion();
  auto countIt = ImageRegionIteratorWithIndexType<IntImageType>(this->m_countImage, region);
  auto warpIt = ImageRegionIteratorWithIndexType<VectorImageType>(this->m_warp, region);

  for(countIt.GoToBegin(), warpIt.GoToBegin(); !countIt.IsAtEnd(); ++countIt, ++warpIt)
    {
      const TPixel<IntImageType>& n = countIt.Get();
      if(n > 0) {
        const TPixel<VectorImageType>& warpPixel = warpIt.Get() / static_cast<float>(n);
        warpIt.Set(warpPixel);
      }
    }

  this->m_mask = BinaryThresholdImage<IntImageType>(this->m_countImage);
  this->m_mask->DisconnectPipeline();
  this->m_warp->DisconnectPipeline();
}

