#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <vector>

using namespace geode::prelude;

// --- ESTRUCTURA DE LA RED NEURONAL ---
struct SimpleNet {
  std::vector<float> weights = {0.5f, -0.2f, 0.8f, 0.1f, -0.5f};

  float feedForward(const std::vector<float> &inputs) {
    float sum = 0.0f;
    for (size_t i = 0; i < inputs.size() && i < weights.size(); ++i) {
      sum += inputs[i] * weights[i];
    }
    return sum;
  }
};

// --- SINGLETON PARA GESTIONAR LA IA ---
class AIManager {
public:
  static AIManager *get() {
    static AIManager instance;
    return &instance;
  }

  SimpleNet currentBrain;
  bool isTraining = false;
};

// --- MODIFICACIÓN DEL JUEGO (HOOKS) ---
class $modify(AIPlayLayer, PlayLayer) {

  bool init(GJGameLevel *level, bool useReplay, bool dontCreateObjects) {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects))
      return false;

    AIManager::get()->isTraining = true;

    // Mensaje de consola para confirmar que el mod cargó
    geode::log::info("IA Iniciada: Esperando inputs...");

    return true;
  }

  void update(float dt) {
    PlayLayer::update(dt);

    if (!AIManager::get()->isTraining || this->m_player1->m_isDead)
      return;

    // 1. OBTENER DATOS (INPUTS)
    float playerX = this->m_player1->getPositionX();
    float playerY = this->m_player1->getPositionY();

    float distToObstacle = 1000.0f;
    float obstacleY = 0.0f;

    // --- BUCLE COMPATIBLE CON GEODE V4 ---
    CCObject *objRef;
    CCARRAY_FOREACH(this->m_objects, objRef) {
      auto obj = typeinfo_cast<GameObject *>(objRef);
      if (!obj)
        continue;

      // Filtro simple: Objetos peligrosos delante del jugador
      if (obj->m_hazard && obj->getPositionX() > playerX) {
        float dist = obj->getPositionX() - playerX;
        if (dist < distToObstacle) {
          distToObstacle = dist;
          obstacleY = obj->getPositionY();
        }
      }
    }

    // 2. INPUTS PARA LA RED
    std::vector<float> inputs = {
        playerY / 1000.0f, distToObstacle / 500.0f, obstacleY / 1000.0f,
        (float)this->m_player1
            ->m_yVelocity, // En v4 a veces es m_yVelocity directamente
        1.0f};

    // 3. DECISIÓN
    float output = AIManager::get()->currentBrain.feedForward(inputs);

    // --- CORRECCIÓN CRÍTICA AQUI ---
    // Usamos m_player1 y el Enum PlayerButton::Jump
    if (output > 0.5f) {
      this->m_player1->pushButton(PlayerButton::Jump);
    } else {
      this->m_player1->releaseButton(PlayerButton::Jump);
    }
  }

  void destroyPlayer(PlayerObject *player, GameObject *object) {
    if (player == this->m_player1 && AIManager::get()->isTraining) {
      // Reiniciar nivel instantáneamente
      this->resetLevel();
      return;
    }
    PlayLayer::destroyPlayer(player, object);
  }
};