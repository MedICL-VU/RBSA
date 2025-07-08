#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkPointData.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkFloatArray.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkIdTypeArray.h>
#include <vtkIdList.h>
#include <vtkTriangleFilter.h>
#include <vtkCellArray.h>
#include <vtkSelectEnclosedPoints.h>
#include <vtkClipPolyData.h>
#include <vtkPolyDataConnectivityFilter.h>
#include <vtkFillHolesFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkCleanPolyData.h>
#include <vtkFeatureEdges.h>
#include <vtkStripper.h>
#include <vtkAppendPolyData.h>
#include <vtkCenterOfMass.h>
#include <vtkDelaunay3D.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkPolyDataNormals.h>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>


// Initialize
typedef itk::Image<short, 3> ImageType;
typedef ImageType::Pointer ImagePointer;
typedef itk::ImageFileReader<ImageType> ImageReaderType;
typedef itk::ImageFileWriter<ImageType> ImageWriterType;



void WritePoly (vtkPolyData *p, std::string filename) {
  vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
  writer->SetInputData(p); 
  writer->SetFileName(filename.c_str());
  writer->Write();
}



bool isValid (float label) {
  if ((label > 0) && (label <= 3)) {
    return true;
  }
  return false;
}



float Merge(float a, float b) {
  if(!isValid(a)) { 
    return b;
  }
  if(!isValid(b)) {
    return a;
  }
  return a;
}



unsigned int MajorityVote(std::vector<unsigned int> votes) {
  unsigned int nVotes = votes.size();
  std::vector<unsigned int> candidates;
  std::vector<unsigned int> count;

  unsigned int nCandidates = 0 ;
  count.resize(nVotes,0);
  for ( unsigned int i = 0 ; i < nVotes ; i++ )
    {
      unsigned int current = votes[i] ;
      if ( ! isValid ( current ) ) 
	continue;
      bool found = false ;
      for ( unsigned int j = 0 ; j < nCandidates ; j++ )
	{
	  if ( current == candidates[j] ) 
	    {
	      found = true ; 
	      count[j]++ ;
	    }
	}
      if ( !found ) 
	{
	  candidates.push_back ( current ) ;
	  count[nCandidates]++ ;
	  nCandidates++ ;
	}
    }

  unsigned int maxVote = 0, choice = 0 ;
  for ( unsigned int i = 0 ; i < nCandidates ; i++ )
    {
      if ( count[i] > maxVote ) 
	{
	  maxVote = count[i] ;
	  choice = candidates[i] ;
	}
    }
  return choice ;
}

unsigned int FillHoles ( vtkPolyData *mesh, vtkSmartPointer<vtkFloatArray> labels ) 
{
  mesh->BuildLinks() ;
  unsigned int nPts = labels->GetNumberOfTuples() ;

  vtkSmartPointer <vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();

  unsigned int nFail = 0 ;
  for ( unsigned int p = 0 ; p < nPts ; p++ )
    {
      std::vector < unsigned int > candidates, neighbors, uniqueNeighbors ;
      float pixel = labels->GetValue ( p ) ;
      if ( !isValid ( pixel ) )
	{
	  // get neighbors 
	  neighbors.empty () ;
	  mesh->GetPointCells ( p, cellIds );
	  unsigned int nCells = cellIds->GetNumberOfIds();
	  for ( unsigned int i = 0; i < nCells; i++ )
	    {
	      vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
	      mesh->GetCellPoints ( cellIds->GetId(i), ptIds );
	      vtkIdType nPts = ptIds->GetNumberOfIds();
	      for ( unsigned j = 0; j < nPts; j++ )
		{
		  if ( ptIds->GetId(j) == p ) continue;
		  neighbors.push_back ( ptIds->GetId ( j ) ) ;
		}
	    }

	  unsigned int nNeighbors = 0 ;
	  for ( unsigned int i = 0 ; i < neighbors.size () ; i++ )
	    {
	      bool found = false ;
	      for ( unsigned int j = 0 ; j < nNeighbors ; j++ )
		{
		  if ( uniqueNeighbors[j] == neighbors[i] ) 
		    {
		      found = true ;
		      continue ;
		    }
		}
	      if ( ! found ) 
		{
		  uniqueNeighbors.push_back ( neighbors[i] ) ;
		  nNeighbors++ ;
		}
	    }
	  candidates.resize ( nNeighbors, 0 ) ;
	  for ( unsigned int n = 0 ; n < nNeighbors ; n++ )
	    {
	      candidates[n] = labels->GetValue ( uniqueNeighbors[n] ) ;
	    }
	  pixel = MajorityVote ( candidates ) ; 
	  labels->SetValue ( p, pixel ) ;
	  if ( !isValid ( pixel ) ) 
	    nFail++ ;
	}
    }

  return nFail ;
}



