#include "sumoMap-graph.h"
#include <iostream>

#include <vector>
#include <stdlib.h>
#include <map>
#include <time.h>

namespace ns3 {

GraphSumoMap::GraphSumoMap ()
{
  size = 0;
}

bool
GraphSumoMap::init (std::vector<VertexSumoMap *> vertices, std::vector<EdgeSumoMap *> edges)
{
  m_vertices = vertices;
  size = m_vertices.size ();
  adjMatrix.resize (size);

  //init adjacency matrix
  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++)
      {
        EdgeSumoMap *e = new EdgeSumoMap ();
        adjMatrix[i].push_back (e);
      }
  //setting adjacency matrix
  for (auto &edge : edges)
    {
      int src = findVertexIdByName (edge->m_from);
      int dest = findVertexIdByName (edge->m_to);
      double weight = std::numeric_limits<double>::max ();
      // 
      adjMatrix[src][dest]->weight = weight;
      adjMatrix[src][dest]->nameId = edge->nameId;
      adjMatrix[src][dest]->m_from = edge->m_from;
      adjMatrix[src][dest]->m_to = edge->m_to;
    }
  //toStringAdjMatrix (); //print to debug
  return adjMatrix.size () > 0;
}

void
GraphSumoMap::toStringAdjMatrix ()
{
  for (int i = 0; i < size; i++)
    {
      for (int j = 0; j < size; j++)
        std::cout << adjMatrix[i][j]->weight << " ";
      std::cout << std::endl;
    }
  std::cout << std::endl;
}

std::string
GraphSumoMap::findVertexNameById (int idx)
{
  return m_vertices[idx]->m_nameId;
}

int
GraphSumoMap::findVertexIdByName (std::string name)
{
  int idx = -1;
  for (int i = 0; i < size; i++)
    if (name.compare (m_vertices[i]->m_nameId) == 0)
      return i;
  return idx;
}

/*
 * A utility function to find the vertex with minimum distance value,
 * from the set of vertices not yet included in shortest path tree
 */
int
GraphSumoMap::minDistance (std::vector<int> &dist, std::vector<bool> &sptSet)
{
  double min = std::numeric_limits<double>::max ();
  int min_index;
  for (int i = 0; i < size; i++)
    if (sptSet[i] == false && dist[i] < min)
      min = dist[i], min_index = i;
  return min_index;
}

/*
 * Function to print shortest path from a node
 */
void
GraphSumoMap::printPath (std::vector<int> &parent, int j)
{
  //base case : if 'j' is source
  if (parent[j] == -1)
    return;
  printPath (parent, parent[j]); //recursive call
  std::cout << j << " ";
}

/*
 * A utility function to print the constructed distance array
 */
void
GraphSumoMap::printSolution (std::vector<int> &dist, std::vector<int> &parent, int src)
{
  std::cout << "Vertex\t Distance\t Path";
  for (int i = 0; i < size; i++)
    if (i != src)
      {
        std::cout << std::endl << src << " -> " << i << "\t\t" << dist[i] << "\t" << src;
        printPath (parent, i);
      }
}

/*
 * Dijkstra shortest path algorithm 
 */
void
GraphSumoMap::dijkstra (int src, int dst, double &pathWeight, std::vector<std::string> &roadPath)
{
  // The output array. dist[i] will hold the shortest distance from src to dst
  std::vector<int> dist (size);
  // Initialize all distances = INFINITE
  fill (dist.begin (), dist.end (), std::numeric_limits<int>::max ());
  dist[src] = 0; // Distance of source vertex from itself is always 0

  // sptSet[i] will true if vertex is included
  std::vector<bool> sptSet (size);
  fill (sptSet.begin (), sptSet.end (), false);

  // Parent array to store shortest path tree
  std::vector<int> parent (size);
  fill (parent.begin (), parent.end (), -1);

  // Find shortest path for all vertices
  for (int count = 0; count < size - 1; count++)
    {
      // Pick the minimum distance vertex from the set of
      // vertices not yet processed. u is always equal to
      // src in first iteration
      int u = minDistance (dist, sptSet);

      // Mark the picked vertex as processed
      sptSet[u] = true;
      // Update dist value of the adjacent vertices of the picked vertex
      for (int v = 0; v < size; v++)
        // Update dist[v] only if is not in sptSet,
        // there is an edge from u to v, and total
        // weight of path from src to v through u is
        // smaller than current value of dist[v]
        if (!sptSet[v] && adjMatrix[u][v]->weight && dist[u] + adjMatrix[u][v]->weight < dist[v])
          {
            parent[v] = u;
            dist[v] = dist[u] + adjMatrix[u][v]->weight;
          }
    }
  //printSolution (dist, parent, src);

  int idx = dst;
  std::vector<int> path;
  while (parent[idx] != -1)
    {
      path.insert (path.begin (), idx);
      idx = parent[idx];
    }
  //
  path.insert (path.begin (), src);
  pathWeight = 0;
  for (int j = 1; j < (int) path.size (); j++)
    {
      int idx_s = path[j - 1];
      int idx_d = path[j];
      roadPath.push_back (adjMatrix[idx_s][idx_d]->nameId);
      pathWeight += adjMatrix[idx_s][idx_d]->weight;
    }
}

int
GraphSumoMap::getDstVertexIdByEdgeName (std::string edgeName)
{
  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++)
      if (edgeName.compare (adjMatrix[i][j]->nameId) == 0)
        return findVertexIdByName (adjMatrix[i][j]->m_to);
  return -1;
}

void
GraphSumoMap::getAllEdgeWeights (std::map<std::string, double> &edgeWeights)
{
  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++)
      {
        if (!adjMatrix[i][j]->nameId.empty ())
          edgeWeights.insert (
              std::pair<std::string, double> (adjMatrix[i][j]->nameId, adjMatrix[i][j]->weight));
      }
}

void
GraphSumoMap::setAllEdgeWeights (std::map<std::string, double> &edgeWeights)
{
  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++)
      {
        if (!adjMatrix[i][j]->nameId.empty ())
          adjMatrix[i][j]->weight = edgeWeights[adjMatrix[i][j]->nameId];
      }
}

} // namespace ns3