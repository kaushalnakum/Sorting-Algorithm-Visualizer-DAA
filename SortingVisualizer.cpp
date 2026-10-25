#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <limits>
using namespace std;
using namespace std::chrono;

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static void enableAnsi() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD m = 0;
    GetConsoleMode(h, &m);
    SetConsoleMode(h, m | 0x0004);  // ENABLE_VIRTUAL_TERMINAL_PROCESSING
}
static void sleepMs(int ms) { Sleep(ms); }
#else
#include <unistd.h>
static void enableAnsi() {}
static void sleepMs(int ms) { usleep(ms * 1000); }
#endif

/* ------------------------------------------------------------------ */
/*  Counters and visualizer state                                      */
/* ------------------------------------------------------------------ */
struct Stats {
    long long cmp = 0;  // number of comparisons
    long long swp = 0;  // number of swaps / writes
};

static bool VIZ = false;      // true only in visualizer mode
static int DELAY_MS = 60;     // delay between steps
static string CUR_NAME;       // algorithm name shown on screen

static const char *YEL = "\033[33m", *RED = "\033[31m", *GRN = "\033[32m", *OFF = "\033[0m";

// type: 'c' = compare (yellow), 's' = swap/write (red), 'd' = done (green)
static void draw(const vector<int> &a, int i, int j, char type, const Stats &s) {
    const int H = 15;
    int n = a.size();
    cout << "\033[H\033[J";
    cout << CUR_NAME << "   Comparisons: " << s.cmp << "   Swaps/Writes: " << s.swp << "\n\n";
    for (int row = H; row >= 1; --row) {
        for (int k = 0; k < n; k++) {
            int h = max(1, a[k] * H / 100);
            if (h >= row) {
                const char *col = "";
                if (type == 'd') col = GRN;
                else if (k == i || k == j) col = (type == 'c') ? YEL : RED;
                cout << col << "##" << (*col ? OFF : "") << " ";
            } else {
                cout << "   ";
            }
        }
        cout << "\n";
    }
    for (int k = 0; k < n; k++) cout << setw(2) << a[k] << " ";
    cout << "\n" << flush;
}

static inline void step(const vector<int> &a, int i, int j, char t, const Stats &s) {
    if (!VIZ) return;
    draw(a, i, j, t, s);
    sleepMs(DELAY_MS);
}

/* ------------------------------------------------------------------ */
/*  Sorting algorithms                                                 */
/* ------------------------------------------------------------------ */
void bubbleSort(vector<int> &a, Stats &s) {
    int n = a.size();
    for (int i = 0; i < n - 1; i++) {
        bool swapped = false;
        for (int j = 0; j < n - i - 1; j++) {
            s.cmp++; step(a, j, j + 1, 'c', s);
            if (a[j] > a[j + 1]) {
                swap(a[j], a[j + 1]); s.swp++; swapped = true;
                step(a, j, j + 1, 's', s);
            }
        }
        if (!swapped) break;  // best case O(n)
    }
}

void selectionSort(vector<int> &a, Stats &s) {
    int n = a.size();
    for (int i = 0; i < n - 1; i++) {
        int m = i;
        for (int j = i + 1; j < n; j++) {
            s.cmp++; step(a, m, j, 'c', s);
            if (a[j] < a[m]) m = j;
        }
        if (m != i) {
            swap(a[i], a[m]); s.swp++;
            step(a, i, m, 's', s);
        }
    }
}

void insertionSort(vector<int> &a, Stats &s) {
    int n = a.size();
    for (int i = 1; i < n; i++) {
        int key = a[i], j = i - 1;
        while (j >= 0) {
            s.cmp++; step(a, j, j + 1, 'c', s);
            if (a[j] > key) {
                a[j + 1] = a[j]; s.swp++;
                step(a, j, j + 1, 's', s);
                j--;
            } else break;
        }
        a[j + 1] = key; s.swp++;
        step(a, j + 1, -1, 's', s);
    }
}

static void mergeParts(vector<int> &a, int l, int m, int r, Stats &s) {
    vector<int> L(a.begin() + l, a.begin() + m + 1);
    vector<int> R(a.begin() + m + 1, a.begin() + r + 1);
    size_t i = 0, j = 0;
    int k = l;
    while (i < L.size() && j < R.size()) {
        s.cmp++;
        a[k] = (L[i] <= R[j]) ? L[i++] : R[j++];
        s.swp++; step(a, k, -1, 's', s); k++;
    }
    while (i < L.size()) { a[k] = L[i++]; s.swp++; step(a, k, -1, 's', s); k++; }
    while (j < R.size()) { a[k] = R[j++]; s.swp++; step(a, k, -1, 's', s); k++; }
}
static void mergeRec(vector<int> &a, int l, int r, Stats &s) {
    if (l >= r) return;
    int m = l + (r - l) / 2;
    mergeRec(a, l, m, s);
    mergeRec(a, m + 1, r, s);
    mergeParts(a, l, m, r, s);
}
void mergeSort(vector<int> &a, Stats &s) {
    if (!a.empty()) mergeRec(a, 0, (int)a.size() - 1, s);
}

