add_executable(sample_instances instances.cpp)
target_link_libraries(sample_instances gathering)

# copy texture and shader files into the executable folder
# here all examples are in the same folder ... so have to call it just once
copy_resources(sample_instances)
