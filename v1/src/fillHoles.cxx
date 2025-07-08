#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkPointData.h>
#include <vtkPolyDataReader.h>
#include <vtkFloatArray.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkIdTypeArray.h>
#include <vtkIdList.h>
#include <vtkTriangleFilter.h>
#include <vtkCellArray.h>
#include <vtkSelectEnclosedPoints.h>
#include <queue>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

typedef itk::Image <short, 3> ImageType;
typedef ImageType::Pointer ImagePointer;
typedef itk::ImageFileReader<ImageType> ImageReaderType;
typedef itk::ImageFileWriter<ImageType> ImageWriterType;

ImagePointer FillContours ( vtkPolyData *holes, ImagePointer inputImage, short pixelValue );

double Cross(double x[3] , double y[3]) 
{
  double z[3];
  z[0] = ( x[1] * y[2] ) - ( x[2] * y[1] );
  z[1] = ( x[2] * y[0] ) - ( x[0] * y[2] );
  z[2] = ( x[0] * y[1] ) - ( x[1] * y[0] );
  double sum = 0 ;
  for ( int i = 0 ; i < 3 ; i++ )
    sum += z[i] * z[i] ;
  sum = sqrt ( sum ) ;
  if ( z[0] < 0 ) sum *= -1 ;
  //std::cout << z[0]/sum << " " << z[1]/sum << " " << z[2]/sum << std::endl ;
  return sum;
}

vtkIdList * GetVertNeighbors ( vtkPolyData *mesh, vtkIdType vert )
{
  vtkSmartPointer <vtkIdList> neighbors = vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer <vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
  mesh->GetPointCells ( vert, cellIds );

  unsigned int nCells = cellIds->GetNumberOfIds();
  for ( unsigned int i = 0; i < nCells; i++ )
    {
      vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
      mesh->GetCellPoints ( cellIds->GetId(i), ptIds );
      vtkIdType nPts = ptIds->GetNumberOfIds();
      for ( unsigned j = 0; j < nPts; j++ )
	{
	  if ( ptIds->GetId(j) == vert ) continue;
	  neighbors->InsertUniqueId ( ptIds->GetId(j) );
	}
    }
  return neighbors;
}

vtkPolyData * FillHoles ( vtkPolyData *dirtyMesh )
{
  vtkPolyData *cleanMesh = dirtyMesh;

  unsigned int nVerts = cleanMesh->GetNumberOfPoints();
  std::cout << "nVerts: " << nVerts << std::endl ;

  vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
  scalars->SetNumberOfComponents ( 1 );
  scalars->SetNumberOfValues ( nVerts );

  std::vector < unsigned short > visited ;
  visited.resize ( nVerts, 0 );

  unsigned int nVisited = 0;
  unsigned int nComponents = 0;
  vtkIdType currentVert = 0;

  std::queue < vtkIdType > toVisit;
  std::vector < unsigned int > componentSizes;

  cleanMesh->BuildCells();
  cleanMesh->BuildLinks();

  while ( nVisited < nVerts )
    {
      std::cout << "new component, so far visited " << nVisited << std::endl ;
      nComponents++;

      // find a new component
      for ( unsigned temp = 0 ; temp < nVerts ; temp++ )
	{
	  if ( !visited[temp] )
	    {
	      currentVert = temp ;
	      break;
	    }
	}
      std::cout << "Current Vert " << currentVert << std::endl ;
      toVisit.push ( currentVert ) ;
      while ( !toVisit.empty() )
	{
	  // visit the pop
	  currentVert = toVisit.front ();
	  toVisit.pop();

	  // nothing to do if we've already visited this point
	  if ( visited[currentVert] ) continue;

	  // put all neighbors of currentVert into queue
	  vtkIdList *currentNeighbors = GetVertNeighbors ( cleanMesh, currentVert );
	  for ( unsigned int n = 0 ; n < currentNeighbors->GetNumberOfIds(); n++ )
	    {
	      toVisit.push ( currentNeighbors->GetId ( n ) );
	    }
	  // std::cout << "done w vert# " << currentVert << std::endl ;
	  visited[currentVert] = nComponents ;
	  nVisited++;
	}
      std::cout << "done w component" << std::endl ;
      //      componentSizes.push_back ( nVisited );
      std::cout << nVisited << std::endl;
    }
  for ( unsigned int v = 0 ; v < nVerts; v++ )
    {
      scalars->SetValue ( v, visited[v] );
    }
  std::cout << "hole filling complete. " << std::endl ;
  cleanMesh->GetPointData()->SetScalars ( scalars ) ;

  return cleanMesh ;
}
 
