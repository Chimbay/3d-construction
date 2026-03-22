# Windows: cmake -B build -G "MinGW Makefiles"
# macOS: use default Unix Makefiles generator, force system git to avoid Homebrew libcurl conflict
cmake -B build -DGIT_EXECUTABLE=/usr/bin/git
echo "================================="
echo "Now building"
echo "================================="
cmake --build build
echo "================================="
echo "Now running"
echo "================================="
# Windows: ./build/renderer.exe
# macOS:
./build/program
