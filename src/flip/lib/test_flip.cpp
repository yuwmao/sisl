/*********************************************************************************
 * Modifications Copyright 2017-2019 eBay Inc.
 *
 * Author/Developer(s): Harihara Kadayam
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *    https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *********************************************************************************/
#include "proto/flip_spec.pb.h"
#include "sisl/flip/flip.hpp"
#include "sisl/flip/flip_client.hpp"
#include <memory>
#include <string>
#include <atomic>
#include <thread>

#include <sisl/options/options.h>

SISL_LOGGING_INIT(flip)
SISL_OPTIONS_ENABLE(logging)

void create_ret_fspec(flip::FlipSpec* fspec) {
    *(fspec->mutable_flip_name()) = "ret_fspec";

    // Create a new condition and add it to flip spec
    auto cond = fspec->mutable_conditions()->Add();
    *cond->mutable_name() = "coll_name";
    cond->set_oper(flip::Operator::EQUAL);
    cond->mutable_value()->set_string_value("item_shipping");

    fspec->mutable_flip_action()->mutable_returns()->mutable_retval()->set_string_value("Error simulated value");

    auto freq = fspec->mutable_flip_frequency();
    freq->set_count(2);
    freq->set_percent(100);
}

void run_and_validate_ret_flip(flip::Flip* flip) {
    std::string my_coll = "item_shipping";
    std::string unknown_coll = "unknown_collection";

    auto result = flip->get_test_flip< std::string >("ret_fspec", my_coll);
    RELEASE_ASSERT(result, "get_test_flip failed for valid conditions - unexpected");
    RELEASE_ASSERT_EQ(result.get(), "Error simulated value", "Incorrect flip returned");

    result = flip->get_test_flip< std::string >("ret_fspec", unknown_coll);
    RELEASE_ASSERT(!result, "get_test_flip succeeded for incorrect conditions - unexpected");

    result = flip->get_test_flip< std::string >("ret_fspec", my_coll);
    RELEASE_ASSERT(result, "get_test_flip failed for valid conditions - unexpected");
    RELEASE_ASSERT_EQ(result.get(), "Error simulated value", "Incorrect flip returned");

    result = flip->get_test_flip< std::string >("ret_fspec", my_coll);
    RELEASE_ASSERT(!result, "get_test_flip freq set to 2, but 3rd time hit as well - unexpected"); // Not more than 2
}

void create_check_fspec(flip::FlipSpec* fspec) {
    *(fspec->mutable_flip_name()) = "check_fspec";

    auto cond = fspec->mutable_conditions()->Add();
    *cond->mutable_name() = "cmd_type";
    cond->set_oper(flip::Operator::EQUAL);
    cond->mutable_value()->set_int_value(1);

    auto freq = fspec->mutable_flip_frequency();
    freq->set_count(2);
    freq->set_percent(100);
}

void run_and_validate_check_flip(flip::Flip* flip) {
    int valid_cmd = 1;
    int invalid_cmd = -1;

    RELEASE_ASSERT(!flip->test_flip("check_fspec", invalid_cmd),
                   "test_flip succeeded for incorrect conditions - unexpected");
    RELEASE_ASSERT(flip->test_flip("check_fspec", valid_cmd), "test_flip failed for valid conditions - unexpected");
    RELEASE_ASSERT(!flip->test_flip("check_fspec", invalid_cmd),
                   "test_flip succeeded for incorrect conditions - unexpected");
    RELEASE_ASSERT(flip->test_flip("check_fspec", valid_cmd), "test_flip failed for valid conditions - unexpected");
    RELEASE_ASSERT(!flip->test_flip("check_fspec", valid_cmd),
                   "test_flip freq set to 2, but 3rd time hit as well - unexpected"); // Not more than 2
}

void create_delay_fspec(flip::FlipSpec* fspec) {
    *(fspec->mutable_flip_name()) = "delay_fspec";

    auto cond = fspec->mutable_conditions()->Add();
    *cond->mutable_name() = "cmd_type";
    cond->set_oper(flip::Operator::EQUAL);
    cond->mutable_value()->set_int_value(2);

    fspec->mutable_flip_action()->mutable_delays()->set_delay_in_usec(100000);
    auto freq = fspec->mutable_flip_frequency();
    freq->set_count(2);
    freq->set_percent(100);
}

