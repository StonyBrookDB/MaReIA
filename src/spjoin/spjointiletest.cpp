#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace std;

bool intersectsWithTile(int minX, int minY, int maxX, int maxY,
			int tileMinX, int tileMinY, int tileMaxX, int tileMaxY)
{
	return !(minX > tileMaxX || maxX < tileMinX
                                || maxY < tileMinY || minY > tileMaxY);
}

