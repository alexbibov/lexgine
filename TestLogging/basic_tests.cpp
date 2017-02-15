#include "stdafx.h"
#include "CppUnitTest.h"
#include "../lexgine/log.h"
#include "../lexgine/window.h"
#include "../lexgine/ring_buffer_task_queue.h"
#include "../lexgine/task_graph.h"

#pragma warning(push)
#pragma warning(disable : 4307)    // this is needed to suppress the warning caused by the static hash function

#include "../lexgine/static_hash_table.h"

#include <sstream>
#include <windows.h>
#include <utility>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


namespace TestLogging
{

class TestErrorBehavioral
{
public:

    void raiseError(std::string const& message) const
    {
        lexgine::core::misc::Log::retrieve()->out(message);
    }

};

    TEST_CLASS(BasicTests)
    {
    public:

        TEST_METHOD(TestBasicLogging)
        {
            using namespace lexgine::core::misc;
            std::stringstream test_log;
            {
                Log const& logger = Log::create(test_log, 2, true);

                auto ref_time = DateTime::now(2, true);
                TestErrorBehavioral test_error_behavioral;

                LEXGINE_ERROR_LOG(
                    &test_error_behavioral,
                    (2 * 2),
                    2, 3, 5, 19, 6, 7, 8, 10, 0
                );

                auto log_time = logger.getLastEntryTimeStamp();

                Assert::IsTrue(ref_time.timeSince(log_time).seconds() < 1e-3);

                std::string month_name[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
                std::string ref_string = std::string{ "/" } +std::to_string(log_time.day()) + " " + month_name[log_time.month() - 1] + " "
                    + std::to_string(log_time.year()) + "/ (" + std::to_string(log_time.hour()) + ":" + std::to_string(log_time.minute()) + ":" + std::to_string(log_time.second()) + "): ";

                std::string log_string = test_log.rdbuf()->str();
                log_string = log_string.substr(log_string.find_first_of('\n') + 1, ref_string.size());


                Assert::IsTrue(log_string.compare(ref_string) == 0);
            }

            Log::shutdown();
        }


        TEST_METHOD(TestStaticHashTable)
        {
            using namespace lexgine::core::misc;
            {
                std::stringstream test_log;
                Log const& logger = Log::create(test_log, 2, true);

                struct custom_type
                {
                    char c;
                };

                LEXGINE_SHT_KVPAIR(key0, uint32_t);
                LEXGINE_SHT_KVPAIR(key1, long long);
                LEXGINE_SHT_KVPAIR(key2, custom_type);

                LEXGINE_SHT(table, key0, key1);
                table::add_entry<key2> my_table;

                my_table.getValue<key0>() = 0;
                my_table.getValue<key1>() = 100;
                my_table.getValue<key2>() = custom_type{ 'c' };

                Assert::IsTrue(my_table.getValue<key0>() == 0 && my_table.getValue<key1>() == 100 && my_table.getValue<key2>().c == 'c');
            }

            Log::shutdown();
        }



        TEST_METHOD(TestConcurrency)
        {
            using namespace lexgine::core::concurrency;

            uint8_t const num_consumption_threads = 7U;

            RingBufferTaskQueue queue{ 65536 };

            bool production_finished = false;
            std::atomic_uint64_t tasks_produced{ 0U };
            std::atomic_uint64_t tasks_consumed{ 0U };

            bool volatile consumption_thread_finished_consumption[num_consumption_threads]{ false };
            bool volatile consumption_thread_finished_work[num_consumption_threads]{ false };

            //! Produces 100 000 elements for the queue
            auto produce = [&queue, &tasks_produced, &production_finished]()->void
            {
                for (uint32_t i = 0; i < 100000; ++i)
                {
                    queue.enqueueTask(nullptr);
                    ++tasks_produced;
                }
                production_finished = true;

                queue.shutdown();
            };

            //! Consumes an element from the queue, simulates "work" by sleeping for 1ms, then consumes next element from the queue and so on
            auto consume = [&queue, &tasks_produced, &tasks_consumed, &production_finished,
                &consumption_thread_finished_consumption, &consumption_thread_finished_work,
                num_consumption_threads](uint8_t thread_id)->void
            {
                while (!production_finished
                    || tasks_produced.load(std::memory_order::memory_order_consume) > tasks_consumed.load(std::memory_order::memory_order_consume))
                {
                    lexgine::core::misc::Optional<AbstractTask const*> val = queue.dequeueTask();
                    if(val.isValid())
                    {
                        //Sleep(1);
                        ++tasks_consumed;
                    }
                }
                consumption_thread_finished_consumption[thread_id] = true;

                bool all_consumption_threads_finished_consumption = false;
                while(!all_consumption_threads_finished_consumption)
                {
                    all_consumption_threads_finished_consumption = true;
                    for (uint8_t i = 0; i < num_consumption_threads; ++i)
                    {
                        if (!consumption_thread_finished_consumption[i])
                            all_consumption_threads_finished_consumption = false;
                    }
                }

                queue.clearCache();
                consumption_thread_finished_work[thread_id] = true;
            };


            std::thread producer{ produce };
            producer.detach();

            std::thread* consumers[num_consumption_threads];
            for (uint8_t i = 0; i < num_consumption_threads; ++i)
            {
                consumers[i] = new std::thread{ consume, i };
                consumers[i]->detach();
            }


            // Stop the main thread until producers and consumers are done
            bool work_completed = false;
            while (!work_completed)
            {
                bool all_consumption_threads_have_reported = true;
                for(uint8_t i = 0; i < num_consumption_threads; ++i)
                {
                    if (!consumption_thread_finished_work[i])
                    {
                        all_consumption_threads_have_reported = false;
                        break;
                    }
                }

                work_completed = production_finished
                    && tasks_produced.load(std::memory_order::memory_order_consume) == tasks_consumed.load(std::memory_order::memory_order_consume)
                    && all_consumption_threads_have_reported;
            }


            for (uint8_t i = 0; i < num_consumption_threads; ++i)
            {
                delete consumers[i];
            }
        }


        TEST_METHOD(TestTaskGraphParser)
        {
            using namespace lexgine::core::concurrency;
            using namespace lexgine::core::misc;

            std::stringstream test_log;
            Log::create(test_log, 2, true);

            class CPUTask : public AbstractTask
            {
            public:
                CPUTask(std::string const& name) :
                    AbstractTask{ name }
                {

                }

            private:
                void do_task(uint8_t worker_id) override
                {

                }

                TaskType get_task_type() const override
                {
                    return TaskType::cpu;
                }
            };

            class GPUDrawTask : public AbstractTask
            {
            public:
                GPUDrawTask(std::string const& name) :
                    AbstractTask{ name }
                {

                }

            private:
                void do_task(uint8_t worker_id) override
                {

                }

                TaskType get_task_type() const override
                {
                    return TaskType::gpu_draw;
                }
            };

            class GPUComputeTask : public AbstractTask
            {
            public:
                GPUComputeTask(std::string const& name) :
                    AbstractTask{ name }
                {

                }

            private:
                void do_task(uint8_t worker_id) override
                {

                }

                TaskType get_task_type() const override
                {
                    return TaskType::gpu_compute;
                }
            };

            class GPUCopyTask : public AbstractTask
            {
            public:
                GPUCopyTask(std::string const& name) :
                    AbstractTask{ name }
                {

                }

            private:
                void do_task(uint8_t worker_id) override
                {

                }

                TaskType get_task_type() const override
                {
                    return TaskType::gpu_copy;
                }
            };

            class OtherTask : public AbstractTask
            {
            public:
                OtherTask(std::string const& name) :
                    AbstractTask{ name }
                {

                }

            private:
                void do_task(uint8_t worker_id) override
                {

                }

                TaskType get_task_type() const override
                {
                    return TaskType::other;
                }
            };

            class ExitTask : public AbstractTask
            {
            public:
                ExitTask(std::string const& name) :
                    AbstractTask{ name }
                {

                }

            private:
                void do_task(uint8_t worker_id) override
                {

                }

                TaskType get_task_type() const override
                {
                    return TaskType::exit;
                }
            };



            {
                GPUDrawTask A{ "A" };
                GPUComputeTask B{ "B" };
                GPUDrawTask C{ "C" };
                GPUCopyTask D{ "D" };
                GPUComputeTask E{ "E" };
                OtherTask F{ "F" };
                CPUTask Head{ "Head" };
                ExitTask LoopExit{ "LoopExit" };

                A.addDependent(B);
                A.addDependent(C);
                B.addDependent(D);
                C.addDependent(D);
                C.addDependent(E);
                F.addDependent(A);
                F.addDependent(C);
                F.addDependent(E);
                D.addDependent(E);
                //E.addDependent(F);
                Head.addDependent(F);
                E.addDependent(LoopExit);


                TaskGraph testGraph{ std::list<AbstractTask*>{&Head, &F, &A} };
                if (testGraph.getErrorState())
                {
                    const char* err_msg = testGraph.getErrorString();
                    std::wstring wstr{ &err_msg[0], &err_msg[strlen(err_msg) - 1] };
                    Assert::Fail(wstr.c_str());
                }
                testGraph.createDotRepresentation("task_graph.gv");
            }

            Log::shutdown();
        }

    };
}



#pragma warning(pop)