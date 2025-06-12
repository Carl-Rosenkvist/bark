// include/analysis_registry.h
#ifndef ANALYSIS_REGISTRY_H
#define ANALYSIS_REGISTRY_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "analysis.h"

class AnalysisRegistry {
public:
    using Factory = std::function<std::shared_ptr<Analysis>()>;

    static AnalysisRegistry& instance();

    void register_factory(const std::string& name, Factory factory);
    std::shared_ptr<Analysis> create(const std::string& name) const;

    std::vector<std::string> list_registered() const;

private:
    std::unordered_map<std::string, Factory> factories_;
};


#define REGISTER_ANALYSIS(NAME, CLASS)                           \
    namespace {                                                  \
        const bool _registered_##CLASS = []() {                  \
            AnalysisRegistry::instance().register_factory(       \
                NAME, []() -> std::shared_ptr<Analysis> {        \
                    return std::make_shared<CLASS>();            \
                });                                              \
            return true;                                         \
        }();                                                     \
    }


#endif // ANALYSIS_REGISTRY_H
