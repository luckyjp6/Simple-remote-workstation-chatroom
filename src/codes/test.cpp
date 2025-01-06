#include "util.hpp"
#include <fstream>

class worm_server {
public:
    void openFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            // 呼叫 err_sys，傳入類別指標、__func__ 和自定義訊息
            err_sys(this, __func__, filename);
        }
    }

    void anotherFunction() {
        // errno = EPERM; // 模擬權限錯誤
        err_sys(this, __func__, "An operation failed due to insufficient permissions.");
    }
};

int main() {
    worm_server obj;
    obj.openFile("nonexistent.txt"); // 測試檔案打開失敗
    // obj.anotherFunction();          // 測試自定義錯誤訊息
    return 0;
}