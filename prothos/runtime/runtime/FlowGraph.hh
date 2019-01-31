#pragma once

#include "runtime/FlowGraph_impl.hh"
#include "runtime/Visitor.hh"

namespace Prothos {
namespace FlowGraph {


class Graph {
public:
    Graph() {}
};

class GraphNode:public Visitable {
public:
    GraphNode(Graph &g)
        : g(g)
    {}

private:
    Graph &g;
};


template<typename T>
inline void makeEdge(Internal::Sender<T> &s, Internal::Receiver<T> &r) {
    s.registerSuccessor(r);
    r.registerPredecessor(s);
}

template<typename T>
inline void removeEdge(Internal::Sender<T> &s, Internal::Receiver<T> &r) {
    s.removeSuccessor(r);
    r.removePredecessor(s);
}


template<typename Input, typename Output>
class FunctionNode : public GraphNode, public Internal::FunctionInput<Input, Output>, public Internal::Sender<Output> {
public:
    template<typename Body>
    FunctionNode(Graph &g, Body body)
        : GraphNode(g)
        , Internal::FunctionInput<Input, Output>(body)
    {}

    std::vector<Internal::Receiver<Output>*> successors() override {
        return Internal::Sender<Output>::successors();
    }

    virtual GraphNode * getThisPointer() override {
        return static_cast<GraphNode *>(this);
    }

    virtual bool accept(Visitor& visitor) {
        if(visitor.beginNode(getThisPointer())) {
            visitor.setNodeType(this, EFunctionNode);
            for (Internal::Receiver<Output> * succ : successors()) {
               visitor.addSuccessor(this, succ->getThisPointer());
            }
        }
    }

};

typedef Internal::ContMsg ContinueMsg;

template <typename Output>
class ContinueNode: public GraphNode, public Internal::ContinueInput<Output>, public Internal::Sender<Output> {
public:
    template<typename Body>
    ContinueNode(Graph &g, Body body, size_t count)
        : GraphNode(g)
        , Internal::ContinueInput<Output>(body, count)
    {}

    template<typename Body>
    ContinueNode(Graph &g, Body body)
        : GraphNode(g)
        , Internal::ContinueInput<Output>(body)
    {}


    std::vector<Internal::Receiver<Output>*> successors() override {
        return Internal::Sender<Output>::successors();
    }
    
    virtual GraphNode * getThisPointer() override {
        return static_cast<GraphNode *>(this);
    }
   
    virtual bool accept(Visitor& visitor) {
        if(visitor.beginNode(getThisPointer())) {
            visitor.setNodeType(this, EContinueNode);
            for (Internal::Receiver<Output> * succ : successors()) {
               visitor.addSuccessor(this, succ->getThisPointer());
            }
        }
    }


};


template <typename Input, typename Output>
class SplitNode: public GraphNode, public Internal::SplitInput<SplitNode<Input, Output>, Input, Output>, public Internal::Sender<Output> {
public:
    template<typename Body>
    SplitNode(Graph &g, Body body)
        : GraphNode(g)
        , Internal::SplitInput<SplitNode<Input, Output>, Input, Output>(this, body)
    {}

    std::vector<Internal::Receiver<Output>*> successors() override {
        return Internal::Sender<Output>::successors();
    }

    virtual GraphNode * getThisPointer() override {
        return static_cast<GraphNode *>(this);
    }
    
    virtual bool accept(Visitor& visitor) {
        if(visitor.beginNode(getThisPointer())) {
            visitor.setNodeType(this, ESplitNode);
            for (Internal::Receiver<Output> * succ : successors()) {
               visitor.addSuccessor(this, succ->getThisPointer());
            }
        }
    }

};

template <typename OutTuple>
class JoinNode : public GraphNode, public Internal::JoinInput<OutTuple>, public Internal::Sender<OutTuple> {
public:
    JoinNode(Graph &g)
        : GraphNode(g)
    {}

    std::vector<Internal::Receiver<OutTuple>*> successors() {
        return Internal::Sender<OutTuple>::successors();
    }

    virtual GraphNode * getThisPointer() override {
        return static_cast<GraphNode *>(this);
    }
 
    virtual bool accept(Visitor& visitor) {
        if(visitor.beginNode(getThisPointer())) {
            visitor.setNodeType(this, ESplitNode);
            for (Internal::Receiver<OutTuple> * succ : successors()) {
               visitor.addSuccessor(this, succ->getThisPointer());
            }
        }
    }

};

template<typename Output>
class SourceNode : public GraphNode, public Internal::Sender<Output> {
public:
    typedef Internal::SourceBody<Output&, bool> SourceBodyType;

    template<typename Body>
    SourceNode(Graph &g, Body body)
        : GraphNode(g)
        , myBody(new Internal::SourceBodyLeaf<Output&, bool, Body>(body))
    {}

    ~SourceNode() {
        delete myBody;
    }

    std::vector<Internal::Receiver<Output>*> successors() {
        return Internal::Sender<Output>::successors();
    }
    
    virtual bool accept(Visitor& visitor) {
        if(visitor.beginNode(static_cast<GraphNode *>(this))) {
            visitor.setNodeType(this, ESplitNode);
            for (Internal::Receiver<Output> * succ : successors()) {
               visitor.addSuccessor(this, succ->getThisPointer());
            }
        }
    }

    void activate() {
        new Internal::SourceTask<SourceNode, Output>(*this);
    }
 

private:
    
    bool applyBody(Output &m) {
        return (*myBody)(m);
    }

    SourceBodyType *myBody;

    template<typename NodeType, typename Out> friend class Internal::SourceTask;
};

template<typename Input, typename Output, size_t Ports>
class ConditionalNode : public GraphNode, public Internal::CondInput<Input, Output, Ports>{
public:
	template<typename Body>
	ConditionalNode(Graph &g, Body body)
		: GraphNode(g)
		, Internal::CondInput<Input, Output, Ports>(body)
	{}

    std::vector<Internal::Receiver<Output>*> successors(size_t port) override {
        ASSERT(port < Ports);
		return sender[port].successors();
    }

    virtual GraphNode * getThisPointer() override {
        return static_cast<GraphNode *>(this);
    }

    virtual bool accept(Visitor& visitor) {
        if(visitor.beginNode(getThisPointer())) {
            visitor.setNodeType(this, ESplitNode);
            for (size_t port = 0; port < Ports; port++) {   
                for (Internal::Receiver<Output> * succ : get(port).successors()) {
                    visitor.addSuccessor(this, succ->getThisPointer());
                }
            }
        }
    }

	Internal::Sender<Output>& get(size_t port){
        ASSERT(port < Ports);
		return sender[port];
	}
    
private:
	Internal::Sender<Output> sender[Ports];
};

}; // FlowGraph
}; // Prothos


