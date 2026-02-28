// From your earlier mort_space.cpp
#include <fstream>
#include <vector>
#include <iostream>

int main() {
    const char* filename = "C:\\MORT\\data\\mort_space.bin";
    std::ofstream ofs(filename, std::ios::binary | std::ios::out);
    
    if (!ofs) {
        std::cout << "Error: Cannot create mort_space.bin" << std::endl;
        return 1;
    }

    std::vector<char> buffer(1024 * 1024 * 1024, 0x42); // 1GB buffer
    
    std::cout << "Generating 50GB MORT Space... This will take a few minutes." << std::endl;
    
    for (int i = 0; i < 50; ++i) {
        ofs.write(buffer.data(), buffer.size());
        std::cout << "\rProgress: " << i + 1 << "/50 GB" << std::flush;
    }

    ofs.close();
    std::cout << "\nSpace Ready at C:\\MORT\\data\\mort_space.bin" << std::endl;
    return 0;
}