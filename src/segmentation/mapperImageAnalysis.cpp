#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <iomanip>
#include <cstdlib> 

#include "HistologicalEntities.h"
#include "BoundaryFix.h"

#include <opencv2/opencv.hpp>
#include <opencv/highgui.h>
#include <opencv/cv.h>
#include "opencv2/core/core_c.h"
#include "opencv2/core/core.hpp"
#include "opencv2/flann/miniflann.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/contrib/contrib.hpp"

using namespace std;
using namespace cv;
using namespace nscale;

#define NUM_CHANNELS 3

string TAB = "\t";
string SEMI_COLON = ";";


//Struct of tile_meta_data
struct tileMetaData
{
	string tileName;
	int baseX;
	int baseY;
	int width; 
	int height;
	int buffer; //Size of tiling buffer
};


//Genereate raster data of a image tile
Mat getRasterData(string fileName, tileMetaData &tileData)
{
	int width = tileData.width;
	int height = tileData.height;

	const char * c = fileName.c_str();
	FILE * file = fopen(c, "rb");

	//Allocate memory for buffer
	int bufSize = height * width * NUM_CHANNELS;
	char *buffer = new char[bufSize + 2];
	int amtRead = fread(buffer, 1 , bufSize, file);

	//Might have to change to if-else to handle different # of channels
	return Mat(Size(width, height), CV_8UC3, buffer, 0); 
}


//Remove the isolated polygons in the buffer zone
void rmBufPolygon(stringstream &formatedStream, tileMetaData &tileData, stringstream &cleanStream)
{
	int coreX = tileData.baseX + tileData.buffer;
	int coreY = tileData.baseY + tileData.buffer;
	int coreWidth = tileData.width - 2 * tileData.buffer;
	int coreHeight = tileData.height - 2 * tileData.buffer;

	string line; 
	while (getline(formatedStream, line))
	{
		//cout << "line: " << line << endl; 
		int pos = line.find_first_of(";");

		string pointsString = line.substr(pos + 3, line.size() - 1);
		//cout << "pointsString: " << pointsString << endl;

		std::vector<Point> polygon;

		int posSpace = pointsString.find_first_of(" ");
		while (posSpace > 0) 
		{
			string point = pointsString.substr(0, posSpace);
			//cout << "pointSting: " << point << endl;
			int posComma = point.find(",");
			string xString = point.substr(0, posComma);
			string yString = point.substr(posComma + 1, point.size() - 1);
			int x = atoi(xString.c_str());
			int y = atoi(yString.c_str());
			//cout << "x: " << x << " y: " << y << endl;

			Point pt = Point(x, y);
			polygon.push_back(pt);

			pointsString = pointsString.substr(posSpace + 1, pointsString.size() - 1);
			posSpace = pointsString.find_first_of(" ");
		}

		//Last point is the same as the first point in a polygon
		polygon.push_back(polygon[0]);

		Rect mbb = boundingRect(polygon);

		if ((mbb.x > coreX && mbb.x < coreX + coreWidth && mbb.y > coreY && mbb.y < coreY + coreHeight) ||
			(mbb.x + mbb.width > coreX && mbb.x + mbb.width < coreX + coreWidth && mbb.y > coreY && mbb.y < coreY + coreHeight) ||
			(mbb.x > coreX && mbb.x < coreX + coreWidth && mbb.y + mbb.height > coreY && mbb.y + mbb.height < coreY + coreHeight) ||
			(mbb.x + mbb.width > coreX && mbb.x + mbb.width < coreX + coreWidth && mbb.y + mbb.height > coreY && mbb.y + mbb.height < coreY + coreHeight))
		{
			//Polygons that are entirely in the core zone
			if ((mbb.x > coreX) && (mbb.x + mbb.width < coreX + coreWidth) && mbb.y > coreY && (mbb.y + mbb.height < coreY + coreHeight))
			{
				cleanStream << line << "\tnon-boundary"<< endl;
			}
			else //Polygons that are on boundary
			{
				cleanStream << line << "\ton-boundary"<< endl;
			}
		}			

	} //end of while, one line is processed
}

void wkt_output(stringstream inputstream) {
   // for every object in inputstream
   //     format to WKT 
   //     output
}

