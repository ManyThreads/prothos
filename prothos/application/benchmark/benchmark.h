#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include "timing.h"

#define BENCHMARK_INIT internal::benchmark_registry::Init(); internal::timer_registry::Init();
#define BENCHMARK_DESTROY internal::benchmark_registry::Destroy(); internal::timer_registry::Destroy();

#define REGISTER_TIMER(x, y, z)  benchmark::register_timer(x, y, z, benchmark::units::MS);
#define REGISTER_TICK_TIMER(x, y, z)  benchmark::register_timer(x, y, z, benchmark::units::TICKS);

#define ADD_MS_METRIC(name, key, value, run) result->add_metric(to_key(name, key), to_value(value, benchmark::units::MS), run);
#define ADD_TICK_METRIC(name, key, value, run) result->add_metric(to_key(name, key), to_value(value, benchmark::units::TICKS), run);

#define REGISTER_BENCHMARK(x, conf) benchmark::register_benchmark(x, conf);
#define RUN_ALL_BENCHMARKS benchmark::run_all_benchmarks();
#define MYCONFIG(x) conf->cast<x *>(conf);
#define BENCHMARK_SIGNATURE void (*pt2Func)(benchmark_result *, benchmark_configuration * config, long run_counter)



namespace benchmark {

    enum units{
        MS,
        TICKS
    };

    /**
     * @brief benchmark_label
     * pair of benchmark name, timer key
     */
    typedef std::tuple<std::string, std::string> benchmark_label;
    typedef std::tuple<long, units> value_t;

    std::tuple<std::string, std::string> to_key(std::string benchmark_name, std::string key) {
        return std::make_tuple(benchmark_name, key);
    }

    std::tuple<long, units> to_value(long value, benchmark::units unit){
        return std::make_tuple(value, unit );
    }

    struct benchmark_configuration {
        std::string benchmark_name;
        long repeats;

        template<typename ConfigType>
        ConfigType cast(benchmark_configuration * conf) {
            return reinterpret_cast<ConfigType>(conf);
        }
    };

    /**
     * @brief The benchmark_result container for the benchmark results
     */
    class benchmark_result {
    public:
        /**
         * @brief add_metric  add benchmark metric to result set
         * @param label tuple of benchmark name and timer key
         * @param value the timer result
         * @param run_number number of benchmark run
         */
        void add_metric(benchmark_label label, value_t value, long run_number){
            bool success = values.insert(std::make_pair(
                              label, std::make_tuple(run_number, value)
                            )
                          ).second;

            if (success) {
                std::cout << "result for <"<< std::get<0>(label) << "," << std::get<1>(label) <<"> added" << std::endl;
            } else {
                std::cout << "result for <"<< std::get<1>(label) <<"> was already present" << std::endl;
            }

        }

        std::string get_string_results(){
            std::stringstream results;
            for (auto const& x : this->values)
            {
                std::string label =  std::get<0>(x.first) + "," + std::get<1>(x.first);

                // unpack results
                // auto result = x.second;
                long run_number = std::get<0>(x.second);
                std::tuple<long, units> value = std::get<1>(x.second);

                std::string result = std::to_string(std::get<0>(value));
                auto unit = std::get<1>(value);

                switch (unit){
                    case units::MS: result = result + " milliseconds"; break;
                    case units::TICKS:  result = result + " ticks"; break;
                }
                results << label << ", run number " << std::to_string(run_number) << ':' << result << "\n";
            }
            std::string printable_result = results.str();
            return printable_result;
    }

    private:
        std::map<std::tuple<std::string,std::string>, std::tuple<long, std::tuple<long, units> > > values;
    };

    typedef benchmark_result * RESULT_STORAGE;

    typedef void func_t(benchmark::benchmark_result *, benchmark_configuration * conf, long run_counter);
    typedef func_t* pfunc_t;



    namespace internal {

        /**
         * @brief The benchmark_registry class there should always only be one instance
         */
        class benchmark_registry {
        public:
            std::vector<std::tuple<pfunc_t, std::vector<benchmark_configuration*> *> > benchmarks;

            /**
             * @brief Init Initialize benchmark registry
             */
            static void Init();

            /**
             * @brief Destroy Destroy benchmark registry
             */
            static void Destroy();

            /**
             * @brief Instance the singleton method to obtain an instance
             * @return a pointer to the registry
             */
            static benchmark_registry * Instance();

        private:
            static benchmark_registry * MyInstance(benchmark_registry * pReg);
        };

        inline void benchmark_registry::Init() {
            benchmark_registry * ptr = new benchmark_registry();
            MyInstance(ptr);
        }

        inline benchmark_registry * benchmark_registry::Instance()
        {
            return MyInstance(NULL);
        }

        inline benchmark_registry * benchmark_registry::MyInstance(benchmark_registry * ptr)
        {
            static benchmark_registry* myInstance = NULL;
            if (ptr)
                myInstance = ptr;
            return myInstance;
        }