static void quickRec(vector<int> &a, int l, int h, Stats &s) {
    if (l >= h) return;
    int pivot = a[h], i = l - 1;  // Lomuto partition, last element as pivot
    for (int j = l; j < h; j++) {
        s.cmp++; step(a, j, h, 'c', s);
        if (a[j] < pivot) {
            i++;
            if (i != j) { swap(a[i], a[j]); s.swp++; step(a, i, j, 's', s); }
        }
    }
    if (i + 1 != h) { swap(a[i + 1], a[h]); s.swp++; step(a, i + 1, h, 's', s); }
    quickRec(a, l, i, s);
    quickRec(a, i + 2, h, s);
}
void quickSort(vector<int> &a, Stats &s) {
    if (!a.empty()) quickRec(a, 0, (int)a.size() - 1, s);
}

static void heapify(vector<int> &a, int n, int i, Stats &s) {
    while (true) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < n) { s.cmp++; step(a, l, m, 'c', s); if (a[l] > a[m]) m = l; }
        if (r < n) { s.cmp++; step(a, r, m, 'c', s); if (a[r] > a[m]) m = r; }
        if (m == i) return;
        swap(a[i], a[m]); s.swp++; step(a, i, m, 's', s);
        i = m;
    }
}
void heapSort(vector<int> &a, Stats &s) {
    int n = a.size();
    for (int i = n / 2 - 1; i >= 0; i--) heapify(a, n, i, s);
    for (int e = n - 1; e > 0; e--) {
        swap(a[0], a[e]); s.swp++; step(a, 0, e, 's', s);
        heapify(a, e, 0, s);
    }
}

/* ------------------------------------------------------------------ */
/*  Algorithm table                                                    */
/* ------------------------------------------------------------------ */
struct Algo {
    string name;
    void (*fn)(vector<int> &, Stats &);
    string best, avg, worst, space, stable;
};

static const vector<Algo> ALGOS = {
    {"Bubble Sort",    bubbleSort,    "O(n)",       "O(n^2)",     "O(n^2)",     "O(1)",     "Yes"},
    {"Selection Sort", selectionSort, "O(n^2)",     "O(n^2)",     "O(n^2)",     "O(1)",     "No"},
    {"Insertion Sort", insertionSort, "O(n)",       "O(n^2)",     "O(n^2)",     "O(1)",     "Yes"},
    {"Merge Sort",     mergeSort,     "O(n log n)", "O(n log n)", "O(n log n)", "O(n)",     "Yes"},
    {"Quick Sort",     quickSort,     "O(n log n)", "O(n log n)", "O(n^2)",     "O(log n)", "No"},
    {"Heap Sort",      heapSort,      "O(n log n)", "O(n log n)", "O(n log n)", "O(1)",     "No"}
};

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */
static mt19937 rng((unsigned)steady_clock::now().time_since_epoch().count());

// type: 1 = random, 2 = already sorted, 3 = reverse sorted
static vector<int> makeData(int n, int type) {
    vector<int> a(n);
    uniform_int_distribution<int> d(5, 99);
    for (int &x : a) x = d(rng);
    if (type == 2) sort(a.begin(), a.end());
    if (type == 3) sort(a.rbegin(), a.rend());
    return a;
}

static int readInt(const string &prompt, int lo, int hi) {
    int v;
    while (true) {
        cout << prompt;
        if (cin >> v && v >= lo && v <= hi) return v;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Please enter a number between " << lo << " and " << hi << ".\n";
    }
}

