#include <gtest/gtest.h>
#include "../order_book.hpp"
#include <stdexcept>

// Test for buy and rest the order on the empty book
TEST(matching, buy_and_rest_on_empty_book) {
    OrderBook book;
    const auto id = book.submit_order(OrderSide::Buy, OrderType::Limit, TimeInForce::GTC, 100, 10);
    // Expect the best bid is 100 in price
    EXPECT_EQ(book.get_best_bid(), 100);
    const Order o = book.search_order_in_order_journal(id);
    // Expect the status of the order is new and the remaining quantity is 10 which is an unsigned int
    EXPECT_EQ(o.get_status(), Status::New);
    EXPECT_EQ(o.get_remaining_quantity(), 10u);
}

// Test for sell and rest the order on the empty book
TEST(matching, sell_and_rest_on_empty_book) {
    OrderBook book;
    const auto id = book.submit_order(OrderSide::Sell, OrderType::Limit, TimeInForce::GTC, 100, 10);
    // Expect the best ask is 100 in price
    EXPECT_EQ(book.get_best_ask(), 100);
    const Order o = book.search_order_in_order_journal(id);
    // Expect the status of the order is new and the remaining quantity is 10 which is an unsigned int
    EXPECT_EQ(o.get_status(), Status::New);
    EXPECT_EQ(o.get_remaining_quantity(), 10u);
}

// Test for a full fill of a maker for first submit and sell order and then a buy order
TEST(matching, full_fill_one_maker_sell_and_buy) {
    OrderBook book;
    // Submit an sell order to create a maker order
    const auto sell_id = book.submit_order(OrderSide::Sell, OrderType::Limit, TimeInForce::GTC, 100, 10);
    // Submit an buy order to fill it with the maker sell order
    const auto buy_id = book.submit_order(OrderSide::Buy, OrderType::Limit, TimeInForce::GTC, 100, 10);
    // Get a reference to an object of storing the maker sell order record
    const Order& sell = book.search_order_in_order_journal(sell_id);
    // Get a reference to an object of storing the buy taker order record
    const Order& buy = book.search_order_in_order_journal(buy_id);
    // Expectations
    EXPECT_EQ(sell.get_status(), Status::Filled);
    EXPECT_EQ(buy.get_status(), Status::Filled);
    EXPECT_EQ(sell.get_remaining_quantity(), 0u);
    EXPECT_EQ(buy.get_remaining_quantity(), 0u);

    // Expected to throw std::out_of_range with search the order book with id because the orders are already filled
    EXPECT_THROW(book.search_order_book(sell_id), std::out_of_range);
    EXPECT_THROW(book.search_order_book(buy_id), std::out_of_range);
}

// Test for partial fill with a smaller quantity of the taker buy order than the maker sell order
TEST(matching, partial_fill_with_smaller_quantity_of_taker) {
    OrderBook book;
    const auto sell_id = book.submit_order(OrderSide::Sell, OrderType::Limit, TimeInForce::GTC, 100, 10);
    const auto buy_id = book.submit_order(OrderSide::Buy, OrderType::Limit, TimeInForce::GTC, 100, 4);
    const Order& sell = book.search_order_in_order_journal(sell_id);
    const Order& buy = book.search_order_in_order_journal(buy_id);
    // Expectations
    EXPECT_EQ(sell.get_status(), Status::PartiallyFilled);
    EXPECT_EQ(buy.get_status(), Status::Filled);
    EXPECT_EQ(sell.get_remaining_quantity(), 6u);
    EXPECT_EQ(buy.get_remaining_quantity(), 0u);

    // Maker should be still resting on the order book
    EXPECT_EQ(book.search_order_book(sell_id).get_remaining_quantity(), 6u);

    // Taker should be already filled and not found in the order book
    EXPECT_THROW(book.search_order_book(buy_id), std::out_of_range);
}

// Test for partial fill with a larger quantity of the taker buy order than the maker sell order
TEST(matching, partial_fill_with_larger_quantity_of_taker) {
    OrderBook book;
    const auto sell_id = book.submit_order(OrderSide::Sell, OrderType::Limit, TimeInForce::GTC, 100, 4);
    const auto buy_id = book.submit_order(OrderSide::Buy, OrderType::Limit, TimeInForce::GTC, 100, 10);
    const Order& sell = book.search_order_in_order_journal(sell_id);
    const Order& buy = book.search_order_in_order_journal(buy_id);
    // Expectations
    EXPECT_EQ(sell.get_status(), Status::Filled);
    EXPECT_EQ(buy.get_status(), Status::PartiallyFilled);
    EXPECT_EQ(sell.get_remaining_quantity(), 0u);
    EXPECT_EQ(buy.get_remaining_quantity(), 6u);

    // The original taker buy order should be placed on the order book after the filling of the maker sell order
    EXPECT_EQ(book.search_order_book(buy_id).get_remaining_quantity(), 6u);

    // Maker should be already filled and not found in the order book
    EXPECT_THROW(book.search_order_book(sell_id), std::out_of_range);
}

