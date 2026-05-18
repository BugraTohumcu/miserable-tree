#ifndef B_PLUS_TREE_UTILS_HPP
#define B_PLUS_TREE_UTILS_HPP


#include <algorithm> 
#include <vector>
#include <utility>

namespace mislib
{

    /**
     * Generic binary search helper to locate the lower bound index of a target.
     * Eliminates boilerplate std::lower_bound + std::distance calls across the tree.
     * 
     * Note: Returns elements.size() if the expected value is greater than all elements.
     */
    template<typename T>
    static auto IdSearch(const std::vector<T> &elements, const T& expected) {
        auto it = std::lower_bound(elements.begin(), elements.end(), expected);
        
        // Using size_t to prevent signed/unsigned mismatch warnings with vector::size()
        size_t index = static_cast<size_t>(std::distance(elements.begin(), it));
        return std::make_pair(index,it);
    }
} // namespace mislib


#endif
