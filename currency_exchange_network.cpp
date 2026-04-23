/*
==========================================================================
    GLOBAL CURRENCY EXCHANGE & BANKING NETWORK
==========================================================================
    DS Used:
      - Graph (vector adjacency list)   : Core banking network
      - BST                             : Bank rate rankings
      - Stack                           : Tarjan SCC internals

    Algorithms:
      - Dijkstra                        : Best conversion + max profit path
      - Bellman-Ford                    : Arbitrage detection
      - Edmonds-Karp (Max Flow)         : Large volume transfer
      - Tarjan SCC                      : Bank collapse simulation

    Roles:
      - User  : Convert, Arbitrage, MaxFlow, Collapse, Rankings
      - Admin : Add/Delete Bank, Add/Delete Edge
==========================================================================
*/

#include <bits/stdc++.h>
using namespace std;

// ==========================================================================
//  CONSTANTS
// ==========================================================================

const double INF = 1e18;
int numBanks = 0;
const string DATA_FILE = "bank_network.csv";

// ==========================================================================
//  STRUCTS
// ==========================================================================

struct Edge
{
    int to;
    string fromCurrency;
    string toCurrency;
    double rate;
    double fee;
    double capacity;
};

struct Bank
{
    int id;
    string name;
};

// ==========================================================================
//  GLOBALS
// ==========================================================================

vector<Bank> banks;
vector<vector<Edge>> graph;

// ==========================================================================
//  UTILITY
// ==========================================================================

void clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pauseScreen()
{
    cout << "\n  Press Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

void displayBanks()
{
    cout << "\n  ---- Available Banks ----\n";
    if (banks.empty())
    {
        cout << "  (No banks)\n\n";
        return;
    }
    for (auto &b : banks)
        cout << "    [" << b.id << "] " << b.name << "\n";
    cout << "\n";
}

string getBankName(int id)
{
    for (auto &b : banks)
        if (b.id == id)
            return b.name;
    return "Unknown";
}

bool bankExists(int id)
{
    for (auto &b : banks)
        if (b.id == id)
            return true;
    return false;
}

// ---------- Validated Input ----------

double getPositiveDouble(const string &prompt)
{
    double val;
    while (true)
    {
        cout << prompt;
        if (cin >> val && val > 0)
            return val;
        cout << "   Must be a positive number. Try again.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

int getValidBankID(const string &prompt)
{
    int id;
    while (true)
    {
        cout << prompt;
        if (cin >> id && bankExists(id))
            return id;
        cout << "   Invalid bank ID. Choose from the list.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

int getMenuChoice(int maxChoice)
{
    int choice;
    while (true)
    {
        cout << "  Choice: ";
        if (cin >> choice && choice >= 0 && choice <= maxChoice)
            return choice;
        cout << "   Enter a number between 0 and " << maxChoice << ".\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

// ==========================================================================
//  FILE PERSISTENCE
// ==========================================================================

void saveToFile()
{
    ofstream f(DATA_FILE);
    if (!f.is_open())
    {
        cout << "   Could not save to file.\n";
        return;
    }

    f << "BANKS\n";
    for (auto &b : banks)
        f << b.id << "," << b.name << "\n";

    f << "EDGES\n";
    for (auto &b : banks)
        for (auto &e : graph[b.id])
            f << b.id << "," << e.to << ","
              << e.fromCurrency << "," << e.toCurrency << ","
              << e.rate << "," << e.fee << "," << e.capacity << "\n";

    f.close();
}

bool loadFromFile()
{
    ifstream f(DATA_FILE);
    if (!f.is_open())
        return false;

    banks.clear();
    graph.clear();
    numBanks = 0;

    string line, section = "";
    int maxID = -1;

    // First pass: load banks
    while (getline(f, line))
    {
        if (line == "BANKS")
        {
            section = "BANKS";
            continue;
        }
        if (line == "EDGES")
        {
            section = "EDGES";
            continue;
        }
        if (line.empty())
            continue;
        if (section == "BANKS")
        {
            stringstream ss(line);
            string tok;
            Bank b;
            getline(ss, tok, ',');
            b.id = stoi(tok);
            getline(ss, tok, ',');
            b.name = tok;
            banks.push_back(b);
            if (b.id > maxID)
                maxID = b.id;
        }
    }

    numBanks = maxID + 1;
    graph.resize(numBanks);

    // Second pass: load edges
    f.clear();
    f.seekg(0);
    section = "";
    while (getline(f, line))
    {
        if (line == "BANKS")
        {
            section = "BANKS";
            continue;
        }
        if (line == "EDGES")
        {
            section = "EDGES";
            continue;
        }
        if (line.empty() || section != "EDGES")
            continue;
        stringstream ss(line);
        string tok;
        int from;
        Edge e;
        getline(ss, tok, ',');
        from = stoi(tok);
        getline(ss, tok, ',');
        e.to = stoi(tok);
        getline(ss, tok, ',');
        e.fromCurrency = tok;
        getline(ss, tok, ',');
        e.toCurrency = tok;
        getline(ss, tok, ',');
        e.rate = stod(tok);
        getline(ss, tok, ',');
        e.fee = stod(tok);
        getline(ss, tok, ',');
        e.capacity = stod(tok);
        if (from < numBanks)
            graph[from].push_back(e);
    }
    f.close();
    return true;
}

// ==========================================================================
//  GRAPH OPERATIONS
// ==========================================================================

void addBank(const string &name)
{
    Bank b;
    b.id = numBanks++;
    b.name = name;
    banks.push_back(b);
    graph.push_back({});
    cout << "   Bank \"" << name << "\" added with ID " << b.id << "\n";
    saveToFile();
}

void deleteBank(int id)
{
    if (!bankExists(id))
    {
        cout << "   Invalid bank ID.\n";
        return;
    }
    string name = getBankName(id);
    for (auto &el : graph)
        el.erase(remove_if(el.begin(), el.end(), [id](const Edge &e)
                           { return e.to == id; }),
                 el.end());

    graph[id].clear();

    banks.erase(remove_if(banks.begin(), banks.end(),
                          [id](const Bank &b)
                          { return b.id == id; }),
                banks.end());
    cout << "   Bank \"" << name << "\" deleted.\n";
    saveToFile();
}

void addEdge(int from, int to, const string &fc, const string &tc, double rate, double fee, double cap)
{
    if (!bankExists(from) || !bankExists(to))
    {
        cout << "   Invalid bank IDs.\n";
        return;
    }
    Edge e;
    e.to = to;
    e.fromCurrency = fc;
    e.toCurrency = tc;
    e.rate = rate;
    e.fee = fee;
    e.capacity = cap;
    graph[from].push_back(e);
    cout << "   " << getBankName(from) << " -> " << getBankName(to)
         << " [" << fc << "->" << tc << " | rate:" << rate
         << " | fee:" << fee << "% | cap:$" << cap << "M]\n";
    saveToFile();
}

void deleteEdge(int from, int to, const string &fc, const string &tc)
{
    if (!bankExists(from))
    {
        cout << "   Invalid bank ID.\n";
        return;
    }
    auto &el = graph[from];
    auto it = remove_if(el.begin(), el.end(), [&](const Edge &e)
                        { return e.to == to && e.fromCurrency == fc && e.toCurrency == tc; });

    if (it == el.end())
    {
        cout << "   Edge not found.\n";
        return;
    }

    el.erase(it, el.end());
    cout << "   Edge deleted.\n";
    saveToFile();
}

void displayGraph()
{
    cout << "\n  ---- Full Banking Network ----\n";
    for (auto &b : banks)
    {
        cout << "  [" << b.id << "] " << b.name << "\n";
        if (graph[b.id].empty())
        {
            cout << "      (no outgoing edges)\n";
            continue;
        }
        for (auto &e : graph[b.id])
            cout << "      -> [" << e.to << "] " << getBankName(e.to)
                 << "  |  " << e.fromCurrency << "->" << e.toCurrency
                 << "  |  rate:" << e.rate
                 << "  |  fee:" << e.fee << "%"
                 << "  |  cap:$" << e.capacity << "M\n";
    }
    cout << "\n";
}

// ==========================================================================
//  BST — Bank Rate Rankings
// ==========================================================================

struct BSTNode
{
    double rate;
    string bankName;
    BSTNode *left;
    BSTNode *right;
    BSTNode(double r, string n) : rate(r), bankName(n), left(nullptr), right(nullptr) {}
};

BSTNode *bstInsert(BSTNode *root, double rate, const string &name)
{
    if (!root)
        return new BSTNode(rate, name);
    if (rate < root->rate)
        root->left = bstInsert(root->left, rate, name);
    else
        root->right = bstInsert(root->right, rate, name);
    return root;
}

void bstInOrder(BSTNode *root, vector<pair<double, string>> &out)
{
    if (!root)
        return;
    bstInOrder(root->left, out);
    out.push_back({root->rate, root->bankName});
    bstInOrder(root->right, out);
}

void bstFree(BSTNode *root)
{
    if (!root)
        return;
    bstFree(root->left);
    bstFree(root->right);
    delete root;
}

void bankRateRankings()
{
    clearScreen();
    cout << "  Enter source currency (e.g. USD): ";
    string fc;
    cin >> fc;
    cout << "  Enter target currency (e.g. INR): ";
    string tc;
    cin >> tc;

    BSTNode *root = nullptr;
    for (auto &b : banks)
        for (auto &e : graph[b.id])
            if (e.fromCurrency == fc && e.toCurrency == tc)
                root = bstInsert(root, e.rate * (1.0 - e.fee / 100.0), b.name);

    if (!root)
    {
        cout << "\n   No banks offer " << fc << " -> " << tc << " directly.\n";
        pauseScreen();
        return;
    }

    vector<pair<double, string>> result;
    bstInOrder(root, result);
    reverse(result.begin(), result.end()); // best rate first

    cout << "\n  ---- Bank Rankings: " << fc << " -> " << tc << " ----\n";
    cout << "  (Effective rate after fee)\n\n";
    for (int i = 0; i < (int)result.size(); i++)
        cout << "  #" << i + 1 << "  " << setw(20) << left << result[i].second
             << "  Effective Rate: " << fixed << setprecision(4) << result[i].first << "\n";

    bstFree(root);
    pauseScreen();
}

// ==========================================================================
//  DIJKSTRA — Convert + Max Profit
// ==========================================================================

struct PathInfo
{
    double weight, amountOut;
    vector<int> bankPath;
    vector<string> pairs;
};

struct DijkState
{
    double weight, amountSoFar;
    int node;
    vector<int> path;
    vector<string> pairs;
    bool operator>(const DijkState &o) const { return weight > o.weight; }
};

vector<PathInfo> dijkstra(const string &fc, const string &tc, double amount, int topK = 3)
{
    vector<PathInfo> results;
    priority_queue<DijkState, vector<DijkState>, greater<DijkState>> pq;

    for (auto &b : banks)
    {
        for (auto &e : graph[b.id])
            if (e.fromCurrency == fc)
            {
                DijkState s;
                s.weight = 0;
                s.node = b.id;
                s.amountSoFar = amount;
                s.path = {b.id};
                pq.push(s);
                break;
            }
    }

    set<vector<int>> visited;
    int found = 0;

    while (!pq.empty() && found < topK)
    {
        DijkState cur = pq.top();
        pq.pop();
        if (visited.count(cur.path))
            continue;

        if (!cur.pairs.empty())
        {
            string lastTo = cur.pairs.back().substr(cur.pairs.back().find('>') + 1);
            if (lastTo == tc)
            {
                visited.insert(cur.path);
                PathInfo pi;
                pi.weight = cur.weight;
                pi.amountOut = cur.amountSoFar;
                pi.bankPath = cur.path;
                pi.pairs = cur.pairs;
                results.push_back(pi);
                found++;
                continue;
            }
        }

        if ((int)cur.path.size() > numBanks)
            continue;

        string curCurr = fc;
        if (!cur.pairs.empty())
        {
            string lp = cur.pairs.back();
            curCurr = lp.substr(lp.find('>') + 1);
        }

        for (auto &e : graph[cur.node])
        {
            if (e.fromCurrency != curCurr)
                continue;
            bool seen = false;
            for (int p : cur.path)
                if (p == e.to)
                {
                    seen = true;
                    break;
                }
            if (seen && e.toCurrency != tc)
                continue;

            double ew = 1.0 / (e.rate * (1.0 - e.fee / 100.0));
            double na = cur.amountSoFar * e.rate * (1.0 - e.fee / 100.0);

            DijkState ns;
            ns.weight = cur.weight + ew;
            ns.node = e.to;
            ns.amountSoFar = na;
            ns.path = cur.path;
            ns.path.push_back(e.to);
            ns.pairs = cur.pairs;
            ns.pairs.push_back(e.fromCurrency + ">" + e.toCurrency);
            pq.push(ns);
        }
    }

    sort(results.begin(), results.end(), [](const PathInfo &a, const PathInfo &b)
         { return a.amountOut > b.amountOut; });
    return results;
}

void printPathResult(const PathInfo &p, int rank, const string &fc, const string &tc, double amount)
{
    cout << "  ---- Path #" << rank << " ----\n";
    cout << "  Banks : ";
    for (int j = 0; j < (int)p.bankPath.size(); j++)
    {
        cout << getBankName(p.bankPath[j]);
        if (j + 1 < (int)p.bankPath.size())
            cout << "  =>  ";
    }
    cout << "\n  Hops  : ";
    for (int j = 0; j < (int)p.pairs.size(); j++)
    {
        string disp = p.pairs[j];
        replace(disp.begin(), disp.end(), '>', '-');
        cout << getBankName(p.bankPath[j]) << "(" << disp << ")";
        if (j + 1 < (int)p.pairs.size())
            cout << "  =>  ";
    }
    cout << "\n  Result: " << fixed << setprecision(4) << amount
         << " " << fc << "  =>  " << p.amountOut << " " << tc << "\n\n";
}

void convertCurrency()
{
    clearScreen();
    cout << "  Enter source currency: ";
    string fc;
    cin >> fc;
    cout << "  Enter target currency: ";
    string tc;
    cin >> tc;
    double amount = getPositiveDouble("  Enter amount: ");

    auto paths = dijkstra(fc, tc, amount, 3);
    if (paths.empty())
    {
        cout << "\n   No conversion path found from " << fc << " to " << tc << ".\n";
        pauseScreen();
        return;
    }
    cout << "\n  ---- Top Conversion Paths ----\n\n";
    for (int i = 0; i < (int)paths.size(); i++)
        printPathResult(paths[i], i + 1, fc, tc, amount);
    pauseScreen();
}

void maximumProfitPath()
{
    clearScreen();
    cout << "  Enter source currency: ";
    string fc;
    cin >> fc;
    cout << "  Enter target currency: ";
    string tc;
    cin >> tc;

    auto paths = dijkstra(fc, tc, 1.0, 5);
    if (paths.empty())
    {
        cout << "\n   No path found from " << fc << " to " << tc << ".\n";
        pauseScreen();
        return;
    }
    cout << "\n  ---- Maximum Profit Paths (per 1 " << fc << ") ----\n\n";
    for (int i = 0; i < (int)paths.size(); i++)
        printPathResult(paths[i], i + 1, fc, tc, 1.0);
    pauseScreen();
}

// ==========================================================================
//  BELLMAN-FORD — Arbitrage Detection
// ==========================================================================

void detectArbitrage()
{
    clearScreen();
    int n = numBanks;

    struct BFEdge
    {
        int u, v;
        double w;
        string fc, tc;
    };
    vector<BFEdge> edges;

    for (auto &b : banks)
        for (auto &e : graph[b.id])
        {
            double er = e.rate * (1.0 - e.fee / 100.0);
            if (er > 0)
                edges.push_back({b.id, e.to, -log(er), e.fromCurrency, e.toCurrency});
        }

    vector<double> dist(n, 0.0);
    vector<int> parent(n, -1);

    for (int i = 0; i < n - 1; i++)
        for (auto &e : edges)
            if (dist[e.u] + e.w < dist[e.v])
            {
                dist[e.v] = dist[e.u] + e.w;
                parent[e.v] = e.u;
            }

    vector<int> negNodes;
    for (auto &e : edges)
        if (dist[e.u] + e.w < dist[e.v])
            negNodes.push_back(e.v);

    if (negNodes.empty())
    {
        cout << "\n   No arbitrage opportunities in current network.\n";
        pauseScreen();
        return;
    }

    cout << "\n  ---- ARBITRAGE OPPORTUNITIES DETECTED ----\n\n";

    set<vector<int>> printed;
    for (int node : negNodes)
    {
        int cur = node;
        for (int i = 0; i < n; i++)
            cur = parent[cur];

        vector<int> cycle;
        int start = cur;
        cycle.push_back(start);
        int nxt = parent[start];
        while (nxt != start && nxt != -1)
        {
            cycle.push_back(nxt);
            nxt = parent[nxt];
        }
        cycle.push_back(start);
        reverse(cycle.begin(), cycle.end());

        if (printed.count(cycle))
            continue;
        printed.insert(cycle);

        double profit = 1.0;
        vector<string> hopInfo;
        for (int i = 0; i + 1 < (int)cycle.size(); i++)
        {
            int u = cycle[i], v = cycle[i + 1];
            for (auto &e : graph[u])
            {
                if (e.to == v)
                {
                    profit *= e.rate * (1.0 - e.fee / 100.0);
                    hopInfo.push_back(
                        getBankName(u) + "(" + e.fromCurrency + "->" + e.toCurrency + ")");
                    break;
                }
            }
        }

        cout << "  Cycle:\n  ";
        for (int i = 0; i < (int)hopInfo.size(); i++)
        {
            cout << hopInfo[i];
            if (i + 1 < (int)hopInfo.size())
                cout << "  =>  ";
        }
        cout << "  =>  back to " << getBankName(cycle[0]) << "\n";
        cout << "  Start: 1.000000  |  End: " << fixed << setprecision(6) << profit;
        if (profit > 1.0)
            cout << "   PROFIT: +" << fixed << setprecision(2) << (profit - 1.0) * 100 << "%";
        cout << "\n\n";
    }
    pauseScreen();
}

// ==========================================================================
//  EDMONDS-KARP — Max Flow
// ==========================================================================

double edmondsKarp(int src, int snk, vector<vector<double>> &cap, vector<vector<double>> &flow)
{
    int n = numBanks;
    double maxFlow = 0;
    while (true)
    {
        vector<int> par(n, -1);
        vector<bool> vis(n, false);
        queue<int> q;
        q.push(src);
        vis[src] = true;
        while (!q.empty() && !vis[snk])
        {
            int u = q.front();
            q.pop();
            for (int v = 0; v < n; v++)
                if (!vis[v] && cap[u][v] - flow[u][v] > 1e-9)
                {
                    vis[v] = true;
                    par[v] = u;
                    q.push(v);
                }
        }
        if (!vis[snk])
            break;
        double pf = INF;
        for (int v = snk; v != src; v = par[v])
            pf = min(pf, cap[par[v]][v] - flow[par[v]][v]);
        for (int v = snk; v != src; v = par[v])
        {
            flow[par[v]][v] += pf;
            flow[v][par[v]] -= pf;
        }
        maxFlow += pf;
    }
    return maxFlow;
}

void largeVolumeTransfer()
{
    clearScreen();
    displayBanks();
    int src = getValidBankID("  Source bank ID     : ");
    int dst = getValidBankID("  Destination bank ID: ");
    double amount = getPositiveDouble("  Amount to transfer ($M): ");

    int n = numBanks;
    vector<vector<double>> cap(n, vector<double>(n, 0)), flow(n, vector<double>(n, 0));
    for (auto &b : banks)
        for (auto &e : graph[b.id])
            cap[b.id][e.to] += e.capacity;

    double mf = edmondsKarp(src, dst, cap, flow);

    cout << "\n  ---- Large Volume Transfer ----\n\n";
    cout << "  From : " << getBankName(src) << "\n";
    cout << "  To   : " << getBankName(dst) << "\n";
    cout << "  Asked: $" << fixed << setprecision(2) << amount << "M\n";
    cout << "  Max/day: $" << mf << "M\n\n";

    if (mf < 1e-9)
        cout << "   No path exists between these banks.\n";
    else if (mf >= amount)
        cout << "   Feasible in one day.\n";
    else
        cout << "    Needs " << (int)ceil(amount / mf) << " day(s) at full capacity.\n";

    cout << "\n  Route breakdown:\n";
    bool any = false;
    for (auto &b : banks)
        for (int v = 0; v < n; v++)
            if (flow[b.id][v] > 1e-9)
            {
                cout << "    " << getBankName(b.id) << " -> " << getBankName(v)
                     << " : $" << fixed << setprecision(2) << flow[b.id][v] << "M\n";
                any = true;
            }
    if (!any)
        cout << "    (no flow)\n";
    pauseScreen();
}

// ==========================================================================
//  TARJAN SCC — Bank Collapse
// ==========================================================================

int sccTimer = 0;

void tarjanDFS(int u, vector<int> &disc, vector<int> &low,
               stack<int> &st, vector<bool> &onStack,
               vector<vector<int>> &sccs, const vector<vector<Edge>> &g)
{
    disc[u] = low[u] = sccTimer++;
    st.push(u);
    onStack[u] = true;
    for (auto &e : g[u])
    {
        int v = e.to;
        if (disc[v] == -1)
        {
            tarjanDFS(v, disc, low, st, onStack, sccs, g);
            low[u] = min(low[u], low[v]);
        }
        else if (onStack[v])
            low[u] = min(low[u], disc[v]);
    }
    if (low[u] == disc[u])
    {
        vector<int> scc;
        while (true)
        {
            int v = st.top();
            st.pop();
            onStack[v] = false;
            scc.push_back(v);
            if (v == u)
                break;
        }
        sccs.push_back(scc);
    }
}

vector<vector<int>> tarjanSCC(const vector<vector<Edge>> &g, int n)
{
    sccTimer = 0;
    vector<int> disc(n, -1), low(n, 0);
    vector<bool> onStack(n, false);
    stack<int> st;
    vector<vector<int>> sccs;
    for (int i = 0; i < n; i++)
        if (disc[i] == -1)
            tarjanDFS(i, disc, low, st, onStack, sccs, g);
    return sccs;
}

void bankCollapseSimulator()
{
    clearScreen();
    displayBanks();
    int bid = getValidBankID("  Bank ID to collapse/sanction: ");
    string name = getBankName(bid);

    vector<vector<Edge>> reduced(numBanks);
    for (auto &b : banks)
    {
        if (b.id == bid)
            continue;
        for (auto &e : graph[b.id])
            if (e.to != bid)
                reduced[b.id].push_back(e);
    }

    auto before = tarjanSCC(graph, numBanks);
    auto after = tarjanSCC(reduced, numBanks);

    cout << "\n  ---- Bank Collapse: " << name << " ----\n\n";

    cout << "  BEFORE — Connected Groups:\n";
    for (auto &scc : before)
    {
        if (scc.size() <= 1)
            continue;
        cout << "    [ ";
        for (int id : scc)
            cout << getBankName(id) << " ";
        cout << "]\n";
    }

    cout << "\n  AFTER  — Connected Groups:\n";
    bool anyIso = false;
    for (auto &scc : after)
    {
        if (scc.size() == 1 && scc[0] != bid)
        {
            cout << "    ISOLATED: " << getBankName(scc[0]) << "\n";
            anyIso = true;
        }
        else if (scc.size() > 1)
        {
            cout << "    [ ";
            for (int id : scc)
                if (id != bid)
                    cout << getBankName(id) << " ";
            cout << "]\n";
        }
    }
    if (!anyIso)
        cout << "    No banks isolated. Network stays connected.\n";

    int groups = 0;
    for (auto &scc : after)
        if (scc.size() > 1)
            groups++;
    cout << "\n  IMPACT: Removing " << name << " created " << groups << " group(s).\n";

    pauseScreen();
}

// ==========================================================================
//  DEMO DATA
// ==========================================================================

void loadDemoData()
{
    addBank("RBI");            // 0
    addBank("FederalReserve"); // 1
    addBank("ECB");            // 2
    addBank("SBI");            // 3
    addBank("HSBC");           // 4 — critical hub
    addBank("JPMorgan");       // 5
    addBank("DeutscheBank");   // 6
    addBank("WesternUnion");   // 7

    addEdge(0, 3, "INR", "USD", 0.01198, 0.3, 500);
    addEdge(0, 4, "INR", "GBP", 0.00952, 0.4, 300);
    addEdge(1, 5, "USD", "EUR", 0.9200, 0.2, 800);
    addEdge(1, 4, "USD", "GBP", 0.7850, 0.3, 600);
    addEdge(2, 6, "EUR", "CHF", 0.9650, 0.2, 400);
    addEdge(2, 4, "EUR", "USD", 1.0870, 0.2, 500);
    addEdge(3, 4, "USD", "EUR", 0.9250, 0.3, 200);
    addEdge(3, 7, "INR", "AED", 0.0440, 0.5, 100);
    addEdge(4, 5, "GBP", "USD", 1.2700, 0.2, 700);
    addEdge(4, 6, "EUR", "CHF", 0.9680, 0.2, 400);
    addEdge(4, 2, "USD", "EUR", 0.9180, 0.2, 500);
    addEdge(4, 7, "GBP", "AED", 4.6300, 0.4, 150);
    addEdge(5, 1, "EUR", "USD", 1.0850, 0.2, 800);
    addEdge(5, 6, "USD", "CHF", 0.8900, 0.3, 300);
    addEdge(5, 4, "USD", "GBP", 0.7870, 0.2, 400);
    addEdge(5, 2, "USD", "EUR", 0.9300, 0.1, 200); // intentional arbitrage edge
    addEdge(6, 2, "CHF", "EUR", 1.0350, 0.2, 350);
    addEdge(6, 4, "CHF", "USD", 1.1200, 0.3, 250);
    addEdge(7, 0, "AED", "INR", 22.700, 0.6, 80);
    addEdge(7, 3, "AED", "USD", 0.2723, 0.5, 60);

    cout << "   Demo data loaded!\n";
}

// ==========================================================================
//  ADMIN PANEL
// ==========================================================================

void adminPanel()
{
    string password;
    cout << "\n  Enter admin password: ";
    cin >> password;
    if (password != "admin123")
    {
        cout << "   Access denied.\n";
        pauseScreen();
        return;
    }

    int choice;
    do
    {
        clearScreen();
        cout << "\n==========================================\n";
        cout << "            ADMIN PANEL\n";
        cout << "==========================================\n";
        displayBanks();
        cout << "  1. Add Bank\n";
        cout << "  2. Delete Bank\n";
        cout << "  3. Add Edge\n";
        cout << "  4. Delete Edge\n";
        cout << "  5. Display Full Network\n";
        cout << "  0. Back\n";
        cout << "==========================================\n";
        choice = getMenuChoice(5);

        if (choice == 1)
        {
            clearScreen();
            cout << "  Enter bank name: ";
            string name;
            cin >> name;
            addBank(name);
            pauseScreen();
        }
        else if (choice == 2)
        {
            clearScreen();
            displayBanks();
            int id = getValidBankID("  Bank ID to delete: ");
            deleteBank(id);
            pauseScreen();
        }
        else if (choice == 3)
        {
            clearScreen();
            displayBanks();
            int from = getValidBankID("  From bank ID: ");
            int to = getValidBankID("  To bank ID  : ");
            string fc, tc;
            cout << "  From currency: ";
            cin >> fc;
            cout << "  To currency  : ";
            cin >> tc;
            double rate = getPositiveDouble("  Exchange rate: ");
            double fee = getPositiveDouble("  Fee (%): ");
            double cap = getPositiveDouble("  Capacity ($M): ");
            addEdge(from, to, fc, tc, rate, fee, cap);
            pauseScreen();
        }
        else if (choice == 4)
        {
            clearScreen();
            displayBanks();
            int from = getValidBankID("  From bank ID: ");
            int to = getValidBankID("  To bank ID  : ");
            string fc, tc;
            cout << "  From currency: ";
            cin >> fc;
            cout << "  To currency  : ";
            cin >> tc;
            deleteEdge(from, to, fc, tc);
            pauseScreen();
        }
        else if (choice == 5)
        {
            clearScreen();
            displayGraph();
            pauseScreen();
        }
    } while (choice != 0);
}

// ==========================================================================
//  USER MENU
// ==========================================================================

void userMenu()
{
    int choice;
    do
    {
        clearScreen();
        cout << "\n==========================================\n";
        cout << "   GLOBAL CURRENCY EXCHANGE NETWORK\n";
        cout << "==========================================\n";
        cout << "  1. Convert Currency        (Dijkstra)\n";
        cout << "  2. Maximum Profit Path     (Dijkstra)\n";
        cout << "  3. Detect Arbitrage        (Bellman-Ford)\n";
        cout << "  4. Large Volume Transfer   (Max Flow)\n";
        cout << "  5. Bank Collapse Simulator (Tarjan SCC)\n";
        cout << "  6. Bank Rate Rankings      (BST)\n";
        cout << "  0. Back\n";
        cout << "==========================================\n";
        choice = getMenuChoice(6);
        switch (choice)
        {
        case 1:
            convertCurrency();
            break;
        case 2:
            maximumProfitPath();
            break;
        case 3:
            detectArbitrage();
            break;
        case 4:
            largeVolumeTransfer();
            break;
        case 5:
            bankCollapseSimulator();
            break;
        case 6:
            bankRateRankings();
            break;
        }
    } while (choice != 0);
}

// ==========================================================================
//  MAIN
// ==========================================================================

int main()
{
    clearScreen();
    cout << "\n==========================================\n";
    cout << "   GLOBAL CURRENCY EXCHANGE NETWORK\n";
    cout << "==========================================\n\n";

    if (loadFromFile())
    {
        cout << "   Loaded saved network: " << banks.size() << " banks.\n";
    }
    else
    {
        cout << "  No saved data found. Loading demo data...\n\n";
        loadDemoData();
    }

    pauseScreen();

    int role;
    do
    {
        clearScreen();
        cout << "\n==========================================\n";
        cout << "   GLOBAL CURRENCY EXCHANGE NETWORK\n";
        cout << "==========================================\n";
        cout << "  1. User\n";
        cout << "  2. Admin\n";
        cout << "  0. Exit\n";
        cout << "==========================================\n";
        role = getMenuChoice(2);
        if (role == 1)
            userMenu();
        else if (role == 2)
            adminPanel();
    } while (role != 0);

    cout << "\n  Goodbye!\n\n";
    return 0;
}
