// https://usaco.org/index.php?page=viewproblem2&cpid=698
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <random>
#include <vector>

using namespace std;

std::uniform_int_distribution<long long> distrib;
std::uniform_real_distribution<double> dbl_distrib;
// 1. Obtain a seed from the system's random device
// random_device produces non-deterministic results if available
std::random_device rd;
// 2. Seed a high-quality 64-bit Mersenne Twister engine
// std::mt19937_64 is preferred for 64-bit random numbers
std::mt19937_64 gen(rd());

void init() {
  //  Define the desired range (e.g., from 0 to the maximum possible long long)
  long long min_val = 0;
  long long max_val = std::numeric_limits<long long>::max();
  // Define the distribution
  distrib = std::uniform_int_distribution<long long>(min_val, max_val);
  dbl_distrib = std::uniform_real_distribution<double>(0.0, 1.0);
}

int compute_lis(const vector<int> &input) {
  if (input.empty()) return 0;
  // dp[v] = max LIS length ending with value v
  int dp[51] = {0};
  int ans = 0;
  for (int x : input) {
    int best = 0;
    for (int v = 1; v <= x; v++) {
      best = max(best, dp[v]);
    }
    dp[x] = max(dp[x], best + 1);
    ans = max(ans, dp[x]);
  }
  return ans;
}

vector<int> reverse_subsequence(const vector<int> &input, long long mask) {
  int n = input.size();
  vector<int> result(input);
  // Collect indices in the subsequence
  vector<int> indices;
  for (int i = 0; i < n; i++) {
    if (mask & (1LL << i)) {
      indices.push_back(i);
    }
  }
  // Reverse the values at those indices
  int left = 0, right = indices.size() - 1;
  while (left < right) {
    swap(result[indices[left]], result[indices[right]]);
    left++;
    right--;
  }
  return result;
}

int brute_solve(const vector<int> &input) {
  assert(input.size() <= 20);
  int n = input.size();
  int best = 0;
  for (int mask = 0; mask < (1 << n); mask++) {
    vector<int> tmp = reverse_subsequence(input, mask);
    best = max(best, compute_lis(tmp));
  }
  return best;
}

double compute_std_deviation(const vector<int> &input, bool use_std_dev) {
  int n = input.size();
  const long long modulo = (1LL << n);
  vector<int> costs;
  for (int iter = 0; iter < 1000; iter++) {
    long long state = distrib(gen) % modulo;
    costs.push_back(compute_lis(reverse_subsequence(input, state)));
  }
  // Compute mean
  double sum = 0.0;
  for (int c : costs) {
    sum += c;
  }
  double mean = sum / costs.size();
  if (!use_std_dev) {
    return mean;
  }
  // Compute variance
  double variance = 0.0;
  for (int c : costs) {
    variance += (c - mean) * (c - mean);
  }
  variance /= costs.size();
  return sqrt(variance);
}

int simulated_annealing_solve_bad(const vector<int> &input) {
  /**
   * This doesn't work because the number of states is too high (2^n == 2^50), and just changing 1 bit each time means that
   * we only end up exploring a very small area around the first randomly selected state.
   */
  int n = input.size();
  const long long FULL = (1LL << n) - 1;
  const int BATCH = 10;
  const double T0 = compute_std_deviation(input, true);
  const double COOLING = 0.99;
  auto acceptance = [&](double c_old, double c_new, const double t) -> double {
    if (c_old < c_new) {
      return 1.0;
    }
    return std::exp((c_new - c_old) / t);
  };
  long long state = distrib(gen) % (1LL << n);
  vector<int> tmp = reverse_subsequence(input, state);
  int best = compute_lis(input);
  int c_old = compute_lis(tmp);
  while (clock() / (double) CLOCKS_PER_SEC <= 1.95) {
    double t = T0;
    for (int b = 0; b < BATCH; b++) {
      int bit_to_flip = (distrib(gen) % n);
      long long neighbor = (1LL << bit_to_flip) ^ state;
      vector<int> tmp2 = reverse_subsequence(input, neighbor);
      int c_new = compute_lis(tmp2);
      best = max(best, max(c_old, c_new));
      if (acceptance(c_old, c_new, t) >= dbl_distrib(gen)) {
        tmp = tmp2;
        state = neighbor;
        c_old = c_new;
      }
    }
    t = t * COOLING;
  }
  return best;
}

int simulated_annealing_solve(const vector<int> &input) {
  int n = input.size();
  const double T0 = compute_std_deviation(input, true);
  const double COOLING = 0.9999;
  auto acceptance = [&](double c_old, double c_new, const double t) -> double {
    if (c_old < c_new) {
      return 1.0;
    }
    return std::exp((c_new - c_old) / t);
  };
  int best = compute_lis(input);
  // Initialize random state.
  long long state = 0;
  for (int idx = 0; idx < n; idx++) {
    if ((distrib(gen) & 1LL) > 0) {
      state ^= (1LL << idx);
    }
  }
  double temperature = T0;
  vector<int> tmp = reverse_subsequence(input, state);
  int batch = 1;
  int c_old = compute_lis(tmp);
  int num_eval = 0;
  while (clock() / (double) CLOCKS_PER_SEC <= 1.95) {
    num_eval++;
    if (num_eval % batch == 0) {
      temperature *= COOLING;
    }
    long long new_state = state;
    int max_iters = n * (temperature / T0);
    if (max_iters == 0) {
      max_iters = min(5, n / 2);
    }
    for (int idx = 0; idx < max_iters; idx++) {
      long long bb = 1LL << (distrib(gen) % n);
      new_state ^= (bb);
    }
    vector<int> tmp2 = reverse_subsequence(input, new_state);
    int c_new = compute_lis(tmp2);
    best = max(best, max(c_old, c_new));
    int x = 1 + rand();
    int y = 1 + rand();
    x %= y;
    // cerr << dbl_distrib(gen) << " " << (x / (double) y) << endl;
    if (acceptance(c_old, c_new, temperature) >= x / (double) y) {
      state = new_state;
      tmp = tmp2;
      c_old = c_new;
    }
  }
  return best;
}

int main() {
  ios_base::sync_with_stdio(false);
  // freopen("subrev.in", "r", stdin);
  // freopen("subrev.out", "w", stdout);
  init();
  int n;
  cin >> n;
  vector<int> arr(n);
  for (int i = 0; i < n; i++) {
    cin >> arr[i];
  }
  int answer = simulated_annealing_solve(arr);
  cout << answer << endl;
  return 0;
}