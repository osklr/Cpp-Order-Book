#include "order_book.hpp"
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <limits>
#include <string>

OrderBook::Price OrderBook::get_best_bid() const {
    if (bid_book.empty()) {
        throw std::out_of_range("Bid book is empty.");
    }
    return bid_book.begin()->first;
}

OrderBook::Price OrderBook::get_best_ask() const {
    if (ask_book.empty()) {
        throw std::out_of_range("Ask book is empty.");
    }
    return ask_book.begin()->first;
}

OrderBook::Price OrderBook::get_spread() const {
    if (bid_book.empty() || ask_book.empty()) {
        return 0;
    }
    return get_best_ask() - get_best_bid();
}

std::list<Order>& OrderBook::get_list_with_price_level(Price p, OrderSide s) {
    if (s == OrderSide::Buy) {
        std::map<Price, std::list<Order>, std::greater<Price>>::iterator current_iterator = bid_book.find(p);
        if (current_iterator == bid_book.end()) {
            throw std::out_of_range("The list of this price level is not found.");
        }
        return current_iterator->second;
    }
    else {
        std::map<Price, std::list<Order>>::iterator current_iterator = ask_book.find(p);
        if (current_iterator == ask_book.end()) {
            throw std::out_of_range("The list of this price level is not found.");
        }
        return current_iterator->second;
    }
}

Order OrderBook::create_order(OrderSide side, OrderType type, TimeInForce t_in_force, Price p, Quantity q) {
    OrderId current_order_id = ++order_id_counter;
    Order::Time current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    order_journal.create_order_record(current_order_id, side, type, t_in_force, p, q, current_time);
    Order current_order(current_order_id, side, type, t_in_force, p, q, current_time);
    return current_order;
}

void OrderBook::place_order(Order& order) {
    if (order.get_order_side() == OrderSide::Buy) {
        auto [price_n_list, list_created] = bid_book.try_emplace(order.get_price());
        auto& current_list = price_n_list->second;
        current_list.emplace_back(order);
        order_book_search_map[order.get_order_id()] = std::prev(current_list.end());
    }
    else {
        auto [price_n_list, list_created] = ask_book.try_emplace(order.get_price());
        auto& current_list = price_n_list->second;
        current_list.emplace_back(order);
        order_book_search_map[order.get_order_id()] = std::prev(current_list.end());
    }
}

void OrderBook::cancel_order(OrderId id) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    Order& current_order = *(current_iterator->second);
    // If the order is already completely filled or canceled, do nothing
    if (current_order.get_status() == Status::Filled || current_order.get_status() == Status::Canceled) {
        return;
    }
    // Set status of the order to canceled in the order book and the order journal
    current_order.set_status(Status::Canceled);
    const OrderId current_order_id = current_order.get_order_id();
    order_journal.set_status_in_order_journal(current_order_id, Status::Canceled);
    // Set canceled time of the order in the order book and order journal
    Order::Time current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    current_order.set_canceled_time(current_time);
    order_journal.set_canceled_time_in_order_journal(current_order_id, current_time);
    // Remove order from a book depending on the order side
    if (current_order.get_order_side() == OrderSide::Buy) {
        const Price current_price = current_order.get_price();
        // Remove order
        remove_order_from_order_book(current_order_id);
        // Remove record of order from the order book search map
        remove_record_from_order_book_search_map(current_order_id);
        std::map<Price, std::list<Order>, std::greater<Price>>::iterator current_map_iterator = bid_book.find(current_price);
        // If list at that price is empty on the bid book after the cancellation, remove the list of that price level on the bid book
        remove_list_if_no_order(bid_book, current_map_iterator);
    }
    else {
        const Price current_price = current_order.get_price();
        // Remove order
        remove_order_from_order_book(current_order_id);
        // Remove record of order from the order book search map
        remove_record_from_order_book_search_map(current_order_id);
        std::map<Price, std::list<Order>>::iterator current_map_iterator = ask_book.find(current_price);
        // If list at that price is empty on the ask book after the cancellation, remove the list of that price level on the ask book
        remove_list_if_no_order(ask_book, current_map_iterator);
    }
}

