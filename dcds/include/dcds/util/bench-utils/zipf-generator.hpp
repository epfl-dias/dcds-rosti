/*
                              Copyright (c) 2023.
          Data Intensive Applications and Systems Laboratory (DIAS)
                  École Polytechnique Fédérale de Lausanne

                              All Rights Reserved.

      Permission to use, copy, modify and distribute this software and
      its documentation is hereby granted, provided that both the
      copyright notice and this permission notice appear in all copies of
      the software, derivative works or modified versions, and any
      portions thereof, and that both notices appear in supporting
      documentation.

      This code is distributed in the hope that it will be useful, but
      WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
      DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
      RESULTING FROM THE USE OF THIS SOFTWARE.
 */

#ifndef DCDS_ZIPF_GENERATOR_HPP
#define DCDS_ZIPF_GENERATOR_HPP

#include <absl/random/zipf_distribution.h>

#include <random>

#include "dcds/common/common.hpp"

namespace dcds {

class ZipfianGenerator {
 public:
  ZipfianGenerator(size_t n_records, double theta)
      : engine((std::random_device{}())),
        _n(n_records - 1),
        //_theta(1.0 / (1.0 - theta)),
        _alpha(1.0 / (1.0 - theta)),
        _zetan(zeta(_n, theta)),
        _eta(calc_eta(theta, _n)),
        alpha_half_pow(1 + pow(0.5, theta)) {
    assert(theta < 1.0 && "Theta should be less 1.0");

    //    _alpha = 1.0 / (1.0 - theta);
    //    _zetan = zeta(_n, theta);
    //    _eta = (1.0 - std::pow(2.0 / _n, 1.0 - theta)) /
    //           (1.0 - zeta(2, theta) / (zeta(_n, theta)));
  }

  ~ZipfianGenerator() = default;

  [[maybe_unused]] inline size_t nextval() { return (*this)(); }

  inline size_t operator()() {
    // FIXME: I am not sure about having thread-local random generator,
    //  would this be correct distribution generator or it will cause problems?
    //    static thread_local std::mt19937 engine(std::random_device{}());
    //    static thread_local std::uniform_real_distribution<double> dist{0.0, 1.0};

    return this->val(dist(engine));
  }

 private:
  static constexpr double calc_eta(double theta, size_t _n) {
    return (1.0 - std::pow(2.0 / _n, 1.0 - theta)) / (1.0 - zeta(2, theta) / (zeta(_n, theta)));
  }
  static constexpr double zeta(size_t n, double theta_z) {
    double sum = 0;
    for (uint64_t i = 1; i <= n; i++) sum += std::pow(1.0 / i, theta_z);
    return sum;
  }

  [[nodiscard]] inline size_t val(double u) const {
    // double alpha_half_pow = 1 + pow(0.5, _theta);

    double uz = u * _zetan;
    size_t v;
    if (uz < 1) {
      v = 0;
    } else if (uz < 1 + alpha_half_pow) {
      v = 1;
    } else {
      v = static_cast<size_t>(_n * std::pow(_eta * u - _eta + 1, _alpha));
    }
    // LOG_IF(FATAL, v >= 0 && v <= _n_rec) << "v: " << v << " | _n_rec:"
    // <<_n_rec;
    assert(v >= 0 && v <= _n);
    return v;
  }

  std::mt19937 engine;
  std::uniform_real_distribution<double> dist{0.0, 1.0};

  const size_t _n;
  // const double _theta;
  const double _alpha;
  const double _zetan;
  const double _eta;
  const double alpha_half_pow;
};

class zip2 {
 private:
  std::mt19937 rw_gen;
  std::uniform_real_distribution<double> rw_dist{0.0, 1.0};
  double *sum_probs;  // Pre-calculated sum of probabilities
  const size_t n_r;

 public:
  ~zip2() { free(sum_probs); }

  zip2(size_t n, double alpha) : rw_gen(std::random_device{}()), n_r(n) {
    size_t i;             // Loop counter
    static double c = 0;  // Normalization constant

    // Compute normalization constant on first call only
    for (i = 1; i <= n; i++) c = c + (1.0 / pow(static_cast<double>(i), alpha));
    c = 1.0 / c;

    sum_probs = static_cast<double *>(malloc((n + 1) * sizeof(*sum_probs)));
    sum_probs[0] = 0;
    for (i = 1; i <= n; i++) {
      sum_probs[i] = sum_probs[i - 1] + c / pow(static_cast<double>(i), alpha);
    }
  }
  size_t zip_x() {
    double z;               // Uniform random number (0 < z < 1)
    size_t zipf_value = 0;  // Computed exponential value to be returned

    //    size_t low, high, mid;

    // Pull a uniform random number (0 < z < 1)
    do {
      z = rw_dist(rw_gen);  // rand_val(0);
    } while ((z == 0.0) || (z == 1.0));

    // Map z to the value
    size_t low = 1, high = n_r, mid;  // Binary-search bounds
    do {
      mid = floor((low + high) / 2);
      if (sum_probs[mid] >= z && sum_probs[mid - 1] < z) {
        zipf_value = mid;
        break;
      } else if (sum_probs[mid] >= z) {
        high = mid - 1;
      } else {
        low = mid + 1;
      }
    } while (low <= high);

    // Assert that zipf_value is between 1 and N
    CHECK((zipf_value >= 0) && (zipf_value <= n_r)) << "zipf: " << zipf_value;
    // assert((zipf_value >= 1) && (zipf_value <= n_r));

    return (zipf_value);
  }

  inline size_t operator()() { return this->zip_x(); }
};

constexpr size_t seed = 42;
class absl_zipf {
 public:
  absl_zipf(size_t n_records, double theta)
      : distrib(n_records, 2.0, theta),
        gen(seed)
  /*engine((std::random_device{}()))*/
  {}

  inline size_t operator()() { return this->distrib(gen); }

  absl::zipf_distribution<size_t> distrib;
  std::mt19937 gen;
};

}  // namespace dcds

#endif  // DCDS_ZIPF_GENERATOR_HPP
