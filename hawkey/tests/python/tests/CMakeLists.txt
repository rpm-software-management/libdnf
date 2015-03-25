CONFIGURE_FILE("run_nosetests.in"  "run_nosetests")
ADD_TEST(test_python run_nosetests -s ${CMAKE_CURRENT_SOURCE_DIR})
