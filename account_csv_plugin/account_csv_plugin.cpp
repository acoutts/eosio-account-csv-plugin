/*********************
* Andrew Coutts
* Copyright 2018
**********************/

#include <eosio/account_csv_plugin/account_csv_plugin.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/block_state.hpp>
#include <fc/io/json.hpp>
#include <fc/network/url.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/algorithm/string.hpp>
#include <unordered_map>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace eosio {
   static appbase::abstract_plugin& _account_csv_plugin = app().register_plugin<account_csv_plugin>();
   using namespace chain;

   class account_csv_plugin_impl {
   public:
      typedef std::unordered_map<transaction_id_type, std::vector<action>>    action_queue_t;
      static const fc::microseconds                                           max_deserialization_time;

      struct action_notif {
         action_notif (const action& act, const variant& action_data) :account(act.account), name(act.name), authorization(act.authorization), action_data(action_data) {}

         account_name               account;
         action_name                name;
         vector<permission_level>   authorization;
         fc::variant                action_data;
      };

      struct transaction {
        transaction_id_type             tx_id;
        std::vector<action_notif>       actions;
      };

      struct message {
        uint32_t                    block_num;
        fc::time_point              timestamp;
        std::vector<transaction>    transactions;
      };

      chain_plugin*                                                           chain_plug = nullptr;
      fc::optional<boost::signals2::scoped_connection>                        accepted_block_conn;
      fc::optional<boost::signals2::scoped_connection>                        applied_tx_conn;
      fc::optional<boost::signals2::scoped_connection>                        irreversible_block_conn;
      action_queue_t                                                          action_queue;
      std::map<string, std::ofstream*>                                        watch_accounts;
      string                                                                  folder_path;

      // Filter returns true if the watched account is either the sender or receiver of the action
      bool filter_account(const action_trace& act) {
        try {
          if (!act.act.authorization.size()) return false;

          if (watch_accounts.count(act.receipt.receiver.to_string()) || watch_accounts.count(act.act.authorization[0].actor.to_string())) {
            return true;
          } else {
            return false;
          }
        } FC_LOG_AND_RETHROW()
      }

      fc::variant deserialize_action_data(action act) {
        auto& chain = chain_plug->chain();
        auto serializer = chain.get_abi_serializer(act.account, max_deserialization_time);
        FC_ASSERT(serializer.valid() &&
        serializer->get_action_type(act.name) != action_name(),
        "Unable to get abi for account: ${acc}, action: ${a} Not sending notification.",
        ("acc", act.account)("a", act.name));
        return serializer->binary_to_variant(act.name.to_string(), act.data, max_deserialization_time);
      }

      void on_action_trace(const action_trace& act, const transaction_id_type& tx_id) {
        if (filter_account(act)) {
          action_queue[tx_id].push_back(act.act);
          std::string data = "";
          if (!act.act.data.empty()) {
            data = fc::json::to_string(deserialize_action_data(act.act));
          }
          ilog("[on_action_trace] [${txid}] Added trace to queue: ${action} | To: ${to} | From: ${from} | Data: ${data}", ("txid",tx_id.str().c_str())("action",act.act.name.to_string().c_str())("to",act.act.account.to_string().c_str())("from",act.act.authorization[0].actor.to_string().c_str())("data",data.c_str()));
        }

        for (const auto& iline : act.inline_traces) {
          on_action_trace(iline, tx_id);
        }
      }

      void on_applied_tx(const transaction_trace_ptr& trace) {
        if (trace->receipt) {
          // Ignore failed deferred tx that may still send an applied_transaction signal
          if (trace->receipt->status != transaction_receipt_header::executed) {
            return;
          }

          // If we later find that a transaction was failed before it's included in a block, remove its actions from the action queue
          if (trace->failed_dtrx_trace) {
            if (action_queue.count(trace->failed_dtrx_trace->id)) {
              action_queue.erase(action_queue.find(trace->failed_dtrx_trace->id));
              return;
            }
          }

          if (action_queue.count(trace->id)) {
            ilog("[on_applied_tx] FORK WARNING: tx_id ${i} already exists -- removing existing entry before processing new actions", ("i", trace->id));
            ilog("[on_applied_tx] -------------------------------------------------------------------------------------------------------------------------------------------");
            ilog("[on_applied_tx] Previously captured tx action contents (to be removed):");
            auto range = action_queue.find(trace->id);
            for (int i = 0; i < range->second.size(); ++i) {
              std::string data = "";
              if (!range->second.at(i).data.empty() && range->second.at(i).name != N(processpool)) {
                data = fc::json::to_string(deserialize_action_data(range->second.at(i)));
              }
              ilog("[on_applied_tx] [${txid}] Action: ${action} | To: ${to} | From: ${from} | Data: ${data}", ("txid",trace->id.str().c_str())("action",range->second.at(i).name.to_string().c_str())("to",range->second.at(i).account.to_string().c_str())("from",range->second.at(i).authorization[0].actor.to_string().c_str())("data",data.c_str()));
            }
            ilog("[on_applied_tx] ==================================================================");
            ilog("[on_applied_tx] ==================================================================");
            ilog("[on_applied_tx] New trace contents to be processed for this tx:");
            for (auto at : trace->action_traces) {
              std::string data = "";
              if (!at.act.data.empty() && at.act.name != N(processpool)) {
                data = fc::json::to_string(deserialize_action_data(at.act));
              }
              ilog("[on_applied_tx] [${txid}] Action: ${action} | To: ${to} | From: ${from} | Data: ${data}", ("txid",trace->id.str().c_str())("action",at.act.name.to_string().c_str())("to",at.act.account.to_string().c_str())("from",at.act.authorization[0].actor.to_string().c_str())("data",data.c_str()));
            }
            ilog("[on_applied_tx] -------------------------------------------------------------------------------------------------------------------------------------------");
            action_queue.erase(action_queue.find(trace->id));
          }

          for (auto& at : trace->action_traces) {
             on_action_trace(at, trace->id);
          }
        }
      }

      void build_message(const transaction_id_type& tx_id, transaction& tx) {
         ilog("inside build_message - tx_id: ${u}", ("u",tx_id));
         auto range = action_queue.find(tx_id);
         if (range == action_queue.end()) return;

         for (int i = 0; i < range->second.size(); ++i) {
            // ilog("inside build_message for loop on iterator for action_queue range");
            // ilog("iterator range->second.at(i): ${u}", ("u",range->second.at(i).name));
            if (!range->second.at(i).data.empty()) {
              auto act_data = deserialize_action_data(range->second.at(i));
              action_notif notif (range->second.at(i), std::forward<fc::variant>(act_data));
              tx.actions.push_back(notif);
              if (range->second.at(i).name == "transfer") {
                i += 2;
              }
            } else {
              variant dummy;
              action_notif notif (range->second.at(i), dummy);
              tx.actions.push_back(notif);
            }
         }
      }

      void write_transactions(const message& msg) {
        // ilog("Inside write_transaction. Msg: ${i}", ("i", msg));

        for (const auto& tx : msg.transactions) {
          for (const auto& action : tx.actions) {
            if (action.name == "transfer") {
              string from;
              string to;
              double dbl_quantity;
              string quantity;
              string symbol;
              asset qty;
              string buy; // Quantity in the 'buy' column
              string sell; // Quantity in the 'sell' column
              string buy_symbol; // Symbol for 'buy' column
              string sell_symbol; // Symbol for 'sell' column
              string exchange; // 'Exchange' column
              string trade_group; // 'Group' column
              string fee;
              string fee_symbol;

              // Reformat timestamp to expected format "YYYY-MM-DD HH:MM:SS"
              string timestamp = msg.timestamp;
              replace( timestamp.begin(), timestamp.end(), 'T', ' ' );
              timestamp = timestamp.substr(0, timestamp.size() - 4);

              fc::from_variant(action.action_data["from"], from);
              fc::from_variant(action.action_data["to"], to);
              fc::from_variant(action.action_data["quantity"], qty);
              // ilog("precision: ${i}", ("i", qty.precision()));
              // ilog("qty: ${i}", ("i", qty.get_amount()));
              // ilog("decimals: ${i}", ("i", qty.decimals()));
              dbl_quantity = double(qty.get_amount()) / double(qty.precision());
              symbol = qty.get_symbol().name();

              std::stringstream stream;
              stream << std::fixed << std::setprecision(qty.decimals()) << dbl_quantity;
              quantity = stream.str();

              // Parse out memo field and add transaction ID to it for the final comment string
              string memo;
              fc::from_variant(action.action_data["memo"], memo);
              string comment = tx.tx_id.str().c_str();
              comment = comment + " | Transfer From: " + from + " | To: " + to + " | Quantity: " + quantity + " | " + memo;

              for (const auto& account : watch_accounts) {
                if (account.first == from || account.first == to) {
                  string type;
                  if (account.first == from) {
                    type = "Spend";
                    sell = quantity;
                    sell_symbol = symbol;
                    buy = "";
                    buy_symbol = "";

                    // Count as a transfer if it's going to an account we own
                    if (watch_accounts.count(to)) {
                      type = "Withdrawal";
                    }
                  }

                  if (account.first == to) {
                    type = "Income";
                    buy = quantity;
                    buy_symbol = symbol;
                    sell = "";
                    sell_symbol = "";

                    // Count as a transfer if it's coming from an account we own
                    if (watch_accounts.count(from)) {
                      type = "Deposit";
                    }
                  }

                  // Add trade groups for known entities
                  if (from == "newdexpocket" || to == "newdexpocket") {
                    trade_group = "Newdex";
                  }

                  if (from == "betdicestake" || from == "betdicegroup" || from == "betdicelucky" || from == "betdicetoken" || to == "betdicestake" || to == "betdicegroup" || to == "betdicelucky" || to == "betdicetoken") {
                    trade_group = "BetDice";
                  }

                  if (from == "betdividends" || from == "eosbetdice11" || to == "betdividends" || to == "eosbetdice11") {
                    trade_group = "EOSBet";
                  }

                  if (from == "krakenkraken" || to == "krakenkraken") {
                    trade_group = "Kraken";
                  }

                  if (from == "chintailease" || to == "chintailease") {
                    trade_group = "Chintai";
                  }

                  if (from == "dexeoswallet" || to == "dexeoswallet") {
                    trade_group = "DEXEOS";
                  }

                  if (from == "eos42mainacc" || from == "e42opcapital" || to == "eos42mainacc" || to == "e42opcapital") {
                    trade_group = "EOS42";
                  }

                  if (from == "eosio.ram" || to == "eosio.ram") {
                    trade_group = "EOS RAM";
                  }

                  if (from == "eosio.stake" || to == "eosio.stake") {
                    trade_group = "EOS STAKING";
                  }

                  // Now write all of the data to the CSV file
                  *watch_accounts[account.first] << std::endl << "\"" << type << "\",\"" << buy << "\",\"" << buy_symbol << "\",\"" << sell << "\",\"" << sell_symbol << "\",\"" << fee << "\",\"" << fee_symbol << "\",\"" << exchange << "\",\"" << trade_group << "\",\"" << comment << "\",\"" << timestamp << "\"";
                  watch_accounts[account.first]->flush();
                }
              }
            }
          }
        }
      }

      void on_accepted_block(const block_state_ptr& block_state) {
        // Accepted block signal
      }

      // Use irreversible blocks to ensure no avoid having to handle forks
      void on_irreversible_block(const block_state_ptr& block_state) {
        fc::time_point btime = block_state->block->timestamp;
        message msg;
        transaction_id_type tx_id;
        uint64_t block_num = block_state->block->block_num();

        //~ Process transactions from `block_state->block->transactions` because it includes all transactions including deferred ones
        //~ ilog("Looping over all transaction objects in block_state->block->transactions");
        for (const auto& trx : block_state->block->transactions) {
          if (trx.trx.contains<transaction_id_type>()) {
            //~ For deferred transactions the transaction id is easily accessible
            // ilog("Running: trx.trx.get<transaction_id_type>()");
            // ilog("===> block_state->block->transactions->trx ID: ${u}", ("u",trx.trx.get<transaction_id_type>()));
            tx_id = trx.trx.get<transaction_id_type>();
          } else {
            //~ For non-deferred transactions we have to access the txid from within the packed transaction. The `trx` structure and `id()` getter method are defined in `transaction.hpp`
            // ilog("Running: trx.trx.get<packed_transaction>().id()");
            // ilog("===> block_state->block->transactions->trx ID: ${u}", ("u",trx.trx.get<packed_transaction>().id()));
            tx_id = trx.trx.get<packed_transaction>().id();
          }

          if (action_queue.count(tx_id)) {
            ilog("[on_irreversible_block] block_num: ${u}", ("u",block_state->block->block_num()));
            ilog("[on_irreversible_block] Matched TX in accepted block: ${tx}", ("tx",tx_id));
            transaction tx;
            tx.tx_id = tx_id;
            build_message(tx_id, tx);
            msg.transactions.push_back(tx);
            action_queue.erase(action_queue.find(tx_id));
            ilog("[on_irreversible_block] Action queue size after removing item: ${i}", ("i",action_queue.size()));

            msg.block_num = block_num;
            msg.timestamp = btime;
            write_transactions(msg);
          }
        }
      }
    };

   const fc::microseconds account_csv_plugin_impl::max_deserialization_time = fc::seconds(5);

   account_csv_plugin::account_csv_plugin() : my(new account_csv_plugin_impl()){}
   account_csv_plugin::~account_csv_plugin() {}

   void account_csv_plugin::set_program_options(options_description&, options_description& cfg) {
      cfg.add_options()
      ("folder-path", bpo::value<std::string>()->default_value("/tmp/eosio_account_csv_plugin"), "Absolute path to folder where account CSV logs will be saved")
      ("watch-account", bpo::value<vector<string>>()->composing(), "Accounts to export CSV of actions");
   }

   void account_csv_plugin::plugin_initialize(const variables_map& options) {
      try {
        if( options.count("folder-path")) {
           my->folder_path = options.at("folder-path").as<string>();
         }

        if( options.count("watch-account")) {
           auto accounts = options.at("watch-account").as<vector<string>>();
           for( auto& account : accounts ) {
             EOS_ASSERT(account.length() <= 12, fc::invalid_arg_exception, "Invalid account name length for --watch-account ${i}", ("i", account));

             ilog("path: ${i}", ("i", my->folder_path + "/" + account + ".csv"));
             my->watch_accounts[account] = new std::ofstream;
             my->watch_accounts[account]->open(my->folder_path + "/" + account + ".csv", std::ios_base::app);

             // If file is empty, print the CSV header in it
             std::ifstream filestr;
             filestr.open(my->folder_path + "/" + account + ".csv", std::ios::binary);
             filestr.seekg(0, std::ios::end);
             int flength = filestr.tellg();
             filestr.close();

             if (flength == 0) {
               *my->watch_accounts[account] << "\"Type\",\"Buy\",\"Cur.\",\"Sell\",\"Cur.\",\"Fee\",\"Cur.\",\"Exchange\",\"Group\",\"Comment\",\"Date\"";
               my->watch_accounts[account]->flush();
             }
           }
         }

         my->chain_plug = app().find_plugin<chain_plugin>();
         auto& chain = my->chain_plug->chain();
         my->accepted_block_conn.emplace(chain.accepted_block.connect(
            [&](const block_state_ptr& b_state) {
               my->on_accepted_block(b_state);
         }));

         my->applied_tx_conn.emplace(chain.applied_transaction.connect(
            [&](const transaction_trace_ptr& tt) {
               my->on_applied_tx(tt);
            }
        ));

         my->irreversible_block_conn.emplace(chain.irreversible_block.connect(
            [&](const chain::block_state_ptr& b_state) {
              my->on_irreversible_block(b_state);
         }));
      } FC_LOG_AND_RETHROW()
   }

   void account_csv_plugin::plugin_startup() {
   }

   void account_csv_plugin::plugin_shutdown() {
      my->applied_tx_conn.reset();
      my->accepted_block_conn.reset();
      my->irreversible_block_conn.reset();

      for (const auto &file : my->watch_accounts) {
        ilog("Closing file for account: ${i}", ("i", file.first));
        delete file.second;
      }
   }

}

FC_REFLECT(eosio::account_csv_plugin_impl::action_notif, (account)(name)(authorization)(action_data))
FC_REFLECT(eosio::account_csv_plugin_impl::message, (block_num)(timestamp)(transactions))
FC_REFLECT(eosio::account_csv_plugin_impl::transaction, (tx_id)(actions))
