#pragma once
#include <map>
#include <list>
#include <unordered_map>
#include <functional>
#include "order_journal.hpp"

class OrderBook {
    public:
        using Price = Order::Price;
        using OrderId = Order::OrderId;
        using Quantity = Order::Quantity;

    private:
        // Order ID counter
        OrderId order_id_counter{0};

        /* Two maps for storing pairs with first element being the price and the second one being a list storing orders for running the order book,
        maps could access the first element with the ordering of the maps (bid_book decreasing, ask_book increasing) for getting the best price immediately in O(1),
        having structure of map with std::list<Order> is for inserting in a O(log n) time complexity with a particular price with a FIFO order. The unordered map is for searching orders in the order book in average O(1). */
        std::map<Price, std::list<Order>, std::greater<Price>> bid_book;
        std::map<Price, std::list<Order>> ask_book;
        std::unordered_map<OrderId, std::list<Order>::iterator> order_book_search_map;

    public:
        OrderBook();

        Price get_best_bid() const;
        Price get_best_ask() const;
        Price get_spread() const;

        OrderId create_order(OrderSide side, OrderType type, TimeInForce t_in_force, Price p, Quantity q);
        void place_order(OrderId id);
        void cancel_order(OrderId id);
        void modify_order(OrderId id, OrderSide side, OrderType type, TimeInForce t_in_force, Price p, Quantity q);
        void match_order(Order order);
        Order search_order_book(OrderId id) const;
};