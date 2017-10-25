#include "stdafx.h"
#include "CppUnitTest.h"
#include "../lexgine/core/misc/log.h"
#include "../lexgine/osinteraction/windows/window.h"
#include "../lexgine/core/concurrency/ring_buffer_task_queue.h"
#include "../lexgine/core/concurrency/task_graph.h"
#include "../lexgine/core/concurrency/task_sink.h"
#include "../lexgine/core/concurrency/schedulable_task.h"
#include "../lexgine/core/misc/misc.h"
#include "../lexgine/core/exception.h"
#include "../lexgine/core/initializer.h"
#include "../lexgine/core/dx/d3d12/d3d12_pso_xml_parser.h"
#include "../lexgine/core/streamed_cache.h"

#pragma warning(push)
#pragma warning(disable : 4307)    // this is needed to suppress the warning caused by the static hash function

#include "../lexgine/core/misc/static_hash_table.h"

#include <sstream>
#include <windows.h>
#include <utility>
#include <fstream>
#include <chrono>
#include <random>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


namespace TestLogging
{

class TestErrorBehavioral
{
public:

    void raiseError(std::string const& message) const
    {
        lexgine::core::misc::Log::retrieve()->out(message, lexgine::core::misc::LogMessageType::error);
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
                Log const& logger = Log::create(test_log, "Test", 2, true);

                auto ref_time = DateTime::now(2, true);
                TestErrorBehavioral test_error_behavioral;

                LEXGINE_ERROR_LOG(
                    &test_error_behavioral,
                    (2 * 2),
                    2, 3, 5, 19, 6, 7, 8, 10, 0
                );

                auto log_time = logger.getLastEntryTimeStamp();

                Assert::IsTrue(ref_time.timeSince(log_time).seconds() < 1e-3);
            }

            Log::shutdown();
        }


