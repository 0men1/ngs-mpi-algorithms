#ifndef __GRAPHDATA_H__
#define __GRAPHDATA_H__

#include <unordered_map>
#include <iostream>
#include <vector>

struct Edge {
	int dest;
	int weight;
};

class GraphData {
public:
	GraphData(int rankId, std::string& graphFile, std::string partFile): m_rankId(rankId) {
		std::cout << "Loading files" << std::endl;
		loadGraphData(graphFile);
		loadPartitionData(partFile);
		std::cout << "Successfully loaded graph data for rank #" << rankId <<  std::endl;
	}

	const int m_rankId;
	std::vector<int> m_localNodes;
	std::vector<int> m_nodeOwnership;
	std::unordered_map<int, std::vector<Edge>> m_adjList;

private:
	void loadGraphData(std::string &graphFile);
	void loadPartitionData(std::string &partFile);
};

#endif //__GRAPHDATA_H__
