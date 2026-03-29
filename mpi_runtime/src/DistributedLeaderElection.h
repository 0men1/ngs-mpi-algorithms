#pragma once

#include <unordered_map>
#ifndef __DISTRIBUTED_LEADER_ELECTION_H__
#define __DISTRIBUTED_LEADER_ELECTION_H__

#include "DistributedAlgorithm.h"

struct ElectMsg {
	int destNode;
	int maxNodeId;
};

class DistributedLeaderElection : public DistributedAlgorithm {
public:
	DistributedLeaderElection(int rounds): m_rounds(rounds) {}
	void execute(GraphData &graph) override;
	void reportMetrics() const override;

private:
	int m_rounds;
	int m_numIterations;
	int m_numMessages;
	int m_bytesSent;
	std::unordered_map<int, int> m_finalLeaders;
	std::chrono::duration<double> m_totalRuntime;
};

#endif //__DISTRIBUTED_DIJKSTRA_H__
