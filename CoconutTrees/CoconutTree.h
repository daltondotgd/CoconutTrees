#ifndef __COCONUT_TREE_H__
#define __COCONUT_TREE_H__

/*
 * To use along with: http://behavior3js.guineashots.com/editor/
 * Author: Krzysztof Pachulski
 * License: MIT
 */

#include "cocos2d.h"
#include "../../libs/json/json.h"
#include <ctime>

namespace cctree {

    enum Status { SUCCESS, FAILURE, RUNNING, ERROR_OCCURED };

    class Blackboard : public cocos2d::Ref
    {
    public:
        virtual bool init() { return true; };

        CREATE_FUNC(Blackboard);

        cocos2d::Node* target;
        cocos2d::Node* agent;

        Json::Value parameters;
        Json::Value properties;
    };

    // ------------------- NODES BEGIN -------------------

    class Node : public cocos2d::Ref
    {
    public:
        virtual Status execute(Blackboard* blackboard) = 0;
        virtual void addChild(Node* node) { }
        virtual bool init() { return true; }

        void setUUID(std::string value) { uuid = value; }

    protected:
        std::string uuid;
    };

    // ------------------- BASIC NODES -------------------

    class Composite : public Node
    {
    public:
        virtual void addChild(Node* node) { children.push_back(node); }

    protected:
        std::vector<Node*> children;
    };

    class Decorator : public Node
    {
    public:
        virtual void addChild(Node* node) { child = node; }

    protected:
        Node* child;
    };

    class Action : public Node
    {
    };

    class Condition : public Node
    {
    };

    // ------------------- ROOT -------------------

    class Root : public Node
    {
    public:
        virtual Status execute(Blackboard* blackboard) { return child->execute(blackboard); }
        virtual void addChild(Node* node) { child = node; }

        CREATE_FUNC(Root);

    protected:
        Node* child;
    };

    // ------------------- COMPOSITE NODES -------------------

    class Sequence : public Composite
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            for (auto child : children)
            {
                auto status = child->execute(blackboard);
                if (status != SUCCESS) return status;
            }

