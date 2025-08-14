#include <asio.hpp>
#include <iostream>

int main() {
    try {
        asio::io_context io;

        asio::steady_timer timer(io, std::chrono::seconds(2)); // 2秒定时器

        timer.async_wait([](const asio::error_code& ec) {
            if (!ec) {
                std::cout << "Timer expired! Hello from ASIO." << std::endl;
            } else {
                std::cout << "Timer error: " << ec.message() << std::endl;
            }
        });

        std::cout << "Waiting for timer..." << std::endl;
        io.run();  // 运行事件循环，直到所有异步操作完成
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }

    return 0;
}
