#pragma once
#include "order.hpp"
#include <vector>
#include <unordered_map>

class OrderJournal {
    public:
        using Price = Order::Price;
        using OrderId = Order::OrderId;
        using Quantity = Order::Quantity;
        using VectorIndex = size_t;
    
    private:
        // Vector is used for storing orders, unordered_map is for searching in O(1)
        std::vector<Order> order_journal;
        std::unordered_map<OrderId, VectorIndex> order_search_book;

    public:
        // Functions concerning the order journel
        void create_order_record(const Order& order);
        void modify_order_record(const Order& order);
        const Order& search_order_record(const OrderId& id) const;
};