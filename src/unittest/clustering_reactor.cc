#include "unittest/gtest.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/dummy_protocol.hpp"
#include "unittest/unittest_utils.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/directory/manager.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/reactor/metadata.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "concurrency/watchable.hpp"
#include "clustering/reactor/reactor.hpp"

namespace unittest {

namespace {

/* `let_stuff_happen()` delays for some time to let events occur */
void let_stuff_happen() {
    nap(1000);

}

/* This is a cluster that is useful for reactor testing... but doesn't actually
 * have a reactor due to the annoyance of needing the peer ids to create a
 * correct blueprint. */
class reactor_test_cluster_t {
public:
    reactor_test_cluster_t(int port, dummy_underlying_store_t *dummy_underlying_store) :
        connectivity_cluster(),
        message_multiplexer(&connectivity_cluster),

        mailbox_manager_client(&message_multiplexer, 'M'),
        mailbox_manager(&mailbox_manager_client),
        mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager),

        semilattice_manager_client(&message_multiplexer, 'S'),
        semilattice_manager_branch_history(&semilattice_manager_client, branch_history_t<dummy_protocol_t>()),
        semilattice_manager_client_run(&semilattice_manager_client, &semilattice_manager_branch_history),

        directory_manager_client(&message_multiplexer, 'D'),
        directory_manager(&directory_manager_client, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<dummy_protocol_t> > >()),
        directory_manager_client_run(&directory_manager_client, &directory_manager),

        message_multiplexer_run(&message_multiplexer),
        connectivity_cluster_run(&connectivity_cluster, port, &message_multiplexer_run),
        dummy_store_view(dummy_underlying_store, a_thru_z_region())
        { }

    peer_id_t get_me() {
        return connectivity_cluster.get_me();
    }

    connectivity_cluster_t connectivity_cluster;
    message_multiplexer_t message_multiplexer;

    message_multiplexer_t::client_t mailbox_manager_client;
    mailbox_manager_t mailbox_manager;
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run;

    message_multiplexer_t::client_t semilattice_manager_client;
    semilattice_manager_t<branch_history_t<dummy_protocol_t> > semilattice_manager_branch_history;
    message_multiplexer_t::client_t::run_t semilattice_manager_client_run;

    message_multiplexer_t::client_t directory_manager_client;
    directory_readwrite_manager_t<boost::optional<directory_echo_wrapper_t<reactor_business_card_t<dummy_protocol_t> > > > directory_manager;
    message_multiplexer_t::client_t::run_t directory_manager_client_run;

    message_multiplexer_t::run_t message_multiplexer_run;
    connectivity_cluster_t::run_t connectivity_cluster_run;

    dummy_store_view_t dummy_store_view;
};

class test_reactor_t {
public:
    test_reactor_t(reactor_test_cluster_t *r, const blueprint_t<dummy_protocol_t> &initial_blueprint)
        : blueprint_watchable(initial_blueprint),
          reactor(&r->mailbox_manager, r->directory_manager.get_root_view(), r->semilattice_manager_branch_history.get_root_view(), &blueprint_watchable, &r->dummy_store_view)
    { }

    watchable_impl_t<blueprint_t<dummy_protocol_t> > blueprint_watchable;
    reactor_t<dummy_protocol_t> reactor;
};

void run_queries(reactor_test_cluster_t *) {
    //do nothing
}

void runOneShardOnePrimaryOneNodeStartupShutdowntest() {
    int port = 10000 + rand() % 20000;

    dummy_underlying_store_t dummy_underlying_store(a_thru_z_region());

    reactor_test_cluster_t test_cluster(port, &dummy_underlying_store);

    blueprint_t<dummy_protocol_t> blueprint;
    blueprint.add_peer(test_cluster.get_me());
    blueprint.add_role(test_cluster.get_me(), a_thru_z_region(), blueprint_t<dummy_protocol_t>::role_primary);

    test_reactor_t reactor(&test_cluster, blueprint);
    let_stuff_happen();
    run_queries(&test_cluster);
}

TEST(CluteringReactor, OneShardOnePrimaryOneNodeStartupShutdown) {
    run_in_thread_pool(&runOneShardOnePrimaryOneNodeStartupShutdowntest);
}



}   /* anonymous namespace */

} // namespace unittest
