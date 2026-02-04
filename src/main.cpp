#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <vector>

using namespace geode::prelude;

// --- ESTRUCTURA DE LA RED NEURONAL ---
struct SimpleNet {
  std::vector<float> weights = {0.6f, -0.3f, 0.9f, 0.2f, -0.1f};

  float feedForward(const std::vector<float> &inputs) {
    float sum = 0.0f;
    for (size_t i = 0; i < inputs.size() && i < weights.size(); ++i) {
      sum += inputs[i] * weights[i];
    }
    return sum;
  }
};

class AIManager {
public:
  static AIManager *get() {
    static AIManager instance;
    return &instance;
  }

  SimpleNet currentBrain;
  bool isTraining = false;
};

class $modify(AIPlayLayer, PlayLayer) {

  bool init(GJGameLevel *level, bool useReplay, bool dontCreateObjects) {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects))
      return false;

    AIManager::get()->isTraining = true;
    geode::log::info("IA Iniciada. Esperando...");
    return true;
  }

  void update(float dt) {
    PlayLayer::update(dt);

    if (!AIManager::get()->isTraining || this->m_player1->m_isDead)
      return;

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
        // 0=Solid, 2=Hazard (Geode v4 mapping)
        if (type == GameObjectType::Solid || type == GameObjectType::Hazard) {

          float dist = obj->getPositionX() - playerX;

          if (dist < distToObstacle && dist < 250.0f) {
            distToObstacle = dist;
            obstacleY = obj->getPositionY();
          }
        }
      }
    }

    std::vector<float> inputs = {playerY / 1000.0f, distToObstacle / 500.0f,
                                 obstacleY / 1000.0f,
                                 (float)this->m_player1->m_yVelocity, 1.0f};

    float output = AIManager::get()->currentBrain.feedForward(inputs);

    if (output > 0.5f) {
      this->m_player1->pushButton(PlayerButton::Jump);
    } else {
      this->m_player1->releaseButton(PlayerButton::Jump);
    }
  }

  // --- AQUÍ ESTÁ EL FIX DEL BUCLE ---
  void destroyPlayer(PlayerObject *player, GameObject *object) {
    // Verificamos 3 cosas para reiniciar:
    // 1. Es el Player 1
    // 2. La IA está activa
    // 3. El jugador ha avanzado al menos 10 bloques (ESTO EVITA EL BUCLE)
    if (player == this->m_player1 && AIManager::get()->isTraining) {

      if (player->getPositionX() > 10.0f) {
        this->resetLevel();
        return;
      }
      // Si muere en X < 10, dejamos que el juego haga su muerte normal
      // para evitar reinicios infinitos durante la carga.
    }

    PlayLayer::destroyPlayer(player, object);
  }
};