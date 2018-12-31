# EOSIO Account CSV Plugin
This plugin monitors accounts on EOSIO blockchains to automatically export a CSV file of each account's activity for tax reporting purposes. The CSV format is configured to import directly into cointracking.info to prevent the need to manually input transactions. I created this plugin to automate the current manual process of tracking EOS transactions for anyone who needs to track all of their transactions for tax purposes.

It will also track/report all altcoin transfers and adjust appropriately for that coin's precision to display the correct number of decimal places.

## Building the plugin [Install on your nodeos server]
1. Copy `account_csv_plugin` folder to `<eosio-source-dir>/plugins/` You should now have `<eosio-source-dir>/plugins/account_csv_plugin`
2. Add the following line to `<eosio-source-dir>/plugins/CMakeLists.txt` with other `add_subdirectory` items
  ```
  add_subdirectory(account_csv_plugin)
  ```

3. Add the following line to the bottom of `<eosio-source-dir>/programs/nodeos/CMakeLists.txt`
  ```
  target_link_libraries( nodeos PRIVATE -Wl,${whole_archive_flag} account_csv_plugin -Wl,${no_whole_archive_flag})
  ```

3. Build and install nodeos with `eosio_build.sh` and `eosio_install.sh`. You can also just `cd <eosio-source-dir>/build` and then `sudo make install`

## Nodeos Configuration
Enable this plugin using the `--plugin` option for nodeos or in your config.ini. Use `nodeos --help` to see options used by this plugin.

Monitor multiple accounts using as many `watch-account` options as needed.

### Example
```
# Account CSV plugin
plugin = eosio::account_csv_plugin
watch-account = useraccount1
watch-account = useraccount2
watch-account = couttsandrew

folder-path = /Users/andrew/Projects/eoshome/account_csv_plugin/files
```

### Sample CSV Output
|            |            |      |        |      |     |      |          |       |                                                                                                                                              |                     | 
|------------|------------|------|--------|------|-----|------|----------|-------|----------------------------------------------------------------------------------------------------------------------------------------------|---------------------| 
| Type       | Buy        | Cur. | Sell   | Cur. | Fee | Cur. | Exchange | Group | Comment                                                                                                                                      | Date                | 
| Deposit    | 4.93364564 | ABC  |        |      |     |      |          |       | 2b17cc4739220fecf9df6b0ee3114b9b665a067d42aaf277da329ff4d924c577  Transfer From: couttsandrew  To: useraccount1  Quantity: 4.93364564  hello | 2018-12-31 22:13:00 | 
| Withdrawal |            |      | 1.2236 | EOS  |     |      |          |       | 157cb7ed097ffb30b159ba703b5354f8ad6b590ae7ea386895ac5fe0ec8f4643  Transfer From: useraccount1  To: couttsandrew  Quantity: 1.2236            | 2018-12-31 22:30:10 | 
| Withdrawal |            |      | 1.2236 | EOS  |     |      |          |       | 0b28999adc01770c35f3176e2a6e698e54198e1e820811320789fe53682d6d97  Transfer From: useraccount1  To: useraccount2  Quantity: 1.2236            | 2018-12-31 22:30:15 | 
| Deposit    | 13.3305    | EOS  |        |      |     |      |          |       | 2504ebc3254923cda30aa7e0c99f39dd773a7b2906f9e41b8cdc384db11ae0be  Transfer From: useraccount2  To: useraccount1  Quantity: 13.3305           | 2018-12-31 22:30:36 | 
