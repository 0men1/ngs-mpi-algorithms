#pragma once

#ifndef __DISTRIBUTED_ALGORITHM_H__
#define __DISTRIBUTED_ALGORITHM_H__

#include "GraphData.h"

class DistributedAlgorithm {
public:
	virtual ~DistributedAlgorithm() = default;
	virtual void execute(GraphData &graph) = 0;
	virtual void reportMetrics() const = 0;
};

#endif
