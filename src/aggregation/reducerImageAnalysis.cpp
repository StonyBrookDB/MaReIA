#include <iostream>
#include <cstring>
#include <cmath>
#include <map>
#include <cstdlib> 
#include <getopt.h>

// tokenizer 
#include "tokenizer.h"

// geos
#include <geos/geom/PrecisionModel.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/Point.h>
#include <geos/io/WKTReader.h>
#include <geos/io/WKTWriter.h>
#include <geos/opBuffer.h>

#include <spatialindex/SpatialIndex.h>

using namespace geos;
using namespace geos::io;
using namespace geos::geom;
using namespace geos::operation::buffer; 

using namespace SpatialIndex;

#define FillFactor 0.9
#define IndexCapacity 10 
#define LeafCapacity 50
#define COMPRESS true
#define NULL 0

// Constants
const int OSM_SRID = 4326;
const int ST_INTERSECTS = 1;
const int ST_TOUCHES = 2;
const int ST_CROSSES = 3;
const int ST_CONTAINS = 4;
const int ST_ADJACENT = 5;
const int ST_DISJOINT = 6;
const int ST_EQUALS = 7;
const int ST_DWITHIN = 8;
const int ST_WITHIN = 9;
const int ST_OVERLAPS = 10;

const int SID_1 = 1;
const int SID_2 = 2;

// separators for parsing
const string TAB = "\t";
const string SEP = "\x02"; // ctrl+a
const string BAR = "|";
const string DASH= "-";
const string COMMA = ",";
const string SPACE = " ";



vector<id_type> hits;

RTree::Data* parseInputPolygon(Geometry *p, id_type m_id);

class MyVisitor : public IVisitor
{
    public:
	void visitNode(const INode& n) {}
	void visitData(std::string &s) {}

	void visitData(const IData& d)
	{
	    hits.push_back(d.getIdentifier());
	    //std::cout << d.getIdentifier()<< std::endl;
	}

	void visitData(std::vector<const IData*>& v) {}
	void visitData(std::vector<uint32_t>& v){}
};


class GEOSDataStream : public IDataStream
{
    public:
	GEOSDataStream(map<int,Geometry*> * inputColl ) : m_pNext(0), len(0),m_id(0)
    {
	if (inputColl->empty())
	    throw Tools::IllegalArgumentException("Input size is ZERO.");
	shapes = inputColl;
	len = inputColl->size();
	iter = shapes->begin();
	readNextEntry();
    }
	virtual ~GEOSDataStream()
	{
	    if (m_pNext != 0) delete m_pNext;
	}

	virtual IData* getNext()
	{
	    if (m_pNext == 0) return 0;

	    RTree::Data* ret = m_pNext;
	    m_pNext = 0;
	    readNextEntry();
	    return ret;
	}

	virtual bool hasNext()
	{
	    return (m_pNext != 0);
	}

	virtual uint32_t size()
	{
	    return len;
	    //throw Tools::NotSupportedException("Operation not supported.");
	}

	virtual void rewind()
	{
	    if (m_pNext != 0)
	    {
		delete m_pNext;
		m_pNext = 0;
	    }

	    m_id  = 0;
	    iter = shapes->begin();
	    readNextEntry();
	}

	void readNextEntry()
	{
	    if (iter != shapes->end())
	    {
		//std::cerr<< "readNextEntry m_id == " << m_id << std::endl;
		m_id = iter->first;
		m_pNext = parseInputPolygon(iter->second, m_id);
		iter++;
	    }
	}

	RTree::Data* m_pNext;
	map<int,Geometry*> * shapes; 
	map<int,Geometry*>::iterator iter; 

	int len;
	id_type m_id;
};


RTree::Data* parseInputPolygon(Geometry *p, id_type m_id) {
    double low[2], high[2];
    const Envelope * env = p->getEnvelopeInternal();
    low [0] = env->getMinX();
    low [1] = env->getMinY();

    high [0] = env->getMaxX();
    high [1] = env->getMaxY();

    Region r(low, high, 2);

    return new RTree::Data(0, 0 , r, m_id);// store a zero size null poiter.
}


// data type declaration 
map<int, std::vector<Geometry*> > polydata;
map<int, std::vector<string> > rawdata;
ISpatialIndex * spidx = NULL;
IStorageManager * storage = NULL;


void init();
void print_stop();
int joinBucket();
void releaseShapeMem(const int k);

bool extractParams(int argc, char** argv );
void ReportResult( int i , int j);
string project( vector<string> & fields, int sid);

void freeObjects();
bool buildIndex(map<int,Geometry*> & geom_polygons);


