add_executable(c5_machine_learner
 attwinnow.c
 c50.c
 confmat.c
 classify.c
 construct.c
 contin.c
 discr.c
 formrules.c
 formtree.c
 getdata.c
 getnames.c
 global.c
 implicitatt.c
 info.c
 mcost.c
 modelfiles.c
 prune.c
 p-thresh.c
 rules.c
 ruletree.c
 siftrules.c
 sort.c
 subset.c
 trees.c
 update.c
 utility.c
 xval.c
)
find_library(M_LIB m)
target_link_libraries(c5_machine_learner ${M_LIB})
set_target_properties(c5_machine_learner PROPERTIES OUTPUT_NAME c5.0)

# Install libraries and executables
install(TARGETS c5_machine_learner
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

