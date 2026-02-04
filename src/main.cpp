#include <Geode/Geode.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <fmt/core.h>
#include <random>
#include <vector>


using namespace geode::prelude;

// --- CONFIGURACIÓN ---
const int AGENTS_PER_GEN = 50;
const int INPUT_NODES = 5;
const int HIDDEN_NODES = 6;

// --- UTILIDADES ---
float randomFloat(float min, float max) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(min, max);
  return dis(gen);
}

// --- CEREBRO ---
struct Genome {
  std::vector<float> weights;
  float fitness = 0.0f;

  void randomize() {
    weights.clear();
    int totalWeights =
        (INPUT_NODES * HIDDEN_NODES) + (HIDDEN_NODES * 1) + HIDDEN_NODES + 1;
    for (int i = 0; i < totalWeights; i++) {
      weights.push_back(randomFloat(-1.0f, 1.0f));
    }
  }

  float predict(const std::vector<float> &inputs) {
    std::vector<float> hiddenOutputs;
    int wIdx = 0;

    for (int i = 0; i < HIDDEN_NODES; i++) {
      float sum = 0.0f;
      for (int j = 0; j < INPUT_NODES; j++) {
        sum += inputs[j] * weights[wIdx++];
      }
      sum += weights[wIdx++];
      hiddenOutputs.push_back(sum > 0 ? sum : 0);
    }

    float outputSum = 0.0f;
    for (int i = 0; i < HIDDEN_NODES; i++) {
      outputSum += hiddenOutputs[i] * weights[wIdx++];
    }
    outputSum += weights[wIdx++];

    return outputSum;
  }

  void mutate(float rate) {
    for (size_t i = 0; i < weights.size(); i++) {
      if (randomFloat(0.0f, 1.0f) < rate) {
        weights[i] += randomFloat(-0.2f, 0.2f);
        if (weights[i] > 2.0f)
          weights[i] = 2.0f;
        if (weights[i] < -2.0f)
          weights[i] = -2.0f;
      }
    }
  }
};

// --- MANAGER ---
class TrainingManager {
public:
  std::vector<Genome> population;
  int currentAgentIndex = 0;
  int generation = 1;
  float currentSpeed = 1.0f;
  bool isWinningRun = false;
  float globalBest = 0.0f;

  static TrainingManager *get() {
    static TrainingManager instance;
    return &instance;
  }

  void initPopulation() {
    population.clear();
    for (int i = 0; i < AGENTS_PER_GEN; i++) {
      Genome g;
      g.randomize();
      population.push_back(g);
    }
    currentAgentIndex = 0;
    generation = 1;
    globalBest = 0.0f;
    isWinningRun = false;
  }

  Genome &getCurrentBrain() {
    if (population.empty())
      initPopulation();
    return population[currentAgentIndex];
  }

  void nextAgent() {
    currentAgentIndex++;
    if (currentAgentIndex >= AGENTS_PER_GEN) {
      evolvePopulation();
    }
  }

  void evolvePopulation() {
    std::sort(
        population.begin(), population.end(),
        [](const Genome &a, const Genome &b) { return a.fitness > b.fitness; });

    if (population[0].fitness > globalBest) {
      globalBest = population[0].fitness;
    }

    geode::log::info("Gen {} Finalizada. Best: {}", generation,
                     population[0].fitness);

    std::vector<Genome> newPop;

    for (int i = 0; i < 5; i++)
      newPop.push_back(population[i]);

    while (newPop.size() < AGENTS_PER_GEN) {
      int parentIdx = (int)randomFloat(0, 15);
      if (parentIdx >= AGENTS_PER_GEN)
        parentIdx = 0;

      Genome child = population[parentIdx];
      child.mutate(0.15f);
      child.fitness = 0;
      newPop.push_back(child);
    }

    population = newPop;
    currentAgentIndex = 0;
    generation++;
  }
};

// --- CONTROLES VELOCIDAD ---
class $modify(SpeedhackDispatcher, CCKeyboardDispatcher) {
  bool dispatchKeyboardMSG(enumKeyCodes key, bool isDown, bool isRepeat) {
    if (isDown && !isRepeat) {
      auto manager = TrainingManager::get();
      if (key == enumKeyCodes::KEY_U) {
        manager->currentSpeed += 1.0f;
        CCScheduler::get()->setTimeScale(manager->currentSpeed);
      }
      if (key == enumKeyCodes::KEY_I) {
        manager->currentSpeed -= 1.0f;
        if (manager->currentSpeed < 1.0f)
          manager->currentSpeed = 1.0f;
        CCScheduler::get()->setTimeScale(manager->currentSpeed);
      }
    }
    return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isDown, isRepeat);
  }
};

// --- JUEGO ---
class $modify(AIPlayLayer, PlayLayer) {
  struct Fields {
    CCLabelBMFont *infoLabel = nullptr;
  };

