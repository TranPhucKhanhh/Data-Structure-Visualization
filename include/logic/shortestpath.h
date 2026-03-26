#pragma once

#include<iostream>
#include<fstream>
#include<utility>
#include<functional>
#include<vector>
#include<string>
#include<queue>

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

    ShortestPathInstruction(ShortestPathOp _op, int _u = -1, int _v = -1, int _w = 0):
        op_type(_op), node_u(_u), node_v(_v), weight(_w) {}
};

struct Edge
{
    int target_node;
    int weight;
};

struct ShortestPath {
private:
    const int INF = 1e9;
    int num_vertices;
    std::vector<std::vector<Edge>> adj_list;

public:
    ShortestPath();
    ~ShortestPath();
    void clear();
    void addEdge(int _u, int _v, int _w);

    //Initialization
    void initFromKeyboard();
    void initFromFile(const std::string &_file_path);

    //Step-by-step Instruction
    std::vector<ShortestPathInstruction> dijkstraStep(int _start, int _end);
};

inline ShortestPath shortestPath;