void run_and_validate_delay_flip(flip::Flip* flip) {
    int valid_cmd = 2;
    int invalid_cmd = -1;
    std::shared_ptr< std::atomic< int > > closure_calls = std::make_shared< std::atomic< int > >(0);

    RELEASE_ASSERT(flip->delay_flip(
                       "delay_fspec", [closure_calls]() { (*closure_calls)++; }, valid_cmd),
                   "delay_flip failed for valid conditions - unexpected");

    RELEASE_ASSERT(!flip->delay_flip(
                       "delay_fspec", [closure_calls]() { (*closure_calls)++; }, invalid_cmd),
                   "delay_flip succeeded for invalid conditions - unexpected");

    RELEASE_ASSERT(flip->delay_flip(
                       "delay_fspec", [closure_calls]() { (*closure_calls)++; }, valid_cmd),
                   "delay_flip failed for valid conditions - unexpected");

    RELEASE_ASSERT(!flip->delay_flip(
                       "delay_fspec", [closure_calls]() { (*closure_calls)++; }, invalid_cmd),
                   "delay_flip succeeded for invalid conditions - unexpected");

    RELEASE_ASSERT(!flip->delay_flip(
                       "delay_fspec", [closure_calls]() { (*closure_calls)++; }, valid_cmd),
                   "delay_flip hit more than the frequency set - unexpected");

    sleep(2);
    RELEASE_ASSERT_EQ((*closure_calls).load(), 2, "Not all delay flips hit are called back");
}

void create_delay_ret_fspec(flip::FlipSpec* fspec) {
    *(fspec->mutable_flip_name()) = "delay_ret_fspec";

    auto cond = fspec->mutable_conditions()->Add();
    *cond->mutable_name() = "cmd_type";
    cond->set_oper(flip::Operator::EQUAL);
    cond->mutable_value()->set_int_value(2);

    fspec->mutable_flip_action()->mutable_delay_returns()->set_delay_in_usec(100000);
    fspec->mutable_flip_action()->mutable_delay_returns()->mutable_retval()->set_string_value(
        "Delayed error simulated value");

    auto freq = fspec->mutable_flip_frequency();
    freq->set_count(2);
    freq->set_percent(100);
}

void run_and_validate_delay_return_flip(flip::Flip* flip) {
    int valid_cmd = 2;
    int invalid_cmd = -1;
    std::shared_ptr< std::atomic< int > > closure_calls = std::make_shared< std::atomic< int > >(0);

    RELEASE_ASSERT(flip->get_delay_flip< std::string >(
                       "delay_ret_fspec",
                       [closure_calls]([[maybe_unused]] std::string error) {
                           (*closure_calls)++;
                           DEBUG_ASSERT_EQ(error, "Delayed error simulated value");
                       },
                       valid_cmd),
                   "delay_flip failed for valid conditions - unexpected");

    RELEASE_ASSERT(!flip->get_delay_flip< std::string >(
                       "delay_ret_fspec",
                       [closure_calls](std::string) {
                           assert(0);
                           (*closure_calls)++;
                       },
                       invalid_cmd),
                   "delay_flip succeeded for invalid conditions - unexpected");

    RELEASE_ASSERT(flip->get_delay_flip< std::string >(
                       "delay_ret_fspec",
                       [closure_calls]([[maybe_unused]] std::string error) {
                           DEBUG_ASSERT_EQ(error, "Delayed error simulated value");
                           (*closure_calls)++;
                       },
                       valid_cmd),
                   "delay_flip failed for valid conditions - unexpected");

    RELEASE_ASSERT(!flip->get_delay_flip< std::string >(
                       "delay_ret_fspec",
                       [closure_calls](std::string) {
                           assert(0);
                           (*closure_calls)++;
                       },
                       invalid_cmd),
                   "delay_flip succeeded for invalid conditions - unexpected");

    RELEASE_ASSERT(!flip->get_delay_flip< std::string >(
                       "delay_ret_fspec",
                       [closure_calls](std::string error) {
                           DEBUG_ASSERT_EQ(error, "Delayed error simulated value");
                           (*closure_calls)++;
                           LOGINFO("Called with error = {}", error);
                       },
                       valid_cmd),
                   "delay_flip hit more than the frequency set - unexpected");

    sleep(2);
    RELEASE_ASSERT_EQ((*closure_calls).load(), 2, "Not all delay flips hit are called back");
}

