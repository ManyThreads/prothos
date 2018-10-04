#pragma once

#include "runtime/FlowGraph_impl.hh"

namespace Prothos {
namespace FlowGraph {


class Graph {
public:
    Graph() {}
};

class GraphNode {
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

} // FlowGraph
} // Prothos


