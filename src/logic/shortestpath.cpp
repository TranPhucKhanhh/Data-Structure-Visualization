#include <logic/shortestpath.h>

std::string ShortestPath::normalizeInput(std::string raw) {
    for (char& c : raw) {
        switch (c) {
        case ',':
		case ';':
		case '|':
		case '(':
		case ')':
		case '[':
		case ']':
		case '{':
		case '}':
            c = ' ';
			break;
        default:
            break;
        }
    }
    return raw;
}

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

int ShortestPath::getNumVertices()
{
    return num_vertices;
}

void ShortestPath::addEdge(int u, int v, int w)
{
    int max_node = std::max(u, v); //Update and resize the adj_list.size()
    if (max_node >= num_vertices)
    {
        num_vertices = max_node + 1;
        adj_list.resize(num_vertices);
    }
    adj_list[u].push_back({v, w});
    adj_list[v].push_back({u, w});
}

//Function transfered from UI to logic
int ShortestPath::lookupEdgeWeight(const std::vector<std::array<int, 3>>& edges, int a, int b)
{
    int bestWeight = std::numeric_limits<int>::max();
    bool found = false;
	for (const auto& edge : edges) {
        const int u = edge[0];
		const int v = edge[1];
		const int w = edge[2];
		if ((u == a && v == b) || (u == b && v == a)) {
            bestWeight = std::min(bestWeight, w);
			found = true;
		}
    }
	return found ? bestWeight : -1;
}


void ShortestPath::initFromKeyboard()
{
    clear();
    int edges_count;
    std::cout << "Enter number of edges: ";
    std::cin >> edges_count;

    for (int i = 0; i < edges_count; ++i)
    {
        int u, v, w;
        std::cout << "Enter the " << i + 1 << " edge: ";
        std::cin >> u >> v >> w;
        addEdge(u, v, w);
    }
}

std::string ShortestPath::initFromString(const std::string &input_content)
{
    if (input_content.empty())
    {
        return "Error: Input file is empty!";
    }

    std::stringstream ss(input_content);
    int n, m;

    if (!(ss >> n >> m))
    {
        return "Error: Invalid header format (expected 'n m').";
    }

    if (n <= 0)
    {
        return "Error: Number of vertices must be positive.";
    }

    clear();
    num_vertices = n;
    adj_list.resize(n);

    for (int i = 0; i < m; ++i)
    {
        int u, v, w;
        if (!(ss >> u >> v >> w))
        {
            return "Error: Edge #" + std::to_string(i + 1) + " is missing or has invalid format. "
                    "Expected " + std::to_string(m) + "edges total.";
        }

        if (u >= 0 && u < n && v >= 0 && v < n)
        {
            addEdge(u, v, w);
        }
        else
        {
            return "Error: Node index out of bounds at edge " + std::to_string(i + 1);
        }
    }

    return "Success: Graph initialized successfully!";
}

std::string ShortestPath::initFromFile(const std::string &file_path)
{
    std::ifstream inp(file_path);
    if (!inp.is_open())
    {
        return "Error: Could not open file at " + file_path;
    }

    std::stringstream buffer;
    buffer << inp.rdbuf();
    inp.close();

    return initFromString(buffer.str());
}

std::vector<std::array<int, 3>> ShortestPath::generateRandomGraph(int& vertexCount)
{
    std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> vertexDist(5, 10);
	std::uniform_int_distribution<int> weightDist(1, 20);

	vertexCount = vertexDist(rng);
	const int maxEdges = vertexCount * (vertexCount - 1) / 2;
	const int minEdges = vertexCount - 1;
	const int desiredUpperBound = std::min(maxEdges, vertexCount + vertexCount / 2 + 2);
	std::uniform_int_distribution<int> edgeDist(minEdges, std::max(minEdges, desiredUpperBound));
	const int targetEdges = edgeDist(rng);

	std::vector<std::array<int, 3>> edges;
	edges.reserve(static_cast<std::size_t>(targetEdges));
	std::set<std::pair<int, int>> used;

    std::vector<int> order(static_cast<std::size_t>(vertexCount));
	for (int i = 0; i < vertexCount; ++i) {
        order[static_cast<std::size_t>(i)] = i;
    }
    std::shuffle(order.begin(), order.end(), rng);

	for (int i = 1; i < vertexCount; ++i) {
        const int u = order[static_cast<std::size_t>(i)];
		std::uniform_int_distribution<int> parentDist(0, i - 1);
        const int v = order[static_cast<std::size_t>(parentDist(rng))];
		const int a = std::min(u, v);
		const int b = std::max(u, v);
		used.insert({ a, b });
		edges.push_back({ u, v, weightDist(rng) });
    }

	std::uniform_int_distribution<int> nodeDist(0, vertexCount - 1);
	while (static_cast<int>(edges.size()) < targetEdges) {
        const int u = nodeDist(rng);
		const int v = nodeDist(rng);
		if (u == v) {
            continue;
        }
		const int a = std::min(u, v);
		const int b = std::max(u, v);
        if (used.find({ a, b }) != used.end()) {
            continue;
        }
		used.insert({ a, b });
		edges.push_back({ u, v, weightDist(rng) });
    }

	return edges;
}

