#include "stdafx.h"
#include "CppUnitTest.h"
#include "../lexgine/log.h"
#include "../lexgine/window.h"

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
            log_string = log_string.substr(84, ref_string.size());


            Assert::IsTrue(log_string.compare(ref_string) == 0);
        }


        TEST_METHOD(TestStaticHashTable)
        {
            using namespace lexgine::core::misc;

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


    };
}



#pragma warning(pop)