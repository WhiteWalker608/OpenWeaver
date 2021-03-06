#ifndef MARLIN_MTEST_NETWORKFIXTURE_HPP
#define MARLIN_MTEST_NETWORKFIXTURE_HPP

#include <marlin/simulator/core/Simulator.hpp>
#include <marlin/simulator/transport/SimulatedTransportFactory.hpp>
#include <marlin/simulator/network/Network.hpp>

#include <sodium.h>

#include "Listener.hpp"

namespace marlin {
namespace mtest {

template<typename NetworkConditionerType = simulator::NetworkConditioner>
struct NetworkFixture : public ::testing::Test {
	simulator::Simulator& simulator = simulator::Simulator::default_instance;

	NetworkConditionerType nc;

	using NetworkType = simulator::Network<NetworkConditionerType>;
	NetworkType network;

	using NetworkInterfaceType = simulator::NetworkInterface<NetworkType>;
	NetworkInterfaceType& i1;
	NetworkInterfaceType& i2;
	NetworkInterfaceType& i3;
	NetworkInterfaceType& i4;
	NetworkInterfaceType& i5;

	uint8_t static_sk1[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk1[crypto_box_PUBLICKEYBYTES];
	uint8_t static_sk2[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk2[crypto_box_PUBLICKEYBYTES];
	uint8_t static_sk3[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk3[crypto_box_PUBLICKEYBYTES];
	uint8_t static_sk4[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk4[crypto_box_PUBLICKEYBYTES];
	uint8_t static_sk5[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk5[crypto_box_PUBLICKEYBYTES];

	NetworkFixture() :
		network(nc),
		i1(network.get_or_create_interface(core::SocketAddress::from_string("192.168.0.1:0"))),
		i2(network.get_or_create_interface(core::SocketAddress::from_string("192.168.0.2:0"))),
		i3(network.get_or_create_interface(core::SocketAddress::from_string("192.168.0.3:0"))),
		i4(network.get_or_create_interface(core::SocketAddress::from_string("192.168.0.4:0"))),
		i5(network.get_or_create_interface(core::SocketAddress::from_string("192.168.0.5:0"))) {
		if(sodium_init() == -1) {
			throw;
		}

		crypto_box_keypair(static_pk1, static_sk1);
		crypto_box_keypair(static_pk2, static_sk2);
		crypto_box_keypair(static_pk3, static_sk3);
		crypto_box_keypair(static_pk4, static_sk4);
		crypto_box_keypair(static_pk5, static_sk5);
	}

	template<typename Delegate>
	using TransportType = simulator::SimulatedTransport<
		simulator::Simulator,
		NetworkInterfaceType,
		Delegate
	>;
	template<typename ListenDelegate, typename TransportDelegate>
	using TransportFactoryType = simulator::SimulatedTransportFactory<
		simulator::Simulator,
		NetworkInterfaceType,
		ListenDelegate,
		TransportDelegate
	>;

	using ListenerType = Listener<NetworkInterfaceType>;
};

using DefaultNetworkFixture = NetworkFixture<>;

} // namespace mtest
} // namespace marlin

#endif // MARLIN_MTEST_NETWORKFIXTURE_HPP
