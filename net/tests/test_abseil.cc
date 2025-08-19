#include <iostream>
#include <chrono>
#include <unordered_map>
#include <absl/container/flat_hash_map.h>
#include <random>

constexpr size_t NUM_OPERATIONS = 1000000; // 1 million operations

template <typename MapType>
void test_map(const std::string& map_name) {
    // 使用随机数生成器填充数据
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000000);

    // 测试插入操作
    auto start = std::chrono::high_resolution_clock::now();
    MapType map;
    for (size_t i = 0; i < NUM_OPERATIONS; ++i) {
        map[dist(gen)] = dist(gen);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> insert_duration = end - start;
    std::cout << map_name << " Insert Time: " << insert_duration.count() << " seconds" << std::endl;

    // 测试查找操作
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < NUM_OPERATIONS; ++i) {
        auto key = dist(gen);
        map.find(key);
    }
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> find_duration = end - start;
    std::cout << map_name << " Find Time: " << find_duration.count() << " seconds" << std::endl;

    // 测试删除操作
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < NUM_OPERATIONS / 10; ++i) {
        auto key = dist(gen);
        map.erase(key);
    }
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> erase_duration = end - start;
    std::cout << map_name << " Erase Time: " << erase_duration.count() << " seconds" << std::endl;
}

int main() {
    std::cout << "Testing std::unordered_map:" << std::endl;
    test_map<std::unordered_map<int, int>>("std::unordered_map");

    std::cout << "\nTesting absl::flat_hash_map:" << std::endl;
    test_map<absl::flat_hash_map<int, int>>("absl::flat_hash_map");

    return 0;
}