const Order OrderBook::search_order_book(OrderId id) const {
    std::unordered_map<OrderId, std::list<Order>::iterator>::const_iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        throw std::out_of_range("The order is not found.");
    }
    return *(current_iterator->second);
}

const Order OrderBook::search_order_in_order_journal(OrderId id) const {
    return order_journal.search_order_record(id);
}

void OrderBook::match_order(Order order) {
    // Determine its order type
    if (order.get_order_type() == OrderType::Stop || order.get_order_type() == OrderType::StopLimit) {
        // Handle if stop or stop-limit type
        handle_stop_order(order);
    }
    else if (order.get_order_type() == OrderType::Limit || order.get_order_type() == OrderType::Market) {
        // Decide whether it is Buy or Sell if it is limit or market order type
        if (order.get_order_side() == OrderSide::Buy) {
            match_buy_order(order);
        }
        else {
            match_sell_order(order);
        }
    }
}

Order::OrderId OrderBook::submit_order(OrderSide side, OrderType type, TimeInForce t_in_force, Price p, Quantity q) {
    Order current_order = create_order(side, type, t_in_force, p, q);
    match_order(current_order);
    return current_order.get_order_id();
}

void OrderBook::set_order_status_from_order_book(OrderId id, Status s) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    std::list<Order>::iterator current_order_iterator = current_iterator->second;
    current_order_iterator->set_status(s);
    order_journal.set_status_in_order_journal(id, s);
}

void OrderBook::set_order_completed_time_from_order_book(OrderId id, Time t) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    std::list<Order>::iterator current_order_iterator = current_iterator->second;
    current_order_iterator->set_completed_time(t);
    order_journal.set_completed_time_in_order_journal(id, t);
}

void OrderBook::set_order_canceled_time_from_order_book(OrderId id, Time t) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    std::list<Order>::iterator current_order_iterator = current_iterator->second;
    current_order_iterator->set_canceled_time(t);
    order_journal.set_canceled_time_in_order_journal(id, t);
}

void OrderBook::subtract_order_remaining_quantity_from_order_book(OrderId id, Quantity q) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    std::list<Order>::iterator current_order_iterator = current_iterator->second;
    current_order_iterator->subtract_remaining_quantity(q);
    order_journal.subtract_remaining_quantity_in_order_journal(id, q);
}

void OrderBook::remove_record_from_order_book_search_map(OrderId id) {
    if (order_book_search_map.find(id) == order_book_search_map.end()) {
        return;
    }
    order_book_search_map.erase(id);
}

void OrderBook::remove_order_from_order_book(OrderId id) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    Order& current_order = *(current_iterator->second);
    std::list<Order>& current_list = get_list_with_price_level(current_order.get_price(), current_order.get_order_side());
    current_list.erase(current_iterator->second);
}

