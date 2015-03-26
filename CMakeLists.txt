cmake_minimum_required (VERSION 2.8.11)
project (VariantDemo)

set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++" )

INCLUDE_DIRECTORIES(
  "include"
)


file(GLOB SRC
  "*.cpp"
)

add_executable(VariantDemo ${SRC})
target_link_libraries(VariantDemo boost)