vtkSmartPointer<vtkFloatArray> AssignLabels(vtkPolyData *mesh, ImagePointer image) {
  mesh->BuildLinks();
  unsigned nPts = mesh->GetNumberOfPoints();
  std::cout << nPts << " points" << std::endl;

  vtkSmartPointer<vtkPolyDataNormals> normalGenerator = vtkSmartPointer<vtkPolyDataNormals>::New();
  normalGenerator->SetInputData(mesh);
  normalGenerator->ComputePointNormalsOn();
  normalGenerator->ComputeCellNormalsOn();
  normalGenerator->Update();
  vtkSmartPointer<vtkDataArray> normals = mesh->GetPointData()->GetNormals();

  vtkSmartPointer<vtkFloatArray> labels = vtkSmartPointer<vtkFloatArray>::New();
  labels->SetNumberOfComponents(1);
  labels->SetNumberOfValues(nPts);
  labels->SetName("Atrophy-Region");

  for(unsigned int p = 0; p < nPts; p++) {
    double vtkPnt[3];
    mesh->GetPoint(p,vtkPnt);

    itk::ContinuousIndex<float,3> current;
    ImageType::IndexType currentIndex;
    current[0] = -vtkPnt[0];
    current[1] = -vtkPnt[1];
    current[2] = vtkPnt[2];

    itk::ContinuousIndex<float,3> current_check;
    ImageType::IndexType currentIndex_check;
    current_check[0] = -(vtkPnt[0] - normals->GetComponent(p,0));
    current_check[1] = -(vtkPnt[1] - normals->GetComponent(p,1));
    current_check[2] = (vtkPnt[2] - normals->GetComponent(p,2));
    bool inside = image->TransformPhysicalPointToIndex(current,currentIndex);
    bool inside_check = image->TransformPhysicalPointToIndex(current_check,currentIndex_check);
 
    if(inside) {
      ImageType::PixelType pixel = image->GetPixel(currentIndex);
      if(pixel == 0) {
	//std::cout << "this should not happen often" << std::endl ;
      }
      // Check to see if we are just outside the desired label
      ImageType::PixelType pixel_check = image->GetPixel(currentIndex_check);
      if(pixel_check > pixel) {
	labels->SetValue(p,pixel_check);
      } else {
	labels->SetValue(p,pixel);
      }
    } else { 
      std::cout << "not inside. huh. " << std::endl;
    }
  }

  return labels;
}



