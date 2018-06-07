#pragma once

#include "Task.hh"
#include "LocalScheduler.hh"

#include <vector>
#include <algorithm>

namespace Prothos{
namespace FlowGraph{

// Since AnyDSL does not need typed messages use a generic message type as placeholder
class GenericMsg{};

template<typename NodeType, typename Input, typename Output>
class ApplyBodyTask : public Task{
public:
	ApplyBodyTask(NodeType &n, const Input &i)
		: Task(TaskState::SuccessorsUnknown)
		, myNode(n)
		, myInput(i)
	{}

	virtual void execute() override{
	myOutput = myNode.applyBody(myInput);
	LocalScheduler::getLocalScheduler().taskDone(this);
	};

	Output *getOutput(){
		return &myOutput;
	};

private:
	NodeType &myNode;
	const Input &myInput;
	Output myOutput;
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
		virtual Task *putTask(const Input &t) = 0;
};

template<typename T>
class BroadcastCache{
	public:
		void registerSuccessor(Receiver<T> *r){
			successors.push_back(r);
		}

		void removeSuccessor(Receiver<T> *r) {
			// erase-remove-idiom
			successors.erase(std::remove(successors.begin(), successors.end(), r), successors.end());
		}

		void tryPut(const T &t) {
			for(auto const& s: successors) {
				s->putTask(t);
			}
		}

	private:
		std::vector<Receiver<T>*> successors;
};


template<typename Output>
class Sender{
	public:
		BroadcastCache<Output> &successors(){
			return mySuccessors;
		}
		void registerSuccessor(Receiver<Output> &r) {
			mySuccessors.registerSuccessor(&r);
		}

		void removeSuccessor(Receiver<Output> &r) {
			mySuccessors.removeSuccessor(&r);
		}

	private:
		 BroadcastCache<Output> mySuccessors;
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

		Task *putTask(const Input &i) override {
			return new ApplyBodyTask<FunctionInput<Input, Output>, Input, Output>(*this, i);
		}

		~FunctionInput(){
			delete myBody;
		}
	private:
		FunctionBodyType *myBody;
	
};

class FunctionNode : public FunctionInput<GenericMsg, GenericMsg>, public Sender<GenericMsg>{
	public:
		template<typename Body>
		FunctionNode(Body body)
			: FunctionInput<GenericMsg, GenericMsg>(body)
		{}
};

template<typename T>
inline void makeEdge(Sender<T> &s, Receiver<T> &r) {
    s.registerSuccessor(r);
}

template<typename T>
inline void removeEdge(Sender<T> &s, Receiver<T> &r) {
    s.removeSuccessor(r);
}

} //FlowGraph
} //Prothos
