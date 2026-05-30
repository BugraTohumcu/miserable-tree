// tests/test_bplustree.cpp
#include "../src/index/BPlusTree.h"
#include <iostream>
#include <cassert>
#include <chrono>

static int passed = 0;
static int failed = 0;

#define TEST(name) void name()
#define RUN(name) \
    try { \
        auto _start = std::chrono::high_resolution_clock::now(); \
        name(); \
        auto _end = std::chrono::high_resolution_clock::now(); \
        auto _us = std::chrono::duration_cast<std::chrono::microseconds>(_end - _start).count(); \
        std::cout << "[PASS] " #name " (" << _us << " us)\n"; \
        passed++; \
    } \
    catch (const std::exception& e) { std::cout << "[FAIL] " #name " -> " << e.what() << "\n"; failed++; } \
    catch (...) { std::cout << "[FAIL] " #name " -> unknown exception\n"; failed++; }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) throw std::runtime_error("Expected " #a " == " #b)
#define ASSERT_TRUE(expr) \
    if (!(expr)) throw std::runtime_error("Expected true: " #expr)
#define ASSERT_FALSE(expr) \
    if ((expr)) throw std::runtime_error("Expected false: " #expr)
// ── tests ──────────────────────────────────────────────────────────────────

TEST(test_insert_and_search) {
    mislib::BPlusTree tree;
    ASSERT_TRUE(tree.insert(10, 100));
    ASSERT_TRUE(tree.insert(20, 200));
    ASSERT_TRUE(tree.insert(30, 300));

    ASSERT_EQ(tree.search(10).value(), 100);
    ASSERT_EQ(tree.search(20).value(), 200);
    ASSERT_EQ(tree.search(30).value(), 300);
}

TEST(test_duplicate_insert) {
    mislib::BPlusTree tree;
    ASSERT_TRUE(tree.insert(5, 50));
    ASSERT_TRUE(tree.insert(5, 99));   // SUCCESS: Multi-map allows duplicates now
    
    // Use searchAll to verify both values exist for key 5
    auto results = tree.searchAll(5);
    ASSERT_EQ(results.size(), 2);
}

TEST(test_search_missing_key) {
    mislib::BPlusTree tree;
    tree.insert(1, 10);
    ASSERT_FALSE(tree.search(99).has_value());
}

TEST(test_remove_existing) {
    mislib::BPlusTree tree;
    tree.insert(7, 70);
    ASSERT_TRUE(tree.remove(7));
    ASSERT_FALSE(tree.search(7).has_value());
}

TEST(test_remove_nonexistent) {
    mislib::BPlusTree tree;
    ASSERT_FALSE(tree.remove(42));
}

TEST(test_split_trigger) {
    // Insert enough keys to force at least one leaf split
    mislib::BPlusTree tree;
    for (size_t i = 1; i <= 20; i++) {
        tree.insert(i, i * 10);
    }
    for (size_t i = 1; i <= 20; i++) {
        ASSERT_EQ(tree.search(i).value(), i * 10);
    }
}

TEST(test_remove_triggers_underflow_borrow) {
    mislib::BPlusTree tree;
    for (size_t i = 1; i <= 10; i++) tree.insert(i, i * 10);

    // Remove until we force a borrow from sibling
    tree.remove(1);
    tree.remove(2);

    ASSERT_FALSE(tree.search(1).has_value());
    ASSERT_FALSE(tree.search(2).has_value());
    ASSERT_TRUE(tree.search(3).has_value());
}

TEST(test_remove_triggers_merge) {
    mislib::BPlusTree tree;
    for (size_t i = 1; i <= 6; i++) tree.insert(i, i * 10);

    // Drain enough to force a merge
    for (size_t i = 1; i <= 4; i++) tree.remove(i);

    ASSERT_FALSE(tree.search(1).has_value());
    ASSERT_TRUE(tree.search(5).has_value());
    ASSERT_TRUE(tree.search(6).has_value());
}

TEST(test_stress_insert_search) {
    mislib::BPlusTree tree;
    const size_t N = 100000;

    for (size_t i = 1; i <= N; i++) {
        tree.insert(i, i * 10);
    }
    for (size_t i = 1; i <= N; i++) {
        auto results = tree.searchAll(i);
        ASSERT_FALSE(results.empty());       // Ensure we found the key
        ASSERT_EQ(results.front(), i * 10);  // Verify the offset
    }
}

TEST(test_stress_remove) {
    mislib::BPlusTree tree;
    const size_t N = 100000;

    for (size_t i = 1; i <= N; i++) tree.insert(i, i * 10);
    for (size_t i = 1; i <= N; i++) ASSERT_TRUE(tree.remove(i));

    // Tree should be empty now
    for (size_t i = 1; i <= N; i++) ASSERT_FALSE(tree.search(i).has_value());
}

TEST(test_stress_random_order) {
    mislib::BPlusTree tree;
    const size_t N = 100000;

    // Insert in reverse to stress the split logic
    for (size_t i = N; i >= 1; i--) tree.insert(i, i * 10);
    
    for (size_t i = 1; i <= N; i++) {
        auto results = tree.searchAll(i);
        ASSERT_FALSE(results.empty());
        ASSERT_EQ(results.front(), i * 10);
    }
}

TEST(test_stress_remove_find_breaking_point) {
    mislib::BPlusTree tree;
    const size_t N = 100000;

    for (size_t i = 1; i <= N; i++) tree.insert(i, i * 10);

    size_t removeCount = 0;
    for (size_t i = 1; i <= N; i++) {
        if (!tree.remove(i)) {
            std::cout << "  Failed at i=" << i << " after " << removeCount << " successful removes\n";
            break;
        }
        removeCount++;
    }
}

// ── entry point ────────────────────────────────────────────────────────────

int main() {
    auto total_start = std::chrono::high_resolution_clock::now();

    RUN(test_insert_and_search);
    RUN(test_duplicate_insert);
    RUN(test_search_missing_key);
    RUN(test_remove_existing);
    RUN(test_remove_nonexistent);
    RUN(test_split_trigger);
    RUN(test_remove_triggers_underflow_borrow);
    RUN(test_remove_triggers_merge);
    RUN(test_stress_insert_search);
    RUN(test_stress_remove);
    RUN(test_stress_random_order);
    RUN(test_stress_remove_find_breaking_point);


    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start).count();

    std::cout << "\n" << passed << " passed, " << failed << " failed. Total: " << total_ms << " ms\n";
    return failed > 0 ? 1 : 0;
}