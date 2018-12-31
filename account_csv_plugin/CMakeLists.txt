file(GLOB HEADERS "include/eosio/account_csv_plugin/*.hpp")
add_library( account_csv_plugin
             account_csv_plugin.cpp
             ${HEADERS} )

target_link_libraries( account_csv_plugin chain_plugin eosio_chain appbase fc )
target_include_directories( account_csv_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
