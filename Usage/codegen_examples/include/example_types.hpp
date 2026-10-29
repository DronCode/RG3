#pragma once

#include <string>
#include <vector>

/**
 * Example types for RG3 codegen integration testing.
 * 
 * This file contains sample C++ types that the codegen example
 * (serialize_codegen.py) will analyze and generate serialization
 * wrappers for.
 */

namespace example {

/// @runtime
/// @brief A simple user structure
struct User {
    int id;
    std::string name;
    std::string email;
    bool active;
};

/// @runtime
/// @brief A product with price and quantity
struct Product {
    int id;
    std::string title;
    double price;
    int quantity;
};

/// @runtime
/// @brief A complex nested structure
struct Order {
    int id;
    User customer;
    std::vector<Product> items;
    double total;
};

/// @runtime
/// @brief An enum for order status
enum class OrderStatus {
    Pending,
    Processing,
    Shipped,
    Delivered,
    Cancelled
};

} // namespace example
