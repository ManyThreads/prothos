#pragma once

#include "runtime/DAG.hh"
#include "runtime/Worker.hh"
#include "utils/FifoQueue.hh"
#include "runtime/Mutex.hh"
#include "util/assert.hh"

#include <vector>
#include <algorithm>
#include <functional>
#include <tuple>

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

		std::atomic<Promise<T>*> next;
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

//// Since AnyDSL does not need typed messages use a generic message type as placeholder
//class GenericMsg{
	//public:
		//void* ptr;
//};

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
	
	template<typename Out> friend class SourceNode;
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

template<typename Input, typename Output>
class FunctionNode : public GraphNode, public FunctionInput<Input, Output>, public Sender<Output>{
	public:
		template<typename Body>
		FunctionNode(Graph &g, Body body)
			: GraphNode(g)
			, FunctionInput<Input, Output>(body)
		{}
		
		std::vector<Receiver<Output>*> successors() override {
			return Sender<Output>::successors();
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

template <typename OutTuple>
class JoinInput;

template <typename Input, class JNode>
class QueueingInputPort : public Receiver<Input>{
	public:
		QueueingInputPort(JNode &node) 
			: myNode(node)
		{
		}

		FlowGraphTask *pushPromise(Promise<Input> &p) override {
			prom.push(&p);
			myNode.tryJoinTask();
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
		JNode &myNode;
		FifoQueue<Promise<Input>> prom;
};


template<typename OutTuple, size_t Num, typename JNode>
struct PortPack;

template<typename OutTuple, size_t Num, typename JNode>
struct PortPack{
		PortPack(JNode& i)
			: input(i)
			, next(i)
		{}

		bool hasElements(){
			return input.hasPromise() ? next.hasElements() : false;
		}

		//QueueingInputPort* get() override { return &input }
		//AbstractPortPack* getNext() override { return &next; }

		QueueingInputPort<typename std::tuple_element<Num, OutTuple>::type, JNode> input;
		PortPack<OutTuple, Num - 1, JNode> next;
};

template<typename OutTuple, typename JNode>
struct PortPack<OutTuple, 0, JNode> {
		PortPack(JNode& i)
			: input(i)
		{}

		bool hasElements(){
			return input.hasPromise(); 
		}

		//QueueingInputPort* get() override { return &input }
		//AbstractPortPack* getNext() override { return nullptr; }

		QueueingInputPort<typename std::tuple_element<0, OutTuple>::type, JNode> input;
};
template<typename OutTuple, size_t Num, typename JNode>
struct FuturePack;

template<typename OutTuple, size_t Num, typename JNode>
struct FuturePack{
	FuturePack(PortPack<OutTuple, Num, JNode>& p)
		: prom(p.input.getPromise())
		, next(p.next)
	{
	}

	void setTask(FlowGraphTask* t){
		f.setTask(t);
		f.setProm(prom);
		prom->registerFuture(f);
		next.setTask(t);
	}

	Promise<typename std::tuple_element<Num, OutTuple>::type >* prom;
	Future<typename std::tuple_element<Num, OutTuple>::type > f;
	FuturePack<OutTuple, Num - 1, JNode> next;
};

template<typename OutTuple, typename JNode>
struct FuturePack<OutTuple, 0, JNode>{
	FuturePack(PortPack<OutTuple, 0, JNode>& p)
		: prom(p.input.getPromise())
	{
	}

	void setTask(FlowGraphTask* t){
		f.setTask(t);
		f.setProm(prom);
		prom->registerFuture(f);
	}

	Promise<typename std::tuple_element<0, OutTuple>::type >* prom;
	Future<typename std::tuple_element<0, OutTuple>::type> f;
};

template<class NodeType, typename OutTuple>
class JoinTask : public FlowGraphTask{
public:
	typedef FuturePack<OutTuple, std::tuple_size<OutTuple>::value - 1, JoinInput<OutTuple> > FutPack; 

	JoinTask(NodeType &n, FutPack* f)
		: FlowGraphTask(std::tuple_size<OutTuple>::value)
		, myNode(n)
		, myInput(f)
		, myOutput(*this)
	{
		f->setTask(this);
	}

	~JoinTask(){
		delete myInput;
	}

	void bodyFunc() override{
		OutTuple t;
		FutureToTuple<OutTuple, NodeType, std::tuple_size<OutTuple>::value - 1> ftt(*myInput, t);
		myOutput.write(t);
	}

	void expand(int depth) override{
		for(auto n : myNode.successors()){
			/*FlowGraphTask *t =*/ n->pushPromise(myOutput);
			//if(t != nullptr && depth != 0){
				//t->expand(depth > 0 ? depth - 1 : depth);
			//}
		}	
	}

	Promise<OutTuple>* getOutput(){
		return &myOutput;
	};

private:
	template<typename Tuple, typename Node, size_t Num>
	struct FutureToTuple{
		FutureToTuple(FuturePack<Tuple, Num, Node>& fp, Tuple& ot)
			: next(fp.next, ot)
		{
			std::get<Num>(ot) = fp.f.getVal();
			fp.f.release();
		}

		FutureToTuple<Tuple, Node, Num - 1> next;
	}; 

	template<typename Tuple, typename Node>
	struct FutureToTuple<Tuple, Node, 0>{
		FutureToTuple(FuturePack<Tuple, 0, Node>& fp, Tuple& ot)
		{
			std::get<0>(ot) = fp.f.getVal();
			fp.f.release();
		}

	}; 

	NodeType &myNode;
	Promise<OutTuple> myOutput;
	FutPack* myInput;
};

template<typename OutTuple, size_t Num>
struct PortStruct{
	PortStruct(PortPack<OutTuple, Num, JoinInput<OutTuple> > &p)
		: next(p.next)
		, pp(p)
	{}

	void* getPort(size_t port){
		return Num == port ? static_cast<void*>(&pp.input) : next.getPort(port - 1);
	}

	PortStruct<OutTuple, Num - 1> next;
	PortPack<OutTuple, Num, JoinInput<OutTuple> > &pp;
};

template<typename OutTuple>
struct PortStruct<OutTuple, 0>{
	PortStruct(PortPack<OutTuple, 0, JoinInput<OutTuple> > &p)
		: pp(p)
	{}

	void* getPort(size_t port){
		return static_cast<void*>(&pp.input);
	}

	PortPack<OutTuple, 0, JoinInput<OutTuple> > &pp;
};

template <typename OutTuple>
class JoinInput {
	public:
		JoinInput()
			: inPorts(*this)
		{
		};

		template<size_t Port>
		Receiver<typename std::tuple_element<Port, OutTuple>::type > &getInPort(){ 
			PortStruct<OutTuple, std::tuple_size<OutTuple>::value - 1> ps(inPorts); 
			void* ret = ps.getPort(Port);
			return *static_cast<Receiver<typename std::tuple_element<Port, OutTuple>::type >* >(ret); 
		}
		
		virtual std::vector<Receiver<OutTuple>* > successors() = 0;

		void tryJoinTask() {
			mythos::Mutex::Lock guard(mutex);

			if(inPorts.hasElements()){
			
				new JoinTask<JoinInput<OutTuple>, OutTuple >(*this, new FuturePack<OutTuple, std::tuple_size<OutTuple>::value - 1, JoinInput<OutTuple> >(inPorts));
			}
		}
	private:

		PortPack<OutTuple, std::tuple_size<OutTuple>::value - 1, JoinInput<OutTuple>> inPorts;
		mythos::Mutex mutex;
};

template <typename OutTuple>
class JoinNode : public GraphNode, public JoinInput<OutTuple>, public Sender<OutTuple>{
	public:
		JoinNode(Graph &g)
			: GraphNode(g)
		{}

		std::vector<Receiver<OutTuple>*> successors(){
			return Sender<OutTuple>::successors();
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

template<typename Output>
class SourceNode : public GraphNode, public Sender<Output>{
	public:
		typedef SourceBody<Output&, bool> SourceBodyType;

		template<typename Body>
		SourceNode(Graph &g, Body body)
			: GraphNode(g)
			, myBody(new SourceBodyLeaf<Output&, bool, Body>(body))
		{}

		~SourceNode(){
			delete myBody;
		}

		std::vector<Receiver<Output>*> successors(){
			return Sender<Output>::successors();
		}

		void activate(){
			new SourceTask<SourceNode, Output>(*this);
		}	

	private:
		bool applyBody(Output &m){
			return (*myBody)(m);	
		}

		SourceBodyType *myBody;

	template<typename NodeType, typename Out> friend class SourceTask;
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
