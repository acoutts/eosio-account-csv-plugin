/*********************
* Andrew Coutts
* 2019
**********************/
#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {
  using namespace appbase;

  class account_csv_plugin : public appbase::plugin<account_csv_plugin> {
  public:
    account_csv_plugin();
    virtual ~account_csv_plugin();

    APPBASE_PLUGIN_REQUIRES((chain_plugin))

    virtual void set_program_options(options_description&, options_description& cfg) override;

    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

  private:
    std::unique_ptr<class account_csv_plugin_impl> my;
  };
}
