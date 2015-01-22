#include "AgentTest.h"

USING_NS_CC;

bool AgentTest::init()
{
    if (!Sprite::init())
    {
        return false;
    }

    initWithFile("sometestimage.png");

    tree = cctree::CoconutTree::create();
    tree->retain();
    tree->initWithFile("testtree"); // will search for trees/testtree.json

    blackboard = tree->getBlackboard();
    blackboard->agent = this;
    // for this tree to run, we'll need to set blackboard->target, after player creation

    scheduleUpdate();

    return true;
}

void AgentTest::update(float dt)
{
    tree->tick();
}