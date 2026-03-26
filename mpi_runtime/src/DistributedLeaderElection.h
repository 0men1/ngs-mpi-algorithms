#pragma once

#include "DistributedAlgorithm.h"
#ifndef __DISTRIBUTED_LEADER_ELECTION_H__
#define __DISTRIBUTED_LEADER_ELECTION_H__

class DistributedLeaderElection : DistributedAlgorithm {
public:
	DistributedLeaderElection(int rounds): m_rounds(rounds) {}

private:

	int m_rounds;
};

#endif //__DISTRIBUTED_DIJKSTRA_H__
