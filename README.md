# EOSIO Account CSV Plugin
This plugin monitors accounts on EOSIO blockchains to automatically export a CSV file of each account's activity for tax reporting purposes. The CSV format is configured to import directly into cointracking.info to prevent the need to manually input transactions. I created this plugin to automate the current manual process of tracking EOS transactions for anyone who needs to track all of their transactions for tax purposes.

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