            return SUCCESS;
        }

        CREATE_FUNC(Sequence);
    };

    class MemSequence : public Composite
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            for (auto child : children)
            {
                auto locked = lastRunning != nullptr;
                if (!locked || lastRunning == child)
                {
                    auto status = child->execute(blackboard);

                    if (status == RUNNING)
                    {
                        lastRunning = child;
                        return RUNNING;
                    }

                    lastRunning = nullptr;
                    if (status != SUCCESS) return status;
                }
            }

            return SUCCESS;
        }

        CREATE_FUNC(MemSequence);

    private:
        Node* lastRunning{ nullptr };
    };

    class Priority : public Composite
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            for (auto child : children)
            {
                auto status = child->execute(blackboard);
                if (status != FAILURE) return status;
            }

            return FAILURE;
        }

        CREATE_FUNC(Priority);
    };

    class MemPriority : public Composite
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            for (auto child : children)
            {
                auto locked = lastRunning != nullptr;
                if (!locked || lastRunning == child)
                {
                    auto status = child->execute(blackboard);

                    if (status == RUNNING)
                    {
                        lastRunning = child;
                        return RUNNING;
                    }

                    lastRunning = nullptr;
                    if (status != FAILURE) return status;
                }
            }

            return FAILURE;
        }

        CREATE_FUNC(MemPriority);

    private:
        Node* lastRunning{ nullptr };
    };

    // ------------------- DECORATORS -------------------

    class Repeater : public Decorator
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            auto count = 0;
            auto maxLoop = blackboard->parameters[uuid]["maxLoop"].asInt();
            while (count < maxLoop)
            {
                auto status = child->execute(blackboard);
                if (status != SUCCESS && status != FAILURE) return status;
                ++count;
            }

            return SUCCESS;
        }

        CREATE_FUNC(Repeater);
    };

    class RepeatUntilFailure : public Decorator
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            Status status;
            int count;
            auto maxLoop = blackboard->parameters[uuid]["maxLoop"].asInt();
            do
            {
                status = child->execute(blackboard);
                if (status != SUCCESS && status != FAILURE) return status;
                ++count;
            } while (status != FAILURE && (maxLoop > 0 ? count < maxLoop : true));

            return SUCCESS;
        }

        CREATE_FUNC(RepeatUntilFailure);
    };

    class RepeatUntilSuccess : public Decorator
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            Status status;
            int count;
            auto maxLoop = blackboard->parameters[uuid]["maxLoop"].asInt();
            do
            {
                status = child->execute(blackboard);
                if (status != SUCCESS && status != FAILURE) return status;
                ++count;
            } while (status != SUCCESS && (maxLoop > 0 ? count < maxLoop : true));

            return SUCCESS;
        }

        CREATE_FUNC(RepeatUntilSuccess);
    };

    class MaxTime : public Decorator
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            if (start < 0) start = std::clock();
            auto maxTime = blackboard->parameters[uuid]["maxTime"].asInt();
            if ((std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) < maxTime)
            {
                auto status = child->execute(blackboard);
                if (status != FAILURE) return status;
            }

            start = -1;
            return FAILURE;
        }

        CREATE_FUNC(MaxTime);

    private:
        std::clock_t start{ -1 };
    };

    class Inverter : public Decorator
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            auto status = child->execute(blackboard);

            if (status == SUCCESS) return FAILURE;
            if (status == FAILURE) return SUCCESS;
            return status;
        }

        CREATE_FUNC(Inverter);
    };

    class Limiter : public Decorator
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            auto maxLoop = blackboard->parameters[uuid]["maxLoop"].asInt();
            if (count < maxLoop)
            {
                auto status = child->execute(blackboard);
                ++count;
                return status;
            }

            return FAILURE;
        }

        CREATE_FUNC(Limiter);

    private:
        int count{ 0 };
    };

    // ------------------- ACTIONS -------------------

    class Failer : public Action
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            CCLOG("Failer");
            return FAILURE;
        }

        CREATE_FUNC(Failer);
    };

    class Succeeder : public Action
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            CCLOG("Succeeder");
            return SUCCESS;
        }

        CREATE_FUNC(Succeeder);
    };

    class Runner : public Action
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            CCLOG("Runner");
            return RUNNING;
        }

        CREATE_FUNC(Runner);
    };

    class Error : public Action
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            CCLOG("Error");
            return ERROR_OCCURED;
        }

        CREATE_FUNC(Error);
    };

    class Wait : public Action
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            if (start < 0) start = std::clock();
            auto milliseconds = blackboard->parameters[uuid]["milliseconds"].asInt();
            if ((std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) < milliseconds)
            {
                CCLOG("Waiting");
                return RUNNING;
            }

            start = -1;
            return SUCCESS;
        }

        CREATE_FUNC(Wait);

    private:
        std::clock_t start{ -1 };
    };

    // ------------------- CUSTOM NODES -------------------

    class SeePlayer : public Condition
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            if ((blackboard->agent->getPosition() - blackboard->target->getPosition()).length() < 150.0f)
                return SUCCESS;
            return FAILURE;
        }

        CREATE_FUNC(SeePlayer);
    };

    class PlayerInRange : public Condition
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            if ((blackboard->target->getPosition() - blackboard->agent->getPosition()).length() < 25.0f)
                return SUCCESS;
            return FAILURE;
        }

        CREATE_FUNC(PlayerInRange);
    };

    class Wander : public Action
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            CCLOG("Wander");
            return SUCCESS;
        }

        CREATE_FUNC(Wander);
    };

    class Follow : public Action
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            auto direction = (blackboard->target->getPosition() - blackboard->agent->getPosition()).getNormalized();

            blackboard->agent->setPosition(blackboard->agent->getPosition() + direction * 5.0f);

            return SUCCESS;
        }

        CREATE_FUNC(Follow);
    };

    class Attack : public Action
    {
    public:
        virtual Status execute(Blackboard* blackboard) override
        {
            CCLOG("Attack");
            return SUCCESS;
        }

        CREATE_FUNC(Attack);
    };

    // -------------------- NODES END --------------------

