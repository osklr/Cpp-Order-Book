#pragma once
#include <atomic>
#include <cstdint>
#include <map>
#include <list>
#include <unordered_map>
#include "order.hpp"

class OrderBook {
    public:
        using Price = int64_t;
        using OrderId = uint64_t;
        using Quantity = int;

    private:
        std::atomic<OrderId> order_id_counter{0};
        std::map<Price, std::list<Order>, std::greater<Price>> bid_book;
        std::map<Price, std::list<Order>> ask_book;
        std::unordered_map<OrderId, std::list<Order>::iterator> search_book;

    public:
        OrderBook() {

        }
        OrderId create_order(OrderType type, Price p, Quantity q);
        void cancel_order(OrderId id);
        void modify_order(OrderId id);
        Price get_best_bid() const;
        Price get_best_ask() const;
        Price get_spread() const;
};