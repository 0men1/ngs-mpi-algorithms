#pragma once

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
};

#endif //__DISTRIBUTED_DIJKSTRA_H__
