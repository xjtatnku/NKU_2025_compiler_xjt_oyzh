#ifndef __INTERFACES_MIDDLEEND_ANALYSIS_DOMINFO_H__
#define __INTERFACES_MIDDLEEND_ANALYSIS_DOMINFO_H__

#include <middleend/pass/analysis/analysis_manager.h>
#include <middleend/pass/analysis/cfg.h>
#include <dom_analyzer.h>

namespace ME::Analysis
{
    class DomInfo
    {
      public:
        static inline const size_t TID = getTID<DomInfo>();

        DomAnalyzer* domAnalyzer;

      public:
        DomInfo();
        ~DomInfo();

        void build(CFG& cfg);

        const std::vector<std::vector<int>>& getDomTree() const { return domAnalyzer->dom_tree; }
        const std::vector<std::set<int>>&    getDomFrontier() const { return domAnalyzer->dom_frontier; }
        const std::vector<int>&              getImmDom() const { return domAnalyzer->imm_dom; }
    };

    template <>
    DomInfo* Manager::get<DomInfo>(Function& func);
}  // namespace ME::Analysis

#endif  // __INTERFACES_MIDDLEEND_ANALYSIS_DOMINFO_H__
