mkdir build
cd build
conan install .. -of . -pr:b default -s:b build_type=Release -pr:h default -s:h build_type=Debug --build missing