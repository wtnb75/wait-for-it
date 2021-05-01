#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#define main do_main
#include "wait-for-it.cpp"
#undef main

class waitforTest : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(waitforTest);
    CPPUNIT_TEST(test_resolve);
    CPPUNIT_TEST(test_connect);
    CPPUNIT_TEST(test_run_command);
    CPPUNIT_TEST(test_run);
    CPPUNIT_TEST_SUITE_END();

   public:
    void test_resolve() {}
    void test_connect() {}
    void test_run_command() {}
    void test_run() {}
};

CPPUNIT_TEST_SUITE_REGISTRATION(waitforTest);

int main(int argc, char const *argv[]) {
    CPPUNIT_NS::TestResult controller;
    CPPUNIT_NS::TestResultCollector result;
    controller.addListener(&result);
    CPPUNIT_NS::BriefTestProgressListener progress;
    controller.addListener(&progress);
    CPPUNIT_NS::TestRunner runner;
    runner.addTest(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());
    runner.run(controller);
    CPPUNIT_NS::CompilerOutputter outputter(&result, CPPUNIT_NS::stdCOut());
    outputter.write();
    return result.wasSuccessful() ? 0 : 1;
}
