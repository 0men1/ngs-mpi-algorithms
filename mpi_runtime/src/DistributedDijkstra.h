#pragma once

#ifndef __DISTRIBUTED_DIJKSTRA_H__
#define __DISTRIBUTED_DIJKSTRA_H__

#include <chrono>
#include "DistributedAlgorithm.h"

struct UpdateMsg {
  int nodeId;
  float dist;
};

class DistributedDijkstra : public DistributedAlgorithm {
public:
  DistributedDijkstra(int source) : m_sourceNode(source) {}
  void execute(GraphData &graph) override;
  void reportMetrics() const override;

  std::vector<float> getDistances() const { return m_distances; }

  int getNumMessagesSent() const { return m_numMessages; }

  int getNumBytesSent() const { return m_bytesSent; }

private:
  int m_sourceNode;
  int m_numIterations;
  int m_numMessages;
  int m_bytesSent;
  std::vector<float> m_distances;
  std::chrono::duration<double> m_totalRuntime;
};

#endif //__DISTRIBUTED_DIJKSTRA_H__
