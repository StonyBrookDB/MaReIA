#include "clipper.h"
#include <iomanip>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <stdio.h>
#include <ios>
#include <map>
#include <fstream>
#include <vector>
#include <list>
#include <set>
#include <fts.h>
#include <sstream>

using namespace std;

void processBoundaryFixing(stringstream &text, string tileID, stringstream &outstream, stringstream &formatedStream);
void processBoundaryFixing2(stringstream &instream, string tileID, int baseX, int baseY, stringstream &outstream);

struct points
    {
        float x;
        float y;
    };