void OrderBook::match_buy_order(Order& order) {
    // Buy side, check time in force
    if (order.get_time_in_force() == TimeInForce::GTC) {
        // Check if the ask book is empty. If yes, direct emplace a new order in the bid book. If not, check if the order could be filled.
        if (ask_book.empty()) {
            place_order(order);
        }
        else {
            /* While the price of order is higher or equal to the best ask and the quantity of the order is larger 
            than the quantity of the first order in the queue at the best ask, continue to fill the order. */
            while (!ask_book.empty() && order.get_price() >= ask_book.begin()->first && order.get_remaining_quantity() >= ask_book.begin()->second.begin()->get_remaining_quantity()) {
                std::map<Price, std::list<Order>>::iterator current_iterator = ask_book.begin();
                std::list<Order>& current_list = current_iterator->second;
                Order& current_order = current_list.front();
                Quantity trade_quantity = current_order.get_remaining_quantity();
                Price current_price = current_iterator->first;
                // Create a new trade record in the trade journal
                Trade::TradeId current_trade_id = trade_journal.create_trade(current_order.get_order_id(), order.get_order_id(), current_price, trade_quantity);
                Time current_completed_time = trade_journal.get_trade_completed_time(current_trade_id);
                // Set completed time in both order book and order journal for maker order
                OrderId maker_order_id = current_order.get_order_id();
                set_order_completed_time_from_order_book(maker_order_id, current_completed_time);
                // Set status for the maker order
                set_order_status_from_order_book(maker_order_id, Status::Filled);
                // Subtract remaining quantity for both orders
                subtract_order_remaining_quantity_from_order_book(maker_order_id, trade_quantity);
                OrderId taker_order_id = order.get_order_id();
                order.subtract_remaining_quantity(trade_quantity);
                order_journal.subtract_remaining_quantity_in_order_journal(taker_order_id, trade_quantity);
                // Check if all quantity is filled for the taker order. If yes, set its status as Filled and set completed time. If not, set status as partially filled.
                if (order.get_remaining_quantity() == 0) {
                    order.set_status(Status::Filled);
                    order_journal.set_status_in_order_journal(taker_order_id, Status::Filled);
                    order.set_completed_time(current_completed_time);
                    order_journal.set_completed_time_in_order_journal(taker_order_id, current_completed_time);
                }
                else {
                    order.set_status(Status::PartiallyFilled);
                    order_journal.set_status_in_order_journal(taker_order_id, Status::PartiallyFilled);
                }
                // Remove the maker order from the order book
                current_list.pop_front();
                // Remove the maker order from the order book search map
                remove_record_from_order_book_search_map(current_order.get_order_id());
                // Remove the list at the trade price from the ask book if no order left at that price
                remove_list_if_no_order(ask_book, current_iterator);
            }
            if (!ask_book.empty() && order.get_price() >= ask_book.begin()->first && order.get_remaining_quantity() > 0 && order.get_remaining_quantity() < ask_book.begin()->second.begin()->get_remaining_quantity()) {
                // Fill the remaining quantity of the taker order
                std::map<Price, std::list<Order>>::iterator current_iterator = ask_book.begin();
                Order& current_order = current_iterator->second.front();
                Quantity trade_quantity = order.get_remaining_quantity();
                Price current_price = current_iterator->first;
                // Create a new trade record in the trade journal
                Trade::TradeId current_trade_id = trade_journal.create_trade(current_order.get_order_id(), order.get_order_id(), current_price, trade_quantity);
                Time current_completed_time = trade_journal.get_trade_completed_time(current_trade_id);
                // Set completed time in order journal for taker order
                OrderId taker_order_id = order.get_order_id();
                order.set_completed_time(current_completed_time);
                order_journal.set_completed_time_in_order_journal(taker_order_id, current_completed_time);
                // Subtract remaining quantity for both orders
                OrderId maker_order_id = current_order.get_order_id();
                subtract_order_remaining_quantity_from_order_book(maker_order_id, trade_quantity);
                order.subtract_remaining_quantity(trade_quantity);
                order_journal.subtract_remaining_quantity_in_order_journal(taker_order_id, trade_quantity);
                // Taker order is completely filled, set status to filled. Maker order set to partially filled
                set_order_status_from_order_book(maker_order_id, Status::PartiallyFilled);
                order.set_status(Status::Filled);
                order_journal.set_status_in_order_journal(taker_order_id, Status::Filled);
                return;
            }
            if (order.get_remaining_quantity() > 0) {
                place_order(order);
            }
        }
    }
    else {
        return;
    }
}

