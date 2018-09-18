#pragma once

#include "runtime/DAG.hh"
#include "runtime/Worker.hh"
#include "utils/FifoQueue.hh"
#include "runtime/Mutex.hh"
#include "util/assert.hh"

#include <vector>
#include <algorithm>
#include <functional>

namespace Prothos{
namespace FlowGraph{

static const int MaxDecodingDepth = 5; // (-1) for infinite unrolling

class Graph{
	public: 
		Graph(){}
		//void incOpenTask(int num = 1){
			//openTasks += num;
		//}

		//void decOpenTasks(int num = 1){
			//openTasks -= num;
		//}
		
		//void waitForAll(){
			//while(openTasks != 0){
				//Prothos::LocalWorker::getInstance()->runWithoutBlocking();
				
			//}
		//}

	//private:
			
		//int openTasks;
};

class GraphNode{
	public:
		GraphNode(Graph &g)
			: g(g)
		{}

		
	private:
		Graph &g;
};

class FlowGraphTask : public DagTask{
	public:
		FlowGraphTask(int deps)
			: DagTask(deps)
			, expanded(false)
		{
		}

		virtual void expand(int depth) = 0;
		
		void body(){
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

template<typename T> 
class Future;

template<typename T>
class Promise{
	public:
		Promise(FlowGraphTask& myTask)
			: refCount(0)
			, myTask(myTask)
	{}
		void write(T val){
			this->val = val;
		}

		void reserve(){
			refCount++;
		}

		void registerFuture(Future<T> &f){
			myTask.addSucc(&f.getTask());
		}

		T& getVal(){
			return val;
		}

		void release(){
			refCount--;
			//if(refCount == 0)
				//delete &myTask;
		};

	private:
		T val;
		size_t refCount;
		FlowGraphTask& myTask;
};

template<typename T>
class Future{
	public:
		Future()
			: prom(nullptr)
			, myTask(nullptr)
		{}

		Future(Promise<T> *promise, FlowGraphTask* myTask)
			: prom(promise)
			, myTask(myTask)
		{
			ASSERT(prom);
			ASSERT(myTask);
			prom->reserve();
		}

		void setProm(Promise<T> *promise){
			ASSERT(promise);
			prom = promise;
		}

		void setTask(FlowGraphTask *task){
			ASSERT(task);
			myTask = task;
		}

		T& getVal(){
			ASSERT(prom);
			return prom->getVal();
		}

		FlowGraphTask& getTask(){
			ASSERT(myTask);
			return *myTask;
		}

		void release(){
			ASSERT(prom);
			prom->release();
			prom = nullptr;
		}
		
	private:
		Promise<T>* prom; //use pointer, so prom can be deleted
		FlowGraphTask* myTask;
		
};

template<typename T>
class DummyInputTask : public FlowGraphTask{
	public:
		DummyInputTask(const T &t)
		: FlowGraphTask(0)
        , prom(*this)
		{
			prom.write(t);
		}

		void bodyFunc() override {};
		void expand(int depth) override {};

		Promise<T> prom;
};

// Since AnyDSL does not need typed messages use a generic message type as placeholder
class GenericMsg{
	public:
		void* ptr;
};

template<typename NodeType, typename Input, typename Output>
class ApplyBodyTask : public FlowGraphTask{
public:
	ApplyBodyTask(NodeType &n, Promise<Input> &p)
		: FlowGraphTask(1)
		, myNode(n)
		, myInput(&p, this)
		, myOutput(*this)
	{
		p.registerFuture(myInput);
	}

	void bodyFunc() override{
		myOutput.write(myNode.applyBody(myInput.getVal()));
		myInput.release();
	};

	void expand(int depth) override{
		for(auto n : myNode.successors()){
			ASSERT(n);
			/*FlowGraphTask *t =*/ n->pushPromise(myOutput);
			//if(t != nullptr && depth != 0){
				//t->expand(depth > 0 ? depth - 1 : depth);
			//}
		}
		expanded = true;	
	}

	Output *getOutput(){
		return &myOutput;
	};

private:
	NodeType &myNode;
	Future<Input> myInput;
	Promise<Output> myOutput;
};

template<typename Input, typename Output>
class FunctionBody{
	public:
		virtual ~FunctionBody(){};
		virtual Output operator()(const Input &i) = 0;
};

template<typename Input, typename Output, typename Body>
class FunctionBodyLeaf : public FunctionBody<Input, Output> {
	public:
		FunctionBodyLeaf( const Body body)
			: myBody(body)
		{}

		Output operator() (const Input &i){
			return myBody(i);
		}

	private:
		Body myBody;
};

template<typename Output>
class Sender;

template<typename Input>
class Receiver{
	public:
		virtual FlowGraphTask *pushPromise(Promise<Input> &p) = 0;
		virtual void registerPredecessor(Sender<Input>& s){};
		virtual void removePredecessor(Sender<Input>& s){};

		FlowGraphTask* pushValue(const Input &i){
			DummyInputTask<Input> *t = new DummyInputTask<Input>(i);
			return pushPromise(t->prom);
		}
};

template<typename Output>
class Sender{
	public:
		std::vector<Receiver<Output>* > successors(){
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
	
	friend class SourceNode;
};

template<typename Input, typename Output>
class FunctionInput : public Receiver<Input>{
	public:
		typedef FunctionBody<Input, Output> FunctionBodyType;

		template<typename Body>
		FunctionInput(Body &body)
			: myBody(new FunctionBodyLeaf<Input, Output, Body>(body))
		{}

		 Output applyBody( const Input &i){
			return (*myBody)(i); 
		}	

		FlowGraphTask *pushPromise(Promise<Input> &p) override {
			return new ApplyBodyTask<FunctionInput<Input, Output>, Input, Output>(*this, p);
		}

		~FunctionInput(){
			delete myBody;
		}

		virtual std::vector<Receiver<Output>*> successors() = 0;
	private:
		FunctionBodyType *myBody;
	
};

class FunctionNode : public GraphNode, public FunctionInput<GenericMsg, GenericMsg>, public Sender<GenericMsg>{
	public:
		template<typename Body>
		FunctionNode(Graph &g, Body body)
			: GraphNode(g)
			, FunctionInput<GenericMsg, GenericMsg>(body)
		{}
		
		std::vector<Receiver<GenericMsg>*> successors(){
			return Sender<GenericMsg>::successors();
		}
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

class Handler{
	public:
		virtual void handle() = 0;
};

template <typename Input>
class QueueingInputPort : public Receiver<Input>{
	public:
		QueueingInputPort() 
			: handler(nullptr)
		{
		}

		void setHandler(Handler *h){
			handler = h;
		}

		FlowGraphTask *pushPromise(Promise<Input> &p) override {
			p.reserve();
			prom.push(&p);
			ASSERT(handler);
			handler->handle();
			return nullptr;
		}
		
		bool hasPromise(){ 
			return !prom.empty(); 
		}

		Promise<Input> *getPromise(){ 
			Promise<Input> *ret = prom.pop();
			ASSERT(ret);
			return ret; 
		}

	private:
		Handler *handler;
		FifoQueue<Promise<Input>*> prom;
};

template<typename T>
struct JoinMsg : public GenericMsg{
	std::vector<T> elements;
};

template<typename NodeType, typename Input, typename Output, size_t NumPorts>
class JoinTask : public FlowGraphTask{
public:
	JoinTask(NodeType &n, std::array<Promise<Input>*, NumPorts> *p)
		: FlowGraphTask(NumPorts)
		, myNode(n)
		, myOutput(*this)
	{
		for(size_t i = 0; i < NumPorts; i++){
			myInput[i].setTask(this);
			myInput[i].setProm((*p)[i]);
			(*p)[i]->registerFuture(myInput[i]);
			(*p)[i]->release();
		}
		delete p;
	}

	void bodyFunc() override{
		JoinMsg<Input> msg;
		for(auto &i : myInput){
			msg.elements.push_back(i.getVal());
			i.release();
		}
		myOutput.write(msg);
	};

	void expand(int depth) override{
		for(auto n : myNode.successors()){
			FlowGraphTask *t = n->pushPromise(myOutput);
			if(t != nullptr && depth != 0){
				t->expand(depth > 0 ? depth - 1 : depth);
			}
		}	
	}

	Output *getOutput(){
		return &myOutput;
	};

private:
	NodeType &myNode;
	Promise<Output> myOutput;
	std::array<Future<Input>, NumPorts> myInput;
};

template <typename Input, typename Output, size_t NumPorts>
class JoinInput : public Handler{
	public:
		JoinInput()
		{
			for(auto &i : inPorts){
				i.setHandler(this);
			}	
		};

		Receiver<Input> &getInPort(size_t portNum){ 
			return inPorts[portNum]; 
		}
		
		void handle() override {
			tryJoinTask();
		}

		virtual std::vector<Receiver<GenericMsg>*> successors() = 0;
	private:
		void tryJoinTask() {
			mythos::Mutex::Lock guard(mutex);
			for(auto &i : inPorts){
				if(!i.hasPromise())
					return;
			}
			std::array<Promise<Input>*, NumPorts> *pred = new std::array<Promise<Input>*, NumPorts>;
			for(size_t i = 0; i < NumPorts; i++){
				(*pred)[i] = inPorts[i].getPromise();	
				ASSERT((*pred)[i] != nullptr);
			}
			new JoinTask<JoinInput<Input, Output, NumPorts>, Input, Output, NumPorts>(*this, pred);
		}

		std::array<QueueingInputPort<Input>, NumPorts> inPorts;
		mythos::Mutex mutex;
};

template <size_t NumPorts>
class JoinNode : public GraphNode, public JoinInput<GenericMsg, GenericMsg, NumPorts>, public Sender<GenericMsg>{
	public:
		JoinNode(Graph &g)
			: GraphNode(g)
		{}

		std::vector<Receiver<GenericMsg>*> successors(){
			return Sender<GenericMsg>::successors();
		}	

};

template<typename Input, typename Output>
class SourceBody{
	public:
		virtual ~SourceBody(){};
		virtual Output operator()(Input &i) = 0;
};

template<typename Input, typename Output, typename Body>
class SourceBodyLeaf : public SourceBody<Input, Output> {
	public:
		SourceBodyLeaf( Body body)
			: myBody(body)
		{}

		Output operator() (Input &i){
			return myBody(i);
		}

	private:
		Body myBody;
};

template<typename NodeType, typename Output>
class SourceTask : public FlowGraphTask{
public:
	SourceTask(NodeType &n)
		: FlowGraphTask(0)
		, myNode(n)
	{
	}

	void bodyFunc() override{
		Output o;
		while(myNode.applyBody(o)){
			Promise<Output> *p = new Promise<Output>(*this);
			p->write(o);
			for(auto n : myNode.successors()){
				/*FlowGraphTask *t = */n->pushPromise(*p);
				//if(t != nullptr && depth != 0){
					//t->expand(depth > 0 ? depth - 1 : depth);
				//}
			}

		}
	};

	void expand(int depth) override{
		expanded = true;	
	}

private:
	NodeType &myNode;
};

class SourceNode : public GraphNode, public Sender<GenericMsg>{
	public:
		typedef SourceBody<GenericMsg&, bool> SourceBodyType;

		template<typename Body>
		SourceNode(Graph &g, Body body)
			: GraphNode(g)
			, myBody(new SourceBodyLeaf<GenericMsg&, bool, Body>(body))
		{}

		~SourceNode(){
			delete myBody;
		}

		std::vector<Receiver<GenericMsg>*> successors(){
			return Sender<GenericMsg>::successors();
		}

		void activate(){
			new SourceTask<SourceNode, GenericMsg>(*this);
		}	

	private:
		bool applyBody(GenericMsg &m){
			return (*myBody)(m);	
		}

		SourceBodyType *myBody;

	template<typename NodeType, typename Output> friend class SourceTask;
};

//template<typename NodeType, typename Output, size_t NumPorts>
//class ContinueTask : public FlowGraphTask{
//public:
	//ContinueTask(NodeType &n, std::array<Promise<GenericMsg>*, NumPorts> *p)
		//: FlowGraphTask(NumPorts)
		//, myNode(n)
		//, myOutput(*this)
	//{
		//for(size_t i = 0; i < NumPorts; i++){
			//myInput[i].setTask(this);
			//myInput[i].setProm((*p)[i]);
			//(*p)[i]->registerFuture(myInput[i]);
			//(*p)[i]->release();
		//}
		//delete p;
	//}

	//void bodyFunc() override{
		//myOutput.write(msg);
	//};

	//void expand(int depth) override{
		//for(auto n : myNode.successors()){
			//FlowGraphTask *t = n->pushPromise(myOutput);
			//if(t != nullptr && depth != 0){
				//t->expand(depth > 0 ? depth - 1 : depth);
			//}
		//}	
	//}

	//Output *getOutput(){
		//return &myOutput;
	//};

//private:
	//NodeType &myNode;
	//Promise<Output> myOutput;
	//std::array<Future<Input>, NumPorts> myInput;
//};
//template<typename Output>
//class ContinueInput : public Receiver<GenericMsg>{
	//public:
		//typedef FunctionBody<void, Output> FunctionBodyType;

		//ContinueInput(size_t num)
			//: myBody(new FunctionBodyLeaf<void, Output, Body>(body))
		//{}

		//~ContinueInput(){
			//delete myBody;
		//}

		//FlowGraphTask *pushPromise(Promise<GenericMsg> &p) override {
			//return new ApplyBodyTask<FunctionInput<void, Output>, Output>(*this, p);
		//}
	//protected:
		//void registerPredecessor(Sender<Input>& s) override {
			////inc num
		//}

		//void removePredecessor(Sender<Input>& s)override {
			//// dec num
		//}

	//private:
		//FunctionBodyType *myBody;
		

//};

//class ContinueNode : public GraphNode, public ContinueInput, public Sender<GenericMsg>{
	//public:
		//template<typename Body>
		//ContinueNode(Graph &g, Body body, size_t num = 0)
			//: GraphNode(g)
			//, ContinueNode(body, num)
		//{
		
		//}

		//std::vector<Receiver<GenericMsg>*> successors(){
			//return Sender<GenericMsg>::successors();
		//}


//};

} //FlowGraph
} //Prothos
