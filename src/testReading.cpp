#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
// #include <boost/lexical_cast.hpp>
#include <cstdlib>

using namespace std;

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
	
	char *buffer;
	/* Retrieve the base coordinates from filename */

	//fileName = "TCGA-06-0122-01Z-00-DX3.svs_12288_0.pnm";

	fileName = getenv("map_input_file");

	cout << "1\tfilename: " << fileName << endl;
	//width = atoi(getenv("imagewidth"));
	//height= atoi(getenv("imageheight"));
	width = 4096;
	height = 4096;

	cout << "1\twidth: " << width << endl;
	cout << "1\theight: " << height << endl;

	int pos1 = fileName.find_last_of(".");
	int pos2 = fileName.find_last_of("_", pos1 - 1);
	int pos3 = fileName.find_last_of("_", pos2 - 1);
		
	tileName = fileName.substr(0, pos1);
	baseY = atoi(fileName.substr(pos3 + 1, pos2 - pos3 - 1).c_str());
	baseX = atoi(fileName.substr(pos2 + 1, pos1 - pos2 - 1).c_str());

	bufSize = height * width * NUM_CHANNELS;

	/* Allocate memory for buffer */
	//buffer = (char *) calloc(bufSize + 2, sizeof(char));
	buffer = new char[bufSize + 2];

	/*
	if (fread(buffer, 1 , bufSize, stdin) != bufSize) {
		cerr << "Error reading standard input" << endl;
	}	
	*/
	fread(buffer, 1 , bufSize, stdin);

	cout << "0\tReading done" << endl;
	cout << "1\tShould have finished" << endl;
	cout << "2\t" << (int) buffer[0] << " " << (int) buffer[1] << endl;
	return 0;
}