int main(int argc, char *argv[])

  if ( argc < 4 )
    {
      std::cout << "Usage: " << argv[0] << " " << " inputHoles.vtk inputReferenceImage.nrrd outputImage.nrrd pixelValue" << std::endl;
      return EXIT_FAILURE;
    }

  std::string inputHolesFilename = argv[1];
  std::string inputImageFilename = argv[2];
  std::string outputFilename = argv[3];
  short pixelValue = atoi ( argv[4] ) ;

  vtkSmartPointer<vtkPolyDataReader> holesreader = vtkSmartPointer<vtkPolyDataReader>::New();
  holesreader->SetFileName(inputHolesFilename.c_str());
  holesreader->Update();
  vtkSmartPointer<vtkPolyData> inputHoles = holesreader->GetOutput();

  // read the input image
  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName(inputImageFilename.c_str());       
  imageReader->Update();
  ImagePointer inputImage = imageReader->GetOutput();

  ImagePointer outputImage = FillContours ( inputHoles, inputImage, pixelValue ) ;

  ImageWriterType::Pointer writer = ImageWriterType::New();
  writer->SetFileName ( outputFilename.c_str() );
  writer->SetInput ( outputImage );
  writer->Update();
  return EXIT_SUCCESS;
}

ImagePointer FillContours ( vtkPolyData *holes, ImagePointer inputImage, short pixelValue )
{
  unsigned int nContour = holes->GetNumberOfCells();
  std::cout << nContour << std::endl ;

  vtkIdList *pts = vtkIdList::New();

  holes->BuildCells();

  // start filling the holes
  for ( unsigned int c = 0; c < nContour; c++ )
    {
      pts->Reset();
      holes->GetCellPoints ( c, pts );
      unsigned int nPts = pts->GetNumberOfIds();

      // first compute the center of gravity for this contour
      double vtkPnt[3], cog[3];
      cog[0] = cog[1] = cog[2] = 0;
      for ( unsigned int v = 0 ; v < nPts; v++ )
	{
	  holes->GetPoint ( pts->GetId ( v ), vtkPnt );

	  for ( int i = 0 ; i < 3 ; i++ )
	    {
	      cog[i] += vtkPnt[i];
	    }
	}
      for ( int i = 0 ; i < 3 ; i++ )
	cog[i] /= nPts ;

      // now, from each point on the contour, draw a line to the COG
      double dir[3], pt[3], dist ;
      double stepSize = 0.05 ;
      for ( unsigned int v = 0 ; v < nPts ; v++ )
	{
	  holes->GetPoint ( pts->GetId ( v ), vtkPnt );
	  dist = 0;
	  for ( int i = 0 ; i < 3 ; i++ )
	    {
	      dir[i] = vtkPnt[i] - cog[i] ;
	      dist += dir[i] * dir[i] ;
	    }
	  dist = sqrt ( dist );
	  for ( int i = 0 ; i < 3 ; i++ )
	    {
	      dir[i] /= dist;
	      pt[i] = vtkPnt[i];
	    }
	  int nSteps = dist / stepSize;

	  for ( unsigned int i = 0 ; i < nSteps ; i++ )
	    {
	      itk::ContinuousIndex < float, 3 > current ;
	      ImageType::IndexType currentIndex ;
	      current[0] = -pt[0] ;
	      current[1] = -pt[1] ;
	      current[2] = pt[2] ;
	      bool inside = inputImage->TransformPhysicalPointToIndex ( current, currentIndex ) ;
	      if ( inside )
		inputImage->SetPixel ( currentIndex, pixelValue ) ;
	      else 
		std::cout << "not inside. huh. " << std::endl ;
	      for ( unsigned int j = 0 ; j < 3 ; j++ )
		pt[j] -= dir[j] * stepSize;
	    }
	}
    }

  return inputImage ;
}
