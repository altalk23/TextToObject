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

ObjectKernel kernelFromObject(int id, double scale, double rotation, double multiplier) {
    // get the texture name 
    auto spriteName = ObjectToolbox::sharedState()->intKeyToFrame(id);

    log::debug("Sprite name: {}", spriteName);

    // first get the sprite texture
    auto spriteFrame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(spriteName);
    auto frameSize = spriteFrame->getOriginalSizeInPixels();

    int width = frameSize.width * 2 * scale;
    int height = frameSize.height * 2 * scale;

    log::debug("Frame size: {}, {}", frameSize.width, frameSize.height);
    log::debug("Width: {}, Height: {}", width, height);

    // create a render texture
    auto renderTexture = CCRenderTexture::create(width, height, kCCTexture2DPixelFormat_RGBA8888);
    renderTexture->beginWithClear(0, 0, 0, 0);

    // draw the sprite
    auto sprite = CCSprite::createWithSpriteFrame(spriteFrame);
    sprite->setScale(scale);
    sprite->setRotation(rotation);
    sprite->setPosition({width / 8.0, height / 8.0});
    sprite->setFlipY(true);
    sprite->setAnchorPoint({0.5, 0.5});
    sprite->visit();

    // end the render texture
    renderTexture->end();

    // get the raw image
    auto image = renderTexture->newCCImage(false);
    auto data = image->getData();
    auto iWidth = image->getWidth();
    auto iHeight = image->getHeight();
    log::debug("Image size: {}, {}", iWidth, iHeight);

    // make image grayscale 
    std::vector<double> grayscale(width * height);
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++){
            auto index = (i * iWidth) + j;
            auto r = data[index * 4 + 0];
            auto g = data[index * 4 + 1];
            auto b = data[index * 4 + 2];
            auto a = data[index * 4 + 3];
            grayscale[i * width + j] = (0.299 * r + 0.587 * g + 0.114 * b) / 255.0;
            grayscale[i * width + j] *= a / 255.0;
            if (grayscale[i * width + j] < 0.7) {
                grayscale[i * width + j] = 0.0;
            }
            else {
                grayscale[i * width + j] = multiplier;
            }
        }
    }

    delete image;

    // // surround image with a 1px negative border
    // for (int i = 1; i < width - 1; ++i) {
    //     for (int j = 1; j < height - 1; ++j) {
    //         auto index = i * width + j;

    //         if (grayscale[index] >= 0.7) {
    //             if (grayscale[index - 1] < 0.7)
    //                 grayscale[index - 1] = -0.5;
    //             if (grayscale[index + 1] < 0.7)
    //                 grayscale[index + 1] = -0.5;
    //             if (grayscale[index - width] < 0.7)
    //                 grayscale[index - width] = -0.5;
    //             if (grayscale[index + width] < 0.7)
    //                 grayscale[index + width] = -0.5;
    //         }
    //     }
    // }

    // create the kernel
    ObjectKernel kernel = {
        std::move(grayscale),
        width,
        height,
        width / 8.0,
        height / 8.0,
        id,
        scale * 2.0,
        rotation
    };

    return kernel;
}

void testGenerator() {

    std::vector<ObjectKernel> kernels;

    // kernels.push_back(kernelFromObject(211, 0.04, 0.0));
    
    for (int i = 0; i < 1; ++i) {
        for (int j = 11; j <= 11; ++j) {
            double multiplier = 1.0;
            if (i == 0) multiplier = 1.1;
            kernels.push_back(kernelFromObject(211, j * 0.00833333333, i * 7.5, multiplier));
        }
    }

    // for (int i = 3; i <= 15; ++i) {
    //     kernels.push_back(kernelFromObject(1764, 0.05*i, 0.0));
    // }

    for (int i = 0; i < kernels.size(); ++i) {
        for (int x = 0; x < kernels[i].width; ++x) {
            for (int y = 0; y < kernels[i].height; ++y) {
                std::cout << (kernels[i].data[x * kernels[i].width + y] > 0.5 ? '#' : ' ')  << " ";
            }

            std::cout << std::endl;
        }
    }

    GeneratorConfig config = {
        std::mutex(),
        0.0,
        0.0,
        0.5,
        0.5,
        std::move(kernels),
        "/Users/student/Desktop/NotoSansJP-Regular.ttf",
        144.0,
        50,
        10.0,
        -5.0
    };

    auto objects = Generator::get()->create(U"コ", config);

    for (auto const& object : objects) {
        log::info("Object: x: {}, y: {}, id: {}, scale: {}, rotation: {}", object.x, object.y, object.objectId, object.scale, object.rotation);
        auto obj = LevelEditorLayer::get()->createObject(object.objectId, {object.x, object.y}, false);
        obj->setRotation(-object.rotation);
        obj->m_scale = object.scale;
        obj->setRScale(1.0f);

        obj->m_isObjectRectDirty = true;
        obj->m_textureRectDirty = true;
    }
}