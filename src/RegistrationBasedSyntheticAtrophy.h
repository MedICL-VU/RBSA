#include "BinaryImageMorphology.h"
#include "BlurMask.h"
#include "HighResCrop.h"
#include "MSDD.h"
#include "ParcellateSurface.h"
#include "Warp.h"
#include "utils.h"

using RBSAOutTuple = std::tuple<UCharImageType::Pointer,
				VectorImageType::Pointer,
				VectorImageType::Pointer>;

class RBSA {
 public:
  RBSA();

  // Input images
  void SetInputParcellation(IntImageType::Pointer parc) { this->m_parcellation = parc; }
  void SetBrainMask(UCharImageType::Pointer mask) { this->m_brainMask = mask; }
  void SetSkullStripMask(UCharImageType::Pointer mask) { this->m_skullStripMask = mask; }
  void SetWMMask(UCharImageType::Pointer mask) { this->m_wmMask = mask; }

  // Parameters
  void SetNumberOfErosionIterations(unsigned int nIters) { this->m_nErosionIters = nIters; }
  void SetUpsamplingFactor(float factor) { this->m_upsamplingFactor = factor; }
  
  // Main method
  RBSAOutTuple GenerateAtrophyTransforms(unsigned int label);

  
 private:
  IntImageType::Pointer m_parcellation;
  UCharImageType::Pointer m_brainMask;
  UCharImageType::Pointer m_skullStripMask;
  UCharImageType::Pointer m_wmMask;
  unsigned int m_nErosionIters;
  float m_upsamplingFactor;

  VectorImageType::Pointer m_warp;
  VectorImageType::Pointer m_inverseWarp;
};


