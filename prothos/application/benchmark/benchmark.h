#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <map>


#define REGISTER_TIMER(x, y, z)  benchmark::register_timer(x, y, z, benchmark::units::MS);
#define REGISTER_TICK_TIMER(x, y, z)  benchmark::register_timer(x, y, z, benchmark::units::TICKS);

#define ADD_MS_METRIC(name, key, value) result->add_metric(to_key(name, key), to_value(value, benchmark::units::MS));
#define ADD_TICK_METRIC(name, key, value) result->add_metric(to_key(name, key), to_value(value, benchmark::units::TICKS));

#define REGISTER_BENCHMARK(x, conf) benchmark::register_benchmark(x, conf);
#define RUN_ALL_BENCHMARKS benchmark::run_all_benchmarks();
#define MYCONFIG(x) conf->cast<x *>(conf);
#define BENCHMARK_SIGNATURE void (*pt2Func)(benchmark_result *, base_configuration * config)

namespace benchmark {

    enum units{
        MS,
        TICKS
    };


    typedef std::tuple<std::string, std::string> benchmark_label;
    typedef std::tuple<long, units> value_t;

    std::tuple<std::string, std::string> to_key(std::string benchmark_name, std::string key);

    std::tuple<long, units> to_value(long value, benchmark::units unit);

    struct base_configuration {
        std::string benchmark_name;
        long repeats;

        template<typename ConfigType>
        ConfigType cast(base_configuration * conf) {
            return (ConfigType)(conf);
        }
    };

    /**
     * @brief The benchmark_result container for the benchmark results
     */
    class benchmark_result {
    public:
        void add_metric(benchmark_label, value_t value);

        std::string get_string_results();

    private:
        std::map<std::tuple<std::string,std::string>, std::tuple<long, units> > values;
    };

    typedef benchmark_result * RESULT_STORAGE;

    typedef void func_t(benchmark::benchmark_result *, base_configuration * conf);
    typedef func_t* pfunc_t;



    namespace internal {

        /**
         * @brief The benchmark_registry class there should alway only be one instance
         */
        class benchmark_registry {
        public:
            std::vector<std::tuple<pfunc_t, base_configuration*> > benchmarks;

            /**
             * @brief Instance the singleton method to obtain an instance
             * @return a pointer to the registry
             */
            static benchmark_registry * Instance();

            static benchmark_registry * m_pInstance;
        };


        class abstract_timer {
        public:
            abstract_timer(benchmark_result * result, std::string benchmark_name, std::string key);

            /**
             * @brief start starts the timer
             */
            virtual void start() = 0;

            /**
             * @brief stop stops the timer and pushes the value to result
             */
            virtual void stop() = 0;

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

            static timer_registry * Instance();

            ~timer_registry();
        private:
            static timer_registry * m_pInstance;
        };
    }

    /**
     * @brief The timer class provides access to timing
     */
    class simple_timer:public internal::abstract_timer {
      public:
        simple_timer(std::string benchmark_name, std::string key, benchmark_result * result);

        void start() override;
        void stop() override;
        ~simple_timer();

    private:
        long start_time;
    };

    /**
     * @brief The tick_timer class provides access to cpu ticks
     */
    class tick_timer:public internal::abstract_timer {
      public:
        tick_timer(std::string benchmark_name, std::string key, benchmark_result * result);

        void start() override;
        void stop() override;
        ~tick_timer();

    private:
         clock_t start_time;
    };


    /**
     * @brief register_timer
     * @param benchmark_name
     * @param key
     * @param result
     */
    internal::abstract_timer * register_timer(std::string benchmark_name, std::string key, benchmark_result * result, benchmark::units unit);

    /**
     * @brief register_benchmark registers a benchmark function
     */
    void register_benchmark(BENCHMARK_SIGNATURE , base_configuration * config);

    /**
     * @brief run_all_benchmarks runs all registered benchmarks
     * @return result object for all registered benchmarks
     */
    benchmark_result * run_all_benchmarks();

}

#endif // BENCHMARK_H
