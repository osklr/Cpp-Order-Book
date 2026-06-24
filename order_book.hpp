#pragma once
#include <map>
#include <list>
#include <unordered_map>
#include <functional>
#include "order_journal.hpp"
#include "trade_journal.hpp"

class OrderBook {
    public:
        using Price = Order::Price;
        using OrderId = Order::OrderId;
        using Quantity = Order::Quantity;
        using Time = Trade::Time;
        using TradeId = Trade::TradeId;

    private:
        // Order ID counter
        OrderId order_id_counter{0};

        /* Two maps for storing pairs with first element being the price and the second one being a list storing orders for running the order book,
        maps could access the first element with the ordering of the maps (bid_book decreasing, ask_book increasing) for getting the best price immediately in O(1),
        having structure of map with std::list<Order> is for inserting in a O(log n) time complexity with a particular price with a FIFO order. The unordered map is for searching orders in the order book in average O(1). */
        std::map<Price, std::list<Order>, std::greater<Price>> bid_book;
        std::map<Price, std::list<Order>> ask_book;
        std::unordered_map<OrderId, std::list<Order>::iterator> order_book_search_map;

        OrderJournal order_journal;
        TradeJournal trade_journal;

    public:
        OrderBook() = default;
        
        // Get Functions
        Price get_best_bid() const;
        Price get_best_ask() const;
        Price get_spread() const;
        std::list<Order>& get_list_with_price_level(Price p, OrderSide s);

        // Main Functions
        Order create_order(OrderSide side, OrderType type, TimeInForce t_in_force, Price p, Quantity q);
        void place_order(Order& order);
        void cancel_order(OrderId id);
        void match_order(Order order);

        OrderId submit_order(OrderSide side, OrderType type, TimeInForce t_in_force, Price p, Quantity q);
        const Order search_order_book(OrderId id) const;
        const Order search_order_in_order_journal(OrderId id) const;

        // Set Functions
        void set_order_status_from_order_book(OrderId id, Status s);
        void set_order_completed_time_from_order_book(OrderId id, Time t);
        void set_order_canceled_time_from_order_book(OrderId id, Time t);
        void subtract_order_remaining_quantity_from_order_book(OrderId id, Quantity q);

        // Remove Functions
        // For this function, make sure current_iterator won't be used again as the iterator is erased
        template <typename Book>
        void remove_list_if_no_order(Book& book, typename Book::iterator current_iterator) {
            if (current_iterator != book.end() && current_iterator->second.empty()) {
                book.erase(current_iterator);
            }
        }

        void remove_record_from_order_book_search_map(OrderId id);
        void remove_order_from_order_book(OrderId id);

        // Matching Functions
        void match_buy_order(Order& order);
        void match_sell_order(Order& order);
        void handle_stop_order(Order& order);
        void finalize_filled_order(Order& order);
        void finalize_canceled_order(Order& order);

        // Print Functions
        void print_book() const;
        void print_main_menu() const;
        void print_order_id(const Order& order) const;
        void print_trade_id(const Trade& trade) const;
        void print_order(const OrderId& id) const;
        void print_trade(const TradeId& id) const;

        // CLI
        void run_cli();
        int choose_side() const;
        int choose_type() const;
        int choose_time_in_force() const;
        std::pair<Price, bool> choose_price() const;
        std::pair<Quantity, bool> choose_quantity() const;

        // To string helper functions
        static const char* to_string(OrderSide s);
        static const char* to_string(OrderType t);
        static const char* to_string(TimeInForce t_in_force);
        static const char* to_string(Status s);
};