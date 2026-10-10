# Cpp-Order-Book

## Introduction
An order book and matching engine written with C++20 with a CLI developed for learning purposes. Currently including functionalities of creating orders, resting orders on the bid/ask book, FIFO matching, GTC, IOC, and FOK time-in-force, order and trade journals, cancel outstanding orders and search orders and trades.

## Features

### Implemented
- **Bid and ask books**: order books with O(log n) insertion and placing time complexity with FIFO mechanism.
- **Order Search in O(1)**: functionalities including order search, trade search and order cancellation are running in O(1) with std::unordered_map.
- **GTC limit order**: Limit orders with GTC rest any unfilled quantity on the book.
- **IOC limit orders**: match immediately; any unfilled quantity is canceled and never rests on the book.
- **FOK limit orders**: fill the entire size immediately or cancel the whole order (no partial fills, no resting).
- **Partial and full fills**: The book records remaining quantity for partial filled orders.
- **Journals**: Order journal (history) and trade journal (fills); live book is the source of truth for resting state.
- **CLI**: Terminal CLI is implemented for user action.

### In progress
- Market, Stop, Stop-Limit order types.
- GFD time in force setting.
- Automatic trade listing.

## Current Architecture

### User-defined Classes

- `Order`: Class for order.
- `OrderBook`: Class for the matching engine.
- `OrderJournal`: Class for the order journal.
- `TradeJournal`: Class for the trade journal.
- `Cli`: Class for the CLI interface.

### Standard Classes

Book classes use an ordered map in order to find the best bid or ask price at an O(1) time complexity, and for a FIFO inserting in O(log n) time complexity in searching price in the map. A list in each price level is used when placing the order into the book.

- `std::map<Price, std::list<Order>, std::greater<Price>>`: Class for the bid book, ordering the highest price at the beginning.
- `std::map<Price, std::list<Order>>`: Class for the ask book.

Searching functionality uses an unordered map to match each order or trade ID for searching them in O(1).

### Data flow

1. `submit_order` creates the order, records it in the order journal, then matches.
2. On a fill: update **live** maker/taker first, then `sync_order_to_journal`, then remove filled makers from the book.
3. Unfilled leftover rests on the bid/ask book.
4. Each fill also creates a trade in the trade journal.

Remark: Cancel updates the live order, syncs the journal, then removes it from the book.

**Live book** = current resting orders. **Order journal** = history snapshots. **Trade journal** = match events.

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