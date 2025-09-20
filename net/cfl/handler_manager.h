#pragma once

#include <unordered_map>
#include <vector>
#include <functional>
#include <algorithm>
#include <memory>

class HandlerManager {
public:
    using HandlerFunc = std::function<bool(void*)>;

    HandlerManager() = default;
    ~HandlerManager() = default;

    // 禁止拷贝
    HandlerManager(const HandlerManager&) = delete;
    HandlerManager& operator=(const HandlerManager&) = delete;

    // 注册消息处理函数
    template <typename TClass, typename TParam>
    bool registerHandler(int msgID, bool (TClass::*func)(TParam*), TClass* obj) {
        if (!obj || !func) return false;

        HandlerFunc wrapper = [obj, func](void* data) -> bool {
            return (obj->*func)(reinterpret_cast<TParam*>(data));
        };

        handlers[msgID].emplace_back(wrapper, reinterpret_cast<void*>(obj));
        return true;
    }

    // 取消注册：根据对象地址
    template <typename TClass>
    bool unregisterHandler(int msgID, TClass* obj) {
        auto it = handlers.find(msgID);
        if (it == handlers.end()) return true;

        auto& vec = it->second;
        for (auto& [func, thisPtr] : vec) {
            if (thisPtr == reinterpret_cast<void*>(obj)) {
                thisPtr = nullptr; // 置空，不移除，避免迭代中出错
            }
        }
        return true;
    }

    // 触发消息
    template <typename TParam>
    bool fireMessage(int msgID, TParam* data) {
        auto it = handlers.find(msgID);
        if (it == handlers.end()) return false;

        for (auto& [func, thisPtr] : it->second) {
            if (thisPtr) {
                func(static_cast<void*>(data));
            }
        }
        return true;
    }

    // 清空所有消息处理器
    void clearAll() {
        handlers.clear();
    }

private:
    // 每个 handler 记录回调和所属对象指针
    using HandlerEntry = std::pair<HandlerFunc, void*>;
    std::unordered_map<int, std::vector<HandlerEntry>> handlers;
};

// 单例封装
class MsgHandlerManager : public HandlerManager {
public:
    static MsgHandlerManager& instance() {
        static MsgHandlerManager inst;
        return inst;
    }

private:
    MsgHandlerManager() = default;
    ~MsgHandlerManager() = default;
};