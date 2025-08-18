# Registration-Based Synthetic Atrophy (RBSA): Synthetic Atrophy for Longitudinal Cortical Surfaces

## Overview

This repository contains a Registration-based tool for inducing ground truth, synthetic gray matter (GM) atrophy in brain images and cortical surfaces. Given an input cortical label map, the RBSA pipeline performs the following steps to induce synthetic atrophy in a user-specified target label (or labels):

1. Isolates the target GM label(s) from the input parcellation to create a mask for the original timepoint
2. Performs a series of binary image morphology operations in each target label to create corresponding masks for the "atrophied" timepoint
3. Registers the original masks to the "atrophied" masks to yield a displacement field that pushes the GM/CSF boundary inwards towards the WM. Individual fields are generated for each label and then combined to produce single, atrophy inducing transformation.
4. Applies the transformation to any input images/surfaces.
5. Calculates the ground truth, mean change induced within each target label, defined as the mean surface displacement difference (MSDD):

```math
\textrm{MSDD}_{\textrm{label}} =
\frac{1}{2}( \textrm{GM}_{\textrm{orig}\rightarrow\textrm{atrophy}} + \textrm{GM}_{\textrm{atrophy}\rightarrow\textrm{orig}} ) - \frac{1}{2}( \textrm{WM}_{\textrm{orig}\rightarrow\textrm{atrophy}} + \textrm{WM}_{\textrm{atrophy}\rightarrow\textrm{orig}} )
```


## Dependencies

- ITK:  
  - https://itk.org/download/ 
  - version 4.13.2 recommended, as this is what I had access to when developing the new version... but feel free to let me know if it works with a more recent version
- VTK:
  - https://vtk.org/download/ 
  - version 9.3.0 or later
- FreeSurfer (optional)
  - https://surfer.nmr.mgh.harvard.edu/fswiki/DownloadAndInstall
  - Not required to run, but there is an option to run the pipeline in a way to easily interface with FreeSurfer's recon-all outputs instead of using user-provided paths for all inputs

## References

If you use this tool, please cite the original paper:

Synthetic Atrophy for Longitudinal Cortical Surface Analyses, K. E. Larson, I. Oguz. Frontiers in Neuroimaging, 2022.
https://doi.org/10.3389/fnimg.2022.861687