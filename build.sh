cmake -B build -G "MinGW Makefiles"
echo "================================="
echo "Now building"
echo "================================="
cmake --build build
echo "================================="
echo "Now running"
echo "================================="
./build/renderer.exe
