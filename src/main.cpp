#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <vector>

using namespace geode::prelude;

// --- ESTRUCTURA DE LA RED NEURONAL ---
struct SimpleNet {
  // Pesos ajustados ligeramente para favorecer saltar pinchos
  std::vector<float> weights = {0.6f, -0.3f, 0.9f, 0.2f, -0.1f};

  float feedForward(const std::vector<float> &inputs) {
    float sum = 0.0f;
    for (size_t i = 0; i < inputs.size() && i < weights.size(); ++i) {
      sum += inputs[i] * weights[i];
    }
    return sum;
  }
};

// --- SINGLETON ---
class AIManager {
public:
  static AIManager *get() {
    static AIManager instance;
    return &instance;
  }

  SimpleNet currentBrain;
  bool isTraining = false;
};

// --- EL MOD ---
class $modify(AIPlayLayer, PlayLayer) {

  bool init(GJGameLevel *level, bool useReplay, bool dontCreateObjects) {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects))
      return false;

    AIManager::get()->isTraining = true;
    geode::log::info("IA V4 Iniciada: Detectando Obstaculos Reales");
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

    // --- BUCLE INTELIGENTE ---
    CCObject *objRef;
    CCARRAY_FOREACH(this->m_objects, objRef) {
      auto obj = typeinfo_cast<GameObject *>(objRef);
      if (!obj)
        continue;

      // Solo nos importan los objetos DELANTE del jugador
      if (obj->getPositionX() > playerX) {

        // --- AQUI ESTA LA MAGIA: FILTRO POR TIPO ---
        // Usamos el Enum GameObjectType para saber qué es cada cosa
        // 7 = Spike, 0 = Solid (Bloques), 8 = Hazard (Sierras, etc)
        // Nota: Usamos comparacion de Enums o ints directos si el enum varia

        auto type = obj->getType();
        bool isDangerous = false;

        if (type == GameObjectType::Spike || type == GameObjectType::Solid ||
            type == GameObjectType::Hazard || type == GameObjectType::Monster) {
          isDangerous = true;
        }

        if (isDangerous) {
          float dist = obj->getPositionX() - playerX;

          // Buscamos el peligro mas cercano (rango de vision 250 bloques)
          if (dist < distToObstacle && dist < 250.0f) {
            distToObstacle = dist;
            obstacleY = obj->getPositionY();
          }
        }
      }
    }

    // INPUTS
    std::vector<float> inputs = {playerY / 1000.0f, distToObstacle / 500.0f,
                                 obstacleY / 1000.0f,
                                 (float)this->m_player1->m_yVelocity, 1.0f};

    // DECISIÓN
    float output = AIManager::get()->currentBrain.feedForward(inputs);

    if (output > 0.5f) {
      this->m_player1->pushButton(PlayerButton::Jump);
    } else {
      this->m_player1->releaseButton(PlayerButton::Jump);
    }
  }

  void destroyPlayer(PlayerObject *player, GameObject *object) {
    if (player == this->m_player1 && AIManager::get()->isTraining) {
      this->resetLevel();
      return;
    }
    PlayLayer::destroyPlayer(player, object);
  }
};