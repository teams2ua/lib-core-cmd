// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from addresses.djinni

#ifndef DJINNI_GENERATED_BITCOINLIKEADDRESS_HPP
#define DJINNI_GENERATED_BITCOINLIKEADDRESS_HPP

#include <cstdint>
#include <string>
#include <vector>
#ifndef LIBCORE_EXPORT
    #if defined(_MSC_VER)
       #include <libcore_export.h>
    #else
       #define LIBCORE_EXPORT
    #endif
#endif

namespace ledger { namespace core { namespace api {

struct BitcoinLikeNetworkParameters;

/** Helper class for manipulating Bitcoin like addresses */
class LIBCORE_EXPORT BitcoinLikeAddress {
public:
    virtual ~BitcoinLikeAddress() {}

    /**
     * Gets the version of the address (P2SH or P2PKH)
     * @return The version of the address
     */
    virtual std::vector<uint8_t> getVersion() = 0;

    /**
     * Gets the raw hash160 of the public key
     * @return The 20 bytes of the public key hash160
     */
    virtual std::vector<uint8_t> getHash160() = 0;

    /**
     * Gets the network parameters used for serializing the address
     * @return The network parameters of the address
     */
    virtual BitcoinLikeNetworkParameters getNetworkParameters() = 0;

    /**
     * Serializes the hash160 into a Base58 encoded address (with checksum)
     * @return The Base58 serialization
     */
    virtual std::string toBase58() = 0;

    /**
     * Serializes the hash160 to a payment uri (i.e bitcoin:16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvM)
     * @return A payment uri to this address
     * toPaymentUri(): string;
     * Checks if the given address is a P2SH address
     * @return True if the version byte matches the P2SH byte version of the address network parameters
     */
    virtual bool isP2SH() = 0;

    /**
     * Checks if the given address is a P2PKH address
     * @return True if the version byte matches the P2PKH byte version of the address network parameters
     */
    virtual bool isP2PKH() = 0;
};

} } }  // namespace ledger::core::api
#endif //DJINNI_GENERATED_BITCOINLIKEADDRESS_HPP
