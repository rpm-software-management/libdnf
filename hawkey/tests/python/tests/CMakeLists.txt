CONFIGURE_FILE("run_nosetests.sh.in"  "run_nosetests.sh")
ADD_TEST(test_python run_nosetests.sh -s ${CMAKE_CURRENT_SOURCE_DIR})
