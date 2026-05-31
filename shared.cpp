#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <condition_variable>

using namespace std;
using namespace std::chrono;

const long long N = 10000000LL;

long long v_unsafe = 0;
void addUnsafe() {
    for (long long i = 0; i < N; i++)
        v_unsafe = v_unsafe + 1;
}

long long v_mutex = 0;
mutex mtx;
void addMutex() {
    for (long long i = 0; i < N; i++) {
        lock_guard<mutex> lock(mtx);
        v_mutex = v_mutex + 1;
    }
}

atomic<long long> v_atomic(0);
void addAtomic() {
    for (long long i = 0; i < N; i++)
        v_atomic++;
}

void addLocal(long long* result) {
    long long local = 0;
    for (long long i = 0; i < N; i++)
        local++;
    *result = local;
}

const int STEPS = 1000;
int v_sync = 0;
mutex sync_mtx;
condition_variable cv;
int turn = 0;

void syncWorker(int id) {
    for (int i = 0; i < STEPS / 2; i++) {
        unique_lock<mutex> lock(sync_mtx);
        cv.wait(lock, [&]{ return turn == id; });
        v_sync++;
        turn = 1 - id;
        cv.notify_all();
    }
}

template<typename F>
long long measure(F func) {
    auto start = high_resolution_clock::now();
    func();
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

int main() {
    cout << "=== a) WITHOUT critical section (race condition) ===\n";
    long long t1 = measure([](){
        thread a(addUnsafe), b(addUnsafe);
        a.join(); b.join();
    });
    cout << "Expected: " << 2 * N << "\tGot: " << v_unsafe << "\tTime: " << t1 << " ms\n\n";

    cout << "=== b) WITH mutex (correct, but slow) ===\n";
    long long t2 = measure([](){
        thread a(addMutex), b(addMutex);
        a.join(); b.join();
    });
    cout << "Expected: " << 2 * N << "\tGot: " << v_mutex << "\tTime: " << t2 << " ms\n\n";

    cout << "=== WITH atomic (correct, faster than mutex) ===\n";
    long long t3 = measure([](){
        thread a(addAtomic), b(addAtomic);
        a.join(); b.join();
    });
    cout << "Expected: " << 2 * N << "\tGot: " << v_atomic.load() << "\tTime: " << t3 << " ms\n\n";

    cout << "=== Fastest correct (local sums + final merge) ===\n";
    long long r1 = 0, r2 = 0, vfast = 0;
    long long t4 = measure([&](){
        thread a(addLocal, &r1), b(addLocal, &r2);
        a.join(); b.join();
        vfast = r1 + r2;
    });
    cout << "Expected: " << 2 * N << "\tGot: " << vfast << "\tTime: " << t4 << " ms\n\n";

    cout << "=== Fully synchronous step-by-step (0 -> 1000) ===\n";
    long long t5 = measure([&](){
        thread a(syncWorker, 0), b(syncWorker, 1);
        a.join(); b.join();
    });
    cout << "Two parallel threads, shared var goes 0 -> " << v_sync << "\tTime: " << t5 << " ms\n";

    return 0;
}
