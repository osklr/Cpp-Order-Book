#pragma once
#include "order_book.hpp"
#include <utility>

class Cli {
    private:
        OrderBook& cli_book;
    public:
        using Price = OrderBook::Price;
        using Quantity = OrderBook::Quantity;
        using OrderId = OrderBook::OrderId;
        using TradeId = OrderBook::TradeId;

        // Constructor
        Cli(OrderBook& book) : cli_book(book) {};

        // Print Functions
        void print_book() const;
        void print_main_menu() const;
        void print_order_id(const Order& order) const;
        void print_trade_id(const Trade& trade) const;
        void print_order(const OrderId& id) const;
        void print_trade(const TradeId& id) const;

        // To string helper functions
        static const char* to_string(OrderSide s);
        static const char* to_string(OrderType t);
        static const char* to_string(TimeInForce t_in_force);
        static const char* to_string(Status s);

        // CLI
        void run_cli();
        int choose_side() const;
        int choose_type() const;
        int choose_time_in_force() const;
        std::pair<Price, bool> choose_price() const;
        std::pair<Quantity, bool> choose_quantity() const;
};