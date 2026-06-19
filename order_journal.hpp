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
        using Time = Order::Time;
    
    private:
        // Vector is used for storing orders, unordered_map is for searching in O(1)
        std::vector<Order> order_journal;
        std::unordered_map<OrderId, VectorIndex> order_journal_search_book;

    public:
        // Functions concerning the order journel
        void create_order_record(OrderId id, OrderSide side, OrderType type, TimeInForce t_in_force, Price p, Quantity q, Time current_t);
        const Order& search_order_record(const OrderId& id) const;

        void set_canceled_time_in_order_journal(OrderId id, Time canceled_t);
        void set_completed_time_in_order_journal(OrderId id, Time completed_t);
        void set_status_in_order_journal(OrderId id, Status s);
        void add_filled_quantity_in_order_journal(OrderId id, Quantity q);
};