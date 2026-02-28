// Use harvested energy to charge laptop battery
class BatteryHarvester {
public:
    void chargeFromReactor(double watts) {
        // Windows doesn't allow direct battery charging control
        // You'd need:
        // 1. External charging circuit
        // 2. Relay to switch between brick and reactor
        // 3. Voltage regulation
        
        std::cout << "Charging at " << watts << "W\n";
        std::cout << "Time to full: " << (50 / watts) << " hours\n";
    }
};