        inline void benchmark_registry::Destroy()
        {
            benchmark_registry* pReg = MyInstance(NULL);
            if (pReg)
                delete pReg;
        }

        /**
         * @brief The abstract_timer class interface for the timer implementation
         */
        class abstract_timer {
        public:
            abstract_timer(benchmark_result * result, std::string benchmark_name, std::string key):
                result(result),
                benchmark_name(benchmark_name),
                key(key) {}

            /**
             * @brief start starts the timer
             */
            virtual void start() = 0;

            /**
             * @brief stop stops the timer and pushes the value to result
             * @param run_number
             */
            virtual void stop(long run_number) = 0;

            ~abstract_timer() {}

        protected:
            benchmark_result * result;
            std::string benchmark_name;
            std::string key;
        private:

        };


        /**
         * @brief The timer_registry class
         */
        class timer_registry {
        public:
            std::vector<benchmark::internal::abstract_timer *> timers;

            static void Init();
            static void Destroy();

            /**
             * @brief Instance the singleton method to obtain an instance
             * @return a pointer to the registry
             */
            static timer_registry * Instance();

            ~timer_registry(){
                // delete all timers
                for ( auto i = 0; i < timers.size(); i++)
                {
                    delete timers[i];
                }
            }

        private:
            static timer_registry * MyInstance(timer_registry * pReg);
        };

        inline void timer_registry::Init() {
            timer_registry * ptr = new timer_registry();
            MyInstance(ptr);
        }

        inline timer_registry * timer_registry::Instance()
        {
            return MyInstance(NULL);
        }

        inline timer_registry * timer_registry::MyInstance(timer_registry * ptr)
        {
            static timer_registry* myInstance = NULL;
            if (ptr)
                myInstance = ptr;
            return myInstance;
        }

        inline void timer_registry::Destroy()
        {
            timer_registry* pReg = MyInstance(NULL);
            if (pReg)
                delete pReg;
        }


    }

    /**
     * @brief The timer class provides access to timing
     */
    class simple_timer:public internal::abstract_timer {
      public:
        simple_timer(std::string benchmark_name, std::string key, benchmark_result * result):
            internal::abstract_timer(result, benchmark_name, key),
            start_time(0) {}

        ~simple_timer() {}

        void start() override {
             start_time = benchmark::get_time();
        }

        void stop(long run_number) override {
                 long stop_time = benchmark::get_time();
                 ADD_MS_METRIC(benchmark_name, key, (stop_time - start_time), run_number)
        }

    private:
        long start_time;
    };

    /**
     * @brief The tick_timer class provides access to cpu ticks
     */
    class tick_timer:public internal::abstract_timer {
      public:
        tick_timer(std::string benchmark_name, std::string key, benchmark_result * result):
            internal::abstract_timer(result, benchmark_name, key),
            start_time(0) {}

        void start() override{
             start_time = benchmark::get_clock();
        }

        void stop(long run_number) override {
                 long stop_time = benchmark::get_clock();
                 ADD_TICK_METRIC(benchmark_name, key, (stop_time - start_time), run_number)
        }

        ~tick_timer() {}

    private:
         clock_t start_time;
    };


    /**
     * @brief register_timer creates a timer for the benchmark and registers it in the tmer registry
     * @param benchmark_name the name of the benchmark
     * @param key the unique name of the timer
     * @param result the pointer to the result set for the benchmark
     */
    internal::abstract_timer * register_timer(std::string benchmark_name, std::string key, benchmark_result * result, benchmark::units unit){
        internal::abstract_timer * timer;
        switch (unit) {
            case MS: timer = new simple_timer(benchmark_name, key, result); break;
            case TICKS: timer = new tick_timer(benchmark_name, key, result); break;
            default: break;
        }
        return timer;
   }

    /**
     * @brief register_benchmark registers a benchmark function
     */
    void register_benchmark(BENCHMARK_SIGNATURE , std::vector<benchmark_configuration *> * config){
        internal::benchmark_registry * reg = internal::benchmark_registry::Instance();
        reg->benchmarks.push_back(std::make_tuple(pt2Func, config));
    }

    /**
     * @brief run_all_benchmarks runs all registered benchmarks
     * @return result object for all registered benchmarks
     */
    benchmark_result * run_all_benchmarks(){
        internal::benchmark_registry * reg = internal::benchmark_registry::Instance();
        benchmark_result * result = new benchmark_result;
        for(std::vector<std::tuple<pfunc_t, std::vector<benchmark_configuration*> *> >::iterator it = reg->benchmarks.begin(); it != reg->benchmarks.end(); ++it) {
            auto benchmark_tuple = *it;
            auto bm_configs = std::get<1>(benchmark_tuple);
            auto bm_fp = std::get<0>(benchmark_tuple);
            long counter = 1;
            for (auto bm_iterator = bm_configs->begin(); bm_iterator != bm_configs->end(); ++bm_iterator) {
                bm_fp(result, *bm_iterator, counter);
                counter ++;
            }
        }
        return result;
   }

}



#endif // BENCHMARK_H
