# NSE Futures Contango Scanner

A small C command-line tool that pulls NSE futures chains from the Fyers API and compares prices across contract months, so you can see which stocks are in contango and which ones aren't.

It's a hobby project and still rough around the edges. The scan works, but it's a first-pass filter, not a trading signal (more on that below).

## What it does

- Downloads Fyers' NSE symbol master and keeps a local copy of every instrument that has futures.
- Shows the futures chain for any stock, with the % gap between consecutive contract months.
- Scans every futures-eligible stock and flags the ones where that gap behaves unexpectedly.

## What you'll need

- gcc or clang
- CMake 3.23 or newer
- libcurl and cJSON: `sudo apt install libcurl4-openssl-dev libcjson-dev`
- The Fyers C client, installed so CMake can find it (`find_package(fyers-api-c)`). The binary expects its shared library in `/usr/local/lib`.
- A Fyers API app (client ID, redirect URI and secret key)

## Building

```bash
mkdir -p data db
cmake -B build && cmake --build build
```

The binary lands in `out/`. Heads up: the build turns on AddressSanitizer and UBSan, so it's a debug-friendly build and runs slower than a plain release.

## Running it

```bash
out/cotango-backwardation
```

**First run:** you'll be asked for your Fyers client ID, redirect URI and secret key. After that the tool prints a login URL; sign in, paste the auth code it gives you back, and you're connected. Both the credentials and the access token are saved under `data/` (as plain text, and git-ignored), so later runs skip all of this. If your token expires, delete `data/access_token.txt` and run again.

Then you get a menu:

| Choice | What it does |
|--------|--------------|
| **1. Seed the instrument list** | Downloads the NSE symbol master from Fyers and builds `db/instruments.dat`. Do this first, and again whenever you want fresh symbols. |
| **2. Run contango across all EQs** | Walks every stock with futures, fetches its chain, and prints the ones that look off. Prints the total time at the end. |
| **3. Read the instrument list** | Dumps the futures-eligible instruments from the local db. Handy for checking the seed worked. |
| **4. Check an EQ** | Enter a symbol like `NSE:SBIN-EQ` to see its futures chain (last price, change, volume, expiry) and the % difference between consecutive months. |
| **5. Order any instrument** | Placeholder. It doesn't place orders. |
| **10. Stop** | Exits. |

## About the scan

For each stock it takes the near, next and far month futures and works out the % difference between each pair. In a normal contango curve those gaps grow as you go further out, and the scan prints a line whenever they don't.

A few honest caveats:

- It's a rough heuristic. A stock can show up more than once, and nothing is ranked or saved yet. Treat the output as a list of things to look at, not things to trade.
- It makes one API call per symbol, so a full run takes a while and you may hit Fyers' rate limits.
- It handles up to 10,000 symbols at once, which is plenty for NSE today.
- Stocks with missing or zero prices are skipped.

## Project layout

- `main.c`: menu, symbol master download, instrument db, futures parsing and the scan
- `fyers-session-helpers.c`: credential and access-token handling
- `CMakeLists.txt`: build config
- `data/` and `db/`: created locally for credentials, tokens, the downloaded master and the instrument db (both git-ignored)

## License

[PolyForm Noncommercial 1.0.0](./LICENSE.md). Free for non-commercial use with credit. For commercial use, get in touch with the author.

This isn't financial advice, and it isn't affiliated with Fyers.