std::vector<ShortestPathInstruction> ShortestPath::getDijkstraStep(int start, int finish)
{
    std::vector<ShortestPathInstruction> steps;

    //If the number of the starting node or the end node is greater than the current number of vertices,
    //it means that there exist one node outside the graph -> cannot go from/to it
    if (start >= num_vertices || finish >= num_vertices)
    {
        steps.push_back(ShortestPathInstruction(ShortestPathOp::NOT_FOUND));
        return steps;
    }

    std::vector<int> dist(num_vertices, INF);
    std::vector<int> parent(num_vertices, -1); //Used for tracing

    dist[start] = 0;

    //Define pair for priority_queue
    using NodePair = std::pair<int, int>;
    std::priority_queue<NodePair, std::vector<NodePair>, std::greater<NodePair>> pq;

    pq.push({0, start});
    steps.push_back(ShortestPathInstruction(ShortestPathOp::UPDATE_DISTANCE, start, -1, 0));

    while (!pq.empty())
    {
        int u = pq.top().second, c = pq.top().first;

        pq.pop();
        if (c > dist[u]) continue;

        steps.push_back(ShortestPathInstruction(ShortestPathOp::HIGHLIGHT_NODE, u));
        steps.push_back(ShortestPathInstruction(ShortestPathOp::MARK_PERMANENT, u));

        if (u == finish) break;

        for (Edge &edge: adj_list[u])
        {
            int v = edge.target_node;
            int w = edge.weight;

            //Show that the edge is being inspected
            steps.push_back(ShortestPathInstruction(ShortestPathOp::RELAX_EDGE, u, v, w));

            if (dist[v] > dist[u] + w)
            {
                dist[v] = dist[u] + w;
                parent[v] = u;
                pq.push({dist[v], v});

                steps.push_back(ShortestPathInstruction(ShortestPathOp::UPDATE_DISTANCE, v, -1, dist[v]));
            }
        }
    }

    //Trace the final path if reachable
    if (dist[finish] == INF)
    {
        steps.push_back(ShortestPathInstruction(ShortestPathOp::NOT_FOUND));
    }
    else
    {
        int cur = finish;
        while (cur != -1)
        {
            steps.push_back(ShortestPathInstruction(ShortestPathOp::FOUND_PATH, cur, parent[cur]));
            cur = parent[cur];
        }
    }

    return steps;
}

SPVisualState ShortestPath::buildVisualState(const std::vector<ShortestPathInstruction>& steps, int appliedCount, int vertexCount)
{
    SPVisualState state;
    state.distances.assign(static_cast<std::size_t>(std::max(0, vertexCount)), std::numeric_limits<int>::max());
	state.parent.assign(static_cast<std::size_t>(std::max(0, vertexCount)), -1);
	state.settled.assign(static_cast<std::size_t>(std::max(0, vertexCount)), false);
	state.inPath.assign(static_cast<std::size_t>(std::max(0, vertexCount)), false);

	const int count = std::clamp(appliedCount, 0, static_cast<int>(steps.size()));
	for (int i = 0; i < count; ++i) {
        const ShortestPathInstruction& step = steps[static_cast<std::size_t>(i)];
		switch (step.op_type) {
		case ShortestPathOp::HIGHLIGHT_NODE:
            state.activeNode = step.node_u;
			state.relaxU = -1;
			state.relaxV = -1;
			break;
        case ShortestPathOp::RELAX_EDGE:
            state.relaxU = step.node_u;
			state.relaxV = step.node_v;
			state.activeNode = step.node_u;
			break;
        case ShortestPathOp::UPDATE_DISTANCE:
            if (step.node_u >= 0 && step.node_u < static_cast<int>(state.distances.size())) {
                state.distances[static_cast<std::size_t>(step.node_u)] = step.weight;
				if (state.relaxU >= 0 && state.relaxU < static_cast<int>(state.parent.size())) {
                    state.parent[static_cast<std::size_t>(step.node_u)] = state.relaxU;
                }
                state.activeNode = step.node_u;
            }
			break;
        case ShortestPathOp::MARK_PERMANENT:
            if (step.node_u >= 0 && step.node_u < static_cast<int>(state.settled.size())) {
                state.settled[static_cast<std::size_t>(step.node_u)] = true;
                state.activeNode = step.node_u;
            }
			break;
        case ShortestPathOp::FOUND_PATH:
            if (step.node_u >= 0 && step.node_u < static_cast<int>(state.inPath.size())) {
                state.inPath[static_cast<std::size_t>(step.node_u)] = true;
				if (step.node_v >= 0) {
                    state.parent[static_cast<std::size_t>(step.node_u)] = step.node_v;
					state.relaxU = step.node_v;
					state.relaxV = step.node_u;
                }
				state.activeNode = step.node_u;
            }
			break;
        case ShortestPathOp::NOT_FOUND:
            state.noPath = true;
			break;
        default:
            break;
        }
    }

	return state;
}

std::vector<int> ShortestPath::extractPathFromSteps(const std::vector<ShortestPathInstruction>& steps) {
    std::vector<int> path;
	for (const auto& step : steps) {
        if (step.op_type == ShortestPathOp::FOUND_PATH && step.node_u >= 0) {
            path.push_back(step.node_u);
        }
    }
	std::reverse(path.begin(), path.end());
	return path;
}
