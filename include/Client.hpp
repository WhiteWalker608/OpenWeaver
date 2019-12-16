#ifndef MARLIN_RELAY_CLIENT_HPP
#define MARLIN_RELAY_CLIENT_HPP

#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>

using namespace marlin;
using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define CLIENT_PUBSUB_PROTOCOL_NUMBER 0x10000002

class Client {
private:
	size_t max_sol_conns = 3;
	uint32_t pubsub_port;

	using PubSubNodeType = marlin::pubsub::PubSubNode<
		Client,
		false,
		false,
		false
	>;

	const uint32_t my_protocol = CLIENT_PUBSUB_PROTOCOL_NUMBER;
	bool is_discoverable = true; // false for client
	PubSubNodeType *ps;
	marlin::beacon::DiscoveryClient<Client> *b;

public:

	Client(
		uint32_t pubsub_port,
		const net::SocketAddress &pubsub_addr,
		const net::SocketAddress &beacon_addr,
		const net::SocketAddress &beacon_server_addr
	) {
		//PROTOCOL HACK
		this->pubsub_port = pubsub_port;

		// setting up pusbub and beacon variables
		ps = new PubSubNodeType(pubsub_addr, max_sol_conns);
		ps->delegate = this;
		b = new DiscoveryClient<Client>(beacon_addr);
		b->is_discoverable = this->is_discoverable;
		b->delegate = this;

		b->start_discovery(beacon_server_addr);
	}


	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {
			std::make_tuple(my_protocol, 0, pubsub_port)
		};
	}

	// relay logic
	void new_peer(
		net::SocketAddress const &addr,
		uint32_t protocol,
		uint16_t
	) {
		{
			if(protocol == RELAY_PUBSUB_PROTOCOL_NUMBER) {
				ps->subscribe(addr);
				ps->add_sol_conn(addr);
			}
		}
	}

	std::vector<std::string> channels = {"eth"};

	void did_unsubscribe(
		PubSubNodeType &,
		std::string channel __attribute__((unused))
	) {
		SPDLOG_DEBUG("Did unsubscribe: {}", channel);
	}

	void did_subscribe(
		PubSubNodeType &,
		std::string channel __attribute__((unused))
	) {
		SPDLOG_DEBUG("Did subscribe: {}", channel);
	}

	void did_recv_message(
		PubSubNodeType &,
		Buffer &&message __attribute__((unused)),
		std::string &channel __attribute__((unused)),
		uint64_t message_id __attribute__((unused))
	) {
		SPDLOG_INFO(
			"Received message {} on channel {}",
			message_id,
			channel
		);
	}

	void manage_subscriptions(
		size_t max_sol_conns,
		typename PubSubNodeType::TransportSet& sol_conns,
		typename PubSubNodeType::TransportSet& sol_standby_conns
	) {
		// TODO: remove comment
		SPDLOG_INFO(
			"manage_subscriptions port: {} sol_conns size: {}",
				pubsub_port,
				sol_conns.size()
		);

		// move some of the subscribers to potential subscribers if oversubscribed
		if (sol_conns.size() >= max_sol_conns) {
			// insert churn algorithm here. need to find a better algorithm to give old bad performers a chance gain. Pick randomly from potential peers?
			// send message to removed and added peers

			auto* toReplaceTransport = sol_conns.find_max_rtt_transport();
			auto* toReplaceWithTransport = sol_standby_conns.find_min_rtt_transport();

			if (toReplaceTransport != nullptr &&
				toReplaceWithTransport != nullptr) {

				SPDLOG_INFO("Moving address: {} from sol_conns to sol_standby_conns",
					toReplaceTransport->dst_addr.to_string()
				);

				// TODO: do away with individual subscribe for each channel
				std::for_each(
					channels.begin(),
					channels.end(),
					[&] (std::string const channel) {
						ps->send_UNSUBSCRIBE(*toReplaceTransport, channel);
					}
				);
				ps->remove_conn(sol_conns, *toReplaceTransport);
				ps->add_sol_standby_conn(*toReplaceTransport);

				SPDLOG_INFO("Moving address: {} from sol_standby_conns to sol_conns",
					toReplaceWithTransport->dst_addr.to_string()
				);

				// TODO: do away with individual unsubscribe for each channel
				std::for_each(
					channels.begin(),
					channels.end(),
					[&] (std::string const channel) {
						ps->send_SUBSCRIBE(*toReplaceWithTransport, channel);
					}
				);
				ps->remove_conn(sol_standby_conns, *toReplaceWithTransport);
				ps->add_sol_conn(*toReplaceWithTransport);
			}
		}

		for (auto* transport : sol_standby_conns) {
			SPDLOG_INFO("STANDBY Sol : {}  rtt: {}", transport->dst_addr.to_string(), transport->get_rtt());
		}

		for (auto* transport : sol_conns) {
			SPDLOG_INFO("Sol : {}  rtt: {}", transport->dst_addr.to_string(), transport->get_rtt());
		}
	}
};

#endif // MARLIN_RELAY_CLIENT_HPP