#if 0
void create_multi_cond_fspec(flip::FlipSpec *fspec) {
    *(fspec->mutable_flip_name()) = "multi_cond1_fspec";

    // Create a new condition and add it to flip spec
    auto cond1 = fspec->mutable_conditions()->Add();
    *cond1->mutable_name() = "cmd_type";
    cond1->set_oper(flip::Operator::EQUAL);
    cond1->mutable_value()->set_int_value(1);

    auto cond2 = fspec->mutable_conditions()->Add();
    *cond2->mutable_name() = "coll_name";
    cond2->set_oper(flip::Operator::EQUAL);
    cond2->mutable_value()->set_string_value("item_shipping");

    fspec->mutable_flip_action()->mutable_returns()->mutable_return_()->set_string_value("Error simulated value");

    auto freq = fspec->mutable_flip_frequency();
    freq->set_count(2);
    freq->set_percent(100);
}
#endif

// ========== Callback Flip Tests ==========

// Test basic callback flip (no return value)
void test_basic_callback_flip() {
    flip::Flip flip;
    flip::FlipClient fclient(&flip);

    std::atomic<int> callback_count{0};
    int received_param = 0;

    flip::FlipCondition cond = fclient.create_condition("val", flip::Operator::EQUAL, 1);
    flip::FlipFrequency freq;
    freq.set_count(2);
    freq.set_percent(100);

    fclient.inject_callback_flip<void, int>(
        "test_callback",
        {cond},
        freq,
        std::function<void(int)>([&callback_count, &received_param](int val) {
            callback_count++;
            received_param = val;
        })
    );

    // First trigger - should match
    RELEASE_ASSERT(flip.callback_flip("test_callback", 1), "callback_flip should return true for first trigger");
    RELEASE_ASSERT_EQ(callback_count.load(), 1, "Callback should be called once");
    RELEASE_ASSERT_EQ(received_param, 1, "Received parameter should be 1");

    // Second trigger - should match
    RELEASE_ASSERT(flip.callback_flip("test_callback", 1), "callback_flip should return true for second trigger");
    RELEASE_ASSERT_EQ(callback_count.load(), 2, "Callback should be called twice");
    RELEASE_ASSERT_EQ(received_param, 1, "Received parameter should still be 1");

    // Third trigger - should not match (count exhausted)
    RELEASE_ASSERT(!flip.callback_flip("test_callback", 1), "callback_flip should return false when count exhausted");
    RELEASE_ASSERT_EQ(callback_count.load(), 2, "Callback should still be called only twice");

    LOGINFO("test_basic_callback_flip PASSED");
}

// Test callback flip with non-matching condition
void test_callback_flip_no_match() {
    flip::Flip flip;
    flip::FlipClient fclient(&flip);

    std::atomic<int> callback_count{0};

    flip::FlipCondition cond = fclient.create_condition("val", flip::Operator::EQUAL, 1);
    flip::FlipFrequency freq;
    freq.set_count(10);
    freq.set_percent(100);

    fclient.inject_callback_flip<void, int>(
        "test_callback_no_match",
        {cond},
        freq,
        std::function<void(int)>([&callback_count](int) { callback_count++; })
    );

    // Non-matching parameter - should not trigger
    RELEASE_ASSERT(!flip.callback_flip("test_callback_no_match", 999), "callback_flip should return false for non-matching condition");
    RELEASE_ASSERT_EQ(callback_count.load(), 0, "Callback should not be called");

    // Matching parameter - should trigger
    RELEASE_ASSERT(flip.callback_flip("test_callback_no_match", 1), "callback_flip should return true for matching condition");
    RELEASE_ASSERT_EQ(callback_count.load(), 1, "Callback should be called once");

    LOGINFO("test_callback_flip_no_match PASSED");
}

