add_library(dme_loader SHARED src/data_classes.cpp src/jenkins.cpp)
target_include_directories(dme_loader PUBLIC include)
target_link_libraries(dme_loader PRIVATE spdlog::spdlog)