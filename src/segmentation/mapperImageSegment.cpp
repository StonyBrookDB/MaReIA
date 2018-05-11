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


//remove the isolated polygons in the buffer zone
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

		if ((mbb.x > coreX) && (mbb.x + mbb.width < coreX + coreWidth) && mbb.y > coreY && (mbb.y + mbb.height < coreY + coreHeight))
		{
			cleanStream << line << endl;
		}

	} //end of while, one line is processed
}



void read_tile(vector<string> imgFiles, string segmentMethod, vector<tileMetaData> tileData, int normOption, string outputFormat) 
{
	stringstream cleanStream; //stream withoutout polygons in buffer zone
	stringstream finalStream; //stream of cleanStream of tiles

	for (int i = 0; i < imgFiles.size(); ++i)
	{
		string tileName = tileData[i].tileName;
		int baseX = tileData[i].baseX;
		int baseY = tileData[i].baseY;
		int width = tileData[i].width;
		int height = tileData[i].height;

		string fileName = imgFiles[i]; 
		const char * c = fileName.c_str();
		FILE * file = fopen(c, "rb");

		//Allocate memory for buffer
		int bufSize = height * width * NUM_CHANNELS;
		char *buffer = new char[bufSize + 2];
		int amtRead = fread(buffer, 1 , bufSize, file);

		//Might have to change to if-else to handle different # of channels
		Mat inputImg = Mat(Size(width, height), CV_8UC3, buffer, 0); 


		cout << "Start Segmentation..." << endl;

		Mat outImg; //Output from segmentation
		int compcount;
		int *bbox = NULL;	

		if (segmentMethod.compare("watershed") == 0)
		{
			nscale::HistologicalEntities::segmentNuclei(inputImg, outImg, compcount, bbox);

			imwrite( "/home/cfeng/src/outputimage.jpg", outImg ); //Output a mask image
		}


		cout << "Start Building Contours..." << endl;

		stringstream tmpInStream;
		stringstream outStream;		
		stringstream formatedStream;

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
			
			/*
			if (normOption == 0)
			{
				outStream << tmpInStream.rdbuf();
			}
			*/

			if (normOption > 0 && outputFormat.compare("WKT") == 0)
			{
				std::cout << "Start Boundary Fix1..." << std::endl;

				formatedStream.clear();
				formatedStream.str("");
				processBoundaryFixing(tmpInStream, tileName, outStream, formatedStream); //Boundary fix 1

				if (normOption > 1)
				{
					std::cout << "Start Boundary Fix2..." << std::endl;

					formatedStream.clear();
					formatedStream.str("");
					processBoundaryFixing2(outStream, tileName, baseX, baseY, formatedStream); //Boundary fix 2
				}
			}
			
		}


		rmBufPolygon(formatedStream, tileData[i], cleanStream);


		//cout << "outStreamsize: " << outStream.str().size() << endl;
		finalStream << cleanStream.str();

	}

	ofstream myFile("/home/cfeng/src/output.txt");
	cout << "size " << finalStream.str().size() << endl;
	myFile << finalStream.str();
	myFile.close();
}



int main (int argc, char **argv){

	vector<string> imgFiles;
	vector<tileMetaData> tileData;

	int n = atoi(argv[1]); // n should be 1, 2 or 3 since we only have 3 sample tiles

	string images[3] = {"/home/cfeng/src/TCGA-12-0618-01Z-00-DX1.svs_28672_16384.pnm", "TCGA-12-0618-01Z-00-DX1.svs_28672_20480.pnm", "/home/cfeng/src/TCGA-12-0618-01Z-00-DX1.svs_28672_16384.pnm"};

	//Create meta data for each tile
	for (int i = 0; i < n; ++i)
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

		cout << "baseX is " << newMetaData.baseX << endl;
		cout << "baseY is " << newMetaData.baseY << endl;

		tileData.push_back(newMetaData);
		imgFiles.push_back(images[i]);
	}

	read_tile(imgFiles, "watershed", tileData, 2, "WKT");

	return 0;
}