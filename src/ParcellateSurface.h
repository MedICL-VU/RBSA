#ifndef _PARCELLATE_SURFACE_H_
#define _PARCELLATE_SURFACE_H_

#include "utils.h"

//-------------------------------------------------------------------------------------------------

template <typename TImage>
TPointer<TImage> MakeTargetLabelParcellation(TPointer<TImage> input,
					 std::vector<TPixel<TImage>>& vLabels,
					 TPixel<TImage> defaultPixelValue = 1);

template <typename TImage>
TPixel<TImage> MajorityVote(const std::vector<TPixel<TImage>>& votes,
			  const std::vector<TPixel<TImage>>& vLabels);

template <typename TImage, typename TArray>
vtkSmartPointer<TArray>AssignLabels(vtkSmartPointer<vtkPolyData> surface,
				    TPointer<TImage> image,
				    const std::string& labelArrayName,
				    const std::vector<TPixel<TImage>>& vLabels,
				    bool convertFromRAS);

template <typename TImage, typename TArray>
unsigned int FillHoles(vtkSmartPointer<vtkPolyData> surface,
		       vtkSmartPointer<TArray> labels,
		       const std::vector<TPixel<TImage>>& vLabels,
		       bool convertFromRAS);

template <typename TImage>
void ParcellateSurface(vtkSmartPointer<vtkPolyData> surface,
		       TPointer<TImage> image,
		       const std::string& labelArrayName,
		       const std::vector<TPixel<TImage>>& vLabels,
		       bool convertFromRAS = false);

#endif
