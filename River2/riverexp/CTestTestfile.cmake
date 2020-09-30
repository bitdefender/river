# CMake generated Testfile for 
# Source directory: /home/ciprian/testtools/riverexp
# Build directory: /home/ciprian/testtools/riverexp
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(TutorialComp25 "Riverexp" "25")
set_tests_properties(TutorialComp25 PROPERTIES  PASS_REGULAR_EXPRESSION "25 is 5")
add_test(TutorialComp-25 "Riverexp" "-25")
set_tests_properties(TutorialComp-25 PROPERTIES  PASS_REGULAR_EXPRESSION "-25 is 0")
subdirs(MathFunctions)
