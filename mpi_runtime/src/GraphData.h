#pragma once

#ifndef __GRAPHDATA_H__
#define __GRAPHDATA_H__

#include <unordered_map>
#include <unordered_set>
#include <vector>

struct Edge {
	int dest;
	float weight;
};

class GraphData {
public:
	GraphData(int rankId, std::string& graphFile, std::string partFile): m_rankId(rankId) {
		loadData(graphFile, partFile);
	}

	const int m_rankId;
	std::vector<int> m_nodeOwnership;
	std::unordered_map<int, std::vector<Edge>> m_adjList;
	std::unordered_map<int, std::vector<Edge>> m_incomingEdges;
	std::unordered_set<int> m_ownedNodes;

private:
	void loadData(std::string &graphFile, std::string &partFile);
};

#endif //__GRAPHDATA_H__
