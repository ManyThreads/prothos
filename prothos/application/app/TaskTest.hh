#pragma once

#include "runtime/Task.hh"

struct MandelbrotConf {
	consexpr static size_t gridWidth = 1024;
	consexpr static size_t gridHeight = 1024;
	consexpr static double maxImg = 1.0;
	consexpr static double minImg = -1.0;
	consexpr static double maxReal = 0.5;
	consexpr static double minReal = -1.5;
	consexpr static size_t iterations = 2000;
	consexpr static size_t minChunkSize = 4;
};

struct Subdomain{
	size_t x;
	size_t y;
	size_t w;
	size_t h;
};

class MandelbrotTask : public WorkstealingTask{
	public:
		MandelbrotTask(MandelbrotConf c, Subdomain s)
			: c(c)
			, s(s)
		{}

		void execute() override {
		
		}

	private:



		MandelbrotConf* c;
		Subdomain s;
};


UserTask* runMandelbrot(){
	return new UserTask([](){

		Subdomain s0 = {0, 0, conf.gridWidth/2, conf.gridHeight};
		Subdomain s1 = {conf.gridWidth/2, 0, conf.gridWidth - conf.gridWidth/2, conf.gridHeight};

		new MandelbrotTask(MandelbrotConf(), s0);
		new MandelbrotTask(MandelbrotConf(), s1);
	});
}
