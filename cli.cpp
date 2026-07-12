#include "cli.hpp"
#include <iostream>
#include <string>
#include <limits>

void Cli::print_book() const {
    std::cout << "======= Order Book =======\n";
    std::cout << "Asks:\n";
    const auto& ask_book = cli_book.get_ask_book();
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
    std::cout << "Spread: " << cli_book.get_spread() << "\n";
    std::cout << "========\n";
    std::cout << "Bids:\n";
    const auto& bid_book = cli_book.get_bid_book();
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

void Cli::print_main_menu() const {
    std::cout << "1: Print Order Book\n";
    std::cout << "2: Create Order\n";
    std::cout << "3: Cancel Order\n";
    std::cout << "4: Search Outstanding Order\n";
    std::cout << "5: Search Order\n";
    std::cout << "6: Search Trade\n";
    std::cout << "7: Exit\n";
}

void Cli::print_order_id(const Order& order) const {
    std::cout << "The order id is " << order.get_order_id() << ".\n";
}

void Cli::print_trade_id(const Trade& trade) const {
    std::cout << "The trade id is " << trade.get_trade_id() << ".\n";
}

void Cli::print_order(const OrderId& id) const {
    const Order& order = cli_book.search_order_in_order_journal(id);
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

void Cli::print_trade(const TradeId& id) const {
    const Trade& trade = cli_book.search_trade_in_trade_journal(id);
    std::cout << "Trade information:\n";
    std::cout << "Trade ID: " << trade.get_trade_id() << "\n";
    std::cout << "Complete Time: " << trade.get_timestamp() << "\n";
    std::cout << "Maker Order ID: " << trade.get_maker_order_id() << "\n";
    std::cout << "Taker Order ID: " << trade.get_taker_order_id() << "\n";
    std::cout << "Price: " << trade.get_price() << "\n";
    std::cout << "Quantity: " << trade.get_quantity() << "\n";
}

const char* Cli::to_string(OrderSide s) {
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

const char* Cli::to_string(OrderType t) {
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

const char* Cli::to_string(TimeInForce t_in_force) {
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

const char* Cli::to_string(Status s) {
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

void Cli::run_cli() {
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
                OrderId current_order_id = cli_book.submit_order(side, type, t_in_force, p, q);
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
                        cli_book.search_order_book(order_id);
                        break;
                    }
                    catch (const std::out_of_range& e) {
                        std::cout << e.what() << "\n";
                    }
                }
                cli_book.cancel_order(order_id);
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
                        cli_book.search_order_book(order_id);
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
                        cli_book.search_order_in_order_journal(order_id);
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
                        cli_book.search_trade_in_trade_journal(trade_id);
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

int Cli::choose_side() const {
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

int Cli::choose_type() const {
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

int Cli::choose_time_in_force() const {
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

std::pair<Order::Price, bool> Cli::choose_price() const {
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

std::pair<Order::Quantity, bool> Cli::choose_quantity() const {
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
