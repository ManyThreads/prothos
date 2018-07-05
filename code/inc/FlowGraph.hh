#pragma once

#include "Task.hh"
#include "LocalScheduler.hh"

#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>
#include <functional>
#include <assert.h>

namespace Prothos{
namespace FlowGraph{

template<typename T> 
class Future;

template<typename T>
class Promise{
	public:
		Promise(Task& myTask)
			: refCount(0)
			, openReservations(0)
			, myTask(myTask)
	{}
		void write(T val){
			//std::lock_guard<std::mutex> lg(m);
			this->val = val;
		}

		void reserve(){
			refCount++;
			openReservations++;
		}

		void registerFuture(Future<T> &f){
			myTask.addChild(&f.getTask());
			openReservations--;
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
		//void notifySucc(){
			//std::lock_guard<std::mutex> lg(m);
			//for(auto const &s : succ){
				//s.trigger();
			//}
		//};

		T val;
		//std::vector<std::reference_wrapper<Future<T> > > succ;
		size_t refCount;
		size_t openReservations;
		Task& myTask;
		//std::mutex m;
};

template<typename T>
class Future{
	public:
		Future(Promise<T> promise, Task& myTask)
			: prom(&promise)
			, myTask(myTask)
		{
			prom->reserve();
		}

		T& getVal(){
			assert(prom);
			return prom->getVal();
		}

		Task& getTask(){
			return myTask;
		}

		void release(){
			prom->release();
			prom = nullptr;
		}
		
	private:
		Promise<T>* prom; //use pointer, so prom can be deleted
		Task& myTask;
		
};

template<typename T>
class DummyInputTask : public Task{
	public:
		DummyInputTask(const T &t)
		: Task(AllSuccessorsKnown)
        , prom(*this)
		{
			prom.write(t);
		}

		void execute() override {};
		void expand() override {};

		Promise<T> prom;
};

// Since AnyDSL does not need typed messages use a generic message type as placeholder
class GenericMsg{};

template<typename NodeType, typename Input, typename Output>
class ApplyBodyTask : public Task{
public:
	ApplyBodyTask(NodeType &n, Promise<Input> &p)
		: Task(TaskState::SuccessorsUnknown)
		, myNode(n)
		, myInput(p, *this)
		, myOutput(*this)
	{
		p.registerFuture(myInput);
	}

	void execute() override{
		myOutput.write(myNode.applyBody(myInput.getVal()));
		myInput.release();
	};

	void expand() override{
		for(auto n : myNode.successors()){
			n->pushPromise(myOutput);
		}	
		doneExpanding();
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

template<typename Input>
class Receiver{
	public:
		virtual Task *pushPromise(Promise<Input> &p) = 0;
		Task* pushValue(const Input &i){
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
			r->incPredecessors();
		}

		void removeSuccessor(Receiver<Output> &r) {
			mySuccessors.removeSuccessor(&r);
			// erase-remove-idiom
			mySuccessors.erase(std::remove(mySuccessors.begin(), mySuccessors.end(), r), mySuccessors.end());
			r->decPredecessors();
		}

	private:
		std::vector<Receiver<Output>* > mySuccessors;
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

		Task *pushPromise(Promise<Input> &p) override {
			return new ApplyBodyTask<FunctionInput<Input, Output>, Input, Output>(*this, p);
		}

		~FunctionInput(){
			delete myBody;
		}

		virtual std::vector<Receiver<Output>*> successors() = 0;
	private:
		FunctionBodyType *myBody;
	
};

class FunctionNode : public FunctionInput<GenericMsg, GenericMsg>, public Sender<GenericMsg>{
	public:
		template<typename Body>
		FunctionNode(Body body)
			: FunctionInput<GenericMsg, GenericMsg>(body)
		{}
		
		std::vector<Receiver<GenericMsg>*> successors(){
			return Sender<GenericMsg>::successors();
		}
};

template<typename T>
inline void makeEdge(Sender<T> &s, Receiver<T> &r) {
    s.registerSuccessor(r);
}

template<typename T>
inline void removeEdge(Sender<T> &s, Receiver<T> &r) {
    s.removeSuccessor(r);
}

template <typename Input, typename Output>
class InputHandler{
	public:
		virtual Output handleInput(Input &i) = 0;
};

template <typename Input>
class QueueingInputPort : public Receiver<Input>{
	public:
		QueueingInputPort(InputHandler<void, Task*> &h) 
			: handler(h)
		{}

		Task *putTask(const Task *t) override {
			predecessors.push_back(t);
			return handler.handleInput();
		}
		
		bool hasPredecessors(){ return !predecessors.empty(); }
	private:
		virtual void tryJoinTask() = 0;
		InputHandler<Input, Task*> &handler;
		std::vector<Task*> predecessors;
};

template<typename Input, typename Output, size_t numPorts>
class JoinTask : public Task{
	public:
		JoinTask();
};

template <typename Input, typename Output, size_t NumPorts>
class JoinInput : public InputHandler<void, Task*>{
	public:
		JoinInput(){};

		Receiver<Input> &getInPort(size_t portNum){ 
			assert(portNum < NumPorts);
			return inPorts[portNum]; 
		}
		
		Output applyBody( const Input &i){
			return i; 
		}	

	private:
		void tryJoinTask() override {
			for(size_t i = 0; i < NumPorts; i++){
				if(!inPorts[i].hasPredecessors())
					return nullptr;
			}
			std::vector<Task*> preTasks;
			for(size_t i = 0; i < NumPorts; i++){
				Task* t = inPorts[i].getPredecessors();	
				assert(t);
				preTasks.push_back(t);
			}
			new ApplyBodyTask<JoinInput<Input, NumPorts>, Input, Output>(*this, preTasks);
		}

		QueueingInputPort<Input> inPorts[NumPorts] = QueueingInputPort<Input>(*this);
		
};

template <size_t NumPorts>
class JoinNode : public JoinInput<GenericMsg, NumPorts>, public Sender<GenericMsg>{
};

//enum class JoinPolicy{
	//Queueing,
	//Reserving
//};

////template<typename Input>
////class InputHandler{
	////public:
		
////};

//template<typename Input, size_t NumPorts>
//class ReservingJoinInput{
	//ReservingInputPort<Input> ports[NumPorts];
//};

////template<typename Input>
////class BufferingInputPort : public Receiver<Input>{
	////public:
		////BufferingInputPort();
		////virtual Task *putTask(const Input &t) = 0;
	////private:
		////std::vector<Input>
////};

////template<typename Input, size_t NumPorts>
////class QueueingJoinInput{
	////InputPort<Input> ports[NumPorts];
////};

//template<size_t NumPorts, JoinPolicy Policy=JoinPolicy::Queueing>
//class JoinNode;

//template<size_t NumPorts>
//class JoinNode<NumPorts, JoinPolicy::Queueing> : public QueueingJoinInput<GenericMsg, NumPorts>, public Sender<GenericMsg>{
//};

//template<size_t NumPorts>
//class JoinNode<NumPorts, JoinPolicy::Reserving> : public ReservingJoinInput<GenericMsg, NumPorts>, public Sender<GenericMsg>{
//};

} //FlowGraph
} //Prothos
