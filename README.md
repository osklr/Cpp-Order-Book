# Cpp-Order-Book

## Introduction
An order book and matching engine written with C++20 with a CLI developed for learning purposes. Currently including functionalities of creating orders, resting orders on the bid/ask book, FIFO matching, order and trade journals, cancel outstanding orders and search orders and trades.

## Features

### Implemented
- **Bid and ask books**: order books with O(log n) insertion and placing time complexity with FIFO mechanism.
- **Order Search in O(1)**: functionalities including order search, trade search, order modification and order cancellation are running in O(1) with std::unordered_map.
- **GTC limit order**: Currently, the book is limited to limit order with GTC time in force with both buy and sell sides.
- **Partial and full fills**: The book records remaining quantity for partial filled orders.
- **Journals**: Record all orders and trades.
- **CLI**: Terminal CLI is implemented for user action.

### In progress
- Market, Stop, Stop-Limit order types.
- GFD, IOC, FOK time in force settings.
- Automatic trade listing.

## Current Architecture

### User-defined Classes

- `Order`: Class for order.
- `OrderBook`: Class for the matching engine.
- `OrderJournal`: Class for the order journal.
- `TradeJournal`: Class for the trade journal.
- `Cli`: Class for the CLI interface.

### Standard Classes

Books classes uses a ordered map in order to find the best bid or ask price at a O(1) time complexity, and for a FIFO inserting in O(log n) time complexity in searching price in the map. A list in each price level is used when placing the order into the book.

- `std::map<Price, std::list<Order>, std::greater<Price>>`: Class for the bid book, ordering the highest price at the beginning.
- `std::map<Price, std::list<Order>>`: Class for the ask book.

Searching functionality use a unordered map to match each order or trade ID for searching them in O(1).

### Data flow

1. Call submit_order (create_order + match_order) to create and match with the existing orders in books
2. Record the order in the order_journal, then match the order.
3. Resting the order on the books if a full fill did not occur.
4. When a fill occurs towards the order, a trade is recorded in trade_journal.

## Requirements

- **CMake**: 3.15+
- **C++20 Compiler**
- **Google Test**: For automated testing.

## Build

```bash
cd Cpp-Order-Book
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Run

```bash
cd build
./order_book
```

## Test

Install Google Test before testing:
```bash
brew install googletest
```

Testing:
```bash
cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

## Disclaimer

This project is for educational purposes only. It is not production trading infrastructure and comes with no warranty. Do not use it for real financial decisions.

## License

Copyright (c) 2026 osklr

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.