int main(int argc, char *argv[])
{
  if ( argc < 6 )
    {
      std::cout << "Usage: " << argv[0] << " " << " inputWMMesh.vtk inputGMMesh.vtk inputLabelImage.nrrd outputWMMesh.vtk outputGMMesh.vtk centerImageOn" << std::endl;
      return EXIT_FAILURE;
    }

  std::string inputWMMeshFilename = argv[1];
  std::string inputGMMeshFilename = argv[2];
  std::string inputImageFilename = argv[3];
  std::string outputWMFilename = argv[4];
  std::string outputGMFilename = argv[5];


  // Figure out what type of files we're dealing with
  std::size_t wmPos = inputWMMeshFilename.find(".vt");
  std::string wmFiletype = inputWMMeshFilename.substr(wmPos,4);
  vtkSmartPointer<vtkPolyData> wmMesh = vtkSmartPointer<vtkPolyData>::New();

  if(wmFiletype.compare(".vtk")==0) {
    vtkSmartPointer<vtkPolyDataReader> wmMeshreader = vtkSmartPointer<vtkPolyDataReader>::New();
    wmMeshreader->SetFileName(inputWMMeshFilename.c_str());
    wmMeshreader->Update();
    wmMesh = wmMeshreader->GetOutput();
  } else if(wmFiletype.compare(".vtp")==0) {
    vtkSmartPointer<vtkXMLPolyDataReader> wmMeshreader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
    wmMeshreader->SetFileName(inputWMMeshFilename.c_str());
    wmMeshreader->Update();
    wmMesh = wmMeshreader->GetOutput(); 
  } else {
    std::cout << "WM filetype not detected." << std::endl;
  }

  std::size_t gmPos = inputGMMeshFilename.find(".vt");
  std::string gmFiletype = inputGMMeshFilename.substr(gmPos,4);
  vtkSmartPointer<vtkPolyData> gmMesh = vtkSmartPointer<vtkPolyData>::New();

  if(gmFiletype.compare(".vtk")==0) {
    vtkSmartPointer<vtkPolyDataReader> gmMeshreader = vtkSmartPointer<vtkPolyDataReader>::New();
    gmMeshreader->SetFileName(inputGMMeshFilename.c_str());
    gmMeshreader->Update();
    gmMesh = gmMeshreader->GetOutput();
  } else if(gmFiletype.compare(".vtp")==0) {
    vtkSmartPointer<vtkXMLPolyDataReader> gmMeshreader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
    gmMeshreader->SetFileName(inputGMMeshFilename.c_str());
    gmMeshreader->Update();
    gmMesh = gmMeshreader->GetOutput();
  } else {
    std::cout << "GM filetype not detected." << std::endl;
  }


  // read the input image
  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName(inputImageFilename.c_str());       
  imageReader->Update();
  ImagePointer inputImage = imageReader->GetOutput();


  // figure out which vertex gets which label
  vtkSmartPointer<vtkFloatArray> wmlabels = vtkSmartPointer<vtkFloatArray>::New() ;
  wmlabels = AssignLabels ( wmMesh, inputImage ) ;

  vtkSmartPointer<vtkFloatArray> gmlabels = vtkSmartPointer<vtkFloatArray>::New() ;
  gmlabels = AssignLabels ( gmMesh, inputImage ) ;

  unsigned int it = 0;
  unsigned int nHoles = 1;
  while ( nHoles = FillHoles ( gmMesh, gmlabels ) ) {
    it++;
    std::cout << nHoles << " holes" << std::endl ;
    if( it > 15 ) {
      break;
    }
  }
  
  it = 0;
  nHoles = 1;
  while ( nHoles = FillHoles ( wmMesh, wmlabels ) ) {
    it++;
    std::cout << nHoles << " holes" << std::endl ;
    if( it > 15 ) {
      break;
    }
  }


  // assign the same labels to the same-numbered vertices on the two surfaces
  wmMesh->GetPointData()->AddArray ( wmlabels ) ;
  gmMesh->GetPointData()->AddArray ( gmlabels ) ;

  vtkSmartPointer<vtkXMLPolyDataWriter> wmMeshwriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  wmMeshwriter->SetFileName ( outputWMFilename.c_str() );
  wmMeshwriter->SetInputData ( wmMesh ) ;
  wmMeshwriter->Update();

  vtkSmartPointer<vtkXMLPolyDataWriter> gmMeshwriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  gmMeshwriter->SetFileName ( outputGMFilename.c_str() );
  gmMeshwriter->SetInputData ( gmMesh ) ;
  gmMeshwriter->Update();

  return 0 ;
}
