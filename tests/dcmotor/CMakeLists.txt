add_executable(DCMotorTest DCMotorTest.cpp)
target_link_libraries(DCMotorTest gtest pthread)
add_test(NAME DCMotorTest
         COMMAND DCMotorTest)
