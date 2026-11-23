add_executable(lab5_server server.cpp)
target_link_libraries(lab5_server PRIVATE ws2_32)

add_executable(lab5_client client.cpp)
target_link_libraries(lab5_client PRIVATE ws2_32)