void OrderBook::match_sell_order(Order& order) {
    // Sell side, check time in force
    if (order.get_time_in_force() == TimeInForce::GTC) {
        // Check if the bid book is empty. If yes, direct emplace a new order in the ask book. If not, check if the order could be filled.
        if (bid_book.empty()) {
            place_order(order);
        }
        else {
            /* While the price of order is lower or equal to the best bid and the quantity of the order is larger 
            than the quantity of the first order in the queue at the best bid, continue to fill the order. */
            while (!bid_book.empty() && order.get_price() <= bid_book.begin()->first && order.get_remaining_quantity() >= bid_book.begin()->second.begin()->get_remaining_quantity()) {
                std::map<Price, std::list<Order>, std::greater<Price>>::iterator current_iterator = bid_book.begin();
                std::list<Order>& current_list = current_iterator->second;
                Order& current_order = current_list.front();
                Quantity trade_quantity = current_order.get_remaining_quantity();
                Price current_price = current_iterator->first;
                // Create a new trade record in the trade journal
                Trade::TradeId current_trade_id = trade_journal.create_trade(current_order.get_order_id(), order.get_order_id(), current_price, trade_quantity);
                Time current_completed_time = trade_journal.get_trade_completed_time(current_trade_id);
                // Set completed time in both order book and order journal for maker order
                OrderId maker_order_id = current_order.get_order_id();
                set_order_completed_time_from_order_book(maker_order_id, current_completed_time);
                // Set status for the maker order
                set_order_status_from_order_book(maker_order_id, Status::Filled);
                // Subtract remaining quantity for both orders
                subtract_order_remaining_quantity_from_order_book(maker_order_id, trade_quantity);
                OrderId taker_order_id = order.get_order_id();
                order.subtract_remaining_quantity(trade_quantity);
                order_journal.subtract_remaining_quantity_in_order_journal(taker_order_id, trade_quantity);
                // Check if all quantity is filled for the taker order. If yes, set its status as Filled and set completed time. If not, set status as partially filled.
                if (order.get_remaining_quantity() == 0) {
                    order.set_status(Status::Filled);
                    order_journal.set_status_in_order_journal(taker_order_id, Status::Filled);
                    order.set_completed_time(current_completed_time);
                    order_journal.set_completed_time_in_order_journal(taker_order_id, current_completed_time);
                }
                else {
                    order.set_status(Status::PartiallyFilled);
                    order_journal.set_status_in_order_journal(taker_order_id, Status::PartiallyFilled);
                }
                // Remove the maker order from the order book
                current_list.pop_front();
                // Remove the maker order from the order book search map
                remove_record_from_order_book_search_map(current_order.get_order_id());
                // Remove the list at the trade price from the bid book if no order left at that price
                remove_list_if_no_order(bid_book, current_iterator);
            }
            if (!bid_book.empty() && order.get_price() <= bid_book.begin()->first && order.get_remaining_quantity() > 0 && order.get_remaining_quantity() < bid_book.begin()->second.begin()->get_remaining_quantity()) {
                // Fill the remaining quantity of the taker order
                std::map<Price, std::list<Order>, std::greater<Price>>::iterator current_iterator = bid_book.begin();
                Order& current_order = current_iterator->second.front();
                Quantity trade_quantity = order.get_remaining_quantity();
                Price current_price = current_iterator->first;
                // Create a new trade record in the trade journal
                Trade::TradeId current_trade_id = trade_journal.create_trade(current_order.get_order_id(), order.get_order_id(), current_price, trade_quantity);
                Time current_completed_time = trade_journal.get_trade_completed_time(current_trade_id);
                // Set completed time in order journal for taker order
                OrderId taker_order_id = order.get_order_id();
                order.set_completed_time(current_completed_time);
                order_journal.set_completed_time_in_order_journal(taker_order_id, current_completed_time);
                // Subtract remaining quantity for both orders
                OrderId maker_order_id = current_order.get_order_id();
                subtract_order_remaining_quantity_from_order_book(maker_order_id, trade_quantity);
                order.subtract_remaining_quantity(trade_quantity);
                order_journal.subtract_remaining_quantity_in_order_journal(taker_order_id, trade_quantity);
                // Taker order is completely filled, set status to filled. Maker order set to partially filled
                set_order_status_from_order_book(maker_order_id, Status::PartiallyFilled);
                order.set_status(Status::Filled);
                order_journal.set_status_in_order_journal(taker_order_id, Status::Filled);
                return;
            }
            if (order.get_remaining_quantity() > 0) {
                place_order(order);
            }
        }
    }
    else {
        return;
    }
}

