#pragma once

#include "IDNode.h"
#include <functional>
#include <string>
#include <fishnet/ObjectConcepts.hpp>

template<typename T>
class DataIDNode : public IDNode {
private:
    T data;
public:
    DataIDNode(const T & data):IDNode(), data(data){}
    ~DataIDNode() = default;
    T getData() const {
        return this->data;
    }
    std::string toString() const noexcept requires std::convertible_to<T,std::string>{
        return std::string(data);
    }

    size_t hash() const noexcept requires fishnet::util::Hashable<T> {
        return std::hash<T>{}(this->data);
    }
};

static_assert(fishnet::util::Hashable<DataIDNode<std::string>>);
static_assert(fishnet::util::HasToString<DataIDNode<std::string>>);
