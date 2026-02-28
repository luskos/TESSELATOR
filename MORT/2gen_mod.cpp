// Add this to hook into your laptop's power system
class PowerBrickRelay {
private:
    // Physical relay connected to laptop's power input
    HANDLE hRelay;
    
public:
    void disconnectFromBrick() {
        // Send signal to relay to switch to internal power
        // This would require soldering to your laptop's power circuit
        std::cout << "⚠️ PHYSICAL MODIFICATION REQUIRED ⚠️\n";
        std::cout << "Solder relay to laptop power input\n";
        std::cout << "Connect Tesselator output to battery charging circuit\n";
    }
};