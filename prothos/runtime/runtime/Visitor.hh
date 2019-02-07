#pragma once
#include "runtime/mlog.hh"
#include "runtime/FlowGraph.hh"
#include <map>
#include <vector>
#include <string>

using namespace Prothos;
using namespace FlowGraph;

class Visitor;
class Visitable;

class NodeDescription {
public: 
    NodeDescription(int key): key(key) {
    }

    void setNodeType(NodeType type) { this->nodetype = type; }
    
    void addSuccessor(Visitable * visitable) {
        this->successors.push_back(visitable);
    }
    
    int getKey() { return this->key; }

    NodeType getNodeType() { return this->nodetype; }

    std::vector<Visitable*> getSuccessors() { return this->successors; }

private:
    int key;
    NodeType nodetype;
    std::vector<Visitable*> successors;
};

class Visitable {
public:
    Visitable() {}
    
    virtual bool accept(Visitor& visitor) = 0;    
    
};

class Visitor {
public:
    Visitor(): max_node_key(0) {}

    void traverse(Visitable& rootNode) {
        visit(rootNode);
        MLOG_INFO(mlog::app, "visited node");
        while(!unvisitedNodes.empty()) {
            std::vector<Visitable*>::iterator it = unvisitedNodes.begin();
            Visitable * v = *it;
            unvisitedNodes.erase(it);
            visit(*v);
        }
    }

    void visit(Visitable& visited) {
        visited.accept(*this);
    };

    bool beginNode(Visitable * visitable) {
        // check, if node is already known
        if (this->visitedNodes.find(visitable) == this->visitedNodes.end()) {
            MLOG_INFO(mlog::app, "new node");
            // insert node with new new key
            max_node_key +=1;
            NodeDescription * nd = new NodeDescription(max_node_key);
            this->visitedNodes.insert(std::pair<Visitable*, NodeDescription *>(visitable, nd));        
            return true;    
        }
        MLOG_INFO(mlog::app, "node already visited");
        return false;

    }

    void setNodeType(Visitable* visitable, NodeType type) {
        std::map<Visitable*,NodeDescription *>::iterator it = visitedNodes.find(visitable);
        NodeDescription * nd = it->second;
        nd->setNodeType(type);
    }

    void addSuccessor(Visitable * visitable, Visitable * successor) {
        std::map<Visitable*,NodeDescription *>::iterator it = visitedNodes.find(visitable);
        NodeDescription * nd = it->second;
        nd->addSuccessor(successor);
        unvisitedNodes.push_back(successor);
    }

protected:
    std::map<Visitable*, NodeDescription *> visitedNodes;
    std::vector<Visitable *> unvisitedNodes;
    size_t max_node_key;
};



