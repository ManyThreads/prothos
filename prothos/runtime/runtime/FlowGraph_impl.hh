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

// sender / receiver

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
		    n->pushPromise(myOutput);
			/*FlowGraphTask *t =*/ 
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



} //FlowGraph
} //Prothos
