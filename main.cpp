#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

// ============================================================
// PRACTICE TOOLS - Mod Geode
// Fitur:
//  1. Tampilkan hitbox player secara real-time (kotak merah/custom)
//  2. Speedhack: ubah kecepatan gameplay via multiplier (setting)
// ============================================================

// Node custom untuk menggambar kotak hitbox di atas layar
class HitboxDrawNode : public CCDrawNode {
public:
    static HitboxDrawNode* create() {
        auto ret = new HitboxDrawNode();
        if (ret->CCDrawNode::init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    // Gambar ulang kotak hitbox berdasarkan rect object
    void drawHitbox(CCRect const& rect, ccColor4F const& color) {
        // Susun 4 titik sudut rect (searah jarum jam)
        CCPoint pts[4] = {
            {rect.getMinX(), rect.getMinY()},
            {rect.getMaxX(), rect.getMinY()},
            {rect.getMaxX(), rect.getMaxY()},
            {rect.getMinX(), rect.getMaxY()}
        };
        // drawPolygon dengan isi transparan, hanya garis tepi yang terlihat
        this->drawPolygon(pts, 4, {0, 0, 0, 0}, 1.5f, color);
    }
};

// Hook ke PlayLayer supaya bisa menggambar hitbox tiap frame
// dan menerapkan speedhack pada update loop
class $modify(PTPlayLayer, PlayLayer) {
    struct Fields {
        HitboxDrawNode* hitboxNode = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        // Siapkan node untuk gambar hitbox, taruh di layer paling atas
        auto node = HitboxDrawNode::create();
        node->setID("pt-hitbox-node"_spr);
        node->setZOrder(1000);
        this->addChild(node);
        m_fields->hitboxNode = node;

        return true;
    }

    void update(float dt) {
        // Ambil setting kelipatan kecepatan dari mod.json
        float multiplier = Mod::get()->getSettingValue<double>("speed-multiplier");

        // Terapkan speedhack: kalikan delta time sebelum diteruskan ke game asli.
        // multiplier < 1 = slow motion (bagus buat latihan bagian susah)
        // multiplier > 1 = mempercepat gameplay
        PlayLayer::update(dt * multiplier);

        // Update tampilan hitbox tiap frame kalau fiturnya aktif
        bool showHitbox = Mod::get()->getSettingValue<bool>("show-hitboxes");
        if (m_fields->hitboxNode) {
            m_fields->hitboxNode->clear();

            if (showHitbox && m_player1) {
                auto rgb = Mod::get()->getSettingValue<ccColor3B>("hitbox-color");
                ccColor4F color = {
                    rgb.r / 255.f,
                    rgb.g / 255.f,
                    rgb.b / 255.f,
                    1.f
                };

                // Hitbox player 1
                m_fields->hitboxNode->drawHitbox(m_player1->getObjectRect(), color);

                // Kalau dual mode aktif, gambar juga hitbox player 2
                if (m_player2 && this->m_gameState.m_isDualMode) {
                    m_fields->hitboxNode->drawHitbox(m_player2->getObjectRect(), color);
                }
            }
        }
    }

    // Reset speed & hitbox tampilan tiap kali level di-restart
    void resetLevel() {
        PlayLayer::resetLevel();
        if (m_fields->hitboxNode) {
            m_fields->hitboxNode->clear();
        }
    }
};
