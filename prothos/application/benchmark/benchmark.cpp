#include "benchmark.h"
#include "timing.h"

namespace benchmark {

    std::tuple<std::string, std::string> to_key(std::string benchmark_name, std::string key) {
        return std::make_tuple(benchmark_name, key);
    }

    std::tuple<long, units> to_value(long value, benchmark::units unit) {
        return std::make_tuple(value, unit );
    }

    void benchmark_result::add_metric(benchmark_label label, value_t value) {
            bool success = values.insert(std::make_pair(label, value)).second;

            if (success) {
                std::cout << "result for <"<< std::get<0>(label) << "," << std::get<1>(label) <<"> added" << std::endl;
            } else {
                std::cout << "result for <"<< std::get<1>(label) <<"> was already present" << std::endl;
            }

    }

    std::string benchmark_result::get_string_results() {
            std::stringstream results;
            for (auto const& x : this->values)
            {
                std::string label =  std::get<0>(x.first) + "," + std::get<1>(x.first);
                std::string result = std::to_string(std::get<0>(x.second));

                auto unit = std::get<1>(x.second);
                switch (unit){
                    case units::MS: result = result + " milliseconds"; break;
                    case units::TICKS:  result = result + " ticks"; break;
                }
                results << label << ':' << result << "\n";
            }
            std::string printable_result = results.str();
            return printable_result;
    }

    namespace internal {
        benchmark_registry * benchmark_registry::Instance() {
                 if (!benchmark_registry::m_pInstance) {
                     benchmark_registry::m_pInstance = new benchmark_registry();
                 }
                 return benchmark_registry::m_pInstance;
        }

        timer_registry * timer_registry::Instance() {
                 if (!timer_registry::m_pInstance) {
                     timer_registry::m_pInstance = new timer_registry();
                 }
                 return timer_registry::m_pInstance;
       }

       timer_registry::~timer_registry(){
                 // delete all timers
                 for ( auto i = 0; i < timers.size(); i++)
                 {
                     delete timers[i];
                 }
        }

       abstract_timer::abstract_timer(benchmark_result * result, std::string benchmark_name, std::string key):
           result(result),
           benchmark_name(benchmark_name),
           key(key) {}
    }



    simple_timer::simple_timer(std::string benchmark_name, std::string key, benchmark_result * result):
        internal::abstract_timer(result, benchmark_name, key),
        start_time(0) {}

    simple_timer::~simple_timer() {}

    tick_timer::tick_timer(std::string benchmark_name, std::string key, benchmark_result * result):
        internal::abstract_timer(result, benchmark_name, key),
        start_time(0) {}

    tick_timer::~tick_timer() {}

    void simple_timer::start() {
         start_time = benchmark::get_time();
    }

    void simple_timer::stop() {
             long stop_time = benchmark::get_time();
             ADD_MS_METRIC(benchmark_name, key, (stop_time - start_time))
    }

    void tick_timer::start() {
         start_time = benchmark::get_clock();
    }

    void tick_timer::stop() {
             long stop_time = benchmark::get_clock();
             ADD_TICK_METRIC(benchmark_name, key, (stop_time - start_time))
    }

    void register_benchmark(BENCHMARK_SIGNATURE, base_configuration * config){
        internal::benchmark_registry * reg = internal::benchmark_registry::Instance();
        reg->benchmarks.push_back(std::make_tuple(pt2Func, config));
    }

    internal::abstract_timer * register_timer(std::string benchmark_name, std::string key, benchmark_result * result, benchmark::units unit) {
        internal::abstract_timer * timer;
        switch (unit) {
            case MS: timer = new simple_timer(benchmark_name, key, result); break;
            case TICKS: timer = new tick_timer(benchmark_name, key, result); break;
            default: break;
        }
        return timer;
   }

    benchmark_result * run_all_benchmarks() {
         internal::benchmark_registry * reg = internal::benchmark_registry::Instance();
         benchmark_result * result = new benchmark_result;
         for(std::vector<std::tuple<pfunc_t, base_configuration*> >::iterator it = reg->benchmarks.begin(); it != reg->benchmarks.end(); ++it) {
             auto benchmark_tuple = *it;
             auto bm_conf = std::get<1>(benchmark_tuple);
             auto bm_fp = std::get<0>(benchmark_tuple);
             bm_fp(result, bm_conf);
         }
         return result;
    }

}

benchmark::internal::benchmark_registry * benchmark::internal::benchmark_registry::m_pInstance = NULL;
benchmark::internal::timer_registry * benchmark::internal::timer_registry::m_pInstance = NULL;


