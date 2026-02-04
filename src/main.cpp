#include "GeneticAlgorithm.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>


using namespace geode::prelude;

// Global AI Manager
ai::PopulationManager *g_popManager = nullptr;
CCLabelBMFont *g_statusLabel = nullptr;
bool g_initialized = false;

// Helper to initialize AI if needed
void initAI() {
  if (!g_initialized) {
    // Topology: 5 Inputs -> 8 Hidden -> 8 Hidden -> 1 Output
    std::vector<int> topology = {5, 8, 8, 1};
    g_popManager = new ai::PopulationManager(50, topology);
    g_initialized = true;
    log::info("AI Initialized with Population 50");
  }
}

class $modify(AIPlayLayer, PlayLayer) {
  bool init(GJGameLevel *level, bool useReplay, bool dontCreateObjects) {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects))
      return false;

    initAI();

    // Label for stats
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    g_statusLabel = CCLabelBMFont::create("Gen: 1 | Agent: 1", "bigFont.fnt");
    g_statusLabel->setPosition(winSize.width / 2, winSize.height - 20);
    g_statusLabel->setScale(0.5f);
    this->addChild(g_statusLabel, 1000);

    return true;
  }

  void update(float dt) {
    PlayLayer::update(dt);

    if (!g_popManager || this->m_isPaused)
      return;

    // 1. Get Game State
    auto player = this->m_player1;
    if (!player)
      return;

    double pY = player->getPositionY();
    double pVelY = player->m_yVelocity;

    // 2. Obstacle Detection
    GameObject *nearestObj = nullptr;
    double minDst = 99999.0;

    // Iterate m_objects (CCArray) using Geode v5 CCArrayExt
    if (this->m_objects) {
      for (auto obj : CCArrayExt<GameObject *>(this->m_objects)) {
        if (!obj)
          continue;

        // Only care about objects in front
        double dx = obj->getPositionX() - player->getPositionX();
        if (dx > 0 && dx < minDst) {
          minDst = dx;
          nearestObj = obj;
        }
      }
    }

    double objDist = 0;
    double objY = 0;
    double objType = 0; // 0 = None, 1 = Hazard/Solid

    if (nearestObj) {
      objDist = minDst;
      objY = nearestObj->getPositionY();
      objType = 1.0;
    } else {
      objDist = 2000.0; // Far away
    }

    // 3. Normalize Inputs
    std::vector<double> inputs = {pY / 1000.0, pVelY / 20.0, objDist / 1000.0,
                                  objY / 1000.0, objType};

    // 4. Feed Forward
    ai::Agent &currentAgent = g_popManager->getCurrentAgent();
    std::vector<double> outputs = currentAgent.brain.feedForward(inputs);

    // 5. Act
    if (outputs[0] > 0.5) {
      this->m_player1->pushButton(PlayerButton::Jump);
    } else {
      this->m_player1->releaseButton(PlayerButton::Jump);
    }
  }

  void destroyPlayer(PlayerObject *player, GameObject *object) {
    // Hook death to record fitness and switch agent
    PlayLayer::destroyPlayer(player, object);

    if (!g_popManager)
      return;

    // Only care about P1 death
    if (player != this->m_player1)
      return;

    // Calculate Fitness: Distance traveled
    // Player X is a good metric.
    float fitness = player->getPositionX();
    g_popManager->getCurrentAgent().fitness = fitness;
    g_popManager->getCurrentAgent().dead = true;

    log::info("Agent {} died. Fitness: {}", g_popManager->currentAgentIndex + 1,
              fitness);

    // Move to next
    g_popManager->nextAgent();

    if (g_popManager->isGenerationFinished()) {
      log::info("Generation {} finished. Evolving...",
                g_popManager->generation);
      g_popManager->evolve();
    }

    // Update Label
    if (g_statusLabel) {
      std::string text = fmt::format(
          "Gen: {} | Agent: {} | Best: {:.0f}", g_popManager->generation,
          g_popManager->currentAgentIndex + 1, g_popManager->bestFitness);
      g_statusLabel->setString(text.c_str());
    }

    this->resetLevel();
  }
};