// Test callback flip with reference parameter modification
void test_callback_flip_reference_param() {
    flip::Flip flip;
    flip::FlipClient fclient(&flip);

    int captured_value = 0;
    int received_value = 0;

    flip::FlipCondition cond1 = fclient.create_condition("op", flip::Operator::DONT_CARE, 0);
    flip::FlipCondition cond2 = fclient.create_condition("data", flip::Operator::DONT_CARE, 0);
    flip::FlipFrequency freq;
    freq.set_count(1);
    freq.set_percent(100);

    fclient.inject_callback_flip<void, int, int&>(
        "test_reference",
        {cond1, cond2},
        freq,
        std::function<void(int, int&)>([&captured_value](int, int& data) {
            captured_value = data;  // Capture the original value
            data = 999;  // Modify the original
        })
    );

    received_value = 42;
    RELEASE_ASSERT(flip.callback_flip("test_reference", 1, received_value),
                   "callback_flip should return true");
    RELEASE_ASSERT_EQ(captured_value, 42, "Callback should receive original value");
    RELEASE_ASSERT_EQ(received_value, 999, "Original should be modified by callback");

    LOGINFO("test_callback_flip_reference_param PASSED");
}

// Test callback flip with return value
void test_callback_retval_flip() {
    flip::Flip flip;
    flip::FlipClient fclient(&flip);

    flip::FlipCondition cond = fclient.create_condition("val", flip::Operator::EQUAL, 42);
    flip::FlipFrequency freq;
    freq.set_count(1);
    freq.set_percent(100);

    fclient.inject_callback_retval_flip<int, int>(
        "test_retval_callback",
        {cond},
        freq,
        std::function<int(int)>([](int input) -> int {
            return input * 2;  // Return double the input
        })
    );

    // First trigger
    auto result = flip.get_callback_flip<int>("test_retval_callback", 42);
    RELEASE_ASSERT(result, "get_callback_flip should return a value");
    RELEASE_ASSERT_EQ(result.get(), 84, "Return value should be double input");

    // Count exhausted - should return boost::none
    result = flip.get_callback_flip<int>("test_retval_callback", 42);
    RELEASE_ASSERT(!result, "get_callback_flip should return boost::none when exhausted");

    LOGINFO("test_callback_retval_flip PASSED");
}

// Test callback flip with boolean return
void test_callback_bool_retval() {
    flip::Flip flip;
    flip::FlipClient fclient(&flip);

    flip::FlipCondition cond = fclient.create_condition("check", flip::Operator::DONT_CARE, 0);
    flip::FlipFrequency freq;
    freq.set_count(2);
    freq.set_percent(100);

    fclient.inject_callback_retval_flip<bool, int>(
        "test_bool_callback",
        {cond},
        freq,
        std::function<bool(int)>([](int value) -> bool {
            return value > 10;  // Return true if value > 10
        })
    );

    auto result1 = flip.get_callback_flip<bool>("test_bool_callback", 5);
    RELEASE_ASSERT(result1, "get_callback_flip should return a value");
    RELEASE_ASSERT(!result1.get(), "Should return false for value <= 10");

    auto result2 = flip.get_callback_flip<bool>("test_bool_callback", 20);
    RELEASE_ASSERT(result2, "get_callback_flip should return a value");
    RELEASE_ASSERT(result2.get(), "Should return true for value > 10");

    LOGINFO("test_callback_bool_retval PASSED");
}

// Test callback flip with exception handling
void test_callback_exception_handling() {
    flip::Flip flip;
    flip::FlipClient fclient(&flip);

    std::atomic<int> callback_count{0};

    flip::FlipCondition cond = fclient.create_condition("val", flip::Operator::DONT_CARE, 0);
    flip::FlipFrequency freq;
    freq.set_count(2);
    freq.set_percent(100);

    fclient.inject_callback_flip<void, int>(
        "test_exception",
        {cond},
        freq,
        std::function<void(int)>([&callback_count](int) {
            callback_count++;
            throw std::runtime_error("Test exception in callback");
        })
    );

    // First trigger - should catch exception and return true (callback was called)
    // Note: callback_flip returns true to indicate the flip point was TRIGGERED, not that the
    // callback SUCCEEDED. This allows distinguishing between "flip not activated" (returns false)
    // vs "flip activated but callback threw" (returns true + exception log).
    RELEASE_ASSERT(flip.callback_flip("test_exception", 1),
                   "callback_flip should return true even when callback throws");
    RELEASE_ASSERT_EQ(callback_count.load(), 1, "Callback should be called once despite exception");

    // Second trigger - same behavior
    RELEASE_ASSERT(flip.callback_flip("test_exception", 2),
                   "callback_flip should return true for second trigger");
    RELEASE_ASSERT_EQ(callback_count.load(), 2, "Callback should be called twice");

    LOGINFO("test_callback_exception_handling PASSED");
}