        TEST_METHOD(TestStaticHashTable)
        {
            using namespace lexgine::core::misc;

            std::stringstream test_log;
            {
                Log const& logger = Log::create(test_log, "Test Static Hash Table", 2, true);

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

            RingBufferTaskQueue<int*> queue{ 65536 };

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
                    auto val = queue.dequeueTask();
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
            Log::create(test_log, "Test Task Graph Parser", 2, true);

            class CPUTask : public SchedulableTask
            {
            public:
                CPUTask(std::string const& name) :
                    SchedulableTask{ name }
                {

                }

            private:
                bool do_task(uint8_t worker_id, uint16_t frame_index) override
                {
                    return true;
                }

                TaskType get_task_type() const override
                {
                    return TaskType::cpu;
                }
            };

            class GPUDrawTask : public SchedulableTask
            {
            public:
                GPUDrawTask(std::string const& name) :
                    SchedulableTask{ name }
                {

                }

            private:
                bool do_task(uint8_t worker_id, uint16_t frame_index) override
                {
                    return true;
                }

                TaskType get_task_type() const override
                {
                    return TaskType::gpu_draw;
                }
            };

            class GPUComputeTask : public SchedulableTask
            {
            public:
                GPUComputeTask(std::string const& name) :
                    SchedulableTask{ name }
                {

                }


            private:
                bool do_task(uint8_t worker_id, uint16_t frame_index) override
                {
                    return true;
                }

                TaskType get_task_type() const override
                {
                    return TaskType::gpu_compute;
                }
            };

            class GPUCopyTask : public SchedulableTask
            {
            public:
                GPUCopyTask(std::string const& name) :
                    SchedulableTask{ name }
                {

                }

            private:
                bool do_task(uint8_t worker_id, uint16_t frame_index) override
                {
                    return true;
                }

                TaskType get_task_type() const override
                {
                    return TaskType::gpu_copy;
                }
            };

            class OtherTask : public SchedulableTask
            {
            public:
                OtherTask(std::string const& name) :
                    SchedulableTask{ name }
                {

                }

            private:
                bool do_task(uint8_t worker_id, uint16_t frame_index) override
                {
                    return true;
                }

                TaskType get_task_type() const override
                {
                    return TaskType::other;
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


                TaskGraph testGraph{ std::list<TaskGraphNode*>{&Head, &F, &A} };
                if (testGraph.getErrorState())
                {
                    Assert::Fail(lexgine::core::misc::asciiStringToWstring(testGraph.getErrorString()).c_str());
                }
                testGraph.createDotRepresentation("task_graph.gv");
            }

            Log::shutdown();
        }


        TEST_METHOD(TestTaskScheuling)
        {
            using namespace lexgine::core::concurrency;
            using namespace lexgine::core::misc;

            uint8_t const num_worker_threads = 8U;

            std::ofstream TestTaskShedulingMainLog{ "TestTaskSchedulingLog_Main.html" };
            Log::create(TestTaskShedulingMainLog, "Test Task Scheduling", 2, false);

            std::vector<std::ostream*> worker_thread_logs;
            for (uint8_t i = 0; i < num_worker_threads; ++i)
            {
                worker_thread_logs.push_back(new std::ofstream{ "TestTaskSchedulingLog_WorkerThread" + std::to_string(i) + ".html" });
            }


            {
                enum class operation_type
                {
                    add, subtract, multiply, divide
                };

                class ArithmeticOp : public SchedulableTask
                {
                public:
                    ArithmeticOp(std::string const& debug_name, float& a, float& b, float &result, operation_type op) :
                        SchedulableTask{ debug_name },
                        m_a{ a },
                        m_b{ b },
                        m_result{ result },
                        m_op{ op }
                    {

                    }

                private:
                    bool do_task(uint8_t worker_id, uint16_t frame_index) override
                    {
                        switch (m_op)
                        {
                        case operation_type::add:
                            Log::retrieve()->out(std::to_string(m_a) + "+" + std::to_string(m_b), LogMessageType::information);
                            m_result = m_a + m_b;
                            break;

                        case operation_type::subtract:
                            Log::retrieve()->out(std::to_string(m_a) + "-" + std::to_string(m_b), LogMessageType::information);
                            m_result = m_a - m_b;
                            break;

                        case operation_type::multiply:
                            Log::retrieve()->out(std::to_string(m_a) + "*" + std::to_string(m_b), LogMessageType::information);
                            m_result = m_a * m_b;
                            break;

                        case operation_type::divide:
                            Log::retrieve()->out(std::to_string(m_a) + "/" + std::to_string(m_b), LogMessageType::information);
                            m_result = m_a / m_b;

                            if (!m_b) raiseError("division by zero!");
                            break;

                        }

                        return true;
                    }

                    TaskType get_task_type() const override
                    {
                        return TaskType::cpu;
                    }

                    float& m_a;
                    float& m_b;
                    float& m_result;
                    operation_type m_op;
                };

                class ExitOp : public SchedulableTask
                {
                public:
                    ExitOp() :
                        SchedulableTask{ "ExitTask" }
                    {
                    }

                    void setInput(TaskSink* sink)
                    {
                        m_sink = sink;
                    }

                private:
                    TaskSink* m_sink;

                    bool do_task(uint8_t worker_id, uint16_t frame_index) override
                    {
                        m_sink->dispatchExitSignal();
                        return true;
                    }

                    TaskType get_task_type() const override
                    {
                        return TaskType::cpu;
                    }
                };

                float const control_value = ((5 + 3)*(8 - 1) / 2.f + 1) / ((10 + 2) * (3 - 1) / 6.f + 5);

                float r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11;
                float a0 = 5, a1 = 3, a2 = 8, a3 = 1, a4 = 10, a5 = 2, a6 = 6;
                ArithmeticOp op1{ "5+3", a0, a1, r1, operation_type::add };
                ArithmeticOp op2{ "8-1", a2, a3, r2, operation_type::subtract };
                ArithmeticOp op3{ "10+2", a4, a5, r3, operation_type::add };
                ArithmeticOp op4{ "3-1", a1, a3, r4, operation_type::subtract };
                ArithmeticOp op5{ "*", r1, r2, r5, operation_type::multiply };
                ArithmeticOp op6{ "*", r3, r4, r6, operation_type::multiply };
                ArithmeticOp op7{ "/2", r5, a5, r7, operation_type::divide };
                ArithmeticOp op8{ "/6", r6, a6, r8, operation_type::divide };
                ArithmeticOp op9{ "+1", r7, a3, r9, operation_type::add };
                ArithmeticOp op10{ "+5", r8, a0, r10, operation_type::add };
                ArithmeticOp op11{ "/", r9, r10, r11, operation_type::divide };
                ExitOp exit{};

                op1.addDependent(op5);
                op2.addDependent(op5);
                op5.addDependent(op7);
                op7.addDependent(op9);
                op9.addDependent(op11);

                op3.addDependent(op6);
                op4.addDependent(op6);
                op6.addDependent(op8);
                op8.addDependent(op10);
                op10.addDependent(op11);

                op11.addDependent(exit);

                TaskGraph taskGraph(std::list<TaskGraphNode*>{&op1, &op2, &op3, &op4});
                taskGraph.createDotRepresentation("task_graph.gv");

                TaskSink taskSink{ taskGraph, worker_thread_logs, 1 };
                exit.setInput(&taskSink);

                try 
                {
                    taskSink.run();
                }
                catch (lexgine::core::Exception const& e)
                {
                    Assert::Fail(lexgine::core::misc::asciiStringToWstring(e.description()).c_str());
                }
                

                Assert::IsTrue(r11 == control_value);
            }

            Log::retrieve()->out("Alive entities: " + std::to_string(lexgine::core::Entity::aliveEntities()), LogMessageType::information);
            Log::shutdown();

            for (auto thread_log_stream : worker_thread_logs)
            {
                delete thread_log_stream;
            }
        }

        
        TEST_METHOD(TestD3D12PSOXMLParser)
        {
            using namespace lexgine::core::dx::d3d12;
            using namespace lexgine::core::misc;
            using namespace lexgine::core;

            Initializer::initializeEnvironment("../../", "settings/");

            {
                auto content = readAsciiTextFromSourceFile("../../scripts/d3d12_PSOs/example_serialized_pso.xml");
                D3D12PSOXMLParser xml_parser{ Initializer::getGlobalParameterObjectPool(), content };
            }

            Initializer::shutdownEnvironment();
        }


        TEST_METHOD(TestStreamedCache)
        {
            using namespace lexgine::core;
            using namespace lexgine::core::misc;

            Initializer::initializeEnvironment("../..", "settings/");

            {
                std::default_random_engine rand_eng{ static_cast<unsigned int>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()) };
                std::uniform_int_distribution<uint32_t> distribution{ 0, 0xFFFFFFFF };
                uint32_t* random_value_buffer = new uint32_t[2621440];    // 10MB buffer

                for (uint32_t i = 0; i < 2621440; ++i)
                {
                    random_value_buffer[i] = distribution(rand_eng);
                }


                {
                    std::fstream iofile{ "test.bin", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc };
                    StreamedCache_KeyInt64_Cluster8KB streamed_cache{ iofile, 1024 * 1024 * 10, StreamedCacheCompressionLevel::level0, false };

                    for (int i = 0; i < 10; ++i)
                    {
                        DataBlob blob{ random_value_buffer + i * 262144U, 1048576 };
                        StreamedCache_KeyInt64_Cluster8KB::entry_type e{ i, blob };
                        streamed_cache.addEntry(e);
                    }
                }
                
                {
                    std::fstream iofile{ "test.bin", std::ios::binary | std::ios::in | std::ios::out };
                    StreamedCache_KeyInt64_Cluster8KB streamed_cache{ iofile };

                    for (int i = 9; i >= 0; --i)
                    {
                        SharedDataChunk chunk = streamed_cache.retrieveEntry(i);
                        bool res = std::memcmp(chunk.data(), random_value_buffer + i * 262144U, 1048576) == 0;

                        Assert::IsTrue(res, (L"Memory chunk " + std::to_wstring(i) + L" failed comparison").c_str());
                    }
                }

                delete[] random_value_buffer;
            }

            Initializer::shutdownEnvironment();
        }

    };
}



#pragma warning(pop)