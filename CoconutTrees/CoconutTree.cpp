#include "CoconutTree.h"

USING_NS_CC;

namespace cctree
{

    CoconutTreeManager* CoconutTreeManager::instance = nullptr;
    std::string CoconutTree::rootDir = "trees/";
    std::string CoconutTree::fileExt = ".json";

    Node* CoconutTree::parse(std::string fileName)
    {
        auto data = FileUtils::getInstance()->getDataFromFile((rootDir + fileName + fileExt).c_str());

        unsigned char* buffer = data.getBytes();
        size_t size = data.getSize();

        Json::Value doc;
        Json::Reader reader;
        bool parsingSuccessful = reader.parse((const char*)buffer, (const char*)(buffer + size), doc);
        if (!parsingSuccessful)
        {
            CCLOG("Failed to parse configuration\n%s", reader.getFormattedErrorMessages().c_str());
            return nullptr;
        }

        blackboard = Blackboard::create();
        blackboard->retain();

        std::function<Node*(std::string id)> createNode;
        createNode = [this, doc, &createNode](std::string id) {
            auto typeName = doc["nodes"][id]["name"].asString();
            auto node = CoconutTreeManager::getInstance()->createNodeByTypeName(typeName);
            node->setUUID(id);
            blackboard->parameters[id] = doc["nodes"][id]["parameters"];
            blackboard->properties[id] = doc["nodes"][id]["properties"];

            CCLOG("%s", typeName.c_str());

            for (auto child : doc["nodes"][id]["children"])
            {
                node->addChild(createNode(child.asString()));
            }
            if (!doc["nodes"][id]["child"].empty())
                node->addChild(createNode(doc["nodes"][id]["child"].asString()));

            return node;
        };

        auto rootNode = new Root();

        rootNode->addChild(createNode(doc["root"].asString()));

        return rootNode;
    }
}