// Test for the FIFO ordering by using a taker order if maker sell orders are in same price
TEST(matching, fifo_same_price) {
    OrderBook book;
    // Submit two maker sell orders with a quantity of 5 and a taker buy order with a quantity of 5, the taker should only fill the first maker order
    const auto first_sell_id = book.submit_order(OrderSide::Sell, OrderType::Limit, TimeInForce::GTC, 100, 5);
    const auto second_sell_id = book.submit_order(OrderSide::Sell, OrderType::Limit, TimeInForce::GTC, 100, 5);
    const auto buy_id = book.submit_order(OrderSide::Buy, OrderType::Limit, TimeInForce::GTC, 100, 5);
    const Order& first_sell = book.search_order_in_order_journal(first_sell_id);
    const Order& second_sell = book.search_order_in_order_journal(second_sell_id);
    const Order& buy = book.search_order_in_order_journal(buy_id);

    // Expectations
    EXPECT_EQ(first_sell.get_status(), Status::Filled);
    EXPECT_EQ(second_sell.get_status(), Status::New);
    EXPECT_EQ(buy.get_status(), Status::Filled);

    EXPECT_EQ(first_sell.get_remaining_quantity(), 0u);
    EXPECT_EQ(second_sell.get_remaining_quantity(), 5u);
    EXPECT_EQ(buy.get_remaining_quantity(), 0u);

    EXPECT_THROW(book.search_order_book(first_sell_id), std::out_of_range);
    EXPECT_EQ(book.search_order_book(second_sell_id).get_remaining_quantity(), 5u);
    EXPECT_THROW(book.search_order_book(buy_id), std::out_of_range);
}

// Test for cancellation of a buy order resting on the book
TEST(matching, cancel_resting_buy) {
    OrderBook book;
    // Submit buy order and cancel it
    const auto buy_id = book.submit_order(OrderSide::Buy, OrderType::Limit, TimeInForce::GTC, 100, 10);
    book.cancel_order(buy_id);

    const Order& buy = book.search_order_in_order_journal(buy_id);

    EXPECT_EQ(buy.get_status(), Status::Canceled);
    EXPECT_EQ(buy.get_remaining_quantity(), 10u);

    EXPECT_THROW(book.search_order_book(buy_id), std::out_of_range);
    EXPECT_THROW(book.get_best_bid(), std::out_of_range);
}

// Test for resting both buy and sell order if their prices don't cross, both orders should not be filled
TEST(matching, no_cross_both_rest) {
    OrderBook book;
    // Submit a buy order at 100 in price and a sell order at 105 in price
    const auto buy_id = book.submit_order(OrderSide::Buy, OrderType::Limit, TimeInForce::GTC, 100, 10);
    const auto sell_id = book.submit_order(OrderSide::Sell, OrderType::Limit, TimeInForce::GTC, 105, 10);

    const Order& sell = book.search_order_in_order_journal(sell_id);
    const Order& buy = book.search_order_in_order_journal(buy_id);

    EXPECT_EQ(sell.get_status(), Status::New);
    EXPECT_EQ(buy.get_status(), Status::New);
    EXPECT_EQ(sell.get_remaining_quantity(), 10u);
    EXPECT_EQ(buy.get_remaining_quantity(), 10u);

    EXPECT_EQ(book.get_best_ask(), 105);
    EXPECT_EQ(book.get_best_bid(), 100);
    EXPECT_EQ(book.get_spread(), 5);

    EXPECT_EQ(book.search_order_book(sell_id).get_remaining_quantity(), 10u);
    EXPECT_EQ(book.search_order_book(buy_id).get_remaining_quantity(), 10u);
}

// Test for filling a buy order with the best ask first even its price crosses the best ask, after that then fill with the next best ask
TEST(matching, cross_best_ask_but_fill_with_best_ask_first) {
    OrderBook book;
    // Submit two sell order with one with price 101 and one with 100, then submit a buy order at 101 to fill both order
    const auto sell_id_101 = book.submit_order(OrderSide::Sell, OrderType::Limit, TimeInForce::GTC, 101, 5);
    const auto sell_id_100 = book.submit_order(OrderSide::Sell, OrderType::Limit, TimeInForce::GTC, 100, 5);
    const auto buy_id = book.submit_order(OrderSide::Buy, OrderType::Limit, TimeInForce::GTC, 101, 10);

    const Order& sell_101 = book.search_order_in_order_journal(sell_id_101);
    const Order& sell_100 = book.search_order_in_order_journal(sell_id_100);
    const Order& buy = book.search_order_in_order_journal(buy_id);

    EXPECT_EQ(sell_100.get_status(), Status::Filled);
    EXPECT_EQ(sell_101.get_status(), Status::Filled);
    EXPECT_EQ(buy.get_status(), Status::Filled);

    EXPECT_EQ(sell_100.get_remaining_quantity(), 0u);
    EXPECT_EQ(sell_101.get_remaining_quantity(), 0u);
    EXPECT_EQ(buy.get_remaining_quantity(), 0u);

    EXPECT_THROW(book.search_order_book(sell_id_100), std::out_of_range);
    EXPECT_THROW(book.search_order_book(sell_id_101), std::out_of_range);
    EXPECT_THROW(book.search_order_book(buy_id), std::out_of_range);
}