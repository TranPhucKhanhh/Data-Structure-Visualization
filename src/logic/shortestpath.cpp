#include <logic/shortestpath.h>

ShortestPath::ShortestPath() : num_vertices(0) {}

ShortestPath::~ShortestPath()
{
    clear();
}

void ShortestPath::clear()
{
    adj_list.clear();
    num_vertices = 0;
}

void ShortestPath::addEdge(int _u, int _v, int _w)
{
    int _max_node = std::max(_u, _v); //Update and resize the adj_list.size()
    if (_max_node >= num_vertices)
    {
        num_vertices = _max_node + 1;
        adj_list.resize(num_vertices);
    }
    adj_list[_u].push_back({_v, _w});
    adj_list[_v].push_back({_u, _w});
}

void ShortestPath::initFromKeyboard()
{
    clear();
    int _edges_count;
    std::cout << "Enter number of edges: ";
    std::cin >> _edges_count;

    for (int _i = 0; _i < _edges_count; ++_i)
    {
        int _u, _v, _w;
        std::cout << "Enter the " << _i + 1 << " edge: ";
        std::cin >> _u >> _v >> _w;
        addEdge(_u, _v, _w);
    }
}

void ShortestPath::initFromFile(const std::string &_file_path)
{
    std::ifstream _inp(_file_path);

    if (!_inp.is_open())
    {
        std::cerr << "Error: Cannot open " << _file_path << " for initialization!\n";
        return;
    }

    int _u, _v, _w;
    while (_inp >> _u >> _v >> _w)
    {
        addEdge(_u, _v, _w);
    }

    _inp.close();
}

std::vector<ShortestPathInstruction> ShortestPath::dijkstraStep(int _start, int _end)
{
    std::vector<ShortestPathInstruction> _steps;

    //If the number of the starting node or the end node is greater than the current number of vertices,
    //it means that there exist one node outside the graph -> cannot go from/to it
    if (_start >= num_vertices || _end >= num_vertices)
    {
        _steps.push_back(ShortestPathInstruction(ShortestPathOp::NOT_FOUND));
        return _steps;
    }

    std::vector<int> _dist(num_vertices, INF);
    std::vector<int> _parent(num_vertices, -1); //Used for tracing

    _dist[_start] = 0;

    //Define pair for priority_queue
    using NodePair = std::pair<int, int>;
    std::priority_queue<NodePair, std::vector<NodePair>, std::greater<NodePair>> _pq;

    _pq.push({0, _start});
    _steps.push_back(ShortestPathInstruction(ShortestPathOp::UPDATE_DISTANCE, _start, -1, 0));

    while (!_pq.empty())
    {
        int _u = _pq.top().second, _c = _pq.top().first;

        _pq.pop();
        if (_c > _dist[_u]) continue;

        _steps.push_back(ShortestPathInstruction(ShortestPathOp::HIGHLIGHT_NODE, _u));
        _steps.push_back(ShortestPathInstruction(ShortestPathOp::MARK_PERMANENT, _u));

        if (_u == _end) break;

        for (Edge &_edge: adj_list[_u])
        {
            int _v = _edge.target_node;
            int _w = _edge.weight;

            //Show that the edge is being inspected
            _steps.push_back(ShortestPathInstruction(ShortestPathOp::RELAX_EDGE, _u, _v, _w));

            if (_dist[_v] > _dist[_u] + _w)
            {
                _dist[_v] = _dist[_u] + _w;
                _parent[_v] = _u;
                _pq.push({_dist[_v], _v});

                _steps.push_back(ShortestPathInstruction(ShortestPathOp::UPDATE_DISTANCE, _v, -1, _dist[_v]));
            }
        }
    }

    //Trace the final path if reachable
    if (_dist[_end] == INF)
    {
        _steps.push_back(ShortestPathInstruction(ShortestPathOp::NOT_FOUND));
    }
    else
    {
        int _cur = _end;
        while (_cur != -1)
        {
            _steps.push_back(ShortestPathInstruction(ShortestPathOp::FOUND_PATH, _cur, _parent[_cur]));
            _cur = _parent[_cur];
        }
    }

    return _steps;
}

