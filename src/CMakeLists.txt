cmake_minimum_required(VERSION 2.8)
project( testcv )
find_package( OpenCV REQUIRED )
include_directories(common PAISBoundaryFixer /home/hoang/Downloads/boost_1_48_0 )

add_executable( mapperImageSegment
	mapperImageSegment.cpp
	HistologicalEntities.cpp 
	ConnComponents.cpp
	HistologicalEntities.cpp
	HistologicalEntitiesGPU.cpp
	mapperImageSegment.cpp
	MorphologicOperations.cpp
	MorphologicOperationsGPU.cpp
	NeighborOperations.cpp
	NeighborOperationsGPU.cpp
	PixelOperations.cpp
	PixelOperationsGPU.cpp
	ScanlineOperations.cpp
	UtilsCVImageIO.cpp
	PAISBoundaryFixer/BoundaryFixFunction.cpp
	PAISBoundaryFixer/BoundaryFixFunction2.cpp
	PAISBoundaryFixer/clipper.cpp
)
target_link_libraries( mapperImageSegment ${OpenCV_LIBS} )
