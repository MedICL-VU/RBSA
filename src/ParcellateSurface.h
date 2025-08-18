#ifndef _PARCELLATE_SURFACE_H_
#define _PARCELLATE_SURFACE_H_

#include "utils.h"

//-------------------------------------------------------------------------------------------------

template <typename TImage>
typename TImage::Pointer MakeTargetLabelParcellation
(typename TImage::Pointer input, std::vector<typename TImage::PixelType>& targetLabels,
 typename TImage::PixelType defaultPixelValue = 1);

template <typename TImage>
typename TImage::PixelType MajorityVote
(const std::vector<typename TImage::PixelType>& votes,
 const std::vector<typename TImage::PixelType>& validLabels);

template <typename TImage, typename TArray>
vtkSmartPointer<TArray> AssignLabels
(vtkSmartPointer<vtkPolyData> surface, typename TImage::Pointer image,
 const std::string& labelArrayName, const std::vector<typename TImage::PixelType>& validLabels,
 bool convertFromRAS);

template <typename TImage, typename TArray>
unsigned int FillHoles
(vtkSmartPointer<vtkPolyData> surface, vtkSmartPointer<TArray> labels,
 const std::vector<typename TImage::PixelType>& validLabels, bool convertFromRAS);

template <typename TImage>
void ParcellateSurface
(vtkSmartPointer<vtkPolyData> surface, typename TImage::Pointer image,
 const std::string& labelArrayName, const std::vector<typename TImage::PixelType>& validLabels,
 bool convertFromRAS = false);


#endif