void OrderBook::handle_stop_order(Order& order) {
    return; // Deferred
}

void OrderBook::finalize_filled_order(Order& order) {
    return; // Deferred
}

void OrderBook::finalize_canceled_order(Order& order) {
    return; // Deferred
}

void OrderBook::print_book() const {
    std::cout << "======= Order Book =======\n";
    std::cout << "Asks:\n";
    if (ask_book.empty()) {
        std::cout << "No available sell order.\n";
    }
    else {
        std::cout << "Price | Quantity\n";
        for (const auto& [price, list] : ask_book) {
            Quantity current_quantity = 0;
            for (const Order& o : list) {
                current_quantity += o.get_remaining_quantity();
            }
            std::cout << price << " | " << current_quantity << "\n";
        }
    }
    std::cout << "========\n";
    std::cout << "Spread: " << get_spread() << "\n";
    std::cout << "========\n";
    std::cout << "Bids:\n";
    if (bid_book.empty()) {
        std::cout << "No available buy order.\n";
    }
    else {
        std::cout << "Price | Quantity\n";
        for (const auto& [price, list] : bid_book) {
            Quantity current_quantity = 0;
            for (const Order& o : list) {
                current_quantity += o.get_remaining_quantity();
            }
            std::cout << price << " | " << current_quantity << "\n";
        }
    }
    std::cout << "==========================\n";
}

void OrderBook::print_main_menu() const {
    std::cout << "1: Print Order Book\n";
    std::cout << "2: Create Order\n";
    std::cout << "3: Cancel Order\n";
    std::cout << "4: Search Outstanding Order\n";
    std::cout << "5: Search Order\n";
    std::cout << "6: Search Trade\n";
    std::cout << "7: Exit\n";
}

void OrderBook::print_order_id(const Order& order) const {
    std::cout << "The order id is " << order.get_order_id() << ".\n";
}

void OrderBook::print_trade_id(const Trade& trade) const {
    std::cout << "The trade id is " << trade.get_trade_id() << ".\n";
}

void OrderBook::print_order(const OrderId& id) const {
    const Order& order = search_order_in_order_journal(id);
    std::cout << "Order information:\n";
    std::cout << "Order ID: " << order.get_order_id() << "\n";
    std::cout << "Order Side: " << to_string(order.get_order_side()) << "\n";
    std::cout << "Order Type: " << to_string(order.get_order_type()) << "\n";
    std::cout << "Time in force: " << to_string(order.get_time_in_force()) << "\n";
    std::cout << "Price: " << order.get_price() << "\n";
    std::cout << "Quantity: " << order.get_quantity() << "\n";
    std::cout << "Remaining: " << order.get_remaining_quantity() << '\n';
    std::cout << "Status: " << to_string(order.get_status()) << "\n";
    std::cout << "Created time: " << order.get_created_time() << "\n";
    std::cout << "Completed time: " << order.get_completed_time() << "\n";
    std::cout << "Canceled time: " << order.get_canceled_time() << "\n"; 
}

void OrderBook::print_trade(const TradeId& id) const {
    const Trade& trade = trade_journal.search_trade(id);
    std::cout << "Trade information:\n";
    std::cout << "Trade ID: " << trade.get_trade_id() << "\n";
    std::cout << "Complete Time: " << trade.get_timestamp() << "\n";
    std::cout << "Maker Order ID: " << trade.get_maker_order_id() << "\n";
    std::cout << "Taker Order ID: " << trade.get_taker_order_id() << "\n";
    std::cout << "Price: " << trade.get_price() << "\n";
    std::cout << "Quantity: " << trade.get_quantity() << "\n";
}

