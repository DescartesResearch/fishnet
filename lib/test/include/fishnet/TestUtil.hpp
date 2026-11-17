#pragma once
#include <gtest/gtest.h>
#include <ranges>
#include <stacktrace>
#include <string>
#include <type_traits>
#include <utility>
#include <filesystem>
#include <fishnet/Concepts.hpp>


namespace fishnet::test{

namespace __impl{

template <typename T>
constexpr std::string_view get_type_name() {
    std::string_view name;
#if defined(__clang__) || defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
#else
    return "unknown_type";
#endif

    // 1. Look for the exact starting token of the type value assignment
    size_t start = name.find("T = ");
    if (start != std::string_view::npos) {
        start += 4; // Shift past "T = "
        
        // Find where the type definition stops (either a semicolon or a bracket)
        size_t end = name.find_first_of(";]", start);
        if (end != std::string_view::npos) {
            return name.substr(start, end - start);
        }
    }

    // 2. Fallback for MSVC or non-standard configurations (searches the innermost template)
    start = name.find_last_of('<');
    size_t end = name.find_last_of(">]");
    if (start != std::string_view::npos && end != std::string_view::npos && end > start) {
        return name.substr(start + 1, end - start - 1);
    }

    return name; // Absolute fallback safety
}

template<typename T>
concept Printable = requires(std::stringstream ss,const std::remove_cvref_t<T> & value) {
    {ss << value};
};

template<typename T>
concept OptionalOrExpected = requires(const T & t){
    {t.has_value()} -> std::convertible_to<bool>;
    {t.value()} -> std::convertible_to<const typename std::remove_cvref_t<T>::value_type &>;
};

template<typename T>
concept AssociativeContainer = requires(std::remove_cvref_t<T> container, typename std::remove_cvref_t<T>::key_type key) {
    {container.find(key)} -> std::same_as<typename std::remove_cvref_t<T>::iterator>;
};

static inline void printValue(std::stringstream & ss, const auto & value) {
    if constexpr (Printable<std::remove_cvref_t<decltype(value)>>) {
        ss << value;
    } else {
        ss << '?';
    }
}

static std::string printContainer(std::ranges::input_range auto && container) {
    std::pair<char, char> containerBoundary = std::make_pair('[', ']');
    if constexpr (AssociativeContainer<std::remove_cvref_t<decltype(container)>>) {
        containerBoundary = std::make_pair('{', '}');
    }
    std::stringstream ss;
    ss << containerBoundary.first;
    bool isFirst = true;
    for(const auto & element : container) {
        if(!isFirst) {
            ss << ", ";
        }
        printValue(ss, element);
        isFirst = false;
    }
    ss << containerBoundary.second;
    return ss.str();
}

static auto message(auto... vals) {
    std::stringstream ss;
    ss << "\033[1;31m";
    ((printValue(ss, vals)), ...);
    ss << "\033[0m";
    return ss.str();
}

static auto trace(){
    std::stringstream ss;
    ss << "\033[1;36m";
    ss << "Stack trace:" << std::endl;
    auto st = std::stacktrace::current();
    auto it = st.begin();
    while(it != st.end() && it->source_file().ends_with("TestUtil.hpp"))
         ++it;
    auto first = it;
    while(not it->source_file().ends_with("gtest.cc") && it != st.end()) {
        if (it != first)
            ss << std::endl;
        ss << "\t" << it->source_file() << ":" << it->source_line();
        ++it;
    }
    ss << "\033[0m";
    return ss.str();
}

static auto messageWithTrace(auto... vals) {
    std::stringstream ss;
    ss << message(vals...) << std::endl;
    ss << trace();
    return ss.str();
}

template<typename T, typename Eq = std::equal_to<>>
static testing::AssertionResult contains(std::ranges::input_range auto && collection, const T & element, Eq const & eq = Eq{}) {
    if(std::ranges::end(collection) != std::ranges::find_if(collection, [&element, &eq](const auto & e) { return eq(e, element); })){
        return testing::AssertionSuccess();
    }else{
        return testing::AssertionFailure() << message("Collection ", printContainer(collection), " does not contain element: ", element);
    }
}

template<typename T, typename Eq = std::equal_to<>>
static testing::AssertionResult notContains(std::ranges::input_range auto && collection, const T & element, Eq const & eq = Eq{}) {
    if(std::ranges::end(collection) == std::ranges::find_if(collection, [&element, &eq](const auto & e) { return eq(e, element); })){
        return testing::AssertionSuccess();
    }else{
        return testing::AssertionFailure() << message("Collection ", printContainer(collection), " contains element: ", element);
    }
}

template<typename Eq = std::equal_to<>>
static testing::AssertionResult unsortedRangeEqual(std::ranges::forward_range auto const & actual, std::ranges::forward_range auto const & expected, Eq const & eq = Eq{}) {
    using A = std::ranges::range_value_t<decltype(actual)>;
    using E = std::ranges::range_value_t<decltype(expected)>;
    static_assert(fishnet::util::BiFunction<decltype(eq), A, E,bool>, "eq must be a binary function that takes two arguments of type A and E");
    if(fishnet::util::size(actual) != fishnet::util::size(expected)){
        return testing::AssertionFailure() << message("Ranges have a different size!\nExpecting: ", fishnet::util::size(expected), " but was: ", fishnet::util::size(actual));
    }
    for(const auto & expectedElement : expected){
        auto result = contains(actual, expectedElement, eq);
        if(!result){
            return result;
        }
    }
    return testing::AssertionSuccess() << message("Ranges are equal (ignoring order)");
}

template<typename Eq = std::equal_to<>>
static testing::AssertionResult sortedRangeEqual(std::ranges::forward_range auto const & actual, std::ranges::forward_range auto const & expected, Eq const & eq = Eq{}) {
    using A = std::ranges::range_value_t<decltype(actual)>;
    using E = std::ranges::range_value_t<decltype(expected)>;
    static_assert(std::convertible_to<A,E>, "Actual and expected collection must have convertible value type");
    static_assert(fishnet::util::BiFunction<decltype(eq), A, E,bool>, "eq must be a binary function that takes two arguments of type A and E");
    auto actualIt = std::ranges::cbegin(actual);
    auto expectedIt = std::ranges::cbegin(expected);
    while(actualIt != std::ranges::cend(actual) && expectedIt != std::ranges::cend(expected)){
        if(!eq(*actualIt,*expectedIt)){
            if constexpr (Printable<A> && Printable<E>) {
                return testing::AssertionFailure() << message("Ranges differ at index ", std::distance(std::ranges::cbegin(actual), actualIt), "! Expecting ", *expectedIt, " but was: ", *actualIt);
            } else {
                return testing::AssertionFailure() << message("Ranges differ at index ", std::distance(std::ranges::cbegin(actual), actualIt));
            }
        }
        ++actualIt;
        ++expectedIt;
    }
    if(actualIt != std::ranges::cend(actual) or expectedIt != std::ranges::cend(expected)) {
        return testing::AssertionFailure() << message("Ranges have a different size!", "Expecting: ", fishnet::util::size(expected), " but was: ", fishnet::util::size(actual));
    }
    return testing::AssertionSuccess() << message("Ranges are equal (in order)");
}
} // namespace fishnet::test::__impl

static void TODO() {
   FAIL() << __impl::messageWithTrace("Test not implemented yet");  
}

template<typename T>
static void EXPECT_CONTAINS(fishnet::util::input_range_of<T> auto && collection, const T & element) {
    EXPECT_TRUE(__impl::contains(collection,element)) << __impl::trace();
}

template<typename T>
static void EXPECT_NOT_CONTAINS(fishnet::util::input_range_of<T> auto && collection, const T & element) {
    EXPECT_TRUE(__impl::notContains(collection,element)) << __impl::trace();
}

static void EXPECT_CONTAINS_ALL(std::ranges::input_range auto && collection, std::ranges::input_range auto && expected){
    for(const auto & element : expected){
        EXPECT_TRUE(__impl::contains(collection,element)) << __impl::trace();
    }
}

template<typename... Args>
static void EXPECT_CONTAINS_ALL(std::ranges::input_range auto && collection, const Args & ...args){
    (EXPECT_CONTAINS(collection,args), ...);
}

static void EXPECT_UNSORTED_RANGE_EQ(std::ranges::forward_range auto const & actual, std::ranges::forward_range auto const & expected) {
    EXPECT_TRUE(__impl::unsortedRangeEqual(actual, expected)) << __impl::trace();
}

static void EXPECT_UNSORTED_RANGE_EQ(std::ranges::forward_range auto const & actual, std::ranges::forward_range auto const & expected, auto const & eq) {
    EXPECT_TRUE(__impl::unsortedRangeEqual(actual, expected, eq)) << __impl::trace();
}

static void EXPECT_SORTED_RANGE_EQ(std::ranges::forward_range auto const & actual, std::ranges::forward_range auto const & expected) {
    EXPECT_TRUE(__impl::sortedRangeEqual(actual, expected)) << __impl::trace();
}

static void EXPECT_SORTED_RANGE_EQ(std::ranges::forward_range auto const & actual, std::ranges::forward_range auto const & expected, auto const & eq) {
    EXPECT_TRUE(__impl::sortedRangeEqual(actual, expected, eq)) << __impl::trace();
}
    
static void EXPECT_SIZE(std::ranges::range auto && range, size_t expectedSize) {
    EXPECT_EQ(fishnet::util::size(range),expectedSize) << __impl::messageWithTrace("Expected size of range: ", expectedSize, " but was: ", fishnet::util::size(range));
}

static void EXPECT_EMPTY(std::ranges::range auto && range) {
    EXPECT_EQ(fishnet::util::size(range), 0) << __impl::messageWithTrace("Expected range to be empty but was: ", __impl::printContainer(range));
}

template<typename ExpectedType>
static void EXPECT_TYPE(const auto & value){
    if constexpr (not std::is_same_v<std::remove_cvref_t<decltype(value)>, ExpectedType>){
        constexpr std::string_view expectedTypeName = __impl::get_type_name<ExpectedType>();
        constexpr std::string_view actualTypeName = __impl::get_type_name<std::remove_cvref_t<decltype(value)>>();
        FAIL() << __impl::messageWithTrace("Expected type: '", expectedTypeName, "' but was: '", actualTypeName, "'");
    }else{
        SUCCEED();
    }
}

static void EXPECT_VALUE(const __impl::OptionalOrExpected auto & wrapper, const typename std::remove_cvref_t<decltype(wrapper)>::value_type & expectedValue){
    EXPECT_TRUE(wrapper.has_value()) << __impl::messageWithTrace("Expected wrapper to have value: '", expectedValue, "' but was empty");
    if(wrapper.has_value()){
        if(wrapper.value() == expectedValue){
            SUCCEED();
        }else{
            FAIL() << __impl::messageWithTrace("Wrapper has value: '", wrapper.value(), "' but was expecting: '", expectedValue, "'");
        }
    }
}

static void EXPECT_VALUE(const __impl::OptionalOrExpected auto & wrapper){
    EXPECT_TRUE(wrapper.has_value()) << __impl::messageWithTrace("Expected wrapper to have value but was empty");
    if(wrapper.has_value()){
        SUCCEED();
    }
}

static void ASSERT_VALUE(const __impl::OptionalOrExpected auto & wrapper) {
    ASSERT_TRUE(wrapper.has_value()) << __impl::messageWithTrace("Expected wrapper to have value but was empty");  
}

static void EXPECT_EMPTY(const __impl::OptionalOrExpected auto & wrapper){
    if(wrapper.has_value()){
        FAIL() << __impl::messageWithTrace("Expected wrapper to be empty but was: '", wrapper.value(), "'");
    }else{
        SUCCEED();
    }
}

static void EXPECT_EXISTS(const std::filesystem::path & path){
    EXPECT_TRUE(std::filesystem::exists(path)) << __impl::messageWithTrace("Expecting path ", path, " to exist");
}

static void EXPECT_NOT_EXISTS(const std::filesystem::path & path){
    EXPECT_FALSE(std::filesystem::exists(path)) << __impl::messageWithTrace("Expecting path ", path, " to not exist");
}

void EXPECT_IN_RANGE(fishnet::math::Number auto value, fishnet::math::Number auto lower, fishnet::math::Number auto upper){
    EXPECT_TRUE(value >= lower) << __impl::messageWithTrace("Expected value ", value, " to be greater than or equal to lower bound ", lower);
    EXPECT_TRUE(value < upper) << __impl::messageWithTrace("Expected value ", value, " to be less than upper bound ", upper);
}
} // namespace fishnet::test
