#include <iostream>
#include "benchmark.h"
#include <unistd.h>

using namespace benchmark;

    /**
     * derive from base configuration
     */

    struct bm1_config: public benchmark_configuration {};

    struct bm2_config: public benchmark_configuration {
        long parameter1;
        char* parameter2;
    };


    /**
     * define benchmarks
     */


    void demobenchmark1(benchmark_result * result, benchmark_configuration * conf) {
        auto config = MYCONFIG(bm1_config)
        std::cout << config->benchmark_name << " running" << std::endl;
        std::cout << config->repeats << " repeats" << std::endl;
        long count = 1;
        std::string pbm_key = "";

        /**
          * manual value entering
          */
        pbm_key = "partial benchmark one";
        ADD_MS_METRIC(config->benchmark_name, pbm_key, 1234566, count)

        /**
          * millisecond timer creation
          */
        pbm_key = "partial benchmark two";
        auto timer1 = REGISTER_TICK_TIMER(config->benchmark_name, pbm_key, result)

        timer1->start();
        std::cout << pbm_key << std::endl;
        for (auto i = 0; i < 100; i++) {
            usleep(1000);
        }
        timer1->stop(count);

        /**
          * clock tick timer creation
          */
        pbm_key = "partial benchmark three";
        auto timer2 = REGISTER_TICK_TIMER(config->benchmark_name, pbm_key, result)

        timer2->start();
        std::cout << pbm_key << std::endl;
        for (auto i = 0; i < 100; i++) {
            usleep(100);
        }
        timer2->stop(count);

        std::cout << config->benchmark_name << " done" << std::endl;
    }

    void demobenchmark2(benchmark_result * result, benchmark_configuration * conf) {

        auto config = MYCONFIG(bm2_config)
        std::cout << config->benchmark_name << " running" << std::endl;
        std::cout << config->repeats << " repeats" << std::endl;
        std::cout << "paramter 1 is " << config->parameter1 << std::endl;
        long count = 1;
        ADD_MS_METRIC(config->benchmark_name, "another partial benchmark", 12345, count)
        ADD_TICK_METRIC(config->benchmark_name, "last partial benchmark", 54321, count)

        std::cout << config->benchmark_name << " done" << std::endl;
    }


    int main()
    {
        BENCHMARK_INIT

        /**
         * populate configuration attributes
         */

        bm1_config conf1;

        conf1.benchmark_name = "demo benchmark 1";
        conf1.repeats = 5;

        bm2_config conf2;

        conf2.benchmark_name = "demo benchmark 2";
        conf2.repeats = 5;
        conf2.parameter1 = 1000;
        conf2.parameter2 = "hello world";


        /**
          * register benchmarks
          */

        REGISTER_BENCHMARK(&demobenchmark1, &conf1)
        REGISTER_BENCHMARK(&demobenchmark2, &conf2)


        /**
         * retrieve results
         */

        RESULT_STORAGE result = RUN_ALL_BENCHMARKS

        std::cout << result->get_string_results() << std::endl;

        BENCHMARK_DESTROY

        return 0;
    }