void read_tile(Mat inputImg, string segmentMethod, tileMetaData tileData, int normOption, string outputFormat) 
{
	stringstream tmpInStream;
	stringstream outStream;		
	stringstream formatedStream;
	stringstream cleanStream; //stream withoutout polygons entirely in the buffer zone
	
	string tileName = tileData.tileName;
	int baseX = tileData.baseX;
	int baseY = tileData.baseY;
	int width = tileData.width;
	int height = tileData.height;


	cout << "Start Segmentation..." << endl;

	Mat outImg; //Output from segmentation
	int compcount;
	int *bbox = NULL;	

	if (segmentMethod.compare("watershed") == 0)
	{
		nscale::HistologicalEntities::segmentNuclei(inputImg, outImg, compcount, bbox);

		imwrite( "/home/cfeng/src/outputimage.jpg", outImg ); //Output a watershed mask image
	}


	cout << "Start Building Contours..." << endl;

	if (outImg.data > 0) {
		Mat temp = Mat::zeros(outImg.size() + Size(2,2), outImg.type());
		copyMakeBorder(outImg, temp, 1, 1, 1, 1, BORDER_CONSTANT, 0);

		std::vector<std::vector<Point> > contours;
		std::vector<Vec4i> hierarchy;

		findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE);
		
		int counter = 0;
		if (contours.size() > 0) {
			for (int idx = 0; idx >= 0; idx = hierarchy[idx][0]) {
				tmpInStream << idx << SEMI_COLON;
				formatedStream << tileName << "\t" << idx << "\t";
				for (unsigned int ptc = 0; ptc < contours[idx].size(); ++ptc) {
					tmpInStream << " " << baseX + contours[idx][ptc].x << "," << baseY + contours[idx][ptc].y;
					formatedStream << " " << baseX + contours[idx][ptc].x << "," << baseY + contours[idx][ptc].y;
				}
				tmpInStream << endl;
				formatedStream << endl;
			}
			++counter;
		}


		tmpInStream.clear(); //Clear the stream (reset eof and etc.)
		

		if (normOption > 0 && outputFormat.compare("WKT") == 0)
		{
			cout << "Start Boundary Fix1..." << endl;

			formatedStream.clear();
			formatedStream.str("");
			processBoundaryFixing(tmpInStream, tileName, outStream, formatedStream); //Boundary fix 1

			if (normOption > 1)
			{
				cout << "Start Boundary Fix2..." << endl;

				formatedStream.clear();
				formatedStream.str("");
				processBoundaryFixing2(outStream, tileName, baseX, baseY, formatedStream); //Boundary fix 2
			}
		}
		
	}


	rmBufPolygon(formatedStream, tileData, cleanStream);

	ofstream myFile("/home/cfeng/src/output.txt");
	//cout << "size " << finalStream.str().size() << endl;
	myFile << cleanStream.str();
 
  
  wkt_output(tmpOutputStream);
 
	myFile.close();
}



int main (int argc, char **argv){

	string images[3] = {"/home/cfeng/src/TCGA-12-0618-01Z-00-DX1.svs_28672_16384.pnm", "/home/cfeng/src/TCGA-12-0618-01Z-00-DX1.svs_28672_20480.pnm", "/home/cfeng/src/TCGA-12-0618-01Z-00-DX1.svs_28672_16384.pnm"};
	
	vector<string> imgFiles;
	vector<tileMetaData> tileData;

  // Retrieve meta data from environment variables/program parameters
  //
  //
  //
  struct tileMetaData metaparams;
  // Parse from argc and argv
  //
  //
 

  Mat inputMat;
  
  bool localTesting = true;
  
  if (localTesting) {

	//Create meta data for each sample tile image
 
  for (int i = 0; i < 3; ++i)
	{
		string fileName = images[i];

		tileMetaData newMetaData;
		
		int pos1 = fileName.find_last_of(".");
		int pos2 = fileName.find_last_of("_", pos1 - 1);
		int pos3 = fileName.find_last_of("_", pos2 - 1);
		int posSlash = fileName.find_last_of("/");
			
		newMetaData.tileName = fileName.substr(posSlash + 1, pos1);
		newMetaData.baseY = atoi(fileName.substr(pos2 + 1, pos1 - pos2 - 1).c_str());
		newMetaData.baseX = atoi(fileName.substr(pos3 + 1, pos2 - pos3 - 1).c_str());
		newMetaData.width = 4096;
		newMetaData.height = 4096;
		newMetaData.buffer = 512; //This buffer size is just for testing purpose

		tileData.push_back(newMetaData);
		imgFiles.push_back(images[i]);
	}

	//Print the base coordinated of an image tile
	cout << "baseX is " << tileData[0].baseX << endl;
	cout << "baseY is " << tileData[0].baseY << endl;



	//Generate the raster data of an image tile
	inputMat = getRasterData(imgFiles[0], tileData[0]);

  	/** Actual MapReduce
	fileName = getenv("mapred.input.file");
	width = atoi(getenv("imagewidth"));
	height = atoi(getenv("imageheight"));
	 */
   
  } else {
  
    // MapReduce retrieval
     
    string filename;
    
  	fileName = getenv("map_input_file");
    
  	int pos1 = fileName.find_last_of(".");
  	int pos2 = fileName.find_last_of("_", pos1 - 1);
  	int pos3 = fileName.find_last_of("_", pos2 - 1);
  
  	int posSlash = fileName.find_last_of("/");
  		
  	tileName = fileName.substr(posSlash + 1, pos1);
  	baseY = atoi(fileName.substr(pos3 + 1, pos2 - pos3 - 1).c_str());
  	baseX = atoi(fileName.substr(pos2 + 1, pos1 - pos2 - 1).c_str());
   
    	/* Allocate memory for buffer */
  	//buffer = (char *) calloc(bufSize + 2, sizeof(char));
  
  	buffer = new char[bufSize + 2];
  
  	int amtRead;
  	amtRead = fread(buffer, 1 , bufSize, stdin);
  
  	///* Read in standard input to buffer */
  	//if (fread(buffer, 1 , bufSize, stdin) != bufSize) {
  	//	cerr << "Error reading standard input" << endl;
  	//}
  
  	/* Might have to change to if-else to handle different # of channels*/
  	inputMat = Mat(Size(width, height), CV_8UC3, buffer, 0);

  }
   
	//The map function
	read_tile(inputMat, "watershed", tileData[0], 2, "WKT");

	return 0;
}