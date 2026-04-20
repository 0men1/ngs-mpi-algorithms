#pragma once

#ifndef __GRAPHDATA_H__
#define __GRAPHDATA_H__

#include <map>
#include <set>
#include <vector>

struct Edge {
	int dest;
	float weight;
};

class GraphData {
public:
	GraphData(int rankId, const std::string& graphFile, const std::string partFile): m_rankId(rankId) {
		loadData(graphFile, partFile);
	}

	const int m_rankId;
	std::vector<int> m_nodeOwnership;
	std::map<int, std::vector<Edge>> m_adjList;
	std::map<int, std::vector<Edge>> m_incomingEdges;
	std::set<int> m_ownedNodes;

private:
	void loadData(const std::string &graphFile, const std::string &partFile);
};

#endif //__GRAPHDATA_H__
