#pragma once

#include "NeuralNetwork.hpp"
#include <algorithm>
#include <functional>
#include <vector>


namespace ai {

struct Agent {
  NeuralNetwork brain;
  double fitness = 0.0;
  bool dead = false;

  Agent(const std::vector<int> &topology) : brain(topology) {}
};

class PopulationManager {
public:
  std::vector<Agent> population;
  int generation = 1;
  int currentAgentIndex = 0;
  int populationSize = 50;
  std::vector<int> topology;
  double bestFitness = 0.0;

  PopulationManager(int size, std::vector<int> top)
      : populationSize(size), topology(top) {
    for (int i = 0; i < populationSize; ++i) {
      population.emplace_back(topology);
    }
  }

  Agent &getCurrentAgent() { return population[currentAgentIndex]; }

  bool isGenerationFinished() { return currentAgentIndex >= populationSize; }

  void nextAgent() { currentAgentIndex++; }

  // Simple selection: Sort by fitness, take top %, fill rest with mutated
  // clones
  void evolve() {
    // Sort, Descending fitness
    std::sort(
        population.begin(), population.end(),
        [](const Agent &a, const Agent &b) { return a.fitness > b.fitness; });

    bestFitness = std::max(bestFitness, population[0].fitness);

    std::vector<Agent> newPop;

    // Elitism: Keep best 5 agents
    int eliteCount = 5;
    for (int i = 0; i < eliteCount && i < populationSize; ++i) {
      Agent elite = population[i];
      elite.fitness = 0;
      elite.dead = false;
      newPop.push_back(elite);
    }

    // Fill the rest
    while (newPop.size() < populationSize) {
      // Select parents from top 50%
      int parentIdx = rand() % (populationSize / 2);
      Agent child = population[parentIdx]; // Copy parent

      // Mutate
      child.brain.mutate(0.1, 0.5); // 10% rate, 0.5 strength
      child.fitness = 0;
      child.dead = false;
      newPop.push_back(child);
    }

    population = newPop;
    generation++;
    currentAgentIndex = 0;
  }
};
} // namespace ai
