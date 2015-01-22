#include "GameScene.h"
#include "MenuScene.h"
#include "Utils/AnimatedSprite.h"
#include "Utils/AgentTest.h"

USING_NS_CC;

bool GameScene::init()
{
    if ( !Scene::init() )
    {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // Create scene here
    auto as = AnimatedSprite::create("projectile");
    as->runAnimation("start", [=]() { as->runAnimation("flight", [=]() { as->runAnimation("explosion", [=]() { as->getParent()->removeChild(as); }); }); });
    as->setPosition(origin + visibleSize / 2);
    addChild(as);

    auto backListener = EventListenerKeyboard::create();
    backListener->onKeyReleased = [](EventKeyboard::KeyCode key, Event* event) {
        if (key == EventKeyboard::KeyCode::KEY_BACK) Director::getInstance()->replaceScene(MenuScene::create());
    };
    Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(backListener, this);

    return true;
}