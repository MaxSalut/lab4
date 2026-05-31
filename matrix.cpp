#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <random>

using namespace std;
using namespace std::chrono;

mutex print_mtx;

void fillMatrix(vector<vector<int>>& M, int rows, int cols) {
    mt19937 gen(random_device{}());
    uniform_int_distribution<int> dist(1, 9);
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            M[i][j] = dist(gen);
}

void computeCell(const vector<vector<int>>& A, const vector<vector<int>>& B,
                 vector<vector<int>>& C, int row, int col, int m, bool show) {
    int sum = 0;
    for (int t = 0; t < m; t++)
        sum += A[row][t] * B[t][col];
    C[row][col] = sum;
    if (show) {
        lock_guard<mutex> lock(print_mtx);
        cout << "[" << row << "," << col << "]=" << sum << "  "<<endl;
    }
}

long long multiplyWithThreads(const vector<vector<int>>& A, const vector<vector<int>>& B,
                              vector<vector<int>>& C, int n, int m, int k,
                              int maxThreads, bool show) {
    auto start = high_resolution_clock::now();
    vector<thread> pool;
    int total = n * k;
    for (int idx = 0; idx < total; idx++) {
        int row = idx / k;
        int col = idx % k;
        pool.push_back(thread(computeCell, cref(A), cref(B), ref(C), row, col, m, show));
        if ((int)pool.size() >= maxThreads) {
            for (auto& th : pool) th.join();
            pool.clear();
        }
    }
    for (auto& th : pool) th.join();
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

int main() {
    int n = 8, m = 8, k = 8;

    vector<vector<int>> A(n, vector<int>(m));
    vector<vector<int>> B(m, vector<int>(k));
    vector<vector<int>> C(n, vector<int>(k, 0));

    fillMatrix(A, n, m);
    fillMatrix(B, m, k);

    cout << "Demonstrating parallelism (results appear out of order) \n";
    multiplyWithThreads(A, B, C, n, m, k, 5, true);
    cout << "\n\nPerformance test (large matrix) \n";

    int bn = 150, bm = 150, bk = 150;
    vector<vector<int>> bA(bn, vector<int>(bm));
    vector<vector<int>> bB(bm, vector<int>(bk));
    vector<vector<int>> bC(bn, vector<int>(bk, 0));
    fillMatrix(bA, bn, bm);
    fillMatrix(bB, bm, bk);

    int threadCounts[] = {1, 2, 4, 5, 8, 16, 50};
    for (int tc : threadCounts) {
        long long ms = multiplyWithThreads(bA, bB, bC, bn, bm, bk, tc, false);
        cout << "Threads in parallel: " << tc << "\tTime: " << ms << " ms\n";
    }
    return 0;
}
