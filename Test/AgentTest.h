#ifndef __AGENT_TEST_H__
#define __AGENT_TEST_H__

#include "cocos2d.h"
#include "CoconutTrees/CoconutTree.h"

class AgentTest : public cocos2d::Sprite
{
public:
    virtual bool init();
    virtual void update(float dt);

    cctree::Blackboard* getBlackboard() { return blackboard; };

    CREATE_FUNC(AgentTest);

private:
    cctree::Blackboard* blackboard;
    cctree::CoconutTree* tree;
};

#endif // __AGENT_TEST_H__