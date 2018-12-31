# EOSIO Account CSV Plugin
This plugin is designed to monitor on-chain transfer activity of specified accounts and export in CSV format the transfer activity. The CSV format is designed to fit cointracking.info for tax reporting purposes. There is currently no way to import EOS transactions on cointracking.info so I created this plugin to automate manual recording of transactions.

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

### Example
```
# Account CSV plugin
plugin = eosio::account_csv_plugin
watch-account = useraccount1
watch-account = useraccount2
watch-account = couttsandrew

folder-path = /Users/andrew/Projects/eoshome/account_csv_plugin/files
```
