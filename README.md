# NSE Futures Contango Scanner

A command-line tool in C that pulls NSE futures chains from the Fyers API and compares prices across contract months to spot contango.

Work in progress: the full-market scan isn't implemented yet.

## Requirements

- gcc or clang
- libcurl and cJSON (`sudo apt install libcurl4-openssl-dev libcjson-dev`)
- A Fyers C client providing `fyers_api.h`
- A Fyers API app and access token, configured in `fyers-session-helpers.c`

## Build

```bash
mkdir -p data db
gcc -o futscan main.c -lcurl -lcjson -lfyers_api
```

## Usage

```bash
./futscan
```

1. **Seed the db** downloads the NSE symbol master from Fyers and writes it to `db/instruments.dat`. Run this first.
2. **Check an EQ** shows the futures chain for a symbol (e.g. `NSE:SBIN-EQ`) and the % difference between consecutive contract months.
3. **Contango scan** across all equities (not implemented yet).
5. **Quit**

## License

[PolyForm Noncommercial 1.0.0](./LICENSE). Free for non-commercial use with credit. For commercial use, contact the author.

Not financial advice. Not affiliated with Fyers.
