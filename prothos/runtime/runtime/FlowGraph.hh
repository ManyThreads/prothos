#pragma once

#include "runtime/DAG.hh"

namespace Prothos {
namespace FlowGraph {

namespace Internal {
    /**
    * Forward declaration of implementation classes
    */

    template<typename Output>
    class Sender;

    template<typename T>
    class Future;

    template<typename T>
    class Promise;

    template<typename T>
    class DummyInputTask;

    template<typename Input, typename Output>
    class SourceBody;

    template<typename Input, typename Output, typename Body>
    class SourceBodyLeaf;

    template<typename NodeType, typename Output>
    class SourceTask;

    template <typename OutTuple>
    class JoinInput;

    template<typename Input, typename Output>
    class FunctionInput;
}

static const int MaxDecodingDepth = 5; // (-1) for infinite unrolling


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

class FlowGraphTask : public DagTask {
public:
    FlowGraphTask(int deps)
        : DagTask(deps)
        , expanded(false)
    {
    }

    virtual void expand(int depth) = 0;

    void body() {
        preprocessing();
        bodyFunc();
        postprocessing();
    }

protected:
    virtual void releaseParentTask() {
        //todo: garbage collection
    };

    virtual void preprocessing() {}

    virtual void bodyFunc() = 0;

    virtual void postprocessing() {
        releaseParentTask();
        if(!expanded) expand(MaxDecodingDepth);
    }

    bool expanded;
};

template<typename Output>
class Sender;

template<typename Input>
class Receiver {
public:
    virtual FlowGraphTask *pushPromise(Prothos::FlowGraph::Internal::Promise<Input> &p) = 0;
    virtual void registerPredecessor(Prothos::FlowGraph::Sender<Input>& s) {};
    virtual void removePredecessor(Prothos::FlowGraph::Sender<Input>& s) {};

    FlowGraphTask* pushValue(const Input &i) {
        Prothos::FlowGraph::Internal::DummyInputTask<Input> *t = new Prothos::FlowGraph::Internal::DummyInputTask<Input>(i);
        return pushPromise(t->prom);
    }
};

template<typename Output>
class Sender {
public:
    std::vector<Receiver<Output>* > successors() {
        return mySuccessors;
    }
    void registerSuccessor(Receiver<Output> &r) {
        mySuccessors.push_back(&r);
    }

    void removeSuccessor(Receiver<Output> &r) {
        mySuccessors.removeSuccessor(&r);
        mySuccessors.erase(find(mySuccessors.begin(), mySuccessors.end(), r), mySuccessors.end());
    }

private:
    std::vector<Receiver<Output>* > mySuccessors;

    template<typename Out> friend class SourceNode;
};

template<typename T>
inline void makeEdge(Sender<T> &s, Receiver<T> &r) {
    s.registerSuccessor(r);
    r.registerPredecessor(s);
}

template<typename T>
inline void removeEdge(Sender<T> &s, Receiver<T> &r) {
    s.removeSuccessor(r);
    r.removePredecessor(s);
}


template<typename Input, typename Output>
class FunctionNode : public GraphNode, public Prothos::FlowGraph::Internal::FunctionInput<Input, Output>, public Sender<Output> {
public:
    template<typename Body>
    FunctionNode(Graph &g, Body body)
        : GraphNode(g)
        , Prothos::FlowGraph::Internal::FunctionInput<Input, Output>(body)
    {}

    std::vector<Receiver<Output>*> successors() override {
        return Sender<Output>::successors();
    }
};

template <typename OutTuple>
class JoinNode : public GraphNode, public Prothos::FlowGraph::Internal::JoinInput<OutTuple>, public Sender<OutTuple> {
public:
    JoinNode(Graph &g)
        : GraphNode(g)
    {}

    std::vector<Receiver<OutTuple>*> successors() {
        return Sender<OutTuple>::successors();
    }

};

template<typename Output>
class SourceNode : public GraphNode, public Sender<Output> {
public:
    typedef Prothos::FlowGraph::Internal::SourceBody<Output&, bool> SourceBodyType;

    template<typename Body>
    SourceNode(Graph &g, Body body)
        : GraphNode(g)
        , myBody(new Prothos::FlowGraph::Internal::SourceBodyLeaf<Output&, bool, Body>(body))
    {}

    ~SourceNode() {
        delete myBody;
    }

    std::vector<Receiver<Output>*> successors() {
        return Sender<Output>::successors();
    }

    void activate() {
        new Prothos::FlowGraph::Internal::SourceTask<SourceNode, Output>(*this);
    }

private:
    
    bool applyBody(Output &m) {
        return (*myBody)(m);
    }

    SourceBodyType *myBody;

    template<typename NodeType, typename Out> friend class Prothos::FlowGraph::Internal::SourceTask;
};

} // FlowGraph
} // Prothos

#include "runtime/FlowGraph_impl.hh"
