#ifndef __GRAPHDATA_H__
#define __GRAPHDATA_H__

#include <map>
#include <set>

class GraphData {
public:
	void loadFiles(std::string graphFile, std::string partFile);
private:
	void loadGraphData(std::string graphFile);
	void loadPartitionData(std::string partFile);
	std::map<std::string, std::set<std::string>> m_adjList;
};

#endif //__GRAPHDATA_H__
