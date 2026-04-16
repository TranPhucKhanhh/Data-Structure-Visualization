#pragma once

#include<iostream>
#include<fstream>
#include<utility>
#include<functional>
#include<vector>
#include<string>
#include<queue>
#include<sstream>
#include<array>
#include<set>
#include<algorithm>
#include<random>

enum class ShortestPathOp
{
    HIGHLIGHT_NODE, //Focus on the current node being processed
    RELAX_EDGE,     //Inspect/Update an edge weight
    UPDATE_DISTANCE,//Update the distance value in the tracking table
    MARK_PERMANENT, //Mark a node as having its shortest path finalized
    FOUND_PATH,     //Identify and mark nodes belonging to the successful route
    NOT_FOUND       //Mark that no path exists between the selected nodes
};

struct ShortestPathInstruction
{
    ShortestPathOp op_type;
    int node_u;
    int node_v;
    int weight; //Assume that every weight is integer

    ShortestPathInstruction(ShortestPathOp op, int u = -1, int v = -1, int w = 0):
        op_type(op), node_u(u), node_v(v), weight(w) {}
};

//Structure to hold the visual state of the graph at any step
struct SPVisualState {
    std::vector<int> distances;
	std::vector<int> parent;
	std::vector<bool> settled;
	std::vector<bool> inPath;
	int activeNode = -1;
	int relaxU = -1;
	int relaxV = -1;
	bool noPath = false;
};

struct Edge
{
    int target_node;
    int weight;
};

struct ShortestPath {
private:
    const int INF = 1e9;
//    int num_vertices;

public:
    //Set public for UI to access directly
    int num_vertices;
    std::vector<std::vector<Edge>> adj_list;

    ShortestPath();
    ~ShortestPath();
    void clear();
    void addEdge(int u, int v, int w);

    ///Initialization

    //Use for Debugging
    void initFromKeyboard();

    //std::string means to return a status message
    std::string initFromFile(const std::string &file_path);

    //Parses a raw string to initialize
    std::string initFromString(const std::string &input_content);

    std::vector<std::array<int, 3>> generateRandomGraph(int& vertexCount);

    ///Dijkstra running process
    //Step-by-step Instruction
    std::vector<ShortestPathInstruction> dijkstraStep(int start, int finish);


};

inline ShortestPath shortestPath;
