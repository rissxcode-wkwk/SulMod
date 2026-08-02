#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

// ============================================================
// SuLMod - Mod Geode
// Fitur:
//  1. Tampilkan hitbox player secara real-time (kotak merah/custom)
//  2. Speedhack: ubah kecepatan gameplay via multiplier
//  3. Menu UI in-game (tombol di pause menu) buat toggle & atur speed
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

    void drawHitbox(CCRect const& rect, ccColor4F const& color) {
        CCPoint pts[4] = {
            {rect.getMinX(), rect.getMinY()},
            {rect.getMaxX(), rect.getMinY()},
            {rect.getMaxX(), rect.getMaxY()},
            {rect.getMinX(), rect.getMaxY()}
        };
        this->drawPolygon(pts, 4, {0, 0, 0, 0}, 1.5f, color);
    }
};

// ============================================================
// POPUP MENU: SuLMod
// Popup kecil berisi toggle hitbox + tombol -/+ buat atur speed
// Catatan: di Geode SDK v5.x, geode::Popup TIDAK pakai template <>
// lagi, cukup "public geode::Popup" dan override init() (bukan setup()).
// ============================================================
class SuLMenuPopup : public geode::Popup {
protected:
    CCLabelBMFont* m_speedLabel = nullptr;
    CCMenuItemToggler* m_hitboxToggle = nullptr;

    // Format angka speed jadi teks 1 desimal, misal "1.0x"
    std::string speedText(double val) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1fx", val);
        return std::string(buf);
    }

    void onToggleHitbox(CCObject* sender) {
        auto toggler = static_cast<CCMenuItemToggler*>(sender);
        bool newState = !toggler->isOn();
        Mod::get()->setSettingValue<bool>("show-hitboxes", newState);
    }

    void onSpeedDown(CCObject*) {
        double val = Mod::get()->getSettingValue<double>("speed-multiplier");
        val = std::max(0.1, val - 0.1);
        Mod::get()->setSettingValue<double>("speed-multiplier", val);
        m_speedLabel->setString(this->speedText(val).c_str());
    }

    void onSpeedUp(CCObject*) {
        double val = Mod::get()->getSettingValue<double>("speed-multiplier");
        val = std::min(3.0, val + 0.1);
        Mod::get()->setSettingValue<double>("speed-multiplier", val);
        m_speedLabel->setString(this->speedText(val).c_str());
    }

    bool init() {
        if (!Popup::init(240.f, 160.f)) return false;

        this->setTitle("SuLMod");

        auto winSize = m_mainLayer->getContentSize();
        auto popupMenu = CCMenu::create();
        popupMenu->setPosition({0, 0});
        m_mainLayer->addChild(popupMenu);

        // --- Toggle Hitbox ---
        auto hitboxLabel = CCLabelBMFont::create("Tampilkan Hitbox", "bigFont.fnt");
        hitboxLabel->setScale(0.4f);
        hitboxLabel->setAnchorPoint({0, 0.5f});
        hitboxLabel->setPosition({winSize.width / 2 - 85, winSize.height / 2 + 20});
        m_mainLayer->addChild(hitboxLabel);

        bool hitboxOn = Mod::get()->getSettingValue<bool>("show-hitboxes");
        auto offSpr = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
        auto onSpr = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
        m_hitboxToggle = CCMenuItemToggler::create(
            offSpr, onSpr, this, menu_selector(SuLMenuPopup::onToggleHitbox)
        );
        m_hitboxToggle->toggle(hitboxOn);
        m_hitboxToggle->setPosition({winSize.width / 2 + 60, winSize.height / 2 + 20});
        popupMenu->addChild(m_hitboxToggle);

        // --- Atur Speed Multiplier ---
        auto speedTitle = CCLabelBMFont::create("Kecepatan", "bigFont.fnt");
        speedTitle->setScale(0.4f);
        speedTitle->setAnchorPoint({0, 0.5f});
        speedTitle->setPosition({winSize.width / 2 - 85, winSize.height / 2 - 20});
        m_mainLayer->addChild(speedTitle);

        auto minusSpr = ButtonSprite::create("-");
        minusSpr->setScale(0.7f);
        auto minusBtn = CCMenuItemSpriteExtra::create(
            minusSpr, this, menu_selector(SuLMenuPopup::onSpeedDown)
        );
        minusBtn->setPosition({winSize.width / 2 + 30, winSize.height / 2 - 20});
        popupMenu->addChild(minusBtn);

        double speedVal = Mod::get()->getSettingValue<double>("speed-multiplier");
        m_speedLabel = CCLabelBMFont::create(this->speedText(speedVal).c_str(), "bigFont.fnt");
        m_speedLabel->setScale(0.5f);
        m_speedLabel->setPosition({winSize.width / 2 + 60, winSize.height / 2 - 20});
        m_mainLayer->addChild(m_speedLabel);

        auto plusSpr = ButtonSprite::create("+");
        plusSpr->setScale(0.7f);
        auto plusBtn = CCMenuItemSpriteExtra::create(
            plusSpr, this, menu_selector(SuLMenuPopup::onSpeedUp)
        );
        plusBtn->setPosition({winSize.width / 2 + 90, winSize.height / 2 - 20});
        popupMenu->addChild(plusBtn);

        return true;
    }

