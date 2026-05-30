#ifndef B_PLUS_TREE_UTILS_HPP
#define B_PLUS_TREE_UTILS_HPP

#include <algorithm> 
#include <vector>
#include <utility>
#include <string> // STRING KUTUPHANESI EKLENDI

namespace mislib
{
    template<typename T>
    static auto IdSearch(const std::vector<T> &elements, const T& expected) {
        auto it = std::lower_bound(elements.begin(), elements.end(), expected);
        size_t index = static_cast<size_t>(std::distance(elements.begin(), it));
        return std::make_pair(index,it);
    }

    /**
     * @brief DJB2 Hash algorithm to safely convert strings into size_t numbers.
     * Extremely fast and provides excellent distribution to prevent collisions.
     */
    static size_t HashString(const std::string& str) {
        size_t hash = 5381;
        for (char c : str) {
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }
        return hash;
    }
} // namespace mislib

#endif