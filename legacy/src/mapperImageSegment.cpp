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

int main (int argc, char **argv){
	int *bbox = NULL;
	int compcount;
	int bufSize;
	int width; /* Width of image (maxX - minX) */
	int height; /* Height of image (maxY - minY)*/
	int baseX;
	int baseY;
	string fileName;
	string tileName;
	
	stringstream tmpInStream;
	stringstream tmpOutStream;

	char *buffer;
	Mat inputImg; /* InputImg */
	Mat outImg; /* Output from segmentation */

	/* Retrieve the base coordinates from filename */

	//fileName = "image1002.0004096.004096.dat";

	width = 4096;
	height = 4096;

	fileName = getenv("map_input_file");


	/** Actual MapReduce
	fileName = getenv("mapred.input.file");
	width = atoi(getenv("imagewidth"));
	height = atoi(getenv("imageheight"));
	 */
	int pos1 = fileName.find_last_of(".");
	int pos2 = fileName.find_last_of("_", pos1 - 1);
	int pos3 = fileName.find_last_of("_", pos2 - 1);

	int posSlash = fileName.find_last_of("/");
		
	tileName = fileName.substr(posSlash + 1, pos1);
	baseY = atoi(fileName.substr(pos3 + 1, pos2 - pos3 - 1).c_str());
	baseX = atoi(fileName.substr(pos2 + 1, pos1 - pos2 - 1).c_str());

	bufSize = height * width * NUM_CHANNELS;

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
	inputImg = Mat(Size(width, height), CV_8UC3, buffer, 0);
	
	/*
	namedWindow( "Display Image", CV_WINDOW_AUTOSIZE );
	imshow( "Display Image", inputImg );
	waitKey(0);
	*/
	
	nscale::HistologicalEntities::segmentNuclei(inputImg, outImg, compcount, bbox);


	if (outImg.data > 0) {
		Mat temp = Mat::zeros(outImg.size() + Size(2,2), outImg.type());
		copyMakeBorder(outImg, temp, 1, 1, 1, 1, BORDER_CONSTANT, 0);

		std::vector<std::vector<Point> > contours;
		std::vector<Vec4i> hierarchy;

		/* Find the contour */
		findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE);
		
		int counter = 0;
		if (contours.size() > 0) {
			for (int idx = 0; idx >= 0; idx = hierarchy[idx][0]) {
				tmpInStream << idx << SEMI_COLON;
				for (unsigned int ptc = 0; ptc < contours[idx].size(); ++ptc) {
					tmpInStream << " " << baseX + contours[idx][ptc].x << "," << baseY + contours[idx][ptc].y;
				}
				tmpInStream << endl;
			}
			++counter;
		}

		/* Clear the stream (reset eof and etc.) */
		tmpInStream.clear();

		//cout << "done segmenting: length stream: " << tmpInStream.str().size()  << endl;
		
		/* Boundary fix 1 */
		processBoundaryFixing(tmpInStream, tmpOutStream);

		// cout << "done fixing 1: length stream" << tmpOutStream.str().size() << endl;

		/* Output boundary fix 2 to std output */
		processBoundaryFixing2(tmpOutStream,tileName, baseX, baseY);
	}


	return 0;
}