int selfJoin()
{
  string input_line;
  vector<string> fields;
  PrecisionModel *pm = new PrecisionModel();
  GeometryFactory *gf = new GeometryFactory(pm,OSM_SRID);
  WKTReader *wkt_reader = new WKTReader(gf);
  Geometry *poly = NULL;
  Geometry *buffer_poly = NULL;

  int object_counter = 0;
  // parse the cache file from Distributed Cache
  std::ifstream skewFile(cacheFile);
  while (std::getline(skewFile, input_line))
  {
    tokenize(input_line, fields, TAB, true);
    if (fields[stop.shape_idx_2].size() < 4) // this number 4 is really arbitrary
      continue ; // empty spatial object 

    try { 
      poly = wkt_reader->read(fields[stop.shape_idx_2]);
    }
    catch (...) {
      std::cerr << "******Geometry Parsing Error******" << std::endl;
      return -1;
    }

    if (ST_DWITHIN == stop.JOIN_PREDICATE && stop.expansion_distance != 0.0 )
    {
      buffer_poly = poly ;
      poly = BufferOp::bufferOp(buffer_poly,stop.expansion_distance,0,BufferParameters::CAP_SQUARE);
      delete buffer_poly ;
    }

    polydata[SID_2].push_back(poly);
    //--- a better engine implements projection 
    rawdata[SID_2].push_back(stop.proj2.size()>0 ? project(fields,SID_2) : input_line); 

    fields.clear();
  }
  skewFile.close();
  // update predicate to st_intersects
  if (ST_DWITHIN == stop.JOIN_PREDICATE )
    stop.JOIN_PREDICATE = ST_INTERSECTS ;

  // parse the main dataset 
  object_counter = 0;
  while(cin && getline(cin, input_line) && !cin.eof()) {
    tokenize(input_line, fields, TAB, true);
    //cerr << "Shape size: " << fields[stop.shape_idx_1].size()<< endl;
    if (fields[stop.shape_idx_1].size() < 4) // this number 4 is really arbitrary
      continue ; // empty spatial object 


   //  TBI
   // Check for object class and output non-boundary objects first
   
   
   

    try { 
      poly = wkt_reader->read(fields[stop.shape_idx_1]);
    }
    catch (...) {
      std::cerr << "******Geometry Parsing Error******" << std::endl;
      return -1;
    }

    if (object_counter++ % OBJECT_LIMIT == 0 && object_counter >1) {
      int  pairs = joinBucket();
      std::cerr <<rawdata[SID_1].size() << "|x|" << rawdata[SID_2].size() << "|=|" << pairs << "|" <<std::endl;
      releaseShapeMem(1);
    }

    // populate the bucket for join 
    polydata[SID_1].push_back(poly);
    rawdata[SID_1].push_back(stop.proj1.size()>0 ? project(fields,SID_1) : input_line); 

    fields.clear();
  }

  // last batch 
  int  pairs = joinBucket();
  std::cerr <<rawdata[SID_1].size() << "|x|" << rawdata[SID_2].size() << "|=|" << pairs << "|" <<std::endl;
  releaseShapeMem(stop.join_cardinality);

  // clean up newed objects
  delete wkt_reader ;
  delete gf ;
  delete pm ;

  return object_counter;
}

void releaseShapeMem(const int k ){
  if (k <=0)
    return ;
  for (int j =0 ; j <k ;j++ )
  {
    int delete_index = j+1 ;
    int len = polydata[delete_index].size();

    for (int i = 0; i < len ; i++) 
      delete polydata[delete_index][i];

    polydata[delete_index].clear();
    rawdata[delete_index].clear();
  }
}

bool join_with_predicate(const Geometry * geom1 , const Geometry * geom2, 
        const Envelope * env1, const Envelope * env2,
        const int jp){
  bool flag = false ; 
//  const Envelope * env1 = geom1->getEnvelopeInternal();
//  const Envelope * env2 = geom2->getEnvelopeInternal();
  BufferOp * buffer_op1 = NULL ;
  BufferOp * buffer_op2 = NULL ;
  Geometry* geom_buffer1 = NULL;
  Geometry* geom_buffer2 = NULL;
 
  switch (jp){

    case ST_INTERSECTS:
      flag = env1->intersects(env2) && geom1->intersects(geom2);
      break;

    case ST_TOUCHES:
      flag = geom1->touches(geom2);
      break;

    case ST_CROSSES:
      flag = geom1->crosses(geom2);
      break;

    case ST_EQUALS:
      flag = env1->equals(env2) && geom1->equals(geom2);
      break;

    case ST_DWITHIN:
      buffer_op1 = new BufferOp(geom1);
      // buffer_op2 = new BufferOp(geom2);
      if (NULL == buffer_op1)
        cerr << "NULL: buffer_op1" <<endl;

      geom_buffer1 = buffer_op1->getResultGeometry(stop.expansion_distance);
      // geom_buffer2 = buffer_op2->getResultGeometry(expansion_distance);
      //Envelope * env_temp = geom_buffer1->getEnvelopeInternal();
      if (NULL == geom_buffer1)
        cerr << "NULL: geom_buffer1" <<endl;

      flag = join_with_predicate(geom_buffer1,geom2, env1, env2, ST_INTERSECTS);
      break;

    case ST_WITHIN:
      flag = geom1->within(geom2);
      break; 

    case ST_OVERLAPS:
      flag = geom1->overlaps(geom2);
      break;
      

    default:
      std::cerr << "ERROR: unknown spatial predicate " << endl;
      break;
  }
  return flag; 
}


