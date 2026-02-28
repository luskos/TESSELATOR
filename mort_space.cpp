#include <fstream>
#include <vector>
#include <iostream>

int main() {
    const char* filename = "D:\\mort_space.bin";
    std::ofstream ofs(filename, std::ios::binary | std::ios::out);
    
    if (!ofs) {
        std::cout << "Error: Ensure D: drive is connected and writable." << std::endl;
        return 1;
    }

    // Create 1GB buffer of 'entropy'
    std::vector<char> buffer(1024 * 1024 * 1024, 0x42); 
    
    std::cout << "Generating 50GB MORT Space... This will take a few minutes." << std::endl;
    
    for (int i = 0; i < 50; ++i) {
        ofs.write(buffer.data(), buffer.size());
        std::cout << "\rProgress: " << i + 1 << "/50 GB" << std::flush;
    }

    ofs.close();
    std::cout << "\nSpace Ready. You can now run the Resonator." << std::endl;
    return 0;
}