#ifndef RBSA_H
#define RBSA_H

#include "BinaryImageMorphology.h"
#include "BlurMask.h"
#include "HighResCrop.h"
#include "MSDD.h"
#include "ParcellateSurface.h"
#include "Warp.h"
#include "utils.h"

#include "itkBinaryFillholeImageFilter.h"

class RBSA {
 public:
  RBSA();

  // Input images
  void SetInputParcellation(TPointer<IntImageType> parc);
  void SetSkullStripMask(TPointer<UCharImageType> mask);

  // Labels
  void SetTargetLabels(const std::vector<int>& inputLabels);
  void SetWMLabels(const std::vector<int>& inputLabels);

  // Parameters
  void SetNumberOfErosionIterations(unsigned int nIters) { this->m_nErosionIters = nIters; }
  void SetUpsamplingFactor(float factor) { this->m_upsamplingFactor = factor; }
    
  // Methods
  void GenerateTransformForLabel(unsigned int label);
  void GenerateAtrophyTransforms();

  // Outputs
  TPointer<UCharImageType> GetWarpMask() const { return this->m_mask; }
  TPointer<VectorImageType> GetWarp() const { return this->m_warp; }
  
 private:
  // Inputs
  TPointer<IntImageType> m_parc;
  TPointer<UCharImageType> m_brainMask;
  TPointer<UCharImageType> m_skullStripMask;
  TPointer<UCharImageType> m_wmMask;

  // Parameters
  std::vector<int> m_targetLabels;
  unsigned int m_nErosionIters;
  float m_upsamplingFactor;

  // Composite output images
  TPointer<IntImageType> m_countImage;
  TPointer<UCharImageType> m_mask;
  TPointer<VectorImageType> m_warp;
};

using BinaryFillHolesFilterType = itk::BinaryFillholeImageFilter<UCharImageType>;

#endif