string project( vector<string> & fields, int sid) {
  std::stringstream ss;
  switch (sid){
    case 1:
      for (int i =0 ; i <stop.proj1.size();i++)
      {
        if ( 0 == i )
          ss << fields[stop.proj1[i]] ;
        else
        {
          if (stop.proj1[i] < fields.size())
            ss << TAB << fields[stop.proj1[i]];
        }
      }
      break;
    case 2:
      for (int i =0 ; i <stop.proj2.size();i++)
      {
        if ( 0 == i )
          ss << fields[stop.proj2[i]] ;
        else{ 
          if (stop.proj2[i] < fields.size())
            ss << TAB << fields[stop.proj2[i]];
        }
      }
      break;
    default:
      break;
  }

  return ss.str();
}

void ReportResult( int i , int j)
{
  switch (stop.join_cardinality){
    case 1:
      cout << rawdata[SID_1][i] << SEP << rawdata[SID_1][j] << endl;
      break;
    case 2:
      cout << rawdata[SID_1][i] << SEP << rawdata[SID_2][j] << endl; 
      break;
    default:
      return ;
  }
}

int joinBucket() 
{
  double low[2], high[2];
  // cerr << "---------------------------------------------------" << endl;
  int pairs = 0;
  bool selfjoin = stop.join_cardinality ==1 ? true : false ;
  int idx1 = SID_1 ; 
  int idx2 = selfjoin ? SID_1 : SID_2 ;

  // for each tile (key) in the input stream 
  try { 

    std::vector<Geometry*>  & poly_set_one = polydata[idx1];
    
    int len1 = poly_set_one.size();
    
    map<int,Geometry*> geom_polygons;
    for (int j = 0; j < len1; j++) {
        geom_polygons[j] = poly_set_one[j];
    }
    
    // build spatial index for input polygons 
    bool ret = buildIndex(geom_polygons);
    if (ret == false) {
        return -1;
    }
    // cerr << "len1 = " << len1 << endl;
    // cerr << "len2 = " << len2 << endl;
    bool* joinedElem = new bool[len1]();
    for (int j = 0; j < len1; j++) {
        joinedElem[j] = false;
    }
    
    for (int i = 0; i < len1; i++) {
        if (!joinedElem[i]) {
          joinedElem[i] = true;
             
          const Geometry* geom1 = poly_set_one[i];
          const Envelope * env1 = geom1->getEnvelopeInternal();
          low[0] = env1->getMinX();
          low[1] = env1->getMinY();
          high[0] = env1->getMaxX();
          high[1] = env1->getMaxY();
          /* Handle the buffer expansion for R-tree */
          if (stop.JOIN_PREDICATE == ST_DWITHIN) {
              low[0] -= stop.expansion_distance;
              low[1] -= stop.expansion_distance;
              high[0] += stop.expansion_distance;
              high[1] += stop.expansion_distance;
          }
          
          Region r(low, high, 2);
          hits.clear();
          MyVisitor vis;
          spidx->intersectsWithQuery(r, vis);
          //cerr << "j = " << j << " hits: " << hits.size() << endl;
          for (uint32_t j = 0 ; j < hits.size(); j++ ) 
          {
              if (hits[j] == i && selfjoin) {
                  continue;
              }
              const Geometry* geom2 = poly_set_one[hits[j]];
              const Envelope * env2 = geom2->getEnvelopeInternal();
              if (join_with_predicate(geom1, geom2, env1, env2,
                      ST_INTERSECTS))  {
                 const Geometry * newtemp = geom1->Union(geom2);
                 // replace the old geometry
                 geom_polygons[i] = newtemp;
                 joinedElem[i] = true;
                 // remove the old geometry
                 
                 
                 poly_set_one[i] = NULL;
              }
          }
          
        }
    }
    
    // TBI Output the final geometries
    //
    for (int i = 0; i < len1; i++) {
      if (poly_set_one[i] != NULL) {
        // output the geometry
      }
    }
    
  } // end of try
  //catch (Tools::Exception& e) {
  catch (...) {
    std::cerr << "******ERROR******" << std::endl;
    //std::string s = e.what();
    //std::cerr << s << std::endl;
    return -1;
  } // end of catch
  return pairs ;
}


bool buildIndex(map<int,Geometry*> & geom_polygons) {
    // build spatial index on tile boundaries 
    id_type  indexIdentifier;
    GEOSDataStream stream(&geom_polygons);
    storage = StorageManager::createNewMemoryStorageManager();
    spidx   = RTree::createAndBulkLoadNewRTree(RTree::BLM_STR, stream, *storage, 
	    FillFactor,
	    IndexCapacity,
	    LeafCapacity,
	    2, 
	    RTree::RV_RSTAR, indexIdentifier);

    // Error checking 
    return spidx->isIndexValid();
}


void freeObjects() {
    // garbage collection
    delete spidx;
    delete storage;
}


void usage(){
  cerr  << endl << "Usage: aggregate [OPTIONS]" << endl << "OPTIONS:" << endl;
}

// main body of the engine
int main(int argc, char** argv)
{
  // initilize query operator 
  init();

  int c = 0 ;

  c = selfJoin();
  
  freeObjects();
  cout.flush();
  cerr.flush();

  return 0;
}