  bool init(GJGameLevel *level, bool useReplay, bool dontCreateObjects) {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects))
      return false;

    auto manager = TrainingManager::get();
    if (manager->population.empty())
      manager->initPopulation();

    CCScheduler::get()->setTimeScale(
        manager->isWinningRun ? 1.0f : manager->currentSpeed);

    auto label = CCLabelBMFont::create("Cargando IA...", "bigFont.fnt");
    auto winSize = CCDirector::get()->getWinSize();
    label->setPosition({5.0f, winSize.height - 5.0f});
    label->setAnchorPoint({0.0f, 1.0f});
    label->setScale(0.4f);
    label->setZOrder(1000);

    m_fields->infoLabel = label;

    if (this->m_uiLayer) {
      this->m_uiLayer->addChild(label);
    } else {
      this->addChild(label);
    }

    return true;
  }

  void update(float dt) {
    PlayLayer::update(dt);
    if (this->m_player1->m_isDead)
      return;

    // --- ACTUALIZAR HUD ---
    auto manager = TrainingManager::get();
    if (m_fields->infoLabel) {
      float currentX = this->m_player1->getPositionX();
      // Aseguramos que levelLength sea valido para evitar division por cero
      float levelLength =
          (this->m_levelLength > 0.0f) ? this->m_levelLength : 1000.0f;

      float currentPct = (currentX / levelLength) * 100.0f;
      float bestPct = (manager->globalBest / levelLength) * 100.0f;

      if (bestPct > 100.0f)
        bestPct = 100.0f;
      if (currentPct > 100.0f)
        currentPct = 100.0f;

      if (currentX > manager->globalBest) {
        manager->globalBest = currentX;
        bestPct = currentPct;
      }

      std::string text =
          fmt::format("Gen: {} | Agente: {}/{}\nSpeed: {}x\nBest: {:.2f}%",
                      manager->generation, manager->currentAgentIndex + 1,
                      AGENTS_PER_GEN, (int)manager->currentSpeed, bestPct);
      m_fields->infoLabel->setString(text.c_str());
    }

    // --- LÓGICA IA ---
    float playerX = this->m_player1->getPositionX();
    float playerY = this->m_player1->getPositionY();
    float distToObstacle = 1000.0f;
    float obstacleY = 0.0f;

    CCObject *objRef;
    CCARRAY_FOREACH(this->m_objects, objRef) {
      auto obj = typeinfo_cast<GameObject *>(objRef);
      if (!obj)
        continue;

      if (obj->getPositionX() > playerX) {
        auto type = obj->getType();
        if (type == GameObjectType::Solid || type == GameObjectType::Hazard) {
          float dist = obj->getPositionX() - playerX;
          if (dist < distToObstacle && dist < 300.0f) {
            distToObstacle = dist;
            obstacleY = obj->getPositionY();
          }
        }
      }
    }

    std::vector<float> inputs = {
        playerY / 1000.0f, distToObstacle / 500.0f, obstacleY / 1000.0f,
        (float)this->m_player1->m_yVelocity / 20.0f, 1.0f};

    float output = manager->getCurrentBrain().predict(inputs);

    if (output > 0.0f) {
      this->m_player1->pushButton(PlayerButton::Jump);
    } else {
      this->m_player1->releaseButton(PlayerButton::Jump);
    }
  }

  // --- FIX CRÍTICO DEL BUCLE ---
  void destroyPlayer(PlayerObject *player, GameObject *object) {
    auto manager = TrainingManager::get();

    // Verificamos que sea nuestro jugador
    if (player == this->m_player1) {

      // PROTECCIÓN: Si el jugador muere nada más nacer (x < 10),
      // es un bug de carga o spawn kill.
      // IGNORAMOS la lógica de la IA y dejamos que el juego haga un respawn
      // normal.
      if (player->getPositionX() < 10.0f) {
        PlayLayer::destroyPlayer(player, object);
        return;
      }

      // Si es un intento válido:
      if (manager->isWinningRun) {
        manager->isWinningRun = false;
        CCScheduler::get()->setTimeScale(manager->currentSpeed);
        this->resetLevel();
        return;
      }

      // Guardar progreso
      float distance = player->getPositionX();
      manager->getCurrentBrain().fitness = distance;

      if (distance > manager->globalBest) {
        manager->globalBest = distance;
      }

      // Siguiente agente
      manager->nextAgent();

      // Reiniciar nivel de forma segura
      Loader::get()->queueInMainThread([this] { this->resetLevel(); });
      return;
    }

    PlayLayer::destroyPlayer(player, object);
  }

  void levelComplete() {
    auto manager = TrainingManager::get();
    if (!manager->isWinningRun) {
      manager->isWinningRun = true;
      manager->globalBest = this->m_levelLength;

      Loader::get()->queueInMainThread([this] { this->resetLevel(); });
      return;
    }
    PlayLayer::levelComplete();
  }
};