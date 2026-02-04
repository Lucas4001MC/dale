#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>


namespace ai {

// Simple Matrix structure to hold weights and biases
struct Matrix {
  int rows;
  int cols;
  std::vector<std::vector<double>> data;

  Matrix(int r, int c) : rows(r), cols(c) {
    data.resize(r, std::vector<double>(c, 0.0));
  }

  static Matrix fromVector(const std::vector<double> &vec) {
    Matrix m(vec.size(), 1);
    for (size_t i = 0; i < vec.size(); ++i) {
      m.data[i][0] = vec[i];
    }
    return m;
  }

  std::vector<double> toVector() const {
    std::vector<double> vec;
    for (const auto &row : data) {
      vec.insert(vec.end(), row.begin(), row.end());
    }
    return vec;
  }

  void randomize() {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < cols; ++j) {
        data[i][j] = dist(gen);
      }
    }
  }

  Matrix operator*(const Matrix &other) const {
    if (cols != other.rows) {
      // In a real app we'd throw or log error, currently just return empty
      return Matrix(0, 0);
    }
    Matrix result(rows, other.cols);
    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < other.cols; ++j) {
        double sum = 0;
        for (int k = 0; k < cols; ++k) {
          sum += data[i][k] * other.data[k][j];
        }
        result.data[i][j] = sum;
      }
    }
    return result;
  }

  void add(const Matrix &other) {
    if (rows != other.rows || cols != other.cols)
      return;
    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < cols; ++j) {
        data[i][j] += other.data[i][j];
      }
    }
  }

  void add(double val) {
    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < cols; ++j) {
        data[i][j] += val;
      }
    }
  }

  void map(double (*func)(double)) {
    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < cols; ++j) {
        data[i][j] = func(data[i][j]);
      }
    }
  }
};

inline double sigmoid(double x) { return 1.0 / (1.0 + std::exp(-x)); }

// Standard normal distribution for mutation
inline double randomGaussian() {
  static std::mt19937 gen(std::random_device{}());
  static std::normal_distribution<double> d(0.0, 1.0);
  return d(gen);
}

inline double randomDouble() {
  static std::mt19937 gen(std::random_device{}());
  static std::uniform_real_distribution<double> d(0.0, 1.0);
  return d(gen);
}

class NeuralNetwork {
public:
  std::vector<int> topology;
  std::vector<Matrix> weights;
  std::vector<Matrix> biases;

  NeuralNetwork(const std::vector<int> &_topology) : topology(_topology) {
    for (size_t i = 0; i < topology.size() - 1; ++i) {
      Matrix w(topology[i + 1], topology[i]);
      w.randomize();
      weights.push_back(w);

      Matrix b(topology[i + 1], 1);
      b.randomize();
      biases.push_back(b);
    }
  }

  // Copy constructor for cloning
  NeuralNetwork(const NeuralNetwork &other)
      : topology(other.topology), weights(other.weights), biases(other.biases) {
  }

  std::vector<double> feedForward(const std::vector<double> &inputVec) {
    if (inputVec.size() != topology[0]) {
      return {};
    }

    Matrix inputs = Matrix::fromVector(inputVec);

    for (size_t i = 0; i < weights.size(); ++i) {
      Matrix hidden = weights[i] * inputs;
      hidden.add(biases[i]);
      hidden.map(sigmoid);
      inputs = hidden;
    }

    return inputs.toVector();
  }

  void mutate(double rate, double strength) {
    for (auto &w : weights) {
      for (int i = 0; i < w.rows; ++i) {
        for (int j = 0; j < w.cols; ++j) {
          if (randomDouble() < rate) {
            w.data[i][j] += randomGaussian() * strength;
          }
        }
      }
    }
    for (auto &b : biases) {
      for (int i = 0; i < b.rows; ++i) {
        for (int j = 0; j < b.cols; ++j) {
          if (randomDouble() < rate) {
            b.data[i][j] += randomGaussian() * strength;
          }
        }
      }
    }
  }
};
} // namespace ai