/*
 * TO USE INSIDE CoconutTreeManager ONLY!
 * USE REGISTER_CUSTOM_NODE_TYPE(type) INSTEAD!
 */
#define REGISTER_NODE_TYPE(type) nodeTypeMap[#type] = []() { \
        auto node = type::create(); \
        node->retain(); \
        return node; \
    }

#define REGISTER_CUSTOM_NODE_TYPE(type) CoconutTreeManager::getInstance()->registerNodeType(#type, []() { \
        auto node = type::create(); \
        node->retain(); \
        return node; \
    })

    class CoconutTreeManager
    {
    public:
        static CoconutTreeManager* getInstance()
        {
            if (instance == nullptr)
                instance = new CoconutTreeManager();

            return instance;
        }

        Node* createNodeByTypeName(std::string typeName)
        {
            CCASSERT(nodeTypeMap.find(typeName) != nodeTypeMap.end(),
                cocos2d::String::createWithFormat(
                    "Node '%s' not defined in game!", typeName.c_str()
                )->getCString());
            return nodeTypeMap[typeName]();
        }

        void registerNodeType(std::string name, std::function<Node*()> functionReturningAPointerToNode)
        {
            nodeTypeMap[name] = functionReturningAPointerToNode;
        }

    private:
        CoconutTreeManager()
        {
            // root
            REGISTER_NODE_TYPE(Root);

            // composite
            REGISTER_NODE_TYPE(Sequence);
            REGISTER_NODE_TYPE(MemSequence);
            REGISTER_NODE_TYPE(Priority);
            REGISTER_NODE_TYPE(MemPriority);

            // decorator
            REGISTER_NODE_TYPE(Repeater);
            REGISTER_NODE_TYPE(RepeatUntilFailure);
            REGISTER_NODE_TYPE(RepeatUntilSuccess);
            REGISTER_NODE_TYPE(MaxTime);
            REGISTER_NODE_TYPE(Inverter);
            REGISTER_NODE_TYPE(Limiter);

            // actions
            REGISTER_NODE_TYPE(Failer);
            REGISTER_NODE_TYPE(Succeeder);
            REGISTER_NODE_TYPE(Runner);
            REGISTER_NODE_TYPE(Error);
            REGISTER_NODE_TYPE(Wait);

            // custom nodes
            REGISTER_NODE_TYPE(SeePlayer);
            REGISTER_NODE_TYPE(PlayerInRange);
            REGISTER_NODE_TYPE(Wander);
            REGISTER_NODE_TYPE(Follow);
            REGISTER_NODE_TYPE(Attack);
        }

        std::map<std::string, std::function<Node*()>> nodeTypeMap;
        static CoconutTreeManager* instance;

    };

    class CoconutTree : public cocos2d::Ref
    {
    public:
        virtual bool init() { return true; }
        virtual bool initWithFile(std::string fileName)
        {
            root = parse(fileName);
            if (root)
                return true;

            return false;
        }

        virtual void tick()
        {
            root->execute(blackboard);
        }

        Blackboard* getBlackboard() { return blackboard; }

        static void setRootDir(std::string dir) { rootDir = dir; }
        static void setFileExt(std::string ext) { fileExt = ext; }

        CREATE_FUNC(CoconutTree);

    private:
        Node* parse(std::string fileName);

        Node* root;
        Blackboard* blackboard;
        static std::string rootDir;
        static std::string fileExt;
    };

}

#endif // __COCONUT_TREE_H__