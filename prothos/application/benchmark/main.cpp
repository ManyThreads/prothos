#include <iostream>
#include "benchmark.h"
#include <unistd.h>


using namespace benchmark;

    /**
     * derive from base configuration
     */

    struct bm1_config: public base_configuration {
    };

    struct bm2_config: public base_configuration {
        long parameter1;
        char* parameter2;
    };


    /**
     * define benchmarks
     */


    void demobenchmark1(benchmark_result * result, base_configuration * conf) {
        auto config = MYCONFIG(bm1_config)
        std::cout << config->benchmark_name << " running" << std::endl;
        std::cout << config->repeats << " repeats" << std::endl;

        std::string pbm_key = "";

        /**
          * manual value entering
          */
        pbm_key = "partial benchmark one";
        ADD_MS_METRIC(config->benchmark_name, pbm_key, 1234566)

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
        timer1->stop();

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
        timer2->stop();

        std::cout << config->benchmark_name << " done" << std::endl;
    }

    void demobenchmark2(benchmark_result * result, base_configuration * conf) {

        auto config = MYCONFIG(bm2_config)
        std::cout << config->benchmark_name << " running" << std::endl;
        std::cout << config->repeats << " repeats" << std::endl;
        std::cout << "paramter 1 is " << config->parameter1 << std::endl;

        ADD_MS_METRIC(config->benchmark_name, "another partial benchmark", 12345)
        ADD_TICK_METRIC(config->benchmark_name, "last partial benchmark", 54321)

        std::cout << config->benchmark_name << " done" << std::endl;
    }


    int main()
    {

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

        return 0;
    }