void OrderBook::run_cli() {
    std::cout << "Welcome to C++ Order Book!\n";
    while (true) {
        print_main_menu();
        int choice;
        std::cout << "Choose option: ";
        std::cin >> choice;
        switch (choice) {
            case 1: {
                print_book();
                break;
            }
            case 2: {
                int temp;
                bool exit = false;
                OrderSide side;
                OrderType type;
                TimeInForce t_in_force;
                Price p;
                Quantity q;
                temp = choose_side();
                switch (temp) {
                    case 1:
                        side = OrderSide::Buy;
                        break;
                    case 2:
                        side = OrderSide::Sell;
                        break;
                    case 3:
                        exit = true;
                }
                if (exit) {
                    break;
                }
                temp = choose_type();
                switch (temp) {
                    case 1:
                        type = OrderType::Limit;
                        break;
                    case 2:
                        exit = true;
                }
                if (exit) {
                    break;
                }
                temp = choose_time_in_force();
                switch (temp) {
                    case 1:
                        t_in_force = TimeInForce::GTC;
                        break;
                    case 2:
                        exit = true;
                }
                if (exit) {
                    break;
                }
                std::pair<Price, bool> price_result = choose_price();
                if (price_result.second) {
                    break;
                }
                p = price_result.first;
                std::pair<Quantity, bool> quantity_result = choose_quantity();
                if (quantity_result.second) {
                    break;
                }
                q = quantity_result.first;
                OrderId current_order_id = submit_order(side, type, t_in_force, p, q);
                std::cout << "Order submitted.\n";
                print_order(current_order_id);
                break;
            }
            case 3: {
                OrderId order_id;
                while (true) {
                    std::cout << "Enter the order ID for cancellation: ";
                    std::cin >> order_id;
                    try {
                        search_order_book(order_id);
                        break;
                    }
                    catch (const std::out_of_range& e) {
                        std::cout << e.what() << "\n";
                    }
                }
                cancel_order(order_id);
                std::cout << "The order is canceled.\n";
                print_order(order_id);
                break;
            }
            case 4: {
                OrderId order_id;
                while (true) {
                    std::cout << "Enter the order ID for searching: ";
                    std::cin >> order_id;
                    try {
                        search_order_book(order_id);
                        print_order(order_id);
                        break;
                    }
                    catch (const std::out_of_range& e) {
                        std::cout << e.what() << "\n";
                    }
                }
                break;
            }
            case 5: {
                OrderId order_id;
                while (true) {
                    std::cout << "Enter the order ID for searching: ";
                    std::cin >> order_id;
                    try {
                        search_order_in_order_journal(order_id);
                        print_order(order_id);
                        break;
                    }
                    catch (const std::out_of_range& e) {
                        std::cout << e.what() << "\n";
                    }
                }
                break;
            }
            case 6: {
                TradeId trade_id;
                while (true) {
                    std::cout << "Enter the trade ID for searching: ";
                    std::cin >> trade_id;
                    try {
                        trade_journal.search_trade(trade_id);
                        print_trade(trade_id);
                        break;
                    }
                    catch (const std::out_of_range& e) {
                        std::cout << e.what() << "\n";
                    }
                }
                break;
            }
            case 7: {
                std::cout << "Goodbye!\n";
                return;
            }
            default: {
                std::cout << "Invalid option.\n"; 
            }
        }
    }
}

int OrderBook::choose_side() const {
    int temp;
    while (true) {
        std::cout << "1. Buy\n";
        std::cout << "2. Sell\n";
        std::cout << "3. Cancel and Back to main menu\n";
        std::cout << "Choose option: ";
        std::cin >> temp;
        switch (temp) {
            case 1:
                return temp;
            case 2:
                return temp;
            case 3: 
                return temp;
            default:
                std::cout << "Invalid Option.\n";
        }
    }
}

int OrderBook::choose_type() const {
    int temp;
    while (true) {
        std::cout << "1. Limit\n";
        std::cout << "2. Cancel and Back to main menu\n";
        std::cout << "Choose option: ";
        std::cin >> temp;
        switch (temp) {
            case 1:
                return temp;
            case 2:
                return temp;
            default:
                std::cout << "Invalid Option.\n";
        }
    }
}