public:
    static SuLMenuPopup* create() {
        auto ret = new SuLMenuPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

// ============================================================
// Hook ke PlayLayer supaya bisa menggambar hitbox tiap frame
// dan menerapkan speedhack pada update loop
// ============================================================
class $modify(PTPlayLayer, PlayLayer) {
    struct Fields {
        HitboxDrawNode* hitboxNode = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        auto node = HitboxDrawNode::create();
        node->setID("pt-hitbox-node"_spr);
        node->setZOrder(1000);
        this->addChild(node);
        m_fields->hitboxNode = node;

        return true;
    }

    void update(float dt) {
        float multiplier = Mod::get()->getSettingValue<double>("speed-multiplier");
        PlayLayer::update(dt * multiplier);

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

                m_fields->hitboxNode->drawHitbox(m_player1->getObjectRect(), color);

                if (m_player2 && this->m_gameState.m_isDualMode) {
                    m_fields->hitboxNode->drawHitbox(m_player2->getObjectRect(), color);
                }
            }
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (m_fields->hitboxNode) {
            m_fields->hitboxNode->clear();
        }
    }
};

// ============================================================
// Hook ke PauseLayer buat nambahin tombol "SuLMod" di menu pause
// ============================================================
class $modify(PTPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        CCSprite* spr = CCSprite::createWithSpriteFrameName("geode.loader/settings.png");
        CCMenuItemSpriteExtra* btn;

        if (spr) {
            spr->setScale(0.9f);
            btn = CCMenuItemSpriteExtra::create(
                spr, this, menu_selector(PTPauseLayer::onSuLMod)
            );
        } else {
            // fallback kalau sprite frame gak ketemu, pakai ButtonSprite teks
            auto bs = ButtonSprite::create("SuL");
            bs->setScale(0.6f);
            btn = CCMenuItemSpriteExtra::create(
                bs, this, menu_selector(PTPauseLayer::onSuLMod)
            );
        }

        // Cari menu tombol pause yang sudah ada, taruh di pojok
        if (auto existingMenu = this->getChildByID("left-button-menu")) {
            btn->setID("sulmod-open-btn"_spr);
            existingMenu->addChild(btn);
            static_cast<CCMenu*>(existingMenu)->updateLayout();
        } else {
            // fallback: taruh manual kalau ID menu gak ketemu
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            btn->setPosition({40, winSize.height - 40});
            auto fallbackMenu = CCMenu::create();
            fallbackMenu->addChild(btn);
            fallbackMenu->setPosition({0, 0});
            this->addChild(fallbackMenu);
        }
    }

    void onSuLMod(CCObject*) {
        SuLMenuPopup::create()->show();
    }
};