// Test callback flip with thread safety (concurrent triggers)
void test_callback_thread_safety() {
    flip::Flip flip;
    flip::FlipClient fclient(&flip);

    std::atomic<int> callback_count{0};
    constexpr int num_threads = 4;
    constexpr int triggers_per_thread = 10;

    flip::FlipCondition cond = fclient.create_condition("val", flip::Operator::DONT_CARE, 0);
    flip::FlipFrequency freq;
    freq.set_count(num_threads * triggers_per_thread);  // Allow all triggers
    freq.set_percent(100);

    fclient.inject_callback_flip<void, int>(
        "test_concurrent",
        {cond},
        freq,
        std::function<void(int)>([&callback_count](int) {
            callback_count++;
        })
    );

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&flip, triggers_per_thread]() {
            for (int i = 0; i < triggers_per_thread; ++i) {
                flip.callback_flip("test_concurrent", i);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    RELEASE_ASSERT_EQ(callback_count.load(), num_threads * triggers_per_thread,
                      "All concurrent callbacks should be executed");

    LOGINFO("test_callback_thread_safety PASSED");
}

// Test callback flip without any callback registered
void test_callback_not_registered() {
    flip::Flip flip;

    // Trigger a flip that has no callback registered
    auto result = flip.callback_flip("non_existent_flip", 1);
    RELEASE_ASSERT(!result, "callback_flip should return false for non-existent flip");

    LOGINFO("test_callback_not_registered PASSED");
}

// Test callback flip with multiple parameters
void test_callback_multiple_params() {
    flip::Flip flip;
    flip::FlipClient fclient(&flip);

    int received_a = 0;
    int received_b = 0;
    double received_c = 0.0;

    flip::FlipCondition cond1 = fclient.create_condition("a", flip::Operator::DONT_CARE, 0);
    flip::FlipCondition cond2 = fclient.create_condition("b", flip::Operator::DONT_CARE, 0);
    flip::FlipCondition cond3 = fclient.create_condition("c", flip::Operator::DONT_CARE, 0);
    flip::FlipFrequency freq;
    freq.set_count(1);
    freq.set_percent(100);

    fclient.inject_callback_flip<void, int, int, double>(
        "test_multi_params",
        {cond1, cond2, cond3},
        freq,
        std::function<void(int, int, double)>([&received_a, &received_b, &received_c](int a, int b, double c) {
            received_a = a;
            received_b = b;
            received_c = c;
        })
    );

    RELEASE_ASSERT(flip.callback_flip("test_multi_params", 42, 100, 3.14),
                   "callback_flip should return true");

    RELEASE_ASSERT_EQ(received_a, 42, "First parameter should be received correctly");
    RELEASE_ASSERT_EQ(received_b, 100, "Second parameter should be received correctly");
    RELEASE_ASSERT_EQ(received_c, 3.14, "Third parameter should be received correctly");

    LOGINFO("test_callback_multiple_params PASSED");
}

int main(int argc, char* argv[]) {
    SISL_OPTIONS_LOAD(argc, argv, logging)
    sisl::logging::SetLogger(std::string(argv[0]));
    spdlog::set_pattern("[%D %T%z] [%^%l%$] [%n] [%t] %v");

    flip::FlipSpec ret_fspec;
    create_ret_fspec(&ret_fspec);

    flip::FlipSpec check_fspec;
    create_check_fspec(&check_fspec);

    flip::FlipSpec delay_fspec;
    create_delay_fspec(&delay_fspec);

    flip::FlipSpec delay_ret_fspec;
    create_delay_ret_fspec(&delay_ret_fspec);

    flip::Flip flip;
    flip.start_rpc_server();
    flip.add(ret_fspec);
    flip.add(check_fspec);
    flip.add(delay_fspec);
    flip.add(delay_ret_fspec);

    run_and_validate_ret_flip(&flip);
    run_and_validate_check_flip(&flip);
    run_and_validate_delay_flip(&flip);
    run_and_validate_delay_return_flip(&flip);

    // Run callback tests
    test_basic_callback_flip();
    test_callback_flip_no_match();
    test_callback_flip_reference_param();
    test_callback_retval_flip();
    test_callback_bool_retval();
    test_callback_exception_handling();
    test_callback_thread_safety();
    test_callback_not_registered();
    test_callback_multiple_params();

    LOGINFO("All flip tests (including callbacks) PASSED");
    return 0;
}
