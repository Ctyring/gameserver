#include <iostream>
#include <cassert>
#include "cfl/handler_manager.h"
#include "cfl/event_handler_manager.h"
#include <chrono>
#include <Windows.h>

using namespace cfl;

// 测试用的类
class TestClass {
public:
    int eventCount = 0;
    int messageCount = 0;

    bool handleEvent(cfl::EventParam* param) {
        eventCount++;
        std::cout << "处理事件 ID: " << param->eventId << std::endl;
        std::cout << "参数1: " << param->intParams[0] << ", 参数2: " << param->intParams[1] << std::endl;
        std::cout << "长整型参数1: " << param->longParams[0] << ", 长整型参数2: " << param->longParams[1] << std::endl;
        return true;
    }

    bool handleMessage(int* data) {
        messageCount++;
        std::cout << "处理消息，数据值: " << *data << std::endl;
        return true;
    }
};

// 用于 HandlerManager 测试的简单结构
struct SimpleMessage {
    int value;
    std::string text;
};

class SimpleHandler {
public:
    int handleCount = 0;

    bool handleSimpleMessage(SimpleMessage* msg) {
        handleCount++;
        std::cout << "处理简单消息 - 值: " << msg->value << ", 文本: " << msg->text << std::endl;
        return true;
    }
};

int globalMessageCount = 0;
bool handleGlobalMessage(int* data) {
    globalMessageCount++;
    std::cout << "处理全局函数消息，数据值: " << *data << std::endl;
    return true;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::cout << "=== HandlerManager 和 EventHandlerManager 测试 ===" << std::endl;

    // 测试 HandlerManager
    std::cout << "\n--- HandlerManager 基本功能测试 ---" << std::endl;
    HandlerManager handlerManager;

    // 测试类成员函数注册
    TestClass testObj;
    int memberMsgId = 1001;
    handlerManager.registerHandler(memberMsgId, &TestClass::handleMessage, &testObj);

    int data = 123;
    handlerManager.fireMessage(memberMsgId, &data);
    assert(testObj.messageCount == 1);

    // 测试全局函数注册
//    int globalMsgId = 1002;
//    handlerManager.registerHandler(globalMsgId, &GlobalFunctionWrapper::handleGlobalMessage, &globalWrapper);
//    handlerManager.fireMessage(globalMsgId, &data);
//    assert(globalMessageCount == 1);

    // 测试 SimpleHandler 类
    SimpleHandler simpleObj;
    int simpleMsgId = 1003;
    SimpleMessage msg{456, "Hello"};
    handlerManager.registerHandler(simpleMsgId, &SimpleHandler::handleSimpleMessage, &simpleObj);
    handlerManager.fireMessage(simpleMsgId, &msg);
    assert(simpleObj.handleCount == 1);

    // 测试 MsgHandlerManager 单例
    std::cout << "\n--- MsgHandlerManager 单例测试 ---" << std::endl;
    int singletonMsgId = 1004;
    MsgHandlerManager::instance().registerHandler(singletonMsgId, &TestClass::handleMessage, &testObj);
    int singletonData = 789;
    MsgHandlerManager::instance().fireMessage(singletonMsgId, &singletonData);
    assert(testObj.messageCount == 2);

    // 测试 EventHandlerManager
    std::cout << "\n--- EventHandlerManager 测试 ---" << std::endl;
    auto& eventManager = cfl::EventHandlerManager::instance();

    // 注册事件处理器
    uint32_t eventId = 2001;
    eventManager.registerHandler(eventId, &TestClass::handleEvent, &testObj);

    // 触发事件
    std::cout << "触发事件..." << std::endl;
    eventManager.fireEvent(eventId, 100, 200, 1000ULL, 2000ULL);
    assert(testObj.eventCount == 1);

    // 测试注销功能
    std::cout << "\n--- 注销功能测试 ---" << std::endl;
    eventManager.unregisterHandler(eventId, &testObj);
    std::cout << "注销后再次触发事件:" << std::endl;
    eventManager.fireEvent(eventId, 100, 200, 1000ULL, 2000ULL); // 应该不会增加计数
    assert(testObj.eventCount == 1); // 计数应该保持不变

    // 测试多个处理器
    std::cout << "\n--- 多个事件处理器测试 ---" << std::endl;
    TestClass testObj2;
    uint32_t eventId2 = 3001;

    eventManager.registerHandler(eventId2, &TestClass::handleEvent, &testObj);
    eventManager.registerHandler(eventId2, &TestClass::handleEvent, &testObj2);

    eventManager.fireEvent(eventId2, 111, 222, 333ULL, 444ULL);
    assert(testObj.eventCount == 2);
    assert(testObj2.eventCount == 1);

    // 测试 HandlerManager 清空功能
    std::cout << "\n--- HandlerManager 清空功能测试 ---" << std::endl;
    handlerManager.clearAll();
    handlerManager.fireMessage(memberMsgId, &data); // 应该不会有任何效果
    assert(testObj.messageCount == 2); // 计数应该保持不变

    std::cout << "\n=== 所有测试完成 ===" << std::endl;
    return 0;
}
