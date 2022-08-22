#ifndef SUMOMAP_GRAPH_H
#define SUMOMAP_GRAPH_H

#include <vector>
#include <stdlib.h>
#include <map>
#include <time.h>

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

namespace ns3 {

class EdgeSumoMap //edge for Sumo - Simulation of Urban MObility
{
public:
  /* encapsulate : change to private */
  bool enable;
  int src, dest;
  double weight;
  std::string nameId, m_from, m_to;

  /*getters & setters - (future versions) code refactoring to encapsulate*/

  EdgeSumoMap (int s, int d, double w)
  {
    src = s;
    dest = d;
    weight = w;
    nameId.erase ();
    m_from.erase ();
    m_to.erase ();
  }

  EdgeSumoMap ()
  {
    src = -1; //note: -1 = disable
    dest = -1;
    weight = 0;
    nameId.erase ();
    m_from.erase ();
    m_to.erase ();
  }
};

//
class VertexSumoMap //vertex for Sumo - Simulation of Urban MObility
{
public:
  /* (future versions) encapsulate : change to private */
  int m_id;
  std::string m_nameId;

  /* getters & setters*/

  VertexSumoMap (int id, std::string name)
  {
    m_id = id;
    m_nameId = name;
  }
};

class GraphSumoMap : public Object
{
private:
  int size;
  int minDistance (std::vector<int> &dist, std::vector<bool> &sptSet);

  void printPath (std::vector<int> &parent, int j);
  void printSolution (std::vector<int> &dist, std::vector<int> &parent, int src);
  void toStringAdjMatrix ();

public:
  GraphSumoMap ();
  bool init (std::vector<VertexSumoMap *> vertices, std::vector<EdgeSumoMap *> edges);
  void dijkstra (int src, int dst, double &pathWeight, std::vector<std::string> &path);

  //vector of vectors to represent (make) an graph adjacency matrix
  std::vector<std::vector<EdgeSumoMap *>> adjMatrix; //"matrix"
  std::vector<VertexSumoMap *> m_vertices;

  std::string findVertexNameById (int id);
  int findVertexIdByName (std::string name);

  int getDstVertexIdByEdgeName (std::string edgeName);
  void getAllEdgeWeights (std::map<std::string, double> &edgeWeights);
  void setAllEdgeWeights (std::map<std::string, double> &edgeWeights);
};

} // namespace ns3

#endif