#pragma once

#include <chrono>
#ifndef __DISTRIBUTED_DIJKSTRA_H__
#define __DISTRIBUTED_DIJKSTRA_H__

#include "DistributedAlgorithm.h"


struct UpdateMsg {
	int nodeId;
	float dist;
};

class DistributedDijkstra : public DistributedAlgorithm {
public:
	DistributedDijkstra(int source): m_sourceNode(source) {}
	void execute(GraphData &graph) override;
	void reportMetrics() const override;

private:
	int m_sourceNode;
	int m_numIterations;
	int m_numMessages;
	int m_bytesSent;
	std::vector<float> m_distances;
	std::chrono::duration<double> m_totalRuntime;
};

#endif //__DISTRIBUTED_DIJKSTRA_H__
