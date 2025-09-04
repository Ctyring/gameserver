#include <mysqlx/xdevapi.h>
#include <iostream>

int main() {
    try {
        mysqlx::Session sess("127.0.0.1", 33060, "root", "root");
        mysqlx::Schema db = sess.getSchema("test");
        mysqlx::SqlResult result = sess.sql("SELECT NOW() AS cur_time;").execute();
        for (mysqlx::Row row : result) {
            std::cout << "MySQL Time: " << row[0].get<std::string>() << std::endl;
        }

    } catch (const mysqlx::Error &err) {
        std::cerr << "ERROR: " << err << std::endl;
    } catch (std::exception &ex) {
        std::cerr << "STD EXCEPTION: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "UNKNOWN ERROR" << std::endl;
    }
    return 0;
}
