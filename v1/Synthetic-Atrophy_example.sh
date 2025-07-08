#!/bin/bash

set -x

# NOTE:  you must also have vtk (version 7-11 or later) and itk (4-13-3 or later) downloaded and compiled. you can find these here:
# VTK:  https://vtk.org/download/
# ITK:  https://itk.org/download/

WDIR=Synthetic-Atrophy-For-Longtidunal-Cortical-Surface-Analyses


# External tools you will need to run this example:
c3d= # (https://sourceforge.net/p/c3d/git/ci/master/tree/)
slicer= # (https://download.slicer.org/)
greedy= # (https://sites.google.com/view/greedyreg/)
imagemath= # (https://github.com/NIRALUser/niral_utilities/tree/master/ImageMath)
holedetection= # (https://www.insight-journal.org/browse/publication/43)


# Synthetic-Atrophy tools
distanceBetweenSurfaces=$WDIR/bin/distanceBetweenSurfaces
getBlurMask=$WDIR/bin/getBlurMask
getTrimRegion=$WDIR/bin/getTrimRegion
maskField=$WDIR/bin/maskField
padImage=$WDIR/bin/padImage
padVectorImage=$WDIR/bin/padVectorImage
parcellateAtrophyRegion=$WDIR/bin/parcellateAtrophyRegion
performErosion=$WDIR/bind/performErosion
readMeshArray=$WDIR/bin/readMeshArray
trimFloatImage=$WDIR/bin/trimFloatImage
trimImage=$WDIR/bin/trimImage
warpSurfaces=$WDIR/bin/warpSurfs
VTK2VTP=$WDIR/bin/VTK2VTP
VTP2VTK=$WDIR/bin/VTP2VTK



#########################
##  Utility functions  ##
#########################

# Crops an image and upsamples to 400%
function highRes_crop(){
    original=$1
    rescaleFactor=$2
    cropRegion_file=$3
    outbase=$4

    trim=${outbase}_trim.nii.gz
    cropRegion_info=$(sed -n 1p $cropRegion_file)
    $trimImage $original $trim $cropRegion_info

    cropRegion_origin=$(sed -n 2p $cropRegion_file)

    trim_highRes=${outbase}_trim_highRes.nii.gz
    c3d -interpolation NearestNeighbor $trim -resample ${rescaleFactor}% -holefill 1 0 -o $trim_highRes
    cropRegion_origin=$(sed -n 2p $cropRegion_file)
}


# Remove handles on surfaces
function Remove-Handles(){
    dir=${1?}
    mkdir -p $dir

    image0=${2?}
    mesh0=${3?}
    outbase=${4?}

    cp $image0 $dir/image_0.nii.gz
    cp $mesh0 $dir/mesh_0.vtk

    flip=0

    for i in `seq 1 10`
    do
      echo $i
      prev=`expr $i - 1`
      holes=$dir/holes_$i.vtk
      mesh=$dir/mesh_${prev}.vtk
      newmesh=$dir/mesh_$i
      image=$dir/image_$prev.nii.gz
      newimage=$dir/image_$i.nii.gz
      finalimage=${outbase}holeFill.nii.gz
      finalmesh=${outbase}holeFill

      flip=`expr 1 - $flip`

      $holedetection 0 $mesh $holes

      holecount=`grep LINES $holes | wc -l `
      if [ $holecount -eq 0 ]
      then
         $imagemath $image -outfile $finalimage -conComp 1
         $slicer --launch ModelMaker $finalimage --generateAll --smooth 10 --decimate 0 --pointnormals --name $finalmesh >> /dev/null
         mv ${finalmesh}_1.vtk $finalmesh.vtk
         break
      else
        echo "holes found"
        $fillholes $holes $image $newimage $flip
	$slicer --launch ModelMaker $newimage --generateAll --smooth 0 --decimate 0 --pointnormals --name $newmesh >> /dev/null
        mv ${newmesh}_1.vtk $newmesh.vtk
      fi
    done
    rm *mrml
    rm -r $dir
}