static void waitEnter() {
    cout << "\nPress Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

/* ------------------------------------------------------------------ */
/*  Menu options                                                       */
/* ------------------------------------------------------------------ */
static void printAlgoMenu() {
    for (size_t i = 0; i < ALGOS.size(); i++)
        cout << "  " << i + 1 << ". " << ALGOS[i].name << "\n";
}

static void visualize() {
    cout << "\nChoose algorithm:\n";
    printAlgoMenu();
    int c = readInt("Choice: ", 1, ALGOS.size()) - 1;
    int n = readInt("Array size (5 to 25): ", 5, 25);
    DELAY_MS = readInt("Delay per step in ms (0 to 1000): ", 0, 1000);

    vector<int> a = makeData(n, 1);
    Stats s;
    CUR_NAME = ALGOS[c].name;
    VIZ = true;
    ALGOS[c].fn(a, s);
    draw(a, -1, -1, 'd', s);
    VIZ = false;

    cout << "\nSorted correctly: " << (is_sorted(a.begin(), a.end()) ? "Yes" : "No") << "\n";
    cout << "Average time complexity: " << ALGOS[c].avg << "\n";
    waitEnter();
}

static void analyze() {
    cout << "\nInput type:\n  1. Random\n  2. Already sorted\n  3. Reverse sorted\n";
    int type = readInt("Choice: ", 1, 3);
    const vector<int> sizes = {100, 500, 1000, 5000, 10000};
    const int RUNS = 3;

    // results[algo][size] = {avg time in ms, comparisons, swaps}
    vector<vector<double>> T(ALGOS.size(), vector<double>(sizes.size()));
    vector<vector<Stats>> S(ALGOS.size(), vector<Stats>(sizes.size()));

    cout << "\nRunning analysis (each case is run " << RUNS << " times)...\n";
    for (size_t si = 0; si < sizes.size(); si++) {
        vector<int> data = makeData(sizes[si], type);
        for (size_t ai = 0; ai < ALGOS.size(); ai++) {
            double total = 0;
            for (int r = 0; r < RUNS; r++) {
                vector<int> copy = data;
                Stats st;
                auto t0 = steady_clock::now();
                ALGOS[ai].fn(copy, st);
                auto t1 = steady_clock::now();
                total += duration<double, milli>(t1 - t0).count();
                S[ai][si] = st;
            }
            T[ai][si] = total / RUNS;
        }
        cout << "  n = " << sizes[si] << " done\n";
    }

    cout << "\nAverage running time (ms)\n" << string(78, '-') << "\n"
         << left << setw(16) << "Algorithm";
    for (int n : sizes) cout << right << setw(11) << ("n=" + to_string(n));
    cout << "\n" << string(78, '-') << "\n" << fixed << setprecision(3);
    for (size_t ai = 0; ai < ALGOS.size(); ai++) {
        cout << left << setw(16) << ALGOS[ai].name;
        for (size_t si = 0; si < sizes.size(); si++) cout << right << setw(11) << T[ai][si];
        cout << "\n";
    }

    size_t last = sizes.size() - 1;
    cout << "\nOperation counts for n = " << sizes[last] << "\n" << string(50, '-') << "\n"
         << left << setw(16) << "Algorithm" << right << setw(17) << "Comparisons" << setw(17) << "Swaps/Writes" << "\n";
    for (size_t ai = 0; ai < ALGOS.size(); ai++)
        cout << left << setw(16) << ALGOS[ai].name << right
             << setw(17) << S[ai][last].cmp << setw(17) << S[ai][last].swp << "\n";

    size_t best = 0;
    for (size_t ai = 1; ai < ALGOS.size(); ai++)
        if (T[ai][last] < T[best][last]) best = ai;
    cout << "\nFastest for n = " << sizes[last] << ": " << ALGOS[best].name << "\n";

    // Save to CSV so you can make graphs in Excel
    ofstream f("results.csv");
    f << "Algorithm";
    for (int n : sizes) f << ",n=" << n << "(ms)";
    f << "\n";
    for (size_t ai = 0; ai < ALGOS.size(); ai++) {
        f << ALGOS[ai].name;
        for (size_t si = 0; si < sizes.size(); si++) f << "," << T[ai][si];
        f << "\n";
    }
    cout << "Results saved to results.csv (open in Excel to draw a graph).\n";
    waitEnter();
}

static void complexityTable() {
    cout << "\n" << left << setw(16) << "Algorithm" << setw(13) << "Best" << setw(13) << "Average"
         << setw(13) << "Worst" << setw(10) << "Space" << "Stable\n" << string(72, '-') << "\n";
    for (const Algo &a : ALGOS)
        cout << left << setw(16) << a.name << setw(13) << a.best << setw(13) << a.avg
             << setw(13) << a.worst << setw(10) << a.space << a.stable << "\n";
    waitEnter();
}

/* ------------------------------------------------------------------ */
int main() {
    enableAnsi();
    while (true) {
        cout << "\n==========================================\n"
             << " Sorting Algorithm Visualizer & Analyzer\n"
             << "==========================================\n"
             << "  1. Visualize a sorting algorithm\n"
             << "  2. Performance analysis\n"
             << "  3. Complexity table\n"
             << "  4. Exit\n";
        int ch = readInt("Choice: ", 1, 4);
        if (ch == 1) visualize();
        else if (ch == 2) analyze();
        else if (ch == 3) complexityTable();
        else break;
    }
    cout << "Bye!\n";
    return 0;
}