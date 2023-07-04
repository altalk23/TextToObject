#include <Geode/Geode.hpp>
#include <Generator.hpp>

using namespace geode::prelude;
using namespace tulip::text;

#include <Geode/modify/CCIMEDispatcher.hpp>

void testGenerator();

struct DispatcherHook : Modify<DispatcherHook, CCIMEDispatcher> {
    void dispatchInsertText(const char* text, int len) {
        auto text2 = reinterpret_cast<const unsigned char*>(text);
        std::vector<int> buffer(text2, text2 + len);
        log::info("Text: {}", fmt::format("{:#x}", fmt::join(buffer, ", ")));
        CCIMEDispatcher::dispatchInsertText(text, len);

        if (text[0] == 'J') {
            testGenerator();
        }
    }
};

void testGenerator() {
    ObjectKernel block01 = {
        std::vector<double>(6*6, 1.0),
        6,
        6,
        1.5,
        1.5,
        211,
        0.3,
        0.0
    };

    ObjectKernel block005 = {
        std::vector<double>(3*3, 1.0),
        3,
        3,
        3.0,
        3.0,
        211,
        0.05,
        0.0
    };

    GeneratorConfig config = {
        std::mutex(),
        0.0,
        0.0,
        0.5,
        0.5,
        std::vector<ObjectKernel>{block005},
        "/Users/student/Desktop/NotoSansJP-Regular.ttf",
        36.0,
        99,
        10.0,
        -4.0
    };

    auto objects = Generator::get()->create(U"アルカム", config);

    for (auto const& object : objects) {
        log::info("Object: x: {}, y: {}, id: {}, scale: {}, rotation: {}", object.x, object.y, object.objectId, object.scale, object.rotation);
        auto obj = LevelEditorLayer::get()->createObject(object.objectId, {object.x, object.y}, false);
        obj->setRotation(object.rotation);
        obj->m_scale = object.scale;
        obj->setRScale(1.0f);

        obj->m_isObjectRectDirty = true;
        obj->m_textureRectDirty = true;
    }
}