int OrderBook::choose_time_in_force() const {
    int temp;
    while (true) {
        std::cout << "1. Good Till Canceled (GTC)\n";
        std::cout << "2. Cancel and Back to main menu\n";
        std::cout << "Choose option: ";
        std::cin >> temp;
        switch (temp) {
            case 1:
                return temp;
            case 2:
                return temp;
            default:
                std::cout << "Invalid Option.\n";
        }
    }
}

std::pair<Order::Price, bool> OrderBook::choose_price() const {
    std::string input;
    while (true) {
        std::cout << "Type in order price (Type in 'e' to Cancel and Back to the main menu): ";
        std::cin >> input;
        if (input == "e") {
            return {0, true};
        }
        try {
            long long temp = std::stoll(input);
            if (temp > std::numeric_limits<Price>::max() || temp < std::numeric_limits<Price>::min()) {
                std::cout << "Invalid price. The number is out of range.\n";
                continue;
            }
            return {static_cast<Price>(temp), false};
        }
        catch (const std::invalid_argument&) {
            std::cout << "Invalid price. Please type in a number or 'e'.\n";
            continue;
        }
        catch (const std::out_of_range&) {
            std::cout << "Invalid price. The number is out of range.\n";
            continue;
        }
    }
}

std::pair<Order::Quantity, bool> OrderBook::choose_quantity() const {
    std::string input;
    while (true) {
        std::cout << "Type in quantity (Type in 'e' to Cancel and Back to the main menu): ";
        std::cin >> input;
        if (input == "e") {
            return {0, true};
        }
        try {
            unsigned long temp = std::stoul(input);
            if (temp == 0) {
                std::cout << "Invalid quantity. The number must be greater than 0.\n";
                continue;
            }
            if (temp > std::numeric_limits<Quantity>::max()) {
                std::cout << "Invalid quantity. The number is out of range.\n";
                continue;
            }
            return {static_cast<Quantity>(temp), false};
        }
        catch (const std::invalid_argument&) {
            std::cout << "Invalid quantity. Please type in a number or 'e'.\n";
            continue;
        }
        catch (const std::out_of_range&) {
            std::cout << "Invalid quantity. The number is out of range.\n";
            continue;
        }
    }
}

const char* OrderBook::to_string(OrderSide s) {
    switch(s) {
        case OrderSide::Buy: {
            return "Buy";
        }
        case OrderSide::Sell: {
            return "Sell";
        }
    }
    throw std::logic_error("Invalid order side.");
}

const char* OrderBook::to_string(OrderType t) {
    switch(t) {
        case OrderType::Limit: {
            return "Limit";
        }
        case OrderType::Market: {
            return "Market";
        }
        case OrderType::Stop: {
            return "Stop";
        }
        case OrderType::StopLimit: {
            return "Stop Limit";
        }
    }
    throw std::logic_error("Invalid order type.");
}

const char* OrderBook::to_string(TimeInForce t_in_force) {
    switch(t_in_force) {
        case TimeInForce::GFD: {
            return "Good For Day (GFD)";
        }
        case TimeInForce::GTC: {
            return "Good Till Canceled (GTC)";
        }
        case TimeInForce::IOC: {
            return "Immediate Or Cancel (IOC)";
        }
        case TimeInForce::FOK: {
            return "Fill Or Kill (FOK)";
        }
    }
    throw std::logic_error("Invalid time in force.");
}

const char* OrderBook::to_string(Status s) {
    switch(s) {
        case Status::New: {
            return "New";
        }
        case Status::PartiallyFilled: {
            return "Partially Filled";
        }
        case Status::Filled: {
            return "Filled";
        }
        case Status::Canceled: {
            return "Canceled";
        }
        case Status::Rejected: {
            return "Rejected";
        }
    }
    throw std::logic_error("Invalid status.");
}