# Convert input image to surface mesh
function Get-Mesh(){
    inputImage=$1
    outputMesh=$2
    DIR=$3

    mkdir -p $DIR/Dump $DIR/Handle-Removal
    modelMakerMesh_base=$DIR/Dump/mesh
    removeHandles_base=$DIR/Dump/mesh_noHandles_

    $slicer --launch ModelMaker $inputImage --generateAll --smooth 10 --decimate 0 --pointnormals --name $modelMakerMesh_base >> /dev/null
    Remove-Handles $DIR/Handle-Removal $inputImage ${modelMakerMesh_base}_1.vtk $removeHandles_base
    $VTK2VTP ${removeHandles_base}holeFill.vtk $outputMesh

    rm -r $DIR/Dump
}


# Transform image w/ new orientation and origin
function Change-Image-Space(){
    input=$1
    output=$2
    orientation=$3
    originX=$4
    originY=$5
    originZ=$6

    $slicer --launch OrientScalarVolume --orientation ${orientation} $input $output >> /dev/null
    c3d $output -origin-voxel ${originX}x${originY}x${originZ}vox -o $output
}



############
##  Main  ##
############

function main {
    ######################################
    ##    Set up all the data to run    ##
    ######################################

    # Set up directories
    DATA_DIR=$WDIR/Example-Data
    INPUT_DIR=$DATA_DIR/Input
    ROI_DIR=$DATA_DIR/ROI
    MASK_DIR=$DATA_DIR/HighRes-Mask
    TRANSFORM_DIR=$DATA_DIR/Transforms
    SURFACE_DIR=$DATA_DIR/Surfaces
    RESULTS_DIR=$DATA_DIR/Results
    mkdir -p $ROI_DIR $MASK_DIR $TRANSFORM_DIR $SURFACE_DIR $RESULTS_DIR


    # Provided data
    T1_original=$INPUT_DIR/T1-original.nii.gz
    FLAIR_original=$INPUT_DIR/FLAIR-original.nii.gz
    parcellation=$INPUT_DIR/inputParcellation.nii.gz
    skullStripMask=$INPUT_DIR/skullStripMask.nii.gz


    # Make filled in WM mask
    wm=$INPUT_DIR/wm.nii.gz
    labels_wm_lh=( $( awk -F " " '{print $1}' $INPUT_DIR/aparc.DKTatlas+aseg_wmFill_lh.txt ) )
    labels_wm_rh=( $( awk -F " " '{print $1}' $INPUT_DIR/aparc.DKTatlas+aseg_wmFill_rh.txt ) )
    WMmask_original_lh=$SURFACE_DIR/WM_lh_0.nii.gz
    WMmask_original_rh=$SURFACE_DIR/WM_rh_0.nii.gz

    m=0
    c3d_command_wm_lh="c3d $parcellation -as P -thresh 0 0 0 0 -popas M0"
    for label in ${labels_wm_lh[@]} ; do
	let n=$m+1
	c3d_command_wm_lh=$c3d_command_wm_lh" -push P -thresh $label $label $label 0 -push M$m -add -popas M$n"
	let m=$m+1
    done
    c3d_command_wm_lh=$c3d_command_wm_lh" -push M$n -thresh 1 inf 1 0 -holefill 1 0 -o $WMmask_original_lh"
    $c3d_command_wm_lh

    m=0
    c3d_command_wm_rh="c3d $parcellation -as P -thresh 0 0 0 0 -popas M0"
    for label in ${labels_wm_rh[@]} ; do
	let n=$m+1
	c3d_command_wm_rh=$c3d_command_wm_rh" -push P -thresh $label $label $label 0 -push M$m -add -popas M$n"
	let m=$m+1
    done
    c3d_command_wm_rh=$c3d_command_wm_rh" -push M$n -thresh 1 inf 1 0 -holefill 1 0 -o $WMmask_original_rh"
    $c3d_command_wm_rh

    c3d $WMmask_original_lh $WMmask_original_rh -add -o $wm

    
    # Make full brain mask
    fullBrainMask_lh=$SURFACE_DIR/GM_lh_0.nii.gz
    fullBrainMask_rh=$SURFACE_DIR/GM_rh_0.nii.gz
    fullBrainMask=$INPUT_DIR/fullBrainMask.nii.gz
    fullBrainMask_sdt=$INPUT_DIR/fullBrainMask_sdt.nii.gz

    c3d $WMmask_original_lh $parcellation -as P -thresh 1001 1035 1 0 -add -thresh 1 inf 1 0 -o $fullBrainMask_lh
    c3d $WMmask_original_rh $parcellation -as P -thresh 2001 2035 1 0 -add -thresh 1 inf 1 0 -o $fullBrainMask_rh
    c3d $fullBrainMask_lh $fullBrainMask_rh -add -o $fullBrainMask
    c3d $fullBrainMask -sdt $fullBrainMask_sdt
    

    # Convert all inputs to working space
    fullBrainMask_LPI=$INPUT_DIR/fullBrainMask_LPI.nii.gz
    parcellation_LPI=$INPUT_DIR/inputParcellation_LPI.nii.gz
    skullStripMask_LPI=$INPUT_DIR/skullStripMask_LPI.nii.gz
    wm_LPI=$INPUT_DIR/wm_LPI.nii.gz
    fullBrainMask_sdt_LPI=$INPUT_DIR/fullBrainMask_sdt_LPI.nii.gz

    Change-Image-Space $fullBrainMask $fullBrainMask_LPI LPI 0 0 0
    Change-Image-Space $parcellation $parcellation_LPI LPI 0 0 0
    Change-Image-Space $skullStripMask $skullStripMask_LPI LPI 0 0 0
    Change-Image-Space $wm $wm_LPI LPI 0 0 0
    Change-Image-Space $fullBrainMask_sdt $fullBrainMask_sdt_LPI LPI 0 0 0
    
    

    ##############################################################
    ##    Get high-resolution images cropped to selected ROI    ##
    ##############################################################

    # Get label image
    label=1030 # label for atrophy induction (this is the ROI we used as an example in our publication)
    labelMask_original=$ROI_DIR/label_original.nii.gz
    $c3d $parcellation_LPI -threshold $label $label 1 0 -o $labelMask_original

    
    # Blur mask (mask used for blurring/smoothing the atrophy deformation field)
    blurMask=$MASK_DIR/blurMask.nii.gz
    $getBlurMask $labelMask_original $fullBrainMask_sdt_LPI $skullStripMask_LPI $blurMask 0.4
    
    
    # Get cropping data from blur mask (we are going to crop all the images to the blur mask + a 4 voxel buffer on all sides) 
    crop_region_info_file=$MASK_DIR/crop_region_info.txt
    if [ -f $crop_region_info_file ] ; then
	rm $crop_region_info_file
    fi
    trim_info=$($getTrimRegion $blurMask 4)
    echo $trim_info >> $crop_region_info_file

    blurMask_trim=$MASK_DIR/blurMask_trim.nii.gz
    $trimImage $blurMask $blurMask_trim $trim_info
    ref_info=$($c3d $blurMask_trim -info)
    x=$(echo $ref_info | cut -d "[" -f 3 | cut -d " " -f 1)
    y=$(echo $ref_info | cut -d "[" -f 3 | cut -d " " -f 2)
    z=$(echo $ref_info | cut -d "[" -f 3 | cut -d " " -f 3 | cut -d "]" -f 1)
    echo $x $y $z >> $crop_region_info_file


    # Upsample and crop image data
    blurMask_trim_highRes=$MASK_DIR/blurMask_trim_highRes.nii.gz
    resampleFactor=400

    c3d -interpolation NearestNeighbor $blurMask_trim -resample ${resampleFactor}% -holefill 1 0 -o $blurMask_trim_highRes
    highRes_crop $skullStripMask_LPI $resampleFactor $crop_region_info_file $MASK_DIR/skullStripMask
    highRes_crop $parcellation_LPI $resampleFactor $crop_region_info_file $MASK_DIR/inputParcellation
    highRes_crop $labelMask_original $resampleFactor $crop_region_info_file $ROI_DIR/label_original
    highRes_crop $wm_LPI $resampleFactor $crop_region_info_file $MASK_DIR/wm
    highRes_crop $fullBrainMask_sdt_LPI $resampleFactor $crop_region_info_file $MASK_DIR/sdt



    ###########################################
    ##    Induce synthetic atrophy in ROI    ##
    ###########################################

    # Perform erosion
    mkdir -p $ROI_DIR/Erosion-Iterations
    labelMask_atrophy=$ROI_DIR/label_atrophy_trim_highRes.nii.gz
    nIterations=4 # number of iterations for erosion via binary morphology operations

    n=1
    while [ $n -le $nIterations ] ; do
	if [ $n == 1 ] ; then
	    prev=$ROI_DIR/label_original_trim_highRes.nii.gz
	else
	    let n_prev=$n-1
	    prev=$ROI_DIR/Erosion-Iterations/${n_prev}.nii.gz
	fi
	next=$ROI_DIR/Erosion-Iterations/${n}.nii.gz
	$performErosion $prev $MASK_DIR/wm_trim_highRes.nii.gz $next 1 0
	let n=$n+1
    done
    
    cp $next $labelMask_atrophy
    #rm $ROI_DIR/Erosion-Iterations


    # Get full brain mask for atrophied timepoint
    joint_labelMask_original=$ROI_DIR/label_original_joint_trim_highRes.nii.gz
    c3d $MASK_DIR/inputParcellation_trim_highRes.nii.gz -as P -threshold 1 inf 1 0 -o $joint_labelMask_original
    
    joint_labelMask_atrophy=$ROI_DIR/label_atrophy_joint_trim_highRes.nii.gz
    let lower_threshold=$label-1
    let upper_threshold=$label+1

    c3d $MASK_DIR/inputParcellation_trim_highRes.nii.gz -as P -threshold 1 $lower_threshold 1 0 -popas LM \
	-push P -threshold $upper_threshold inf 1 0 -popas UM \
	-push LM -push UM -add $labelMask_atrophy -add -o $joint_labelMask_atrophy


 
    # Register original --> atrophied
    field_outbase=$TRANSFORM_DIR/field
    field_inverse_outbase=$TRANSFORM_DIR/field_inverse
    
    $greedy -d 3 -m MSQ -i $joint_labelMask_atrophy $joint_labelMask_original -o ${field_outbase}_full_trim_highRes.nii.gz \
	    -oinv ${field_inverse_outbase}_full_trim_highRes.nii.gz \
	    -mm $joint_labelMask_original -n 100x100x50x10 >> /dev/null
    $c3d -mcs ${field_outbase}_full_trim_highRes.nii.gz -foreach -resample 25% -origin 0x0x0mm -endfor -omc ${field_outbase}_full_trim.nii.gz
    $c3d -mcs ${field_inverse_outbase}_full_trim_highRes.nii.gz -foreach -resample 25% -origin 0x0x0mm -endfor -omc ${field_inverse_outbase}_full_trim.nii.gz
    
    
    # Mask/blur fields
    pad_info=$(sed -n 1p $crop_region_info_file)
    mkdir -p $TRANSFORM_DIR/Dump
    
    c3d $MASK_DIR/sdt_trim.nii.gz -origin 0x0x0mm -o $TRANSFORM_DIR/Dump/brainSDT_trim.nii.gz
    c3d $ROI_DIR/label_original_trim.nii.gz -origin 0x0x0mm -o $TRANSFORM_DIR/Dump/label_original_trim.nii.gz
    c3d $MASK_DIR/skullStripMask_trim.nii.gz -origin 0x0x0mm -o $TRANSFORM_DIR/Dump/skullStripMask_trim.nii.gz
    c3d $MASK_DIR/wm_trim.nii.gz -origin 0x0x0mm -o $TRANSFORM_DIR/Dump/wm_trim.nii.gz

    $maskField $TRANSFORM_DIR/Dump/brainSDT_trim.nii.gz $TRANSFORM_DIR/Dump/label_original_trim.nii.gz $TRANSFORM_DIR/Dump/skullStripMask_trim.nii.gz ${field_outbase}_full_trim.nii.gz \
	       $TRANSFORM_DIR/Dump/wm_trim.nii.gz 0.4 $TRANSFORM_DIR/Dump/points.vtp ${field_outbase}_maskBlur_trim.nii.gz
    $padVectorImage $MASK_DIR/blurMask.nii.gz ${field_outbase}_maskBlur_trim.nii.gz ${field_outbase}_maskBlur_pad.nii.gz $pad_info
    c3d -mcs ${field_outbase}_maskBlur_pad.nii.gz -foreach -origin 0x0x0vox -endfor -omc ${field_outbase}_maskBlur_pad.nii.gz
    
    $maskField $TRANSFORM_DIR/Dump/brainSDT_trim.nii.gz $TRANSFORM_DIR/Dump/label_original_trim.nii.gz $TRANSFORM_DIR/Dump/skullStripMask_trim.nii.gz ${field_inverse_outbase}_full_trim.nii.gz \
	       $TRANSFORM_DIR/Dump/wm_trim.nii.gz 0.4 $TRANSFORM_DIR/Dump/points.vtp ${field_inverse_outbase}_maskBlur_trim.nii.gz
    $padVectorImage $MASK_DIR/blurMask.nii.gz ${field_inverse_outbase}_maskBlur_trim.nii.gz ${field_inverse_outbase}_maskBlur_pad.nii.gz $pad_info
        c3d -mcs ${field_inverse_outbase}_maskBlur_pad.nii.gz -foreach -origin 0x0x0vox -endfor -omc ${field_inverse_outbase}_maskBlur_pad.nii.gz

    rm -r $TRANSFORM_DIR/Dump
    
    
    
    #######################################
    ##    Create synthetic image data    ##
    ######################################
    
    # Apply warp to T1 image
    T1_atrophy=$RESULTS_DIR/T1-atrophy.nii.gz
    FLAIR_atrophy=$RESULTS_DIR/FLAIR-atrophy.nii.gz
    
    $greedy -d 3 -rf $T1_original -rm $T1_original $T1_atrophy -r ${field_outbase}_maskBlur_pad.nii.gz
    $greedy -d 3 -rf $T1_original -rm $FLAIR_original $FLAIR_atrophy -r ${field_outbase}_maskBlur_pad.nii.gz



    ###############################
    ##    Create surface data    ##
    ###############################

    # Make label map for ROI
    labelMask_original_dataSpace=$ROI_DIR/label_original_dataSpace.nii.gz
    Change-Image-Space $labelMask_original $labelMask_original_dataSpace LAI 0 0 0
    
    fullBrainMask_parc=$DATA_DIR/parcellation_ROI.nii.gz
    c3d $fullBrainMask $labelMask_original_dataSpace -as ROI -scale -1 -add -popas M \
	-push ROI -dilate 1 4x4x4vox -push M -times -push M -add -push ROI -scale 3 -add -o $fullBrainMask_parc
    
 
    # Get meshes of original (unatrophied) data
    WM_lh_mesh=$SURFACE_DIR/WM_lh.vtp
    WM_rh_mesh=$SURFACE_DIR/WM_rh.vtp
    GM_lh_mesh=$SURFACE_DIR/GM_lh.vtp
    GM_rh_mesh=$SURFACE_DIR/GM_rh.vtp

    Get-Mesh $WMmask_original_lh $WM_lh_mesh $SURFACE_DIR
    Get-Mesh $WMmask_original_rh $WM_rh_mesh $SURFACE_DIR
    Get-Mesh $fullBrainMask_lh $GM_lh_mesh $SURFACE_DIR
    Get-Mesh $fullBrainMask_lh $GM_rh_mesh $SURFACE_DIR

    
    # Parcellate meshes
    WM_lh_mesh_original=$SURFACE_DIR/WM_lh_original.vtp
    WM_rh_mesh_original=$SURFACE_DIR/WM_rh_original.vtp
    GM_lh_mesh_original=$SURFACE_DIR/GM_lh_original.vtp
    GM_rh_mesh_original=$SURFACE_DIR/GM_rh_original.vtp

    WM_lh_mesh_parc=$RESULTS_DIR/WM_lh_atrophyLabelParcellation.txt
    WM_rh_mesh_parc=$RESULTS_DIR/WM_rh_atrophyLabelParcellation.txt
    GM_lh_mesh_parc=$RESULTS_DIR/GM_lh_atrophyLabelParcellation.txt
    GM_rh_mesh_parc=$RESULTS_DIR/GM_rh_atrophyLabelParcellation.txt

    $parcellateAtrophyRegion $WM_lh_mesh $GM_lh_mesh $fullBrainMask_parc $WM_lh_mesh_original $GM_lh_mesh_original 0
    $parcellateAtrophyRegion $WM_rh_mesh $GM_rh_mesh $fullBrainMask_parc $WM_rh_mesh_original $GM_rh_mesh_original 0

    
    # Apply warp to surfaces
    WM_lh_mesh_atrophy=$SURFACE_DIR/WM_lh_atrophy.vtp
    WM_rh_mesh_atrophy=$SURFACE_DIR/WM_rh_atrophy.vtp
    GM_lh_mesh_atrophy=$SURFACE_DIR/GM_lh_atrophy.vtp
    GM_rh_mesh_atrophy=$SURFACE_DIR/GM_rh_atrophy.vtp

    $warpSurfaces $WM_lh_mesh_original ${field_inverse_outbase}_maskBlur_pad.nii.gz $MASK_DIR/blurMask.nii.gz $WM_lh_mesh_atrophy 0 1
    $warpSurfaces $WM_rh_mesh_original ${field_inverse_outbase}_maskBlur_pad.nii.gz $MASK_DIR/blurMask.nii.gz $WM_rh_mesh_atrophy 0 1
    $warpSurfaces $GM_lh_mesh_original ${field_inverse_outbase}_maskBlur_pad.nii.gz $MASK_DIR/blurMask.nii.gz $GM_lh_mesh_atrophy 0 0
    $warpSurfaces $GM_rh_mesh_original ${field_inverse_outbase}_maskBlur_pad.nii.gz $MASK_DIR/blurMask.nii.gz $GM_rh_mesh_atrophy 0 0



    ####################################
    ##    Measure thickness change    ##
    ####################################
    
    # Get distance between surfaces
    WM_lh_mesh_original_surfaceDistance=$SURFACE_DIR/WM_lh_original_surfaceDistance.vtp
    WM_rh_mesh_original_surfaceDistance=$SURFACE_DIR/WM_rh_original_surfaceDistance.vtp
    GM_lh_mesh_original_surfaceDistance=$SURFACE_DIR/GM_lh_original_surfaceDistance.vtp
    GM_rh_mesh_original_surfaceDistance=$SURFACE_DIR/GM_rh_original_surfaceDistance.vtp

    WM_lh_mesh_atrophy_surfaceDistance=$SURFACE_DIR/WM_lh_atrophy_surfaceDistance.vtp
    WM_rh_mesh_atrophy_surfaceDistance=$SURFACE_DIR/WM_rh_atrophy_surfaceDistance.vtp
    GM_lh_mesh_atrophy_surfaceDistance=$SURFACE_DIR/GM_lh_atrophy_surfaceDistance.vtp
    GM_rh_mesh_atrophy_surfaceDistance=$SURFACE_DIR/GM_rh_atrophy_surfaceDistance.vtp

    $distanceBetweenSurfaces $WM_lh_mesh_original $WM_lh_mesh_atrophy $WM_lh_mesh_original_surfaceDistance
    $distanceBetweenSurfaces $WM_lh_mesh_atrophy $WM_lh_mesh_original $WM_lh_mesh_atrophy_surfaceDistance
    $distanceBetweenSurfaces $WM_rh_mesh_original $WM_rh_mesh_atrophy $WM_rh_mesh_original_surfaceDistance
    $distanceBetweenSurfaces $WM_rh_mesh_atrophy $WM_rh_mesh_original $WM_rh_mesh_atrophy_surfaceDistance
    $distanceBetweenSurfaces $GM_lh_mesh_original $GM_lh_mesh_atrophy $GM_lh_mesh_original_surfaceDistance
    $distanceBetweenSurfaces $GM_lh_mesh_atrophy $GM_lh_mesh_original $GM_lh_mesh_atrophy_surfaceDistance
    $distanceBetweenSurfaces $GM_rh_mesh_original $GM_rh_mesh_atrophy $GM_rh_mesh_original_surfaceDistance
    $distanceBetweenSurfaces $GM_rh_mesh_atrophy $GM_rh_mesh_original $GM_rh_mesh_atrophy_surfaceDistance
    

    # Write everything out into a text file
    WM_lh_mesh_parc_txt=$RESULTS_DIR/WM_lh_atrophyLabelParcellation.txt
    WM_rh_mesh_parc_txt=$RESULTS_DIR/WM_rh_atrophyLabelParcellation.txt
    GM_lh_mesh_parc_txt=$RESULTS_DIR/GM_lh_atrophyLabelParcellation.txt
    GM_rh_mesh_parc_txt=$RESULTS_DIR/GM_rh_atrophyLabelParcellation.txt

    $readMeshArray $WM_lh_mesh_original $WM_lh_mesh_parc_txt "Atrophy-Region"
    $readMeshArray $WM_rh_mesh_original $WM_rh_mesh_parc_txt "Atrophy-Region"
    $readMeshArray $GM_lh_mesh_original $GM_lh_mesh_parc_txt "Atrophy-Region"
    $readMeshArray $GM_rh_mesh_original $GM_rh_mesh_parc_txt "Atrophy-Region"
    
    WM_lh_mesh_original_surfaceDistance_txt=$RESULTS_DIR/WM_lh_surfaceDistance_original2atrophy.txt
    WM_rh_mesh_original_surfaceDistance_txt=$RESULTS_DIR/WM_rh_surfaceDistance_original2atrophy.txt
    WM_lh_mesh_atrophy_surfaceDistance_txt=$RESULTS_DIR/WM_lh_surfaceDistance_atrophy2original.txt
    WM_rh_mesh_atrophy_surfaceDistance_txt=$RESULTS_DIR/WM_rh_surfaceDistance_atrophy2original.txt
    GM_lh_mesh_original_surfaceDistance_txt=$RESULTS_DIR/GM_lh_surfaceDistance_original2atrophy.txt
    GM_rh_mesh_original_surfaceDistance_txt=$RESULTS_DIR/GM_rh_surfaceDistance_original2atrophy.txt
    GM_lh_mesh_atrophy_surfaceDistance_txt=$RESULTS_DIR/GM_lh_surfaceDistance_atrophy2original.txt
    GM_rh_mesh_atrophy_surfaceDistance_txt=$RESULTS_DIR/GM_rh_surfaceDistance_atrophy2original.txt

    $readMeshArray $WM_lh_mesh_original_surfaceDistance $WM_lh_mesh_original_surfaceDistance_txt "Surface-Distance"
    $readMeshArray $WM_rh_mesh_original_surfaceDistance $WM_rh_mesh_original_surfaceDistance_txt "Surface-Distance"
    $readMeshArray $WM_lh_mesh_atrophy_surfaceDistance $WM_lh_mesh_atrophy_surfaceDistance_txt "Surface-Distance"
    $readMeshArray $WM_rh_mesh_atrophy_surfaceDistance $WM_rh_mesh_atrophy_surfaceDistance_txt "Surface-Distance"
    $readMeshArray $GM_lh_mesh_atrophy_surfaceDistance $GM_lh_mesh_original_surfaceDistance_txt "Surface-Distance"
    $readMeshArray $GM_rh_mesh_original_surfaceDistance $GM_rh_mesh_original_surfaceDistance_txt "Surface-Distance"
    $readMeshArray $GM_lh_mesh_atrophy_surfaceDistance $GM_lh_mesh_atrophy_surfaceDistance_txt "Surface-Distance"
    $readMeshArray $GM_rh_mesh_atrophy_surfaceDistance $GM_rh_mesh_atrophy_surfaceDistance_txt "Surface-Distance"
}



if [[ $1 ]]; then
  command=$1
  echo $1
  shift
  $command $@
else
    main
fi

