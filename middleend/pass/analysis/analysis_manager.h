#ifndef __INTERFACES_MIDDLEEND_ANALYSIS_MANAGER_H__
#define __INTERFACES_MIDDLEEND_ANALYSIS_MANAGER_H__

#include <functional>
#include <set>
#include <type_utils.h>
#include <unordered_map>
#include <vector>

/*
 * 中端分析管理器 (Analysis Manager)
 *
 * 用法速览:
 * - 注册/获取分析: 通过 AM.get<YourAnalysis>(function) 获得并缓存某函数上的分析结果。
 * - 缓存失效: 当函数 IR 发生改变后，调用 AM.invalidate(function) 使相关分析失效。
 * - 分析类需定义静态常量 TID = getTID<AP>()，用于唯一标识。
 *   该标识实际上是 getTID<AP>() 实例化后的函数地址。不同实例的 getTID<AP>()
 *   所在地址不同，因此我们可以将它用作每个类的唯一 ID
 * - 参考已有示例: CFG、DomInfo 的 get<> 特化与调用方式。
 */

namespace ME
{
    class Module;
    class Function;
    class Block;

    namespace Analysis
    {
        class Manager
        {
          private:
            // 此处使用 utils/type_utils.h 的 getTID 来为每个分析类生成一个唯一的 ID
            // 类型为 uintptr_t(函数地址) -> size_t
            using AnalysisMap = std::unordered_map<size_t, void*>;
            std::unordered_map<Function*, AnalysisMap> analysisCache;

            using Deleter = void (*)(void*);
            std::unordered_map<size_t, Deleter> deleterMap;

            Manager() = default;
            ~Manager();

          public:
            static Manager& getInstance();

            template <typename Target>
            Target* get(Function& func);

            void invalidate(Function& func);

          private:
            template <typename Target>
            void registerDeleter()
            {
                size_t tid = Target::TID;
                if (deleterMap.find(tid) == deleterMap.end())
                {
                    deleterMap[tid] = [](void* p) { delete static_cast<Target*>(p); };
                }
            }

            template <typename Target>
            void cache(Function& func, Target* analysis)
            {
                analysisCache[&func][Target::TID] = analysis;
            }

            template <typename Target>
            Target* getCached(Function& func)
            {
                if (analysisCache.count(&func))
                {
                    auto& funcCache = analysisCache.at(&func);
                    if (funcCache.count(Target::TID)) return static_cast<Target*>(funcCache.at(Target::TID));
                }
                return nullptr;
            }
        };

        extern Manager& AM;
    }  // namespace Analysis
}  // namespace ME

#endif  // __INTERFACES_MIDDLEEND_ANALYSIS_MANAGER_H__
