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
| Type   | Buy     | Cur. | Sell   | Cur. | Fee | Cur. | Exchange | Group       | Comment                                                                                                                                               | Date                | 
|--------|---------|------|--------|------|-----|------|----------|-------------|-------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------| 
| Income | 10.0000 | EOS  |        |      |     |      |          |             | 4604626c34f6a72bf9f2ac22252b8bd7db2041fd6091345ed00b92112633efd6 | Transfer From: eosio | To: gi4dcnbug4ge | Quantity: 10.0000 | init                 | 2018-06-09 12:58:53 | 
| Spend  |         |      | 0.0498 | EOS  |     |      |          | EOS RAM     | c2cb29e2ae75916745911a90b944c4b08ecab327e2c48e1d9caa92342ed60eea | Transfer From: gi4dcnbug4ge | To: eosio.ram | Quantity: 0.0498 | buy ram           | 2018-06-10 15:03:32 | 
| Spend  |         |      | 0.0002 | EOS  |     |      |          |             | c2cb29e2ae75916745911a90b944c4b08ecab327e2c48e1d9caa92342ed60eea | Transfer From: gi4dcnbug4ge | To: eosio.ramfee | Quantity: 0.0002 | ram fee        | 2018-06-10 15:03:32 | 
| Spend  |         |      | 0.1000 | EOS  |     |      |          | EOS STAKING | c2cb29e2ae75916745911a90b944c4b08ecab327e2c48e1d9caa92342ed60eea | Transfer From: gi4dcnbug4ge | To: eosio.stake | Quantity: 0.1000 | stake bandwidth | 2018-06-10 15:03:32 | 
| Spend  |         |      | 0.0498 | EOS  |     |      |          | EOS RAM     | a607e596675d3e583025d19ecf597e35544ce953b6a039bd585ccc8b2fcaf46b | Transfer From: gi4dcnbug4ge | To: eosio.ram | Quantity: 0.0498 | buy ram           | 2018-06-10 15:11:46 | 
| Spend  |         |      | 0.0002 | EOS  |     |      |          |             | a607e596675d3e583025d19ecf597e35544ce953b6a039bd585ccc8b2fcaf46b | Transfer From: gi4dcnbug4ge | To: eosio.ramfee | Quantity: 0.0002 | ram fee        | 2018-06-10 15:11:46 | 
| Spend  |         |      | 0.1000 | EOS  |     |      |          | EOS STAKING | a607e596675d3e583025d19ecf597e35544ce953b6a039bd585ccc8b2fcaf46b | Transfer From: gi4dcnbug4ge | To: eosio.stake | Quantity: 0.1000 | stake bandwidth | 2018-06-10 15:11:46 | 
| Spend  |         |      | 0.0498 | EOS  |     |      |          | EOS RAM     | 0785692938ff6c16b3753ff8901941c7fb1af640b777cda42c52484b4cd34154 | Transfer From: gi4dcnbug4ge | To: eosio.ram | Quantity: 0.0498 | buy ram           | 2018-06-10 15:18:33 | 
