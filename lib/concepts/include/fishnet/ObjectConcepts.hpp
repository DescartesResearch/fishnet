#pragma once
#include <ostream>
#include <concepts>

namespace fishnet::util {
/**
 * @brief Helper concept which checks whether a class has a toString method
 * 
 * @tparam T 
 */
template<typename T>
concept HasToString = requires(const T &obj){
    { obj.toString() } -> std::convertible_to<std::string>;
};

/**
 * @brief Concept for objects that can be printed to an ostream
 * 
 * @tparam T 
 */
template<typename T>
concept Printable = HasToString<T> || requires(const T & obj, std::ostream & os){
    { os << obj } -> std::same_as<std::ostream &>;
};

/**
 * @brief Helper concept which checks whether a class has a hash method
 * 
 * @tparam T 
 */
template<typename T>
concept HasHashMethod = requires(const std::remove_cvref_t<T> & obj){
    {obj.hash()} -> std::convertible_to<std::size_t>;
};

/**
 * @brief Helper concept which checks whether a class is hashable, either by having a hash method or by being compatible with std::hash
 * 
 * @tparam Key 
 */
template<typename Key>
concept Hashable = HasHashMethod<Key> || requires(Key key)
{
    { std::hash<std::remove_cvref_t<Key>>{}(key) } -> std::convertible_to<std::size_t>;
};

/**
 * @brief Helper concept which checks whether a class can be natively used as a key in a map, meaning it is hashable and equality comparable
 * 
 * @tparam T 
 */
template<typename T>
concept Mapable = Hashable<T> && std::equality_comparable<T>;

/**
 * @brief Helper concept which checks whether a class is an object, meaning it has a string representation, is hashable, and is equality comparable
 * 
 * @tparam T 
 */
template<typename T>
concept Object = HasToString<T> && Hashable<T> && std::equality_comparable<T>;

} // fishnet::util

namespace std{
    template<fishnet::util::HasHashMethod T>
    struct hash<T>{
        size_t operator()(const fishnet::util::HasHashMethod auto & obj) const {
            return obj.hash();
        }
    };
}

constexpr std::ostream & operator << (std::ostream & os,fishnet::util::HasToString auto const & obj) noexcept{
    os << obj.toString();
    return os;
}
