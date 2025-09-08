#pragma once
#include <memory>
#include "cfl.h"
namespace cfl {

/**
 * @brief 单例模式封装类
 * @tparam T 要单例化的类型
 * @tparam Tag 用于区分不同实例的标记类型
 * @tparam N  同一 Tag 下的实例编号
 */
    template<class T, class Tag = void, int N = 0>
    class Singleton {
    public:
        /**
         * @brief 获取单例对象的裸指针
         */
        CFL_API static T* instance() noexcept {
            static T v;
            return &v;
        }
    };

/**
 * @brief 智能指针形式的单例封装
 * @tparam T 要单例化的类型
 * @tparam Tag 用于区分不同实例的标记类型
 * @tparam N  同一 Tag 下的实例编号
 */
    template<class T, class Tag = void, int N = 0>
    class SingletonPtr {
    public:
        /**
         * @brief 获取单例对象的智能指针
         */
        CFL_API static std::shared_ptr<T> instance() noexcept {
            static std::shared_ptr<T> obj = std::make_shared<T>();
            return obj;
        }
    };

} // namespace cfl
